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
#include "FileSystem.h"
#include "Partition.h"
#include "Utils.h"

#include <stddef.h>
#include <stdlib.h>
#include <glibmm/ustring.h>

namespace GParted
{

const Byte_Value MIN_UDF_BLOCKS = 300;
const Byte_Value MAX_UDF_BLOCKS = (1LL << 32) - 1;

FS udf::get_filesystem_support()
{
	FS fs( FS_UDF );

	fs.busy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs.copy = FS::GPARTED;
	fs.online_read = FS::GPARTED;

	old_mkudffs = false;
	if ( ! Glib::find_program_in_path( "mkudffs" ).empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;

		// Detect old mkudffs prior to version 1.1 by lack of --label option.
		Utils::execute_command( "mkudffs --help", output, error, true );
		old_mkudffs = Utils::regexp_label( output + error, "--label" ).empty();
	}

	if ( ! Glib::find_program_in_path( "udfinfo" ).empty() )
	{
		fs.read = FS::EXTERNAL;
		fs.read_uuid = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "udflabel" ).empty() )
	{
		fs.read_label = FS::EXTERNAL;
		fs.write_label = FS::EXTERNAL;
		fs.write_uuid = FS::EXTERNAL;
	}

	return fs;
}

FS_Limits udf::get_filesystem_limits( const Partition & partition ) const
{
	if ( partition.filesystem == FS_UDF && partition.fs_block_size > 0 )
		// Resizing existing UDF file system
		return FS_Limits( MIN_UDF_BLOCKS * partition.fs_block_size, MAX_UDF_BLOCKS * partition.fs_block_size );
	else
		// Creating new UDF file system
		return FS_Limits( MIN_UDF_BLOCKS * partition.sector_size  , MAX_UDF_BLOCKS * partition.sector_size );
}

void udf::set_used_sectors( Partition & partition )
{
	exit_status = Utils::execute_command( "udfinfo --utf8 " + Glib::shell_quote( partition.get_path() ),
	                                      output, error, true );
	if ( exit_status != 0 )
	{
		if ( ! output.empty() )
			partition.push_back_message( output );

		if ( ! error.empty() )
			partition.push_back_message( error );
		return;
	}

	// UDF file system block size.  All other values are numbers of blocks.
	const Glib::ustring block_size_str = Utils::regexp_label( output, "^blocksize=([0-9]+)$" );
	// Number of available blocks on block device usable for UDF
	const Glib::ustring blocks_str = Utils::regexp_label( output, "^blocks=([0-9]+)$" );
	// Number of blocks already used for stored files
	const Glib::ustring used_blocks_str = Utils::regexp_label( output, "^usedblocks=([0-9]+)$" );
	// Number of blocks which are free for storing new files
	const Glib::ustring free_blocks_str = Utils::regexp_label( output, "^freeblocks=([0-9]+)$" );
	// Number of blocks which are after the last block used by UDF
	const Glib::ustring behind_blocks_str = Utils::regexp_label( output, "^behindblocks=([0-9]+)$" );

	if ( block_size_str.empty()  || blocks_str.empty()      ||
	     used_blocks_str.empty() || free_blocks_str.empty() || behind_blocks_str.empty() )
		return;

	unsigned long long int block_size = atoll( block_size_str.c_str() );
	unsigned long long int blocks = atoll( blocks_str.c_str() );
	unsigned long long int used_blocks = atoll( used_blocks_str.c_str() );
	unsigned long long int free_blocks = atoll( free_blocks_str.c_str() );
	unsigned long long int behind_blocks = atoll( behind_blocks_str.c_str() );

	// Number of blocks used by UDF
	unsigned long long int udf_blocks = blocks - behind_blocks;

	// Number of used blocks stored in UDF file system does not have to be correct
	if ( used_blocks > udf_blocks )
		used_blocks = udf_blocks;
	if ( free_blocks > udf_blocks - used_blocks )
		free_blocks = 0;

	Sector total_sectors = ( udf_blocks * block_size + partition.sector_size - 1 ) / partition.sector_size;
	Sector free_sectors = ( free_blocks * block_size ) / partition.sector_size;

	partition.set_sector_usage( total_sectors, free_sectors );
	partition.fs_block_size = block_size;
}

