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
 
#include "../include/Partition.h"

namespace GParted
{
	
Partition::Partition( )
{
	Reset( ) ;
}

void Partition::Reset( )
{
	partition = filesystem = error = flags = color_string = "" ; 
	status = GParted::STAT_REAL ;
	type = GParted::UNALLOCATED ;
	partition_number = sector_start = sector_end = sectors_used = sectors_unused = -1;
	color.set( "black" ) ;
	inside_extended = busy = false ;
}

void Partition::Set(	const Glib::ustring & partition,
			const int partition_number,
			const PartitionType type,
			const Glib::ustring & filesystem,
			const Sector & sector_start,
			const Sector & sector_end,
			const bool inside_extended,
			const bool busy )
{
	this ->partition = partition;
	this ->partition_number = partition_number;
	this ->type = type;
	this ->filesystem = filesystem;
	this ->sector_start = sector_start;
	this ->sector_end = sector_end;
	this ->color.set( Get_Color( filesystem ) );
	this ->color_string = Get_Color( filesystem );
	this ->inside_extended = inside_extended;
	this ->busy = busy;
}

void Partition::Set_Used( Sector used ) 
{
	this ->sectors_used = used;
	this ->sectors_unused = ( used == -1 ) ? -1 : ( sector_end - sector_start) - used ;
}

void Partition::Set_Unallocated( Sector sector_start, Sector sector_end, bool inside_extended )
{
	this ->Set( _("Unallocated"), -1, GParted::UNALLOCATED, "unallocated", sector_start, sector_end , inside_extended, false); 
	this ->error = this ->flags = "" ;
	this ->status = GParted::STAT_REAL ;
}

void Partition::Update_Number( int new_number )
{   //of course this fails when we have devicenames with numbers over 99
	partition_number >= 10 ? partition = partition.substr( 0, partition.length() -2 ) : partition = partition.substr( 0, partition.length() -1 ) ;
		
	this ->partition_number = new_number;
	this ->partition += num_to_str( partition_number ) ;
}

long Partition::Get_Length_MB( )
{
	return Sector_To_MB( this ->sector_end - this ->sector_start) ;
}

long Partition::Get_Used_MB( )
{ 
	return Sector_To_MB( this ->sectors_used) ;
}

long Partition::Get_Unused_MB( )
{
	return Get_Length_MB( ) - Get_Used_MB( ) ;
}

Partition::~Partition( )
{
}

} //GParted
