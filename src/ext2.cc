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
 
 
#include "../include/ext2.h"

namespace GParted
{

FS ext2::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "ext2" ;
	if ( ! system( "which dumpe2fs 1>/dev/null 2>/dev/null" ) ) 
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkfs.ext2 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which e2fsck 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing is a delicate process ...
	if ( ! system( "which resize2fs 1>/dev/null 2>/dev/null" ) && fs .check != GParted::FS::NONE ) 
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read != GParted::FS::NONE ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}
	
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) && fs .grow != GParted::FS::NONE ) 
		fs .copy = GParted::FS::EXTERNAL ;
		
	return fs ;
}

void ext2::Set_Used_Sectors( Partition & partition ) 
{
	char c_buf[ 512 ] ;
	FILE *f ;
	
	Glib::ustring output ;
	Sector free_blocks = -1, blocksize = -1 ;

        //get free blocks..
	f = popen( ( "LC_ALL=C dumpe2fs -h " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//free blocks
		if ( output .find( "Free blocks" ) < output .length( ) )
			free_blocks = atol( (output .substr( output .find( ":" ) +1, output .length( ) ) ) .c_str( ) ) ;
			
		//blocksize
		if ( output .find( "Block size" ) < output .length( ) )
			blocksize = atol( (output .substr( output .find( ":" ) +1, output .length( ) ) ) .c_str( ) ) ;
	}
	pclose( f ) ;
	
	if ( free_blocks > -1 && blocksize > -1 )
		partition .Set_Unused( free_blocks * blocksize / 512 ) ;
}

bool ext2::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkfs.ext2 " + new_partition .partition ) ;
}

bool ext2::Resize( const Partition & partition_new, bool fill_partition )
{
	Glib::ustring str_temp = "resize2fs " + partition_new .partition ;
	
	if ( ! fill_partition )
		str_temp += " " + num_to_str( partition_new .Get_Length_MB( ) - cylinder_size, true ) + "M" ;
	
	return ! Execute_Command( str_temp ) ;
}

bool ext2::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	if ( ! Execute_Command( "dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) )
	{
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, true ) ;
	}
	
	return false ;
}

bool ext2::Check_Repair( const Partition & partition )
{
	return Execute_Command( "e2fsck -fy " + partition .partition ) <= 1 ;
}

int ext2::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


