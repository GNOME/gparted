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

#include "../include/OperationCheck.h"

namespace GParted
{

OperationCheck::OperationCheck( const Device & device, const Partition & partition )
{
	type = OPERATION_CHECK ;

	this->device = device.get_copy_without_partitions();
	partition_original = partition ;
}
	
void OperationCheck::apply_to_visual( std::vector<Partition> & partitions ) 
{
}

void OperationCheck::create_description() 
{
	/*TO TRANSLATORS: looks like  Check and repair file system (ext3) on /dev/hda4 */
	description = String::ucompose( _("Check and repair file system (%1) on %2"),
					Utils::get_filesystem_string( partition_original .filesystem ),	
					partition_original .get_path() ) ;
}

bool OperationCheck::merge_operations( const Operation & candidate )
{
	if ( candidate.type     == OPERATION_CHECK              &&
	     partition_original == candidate.partition_original    )
		// No steps required to merge this operation
		return true;

	return false;
}

} //GParted
