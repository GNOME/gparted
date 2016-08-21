/* Copyright (C) 2004 Bart 'plors' Hakvoort
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

#include "OperationDelete.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

OperationDelete::OperationDelete( const Device & device, const Partition & partition_orig )
{
	type = OPERATION_DELETE ;

	this->device = device.get_copy_without_partitions();
	this->partition_original = partition_orig.clone();
}

OperationDelete::~OperationDelete()
{
	delete partition_original;
	partition_original = NULL;
}

Partition & OperationDelete::get_partition_new()
{
	g_assert( false );  // Bug: OperationDelete class doesn't use partition_new

	// Not reached.  Return value to keep compiler quiet.
	return *partition_new;
}

const Partition & OperationDelete::get_partition_new() const
{
	g_assert( false );  // Bug: OperationDelete class doesn't use partition_new

	// Not reached.  Return value to keep compiler quiet.
	return *partition_new;
}

void OperationDelete::apply_to_visual( PartitionVector & partitions )
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by constructor or reset later

	int index_extended;
	int index;

	if ( partition_original->inside_extended )
	{
		index_extended = find_extended_partition( partitions );
		if ( index_extended >= 0 )
		{
			index = find_index_original( partitions[ index_extended ] .logicals ) ;

			if ( index >= 0 )
			{
				remove_original_and_adjacent_unallocated( partitions[index_extended].logicals, index );

				insert_unallocated( partitions[index_extended].logicals,
				                    partitions[index_extended].sector_start,
				                    partitions[index_extended].sector_end,
				                    device.sector_size,
				                    true );

				// If deleted partition was logical we have to decrease
				// the partition numbers of the logicals with higher
				// numbers by one (only if its a real partition)
				if ( partition_original->status != STAT_NEW )
					for ( unsigned int t = 0 ; t < partitions[index_extended].logicals .size() ; t++ )
						if ( partitions[index_extended].logicals[t].partition_number >
						     partition_original->partition_number                      )
							partitions[index_extended].logicals[t].Update_Number(
								partitions[index_extended].logicals[t].partition_number -1 );
			}
		}
	}
	else
	{
		index = find_index_original( partitions ) ;

		if ( index >= 0 )
		{
			remove_original_and_adjacent_unallocated( partitions, index ) ;
			
			insert_unallocated( partitions, 0, device .length -1, device .sector_size, false ) ;
		}
	}
}

void OperationDelete::create_description() 
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by constructor or reset later

	if ( partition_original->type == TYPE_LOGICAL )
		description = _("Logical Partition") ;
	else
		description = partition_original->get_path();

	/*TO TRANSLATORS: looks like   Delete /dev/hda2 (ntfs, 345 MiB) from /dev/hda */
	description = String::ucompose( _("Delete %1 (%2, %3) from %4"),
	                                description,
	                                partition_original->get_filesystem_string(),
	                                Utils::format_size( partition_original->get_sector_length(),
	                                                    partition_original->sector_size ),
	                                partition_original->device_path );
}

bool OperationDelete::merge_operations( const Operation & candidate )
{
	return false;  // Can't merge with an already deleted partition
}

void OperationDelete::remove_original_and_adjacent_unallocated( PartitionVector & partitions, int index_orig )
{
	//remove unallocated space following the original partition
	if ( index_orig +1 < static_cast<int>( partitions .size() ) &&
	     partitions[ index_orig +1 ] .type == GParted::TYPE_UNALLOCATED )
		partitions .erase( partitions .begin() + index_orig +1 );
	
	//remove unallocated space preceding the original partition and the original partition
	if ( index_orig -1 >= 0 && partitions[ index_orig -1 ] .type == GParted::TYPE_UNALLOCATED )
		partitions .erase( partitions .begin() + --index_orig ) ;
				
	//and finally remove the original partition
	partitions .erase( partitions .begin() + index_orig ) ;
}

} //GParted
