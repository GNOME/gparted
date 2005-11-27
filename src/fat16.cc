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
	
	fs .filesystem = "fat16" ;
		
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
	char c_buf[ 512 ] ;
	FILE *f ;
	
	Glib::ustring output ;
	Sector free_clusters = -1, bytes_per_cluster = -1 ;

        //get free blocks..
	f = popen( ( "LC_ALL=C echo 2 | dosfsck -v " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//bytes per cluster
		if ( output .find( "bytes per cluster" ) < output .length( ) )
			bytes_per_cluster = atol( output .substr( 0, output .find( "b" ) ) .c_str( )  ) ;
		
		//free clusters
		if ( output .find( partition .partition ) < output .length( ) )
		{
			output = output .substr( output .find( "," ) +2, output .length( ) ) ;
			free_clusters = atol( output .substr( output .find( "/" ) +1, output .find( " " ) ) .c_str( ) ) - atol( output .substr( 0, output .find( "/" ) ) .c_str( ) ) ;
		}
	}
	pclose( f ) ;
	
	if ( free_clusters > -1 && bytes_per_cluster > -1 )
		partition .Set_Unused( free_clusters * bytes_per_cluster / 512 ) ;
}
	
bool fat16::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkdosfs -F16 " + new_partition .partition ) ;
}

bool fat16::Resize( const Partition & partition_new, bool fill_partition )
{
	//handled in GParted_Core::Resize_Normal_Using_Libparted
	return false ;
}

bool fat16::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	return ! Execute_Command( "dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) ;
}

bool fat16::Check_Repair( const Partition & partition )
{
	return ! Execute_Command( "dosfsck -aw " + partition .partition ) ;
}

int fat16::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	
} //GParted


