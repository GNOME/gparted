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

#include "Operation.h"
#include "GParted_Core.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

Operation::Operation()
{
}

Partition & Operation::get_partition_original()
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by derived Operation*() constructor or reset later

	return *partition_original;
}

const Partition & Operation::get_partition_original() const
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by derived Operation*() constructor or reset later

	return *partition_original;
}

Partition & Operation::get_partition_new()
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by derived Operation*() constructor or reset later

	return *partition_new;
}

const Partition & Operation::get_partition_new() const
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by derived Operation*() constructor or reset later

	return *partition_new;
}

int Operation::find_index_original( const PartitionVector & partitions )
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by derived Operation*() constructor or reset later

	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
		if ( partition_original->sector_start >= partitions[t].sector_start &&
		     partition_original->sector_end   <= partitions[t].sector_end      )
			return t ;

	return -1 ;
}

// Find the partition in the vector that exactly matches or fully encloses
// this->partition_new.  Return vector index or -1 when no match found.
int Operation::find_index_new( const PartitionVector & partitions )
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	for ( unsigned int i = 0 ; i < partitions.size() ; i ++ )
		if ( partition_new->sector_start >= partitions[i].sector_start &&
		     partition_new->sector_end   <= partitions[i].sector_end      )
			return i;

	return -1;
}

void Operation::insert_unallocated( PartitionVector & partitions,
                                    Sector start, Sector end, Byte_Value sector_size, bool inside_extended )
{
	GParted_Core::insert_unallocated( device.get_path(), partitions,
	                                  start, end, sector_size, inside_extended );
}

// Visual re-apply this operation, for operations which don't change the partition
// boundaries.  Matches this operation's original partition in the vector and substitutes
// it with this operation's new partition.
void Operation::substitute_new( PartitionVector & partitions )
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by constructor or reset later
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	int index_extended;
	int index;

	if ( partition_original->inside_extended )
	{
		index_extended = find_extended_partition( partitions );
		if ( index_extended >= 0 )
		{
			index = find_index_original( partitions[index_extended].logicals );
			if ( index >= 0 )
				partitions[index_extended].logicals.replace_at( index, partition_new );
		}
	}
	else
	{
		index = find_index_original( partitions );
		if ( index >= 0 )
			partitions.replace_at( index, partition_new );
	}
}

// Visually re-apply this operation, for operations which create new partitions.
void Operation::insert_new( PartitionVector & partitions )
{
	// Create operations are unique in that they apply to unallocated space.  It only
	// matters that the new partition being created fits in an unallocated space when
	// visually re-applying this operation to the disk graphic.  Hence the use of,
	// find_index_new() here.
	//
	// All other operation types apply to existing partitions which do or will exist
	// on disk.  Therefore they match the original partition when visually re-applying
	// their operations to the disk graphic.  Hence their use of,
	// find_index_original().

	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	int index_extended;
	int index;

	if ( partition_new->inside_extended )
	{
		index_extended = find_extended_partition( partitions );
		if ( index_extended >= 0 )
		{
			index = find_index_new( partitions[index_extended].logicals );
			if ( index >= 0 )
			{
				partitions[index_extended].logicals.replace_at( index, partition_new );

				insert_unallocated( partitions[index_extended].logicals,
				                    partitions[index_extended].sector_start,
				                    partitions[index_extended].sector_end,
				                    device.sector_size,
				                    true );
			}
		}
	}
	else
	{
		index = find_index_new( partitions );
		if ( index >= 0 )
		{
			partitions.replace_at( index, partition_new );

			insert_unallocated( partitions, 0, device.length-1, device.sector_size, false );
		}
	}
}

} //GParted
