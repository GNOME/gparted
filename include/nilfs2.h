/* Copyright (C) 2011 Mike Fleetwood
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


#ifndef GPARTED_NILFS2_H
#define GPARTED_NILFS2_H

#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

class nilfs2 : public FileSystem
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
};

} //GParted

#endif /* GPARTED_NILFS2_H */
