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
	partition = filesystem = error = flags = "" ;
	status = GParted::STAT_REAL ;
	type = GParted::UNALLOCATED ;
	partition_number = sector_start = sector_end = sectors_used = sectors_unused = -1;
	color.set( "black" ) ;
	inside_extended = busy = false ;
	logicals .clear( ) ;
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
	this ->inside_extended = inside_extended;
	this ->busy = busy;
}

void Partition::Set_Unused( Sector sectors_unused )
{
	this ->sectors_unused = sectors_unused ;
	this ->sectors_used = ( sectors_unused == -1 ) ? -1 : ( sector_end - sector_start) - sectors_unused ;
}

void Partition::Set_Unallocated( Sector sector_start, Sector sector_end, bool inside_extended )
{
	this ->Set( _("Unallocated"), -1, GParted::UNALLOCATED, "unallocated", sector_start, sector_end , inside_extended, false ); 
	this ->error = this ->flags = "" ;
	this ->status = GParted::STAT_REAL ;
}

void Partition::Update_Number( int new_number )
{   //of course this fails when we have devicenames with numbers over 99
	partition = partition .substr( 0, partition .length( ) - ( partition_number >= 10 ? 2 : 1 ) ) ;	
	this ->partition_number = new_number;
	this ->partition += num_to_str( partition_number ) ;
}

const long Partition::Get_Length_MB( ) const
{
	return Sector_To_MB( sector_end - sector_start ) ;
}

const long Partition::Get_Used_MB( ) const
{ 
	return Sector_To_MB( this ->sectors_used ) ;
}

const long Partition::Get_Unused_MB( ) const
{
	return Get_Length_MB( ) - Get_Used_MB( ) ;
}

Partition::~Partition( )
{
}

} //GParted
