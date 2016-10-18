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

#ifndef GPARTED_OPERATIONDELETE_H
#define GPARTED_OPERATIONDELETE_H

#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

class OperationDelete : public Operation
{
public:
	OperationDelete( const Device & device, const Partition & partition_orig ) ;
	virtual ~OperationDelete();

	void apply_to_visual( PartitionVector & partitions );

private:
	OperationDelete( const OperationDelete & src );              // Not implemented copy constructor
	OperationDelete & operator=( const OperationDelete & rhs );  // Not implemented copy assignment operator

	Partition & get_partition_new();
	const Partition & get_partition_new() const;

	void create_description() ;
	bool merge_operations( const Operation & candidate );
	void remove_original_and_adjacent_unallocated( PartitionVector & partitions, int index_orig );
};

} //GParted

#endif /* GPARTED_OPERATIONDELETE_H */
