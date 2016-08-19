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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "OperationFormat.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

OperationFormat::OperationFormat( const Device & device,
				  const Partition & partition_orig,
				  const Partition & partition_new )
{
	type = OPERATION_FORMAT ;

	this->device = device.get_copy_without_partitions();
	this->partition_original = partition_orig.clone();
	this->partition_new      = partition_new.clone();
}

OperationFormat::~OperationFormat()
{
	delete partition_original;
	delete partition_new;
	partition_original = NULL;
	partition_new = NULL;
}

void OperationFormat::apply_to_visual( PartitionVector & partitions )
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by constructor or reset later
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	if ( partition_original->type == TYPE_UNPARTITIONED && partition_new->filesystem == FS_CLEARED )
	{
		// Make format to cleared whole disk device file system preview as
		// unallocated device, matching what happens when implemented.
		partitions.clear();

		Partition * temp_partition = new Partition();
		temp_partition->set_unpartitioned( device.get_path(),
		                                   "",  // Overridden with "unallocated"
		                                   FS_UNALLOCATED,
		                                   device.length,
		                                   device.sector_size,
		                                   false );
		partitions.push_back_adopt( temp_partition );
	}
	else
	{
		substitute_new( partitions );
	}
}

void OperationFormat::create_description() 
{
	g_assert( partition_original != NULL );  // Bug: Not initialised by constructor or reset later
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	/*TO TRANSLATORS: looks like  Format /dev/hda4 as linux-swap */
	description = String::ucompose( _("Format %1 as %2"),
	                                partition_original->get_path(),
	                                partition_new->get_filesystem_string() );
}

bool OperationFormat::merge_operations( const Operation & candidate )
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	if ( candidate.type == OPERATION_FORMAT                   &&
	     *partition_new == candidate.get_partition_original()    )
	{
		delete partition_new;
		partition_new = candidate.get_partition_new().clone();
		create_description();
		return true;
	}

	return false;
}

} //GParted
