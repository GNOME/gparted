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
 
 
#include "../include/reiserfs.h"

namespace GParted
{

FS reiserfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_REISERFS ;
	
	if ( ! Glib::find_program_in_path( "debugreiserfs" ) .empty() )
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "mkreiserfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "reiserfsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
		
	//resizing is a delicate process ...
	if ( ! Glib::find_program_in_path( "resize_reiserfs" ) .empty() && fs .check )
	{
		fs .grow = GParted::FS::EXTERNAL ;
		fs .copy = GParted::FS::GPARTED ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}
	
	fs .MIN = 32 * MEBIBYTE ;
	
	return fs ;
}

void reiserfs::Set_Used_Sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "debugreiserfs " + partition .get_path(), output, error, true ) )
	{
		index = output .find( "Blocksize" ) ;
		if ( index >= output .length() || 
		     sscanf( output .substr( index ) .c_str(), "Blocksize: %Ld", &S ) != 1 )
			S = -1 ;

		index = output .find( ":", output .find( "Free blocks" ) ) +1 ;
		if ( index >= output .length() ||
		     sscanf( output .substr( index ) .c_str(), "%Ld", &N ) != 1 )
			N = -1 ;

		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::round( N * ( S / 512.0 ) ) ) ;
	}
	else
		partition .error = error ;
}
	
bool reiserfs::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	if ( ! execute_command( "mkreiserfs -f " + new_partition .get_path(), operation_details ) )
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

bool reiserfs::Resize( const Partition & partition_new,
		       std::vector<OperationDetails> & operation_details,
		       bool fill_partition )
{ 
	Glib::ustring str_temp = "echo y | resize_reiserfs " + partition_new .get_path() ;
	
	if ( ! fill_partition )
	{
		str_temp += " -s " ;
		str_temp += Utils::num_to_str( Utils::round( Utils::sector_to_unit(
				partition_new .get_length() - cylinder_size, GParted::UNIT_BYTE ) ), true ) ;
	}

	exit_status = execute_command( str_temp, operation_details ) ;
	if ( exit_status == 0 || exit_status == 256 )
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

bool reiserfs::Copy( const Glib::ustring & src_part_path,
		     const Glib::ustring & dest_part_path,
		     std::vector<OperationDetails> & operation_details )
{	
	return true ;
}

bool reiserfs::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	exit_status = execute_command( "reiserfsck --y --fix-fixable " + partition .get_path(), operation_details ) ;
	if ( exit_status == 0 || exit_status == 1 || exit_status == 256 )
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
