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
	
Partition::Partition()
{
	Reset() ;
}

void Partition::Reset()
{
	partition = realpath = error = "" ;
	status = GParted::STAT_REAL ;
	type = GParted::TYPE_UNALLOCATED ;
	filesystem = GParted::FS_UNALLOCATED ;
	partition_number = sector_start = sector_end = sectors_used = sectors_unused = -1;
	color .set( "black" ) ;
	inside_extended = busy = strict = false ;
	logicals .clear() ;
	flags .clear() ;
	mountpoints .clear() ;
}

void Partition::Set(	const Glib::ustring & device_path,
			const Glib::ustring & partition,
			int partition_number,
			PartitionType type,
			FILESYSTEM filesystem,
			Sector sector_start,
			Sector sector_end,
			bool inside_extended,
			bool busy )
{
	this ->device_path = device_path ;
	this ->partition = realpath = partition;
	this ->partition_number = partition_number;
	this ->type = type;
	this ->filesystem = filesystem;
	this ->sector_start = sector_start;
	this ->sector_end = sector_end;
	this ->inside_extended = inside_extended;
	this ->busy = busy;
	
	this ->color.set( Utils::Get_Color( filesystem ) );
}

void Partition::Set_Unused( Sector sectors_unused )
{
	if ( sectors_unused < get_length() )
	{
		this ->sectors_unused = sectors_unused ;
		this ->sectors_used = ( sectors_unused == -1 ) ? -1 : get_length() - sectors_unused ;
	}
}

void Partition::Set_Unallocated( const Glib::ustring & device_path, Sector sector_start, Sector sector_end, bool inside_extended )
{
	Reset() ;
		
	Set( device_path,
	     Utils::Get_Filesystem_String( GParted::FS_UNALLOCATED ),
	     -1,
	     GParted::TYPE_UNALLOCATED,
	     GParted::FS_UNALLOCATED,
	     sector_start,
	     sector_end,
	     inside_extended,
	     false ); 
	
	status = GParted::STAT_REAL ;
}

void Partition::Update_Number( int new_number )
{   
	this ->partition =
		partition .substr( 0, partition .find( Utils::num_to_str( partition_number ) ) ) +
		Utils::num_to_str( new_number ) ;
	
	this ->partition_number = new_number;
}

Sector Partition::get_length() const
{
	return sector_end - sector_start + 1 ;
}

bool Partition::operator==( const Partition & partition ) const
{
	return this ->partition_number == partition .partition_number && 
	       this ->sector_start == partition .sector_start && 
	       this ->type == partition .type ;
}

Partition::~Partition()
{
}

} //GParted
