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
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

# include <glibmm/miscutils.h>
# include <glibmm/shell.h>


namespace GParted
{


FS bcachefs::get_filesystem_support()
{
	FS fs(FS_BCACHEFS);

	fs.busy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs.copy = FS::GPARTED;
	fs.online_read = FS::GPARTED;

	if (! Glib::find_program_in_path("bcachefs").empty())
	{
		fs.online_read       = FS::EXTERNAL;
		fs.create            = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
		fs.read_label        = FS::EXTERNAL;
		fs.read_uuid         = FS::EXTERNAL;
		fs.grow              = FS::EXTERNAL;
#ifdef ENABLE_ONLINE_RESIZE
		if (Utils::kernel_version_at_least(3, 6, 0))
			fs.online_grow = FS::EXTERNAL;
#endif
	}

	fs_limits.min_size = 32 * MEBIBYTE;

	return fs;
}


void bcachefs::set_used_sectors(Partition& partition)
{
	// 'bcachefs fs usage' only reports usage for mounted file systems.
	exit_status = Utils::execute_command("bcachefs fs usage " + Glib::shell_quote(partition.get_mountpoint()),
	                                     output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

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
	return ! execute_command("bcachefs format -L " + Glib::shell_quote(new_partition.get_filesystem_label()) +
	                         " " + Glib::shell_quote(new_partition.get_path()),
				 operationdetail, EXEC_CHECK_STATUS);
}


void bcachefs::read_label(Partition& partition)
{
	exit_status = Utils::execute_command("bcachefs show-super " + Glib::shell_quote(partition.get_path()),
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


void bcachefs::read_uuid(Partition& partition)
{
	exit_status = Utils::execute_command("bcachefs show-super " + Glib::shell_quote(partition.get_path()),
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
	return ! execute_command("bcachefs device resize " + Glib::shell_quote(partition_new.get_path()),
	                         operationdetail, EXEC_CHECK_STATUS);
}


} //GParted
