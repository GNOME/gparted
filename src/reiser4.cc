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
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkfs.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which fsck.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	/*
	 * IT SEEMS RESIZE AND COPY AREN'T IMPLEMENTED YET IN THE TOOLS...
	 * SEE http://marc.theaimsgroup.com/?t=109883161600003&r=1&w=2 for more information.. (it seems NameSys is getting commercial? :| )
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
	return ! Execute_Command( "mkfs.reiser4 --yes " + new_partition .partition ) ;
}

bool reiser4::Resize( const Partition & partition_new, bool fill_partition )
{
	return true ;
}

bool reiser4::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	return true ;
}

bool reiser4::Check_Repair( const Partition & partition )
{
	return ! Execute_Command( "fsck.reiser4 --yes --fix " + partition .partition ) ;
}

int reiser4::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


