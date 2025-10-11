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

#ifndef GPARTED_OPERATIONNAMEPARTITION_H
#define GPARTED_OPERATIONNAMEPARTITION_H


#include "Device.h"
#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"


namespace GParted
{


class OperationNamePartition: public Operation
{
public:
	OperationNamePartition( const Device & device,
	                        const Partition & partition_orig,
	                        const Partition & partition_new );

	OperationNamePartition(const OperationNamePartition& src) = delete;             // Copy construction prohibited
	OperationNamePartition& operator=(const OperationNamePartition& rhs) = delete;  // Copy assignment prohibited

	void apply_to_visual( PartitionVector & partitions );

private:
	void create_description();
	bool merge_operations( const Operation & candidate );
};


}  // namespace GParted


#endif /* GPARTED_OPERATIONNAMEPARTITION_H */
