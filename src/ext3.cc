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
 
 
#include "../include/ext3.h"

namespace GParted
{

FS ext3::get_filesystem_support( )
{
	FS fs ;
	fs .filesystem = GParted::FS_EXT3 ;
	
	if ( ! Glib::find_program_in_path( "dumpe2fs" ) .empty() )
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "mkfs.ext3" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "e2fsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing is a delicate process ...
	if ( ! Glib::find_program_in_path( "resize2fs" ) .empty() && fs .check )
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}
	
	if ( ! Glib::find_program_in_path( "dd" ) .empty() && fs .grow )
		fs .copy = GParted::FS::EXTERNAL ;
		
	return fs ;
}

void ext3::Set_Used_Sectors( Partition & partition ) 
{
	argv .push_back( "dumpe2fs" ) ;
	argv .push_back( "-h" ) ;
	argv .push_back( partition .partition ) ;

	if ( ! execute_command( argv, output ) )
	{
		index = output .find( "Free blocks:" ) ;
		if ( index >= output .length() ||
		     sscanf( output.substr( index ) .c_str(), "Free blocks: %Ld", &N ) != 1 )   
			N = -1 ;
	
		index = output .find( "Block size:" ) ;
		if ( index >= output.length() || 
		     sscanf( output.substr( index ) .c_str(), "Block size: %Ld", &S ) != 1 )  
			S = -1 ;

		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::Round( N * ( S / 512.0 ) ) ) ;
	}
}

bool ext3::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::Get_Filesystem_String( GParted::FS_EXT3 ) ) ) ) ;
	argv .clear() ;
	argv .push_back( "mkfs.ext3" ) ;
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

bool ext3::Resize( const Partition & partition_new,
		   std::vector<OperationDetails> & operation_details,
		   bool fill_partition )
{
	if ( fill_partition )
		operation_details .push_back( OperationDetails( _("grow filesystem to fill the partition") ) ) ;
	else
		operation_details .push_back( OperationDetails( _("resize the filesystem") ) ) ;

	argv .clear() ;
	argv .push_back( "resize2fs" ) ;
	argv .push_back( partition_new .partition ) ;

	if ( ! fill_partition )
		argv .push_back( Utils::num_to_str( Utils::Round( Utils::sector_to_unit( 
			partition_new .get_length() - cylinder_size, GParted::UNIT_MIB ) ), true ) + "M" ) ; 
		
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

bool ext3::Copy( const Glib::ustring & src_part_path,
		 const Glib::ustring & dest_part_path,
		 std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("copy contents of %1 to %2"), src_part_path, dest_part_path ) ) ) ;
	
	argv .clear() ;
	argv .push_back( "dd" ) ;
	argv .push_back( "bs=8192" ) ;
	argv .push_back( "if=" + src_part_path ) ;
	argv .push_back( "of=" + dest_part_path ) ;

	if ( ! execute_command( argv, operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, operation_details, true ) ; 
	}
	
	operation_details .back() .status = OperationDetails::ERROR ;
	return false ;
}

bool ext3::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("check filesystem for errors and (if possible) fix them") ) ) ;
	
	argv .clear() ;
	argv .push_back( "e2fsck" ) ;
	argv .push_back( "-f" ) ;
	argv .push_back( "-y" ) ;
	argv .push_back( "-v" ) ;
	argv .push_back( partition .partition ) ;

	if ( 1 >= execute_command( argv, operation_details .back() .sub_details ) >= 0 )
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

