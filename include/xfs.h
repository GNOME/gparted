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


#ifndef GPARTED_XFS_H
#define GPARTED_XFS_H

#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/ustring.h>

namespace GParted
{

class xfs : public FileSystem
{
public:
	FS get_filesystem_support() ;
	void set_used_sectors( Partition & partition ) ;
	void read_label( Partition & partition ) ;
	bool write_label( const Partition & partition, OperationDetail & operationdetail ) ;
	void read_uuid( Partition & partition ) ;
	bool write_uuid( const Partition & partition, OperationDetail & operationdetail ) ;
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
	bool resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition );
	bool copy( const Partition & src_part,
		   Partition & dest_part,
		   OperationDetail & operationdetail ) ;
	bool check_repair( const Partition & partition, OperationDetail & operationdetail ) ;

private:
	bool copy_progress( OperationDetail * operationdetail );

	Byte_Value src_used;             // Used bytes in the source FS of an XFS copy operation
	Glib::ustring dest_mount_point;  // Temporary FS mount point of an XFS copy operation
};

} //GParted

#endif /* GPARTED_XFS_H */
