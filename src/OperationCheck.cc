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


#include "OperationCheck.h"

#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

#include <glib.h>
#include <glibmm/ustring.h>


namespace GParted
{


OperationCheck::OperationCheck( const Device & device, const Partition & partition )
 : Operation(OPERATION_CHECK, device, partition, partition)
{
}


void OperationCheck::apply_to_visual( PartitionVector & partitions )
{
}


void OperationCheck::create_description() 
{
	g_assert(m_partition_original != nullptr);  // Bug: Not initialised by constructor or reset later

	/*TO TRANSLATORS: looks like  Check and repair file system (ext3) on /dev/hda4 */
	m_description = Glib::ustring::compose(_("Check and repair file system (%1) on %2"),
	                                m_partition_original->get_filesystem_string(),
	                                m_partition_original->get_path());
}


bool OperationCheck::merge_operations( const Operation & candidate )
{
	g_assert(m_partition_new != nullptr);  // Bug: Not initialised by constructor or reset later

	if (candidate.m_type == OPERATION_CHECK                    &&
	    *m_partition_new == candidate.get_partition_original()   )
		// No steps required to merge this operation
		return true;

	return false;
}


}  // namespace GParted
