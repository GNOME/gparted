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
	max_prims = highest_busy = 0 ;
	readonly = false ; 	
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

Glib::ustring Device::get_path() 
{
	if ( paths .size() > 0 )
		return paths .front() ;

	return "" ;
}
	
std::vector<Glib::ustring> Device::get_paths()
{
	return paths ;
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
