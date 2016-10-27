/* Copyright (C) 2012 Mike Fleetwood
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


#ifndef GPARTED_LVM2_PV_H
#define GPARTED_LVM2_PV_H

#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

class lvm2_pv : public FileSystem
{
public:
	const Glib::ustring get_custom_text( CUSTOM_TEXT ttype, int index = 0 ) const;
	FS get_filesystem_support() ;
	bool is_busy( const Glib::ustring & path ) ;
	void set_used_sectors( Partition & partition ) ;
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
	bool resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition );
	bool check_repair( const Partition & partition, OperationDetail & operationdetail ) ;
	bool remove( const Partition & partition, OperationDetail & operationdetail ) ;
};

} //GParted

#endif /* GPARTED_LVM2_PV_H */
