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

#include "../include/OperationFormat.h"

namespace GParted
{

OperationFormat::OperationFormat( const Device & device,
				  const Partition & partition_orig,
				  const Partition & partition_new )
{
	type = OPERATION_FORMAT ;

	this->device = device.get_copy_without_partitions();
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
}
	
void OperationFormat::apply_to_visual( std::vector<Partition> & partitions ) 
{
	if ( partition_original.whole_device && partition_new.filesystem == FS_CLEARED )
	{
		// Make format to cleared whole disk device file system preview as
		// unallocated device, matching what happens when implemented.
		partitions.clear();

		Partition temp_partition;
		temp_partition.Set_Unallocated( device.get_path(),
		                                true,
		                                0LL,
		                                device.length -1LL,
		                                device.sector_size,
		                                false );
		partitions.push_back( temp_partition );
	}
	else
	{
		substitute_new( partitions );
	}
}

void OperationFormat::create_description() 
{
	/*TO TRANSLATORS: looks like  Format /dev/hda4 as linux-swap */
	description = String::ucompose( _("Format %1 as %2"),
					partition_original .get_path(),
					Utils::get_filesystem_string( partition_new .filesystem ) ) ;
}

bool OperationFormat::merge_operations( const Operation & candidate )
{
	if ( candidate.type == OPERATION_FORMAT             &&
	     partition_new  == candidate.partition_original    )
	{
		partition_new = candidate.partition_new;
		create_description();
		return true;
	}

	return false;
}

} //GParted
