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

#ifndef GPARTED_OPERATIONCREATE_H
#define GPARTED_OPERATIONCREATE_H

#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

class OperationCreate : public Operation
{
public:
	OperationCreate( const Device & device,
			 const Partition & partition_orig,
			 const Partition & partition_new ) ;
	virtual ~OperationCreate();

	void apply_to_visual( PartitionVector & partitions );

private:
	OperationCreate( const OperationCreate & src );              // Not implemented copy constructor
	OperationCreate & operator=( const OperationCreate & rhs );  // Not implemented copy assignment operator

	void create_description() ;
	bool merge_operations( const Operation & candidate );
} ;

} //GParted

#endif /* GPARTED_OPERATIONCREATE_H */
