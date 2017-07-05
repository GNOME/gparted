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

#include "udf.h"
#include "Partition.h"
#include "Utils.h"

namespace GParted
{

const Byte_Value MIN_UDF_BLOCKS = 282;
const Byte_Value MAX_UDF_BLOCKS = (1LL << 32) - 1;

const Byte_Value MIN_UDF_BLOCKSIZE = 512;
const Byte_Value MAX_UDF_BLOCKSIZE = 4096;

FS udf::get_filesystem_support()
{
	FS fs( FS_UDF );

	fs.busy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs.copy = FS::GPARTED;
	fs.online_read = FS::GPARTED;
	fs.MIN = MIN_UDF_BLOCKS * MIN_UDF_BLOCKSIZE;
	fs.MAX = MAX_UDF_BLOCKS * MAX_UDF_BLOCKSIZE;

	if ( ! Glib::find_program_in_path( "mkudffs" ).empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	// NOTE: Other external programs do not exist yet

	return fs;
}

bool udf::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	// NOTE: mkudffs from udftools prior to version 1.1 does not check for partition
	// limits and crashes.
	if ( new_partition.get_sector_length() > MAX_UDF_BLOCKS )
	{
		operationdetail.add_child( OperationDetail( String::ucompose(
		                           _("Partition is too large, maximum size is %1"),
		                           Utils::format_size( MAX_UDF_BLOCKS, new_partition.sector_size ) ),
		                           STATUS_ERROR ) );
		return false;
	}
	else if ( new_partition.get_sector_length() < MIN_UDF_BLOCKS )
	{
		operationdetail.add_child( OperationDetail( String::ucompose(
		                           _("Partition is too small, minimum size is %1"),
		                           Utils::format_size( MIN_UDF_BLOCKS, new_partition.sector_size ) ),
		                           STATUS_ERROR ) );
		return false;
	}

	// NOTE: UDF Logical Volume Identifier (--lvid) represents the label but blkid
	// (from util-linux) prior to version v2.26 used the Volume Identifier (--vid).
	// Therefore for compatibility reasons store label in both locations.
	Glib::ustring label_args;
	if ( ! new_partition.get_filesystem_label().empty() )
		label_args = "--lvid=\"" + new_partition.get_filesystem_label() + "\" " +
		             "--vid=\"" + new_partition.get_filesystem_label() + "\" ";

	// NOTE: UDF block size must match logical sector size of underlying media.
	Glib::ustring blocksize_args = "--blocksize=" + Utils::num_to_str( new_partition.sector_size ) + " ";

	// FIXME: mkudffs from udftools prior to version 1.1 damage label if contains
	// non-ascii characters.

	// TODO: Add GUI option for choosing different optical disks and UDF revision.
	// For now format as UDF revision 2.01 for hard disk media type.
	return ! execute_command( "mkudffs --utf8 --media-type=hd --udfrev=0x201 " +
	                          blocksize_args + label_args + new_partition.get_path(),
	                          operationdetail,
	                          EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

} //GParted
