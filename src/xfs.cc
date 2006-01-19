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
		
	if ( ! system( "which xfs_db 1>/dev/null 2>/dev/null" ) ) 
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkfs.xfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which xfs_repair 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing of xfs requires xfs_growfs, xfs_repair and xfs support in the kernel
	if ( ! system( "which xfs_growfs 1>/dev/null 2>/dev/null" ) && fs .check ) 
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
	
	if ( ! system( "which xfsdump xfsrestore 1>/dev/null 2>/dev/null" ) && fs .check && fs .create )
		fs .copy = GParted::FS::EXTERNAL ;
	
	fs .MIN = 32 ;//official minsize = 16MB, but the smallest xfs_repair can handle is 32MB...
	
	return fs ;
}

void xfs::Set_Used_Sectors( Partition & partition ) 
{
	argv .push_back( "xfs_db" ) ;
	argv .push_back( "-c" ) ;
	argv .push_back( "sb 0" ) ;
	argv .push_back( "-c print blocksize" ) ;
	argv .push_back( "-c print fdblocks" ) ;
	argv .push_back( "-r" ) ;
	argv .push_back( partition .partition ) ;

	envp .push_back( "LC_ALL=C" ) ;
	
	try
	{
		Glib::spawn_sync( ".", argv, envp, Glib::SPAWN_SEARCH_PATH, sigc::slot< void >(), &output ) ;
	}
	catch ( Glib::Exception & e )
	{ 
		std::cout << e .what() << std::endl ;
		return ;
	}
	
	//blocksize
	if ( sscanf( output .c_str(), "blocksize = %Ld", &S ) != 1 )
		S = -1 ;

	//free blocks
	output = output .substr( output .find( "fdblocks" ) ) ;
	if ( sscanf( output .c_str(), "fdblocks = %Ld", &N ) != 1 )
		N = -1 ;

	if ( N > -1 && S > -1 )
		partition .Set_Unused( N * S / 512 ) ;
}

bool xfs::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::Get_Filesystem_String( GParted::FS_XFS ) ) ) ) ;
	argv .clear() ;
	argv .push_back( "mkfs.xfs" ) ;
	argv .push_back( "-f" ) ;
	argv .push_back( new_partition .partition ) ;
	if ( ! execute_command( argv, operation_details .back() .sub_details ) )
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
	Glib::ustring TEMP_MP = "/tmp/gparted_tmp_xfs_mountpoint" ;

	//create mountpoint...
	operation_details .back() .sub_details .push_back(
		OperationDetails( String::ucompose( _("create temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
	if ( ! mkdir( TEMP_MP .c_str(), 0 ) )
	{
		operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
		
		//mount partition
		operation_details .back() .sub_details .push_back(
			OperationDetails( String::ucompose( _("mount %1 on %2"), partition_new .partition, TEMP_MP ) ) ) ;
		if ( Utils::mount( partition_new .partition, TEMP_MP, "xfs", error ) )
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
			
			//grow the mounted filesystem..
			operation_details .back() .sub_details .push_back( OperationDetails( _("grow mounted filesystem") ) ) ;
			argv .clear() ;
			argv .push_back( "xfs_growfs" ) ;
			argv .push_back( TEMP_MP ) ;
			if ( ! execute_command( argv, operation_details .back() .sub_details .back() .sub_details ) )
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
				OperationDetails( String::ucompose( _("umount %1"), partition_new .partition ) ) ) ;
			if ( Utils::unmount( partition_new .partition, TEMP_MP, error ) )
			{
				operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
			}
			else
			{
				operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
				operation_details .back() .sub_details .back() .sub_details .push_back(
					OperationDetails( error, OperationDetails::NONE ) ) ;

				return_value = false ;
			}
		}
		else
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
			operation_details .back() .sub_details .back() .sub_details .push_back(
				OperationDetails( error, OperationDetails::NONE ) ) ;
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
{//FIXME
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("copy contents of %1 to %2"), src_part_path, dest_part_path ) ) ) ;
	
	bool return_value = false ;
	Glib::ustring error ;
	Glib::ustring SRC = "/tmp/gparted_tmp_xfs_src_mountpoint" ;
	Glib::ustring DST = "/tmp/gparted_tmp_xfs_dest_mountpoint" ;
	
	mkdir( SRC .c_str(), 0 ) ;
	mkdir( DST .c_str(), 0 ) ;
	
	if (	! Execute_Command( "mkfs.xfs -f " + dest_part_path ) &&
		Utils::mount( src_part_path, SRC, "xfs", error, MS_NOATIME | MS_RDONLY ) &&
		Utils::mount( dest_part_path, DST, "xfs", error ) 
	)
		return_value = ! Execute_Command( "xfsdump -J - " + SRC + " | xfsrestore -J - " + DST ) ;
		
	Utils::unmount( src_part_path, SRC, error ) ;
	Utils::unmount( dest_part_path, DST, error ) ;
	
	rmdir( SRC .c_str() ) ;
	rmdir( DST .c_str() ) ;
		
	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return return_value ;
}

bool xfs::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("check filesystem for errors and (if possible) fix them") ) ) ;
	
	argv .clear() ;
	argv .push_back( "xfs_repair" ) ;
	argv .push_back( "-v" ) ;
	argv .push_back( partition .partition ) ;

	if ( ! execute_command( argv, operation_details .back() .sub_details ) )
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


