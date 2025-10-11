/* Copyright (C) 2011 Curtis Gedak
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

#include "exfat.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


FS exfat::get_filesystem_support()
{
	FS fs( FS_EXFAT );

	fs .busy = FS::GPARTED ;
	fs .copy = FS::GPARTED ;
	fs .move = FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	if (! Glib::find_program_in_path("dump.exfat").empty())
		fs.read = FS::EXTERNAL;

	Glib::ustring output;
	Glib::ustring error;
	if (! Glib::find_program_in_path("mkfs.exfat").empty())
	{
		Utils::execute_command("mkfs.exfat -V", output, error, true);
		if (output.compare(0, 18, "exfatprogs version") == 0)
			fs.create = FS::EXTERNAL;
	}

	if (! Glib::find_program_in_path("tune.exfat").empty())
	{
		fs.read_label = FS::EXTERNAL;
		fs.write_label = FS::EXTERNAL;

		// Get/set exFAT Volume Serial Number support was added to exfatprogs
		// 1.1.0.  Check the help text for the feature before enabling.
		Utils::execute_command("tune.exfat", output, error, true);
		if (error.find("Set volume serial") < error.length())
		{
			fs.read_uuid  = FS::EXTERNAL;
			fs.write_uuid  = FS::EXTERNAL;
		}
	}

	if (! Glib::find_program_in_path("fsck.exfat").empty())
	{
		Utils::execute_command("fsck.exfat -V", output, error, true);
		if (output.compare(0, 18, "exfatprogs version") == 0)
			fs.check = FS::EXTERNAL;
	}

	return fs;
}


void exfat::set_used_sectors(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	Utils::execute_command("dump.exfat " + Glib::shell_quote(partition.get_path()), output, error, true);
	// dump.exfat returns non-zero status for both success and failure.  Instead use
	// non-empty stderr to identify failure.
	if (! error.empty())
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// Example output from exfatprogs >= 1.2.3 (lines of interest only):
	//     # dump.exfat /dev/sdb1
	//     Volume Length(sectors):                  524288
	//     Bytes per Sector:                        512
	//     Sectors per Cluster:                     8
	//     Free Clusters: 	                        65024
	// Example output from exfatprogs <= 1.2.2 (lines of interest only):
	//     # dump.exfat /dev/sdb1
	//     Volume Length(sectors):                  524288
	//     Sector Size Bits:                        9
	//     Sectors per Cluster Bits:                3
	//     Free Clusters: 	                        65019
	//
	// "exFAT file system specification"
	// https://docs.microsoft.com/en-us/windows/win32/fileio/exfat-specification
	// Section 3.1.5 VolumeLength field
	// Section 3.1.14 BytesPerSectorShift field
	// Section 3.1.15 SectorsPerClusterShift field

	// FS size in [FS] sectors
	long long volume_length = -1;
	Glib::ustring::size_type index = output.find("Volume Length(sectors):");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Volume Length(sectors): %lld", &volume_length);

	// FS sector size
	long long bytes_per_sector = -1;
	index = output.find("Bytes per Sector:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Bytes per Sector: %lld", &bytes_per_sector);
	if (bytes_per_sector == -1)
	{
		// FS sector size = 2^(bytes_per_sector_shift)
		long long bytes_per_sector_shift = -1;
		index = output.find("Sector Size Bits:");
		if (index < output.length())
			sscanf(output.substr(index).c_str(), "Sector Size Bits: %lld", &bytes_per_sector_shift);
		if (bytes_per_sector_shift > -1)
			bytes_per_sector = 1 << bytes_per_sector_shift;
	}

	// Cluster size in FS sectors
	long long sectors_per_cluster = -1;
	index = output.find("Sectors per Cluster:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Sectors per Cluster: %lld", &sectors_per_cluster);
	if (sectors_per_cluster == -1)
	{
		// Cluster size in FS sectors = 2^(sectors_per_cluster_shift)
		long long sectors_per_cluster_shift = -1;
		index = output.find("Sector per Cluster bits:");
		if (index < output.length())
			sscanf(output.substr(index).c_str(), "Sector per Cluster bits: %lld", &sectors_per_cluster_shift);
		if (sectors_per_cluster_shift > -1)
			sectors_per_cluster = 1 << sectors_per_cluster_shift;
	}

	// Free clusters
	long long free_clusters = -1;
	index = output.find("Free Clusters:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Free Clusters: %lld", &free_clusters);

	if (volume_length > -1 && bytes_per_sector > -1 && sectors_per_cluster > -1 && free_clusters > -1)
	{
		Byte_Value cluster_size = bytes_per_sector * sectors_per_cluster;
		Sector fs_size = volume_length * bytes_per_sector / partition.sector_size;
		Sector fs_free = free_clusters * cluster_size / partition.sector_size;
		partition.set_sector_usage(fs_size, fs_free);
		partition.fs_block_size = cluster_size;
	}
}


bool exfat::create(const Partition& new_partition, OperationDetail& operationdetail)
{
	return ! operationdetail.execute_command("mkfs.exfat -L " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


void exfat::read_label(Partition& partition)
{
	if (partition.busy)
		// Running tune.exfat on a mounted file system fails so don't try.
		return;

	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("tune.exfat -l " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	partition.set_filesystem_label(Utils::regexp_label(output, "^label: ([^\n]*)"));
}


bool exfat::write_label(const Partition& partition, OperationDetail& operationdetail)
{
	return ! operationdetail.execute_command("tune.exfat -L " +
	                        Glib::shell_quote(partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


void exfat::read_uuid(Partition& partition)
{
	if (partition.busy)
		// Running tune.exfat on a mounted file system fails so don't try.
		return;

	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("tune.exfat -i " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! output.empty())
			partition.push_back_message(error);
		return;
	}

	partition.uuid = serial_to_blkid_uuid(
	                Utils::regexp_label(output, "volume serial : (0x[[:xdigit:]][[:xdigit:]]*)"));
}


bool exfat::write_uuid(const Partition& partition, OperationDetail& operationdetail)
{
	return ! operationdetail.execute_command("tune.exfat -I " + random_serial() + " " +
	                        Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool exfat::check_repair(const Partition& partition, OperationDetail& operationdetail)
{
	return ! operationdetail.execute_command("fsck.exfat -y -v " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


// Private methods

// Reformat exfat printed serial into the same format which blkid reports and GParted
// displays to users.  Returns "" if source is not correctly formatted.
// E.g. "0x772ffe5d" -> "772F-FE5D" or "0x0" -> "0000-0000"
Glib::ustring exfat::serial_to_blkid_uuid(const Glib::ustring& serial)
{
	Glib::ustring verified_serial = Utils::regexp_label(serial, "^(0x[[:xdigit:]][[:xdigit:]]*)$");
	if (verified_serial.empty())
		return verified_serial;

	// verified_serial is formatted with leading "0x" and 1 or more hex digits.
	Glib::ustring temp = Glib::ustring(8, '0') + verified_serial.substr(2);
	size_t len = temp.length();
	Glib::ustring canonical_uuid = temp.substr(len - 8, 4).uppercase() + "-" +
	                               temp.substr(len - 4, 4).uppercase();
	return canonical_uuid;
}


// Generate a random exfat serial.
// E.g. -> "0x772ffe5d"
Glib::ustring exfat::random_serial()
{
	return "0x" + Utils::generate_uuid().substr(0, 8);
}


}  // namespace GParted
