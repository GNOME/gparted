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


#include "OperationChangeUUID.h"

#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"
#include "Utils.h"

#include <glib.h>
#include <glibmm/ustring.h>


namespace GParted
{


OperationChangeUUID::OperationChangeUUID( const Device & device
                                        , const Partition & partition_orig
                                        , const Partition & partition_new
                                        )
 : Operation(OPERATION_CHANGE_UUID, device, partition_orig, partition_new)
{
}


void OperationChangeUUID::apply_to_visual( PartitionVector & partitions )
{
	substitute_new( partitions );
}


void OperationChangeUUID::create_description()
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (m_partition_new->get_filesystem_partition().uuid == UUID_RANDOM_NTFS_HALF)
	{
		/*TO TRANSLATORS: looks like   Set half the UUID to a new random value on ntfs file system on /dev/sda1 */
		m_description = Glib::ustring::compose(_("Set half the UUID to a new random value on %1 file system on %2"),
		                                m_partition_new->get_filesystem_string(),
		                                m_partition_new->get_path());
	}
	else
	{
		/*TO TRANSLATORS: looks like   Set a new random UUID on ext4 file system on /dev/sda1 */
		m_description = Glib::ustring::compose(_("Set a new random UUID on %1 file system on %2"),
		                                m_partition_new->get_filesystem_string(),
		                                m_partition_new->get_path());
	}
}


bool OperationChangeUUID::merge_operations( const Operation & candidate )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (candidate.m_type == OPERATION_CHANGE_UUID              &&
	    *m_partition_new == candidate.get_partition_original()   )
	{
		// Changing half the UUID must not override changing all of it
		if ( candidate.get_partition_new().uuid != UUID_RANDOM_NTFS_HALF )
		{
			m_partition_new->uuid = candidate.get_partition_new().uuid;
			create_description();
		}
		// Else no change required to this operation to merge candidate

		return true;
	}

	return false;
}


}  // namespace GParted
