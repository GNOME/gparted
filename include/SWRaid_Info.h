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


/* SWRaid_Info
 *
 * Cache of information about Linux Software RAID arrays so that the
 * mdadm command only needs to be executed once per refresh.
 */

#ifndef GPARTED_SWRAID_INFO_H
#define GPARTED_SWRAID_INFO_H

#include <glibmm/ustring.h>
#include <vector>

namespace GParted
{

class SWRaid_Info
{
public:
	static void load_cache();
	static bool is_member( const Glib::ustring & member_path );

private:
	static void set_command_found();
	static void load_swraid_info_cache();

	static bool mdadm_found;
	static std::vector<Glib::ustring> swraid_info_cache;
};

}//GParted

#endif /* GPARTED_SWRAID_INFO_H */
