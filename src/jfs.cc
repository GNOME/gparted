/* Copyright (C) 2004 Bart
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
 
#include "../include/jfs.h"

#include <cerrno>

namespace GParted
{

FS jfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_JFS ;
		
	if ( ! Glib::find_program_in_path( "jfs_debugfs" ) .empty() )
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "jfs_tune" ) .empty() )
		fs .get_label = FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "mkfs.jfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "jfs_fsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing of jfs requires mount, unmount, check/repair functionality and jfs support in the kernel
	if ( ! Glib::find_program_in_path( "mount" ) .empty() &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
	     fs .check )
	{
		std::ifstream input( "/proc/filesystems" ) ;
		if ( input )
		{
			Glib::ustring line ;

			while ( input >> line )
				if ( line == "jfs" )
				{
					fs .grow = GParted::FS::EXTERNAL ;
					break ;
				}
	
			input .close() ;
		}
	}


	if ( fs .check )
	{
		fs .move = GParted::FS::GPARTED ;
		fs .copy = GParted::FS::GPARTED ;
	}
	
	fs .MIN = 16 * MEBIBYTE ;
	
	return fs ;
}

void jfs::set_used_sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "echo dm | jfs_debugfs " + partition .get_path(), output, error, true ) )
	{
		//blocksize
		index = output .find( "Block Size:" ) ;
		if ( index >= output .length() || 
		     sscanf( output .substr( index ) .c_str(), "Block Size: %Ld", &S ) != 1 ) 
			S = -1 ;
		
		//free blocks
		index = output .find( "dn_nfree:" ) ;
		if ( index >= output .length() || 
		     sscanf( output .substr( index ) .c_str(), "dn_nfree: %Lx", &N ) != 1 ) 
			N = -1 ;

		if ( S > -1 && N > -1 )
			partition .Set_Unused( Utils::round( N * ( S / 512.0 ) ) ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void jfs::get_label( Partition & partition )
{
	if ( ! Utils::execute_command( "jfs_tune -l " + partition .get_path(), output, error, true ) )
	{
		char buf[512] ;
		index = output .find( "Volume label:" ) ;

		if ( index < output .length() && sscanf( output .substr( index ) .c_str(), "Volume label: %512s", buf ) == 1 )
		{
			partition .label = buf ;

			//remove '' from the label..
			if ( partition .label .size() > 0 && partition .label[0] == '\'' )
				partition .label = partition .label .substr( 1 ) ;

			if ( partition .label .size() > 0 && partition .label[ partition .label .size() -1 ] == '\'' )
				partition .label = partition .label .substr( 0, partition .label .size() -1 ) ;
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

bool jfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkfs.jfs -q " + new_partition .get_path(), operationdetail ) ;
}

bool jfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool return_value = false ;
	Glib::ustring error ;
	Glib::ustring TEMP_MP = Glib::get_tmp_dir() + "/gparted_tmp_jfs_mountpoint" ;
	
	//create mountpoint...
	operationdetail .add_child( OperationDetail( String::ucompose( _("create temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
	if ( ! mkdir( TEMP_MP .c_str(), 0 ) )
	{
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;

		//mount partition
		operationdetail .add_child(
			OperationDetail( String::ucompose( _("mount %1 on %2"), partition_new .get_path(), TEMP_MP ) ) ) ;
		
		if ( ! execute_command( "mount -v -t jfs " + partition_new .get_path() + " " + TEMP_MP,
					operationdetail .get_last_child() ) )
		{
			operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
			
			//remount the partition to resize the filesystem
			operationdetail .add_child(
				OperationDetail( String::ucompose( _("remount %1 on %2 with the 'resize' flag enabled"),
								   partition_new .get_path(),
								   TEMP_MP ) ) ) ;
			
			if ( ! execute_command( 
					"mount -v -t jfs -o remount,resize " + partition_new .get_path() + " " + TEMP_MP,
					operationdetail .get_last_child() ) )
			{
				operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
				return_value = true ;
			}
			else
			{
				operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
			}
			
			//and unmount it...
			operationdetail .add_child(
				OperationDetail( String::ucompose( _("unmount %1"), partition_new .get_path() ) ) ) ;
		
			if ( ! execute_command( "umount -v " + partition_new .get_path(),
						operationdetail .get_last_child() ) )
			{
				operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
			}
			else
			{
				operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
				return_value = false ;
			}
		}
		else
		{
			operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
		}
		
		//remove the mountpoint..
		operationdetail .add_child(
			OperationDetail( String::ucompose( _("remove temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
		if ( ! rmdir( TEMP_MP .c_str() ) )
		{
			operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		}
		else
		{
			operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
			operationdetail .get_last_child() .add_child( 
				OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;

			return_value = false ;
		}
	}
	else
	{
		operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
		operationdetail .get_last_child() .add_child( 
			OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;
	}
	
	return return_value ;
}

bool jfs::copy( const Glib::ustring & src_part_path, 
		const Glib::ustring & dest_part_path,
		OperationDetail & operationdetail )
{
	return true ;
}

bool jfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "jfs_fsck -f " + partition .get_path(), operationdetail ) ;

	return ( exit_status == 0 || exit_status == 1 ) ;
}

} //GParted


