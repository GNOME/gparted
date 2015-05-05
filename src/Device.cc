/* Copyright (C) 2004 Bart
 * Copyright (C) 2010 Curtis Gedak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
 
#include "../include/Device.h"

namespace GParted
{

Device::Device()
{
	Reset() ;	
}

void Device::Reset()
{
	paths .clear() ;
	partitions .clear() ;
	length = cylsize = 0 ;
	heads = sectors = cylinders = 0 ;
	model = disktype = "" ;
	sector_size = max_prims = highest_busy = 0 ;
	readonly = false ; 	
	max_partition_name_length = 0;
}
	
void Device::add_path( const Glib::ustring & path, bool clear_paths )
{
	if ( clear_paths )
		paths .clear() ;

	paths .push_back( path ) ;

	sort_paths_and_remove_duplicates() ;
}

void Device::add_paths( const std::vector<Glib::ustring> & paths, bool clear_paths )
{
	if ( clear_paths )
		this ->paths .clear() ;

	this ->paths .insert( this ->paths .end(), paths .begin(), paths .end() ) ;

	sort_paths_and_remove_duplicates() ;
}

Glib::ustring Device::get_path() const
{
	if ( paths .size() > 0 )
		return paths .front() ;

	return "" ;
}
	
std::vector<Glib::ustring> Device::get_paths() const
{
	return paths ;
}

void Device::enable_partition_naming( int max_length )
{
	if ( max_length > 0 )
		max_partition_name_length = max_length;
	else
		max_partition_name_length = 0;
}

bool Device::partition_naming_supported() const
{
	return max_partition_name_length > 0;
}

int Device::get_max_partition_name_length() const
{
	return max_partition_name_length;
}

bool Device::operator==( const Device & device ) const
{
	return this ->get_path() == device .get_path() ;
}
	
bool Device::operator!=( const Device & device ) const 
{
	return ! ( *this == device ) ;
}
	
void Device::sort_paths_and_remove_duplicates()
{
	//remove duplicates
	std::sort( paths .begin(), paths .end() ) ;
	paths .erase( std::unique( paths .begin(), paths .end() ), paths .end() ) ;

	//sort on length
	std::sort( paths .begin(), paths .end(), compare_paths ) ;
}

bool Device::compare_paths( const Glib::ustring & A, const Glib::ustring & B )
{
	return A .length() < B .length() ;
}

Device::~Device()
{
}


} //GParted
