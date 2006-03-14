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
	
	if ( ! Glib::find_program_in_path( "dd" ) .empty() && fs .grow )
		fs .copy = GParted::FS::EXTERNAL ;
	
	fs .MIN = 16 * MEBIBYTE ;
	
	return fs ;
}

void jfs::Set_Used_Sectors( Partition & partition ) 
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
			partition .Set_Unused( Utils::Round( N * ( S / 512.0 ) ) ) ;
	}
	else
		partition .error = error ;
}

bool jfs::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::Get_Filesystem_String( GParted::FS_JFS ) ) ) ) ;
	
	if ( ! execute_command( "mkfs.jfs -q " + new_partition .get_path(), operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

bool jfs::Resize( const Partition & partition_new,
		  std::vector<OperationDetails> & operation_details,
		  bool fill_partition )
{
	if ( fill_partition )
		operation_details .push_back( OperationDetails( _("grow filesystem to fill the partition") ) ) ;
	else
		operation_details .push_back( OperationDetails( _("resize the filesystem") ) ) ;
	
	bool return_value = false ;
	Glib::ustring error ;
	Glib::ustring TEMP_MP = Glib::get_tmp_dir() + "/gparted_tmp_jfs_mountpoint" ;
	
	//create mountpoint...
	operation_details .back() .sub_details .push_back(
		OperationDetails( String::ucompose( _("create temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
	if ( ! mkdir( TEMP_MP .c_str(), 0 ) )
	{
		operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;

		//mount partition
		operation_details .back() .sub_details .push_back(
			OperationDetails( String::ucompose( _("mount %1 on %2"), partition_new .get_path(), TEMP_MP ) ) ) ;
		
		if ( ! execute_command( "mount -v -t jfs " + partition_new .get_path() + " " + TEMP_MP,
					operation_details .back() .sub_details .back() .sub_details ) )
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
			
			//remount the partition to resize the filesystem
			operation_details .back() .sub_details .push_back(
				OperationDetails( String::ucompose( _("remount %1 on %2 with the 'resize' flag enabled"),
							partition_new .get_path(), TEMP_MP ) ) ) ;
			
			if ( ! execute_command( "mount -v -t jfs -o remount,resize " + partition_new .get_path() + " " + TEMP_MP,
						operation_details .back() .sub_details .back() .sub_details ) )
			{
				operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
				return_value = true ;
			}
			else
			{
				operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
			}
			
			//and unmount it...
			operation_details .back() .sub_details .push_back(
				OperationDetails( String::ucompose( _("unmount %1"), partition_new .get_path() ) ) ) ;
		
			if ( ! execute_command( "umount -v " + partition_new .get_path(),
						operation_details .back() .sub_details .back() .sub_details ) )
			{
				operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
			}
			else
			{
				operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
				return_value = false ;
			}
		}
		else
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
		}
		
		//remove the mountpoint..
		operation_details .back() .sub_details .push_back(
			OperationDetails( String::ucompose( _("remove temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
		if ( ! rmdir( TEMP_MP .c_str() ) )
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
		}
		else
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
			operation_details .back() .sub_details .back() .sub_details .push_back(
				OperationDetails( Glib::strerror( errno ), OperationDetails::NONE ) ) ;

			return_value = false ;
		}
	}
	else
	{
		operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
		operation_details .back() .sub_details .back() .sub_details .push_back(
			OperationDetails( Glib::strerror( errno ), OperationDetails::NONE ) ) ;
	}
	
	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return return_value ;
}

bool jfs::Copy( const Glib::ustring & src_part_path, 
		const Glib::ustring & dest_part_path,
		std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("copy contents of %1 to %2"), src_part_path, dest_part_path ) ) ) ;
	
	if ( ! execute_command( "dd bs=8192 if=" + src_part_path + " of=" + dest_part_path,
				operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		
		return Resize( Partition( dest_part_path ), operation_details, true ) ; 
	}
	
	operation_details .back() .status = OperationDetails::ERROR ;
	return false ;
}

bool jfs::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("check filesystem for errors and (if possible) fix them") ) ) ;

	exit_status = execute_command( "jfs_fsck -f " + partition .get_path(), operation_details .back() .sub_details ) ;
	if ( exit_status == 0 || exit_status == 1 )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

} //GParted


