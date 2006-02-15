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
 
 
#include "../include/xfs.h"

#include <cerrno>

namespace GParted
{

FS xfs::get_filesystem_support( )
{
	FS fs ;
	fs .filesystem = GParted::FS_XFS ;
	
	if ( ! Glib::find_program_in_path( "xfs_db" ) .empty() ) 	
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "mkfs.xfs" ) .empty() ) 	
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "xfs_repair" ) .empty() ) 	
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing of xfs requires xfs_growfs, xfs_repair, mount, umount and xfs support in the kernel
	if ( ! Glib::find_program_in_path( "xfs_growfs" ) .empty() &&
	     ! Glib::find_program_in_path( "mount" ) .empty() &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
    	     fs .check ) 	
	{
		Glib::ustring line ;
		std::ifstream input( "/proc/filesystems" ) ;
		while ( input >> line )
			if ( line == "xfs" )
			{
				fs .grow = GParted::FS::EXTERNAL ;
				break ;
			}
		
		input .close( ) ;
	}
	
	if ( ! Glib::find_program_in_path( "xfsdump" ) .empty() &&
	     ! Glib::find_program_in_path( "xfsrestore" ) .empty() &&
	     ! Glib::find_program_in_path( "mount" ) .empty() &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
	     fs .check && fs .create ) 	
		fs .copy = GParted::FS::EXTERNAL ;
	
	fs .MIN = 32 ;//official minsize = 16MB, but the smallest xfs_repair can handle is 32MB...
	
	return fs ;
}

void xfs::Set_Used_Sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( 
			"xfs_db -c 'sb 0' -c 'print blocksize' -c 'print fdblocks' -r " + partition .partition,
			output,
			error,
			true ) )
	{
		//blocksize
		if ( sscanf( output .c_str(), "blocksize = %Ld", &S ) != 1 )
			S = -1 ;

		//free blocks
		output = output .substr( output .find( "fdblocks" ) ) ;
		if ( sscanf( output .c_str(), "fdblocks = %Ld", &N ) != 1 )
			N = -1 ;

		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::Round( N * ( S / 512.0 ) ) ) ;
	}
}

bool xfs::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::Get_Filesystem_String( GParted::FS_XFS ) ) ) ) ;
	
	if ( ! execute_command( "mkfs.xfs -f " + new_partition .partition, operation_details .back() .sub_details ) )
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

bool xfs::Resize( const Partition & partition_new,
		  std::vector<OperationDetails> & operation_details,
		  bool fill_partition )
{
	if ( fill_partition )
		operation_details .push_back( OperationDetails( _("grow filesystem to fill the partition") ) ) ;
	else
		operation_details .push_back( OperationDetails( _("resize the filesystem") ) ) ;

	bool return_value = false ;
	Glib::ustring error ;
	Glib::ustring TEMP_MP = Glib::get_tmp_dir() + "/gparted_tmp_xfs_mountpoint" ;

	//create mountpoint...
	operation_details .back() .sub_details .push_back(
		OperationDetails( String::ucompose( _("create temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
	if ( ! mkdir( TEMP_MP .c_str(), 0 ) )
	{
		operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
		
		//mount partition
		operation_details .back() .sub_details .push_back(
			OperationDetails( String::ucompose( _("mount %1 on %2"), partition_new .partition, TEMP_MP ) ) ) ;

		if ( ! execute_command( "mount -v -t xfs " + partition_new .partition + " " + TEMP_MP,
					operation_details .back() .sub_details .back() .sub_details ) )
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
			
			//grow the mounted filesystem..
			operation_details .back() .sub_details .push_back( OperationDetails( _("grow mounted filesystem") ) ) ;
			
			if ( ! execute_command ( "xfs_growfs " + TEMP_MP, 
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
				OperationDetails( String::ucompose( _("unmount %1"), partition_new .partition ) ) ) ;

			if ( ! execute_command( "umount -v " + partition_new .partition,
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

bool xfs::Copy( const Glib::ustring & src_part_path,
		const Glib::ustring & dest_part_path,
		std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("copy contents of %1 to %2"), src_part_path, dest_part_path ) ) ) ;
	
	bool return_value = false ;
	Glib::ustring error ;
	Glib::ustring SRC = Glib::get_tmp_dir() + "/gparted_tmp_xfs_src_mountpoint" ;
	Glib::ustring DST = Glib::get_tmp_dir() + "/gparted_tmp_xfs_dest_mountpoint" ;
	
	//create xfs filesystem on destination..
	Partition partition ;
	partition .partition = dest_part_path ;
	if ( Create( partition, operation_details .back() .sub_details ) )
	{
		//create source mountpoint...
		operation_details .back() .sub_details .push_back(
			OperationDetails( String::ucompose( _("create temporary mountpoint (%1)"), SRC ) ) ) ;
		if ( ! mkdir( SRC .c_str(), 0 ) )
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
			
			//create destination mountpoint...
			operation_details .back() .sub_details .push_back(
				OperationDetails( String::ucompose( _("create temporary mountpoint (%1)"), DST ) ) ) ;
			if ( ! mkdir( DST .c_str(), 0 ) )
			{
				operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;

				//mount source partition
				operation_details .back() .sub_details .push_back(
					OperationDetails( String::ucompose( _("mount %1 on %2"), src_part_path, SRC ) ) ) ;
				
				if ( ! execute_command( "mount -v -t xfs -o noatime,ro " + src_part_path + " " + SRC,
							operation_details .back() .sub_details .back() .sub_details ) )
				{
					operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;

					//mount destination partition
					operation_details .back() .sub_details .push_back(
					OperationDetails( String::ucompose( _("mount %1 on %2"), dest_part_path, DST ) ) ) ;
			
					if ( ! execute_command( "mount -v -t xfs " + dest_part_path + " " + DST,
								operation_details .back() .sub_details .back() .sub_details ) )
					{
						operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
	
						//copy filesystem..
						operation_details .back() .sub_details .push_back( OperationDetails( _("copy filesystem") ) ) ;
						
						if ( ! execute_command( 
							 "xfsdump -J - " + SRC + " | xfsrestore -J - " + DST,
							 operation_details .back() .sub_details .back() .sub_details ) )
						{
							operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
							return_value = true ;
						}
						else
						{
							operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
						}
						
						//unmount destination partition
						operation_details .back() .sub_details .push_back(
							OperationDetails( String::ucompose( _("unmount %1"), dest_part_path ) ) ) ;
					
						if ( ! execute_command( "umount -v  " + dest_part_path,
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
					
					//unmount source partition
					operation_details .back() .sub_details .push_back(
						OperationDetails( String::ucompose( _("unmount %1"), src_part_path ) ) ) ;
				
					if ( ! execute_command( "umount -v  " + src_part_path,
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
		
				//remove destination mountpoint..
				operation_details .back() .sub_details .push_back(
					OperationDetails( String::ucompose( _("remove temporary mountpoint (%1)"), DST ) ) ) ;
				if ( ! rmdir( DST .c_str() ) )
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
			
			//remove source mountpoint..
			operation_details .back() .sub_details .push_back(
				OperationDetails( String::ucompose( _("remove temporary mountpoint (%1)"), SRC ) ) ) ;
			if ( ! rmdir( SRC .c_str() ) )
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
	}

	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return return_value ;
}

bool xfs::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("check filesystem for errors and (if possible) fix them") ) ) ;
	
	if ( ! execute_command ( "xfs_repair -v " + partition .partition, operation_details .back() .sub_details ) )
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


