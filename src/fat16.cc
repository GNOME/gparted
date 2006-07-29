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

FS fat16::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_FAT16 ;
		
	//find out if we can create fat16 filesystems
	if ( ! Glib::find_program_in_path( "mkdosfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "dosfsck" ) .empty() )
	{
		fs .check = GParted::FS::EXTERNAL ;
		fs .read = GParted::FS::EXTERNAL ;
	}
		
	//resizing of start and endpoint are provided by libparted
	fs .grow = GParted::FS::LIBPARTED ;
	fs .shrink = GParted::FS::LIBPARTED ;
	fs .move = GParted::FS::LIBPARTED ;
		
	fs .copy = GParted::FS::GPARTED ;
	
	fs .MIN = 16 * MEBIBYTE ;
	fs .MAX = 4096 * MEBIBYTE ;
	
	return fs ;
}

void fat16::Set_Used_Sectors( Partition & partition ) 
{
	exit_status = Utils::execute_command( "dosfsck -a -v " + partition .get_path(), output, error, true ) ;
	if ( exit_status == 0 || exit_status == 1 )
	{
		//free clusters
		index = output .find( ",", output .find( partition .get_path() ) + partition .get_path() .length() ) +1 ;
		if ( index < output .length() && sscanf( output .substr( index ) .c_str(), "%Ld/%Ld", &S, &N ) == 2 ) 
			N -= S ;
		else
			N = -1 ;

		//bytes per cluster
		index = output .rfind( "\n", output .find( "bytes per cluster" ) ) +1 ;
		if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "%Ld", &S ) != 1 )
			S = -1 ;
	
		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::round( N * ( S / 512.0 ) ) ) ;
	}
	else
		partition .error = error ;
}
	
bool fat16::Create( const Partition & new_partition, std::vector<OperationDetail> & operation_details )
{
	return ! execute_command( "mkdosfs -F16 -v " + new_partition .get_path(), operation_details ) ;
}

bool fat16::Resize( const Partition & partition_new,
		    std::vector<OperationDetail> & operation_details,
		    bool fill_partition )
{
	return true ;
}

bool fat16::Copy( const Glib::ustring & src_part_path,
		  const Glib::ustring & dest_part_path,
		  std::vector<OperationDetail> & operation_details )
{
	return true ;
}

bool fat16::Check_Repair( const Partition & partition, std::vector<OperationDetail> & operation_details )
{
	exit_status = execute_command( "dosfsck -a -w -v " + partition .get_path(), operation_details ) ;

	return ( exit_status == 0 || exit_status == 1 ) ;
}

} //GParted


