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


#ifndef GPARTED_REISER4_H
#define GPARTED_REISER4_H

#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

class reiser4 : public FileSystem
{
public:
	FS get_filesystem_support() ;
	void set_used_sectors( Partition & partition ) ;
	void read_label( Partition & partition ) ;
	void read_uuid( Partition & partition ) ;
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
	bool check_repair( const Partition & partition, OperationDetail & operationdetail ) ;
};

} //GParted

#endif /* GPARTED_REISER4_H */
