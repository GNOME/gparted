/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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

#ifndef GPARTED_OPERATION_H
#define GPARTED_OPERATION_H

#include "Device.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{
	//FIXME: stop using GParted:: in front of our own enums.. it's not necessary and clutters the code
enum OperationType {
	OPERATION_DELETE           = 0,
	OPERATION_CHECK            = 1,
	OPERATION_CREATE           = 2,
	OPERATION_RESIZE_MOVE      = 3,
	OPERATION_FORMAT           = 4,
	OPERATION_COPY             = 5,
	OPERATION_LABEL_FILESYSTEM = 6,
	OPERATION_CHANGE_UUID      = 7,
	OPERATION_NAME_PARTITION   = 8
};

class Operation
{
	
public:
	Operation() ;
	virtual ~Operation() {}

	Partition & get_partition_original();
	const Partition & get_partition_original() const;
	virtual Partition & get_partition_new();
	const virtual Partition & get_partition_new() const;

	virtual void apply_to_visual( PartitionVector & partitions ) = 0;
	virtual void create_description() = 0 ;
	virtual bool merge_operations( const Operation & candidate ) = 0;

	//public variables
	Device device ;
	OperationType type ;

	Glib::RefPtr<Gdk::Pixbuf> icon ;
	Glib::ustring description ;

	OperationDetail operation_detail ;

protected:
	int find_index_original( const PartitionVector & partitions );
	int find_index_new( const PartitionVector & partitions );
	void insert_unallocated( PartitionVector & partitions,
	                         Sector start, Sector end, Byte_Value sector_size, bool inside_extended );
	void substitute_new( PartitionVector & partitions );
	void insert_new( PartitionVector & partitions );

	Partition * partition_original;
	Partition * partition_new;

private:
	// Disable compiler generated copy constructor and copy assignment operator by
	// providing private declarations and no definition.  Code which tries to copy
	// construct or copy assign this class will fail to compile.
	// References:
	// *   Disable copy constructor
	//     http://stackoverflow.com/questions/6077143/disable-copy-constructor
	// *   Disable compiler-generated copy-assignment operator [duplicate]
	//     http://stackoverflow.com/questions/7823845/disable-compiler-generated-copy-assignment-operator
	Operation( const Operation & src );              // Not implemented copy constructor
	Operation & operator=( const Operation & rhs );  // Not implemented copy assignment operator
};

} //GParted

#endif /* GPARTED_OPERATION_H */
