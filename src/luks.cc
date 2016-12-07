/* Copyright (C) 2015 Mike Fleetwood
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

#include "LUKS_Info.h"
#include "Utils.h"
#include "luks.h"

namespace GParted
{

FS luks::get_filesystem_support()
{
	FS fs;
	fs.filesystem = FS_LUKS;

	fs.busy = FS::EXTERNAL;
	fs.read = FS::EXTERNAL;

	// Setting .copy is just for displaying in the File System Support dialog.
	// (Copying of encrypted content is only performed while open).
	fs.copy = FS::GPARTED;

	fs.online_read = FS::EXTERNAL;

	return fs;
}

bool luks::is_busy( const Glib::ustring & path )
{
	LUKS_Mapping mapping = LUKS_Info::get_cache_entry( path );
	return ! mapping.name.empty();
}

void luks::set_used_sectors( Partition & partition )
{
	// Rational for how used, unused and unallocated are set for LUKS partitions
	//
	// A LUKS formatted partition has metadata at the start, followed by the encrypted
	// data.  The LUKS format only records the offset at which the encrypted data
	// starts.  It does NOT record it's length.  Therefore for inactive LUKS mappings
	// the encrypted data is assumed to extend to the end of the partition.  However
	// an active device-mapper (encrypted) mapping does have a size and can be resized
	// with the "cryptsetup resize" command.
	//
	//  *  Metadata is required and can't be shrunk so treat as used space.
	//  *  Encrypted data, when active, is fully in use by the device-mapper encrypted
	//     mapping so must also be considered used space.
	//  *  Any space after an active mapping, because it has been shrunk, is considered
	//     unallocated.  This matches the equivalent with other file systems.
	//  *  Nothing is considered unused space.
	//
	// Therefore for an active LUKS partition:
	//     used        = LUKS data offset + LUKS data length
	//     unused      = 0
	//     unallocated = remainder
	//
	// And for an inactive LUKS partition:
	//     used        = partition size
	//     unused      = 0
	//     unallocated = 0
	//
	// References:
	// *   LUKS On-Disk Format Specification
	//     https://gitlab.com/cryptsetup/cryptsetup/wikis/LUKS-standard/on-disk-format.pdf
	// *   cryptsetup(8)
	LUKS_Mapping mapping = LUKS_Info::get_cache_entry( partition.get_path() );
	if ( mapping.name.empty() )
	{
		// Inactive LUKS partition
		T = partition.get_sector_length();
		partition.set_sector_usage( T, 0 );
	}
	else
	{
		// Active LUKS partition
		T = Utils::round( ( mapping.offset + mapping.length ) / double(partition.sector_size) );
		partition.set_sector_usage( T, 0 );
	}
}

} //GParted
