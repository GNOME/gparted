/* Copyright (C) 2013 Patrick Verner <exodusrobot@yahoo.com>
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

#include "f2fs.h"
#include "FileSystem.h"
#include "Partition.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


FS f2fs::get_filesystem_support()
{
	FS fs( FS_F2FS );

	fs .busy = FS::GPARTED ;

	if (! Glib::find_program_in_path("dump.f2fs").empty())
		fs.read = FS::EXTERNAL;

	if ( ! Glib::find_program_in_path( "mkfs.f2fs" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if (! Glib::find_program_in_path("fsck.f2fs").empty())
		fs.check = FS::EXTERNAL;

	if (! Glib::find_program_in_path("resize.f2fs").empty())
		fs.grow = FS::EXTERNAL;

	fs .copy = FS::GPARTED ;
	fs .move = FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	return fs ;
}


void f2fs::set_used_sectors(Partition & partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("dump.f2fs -d 1 " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// used FS blocks
	long long user_block_count = -1;
	Glib::ustring::size_type index = output.find("user_block_count");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "user_block_count [0x %*x : %lld]", &user_block_count);

	// total FS blocks
	long long valid_block_count = -1;
	index = output.find("valid_block_count");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "valid_block_count [0x %*x : %lld]", &valid_block_count);

	// log2 of FS block size
	long long log_blocksize = -1;
	index = output.find("log_blocksize");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "log_blocksize [0x %*x : %lld]", &log_blocksize);

	// FS "sector" size
	long long fs_sector_size = -1;
	index = output.find("Info: sector size =");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Info: sector size = %lld", &fs_sector_size);

	// FS size in "sectors"
	long long total_fs_sectors = -1;
	index = output.find("Info: total FS sectors =");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Info: total FS sectors = %lld", &total_fs_sectors);

	if (user_block_count > -1 && valid_block_count > -1 && log_blocksize > -1)
	{
		long long blocksize  = 1 << log_blocksize;
		long long fs_free_bytes = (user_block_count - valid_block_count) * blocksize;
		Sector fs_free_sectors = fs_free_bytes / partition.sector_size;

		// dump.f2fs < 1.5.0 doesn't report "total FS sectors" so leave
		// fs_size_sectors = -1 for unknown.
		Sector fs_size_sectors = -1;
		if (fs_sector_size > -1 && total_fs_sectors > -1)
		{
			long long fs_size_bytes = total_fs_sectors * fs_sector_size;
			fs_size_sectors = fs_size_bytes / partition.sector_size;
		}

		partition.set_sector_usage(fs_size_sectors, fs_free_sectors);
		partition.fs_block_size = blocksize;
	}
}


bool f2fs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkfs.f2fs -l " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool f2fs::resize(const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition)
{
	Glib::ustring size;
	if (! fill_partition)
		// resize.f2fs works in sector size units of whatever device the file
		// system is currently stored on.
		size = "-t " + Utils::num_to_str(partition_new.get_sector_length()) + " ";

	return ! operationdetail.execute_command("resize.f2fs " + size + Glib::shell_quote(partition_new.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool f2fs::check_repair(const Partition & partition, OperationDetail & operationdetail)
{
	return ! operationdetail.execute_command("fsck.f2fs -f -a " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


}  // namespace GParted
