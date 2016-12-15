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

#include "OperationCreate.h"
#include "OperationFormat.h"
#include "OperationResizeMove.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

OperationCreate::OperationCreate( const Device & device,
				  const Partition & partition_orig,
				  const Partition & partition_new )
{
	type = OPERATION_CREATE ;

	this->device = device.get_copy_without_partitions();
	this->partition_original = partition_orig.clone();
	this->partition_new      = partition_new.clone();
}

OperationCreate::~OperationCreate()
{
	delete partition_original;
	delete partition_new;
	partition_original = NULL;
	partition_new = NULL;
}

void OperationCreate::apply_to_visual( PartitionVector & partitions )
{
	insert_new( partitions );
}

void OperationCreate::create_description() 
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	switch( partition_new->type )
	{
		case GParted::TYPE_PRIMARY	:
			description = _("Primary Partition");
			break;
		case GParted::TYPE_LOGICAL	:
			description = _("Logical Partition") ;
			break;	
		case GParted::TYPE_EXTENDED	:
			description = _("Extended Partition");
			break;
	
		default	:
			break;
	}
	/*TO TRANSLATORS: looks like   Create Logical Partition #1 (ntfs, 345 MiB) on /dev/hda */
	description = String::ucompose( _("Create %1 #%2 (%3, %4) on %5"),
	                                description,
	                                partition_new->partition_number,
	                                partition_new->get_filesystem_string(),
	                                Utils::format_size( partition_new->get_sector_length(),
	                                                    partition_new->sector_size ),
	                                device.get_path() );
}

bool OperationCreate::merge_operations( const Operation & candidate )
{
	g_assert( partition_new != NULL );  // Bug: Not initialised by constructor or reset later

	if ( candidate.type                            == OPERATION_FORMAT                   &&
	     candidate.get_partition_original().status == STAT_NEW                           &&
	     *partition_new                            == candidate.get_partition_original()    )
	{
		// Merge a format operation on a not yet created partition with the
		// earlier operation which will create it.
		delete partition_new;
		partition_new = candidate.get_partition_new().clone();
		create_description();
		return true;
	}
	else if ( candidate.type                            == OPERATION_RESIZE_MOVE              &&
	          candidate.get_partition_original().status == STAT_NEW                           &&
	          *partition_new                            == candidate.get_partition_original()    )
	{
		// Merge a resize/move operation on a not yet created partition with the
		// earlier operation which will create it.
		delete partition_new;
		partition_new = candidate.get_partition_new().clone();
		create_description();
		return true;
	}

	return false;
}

} //GParted
