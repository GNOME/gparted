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
#include "Utils.h"

#include <gdkmm/pixbuf.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>
#include <vector>
#include <memory>


namespace GParted
{


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
	Operation(OperationType type, const Device& device, const Partition& partition_orig);
	Operation(OperationType type, const Device& device, const Partition& partition_orig,
	                                                    const Partition& partition_new);
	virtual ~Operation() = default;

	Operation(const Operation& src) = delete;             // Copy construction prohibited
	Operation& operator=(const Operation& rhs) = delete;  // Copy assignment prohibited

	Partition & get_partition_original();
	const Partition & get_partition_original() const;
	virtual Partition & get_partition_new();
	const virtual Partition & get_partition_new() const;

	virtual void apply_to_visual( PartitionVector & partitions ) = 0;
	virtual void create_description() = 0 ;
	virtual bool merge_operations( const Operation & candidate ) = 0;

	//public variables
	const OperationType m_type;
	const Device        m_device;

	Glib::RefPtr<Gdk::Pixbuf> m_icon;
	Glib::ustring             m_description;
	OperationDetail           m_operation_detail;

protected:
	int find_index_original( const PartitionVector & partitions );
	int find_index_new( const PartitionVector & partitions );
	void insert_unallocated( PartitionVector & partitions,
	                         Sector start, Sector end, Byte_Value sector_size, bool inside_extended );
	void substitute_new( PartitionVector & partitions );
	void insert_new( PartitionVector & partitions );

	const std::unique_ptr<Partition> m_partition_original;
	std::unique_ptr<Partition>       m_partition_new;
};


typedef std::vector<std::unique_ptr<Operation>> OperationVector;


}  // namespace GParted


#endif /* GPARTED_OPERATION_H */
