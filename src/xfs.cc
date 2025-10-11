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

#include "xfs.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <sigc++/signal.h>


namespace GParted
{


FS xfs::get_filesystem_support()
{
	FS fs( FS_XFS );

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "xfs_db" ) .empty() ) 	
	{
		fs.read = FS::EXTERNAL;
		fs .read_label = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "xfs_admin" ) .empty() ) 	
	{
		fs .write_label = FS::EXTERNAL ;
		fs .read_uuid = FS::EXTERNAL ;
		fs .write_uuid = FS::EXTERNAL ;
	}

	if (! Glib::find_program_in_path("xfs_io").empty())
	{
		// Check for online label support in xfs_io from xfsprogs >= 4.17 and
		// for kernel >= 4.18 before enabling support.
		Glib::ustring output;
		Glib::ustring error;
		Utils::execute_command("xfs_io -c help", output, error, true);
		if (output.find("\nlabel") < output.length() && Utils::kernel_version_at_least(4, 18, 0))
			fs.online_write_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "mkfs.xfs" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "xfs_repair" ) .empty() ) 	
		fs.check = FS::EXTERNAL;

	//Mounted operations require mount, umount and xfs support in the kernel
	if ( ! Glib::find_program_in_path( "mount" ) .empty()  &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
	     fs .check                                         &&
	     Utils::kernel_supports_fs( "xfs" )                   )
	{
		//Grow
		if ( ! Glib::find_program_in_path( "xfs_growfs" ) .empty() )
			fs .grow = FS::EXTERNAL ;

		//Copy using xfsdump, xfsrestore
		if ( ! Glib::find_program_in_path( "xfsdump" ) .empty()    &&
		     ! Glib::find_program_in_path( "xfsrestore" ) .empty() &&
		     fs .create                                               )
			fs .copy = FS::EXTERNAL ;
	}

	if ( fs .check )
		fs.move = FS::GPARTED;

	fs .online_read = FS::GPARTED ;
	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
		fs .online_grow = fs .grow ;

	// From xfsprogs 5.19.0 the smallest creatable file system is 300 MiB.
	m_fs_limits.min_size = 300 * MEBIBYTE;

	return fs ;
}


