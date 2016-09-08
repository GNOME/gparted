/* Copyright (C) 2008 Curtis Gedak
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

#include "OperationLabelFileSystem.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

OperationLabelFileSystem::OperationLabelFileSystem( const Device & device,
                                                    const Partition & partition_orig,
                                                    const Partition & partition_new )
{
	type = OPERATION_LABEL_FILESYSTEM;

	this->device = device.get_copy_without_partitions();
	this->partition_original = partition_orig.clone();
	this->partition_new      = partition_new.clone();
}

OperationLabelFileSystem::~OperationLabelFileSystem()
{
	delete partition_original;
	delete partition_new;
	partition_original = NULL;
	partition_new = NULL;
}

void OperationLabelFileSystem::apply_to_visual( PartitionVector & partitions )
{
	substitute_new( partitions );
}

void OperationLabelFileSystem::create_description()
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	if( partition_new->get_filesystem_partition().get_filesystem_label().empty() )
	{
		/* TO TRANSLATORS: looks like   Clear file system Label on /dev/hda3 */
		description = String::ucompose( _("Clear file system label on %1"),
		                                partition_new->get_path() );
	}
	else
	{
		/* TO TRANSLATORS: looks like   Set file system label "My Label" on /dev/hda3 */
		description = String::ucompose( _("Set file system label \"%1\" on %2"),
		                                partition_new->get_filesystem_partition().get_filesystem_label(),
		                                partition_new->get_path() );
	}
}

bool OperationLabelFileSystem::merge_operations( const Operation & candidate )
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	if ( candidate.type == OPERATION_LABEL_FILESYSTEM         &&
	     *partition_new == candidate.get_partition_original()    )
	{
		partition_new->set_filesystem_label( candidate.get_partition_new().get_filesystem_label() );
		create_description();
		return true;
	}

	return false;
}

} //GParted
