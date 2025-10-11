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

#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"
#include "Utils.h"

#include <glib.h>
#include <glibmm/ustring.h>


namespace GParted
{


OperationCreate::OperationCreate( const Device & device,
				  const Partition & partition_orig,
				  const Partition & partition_new )
 : Operation(OPERATION_CREATE, device, partition_orig, partition_new)
{
}


void OperationCreate::apply_to_visual( PartitionVector & partitions )
{
	insert_new( partitions );
}


void OperationCreate::create_description() 
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	switch(m_partition_new->type)
	{
		case TYPE_PRIMARY:
			m_description = _("Primary Partition");
			break;
		case TYPE_LOGICAL:
			m_description = _("Logical Partition") ;
			break;	
		case TYPE_EXTENDED:
			m_description = _("Extended Partition");
			break;
	
		default	:
			break;
	}
	/*TO TRANSLATORS: looks like   Create Logical Partition #1 (ntfs, 345 MiB) on /dev/hda */
	m_description = Glib::ustring::compose(_("Create %1 #%2 (%3, %4) on %5"),
	                                m_description,
	                                m_partition_new->partition_number,
	                                m_partition_new->get_filesystem_string(),
	                                Utils::format_size(m_partition_new->get_sector_length(),
	                                                   m_partition_new->sector_size),
	                                m_device.get_path());
}


bool OperationCreate::merge_operations( const Operation & candidate )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (candidate.m_type                          == OPERATION_FORMAT                   &&
	    candidate.get_partition_original().status == STAT_NEW                           &&
	    *m_partition_new                          == candidate.get_partition_original()   )
	{
		// Merge a format operation on a not yet created partition with the
		// earlier operation which will create it.
		m_partition_new.reset(candidate.get_partition_new().clone());
		create_description();
		return true;
	}
	else if (candidate.m_type                          == OPERATION_RESIZE_MOVE              &&
	         candidate.get_partition_original().status == STAT_NEW                           &&
	         *m_partition_new                          == candidate.get_partition_original()   )
	{
		// Merge a resize/move operation on a not yet created partition with the
		// earlier operation which will create it.
		m_partition_new.reset(candidate.get_partition_new().clone());
		create_description();
		return true;
	}

	return false;
}


}  // namespace GParted
