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

#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"
#include "Utils.h"

#include <glib.h>
#include <glibmm/ustring.h>


namespace GParted
{


OperationFormat::OperationFormat( const Device & device,
				  const Partition & partition_orig,
				  const Partition & partition_new )
 : Operation(OPERATION_FORMAT, device, partition_orig, partition_new)
{
}


void OperationFormat::apply_to_visual( PartitionVector & partitions )
{
	g_assert(m_partition_original != nullptr);  // Bug: Not initialised by constructor or reset later
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (m_partition_original->type == TYPE_UNPARTITIONED && m_partition_new->fstype == FS_CLEARED)
	{
		// Make format to cleared whole disk device file system preview as
		// unallocated device, matching what happens when implemented.
		partitions.clear();

		Partition * temp_partition = new Partition();
		temp_partition->set_unpartitioned(m_device.get_path(),
		                                  "",  // Overridden with "unallocated"
		                                  FS_UNALLOCATED,
		                                  m_device.length,
		                                  m_device.sector_size,
		                                  false);
		partitions.push_back_adopt( temp_partition );
	}
	else
	{
		substitute_new( partitions );
	}
}


void OperationFormat::create_description() 
{
	g_assert(m_partition_original != nullptr);  // Bug: Not initialised by constructor or reset later
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	/*TO TRANSLATORS: looks like  Format /dev/hda4 as linux-swap */
	m_description = Glib::ustring::compose(_("Format %1 as %2"),
	                                m_partition_original->get_path(),
	                                m_partition_new->get_filesystem_string());
}


bool OperationFormat::merge_operations( const Operation & candidate )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (candidate.m_type == OPERATION_FORMAT                   &&
	    *m_partition_new == candidate.get_partition_original()   )
	{
		m_partition_new.reset(candidate.get_partition_new().clone());
		create_description();
		return true;
	}

	return false;
}


}  // namespace GParted
