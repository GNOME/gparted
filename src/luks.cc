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
#include "../include/luks.h"

namespace GParted
{

FS luks::get_filesystem_support()
{
	FS fs;
	fs.filesystem = FS_LUKS;

	fs.busy = FS::EXTERNAL;

	return fs;
}

bool luks::is_busy( const Glib::ustring & path )
{
	LUKS_Mapping mapping = LUKS_Info::get_cache_entry( path );
	return ! mapping.name.empty();
}

} //GParted