void udf::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "udflabel --utf8 " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                           )
	{
		partition.set_filesystem_label( Utils::trim( output ) );
	}
	else
	{
		if ( ! output.empty() )
			partition.push_back_message( output );

		if ( ! error.empty() )
			partition.push_back_message( error );
	}
}

bool udf::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "udflabel --utf8 " + Glib::shell_quote( partition.get_path() ) +
	                          " " + Glib::shell_quote( partition.get_filesystem_label() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

void udf::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( "udfinfo --utf8 " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                          )
	{
		partition.uuid = Utils::regexp_label( output, "^uuid=(.*)$" );
	}
	else
	{
		if ( ! output.empty() )
			partition.push_back_message( output );

		if ( ! error.empty() )
			partition.push_back_message( error );
	}
}

bool udf::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "udflabel --utf8 --uuid=random " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
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

	Glib::ustring label_args;
	if ( ! new_partition.get_filesystem_label().empty() )
	{
		const Glib::ustring label = new_partition.get_filesystem_label();

		// Mkudffs from udftools prior to version 1.1 damages the label if it
		// contains non-ASCII characters.  Therefore do not allow a label with
		// such characters with old versions of mkudffs.
		if ( old_mkudffs && ! contains_only_ascii( label ) )
		{
			operationdetail.add_child( OperationDetail(
				_("mkudffs prior to version 1.1 does not support non-ASCII characters in the label."),
				STATUS_ERROR ) );
			return false;
		}

		// NOTE: According to the OSTA specification, UDF supports only strings
		// encoded in 8-bit or 16-bit OSTA Compressed Unicode format.  They are
		// equivalent to ISO-8859-1 (Latin 1) and UCS-2BE respectively.
		// Conversion from UTF-8 passed on the command line to OSTA format is done
		// by mkudffs.  Strictly speaking UDF does not support UTF-16 as the UDF
		// specification was created before the introduction of UTF-16, but lots
		// of UDF tools are able to decode UTF-16 including UTF-16 Surrogate pairs
		// outside the BMP (Basic Multilingual Plane).
		//
		// The Volume Identifier (--vid) can only contain 30 bytes, either 30
		// ISO-8859-1 (Latin 1) characters or 15 UCS-2BE characters.  Store the
		// most characters possible in the Volume Identifier.  Either up to 15
		// UCS-2BE characters when a character needing 16-bit encoding is found in
		// the first 15 characters, or up to 30 characters when a character
		// needing 16-bit encoding is found in the second 15 characters.
		Glib::ustring vid = label.substr( 0, 30 );
		size_t first_non_latin1_pos = find_first_non_latin1( label );
		if ( first_non_latin1_pos < 15 )
			vid = label.substr( 0, 15 );
		else if ( first_non_latin1_pos < 30 )
			vid = label.substr( 0, first_non_latin1_pos );

		// UDF Logical Volume Identifier (--lvid) represents the label, but blkid
		// (from util-linux) prior to version v2.26 reads the Volume Identifier
		// (--vid).  Therefore for compatibility reasons store the label in both
		// locations.
		label_args = "--lvid=" + Glib::shell_quote( label ) + " --vid=" + Glib::shell_quote( vid ) + " ";
	}

	// NOTE: UDF block size must match logical sector size of underlying media.
	Glib::ustring blocksize_args = "--blocksize=" + Utils::num_to_str( new_partition.sector_size ) + " ";

	// TODO: Add GUI option for choosing different optical disks and UDF revision.
	// For now format as UDF revision 2.01 for hard disk media type.
	return ! execute_command( "mkudffs --utf8 --media-type=hd --udfrev=0x201 " +
	                          blocksize_args + label_args + Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail,
	                          EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

// Private methods

// Return true only if all the characters in the string are ASCII (<= 0x7F), otherwise false.
bool udf::contains_only_ascii( const Glib::ustring & str )
{
	for ( Glib::ustring::const_iterator it = str.begin() ; it != str.end() ; ++it )
	{
		if ( *it > 0x7F )
			return false;
	}
	return true;
}

// Return the offset of the first non-Latin1 character (> 0xFF), or npos when none found.
size_t udf::find_first_non_latin1( const Glib::ustring & str )
{
	size_t pos = 0;
	for ( Glib::ustring::const_iterator it = str.begin() ; it != str.end() ; ++it )
	{
		if ( *it > 0xFF )
			return pos;
		++pos;
	}
	return Glib::ustring::npos;
}

} //GParted
