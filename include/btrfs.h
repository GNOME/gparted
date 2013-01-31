/* Copyright (C) 2009,2010 Luca Bruno <lucab@debian.org>
 * Copyright (C) 2010 Curtis Gedak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#ifndef BTRFS_H_
#define BTRFS_H_

#include "../include/FileSystem.h"

namespace GParted
{
class btrfs : public FileSystem
{
public:
	FS get_filesystem_support() ;
	void set_used_sectors( Partition & partition ) ;
	void read_label( Partition & partition ) ;
	bool write_label( const Partition & partition, OperationDetail & operationdetail ) ;
	void read_uuid( Partition & partition ) ;
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
	bool resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition = false ) ;
	bool check_repair( const Partition & partition, OperationDetail & operationdetail ) ;

private:
	static Byte_Value btrfs_size_to_num( Glib::ustring str, Byte_Value ptn_bytes, bool scale_up ) ;
	static gdouble btrfs_size_max_delta( Glib::ustring str ) ;
	static gdouble btrfs_size_to_gdouble( Glib::ustring str ) ;
};
} //GParted

#endif //BTRFS_H_
