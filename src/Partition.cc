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
	partition = error = flags = mountpoint = "" ;
	status = GParted::STAT_REAL ;
	type = GParted::TYPE_UNALLOCATED ;
	filesystem = GParted::FS_UNALLOCATED ;
	partition_number = sector_start = sector_end = sectors_used = sectors_unused = -1;
	color .set( "black" ) ;
	inside_extended = busy = false ;
	logicals .clear( ) ;
}

void Partition::Set(	const Glib::ustring & device_path,
			const Glib::ustring & partition,
			const int partition_number,
			const PartitionType type,
			const FILESYSTEM filesystem,
			const Sector & sector_start,
			const Sector & sector_end,
			const bool inside_extended,
			const bool busy )
{
	this ->device_path = device_path ;
	this ->partition = partition;
	this ->partition_number = partition_number;
	this ->type = type;
	this ->filesystem = filesystem;
	this ->sector_start = sector_start;
	this ->sector_end = sector_end;
	this ->color.set( Utils::Get_Color( filesystem ) );
	this ->inside_extended = inside_extended;
	this ->busy = busy;
}

void Partition::Set_Unused( Sector sectors_unused )
{
	if ( sectors_unused < ( sector_end - sector_start ) )
	{
		this ->sectors_unused = sectors_unused ;
		this ->sectors_used = ( sectors_unused == -1 ) ? -1 : ( sector_end - sector_start) - sectors_unused ;
	}
}

void Partition::Set_Unallocated( const Glib::ustring & device_path, Sector sector_start, Sector sector_end, bool inside_extended )
{
	this ->Set( device_path,
		    Utils::Get_Filesystem_String( GParted::FS_UNALLOCATED ),
		    -1,
		    GParted::TYPE_UNALLOCATED,
		    GParted::FS_UNALLOCATED,
		    sector_start,
		    sector_end,
		    inside_extended,
		    false ); 
	
	this ->error = this ->flags = "" ;
	this ->status = GParted::STAT_REAL ;
}

void Partition::Update_Number( int new_number )
{   
	this ->partition_number = new_number;
	this ->partition = device_path + Utils::num_to_str( partition_number ) ;
}

const long Partition::Get_Length_MB( ) const
{
	return Utils::Sector_To_MB( sector_end - sector_start ) ;
}

const long Partition::Get_Used_MB( ) const
{ 
	return Utils::Sector_To_MB( this ->sectors_used ) ;
}

const long Partition::Get_Unused_MB( ) const
{
	return Get_Length_MB( ) - Get_Used_MB( ) ;
}

Partition::~Partition( )
{
}

} //GParted
