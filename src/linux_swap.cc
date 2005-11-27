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
	
	if ( ! system( "which mkswap 1>/dev/null 2>/dev/null" ) ) 
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .grow = GParted::FS::EXTERNAL ;
		fs .shrink = GParted::FS::EXTERNAL ;
		fs .move = GParted::FS::EXTERNAL ;
	}
	
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) ) 
		fs .copy = GParted::FS::EXTERNAL ;
	
	return fs ;
}

void linux_swap::Set_Used_Sectors( Partition & partition ) 
{
}

bool linux_swap::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkswap " + new_partition .partition ) ;
}

bool linux_swap::Resize( const Partition & partition_new, bool fill_partition )
{
	return Create( partition_new ) ;
}

bool linux_swap::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	return ! Execute_Command( "LC_NUMERIC=C dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) ;
}

bool linux_swap::Check_Repair( const Partition & partition )
{
	return true ;
}

int linux_swap::get_estimated_time( long MB_to_Consider )
{
	return 1 + MB_to_Consider / 5000 ;
}
	

} //GParted
