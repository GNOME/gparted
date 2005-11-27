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
 
 
#include "../include/jfs.h"

namespace GParted
{

FS jfs::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "jfs" ;
		
	if ( ! system( "which jfs_debugfs 1>/dev/null 2>/dev/null" ) ) 
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkfs.jfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which jfs_fsck 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing of jfs requires mount, umount and jfs support in the kernel
	if ( ! system( "which mount umount 1>/dev/null 2>/dev/null" ) ) 
	{
		Glib::ustring line ;
		std::ifstream input( "/proc/filesystems" ) ;
		while ( input >> line )
			if ( line == "jfs" )
			{
				fs .grow = GParted::FS::EXTERNAL ;
				break ;
			}
		
		input .close( ) ;
	}
	
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) && fs .grow != GParted::FS::NONE ) 
		fs .copy = GParted::FS::EXTERNAL ;
	
	fs .MIN = 16 ;
	
	return fs ;
}

void jfs::Set_Used_Sectors( Partition & partition ) 
{
	char c_buf[ 512 ] ;
	FILE *f ;
	
	Glib::ustring output ;

        //get free sectors..
	f = popen( ( "echo dm | LC_ALL=C jfs_debugfs " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//free sectors
		if ( output .find( "dn_nfree" ) < output .length( ) )
		{
			output = output .substr( output .find( ":" ) +1, output .length( ) ) ;
					
			int dec_free_blocks ;
			std::istringstream hex_free_blocks( output .substr( 0, output .find( "[" ) ) );
			hex_free_blocks >> std::hex >> dec_free_blocks ;
				
			partition .Set_Unused( dec_free_blocks * 8 ) ;//4096 / 512 (i've been told jfs blocksize is _always_ 4K)
	
			break ;
		}
	}
	pclose( f ) ;
}

bool jfs::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkfs.jfs -q " + new_partition .partition ) ;
}

bool jfs::Resize( const Partition & partition_new, bool fill_partition )
{
	bool return_value = false ;
	
	//jfs kan only grow if the partition is mounted..
	system( "mkdir /tmp/gparted_tmp_jfs_mountpoint" ) ;
	if ( ! Execute_Command( "mount " + partition_new .partition + " /tmp/gparted_tmp_jfs_mountpoint" ) )
	{
		return_value = ! Execute_Command( "mount -o remount,resize /tmp/gparted_tmp_jfs_mountpoint" ) ;
		Execute_Command( "umount " + partition_new .partition ) ;
	}
	system( "rmdir /tmp/gparted_tmp_jfs_mountpoint" ) ;
	
	return return_value ;
}

bool jfs::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	if ( ! Execute_Command( "dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) )
	{
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, true ) ;
	}
	
	return false ;
}

bool jfs::Check_Repair( const Partition & partition )
{
	return Execute_Command( "jfs_fsck -f " + partition .partition ) <= 1 ;
}

int jfs::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


