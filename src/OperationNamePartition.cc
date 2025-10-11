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

#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

#include <glib.h>
#include <glibmm/ustring.h>


namespace GParted
{


OperationNamePartition::OperationNamePartition( const Device & device,
                                                const Partition & partition_orig,
                                                const Partition & partition_new )
 : Operation(OPERATION_NAME_PARTITION, device, partition_orig, partition_new)
{
}


void OperationNamePartition::apply_to_visual( PartitionVector & partitions )
{
	substitute_new( partitions );
}

void OperationNamePartition::create_description()
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if(m_partition_new->name.empty())
	{
		/* TO TRANSLATORS: looks like   Clear partition name on /dev/hda3 */
		m_description = Glib::ustring::compose(_("Clear partition name on %1"),
		                                m_partition_new->get_path());
	}
	else
	{
		/* TO TRANSLATORS: looks like   Set partition name "My Name" on /dev/hda3 */
		m_description = Glib::ustring::compose(_("Set partition name \"%1\" on %2"),
		                                m_partition_new->name,
		                                m_partition_new->get_path());
	}
}


bool OperationNamePartition::merge_operations( const Operation & candidate )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (candidate.m_type == OPERATION_NAME_PARTITION           &&
	    *m_partition_new == candidate.get_partition_original()   )
	{
		m_partition_new->name = candidate.get_partition_new().name;
		create_description();
		return true;
	}

	return false;
}


}  // namespace GParted
