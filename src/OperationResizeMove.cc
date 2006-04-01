/* Copyright (C) 2004 Bart 'plors' Hakvoort
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

#include "../include/OperationResizeMove.h"

namespace GParted
{

OperationResizeMove::OperationResizeMove( const Device & device,
				  	  const Partition & partition_orig,
				  	  const Partition & partition_new )
{
	type = GParted::RESIZE_MOVE ;
	this ->device = device ;
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;

	create_description() ;
}
	
void OperationResizeMove::apply_to_visual( std::vector<Partition> & partitions ) 
{
	index = index_extended = -1 ;
	
	if ( partition_original .type == GParted::TYPE_EXTENDED )
		apply_extended_to_visual( partitions ) ;
	else 
		apply_normal_to_visual( partitions ) ;
}

void OperationResizeMove::create_description() 
{
	//FIXME:make messages more informative by specifying shrink/grow instead of resize. 
	//if startsector has changed we consider it a move
	Sector diff = std::abs( partition_new .sector_start - partition_original .sector_start ) ;
	if ( diff ) 
	{
		if ( diff > 0 )
			description = String::ucompose( _("Move %1 forward by %2"), 
						 	partition_new .get_path(),
						 	Utils::format_size( diff ) ) ;
		else
			description = String::ucompose( _("Move %1 backward by %2"),
							partition_new .get_path(),
							Utils::format_size( diff ) ) ;
	}
			
	//check if size has changed
	diff = std::abs( partition_original .get_length() - partition_new .get_length() ) ;
	if ( diff )
	{
		if ( description .empty() ) 
			description = String::ucompose( _("Resize %1 from %2 to %3"), 
						 	partition_new .get_path(),
							Utils::format_size( partition_original .get_length() ),
							Utils::format_size( partition_new .get_length() ) ) ;
		else
			description += " " + String::ucompose( _("and Resize %1 from %2 to %3"),
							       partition_new .get_path(),
							       Utils::format_size( partition_original .get_length() ),
							       Utils::format_size( partition_new .get_length() ) ) ;
	}
}

void OperationResizeMove::apply_normal_to_visual( std::vector<Partition> & partitions )
{
	if ( partition_original .inside_extended )
	{
		index_extended = find_index_extended( partitions ) ;
			
		if ( index_extended >= 0 )
			index = find_index_original( partitions[ index_extended ] .logicals ) ;
			
		if ( index >= 0 )
		{
			 partitions[ index_extended ] .logicals[ index ] = partition_new ;
			 remove_adjacent_unallocated( partitions[ index_extended ] .logicals, index ) ;

			 insert_unallocated( partitions[ index_extended ] .logicals,
				    	     partitions[ index_extended ] .sector_start,
				    	     partitions[ index_extended ] .sector_end,
					     true ) ;
		}
	}
	else
	{
		index = find_index_original( partitions ) ;

		if ( index >= 0 )
		{
			partitions[ index ] = partition_new ;
			remove_adjacent_unallocated( partitions, index ) ;

			insert_unallocated( partitions, 0, device .length -1, false ) ;
		}
	}
}

void OperationResizeMove::apply_extended_to_visual( std::vector<Partition> & partitions ) 
{
	//stuff OUTSIDE extended partition
	index_extended = find_index_extended( partitions ) ;
		
	if ( index_extended >= 0 )
	{
		remove_adjacent_unallocated( partitions, index_extended ) ;
		
		index_extended = find_index_extended( partitions ) ;
		if ( index_extended >= 0 )
		{
			partitions[ index_extended ] .sector_start = partition_new .sector_start ;
			partitions[ index_extended ] .sector_end = partition_new .sector_end ;
		}
	
		insert_unallocated( partitions, 0, device .length -1, false ) ;
	}
	
	//stuff INSIDE extended partition
	index_extended = find_index_extended( partitions ) ;
	
	if ( index_extended >= 0 )
	{
		if ( partitions[ index_extended ] .logicals .size() > 0 &&
		     partitions[ index_extended ] .logicals .front() .type == GParted::TYPE_UNALLOCATED )
			partitions[ index_extended ] .logicals .erase( partitions[ index_extended ] .logicals .begin() ) ;
	
		if ( partitions[ index_extended ] .logicals .size() &&
		     partitions[ index_extended ] .logicals .back() .type == GParted::TYPE_UNALLOCATED )
			partitions[ index_extended ] .logicals .pop_back() ;
	
		insert_unallocated( partitions[ index_extended ] .logicals,
				    partitions[ index_extended ] .sector_start,
				    partitions[ index_extended ] .sector_end,
				    true ) ;
	}
}

void OperationResizeMove::remove_adjacent_unallocated( std::vector<Partition> & partitions, int index_orig ) 
{
	//remove unallocated space following the original partition
	if ( index_orig +1 < static_cast<int>( partitions .size() ) &&
	     partitions[ index_orig +1 ] .type == GParted::TYPE_UNALLOCATED )
		partitions .erase( partitions .begin() + index_orig +1 );
	
	//remove unallocated space preceding the original partition
	if ( index_orig -1 >= 0 && partitions[ index_orig -1 ] .type == GParted::TYPE_UNALLOCATED )
		partitions .erase( partitions .begin() + ( index_orig -1 ) ) ;
}

} //GParted
