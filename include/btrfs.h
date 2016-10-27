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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#ifndef GPARTED_BTRFS_H
#define GPARTED_BTRFS_H

#include "FileSystem.h"
#include "BlockSpecial.h"
#include "Partition.h"

namespace GParted
{

struct BTRFS_Device
{
	int                       devid;
	std::vector<BlockSpecial> members;
} ;

class btrfs : public FileSystem
{
public:
	FS get_filesystem_support() ;
	void set_used_sectors( Partition & partition ) ;
	bool is_busy( const Glib::ustring & path ) ;
	void read_label( Partition & partition ) ;
	bool write_label( const Partition & partition, OperationDetail & operationdetail ) ;
	void read_uuid( Partition & partition ) ;
	bool write_uuid( const Partition & partition, OperationDetail & operationdetail );
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
	bool resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition );
	bool check_repair( const Partition & partition, OperationDetail & operationdetail ) ;

	static void clear_cache() ;
	static Glib::ustring get_mount_device( const Glib::ustring & path ) ;
	static std::vector<Glib::ustring> get_members( const Glib::ustring & path ) ;

private:
	static const BTRFS_Device & get_cache_entry( const Glib::ustring & path ) ;
	static Byte_Value btrfs_size_to_num( Glib::ustring str, Byte_Value ptn_bytes, bool scale_up ) ;
	static gdouble btrfs_size_max_delta( Glib::ustring str ) ;
	static gdouble btrfs_size_to_gdouble( Glib::ustring str ) ;
};
} //GParted

#endif /* GPARTED_BTRFS_H */
