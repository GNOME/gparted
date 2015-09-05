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
 
 
#include "../include/jfs.h"

namespace GParted
{

FS jfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_JFS ;

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "jfs_debugfs" ) .empty() ) {
		fs .read = GParted::FS::EXTERNAL ;
	}
	
	if ( ! Glib::find_program_in_path( "jfs_tune" ) .empty() ) {
		fs .read_label = FS::EXTERNAL ;
		fs .write_label = FS::EXTERNAL ;
		fs .read_uuid = FS::EXTERNAL ;
		fs .write_uuid = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "mkfs.jfs" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "jfs_fsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing of jfs requires mount, unmount, check/repair functionality and jfs support in the kernel
	if ( ! Glib::find_program_in_path( "mount" ) .empty()  &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
	     fs .check                                         &&
	     Utils::kernel_supports_fs( "jfs" )                   )
	{
		fs .grow = GParted::FS::EXTERNAL ;
	}

	if ( fs .check )
	{
		fs .move = GParted::FS::GPARTED ;
		fs .copy = GParted::FS::GPARTED ;
	}

	fs .online_read = FS::GPARTED ;
#ifdef ENABLE_ONLINE_RESIZE
	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
		fs .online_grow = fs .grow ;
#endif

	fs .MIN = 16 * MEBIBYTE ;
	
	return fs ;
}

void jfs::set_used_sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "sh -c 'echo dm | jfs_debugfs " + partition.get_path() + "'", output, error, true ) )
	{
		//blocksize
		index = output .find( "Block Size:" ) ;
		if ( index >= output .length() || 
		     sscanf( output .substr( index ) .c_str(), "Block Size: %Ld", &S ) != 1 ) 
			S = -1 ;

		//total blocks
		index = output .find( "dn_mapsize:" ) ;
		if ( index >= output .length() ||
		     sscanf( output .substr( index ) .c_str(), "dn_mapsize: %Lx", &T ) != 1 )
			T = -1 ;

		//free blocks
		index = output .find( "dn_nfree:" ) ;
		if ( index >= output .length() || 
		     sscanf( output .substr( index ) .c_str(), "dn_nfree: %Lx", &N ) != 1 ) 
			N = -1 ;

		if ( T > -1 && N > -1 && S > -1 )
		{
			T = Utils::round( T * ( S / double(partition .sector_size) ) ) ;
			N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;
			partition .set_sector_usage( T, N ) ;
		}
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void jfs::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "jfs_tune -l " + partition .get_path(), output, error, true ) )
	{
		partition.set_filesystem_label( Utils::regexp_label( output, "^Volume label:[\t ]*'(.*)'" ) );
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool jfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "jfs_tune -L \"" + partition.get_filesystem_label() + "\" " + partition.get_path(),
	                          operationdetail, EXEC_CHECK_STATUS );
}

void jfs::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( "jfs_tune -l " + partition .get_path(), output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "^File system UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool jfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "jfs_tune -U random " + partition .get_path(), operationdetail, EXEC_CHECK_STATUS );
}

bool jfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkfs.jfs -q -L \"" + new_partition.get_filesystem_label() + "\" " +
	                          new_partition.get_path(),
	                          operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

bool jfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;

	Glib::ustring mount_point ;
	if ( ! partition_new .busy )
	{
		mount_point = mk_temp_dir( "", operationdetail ) ;
		if ( mount_point .empty() )
			return false ;
		success &= ! execute_command( "mount -v -t jfs " + partition_new .get_path() + " " + mount_point,
		                              operationdetail, EXEC_CHECK_STATUS );
	}
	else
		mount_point = partition_new .get_mountpoint() ;

	if ( success )
	{
		success &= ! execute_command( "mount -v -t jfs -o remount,resize " + partition_new .get_path() + " " + mount_point,
		                              operationdetail, EXEC_CHECK_STATUS );

		if ( ! partition_new .busy )
			success &= ! execute_command( "umount -v " + mount_point, operationdetail, EXEC_CHECK_STATUS );
	}

	if ( ! partition_new .busy )
		rm_temp_dir( mount_point, operationdetail ) ;

	return success ;
}

bool jfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "jfs_fsck -f " + partition.get_path(), operationdetail,
	                               EXEC_CANCEL_SAFE );
	bool success = ( exit_status == 0 || exit_status == 1 );
	set_status( operationdetail, success );
	return success;
}

} //GParted
