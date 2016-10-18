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

#include "BlockSpecial.h"

#include <glibmm/ustring.h>
#include <vector>

namespace GParted
{

struct SWRaid_Member
{
	BlockSpecial  member;
	Glib::ustring array;
	Glib::ustring uuid;
	Glib::ustring label;
	bool          active;
};

class SWRaid_Info
{
public:
	static void load_cache();
	static bool is_member( const Glib::ustring & member_path );
	static bool is_member_active( const Glib::ustring & member_path );
	static Glib::ustring get_array( const Glib::ustring & member_path );
	static Glib::ustring get_uuid( const Glib::ustring & member_path );
	static Glib::ustring get_label( const Glib::ustring & member_path );

private:
	static void initialise_if_required();
	static void set_command_found();
	static void load_swraid_info_cache();
	static SWRaid_Member & get_cache_entry_by_member( const Glib::ustring & member_path );
	static Glib::ustring mdadm_to_canonical_uuid( const Glib::ustring & mdadm_uuid );

	static bool cache_initialised;
	static bool mdadm_found;
	static std::vector<SWRaid_Member> swraid_info_cache;
};

}//GParted

#endif /* GPARTED_SWRAID_INFO_H */
