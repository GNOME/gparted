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

#include "../include/LUKS_Info.h"
#include "../include/Utils.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace GParted
{

struct LUKS_Mapping
{
	Glib::ustring name;   // Name of the dm-crypt mapping
	unsigned long major;  // Major device number of the underlying block device
	unsigned long minor;  // Minor device number of the underlying block device
	Glib::ustring path;   // Path of the underlying block device
};

// Cache of active dm-crypt mappings.
// Example entry:
//     {name="sdb6_crypt", major=8, minor=22, path="/dev/sdb6"}
static std::vector<LUKS_Mapping> luks_mapping_cache;

void LUKS_Info::load_cache()
{
	luks_mapping_cache.clear();

	Glib::ustring output;
	Glib::ustring error;
	Utils::execute_command( "dmsetup table --target crypt", output, error, true );
	// dmsetup output is one line per dm-crypt mapping containing fields:
	//     NAME: START LENGTH TARGET CIPHER KEY IVOFFSET DEVPATH OFFSET ...
	// or the text:
	//     No devices found
	// First 4 fields are defined by the dmsetup program by the print call in the
	// _status() function:
	//     https://git.fedorahosted.org/cgit/lvm2.git/tree/tools/dmsetup.c?id=v2_02_118#n1715
	// Field 5 onwards are called parameters and documented in the kernel source:
	//     https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/Documentation/device-mapper/dm-crypt.txt?id=v4.0

	std::vector<Glib::ustring> lines;
	Utils::tokenize( output, lines, "\n" );
	for ( unsigned int i = 0 ; i < lines.size() ; i ++ )
	{
		LUKS_Mapping luks_map;
		std::vector<Glib::ustring> fields;
		Utils::tokenize( lines[i], fields, " " );
		const unsigned DMCRYPT_FIELD_Name    = 0;
		const unsigned DMCRYPT_FIELD_devpath = 7;

		if ( fields.size() <= DMCRYPT_FIELD_devpath )
			continue;

		// Extract LUKS mapping name
		Glib::ustring name = fields[DMCRYPT_FIELD_Name];
		size_t len = name.length();
		if ( len <= 1 || name[len-1] != ':' )
			continue;
		luks_map.name = name.substr( 0, len-1 );

		// Extract LUKS underlying device containing the encrypted data.  May be
		// either a device name (/dev/sda1) or major, minor pair (8:1).
		luks_map.major = 0UL;
		luks_map.minor = 0UL;
		luks_map.path.clear();
		Glib::ustring devpath = fields[DMCRYPT_FIELD_devpath];
		unsigned long maj = 0;
		unsigned long min = 0;
		if ( devpath.length() > 0 && devpath[0] == '/' )
			luks_map.path = devpath;
		else if ( sscanf( devpath.c_str(), "%lu:%lu", &maj, &min ) == 2 )
		{
			luks_map.major = maj;
			luks_map.minor = min;
		}
		else
			continue;

		luks_mapping_cache.push_back( luks_map );
	}
}

// Return name of the active LUKS mapping for the underlying block device path,
// or "" when no such mapping exists.
Glib::ustring LUKS_Info::get_mapping_name( const Glib::ustring & path )
{
	// First scan the cache looking for an underlying block device path match.
	// (Totally in memory)
	for ( unsigned int i = 0 ; i < luks_mapping_cache.size() ; i ++ )
	{
		if ( path == luks_mapping_cache[i].path )
			return luks_mapping_cache[i].name;
	}

	// Second scan the cache looking for an underlying block device major, minor
	// match.  (Requires single stat(2) call per search)
	struct stat sb;
	if ( stat( path.c_str(), &sb ) == 0 && S_ISBLK( sb.st_mode ) )
	{
		unsigned long maj = major( sb.st_rdev );
		unsigned long min = minor( sb.st_rdev );
		for ( unsigned int i = 0 ; i < luks_mapping_cache.size() ; i ++ )
		{
			if ( maj == luks_mapping_cache[i].major && min == luks_mapping_cache[i].minor )
			{
				// Store path in cache to avoid stat() call on subsequent
				// query for the same path.
				luks_mapping_cache[i].path = path;

				return luks_mapping_cache[i].name;
			}
		}
	}

	return "";
}

//Private methods

}//GParted
