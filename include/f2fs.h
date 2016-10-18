/* Copyright (C) 2013 Patrick Verner <exodusrobot@yahoo.com>
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


#ifndef GPARTED_F2FS_H
#define GPARTED_F2FS_H

#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

class f2fs : public FileSystem
{
public:
	FS get_filesystem_support() ;
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
};

} //GParted

#endif /* GPARTED_F2FS_H */
