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
 
 
#include "../include/ntfs.h"

namespace GParted
{

FS ntfs::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "ntfs" ;
	if ( ! system( "which ntfscluster 1>/dev/null 2>/dev/null" ) ) 
		fs .read = true ;
	
	if ( ! system( "which mkntfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = true ;
	
	if ( ! system( "which ntfsfix 1>/dev/null 2>/dev/null" ) ) 
		fs .check = true ;
	
	//resizing is a delicate process which requires 3 commands..
	if ( ! system( "which ntfsresize 1>/dev/null 2>/dev/null" ) && fs .read && fs .check ) 
		fs .grow = fs .shrink = true ;
	
	//we need ntfsresize to set correct used/unused after cloning
	if ( ! system( "which ntfsclone 1>/dev/null 2>/dev/null" ) && fs .grow ) 
		fs .copy = true ;
	
	return fs ;
}

void ntfs::Set_Used_Sectors( Partition & partition ) 
{
	char c_buf[ 512 ] ;
	FILE *f ;
	
	Glib::ustring output ;

        //get free sectors..
	f = popen( ( "LANG=C ntfscluster --force " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//free sectors
		if ( output .find( "sectors of free space" ) < output .length( ) )
		{
			partition .Set_Unused( atoi( (output .substr( output .find( ":" ) +1, output .length( ) ) ) .c_str( ) ) ) ;
			break ;
		}
	}
	pclose( f ) ;
}

bool ntfs::Create( const Glib::ustring device_path, const Partition & new_partition )
{
	return ! Execute_Command( "mkntfs -Q " + new_partition .partition ) ;
}

bool ntfs::Resize( const Partition & partition_new, bool fill_partition )
{
	Glib::ustring str_temp = "echo y | ntfsresize -f " + partition_new .partition ;
	
	if ( ! fill_partition )
		str_temp += " -s " + num_to_str( partition_new .Get_Length_MB( ) - cylinder_size ) + "M" ;
	
	return ! Execute_Command( str_temp ) ;
}

bool ntfs::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	bool ret_val = ! Execute_Command( "ntfsclone -f --overwrite " + dest_part_path + " " + src_part_path ) ;
	
	//resize to full to set correct used/unused
	Execute_Command( "echo y | ntfsresize -f " + dest_part_path ) ;
	
	return ret_val ;
}

bool ntfs::Check_Repair( const Partition & partition )
{
	//according to Szaka it's best to use ntfsresize to check the partition for errors
	//since --info is read-only i'll leave it out. just calling ntfsresize --force has also a tendency of fixing stuff :)
	return ! Execute_Command( "echo y | ntfsresize -f " + partition .partition ) ;
}

int ntfs::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


