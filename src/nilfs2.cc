/* Copyright (C) 2011 Mike Fleetwood
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

#include "nilfs2.h"
#include "FileSystem.h"
#include "Partition.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


FS nilfs2::get_filesystem_support()
{
	FS fs( FS_NILFS2 );

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "mkfs.nilfs2" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "nilfs-tune" ) .empty() )
	{
		fs.read = FS::EXTERNAL;
		fs.read_label = FS::EXTERNAL;
		fs.write_label = FS::EXTERNAL;
		fs.read_uuid = FS::EXTERNAL;
		fs.write_uuid = FS::EXTERNAL;
	}

	//Nilfs2 resizing is an online only operation and needs:
	//  mount, umount, nilfs-resize and linux >= 3.0 with nilfs2 support.
	if ( ! Glib::find_program_in_path( "mount" ) .empty()        &&
	     ! Glib::find_program_in_path( "umount" ) .empty()       &&
	     ! Glib::find_program_in_path( "nilfs-resize" ) .empty() &&
	     Utils::kernel_supports_fs( "nilfs2" )                   &&
	     Utils::kernel_version_at_least( 3, 0, 0 )                  )
	{
		fs.grow = FS::EXTERNAL;
		if ( fs .read ) //needed to determine a minimum file system size.
			fs.shrink = FS::EXTERNAL;
	}

	fs.copy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs .online_read = FS::GPARTED ;
	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
	{
		fs .online_grow = fs .grow ;
		fs .online_shrink = fs .shrink ;
	}

	// Minimum FS size is 128M+4K using mkfs.nilfs2 defaults
	m_fs_limits.min_size = 128 * MEBIBYTE + 4 * KIBIBYTE;

	return fs ;
}


void nilfs2::set_used_sectors(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("nilfs-tune -l " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// File system size in bytes
	long long device_size = -1;
	Glib::ustring::size_type index = output.find("\nDevice size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nDevice size: %lld", &device_size);

	// Free space in blocks
	long long free_blocks = -1;
	index = output.find("\nFree blocks count:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nFree blocks count: %lld", &free_blocks);

	long long block_size = -1;
	index = output.find("\nBlock size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nBlock size: %lld", &block_size);

	if (device_size > -1 && free_blocks > -1 && block_size > -1)
	{
		Sector fs_size = device_size / partition.sector_size;
		Sector fs_free = free_blocks * block_size / partition.sector_size;
		partition.set_sector_usage(fs_size, fs_free);
		partition.fs_block_size = block_size;
	}
}


void nilfs2::read_label( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "nilfs-tune -l " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                         )
	{
		Glib::ustring label = Utils::regexp_label( output, "^Filesystem volume name:[\t ]*(.*)$" ) ;
		if ( label != "(none)" )
			partition.set_filesystem_label( label );
		else
			partition.set_filesystem_label( "" );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


bool nilfs2::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("nilfs-tune -L " +
	                        Glib::shell_quote(partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


void nilfs2::read_uuid( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "nilfs-tune -l " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                         )
	{
		partition .uuid = Utils::regexp_label( output, "^Filesystem UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


bool nilfs2::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("nilfs-tune -U " + Glib::shell_quote(Utils::generate_uuid()) +
	                        " " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool nilfs2::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkfs.nilfs2 -L " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool nilfs2::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;
	int exit_status = 0;
	Glib::ustring mount_point ;
	if ( ! partition_new .busy )
	{
		mount_point = mk_temp_dir( "", operationdetail ) ;
		if ( mount_point .empty() )
			return false ;

		exit_status = operationdetail.execute_command("mount -v -t nilfs2 " +
		                        Glib::shell_quote(partition_new.get_path()) +
		                        " " + Glib::shell_quote(mount_point),
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;
	}

	if ( success )
	{
		Glib::ustring size;
		if ( ! fill_partition )
			size = " " + Utils::num_to_str(partition_new.get_byte_length() / KIBIBYTE) + "K";
		exit_status = operationdetail.execute_command("nilfs-resize -v -y " +
		                        Glib::shell_quote(partition_new.get_path()) + size,
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;

		if ( ! partition_new. busy )
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


}  // namespace GParted
