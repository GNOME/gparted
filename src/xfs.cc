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
 
 
#include "../include/xfs.h"

namespace GParted
{

FS xfs::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "xfs" ;
		
	if ( ! system( "which xfs_db 1>/dev/null 2>/dev/null" ) ) 
		fs .read = true ;
	
	if ( ! system( "which mkfs.xfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = true ;
	
	if ( ! system( "which xfs_repair 1>/dev/null 2>/dev/null" ) ) 
		fs .check = true ;
	
	//resizing of xfs requires xfs_growfs, xfs_repair, mount, umount and xfs support in the kernel
	if ( ! system( "which xfs_growfs mount umount 1>/dev/null 2>/dev/null" ) && fs .check ) 
	{
		Glib::ustring line ;
		std::ifstream input( "/proc/filesystems" ) ;
		while ( input >> line )
			if ( line == "xfs" )
			{
				fs .grow = true ;
				break ;
			}
		
		input .close( ) ;
	}
	
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) && fs .grow ) 
		fs .copy = true ;
	
	fs .MIN = 32 ;//official minsize = 16MB, but the smallest xfs_repair can handle is 32MB...
	
	return fs ;
}

void xfs::Set_Used_Sectors( Partition & partition ) 
{
	char c_buf[ 512 ] ;
	FILE *f ;
	
	Glib::ustring output ;
	Sector free_blocks = -1, blocksize = -1 ;

        //get free blocks..
	f = popen( ( "LANG=C xfs_db -c 'sb 0' -c print -r " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//free blocks
		if ( output .find( "fdblocks" ) < output .length( ) )
			free_blocks = atoi( (output .substr( output .find( "=" ) +1, output .length( ) ) ) .c_str( ) ) ;
			
		//blocksize
		if ( output .find( "blocksize" ) < output .length( ) )
			blocksize = atoi( (output .substr( output .find( "=" ) +1, output .length( ) ) ) .c_str( ) ) ;
		
	}
	pclose( f ) ;
	
	if ( free_blocks > -1 && blocksize > -1 )
		partition .Set_Unused( free_blocks * blocksize / 512 ) ;
}

bool xfs::Create( const Glib::ustring device_path, const Partition & new_partition )
{
	return ! Execute_Command( "mkfs.xfs " + new_partition .partition ) ;
}

bool xfs::Resize( const Partition & partition_new, bool fill_partition )
{
	bool return_value = false ;
	
	//xfs kan only grow if the partition is mounted..
	system( "mkdir /tmp/gparted_tmp_xfs_mountpoint" ) ;
	if ( ! Execute_Command( "mount " + partition_new .partition + " /tmp/gparted_tmp_xfs_mountpoint" ) )
	{
		return_value = ! Execute_Command( "xfs_growfs /tmp/gparted_tmp_xfs_mountpoint" ) ;
		Execute_Command( "umount " + partition_new .partition ) ;
	}
	system( "rmdir /tmp/gparted_tmp_xfs_mountpoint" ) ;
	
	return return_value ;
}

bool xfs::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	if ( ! Execute_Command( "dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) )
	{
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, true ) ;
	}
	
	return false ;
}

bool xfs::Check_Repair( const Partition & partition )
{
	return ! Execute_Command( "xfs_repair " + partition .partition ) ;
}

int xfs::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


