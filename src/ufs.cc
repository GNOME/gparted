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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
 
#include "../include/ufs.h"

namespace GParted
{

FS ufs::get_filesystem_support()
{
	FS fs ;
	
	fs .filesystem = GParted::FS_UFS ;
	
	fs .copy = GParted::FS::GPARTED ;
	fs .move = GParted::FS::GPARTED ;
	
	return fs ;
}

void ufs::set_used_sectors( Partition & partition ) 
{
}

void ufs::read_label( Partition & partition )
{
}

bool ufs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return true ;
}

void ufs::read_uuid( Partition & partition )
{
}

bool ufs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return true ;
}

bool ufs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return true ;
}

bool ufs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	return true ;
}

bool ufs::move( const Partition & partition_new
              , const Partition & partition_old
              , OperationDetail & operationdetail
              )
{
	return true ;
}

bool ufs::copy( const Glib::ustring & src_part_path,
		const Glib::ustring & dest_part_path,
		OperationDetail & operationdetail )
{
	return true ;
}

bool ufs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return true ;
}

} //GParted


