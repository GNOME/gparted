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


#ifndef GPARTED_EXT2_H
#define GPARTED_EXT2_H

#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/ustring.h>

namespace GParted
{

class ext2 : public FileSystem
{
	const enum FSType specific_type;
	Glib::ustring dump_cmd;
	Glib::ustring fsck_cmd;
	Glib::ustring image_cmd;
	Glib::ustring label_cmd;
	Glib::ustring mkfs_cmd;
	Glib::ustring resize_cmd;
	Glib::ustring tune_cmd;
public:
	ext2( enum FSType type ) : specific_type( type ), dump_cmd( "" ), fsck_cmd( "" ), image_cmd( "" ),
	                               label_cmd( "" ), mkfs_cmd( "" ), resize_cmd( "" ), tune_cmd( "" ),
	                               force_auto_64bit( false ) {};
	FS get_filesystem_support() ;
	void set_used_sectors( Partition & partition ) ;
	void read_label( Partition & partition ) ;
	bool write_label( const Partition & partition, OperationDetail & operationdetail ) ;
	void read_uuid( Partition & partition ) ;
	bool write_uuid( const Partition & partition, OperationDetail & operationdetail ) ;
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
	bool resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition );
	bool check_repair( const Partition & partition, OperationDetail & operationdetail ) ;
	bool move( const Partition & partition_new,
	           const Partition & partition_old,
	           OperationDetail & operationdetail );
	bool copy( const Partition & partition_new,
	           Partition & partition_old,
	           OperationDetail & operationdetail );

private:
	void resize_progress( OperationDetail *operationdetail );
	void create_progress( OperationDetail *operationdetail );
	void check_repair_progress( OperationDetail *operationdetail );
	void copy_progress( OperationDetail *operationdetail );

	Byte_Value fs_block_size;  // Holds file system block size for the copy_progress() callback
	bool force_auto_64bit;     // Manually setting ext4 64bit feature on creation
};

} //GParted

#endif /* GPARTED_EXT2_H */
