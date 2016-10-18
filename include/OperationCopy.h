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

#ifndef GPARTED_OPERATIONCOPY_H
#define GPARTED_OPERATIONCOPY_H

#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

class OperationCopy : public Operation
{
public:
	OperationCopy( const Device & device,
		       const Partition & partition_orig,
		       const Partition & partition_new,
		       const Partition & partition_copied ) ;
	virtual ~OperationCopy();

	Partition & get_partition_copied();
	const Partition & get_partition_copied() const;

	void apply_to_visual( PartitionVector & partitions );

private:
	OperationCopy( const OperationCopy & src );              // Not implemented copy constructor
	OperationCopy & operator=( const OperationCopy & rhs );  // Not implemented copy assignment operator

	void create_description() ;
	bool merge_operations( const Operation & candidate );

	Partition * partition_copied;
};

} //GParted

#endif /* GPARTED_OPERATIONCOPY_H */
