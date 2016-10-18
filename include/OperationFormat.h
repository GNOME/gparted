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

#ifndef GPARTED_OPERATIONFORMAT_H
#define GPARTED_OPERATIONFORMAT_H

#include "Operation.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

class OperationFormat : public Operation
{
public:
	OperationFormat( const Device & device,
			 const Partition & partition_orig,
			 const Partition & partition_new ) ;
	virtual ~OperationFormat();

	void apply_to_visual( PartitionVector & partitions );

private:
	OperationFormat( const OperationFormat & src );              // Not implemented copy constructor
	OperationFormat & operator=( const OperationFormat & rhs );  // Not implemented copy assignment operator

	void create_description() ;
	bool merge_operations( const Operation & candidate );
} ;

} //GParted

#endif /* GPARTED_OPERATIONFORMAT_H */
