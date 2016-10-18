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

#include "OperationCopy.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

OperationCopy::OperationCopy( const Device & device,
			      const Partition & partition_orig,
			      const Partition & partition_new,
			      const Partition & partition_copied )
{
	type = OPERATION_COPY ;

	this->device = device.get_copy_without_partitions();
	this->partition_original = partition_orig.clone();
	this->partition_new      = partition_new.clone();
	this->partition_copied   = partition_copied.clone();
}

OperationCopy::~OperationCopy()
{
	delete partition_original;
	delete partition_new;
	delete partition_copied;
	partition_original = NULL;
	partition_new = NULL;
	partition_copied = NULL;
}

Partition & OperationCopy::get_partition_copied()
{
	g_assert( partition_copied != NULL );  // Bug: Not initialised by constructor or reset later

	return *partition_copied;
}

const Partition & OperationCopy::get_partition_copied() const
{
	g_assert( partition_copied != NULL );  // Bug: Not initialised by constructor or reset later

	return *partition_copied;
}

void OperationCopy::apply_to_visual( PartitionVector & partitions )
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by constructor or reset later

	if ( partition_original->type == TYPE_UNALLOCATED )
		// Paste into unallocated space creating new partition
		insert_new( partitions );
	else
		// Paste into existing partition
		substitute_new( partitions );
}

void OperationCopy::create_description() 
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by constructor or reset later
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later
	g_assert( partition_copied != NULL );  // Bug: Not initialised by constructor or reset later

	if ( partition_original->type == TYPE_UNALLOCATED )
	{
		/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd (start at 250 MiB) */
		description = String::ucompose( _("Copy %1 to %2 (start at %3)"),
		                                partition_copied->get_path(),
		                                device.get_path(),
		                                Utils::format_size( partition_new->sector_start,
		                                                    partition_new->sector_size ) );
	}
	else
	{
		/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd1 */
		description = String::ucompose( _("Copy %1 to %2"),
		                                partition_copied->get_path(),
		                                partition_original->get_path() );
	}
}

bool OperationCopy::merge_operations( const Operation & candidate )
{
	return false;  // Never merge copy operations
}

} //GParted
