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

#include "../include/OperationCopy.h"

namespace GParted
{

OperationCopy::OperationCopy( const Device & device,
			      const Partition & partition_orig,
			      const Partition & partition_new,
			      const Partition & partition_copied )
{
	type = OPERATION_COPY ;

	this->device = device.get_copy_without_partitions();
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
	this ->partition_copied = partition_copied ;

	this ->partition_new .add_path(  
		String::ucompose( _("copy of %1"), this ->partition_copied .get_path() ), true ) ;
}
	
void OperationCopy::apply_to_visual( std::vector<Partition> & partitions ) 
{
	if ( partition_original.type == TYPE_UNALLOCATED )
		// Paste into unallocated space creating new partition
		insert_new( partitions );
	else
		// Paste into existing partition
		substitute_new( partitions );
}

void OperationCopy::create_description() 
{
	if ( partition_original .type == GParted::TYPE_UNALLOCATED )
	{
		/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd (start at 250 MiB) */
		description = String::ucompose( _("Copy %1 to %2 (start at %3)"),
					        partition_copied .get_path(),
					        device .get_path(),
					        Utils::format_size( partition_new .sector_start, partition_new .sector_size ) ) ;
	}
	else
	{
		/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd1 */
		description = String::ucompose( _("Copy %1 to %2"),
					        partition_copied .get_path(),
						partition_original .get_path() ) ;
	}
}

bool OperationCopy::merge_operations( const Operation & candidate )
{
	return false;  // Never merge copy operations
}

} //GParted
