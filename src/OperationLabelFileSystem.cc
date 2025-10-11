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

#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

#include <glib.h>
#include <glibmm/ustring.h>


namespace GParted
{


OperationLabelFileSystem::OperationLabelFileSystem( const Device & device,
                                                    const Partition & partition_orig,
                                                    const Partition & partition_new )
 : Operation(OPERATION_LABEL_FILESYSTEM, device, partition_orig, partition_new)
{
}


void OperationLabelFileSystem::apply_to_visual( PartitionVector & partitions )
{
	substitute_new( partitions );
}


void OperationLabelFileSystem::create_description()
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if(m_partition_new->get_filesystem_partition().get_filesystem_label().empty())
	{
		/* TO TRANSLATORS: looks like   Clear file system Label on /dev/hda3 */
		m_description = Glib::ustring::compose(_("Clear file system label on %1"),
		                                m_partition_new->get_path());
	}
	else
	{
		/* TO TRANSLATORS: looks like   Set file system label "My Label" on /dev/hda3 */
		m_description = Glib::ustring::compose(_("Set file system label \"%1\" on %2"),
		                                m_partition_new->get_filesystem_partition().get_filesystem_label(),
		                                m_partition_new->get_path());
	}
}


bool OperationLabelFileSystem::merge_operations( const Operation & candidate )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (candidate.m_type == OPERATION_LABEL_FILESYSTEM         &&
	    *m_partition_new == candidate.get_partition_original()   )
	{
		m_partition_new->set_filesystem_label(candidate.get_partition_new().get_filesystem_label());
		create_description();
		return true;
	}

	return false;
}


}  // namespace GParted
