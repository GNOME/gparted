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


/* LUKS_Info
 *
 * Cache of active Linux kernel Device-mapper encryption mappings.
 * (Named LUKS because only encryption using the LUKS on disk format is
 * recognised and handled).
 */

#ifndef GPARTED_LUKS_INFO_H
#define GPARTED_LUKS_INFO_H

#include "BlockSpecial.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <vector>

namespace GParted
{

struct LUKS_Mapping
{
	Glib::ustring name;       // Name of the dm-crypt mapping
	BlockSpecial  container;  // Underlying block device containing the LUKS mapping
	Byte_Value    offset;     // Offset to the start of the mapping in the underlying block device
	Byte_Value    length;     // Length of the mapping in the underlying block device
};

class LUKS_Info
{
public:
	static void clear_cache();
	static const LUKS_Mapping & get_cache_entry( const Glib::ustring & path );

private:
	static void initialise_if_required();
	static void load_cache();
	static const LUKS_Mapping & get_cache_entry_internal( const Glib::ustring & path );

	static std::vector<LUKS_Mapping> luks_mapping_cache;
	static bool cache_initialised;
};

}//GParted

#endif /* GPARTED_LUKS_INFO_H */
