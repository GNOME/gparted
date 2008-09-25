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
	type = OPERATION_RESIZE_MOVE ;
	this ->device = device ;
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
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
	//i'm not too happy with this, but i think it is the correct way from a i18n POV
	enum Action
	{
		NONE			= 0,
		MOVE_RIGHT	 	= 1,
		MOVE_LEFT		= 2,
		GROW 			= 3,
		SHRINK			= 4,
		MOVE_RIGHT_GROW		= 5,
		MOVE_RIGHT_SHRINK	= 6,
		MOVE_LEFT_GROW		= 7,
		MOVE_LEFT_SHRINK	= 8
	} ;
	Action action = NONE ;

	if ( partition_new .get_length() > partition_original .get_length() ) {
		//Grow partition
		action = GROW ;
		if ( partition_new .sector_start > partition_original .sector_start )
			action = MOVE_RIGHT_GROW ;
		if ( partition_new .sector_start < partition_original .sector_start )
			action = MOVE_LEFT_GROW ;
	} else if ( partition_new .get_length() < partition_original .get_length() ) {
		//Shrink partition
		action = SHRINK ;
		if ( partition_new .sector_start > partition_original .sector_start )
			action = MOVE_RIGHT_SHRINK ;
		if ( partition_new .sector_start < partition_original .sector_start )
			action = MOVE_LEFT_SHRINK ;
	} else {
		//No change in partition size
		if ( partition_new .sector_start > partition_original .sector_start )
			action = MOVE_RIGHT ;
		if ( partition_new .sector_start < partition_original .sector_start )
			action = MOVE_LEFT ;
	}

	switch ( action )
	{
		case NONE		:
			description = String::ucompose( _("resize/move %1"), partition_original .get_path() ) ;
			description += " (" ;
			description += _("new and old partition have the same size and position -- continuing anyway") ;
			description += ")" ;
			break ;
		case MOVE_RIGHT		:
			description = String::ucompose( _("Move %1 to the right"), partition_original .get_path() ) ;
			break ;
		case MOVE_LEFT		:
			description = String::ucompose( _("Move %1 to the left"), partition_original .get_path() ) ;
			break ;
		case GROW 		:
			description = _("Grow %1 from %2 to %3") ;
			break ;
		case SHRINK		:
			description = _("Shrink %1 from %2 to %3") ;
			break ;
		case MOVE_RIGHT_GROW	:
			description = _("Move %1 to the right and grow it from %2 to %3") ;
			break ;
		case MOVE_RIGHT_SHRINK	:
			description = _("Move %1 to the right and shrink it from %2 to %3") ;
			break ;
		case MOVE_LEFT_GROW	:
			description = _("Move %1 to the left and grow it from %2 to %3") ;
			break ;
		case MOVE_LEFT_SHRINK	:
			description = _("Move %1 to the left and shrink it from %2 to %3") ;
			break ;
	}

	if ( ! description .empty() && action != NONE && action != MOVE_LEFT && action != MOVE_RIGHT )
		description = String::ucompose( description,
						partition_original .get_path(),
						Utils::format_size( partition_original .get_length() ),
						Utils::format_size( partition_new .get_length() ) ) ;
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
