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
#include "BlockSpecial.h"
#include "Utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace GParted
{

// Cache of active dm-crypt mappings.
// Example entry:
//     {name="sdb6_crypt", container=BlockSpecial{"/dev/sdb6", 8, 22}, offset=2097152, length=534773760}
std::vector<LUKS_Mapping> LUKS_Info::luks_mapping_cache;

bool LUKS_Info::cache_initialised = false;

void LUKS_Info::clear_cache()
{
	luks_mapping_cache.clear();
	cache_initialised = false;
}

const LUKS_Mapping & LUKS_Info::get_cache_entry( const Glib::ustring & path )
{
	initialise_if_required();
	return get_cache_entry_internal( path );
}

//Private methods

void LUKS_Info::initialise_if_required()
{
	if ( ! cache_initialised )
	{
		load_cache();
		cache_initialised = true;
	}
}

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
		const unsigned DMCRYPT_FIELD_length  = 2;
		const unsigned DMCRYPT_FIELD_devpath = 7;
		const unsigned DMCRYPT_FIELD_offset  = 8;

		if ( fields.size() <= DMCRYPT_FIELD_offset )
			continue;

		// Extract LUKS mapping name
		Glib::ustring name = fields[DMCRYPT_FIELD_Name];
		size_t len = name.length();
		if ( len <= 1 || name[len-1] != ':' )
			continue;
		luks_map.name = name.substr( 0, len-1 );

		// Extract LUKS underlying device containing the encrypted data.  May be
		// either a device name (/dev/sda1) or major, minor pair (8:1).
		luks_map.container = BlockSpecial();
		Glib::ustring devpath = fields[DMCRYPT_FIELD_devpath];
		unsigned long maj = 0UL;
		unsigned long min = 0UL;
		if ( devpath.length() > 0 && devpath[0] == '/' )
			luks_map.container = BlockSpecial( devpath );
		else if ( sscanf( devpath.c_str(), "%lu:%lu", &maj, &min ) == 2 )
		{
			luks_map.container.m_major = maj;
			luks_map.container.m_minor = min;
		}
		else
			continue;

		// Extract LUKS offset and length.  Dm-crypt reports all sizes in units of
		// 512 byte sectors.
		luks_map.offset = -1LL;
		luks_map.length = -1LL;
		Byte_Value offset = atoll( fields[DMCRYPT_FIELD_offset].c_str() );
		Byte_Value length = atoll( fields[DMCRYPT_FIELD_length].c_str() );
		if ( offset > 0LL && length > 0LL )
		{
			luks_map.offset = offset * 512LL;
			luks_map.length = length * 512LL;
		}
		else
			continue;

		luks_mapping_cache.push_back( luks_map );
	}
}

// Return LUKS cache entry for the named underlying block device path,
// or not found substitute when no entry exists.
const LUKS_Mapping & LUKS_Info::get_cache_entry_internal( const Glib::ustring & path )
{
	BlockSpecial bs = BlockSpecial( path );
	for ( unsigned int i = 0 ; i < luks_mapping_cache.size() ; i ++ )
	{
		if ( bs == luks_mapping_cache[i].container )
		{
			// Store underlying block device path in the BlockSpecial object
			// if not already known.  Not required, just for completeness.
			if ( ! luks_mapping_cache[i].container.m_name.length() )
				luks_mapping_cache[i].container.m_name = path;

			return luks_mapping_cache[i];
		}
	}

	static LUKS_Mapping not_found = {"", BlockSpecial(), -1LL, -1LL};
	return not_found;
}

}//GParted
