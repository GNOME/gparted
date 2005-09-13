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
 
 
#include "../include/hfsplus.h"

namespace GParted
{

FS hfsplus::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = "hfs+" ;
	
	fs .read = true ; //provided by libparted
	
	return fs ;
}

void hfsplus::Set_Used_Sectors( Partition & partition ) 
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

bool hfsplus::Create( const Partition & new_partition )
{
	return true ;
}

bool hfsplus::Resize( const Partition & partition_new, bool fill_partition )
{
	return true ;
}

bool hfsplus::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	return true ;
}

bool hfsplus::Check_Repair( const Partition & partition )
{
	return true ;
}

int hfsplus::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


