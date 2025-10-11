/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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

#include "jfs.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <stdio.h>
#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


FS jfs::get_filesystem_support()
{
	FS fs( FS_JFS );

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "jfs_debugfs" ) .empty() ) {
		fs.read = FS::EXTERNAL;
		fs.online_read = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "jfs_tune" ) .empty() ) {
		fs .read_label = FS::EXTERNAL ;
		fs .write_label = FS::EXTERNAL ;
		fs .read_uuid = FS::EXTERNAL ;
		fs .write_uuid = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "mkfs.jfs" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "jfs_fsck" ) .empty() )
		fs.check = FS::EXTERNAL;

	//resizing of jfs requires mount, unmount, check/repair functionality and jfs support in the kernel
	if ( ! Glib::find_program_in_path( "mount" ) .empty()  &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
	     fs .check                                         &&
	     Utils::kernel_supports_fs( "jfs" )                   )
	{
		fs.grow = FS::EXTERNAL;
	}

	if ( fs .check )
	{
		fs.move = FS::GPARTED;
		fs.copy = FS::GPARTED;
	}

	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
		fs .online_grow = fs .grow ;

	m_fs_limits.min_size = 16 * MEBIBYTE;

	return fs ;
}


void jfs::set_used_sectors( Partition & partition ) 
{
	// Called when the file system is unmounted *and* when mounted.  Always reads the
	// on disk superblock using jfs_debugfs to accurately calculate the overall size
	// of the file system.  Read free space from the kernel via statvfs() when mounted
	// for the up to date figure, and from the on disk Aggregate Disk Map (dmap) when
	// unmounted.
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("jfs_debugfs " + Glib::shell_quote(partition.get_path()),
	                        "superblock\nx\ndmap\nx\nquit\n", output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// s_size - Aggregate size in device (s_pbsize) blocks
	long long agg_size = -1;
	Glib::ustring::size_type index = output.find("s_size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "s_size: %llx", &agg_size);

	// s_bsize - Aggregate block (aka file system allocation) size in bytes
	long long agg_block_size = -1;
	index = output.find("s_bsize:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "s_bsize: %lld", &agg_block_size);

	// s_pbsize - Physical (device) block size in bytes
	long long phys_block_size = -1;
	index = output.find("s_pbsize:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "s_pbsize: %lld", &phys_block_size);

	// s_logpxd.len - Log (Journal) size in Aggregate (s_bsize) blocks
	long long log_len = -1;
	index = output.find("s_logpxd.len:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "s_logpxd.len: %lld", &log_len);

	// s_fsckpxd.len - FSCK Working Space in Aggregate (s_bsize) blocks
	long long fsck_len = -1;
	index = output.find("s_fsckpxd.len:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "s_fsckpxd.len: %lld", &fsck_len);

	Sector fs_size_sectors = -1;
	if (agg_size > -1 && agg_block_size > -1 && phys_block_size > -1 && log_len > -1 && fsck_len > -1)
	{
		fs_size_sectors = (  agg_size * phys_block_size
		                   + log_len  * agg_block_size
		                   + fsck_len * agg_block_size) / partition.sector_size;
	}

	Sector fs_free_sectors = -1;
	if (partition.busy)
	{
		Byte_Value ignored;
		Byte_Value fs_free_bytes;
		if (Utils::get_mounted_filesystem_usage(partition.get_mountpoint(), ignored, fs_free_bytes, error) != 0)
		{
			partition.push_back_message(error);
			return;
		}

		fs_free_sectors = fs_free_bytes / partition.sector_size;
	}
	else // (! partition.busy)
	{
		// dn_nfree - Number of free (s_bsize) blocks in Aggregate
		long long free_blocks = -1;
		index = output.find("dn_nfree:");
		if (index < output.length())
			sscanf(output.substr(index).c_str(), "dn_nfree: %llx", &free_blocks);

		if (agg_block_size && free_blocks > -1)
		{
			fs_free_sectors = free_blocks * agg_block_size / partition.sector_size;
		}
	}

	if (fs_size_sectors > -1 && fs_free_sectors > -1 && agg_block_size > -1)
	{
		partition.set_sector_usage(fs_size_sectors, fs_free_sectors);
		partition.fs_block_size = agg_block_size;
	}
}


void jfs::read_label( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "jfs_tune -l " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                       )
	{
		partition.set_filesystem_label( Utils::regexp_label( output, "^Volume label:[\t ]*'(.*)'" ) );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


bool jfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("jfs_tune -L " + Glib::shell_quote(partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


void jfs::read_uuid( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "jfs_tune -l " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                       )
	{
		partition .uuid = Utils::regexp_label( output, "^File system UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


bool jfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("jfs_tune -U random " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool jfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkfs.jfs -q -L " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


bool jfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;
	int exit_status = 0;
	Glib::ustring mount_point ;
	if ( ! partition_new .busy )
	{
		mount_point = mk_temp_dir( "", operationdetail ) ;
		if ( mount_point .empty() )
			return false ;
		exit_status = operationdetail.execute_command("mount -v -t jfs " +
		                        Glib::shell_quote(partition_new.get_path()) +
		                        " " + Glib::shell_quote(mount_point),
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;
	}
	else
		mount_point = partition_new .get_mountpoint() ;

	if ( success )
	{
		exit_status = operationdetail.execute_command("mount -v -t jfs -o remount,resize " +
		                        Glib::shell_quote(partition_new.get_path()) +
		                        " " + Glib::shell_quote(mount_point),
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;

		if ( ! partition_new .busy )
		{
			exit_status = operationdetail.execute_command("umount -v " + Glib::shell_quote(mount_point),
			                        EXEC_CHECK_STATUS);
			if (exit_status != 0)
				success = false;
		}
	}

	if ( ! partition_new .busy )
		rm_temp_dir( mount_point, operationdetail ) ;

	return success ;
}


bool jfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	int exit_status = operationdetail.execute_command("jfs_fsck -f " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CANCEL_SAFE);
	// From jfs_fsck(8) manual page:
	//     EXIT CODE
	//         The exit code returned by jfs_fsck represents one of the following
	//         conditions:
	//             0   No errors
	//             1   File system errors corrected and/or transaction log replayed
	//                 successfully
	//             2   File system errors corrected, system should be rebooted
	bool success = (exit_status == 0 || exit_status == 1);
	operationdetail.get_last_child().set_success_and_capture_errors(success);
	return success;
}


}  // namespace GParted
