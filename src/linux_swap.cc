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
 
 
#include "../include/linux_swap.h"

namespace GParted
{

FS linux_swap::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "linux-swap" ;
	fs .read = false ; //used/unused isn't relevant for swapspace
	fs .create = true ;
	fs .grow = true ;
	fs .shrink = true ;
	fs .move = true ;
	
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) ) 
		fs .copy = true ;
	
	return fs ;
}

void linux_swap::Set_Used_Sectors( Partition & partition ) 
{
}

bool linux_swap::Create( const Glib::ustring device_path, const Partition & new_partition )
{
	bool return_value = false ;
	
	if ( open_device_and_disk( device_path, device, disk ) )
	{	
		PedPartition *c_part = NULL ;
		PedFileSystemType *fs_type = NULL ;
		PedFileSystem *fs = NULL ;
		
		c_part = ped_disk_get_partition_by_sector( disk, (new_partition .sector_end + new_partition .sector_start) / 2 ) ;
		if ( c_part )
		{
			fs_type = ped_file_system_type_get( "linux-swap" ) ;
			if ( fs_type )
			{
				fs = ped_file_system_create( & c_part ->geom, fs_type, NULL );
				if ( fs )
				{
					if ( ped_partition_set_system(c_part, fs_type ) )
						return_value = Commit( disk ) ;
					
					ped_file_system_close( fs );
				}
			}
		}
		
		close_device_and_disk( device, disk ) ;
	}
		
	return return_value ;
}

bool linux_swap::Resize( const Partition & partition_new, bool fill_partition )
{
	//handled in GParted_Core::Resize_Normal_Using_Libparted
	return false ;
}

bool linux_swap::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	return ! Execute_Command( "LC_NUMERIC=C dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) ;
}

bool linux_swap::Check_Repair( const Partition & partition )
{
	return false ;
}

int linux_swap::get_estimated_time( long MB_to_Consider )
{
	return 1 + MB_to_Consider / 5000 ;
}
	

} //GParted
