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
		
//	if ( ! system( "which xfs_db 1>/dev/null 2>/dev/null" ) ) 
//		fs .read = true ;
	
	if ( ! system( "which mkfs.jfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = true ;
	
	if ( ! system( "which jfs_fsck 1>/dev/null 2>/dev/null" ) ) 
		fs .check = true ;
	
	//resizing of jfs requires mount, umount and jfs support in the kernel
	if ( ! system( "which mount umount 1>/dev/null 2>/dev/null" ) ) 
	{
		Glib::ustring line ;
		std::ifstream input( "/proc/filesystems" ) ;
		while ( input >> line )
			if ( line == "jfs" )
			{
				fs .grow = true ;
				break ;
			}
		
		input .close( ) ;
	}
	
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) && fs .grow) 
		fs .copy = true ;
	
	fs .MIN = 16 ;
	
	return fs ;
}

void jfs::Set_Used_Sectors( Partition & partition ) 
{
}

bool jfs::Create( const Glib::ustring device_path, const Partition & new_partition )
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


