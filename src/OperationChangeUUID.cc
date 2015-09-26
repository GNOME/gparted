/* Copyright (C) 2012 Rogier Goossens
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

#include "../include/OperationChangeUUID.h"

namespace GParted
{

OperationChangeUUID::OperationChangeUUID( const Device & device
                                        , const Partition & partition_orig
                                        , const Partition & partition_new
                                        )
{
	type = OPERATION_CHANGE_UUID ;

	this->device = device.get_copy_without_partitions();
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
}

void OperationChangeUUID::apply_to_visual( std::vector<Partition> & partitions )
{
	substitute_new( partitions );
}

void OperationChangeUUID::create_description()
{
	if ( partition_new .uuid == UUID_RANDOM_NTFS_HALF ) {
		/*TO TRANSLATORS: looks like   Set half the UUID to a new random value on ntfs file system on /dev/sda1 */
		description = String::ucompose( _("Set half the UUID to a new random value on %1 file system on %2")
		                              , Utils::get_filesystem_string( partition_new .filesystem )
		                              , partition_new .get_path()
		                              ) ;
	} else {
		/*TO TRANSLATORS: looks like   Set a new random UUID on ext4 file system on /dev/sda1 */
		description = String::ucompose( _("Set a new random UUID on %1 file system on %2")
		                              , Utils::get_filesystem_string( partition_new .filesystem )
		                              , partition_new .get_path()
		                              ) ;
	}
}

bool OperationChangeUUID::merge_operations( const Operation & candidate )
{
	if ( candidate.type == OPERATION_CHANGE_UUID        &&
	     partition_new  == candidate.partition_original    )
	{
		// Changing half the UUID must not override changing all of it
		if ( candidate.partition_new.uuid != UUID_RANDOM_NTFS_HALF )
		{
			partition_new.uuid = candidate.partition_new.uuid;
			create_description();
		}
		// Else no change required to this operation to merge candidate

		return true;
	}

	return false;
}

} //GParted
