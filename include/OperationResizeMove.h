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

#ifndef GPARTED_OPERATIONRESIZEMOVE_H
#define GPARTED_OPERATIONRESIZEMOVE_H


#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"


namespace GParted
{


class OperationResizeMove : public Operation
{
public:
	OperationResizeMove( const Device & device,
			     const Partition & partition_orig,
			     const Partition & partition_new ) ;

	OperationResizeMove(const OperationResizeMove& src) = delete;             // Copy construction prohibited
	OperationResizeMove& operator=(const OperationResizeMove& rhs) = delete;  // Copy assignment prohibited

	void apply_to_visual( PartitionVector & partitions );

private:
	void create_description() ;
	bool merge_operations( const Operation & candidate );

	void apply_normal_to_visual( PartitionVector & partitions );
	void apply_extended_to_visual( PartitionVector & partitions );

	void remove_adjacent_unallocated( PartitionVector & partitions, int index_orig );
};


}  // namespace GParted


#endif /* GPARTED_OPERATIONRESIZEMOVE_H */
