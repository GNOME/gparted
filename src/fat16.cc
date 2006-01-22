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
 
 
#include "../include/fat16.h"

namespace GParted
{

FS fat16::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = GParted::FS_FAT16 ;
		
	//find out if we can create fat16 filesystems
	if ( ! system( "which mkdosfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which dosfsck 1>/dev/null 2>/dev/null" ) ) 
	{
		fs .check = GParted::FS::EXTERNAL ;
		fs .read = GParted::FS::EXTERNAL ;
	}
		
	//resizing of start and endpoint are provided by libparted
	fs .grow = GParted::FS::LIBPARTED ;
	fs .shrink = GParted::FS::LIBPARTED ;
	fs .move = GParted::FS::LIBPARTED ;
		
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) ) 
		fs .copy = GParted::FS::EXTERNAL ;
	
	fs .MIN = 16 ;
	fs .MAX = 4096 ;
	
	return fs ;
}

void fat16::Set_Used_Sectors( Partition & partition ) 
{
	argv .push_back( "dosfsck" ) ;
	argv .push_back( "-a" ) ;
	argv .push_back( "-v" ) ;
	argv .push_back( partition .partition ) ;

	if ( 1 >= execute_command( argv, output ) >= 0 )
	{
		//free clusters
		index = output .find( ",", output .find( partition .partition ) + partition .partition .length() ) +1 ;
		if ( index < output .length() && sscanf( output .substr( index ) .c_str(), "%Ld/%Ld", &S, &N ) == 2 ) 
			N -= S ;
		else
			N = -1 ;

		//bytes per cluster
		index = output .rfind( "\n", output .find( "bytes per cluster" ) ) +1 ;
		if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "%Ld", &S ) != 1 )
			S = -1 ;
	
		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::Round( N * ( S / 512.0 ) ) ) ;
	}
}
	
bool fat16::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::Get_Filesystem_String( GParted::FS_FAT16 ) ) ) ) ;
	argv .clear() ;
	argv .push_back( "mkdosfs" ) ;
	argv .push_back( "-F16" ) ;
	argv .push_back( "-v" ) ;
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

bool fat16::Resize( const Partition & partition_new,
		    std::vector<OperationDetails> & operation_details,
		    bool fill_partition )
{
	//handled in GParted_Core::resize_normal_using_libparted
	return false ;
}

bool fat16::Copy( const Glib::ustring & src_part_path,
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
		return true ;	
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

bool fat16::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("check filesystem for errors and (if possible) fix them") ) ) ;
	
	argv .clear() ;
	argv .push_back( "dosfsck" ) ;
	argv .push_back( "-a" ) ;
	argv .push_back( "-w" ) ;
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


