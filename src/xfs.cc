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
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkfs.xfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which xfs_repair 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing of xfs requires xfs_growfs, xfs_repair, mount, umount and xfs support in the kernel
	if ( ! system( "which xfs_growfs mount umount 1>/dev/null 2>/dev/null" ) && fs .check ) 
	{
		Glib::ustring line ;
		std::ifstream input( "/proc/filesystems" ) ;
		while ( input >> line )
			if ( line == "xfs" )
			{
				fs .grow = GParted::FS::EXTERNAL ;
				break ;
			}
		
		input .close( ) ;
	}
	
	if ( ! system( "which xfsdump xfsrestore mount umount 1>/dev/null 2>/dev/null" ) && fs .check && fs .create )
		fs .copy = GParted::FS::EXTERNAL ;
	
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
	f = popen( ( "LC_ALL=C xfs_db -c 'sb 0' -c print -r " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//free blocks
		if ( output .find( "fdblocks" ) < output .length( ) )
			free_blocks = atol( (output .substr( output .find( "=" ) +1, output .length( ) ) ) .c_str( ) ) ;
			
		//blocksize
		if ( output .find( "blocksize" ) < output .length( ) )
			blocksize = atol( (output .substr( output .find( "=" ) +1, output .length( ) ) ) .c_str( ) ) ;
		
	}
	pclose( f ) ;
	
	if ( free_blocks > -1 && blocksize > -1 )
		partition .Set_Unused( free_blocks * blocksize / 512 ) ;
}

bool xfs::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkfs.xfs -f " + new_partition .partition ) ;
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
	bool return_value = false ;
	
	system( "mkdir /tmp/gparted_tmp_xfs_src_mountpoint /tmp/gparted_tmp_xfs_dest_mountpoint" ) ;
	
	if (	! Execute_Command( "mkfs.xfs -f " + dest_part_path ) &&
		! Execute_Command( "mount " + src_part_path + " /tmp/gparted_tmp_xfs_src_mountpoint" ) &&
		! Execute_Command( "mount " + dest_part_path + " /tmp/gparted_tmp_xfs_dest_mountpoint" )
	)
		return_value = ! Execute_Command( "xfsdump -J - /tmp/gparted_tmp_xfs_src_mountpoint | xfsrestore -J - /tmp/gparted_tmp_xfs_dest_mountpoint" ) ;
		
	Execute_Command( "umount " + src_part_path + " " + dest_part_path ) ;
	system( "rmdir /tmp/gparted_tmp_xfs_src_mountpoint /tmp/gparted_tmp_xfs_dest_mountpoint" ) ;
	
	return return_value ;
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


