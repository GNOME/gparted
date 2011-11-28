/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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
	
Partition::Partition( const Glib::ustring & path ) 
{
	Reset() ;

	paths .push_back( path ) ;
}

void Partition::Reset()
{
	paths .clear() ;
	messages .clear() ;
	status = GParted::STAT_REAL ;
	type = GParted::TYPE_UNALLOCATED ;
	filesystem = GParted::FS_UNALLOCATED ;
	label .clear() ;
	uuid .clear() ;
	partition_number = sector_start = sector_end = sectors_used = sectors_unused = -1;
	free_space_before = -1 ;
	sector_size = 0 ;
	color .set( "black" ) ;
	inside_extended = busy = strict_start = false ;
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
			Byte_Value sector_size,
			bool inside_extended,
			bool busy )
{
	this ->device_path = device_path ;

	paths .push_back( partition ) ;

	this ->partition_number = partition_number;
	this ->type = type;
	this ->filesystem = filesystem;
	this ->sector_start = sector_start;
	this ->sector_end = sector_end;
	this ->sector_size = sector_size;
	this ->inside_extended = inside_extended;
	this ->busy = busy;
	
	this ->color .set( Utils::get_color( filesystem ) );
}

void Partition::Set_Unused( Sector sectors_unused )
{
	if ( sectors_unused <= get_sector_length() )
	{
		this ->sectors_unused = sectors_unused ;
		this ->sectors_used = ( sectors_unused == -1 ) ? -1 : get_sector_length() - sectors_unused ;
	}
}
	
void Partition::set_used( Sector sectors_used ) 
{
	if ( sectors_used < get_sector_length() )
	{
		this ->sectors_used = sectors_used ;
		this ->sectors_unused = ( sectors_used == -1 ) ? -1 : get_sector_length() - sectors_used ;
	}
}

void Partition::Set_Unallocated( const Glib::ustring & device_path,
				 Sector sector_start,
				 Sector sector_end,
				 Byte_Value sector_size,
				 bool inside_extended )
{
	Reset() ;
	
	Set( device_path,
	     Utils::get_filesystem_string( GParted::FS_UNALLOCATED ),
	     -1,
	     GParted::TYPE_UNALLOCATED,
	     GParted::FS_UNALLOCATED,
	     sector_start,
	     sector_end,
	     sector_size,
	     inside_extended,
	     false ); 
	
	status = GParted::STAT_REAL ;
}

void Partition::Update_Number( int new_number )
{  
	unsigned int index ;
	for ( unsigned int t = 0 ; t < paths .size() ; t++ )
	{
		index = paths[ t ] .rfind( Utils::num_to_str( partition_number ) ) ;

		if ( index < paths[ t ] .length() )
			paths[ t ] .replace( index,
				       Utils::num_to_str( partition_number ) .length(),
				       Utils::num_to_str( new_number ) ) ;
	}

	partition_number = new_number;
}
	
void Partition::add_path( const Glib::ustring & path, bool clear_paths ) 
{
	if ( clear_paths )
		paths .clear() ;

	paths .push_back( path ) ;

	sort_paths_and_remove_duplicates() ;
}
	
void Partition::add_paths( const std::vector<Glib::ustring> & paths, bool clear_paths )
{
	if ( clear_paths )
		this ->paths .clear() ;

	this ->paths .insert( this ->paths .end(), paths .begin(), paths .end() ) ;

	sort_paths_and_remove_duplicates() ;
}

Byte_Value Partition::get_byte_length() const 
{
	if ( get_sector_length() >= 0 )
		return get_sector_length() * sector_size ;
	else
		return -1 ;
}

Sector Partition::get_sector_length() const 
{
	if ( sector_start >= 0 && sector_end >= 0 )
		return sector_end - sector_start + 1 ;
	else
		return -1 ;
}

Glib::ustring Partition::get_path() const
{
	if ( paths .size() > 0 )
		return paths .front() ;
	
	return "" ;
}

std::vector<Glib::ustring> Partition::get_paths() const
{
	return paths ;
}

bool Partition::operator==( const Partition & partition ) const
{
	return device_path == partition .device_path &&
	       partition_number == partition .partition_number && 
	       sector_start == partition .sector_start && 
	       type == partition .type ;
}

bool Partition::operator!=( const Partition & partition ) const
{
	return ! ( *this == partition ) ;
}

void Partition::sort_paths_and_remove_duplicates()
{
	//remove duplicates
	std::sort( paths .begin(), paths .end() ) ;
	paths .erase( std::unique( paths .begin(), paths .end() ), paths .end() ) ;

	//sort on length
	std::sort( paths .begin(), paths .end(), compare_paths ) ;
}

void Partition::add_mountpoints( const std::vector<Glib::ustring> & mountpoints, bool clear_mountpoints ) 
{
	if ( clear_mountpoints )
		this ->mountpoints .clear() ;

	this ->mountpoints .insert( this ->mountpoints .end(), mountpoints .begin(), mountpoints .end() ) ;
}

Glib::ustring Partition::get_mountpoint() const 
{
	if ( mountpoints .size() > 0 )
		return mountpoints .front() ;

	return "" ;
}

std::vector<Glib::ustring> Partition::get_mountpoints() const 
{
	return mountpoints ;
}

Sector Partition::get_sector() const 
{
	return (sector_start + sector_end) / 2 ; 
}
	
bool Partition::test_overlap( const Partition & partition ) const
{
	return ( (partition .sector_start >= sector_start && partition .sector_start <= sector_end) 
		 ||
		 (partition .sector_end >= sector_start && partition .sector_end <= sector_end)
		 ||
		 (partition .sector_start < sector_start && partition .sector_end > sector_end) ) ;
}

void Partition::clear_mountpoints()
{
	mountpoints .clear() ;
}

bool Partition::compare_paths( const Glib::ustring & A, const Glib::ustring & B )
{
	return A .length() < B .length() ;
}

Partition::~Partition()
{
}

} //GParted
