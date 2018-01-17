/* Copyright (C) 2017 Pali Rohár <pali.rohar@gmail.com>
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


#ifndef GPARTED_UDF_H
#define GPARTED_UDF_H

#include "FileSystem.h"
#include "Partition.h"

#include <stddef.h>
#include <glibmm/ustring.h>

namespace GParted
{

class udf : public FileSystem
{
public:
	udf() : old_mkudffs( false ) {};

	FS get_filesystem_support();
	FS_Limits get_filesystem_limits( const Partition & partition ) const;
	void set_used_sectors( Partition & partition );
	void read_label( Partition & partition );
	bool write_label( const Partition & partition, OperationDetail & operationdetail );
	void read_uuid( Partition & partition );
	bool write_uuid( const Partition & partition, OperationDetail & operationdetail );
	bool create( const Partition & new_partition, OperationDetail & operationdetail );

private:
	static bool contains_only_ascii( const Glib::ustring & str );
	static size_t find_first_non_latin1( const Glib::ustring & str );

	bool old_mkudffs;  // Pre 1.1 version of mkudffs
};

} //GParted

#endif /* GPARTED_UDF_H */
