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
 
 
#include "../include/reiser4.h"

namespace GParted
{

FS reiser4::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "reiser4" ;
	
	if ( ! system( "which debugfs.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .read = true ;
	
	if ( ! system( "which mkfs.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .create = true ;
	
	if ( ! system( "which fsck.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .check = true ;
	
	/*IT SEEMS RESIZE AND COPY AREN'T IMPLEMENTED YET IN THE TOOLS...
	  SEE http://marc.theaimsgroup.com/?t=109883161600003&r=1&w=2 for more information.. (it seems NameSys is getting commercial? :| )
	//resizing is a delicate process ...
	if ( ! system( "which resizefs.reiser4 1>/dev/null 2>/dev/null" ) && fs .check ) 
	{
		fs .grow = true ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = true ;
	}
	
	//we need to call resize_reiserfs after a copy to get proper used/unused
	if ( ! system( "which cpfs.reiser4 1>/dev/null 2>/dev/null" ) && fs .grow ) 
		fs .copy = true ;
	*/
	return fs ;
}

void reiser4::Set_Used_Sectors( Partition & partition ) 
{
	char c_buf[ 512 ] ;
	FILE *f ;
	
	Glib::ustring output ;
	Sector free_blocks = -1, blocksize = -1 ;

        //get free blocks..
	f = popen( ( "LC_ALL=C debugfs.reiser4 " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//blocksize
		if ( output .find( "blksize" ) < output .length( ) )
			blocksize = atol( (output .substr( output .find( ":" ) +1, output .length( ) ) ) .c_str( ) ) ;
		
		//free blocks
		if ( output .find( "free blocks" ) < output .length( ) )
			free_blocks = atol( (output .substr( output .find( ":" ) +1, output .length( ) ) ) .c_str( ) ) ;
	}
	pclose( f ) ;
	
	if ( free_blocks > -1 && blocksize > -1 )
		partition .Set_Unused( free_blocks * blocksize / 512 ) ;
}

bool reiser4::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkfs.reiser4 -y " + new_partition .partition ) ;
}

bool reiser4::Resize( const Partition & partition_new, bool fill_partition )
{/*
	Glib::ustring str_temp = "LC_NUMERIC=C echo y | resize_reiserfs " + partition_new .partition ;
	
	if ( ! fill_partition )
		str_temp += " -s " + num_to_str( partition_new .Get_Length_MB( ) - cylinder_size ) + "M" ;
	
	return ! Execute_Command( str_temp ) ; */
	return true ;
}

bool reiser4::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
/*	if ( ! Execute_Command( "ntfsclone -f --overwrite " + dest_part_path + " " + src_part_path ) )
	{
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, true ) ;
	}
	
	return false ;*/
	
	return true ;
}

bool reiser4::Check_Repair( const Partition & partition )
{
	return ! Execute_Command( "fsck.reiser4 -y --fix " + partition .partition ) ;
}

int reiser4::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


