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

FS fat32::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "fat32" ;
	fs .read = true ; //provided by libparted
	
	//find out if we can create fat32 filesystems
	if ( ! system( "which mkdosfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = true ;
	
	if ( ! system( "which dosfsck 1>/dev/null 2>/dev/null" ) ) 
		fs .check = true ;
		
	//resizing of start and endpoint are provided by libparted
	fs .grow = true ;
	fs .shrink = true ;
	fs .move = true ;
		
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) ) 
		fs .copy = true ;
	
	fs .MIN = 256 ;
	
	return fs ;
}

void fat32::Set_Used_Sectors( Partition & partition ) 
{
	PedFileSystem *fs = NULL;
	PedConstraint *constraint = NULL;
	PedPartition *c_part = NULL ;
	
	if ( disk )
	{
		c_part = ped_disk_get_partition_by_sector( disk, (partition .sector_end + partition .sector_start) / 2 ) ;
		if ( c_part )
		{
			fs = ped_file_system_open( & c_part ->geom ); 	
					
			if ( fs )
			{
				constraint = ped_file_system_get_resize_constraint ( fs ) ;
				if ( constraint )
				{
					partition .Set_Unused( (partition .sector_end - partition .sector_start) - constraint ->min_size ) ;
					
					ped_constraint_destroy ( constraint );
				}
												
				ped_file_system_close( fs ) ;
			}
		}
	}
}

bool fat32::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkdosfs -F32 " + new_partition .partition ) ;
}

bool fat32::Resize( const Partition & partition_new, bool fill_partition )
{
	//handled in GParted_Core::Resize_Normal_Using_Libparted
	return false ;
}

bool fat32::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	return ! Execute_Command( "LC_NUMERIC=C dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) ;
}

bool fat32::Check_Repair( const Partition & partition )
{
	return ! Execute_Command( "dosfsck -aw " + partition .partition ) ;
}

int fat32::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}

	


} //GParted
