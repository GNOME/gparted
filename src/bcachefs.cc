/* Copyright (C) 2024 Mike Fleetwood
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include "bcachefs.h"

#include "BlockSpecial.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <errno.h>
#include <fcntl.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <glibmm/stringutils.h>
#include <glibmm/ustring.h>
#include <linux/fs.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>


namespace GParted
{


FS bcachefs::get_filesystem_support()
{
	FS fs(FS_BCACHEFS);

	fs.busy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs.copy = FS::GPARTED;

	// Writing a new label can only be done while mounted.
	if (Utils::kernel_supports_fs("bcachefs")    &&
	    Utils::kernel_version_at_least(6, 11, 0)   )
	{
		fs.online_write_label = FS::EXTERNAL;
	}

	if (! Glib::find_program_in_path("bcachefs").empty())
	{
		fs.online_read       = FS::EXTERNAL;
		fs.create            = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
		fs.read_label        = FS::EXTERNAL;
		fs.read_uuid         = FS::EXTERNAL;
		fs.grow              = FS::EXTERNAL;
		if (Utils::kernel_version_at_least(3, 6, 0))
			fs.online_grow = FS::EXTERNAL;
		fs.check             = FS::EXTERNAL;
	}

	m_fs_limits.min_size = 32 * MEBIBYTE;

	return fs;
}


void bcachefs::set_used_sectors(Partition& partition)
{
	// 'bcachefs fs usage' only reports usage for mounted file systems.
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("bcachefs fs usage " + Glib::shell_quote(partition.get_mountpoint()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// Example output:
	//     # bcachefs fs usage /mnt/1 | egrep ' \(device |free:|capacity:'
	//     (no label) (device 0):          sdb1              rw
	//       free:                    522190848            3984
	//       capacity:               1073741824            8192
	//     (no label) (device 1):          sdb2              rw
	//       free:                   1061945344            8102
	//       capacity:               1073741824            8192
	//
	// Substring the output down to just the device section for this partition.
	BlockSpecial wanted = BlockSpecial(partition.get_path());
	bool found = false;
	Glib::ustring::size_type start_offset = output.find(" (device ");
	while (start_offset != Glib::ustring::npos)
	{
		Glib::ustring device_name = Utils::regexp_label(output.substr(start_offset),
		                                                " \\(device [[:digit:]]+\\):[[:blank:]]+([[:graph:]]+)");
		Glib::ustring::size_type end_offset = output.find(" (device ", start_offset + 9);
		if (wanted == BlockSpecial("/dev/" + device_name))
		{
			output = output.substr(start_offset, end_offset - start_offset);
			found = true;
			break;
		}
		start_offset = end_offset;
	}
	if (! found)
		return;

	// Device specific free space in bytes
	long long dev_free_bytes = -1;
	Glib::ustring::size_type index = output.find("free:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "free: %lld", &dev_free_bytes);

	// Device specific size in bytes
	long long dev_capacity_bytes = -1;
	index = output.find("capacity:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "capacity: %lld", &dev_capacity_bytes);

	if (dev_free_bytes > -1 && dev_capacity_bytes > -1)
	{
		Sector fs_size = dev_capacity_bytes / partition.sector_size;
		Sector fs_free = dev_free_bytes / partition.sector_size;
		partition.set_sector_usage(fs_size, fs_free);
		// Block size is not available in 'bcachefs fs usage' output.
	}
}


bool bcachefs::create(const Partition& new_partition, OperationDetail& operationdetail)
{
	return ! operationdetail.execute_command("bcachefs format -L " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


void bcachefs::read_label(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("bcachefs show-super " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	partition.set_filesystem_label(Utils::regexp_label(output, "^Label:[[:blank:]]*(.*)$"));
}


bool bcachefs::write_label(const Partition& partition, OperationDetail& operationdetail)
{
	operationdetail.add_child(OperationDetail(_("set file system label")));
	OperationDetail& child_od = operationdetail.get_last_child();

	int fd = open(partition.get_mountpoint().c_str(), O_RDONLY);
	if (fd == -1)
	{
		int e = errno;
		child_od.add_child(OperationDetail(Glib::ustring::compose("open(\"%1\", O_RDONLY): %2",
		                                                          partition.get_mountpoint(),
		                                                          Glib::strerror(e)),
		                                   STATUS_NONE,
		                                   FONT_MONOSPACE));
		child_od.set_success_and_capture_errors(false);
		return false;
	}

	bool success = true;
	char label_buf[FSLABEL_MAX+1];
	snprintf(label_buf, sizeof(label_buf), "%s", partition.get_filesystem_label().c_str());
	int status = ioctl(fd, FS_IOC_SETFSLABEL, label_buf);
	if (status == -1)
	{
		int e = errno;
		child_od.add_child(OperationDetail(Glib::ustring::compose("ioctl(%1, FS_IOC_SETFSLABEL, \"%2\"): %3",
		                                                          fd,
		                                                          partition.get_filesystem_label(),
		                                                          Glib::strerror(e)),
		                                   STATUS_NONE,
		                                   FONT_MONOSPACE));
		success = false;
	}

	close(fd);
	child_od.set_success_and_capture_errors(success);
	return success;
}


void bcachefs::read_uuid(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("bcachefs show-super " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	partition.uuid = Utils::regexp_label(output, "^External UUID:  *(" RFC4122_NONE_NIL_UUID_REGEXP ")");
}


bool bcachefs::resize(const Partition& partition_new, OperationDetail& operationdetail, bool fill_partition)
{
	return ! operationdetail.execute_command("bcachefs device resize " +
	                        Glib::shell_quote(partition_new.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool bcachefs::check_repair(const Partition& partition, OperationDetail& operationdetail)
{
	return ! operationdetail.execute_command("bcachefs fsck -f -y -v " +
	                        Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


}  // namespace GParted
