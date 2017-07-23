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

	// Detect old mkudffs prior to version 1.1 by lack of --label option.
	Utils::execute_command( "mkudffs --help", output, error, true );
	old_mkudffs = Utils::regexp_label( output + error, "--label" ).empty();

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
	{
		const Glib::ustring label = new_partition.get_filesystem_label();
		int non_ascii_pos_label = -1;
		int non_latin1_pos_label = -1;
		int pos = 0;

		for ( Glib::ustring::const_iterator it = label.begin(); it != label.end(); ++it )
		{
			if ( *it > 0x7F && non_ascii_pos_label == -1 )
				non_ascii_pos_label = pos;
			if ( *it > 0xFF && non_latin1_pos_label == -1 )
				non_latin1_pos_label = pos;
			if ( non_ascii_pos_label != -1 && non_latin1_pos_label != -1 )
				break;
			++pos;
		}

		// NOTE: mkudffs from udftools prior to version 1.1 damage label if contains
		// non-ASCII characters. So do not allow non-ASCII characters in old mkudffs.
		if ( old_mkudffs && non_ascii_pos_label != -1 )
		{
			operationdetail.add_child( OperationDetail(
			                           _("mkudffs prior to version 1.1 does not support non-ASCII characters in the label."),
			                           STATUS_ERROR ) );
			return false;
		}

		// NOTE: UDF Volume Identifier (--vid) can contain maximally 30 Unicode code
		// points. And if one is above U+FF then only 15. UDF Logical Volume Identifier
		// (--lvid) can contain maximally 126 resp. 63 Unicode code points. To allow
		// long 126 characters in label, UDF Volume Identifier would be truncated.
		// When UDF Volume Identifier or UDF Logical Volume Identifier is too long
		// mkuddfs fail with error.

		// NOTE: According to the OSTA specification, UDF supports only strings
		// encoded in 8-bit or 16-bit OSTA Compressed Unicode format.  They are
		// equivalent to ISO-8859-1 (Latin 1) and UCS-2BE respectively.
		// Conversion from UTF-8 passed on the command line to OSTA format is done
		// by mkudffs.  Strictly speaking UDF does not support UTF-16 as the UDF
		// specification was created before the introduction of UTF-16, but lots
		// of UDF tools are able to decode UTF-16 including UTF-16 Surrogate pairs
		// outside the BMP (Basic Multilingual Plane).

		Glib::ustring vid_arg;
		if ( non_latin1_pos_label > 30 )
			vid_arg = new_partition.get_filesystem_label().substr(0, 30);
		else if ( non_latin1_pos_label > 15 )
			vid_arg = new_partition.get_filesystem_label().substr(0, non_latin1_pos_label-1);
		else if ( non_latin1_pos_label == -1 && label.length() > 30 )
			vid_arg = new_partition.get_filesystem_label().substr(0, 30);
		else if ( label.length() > 15 )
			vid_arg = new_partition.get_filesystem_label().substr(0, 15);
		else
			vid_arg = new_partition.get_filesystem_label();

		label_args = "--lvid=\"" + label + "\" " + "--vid=\"" + vid_arg + "\" ";
	}

	// NOTE: UDF block size must match logical sector size of underlying media.
	Glib::ustring blocksize_args = "--blocksize=" + Utils::num_to_str( new_partition.sector_size ) + " ";

	// TODO: Add GUI option for choosing different optical disks and UDF revision.
	// For now format as UDF revision 2.01 for hard disk media type.
	return ! execute_command( "mkudffs --utf8 --media-type=hd --udfrev=0x201 " +
	                          blocksize_args + label_args + new_partition.get_path(),
	                          operationdetail,
	                          EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

} //GParted