void xfs::set_used_sectors(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("xfs_db -r -c 'sb 0' -c 'print blocksize' -c 'print dblocks'"
	                        " -c 'print fdblocks' " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// blocksize
	long long block_size = -1;
	sscanf(output.c_str(), "blocksize = %lld", &block_size);

	// filesystem data blocks
	long long data_blocks = -1;
	Glib::ustring::size_type index = output.find("\ndblocks");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\ndblocks = %lld", &data_blocks);

	// free data blocks
	long long free_data_blocks = -1;
	index = output.find("\nfdblocks");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nfdblocks = %lld", &free_data_blocks);

	if (block_size > -1 && data_blocks > -1 && free_data_blocks > -1)
	{
		Sector fs_size = data_blocks * block_size / partition.sector_size;
		Sector fs_free = free_data_blocks * block_size / partition.sector_size;
		partition.set_sector_usage(fs_size, fs_free);
		partition.fs_block_size = block_size;
	}
}


void xfs::read_label( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "xfs_db -r -c 'label' " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                                )
	{
		partition.set_filesystem_label( Utils::regexp_label( output, "^label = \"(.*)\"" ) );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


bool xfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd;
	bool empty_label = partition.get_filesystem_label().empty();

	if (partition.busy)
	{
		if (empty_label)
			cmd = "xfs_io -c " + Glib::shell_quote("label -c") + " " + partition.get_mountpoint();
		else
			cmd = "xfs_io -c " + Glib::shell_quote("label -s " + partition.get_filesystem_label()) +
			      " " + partition.get_mountpoint();

		operationdetail.execute_command(cmd);
		// In some error situations xfs_io reports exit status zero and writes a
		// failure message to stdout.  Therefore determine success based on the
		// output starting with the fixed text, reporting the new label.
		const Glib::ustring& output = operationdetail.get_command_output();
		bool success = output.compare(0, 9, "label = \"") == 0;
		operationdetail.get_last_child().set_success_and_capture_errors(success);
		return success;
	}
	else
	{
		if (empty_label)
			cmd = "xfs_admin -L -- " + Glib::shell_quote(partition.get_path());
		else
			cmd = "xfs_admin -L " + Glib::shell_quote(partition.get_filesystem_label()) +
			      " " + partition.get_path();

		return ! operationdetail.execute_command(cmd, EXEC_CHECK_STATUS);
	}
}


void xfs::read_uuid( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "xfs_admin -u " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                        )
	{
		partition .uuid = Utils::regexp_label( output, "^UUID[[:blank:]]*=[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


bool xfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("xfs_admin -U generate " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool xfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkfs.xfs -f -L " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


bool xfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;
	int exit_status = 0;
	Glib::ustring mount_point ;
	if ( ! partition_new .busy )
	{
		mount_point = mk_temp_dir( "", operationdetail ) ;
		if ( mount_point.empty() )
			return false ;
		exit_status = operationdetail.execute_command("mount -v -t xfs " +
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
		exit_status = operationdetail.execute_command("xfs_growfs " + Glib::shell_quote(mount_point),
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

bool xfs::copy( const Partition & src_part,
		Partition & dest_part,
		OperationDetail & operationdetail )
{
	bool success = true ;
	int exit_status = operationdetail.execute_command("mkfs.xfs -f -L " +
	                        Glib::shell_quote(dest_part.get_filesystem_label()) +
	                        " -m uuid=" + Glib::shell_quote(dest_part.uuid) +
	                        " " + Glib::shell_quote(dest_part.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
	if (exit_status != 0)
		return false ;

	Glib::ustring src_mount_point = mk_temp_dir( "src", operationdetail ) ;
	if ( src_mount_point .empty() )
		return false ;

	m_dest_mount_point = mk_temp_dir("dest", operationdetail);
	if (m_dest_mount_point.empty())
	{
		rm_temp_dir( src_mount_point, operationdetail ) ;
		return false ;
	}

	exit_status = operationdetail.execute_command("mount -v -t xfs -o noatime,ro " +
	                        Glib::shell_quote(src_part.get_path()) +
	                        " " + Glib::shell_quote(src_mount_point),
	                        EXEC_CHECK_STATUS);
	if (exit_status != 0)
		success = false;

	// Get source FS used bytes, needed in progress update calculation
	Byte_Value fs_size;
	Byte_Value fs_free;
	Glib::ustring error;
	if ( Utils::get_mounted_filesystem_usage( src_mount_point, fs_size, fs_free, error ) == 0 )
		m_src_used = fs_size - fs_free;
	else
		m_src_used = -1LL;

	if ( success )
	{
		exit_status = operationdetail.execute_command("mount -v -t xfs -o nouuid " +
		                        Glib::shell_quote(dest_part.get_path()) +
		                        " " + Glib::shell_quote(m_dest_mount_point),
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;

		if ( success )
		{
			const Glib::ustring copy_cmd = "xfsdump -J - " + Glib::shell_quote( src_mount_point ) +
			                               " | xfsrestore -J - " + Glib::shell_quote(m_dest_mount_point);
			exit_status = operationdetail.execute_command("sh -c " + Glib::shell_quote(copy_cmd),
			                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE|EXEC_PROGRESS_TIMED,
			                        static_cast<TimedSlot>(sigc::mem_fun(*this, &xfs::copy_progress)));
			if (exit_status != 0)
				exit_status = false;

			exit_status = operationdetail.execute_command("umount -v " + Glib::shell_quote(m_dest_mount_point),
			                        EXEC_CHECK_STATUS);
			if (exit_status != 0)
				exit_status = false;
		}

		exit_status = operationdetail.execute_command("umount -v " + Glib::shell_quote(src_mount_point),
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;
	}

	rm_temp_dir(m_dest_mount_point, operationdetail);
	rm_temp_dir( src_mount_point, operationdetail ) ;

	return success ;
}


bool xfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("xfs_repair -v " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


//Private methods

// Report progress of XFS copy.  Monitor destination FS used bytes and track against
// recorded source FS used bytes.
bool xfs::copy_progress( OperationDetail * operationdetail )
{
	if (m_src_used <= 0LL)
	{
		operationdetail->stop_progressbar();
		// Failed to get source FS used bytes.  Remove this timed callback early.
		return false;
	}
	Byte_Value fs_size;
	Byte_Value fs_free;
	Byte_Value dst_used;
	Glib::ustring error;
	if (Utils::get_mounted_filesystem_usage(m_dest_mount_point, fs_size, fs_free, error) != 0)
	{
		operationdetail->stop_progressbar();
		// Failed to get destination FS used bytes.  Remove this timed callback early.
		return false;
	}
	dst_used = fs_size - fs_free;
	operationdetail->run_progressbar((double)dst_used, (double)m_src_used, PROGRESSBAR_TEXT_COPY_BYTES);
	return true;
}


}  // namespace GParted
