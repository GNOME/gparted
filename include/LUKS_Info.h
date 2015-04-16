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

#include <glibmm/ustring.h>

namespace GParted
{

class LUKS_Info
{
public:
	static void load_cache();
	static Glib::ustring get_mapping_name( const Glib::ustring & path );
};

}//GParted

#endif /* GPARTED_LUKS_INFO_H */
