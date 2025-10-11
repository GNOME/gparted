/* Copyright (C) 2004 Bart 'plors' Hakvoort
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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include "OperationResizeMove.h"

#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"
#include "Utils.h"

#include <glib.h>
#include <glibmm/ustring.h>


namespace GParted
{


OperationResizeMove::OperationResizeMove( const Device & device,
				  	  const Partition & partition_orig,
				  	  const Partition & partition_new )
 : Operation(OPERATION_RESIZE_MOVE, device, partition_orig, partition_new)
{
}


void OperationResizeMove::apply_to_visual( PartitionVector & partitions )
{
	g_assert(m_partition_original != nullptr);  // Bug: Not initialised by constructor or reset later

	if (m_partition_original->type == TYPE_EXTENDED)
		apply_extended_to_visual( partitions ) ;
	else 
		apply_normal_to_visual( partitions ) ;
}


void OperationResizeMove::create_description() 
{
	g_assert(m_partition_original != nullptr);  // Bug: Not initialised by constructor or reset later
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

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

	if (m_partition_new->get_sector_length() > m_partition_original->get_sector_length())
	{
		//Grow partition
		action = GROW ;
		if (m_partition_new->sector_start > m_partition_original->sector_start)
			action = MOVE_RIGHT_GROW ;
		if (m_partition_new->sector_start < m_partition_original->sector_start)
			action = MOVE_LEFT_GROW ;
	}
	else if (m_partition_new->get_sector_length() < m_partition_original->get_sector_length())
	{
		//Shrink partition
		action = SHRINK ;
		if (m_partition_new->sector_start > m_partition_original->sector_start)
			action = MOVE_RIGHT_SHRINK ;
		if (m_partition_new->sector_start < m_partition_original->sector_start)
			action = MOVE_LEFT_SHRINK ;
	}
	else
	{
		//No change in partition size
		if (m_partition_new->sector_start > m_partition_original->sector_start)
			action = MOVE_RIGHT ;
		if (m_partition_new->sector_start < m_partition_original->sector_start)
			action = MOVE_LEFT ;
	}

	switch ( action )
	{
		case NONE		:
			m_description = Glib::ustring::compose(_("resize/move %1"), m_partition_original->get_path());
			m_description += " (";
			m_description += _("new and old partition have the same size and position.  Hence continuing anyway");
			m_description += ")";
			break ;
		case MOVE_RIGHT		:
			m_description = Glib::ustring::compose(_("Move %1 to the right"), m_partition_original->get_path());
			break ;
		case MOVE_LEFT		:
			m_description = Glib::ustring::compose(_("Move %1 to the left"), m_partition_original->get_path());
			break ;
		case GROW 		:
			m_description = _("Grow %1 from %2 to %3");
			break ;
		case SHRINK		:
			m_description = _("Shrink %1 from %2 to %3");
			break ;
		case MOVE_RIGHT_GROW	:
			m_description = _("Move %1 to the right and grow it from %2 to %3");
			break ;
		case MOVE_RIGHT_SHRINK	:
			m_description = _("Move %1 to the right and shrink it from %2 to %3");
			break ;
		case MOVE_LEFT_GROW	:
			m_description = _("Move %1 to the left and grow it from %2 to %3");
			break ;
		case MOVE_LEFT_SHRINK	:
			m_description = _("Move %1 to the left and shrink it from %2 to %3");
			break ;
	}

	if (! m_description.empty() && action != NONE && action != MOVE_LEFT && action != MOVE_RIGHT)
	{
		m_description = Glib::ustring::compose(m_description,
		                                m_partition_original->get_path(),
		                                Utils::format_size(m_partition_original->get_sector_length(),
		                                                   m_partition_original->sector_size),
		                                Utils::format_size(m_partition_new->get_sector_length(),
		                                                   m_partition_new->sector_size));
	}
}


void OperationResizeMove::apply_normal_to_visual( PartitionVector & partitions )
{
	g_assert(m_partition_original != nullptr);  // Bug: Not initialised by constructor or reset later
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	int index_extended;
	int index;

	if (m_partition_original->inside_extended)
	{
		index_extended = find_extended_partition( partitions );
		if ( index_extended >= 0 )
		{
			index = find_index_original( partitions[ index_extended ] .logicals ) ;

			if ( index >= 0 )
			{
				partitions[index_extended].logicals.replace_at(index, m_partition_new.get());
				remove_adjacent_unallocated( partitions[index_extended].logicals, index );

				insert_unallocated(partitions[index_extended].logicals,
				                   partitions[index_extended].sector_start,
				                   partitions[index_extended].sector_end,
				                   m_device.sector_size,
				                   true);
			}
		}
	}
	else
	{
		index = find_index_original( partitions ) ;

		if ( index >= 0 )
		{
			partitions.replace_at(index, m_partition_new.get());
			remove_adjacent_unallocated( partitions, index ) ;

			insert_unallocated(partitions, 0, m_device.length -1, m_device.sector_size, false);
		}
	}
}


void OperationResizeMove::apply_extended_to_visual( PartitionVector & partitions )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	int index_extended;

	//stuff OUTSIDE extended partition
	index_extended = find_extended_partition( partitions );
	if ( index_extended >= 0 )
	{
		remove_adjacent_unallocated( partitions, index_extended ) ;

		index_extended = find_extended_partition( partitions );
		if ( index_extended >= 0 )
		{
			partitions[index_extended].sector_start = m_partition_new->sector_start;
			partitions[index_extended].sector_end   = m_partition_new->sector_end;
		}

		insert_unallocated(partitions, 0, m_device.length -1, m_device.sector_size, false);
	}

	//stuff INSIDE extended partition
	index_extended = find_extended_partition( partitions );
	if ( index_extended >= 0 )
	{
		if ( partitions[ index_extended ] .logicals .size() > 0 &&
		     partitions[index_extended].logicals.front().type == TYPE_UNALLOCATED )
			partitions[ index_extended ] .logicals .erase( partitions[ index_extended ] .logicals .begin() ) ;
	
		if ( partitions[ index_extended ] .logicals .size() &&
		     partitions[index_extended].logicals.back().type == TYPE_UNALLOCATED )
			partitions[ index_extended ] .logicals .pop_back() ;
	
		insert_unallocated(partitions[index_extended].logicals,
				   partitions[index_extended].sector_start,
				   partitions[index_extended].sector_end,
				   m_device.sector_size,
				   true);
	}
}


void OperationResizeMove::remove_adjacent_unallocated( PartitionVector & partitions, int index_orig )
{
	//remove unallocated space following the original partition
	if ( index_orig +1 < static_cast<int>( partitions .size() ) &&
	     partitions[index_orig+1].type == TYPE_UNALLOCATED )
		partitions .erase( partitions .begin() + index_orig +1 );
	
	//remove unallocated space preceding the original partition
	if (index_orig-1 >= 0 && partitions[index_orig-1].type == TYPE_UNALLOCATED)
		partitions .erase( partitions .begin() + ( index_orig -1 ) ) ;
}


bool OperationResizeMove::merge_operations( const Operation & candidate )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (candidate.m_type == OPERATION_RESIZE_MOVE              &&
	    *m_partition_new == candidate.get_partition_original()   )
	{
		m_partition_new.reset(candidate.get_partition_new().clone());
		create_description();
		return true;
	}

	return false;
}


}  // namespace GParted
