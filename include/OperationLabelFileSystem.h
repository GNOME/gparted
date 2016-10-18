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

#ifndef GPARTED_OPERATIONLABELFILESYSTEM_H
#define GPARTED_OPERATIONLABELFILESYSTEM_H

#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

class OperationLabelFileSystem : public Operation
{
public:
	OperationLabelFileSystem( const Device & device,
	                          const Partition & partition_orig,
	                          const Partition & partition_new );
	virtual ~OperationLabelFileSystem();

	void apply_to_visual( PartitionVector & partitions );

private:
	OperationLabelFileSystem( const OperationLabelFileSystem & src );              // Not implemented copy constructor
	OperationLabelFileSystem & operator=( const OperationLabelFileSystem & rhs );  // Not implemented copy assignment operator

	void create_description() ;
	bool merge_operations( const Operation & candidate );
} ;

} //GParted

#endif /* GPARTED_OPERATIONLABELFILESYSTEM_H */
