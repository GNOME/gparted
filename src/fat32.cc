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
 
 
#include "../include/fat32.h"

namespace GParted
{

FS fat32::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_FAT32 ;
		
	//find out if we can create fat32 filesystems
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
	
	fs .MIN = 32 * MEBIBYTE ; //smaller fs'es will cause windows scandisk to fail..
	
	return fs ;
}

void fat32::set_used_sectors( Partition & partition ) 
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
		partition .messages .push_back( error ) ;
}

bool fat32::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkdosfs -F32 -v " + new_partition .get_path(), operationdetail ) ;
}

bool fat32::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	return true ;
}

bool fat32::copy( const Glib::ustring & src_part_path, 
		  const Glib::ustring & dest_part_path,
		  OperationDetail & operationdetail )
{
	return true ;
}

bool fat32::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "dosfsck -a -w -v " + partition .get_path(), operationdetail ) ;

	return ( exit_status == 0 || exit_status == 1 ) ;
}

} //GParted
