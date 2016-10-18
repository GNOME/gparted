/* Copyright (C) 2015 Michael Zimmermann
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

#include "OperationNamePartition.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

OperationNamePartition::OperationNamePartition( const Device & device,
                                                const Partition & partition_orig,
                                                const Partition & partition_new )
{
	type = OPERATION_NAME_PARTITION;

	this->device = device.get_copy_without_partitions();
	this->partition_original = partition_orig.clone();
	this->partition_new      = partition_new.clone();
}

OperationNamePartition::~OperationNamePartition()
{
	delete partition_original;
	delete partition_new;
	partition_original = NULL;
	partition_new = NULL;
}

void OperationNamePartition::apply_to_visual( PartitionVector & partitions )
{
	substitute_new( partitions );
}

void OperationNamePartition::create_description()
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	if( partition_new->name.empty() )
	{
		/* TO TRANSLATORS: looks like   Clear partition name on /dev/hda3 */
		description = String::ucompose( _("Clear partition name on %1"),
		                                partition_new->get_path() );
	}
	else
	{
		/* TO TRANSLATORS: looks like   Set partition name "My Name" on /dev/hda3 */
		description = String::ucompose( _("Set partition name \"%1\" on %2"),
		                                partition_new->name,
		                                partition_new->get_path() );
	}
}

bool OperationNamePartition::merge_operations( const Operation & candidate )
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	if ( candidate.type == OPERATION_NAME_PARTITION           &&
	     *partition_new == candidate.get_partition_original()    )
	{
		partition_new->name = candidate.get_partition_new().name;
		create_description();
		return true;
	}

	return false;
}

} //GParted
