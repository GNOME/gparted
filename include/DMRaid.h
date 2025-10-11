/* Copyright (C) 2009, 2010, 2011 Curtis Gedak
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

/* READ THIS!
 * This class was created in an effort to reduce the complexity of the
 * GParted_Core class.
 * This class provides support for motherboard based RAID devices, also know
 * as Fake RAID.
 * Static elements are used in order to reduce the disk accesses required to
 * load the data structures upon each initialization of the class.
 */

#ifndef GPARTED_DMRAID_H
#define GPARTED_DMRAID_H

#include "Utils.h"
#include "BlockSpecial.h"
#include "Partition.h"
#include "OperationDetail.h"

#include <glibmm/ustring.h>
#include <vector>


namespace GParted
{


struct DMRaid_Member
{
	BlockSpecial  member;
	Glib::ustring array;
};


class DMRaid
{
public:
	static void load_cache();
	static bool is_dmraid_supported();
	static bool is_dmraid_device(const Glib::ustring& dev_path);
	static std::vector<Glib::ustring> get_devices();
	static Glib::ustring get_dmraid_name(const Glib::ustring& dev_path);
	static Glib::ustring make_path_dmraid_compatible(Glib::ustring partition_path);
	static bool create_dev_map_entries(const Partition& partition, OperationDetail& operationdetail);
	static bool create_dev_map_entries(const Glib::ustring& dev_path);
	static bool delete_affected_dev_map_entries(const Partition& partition, OperationDetail& operationdetail);
	static bool purge_dev_map_entries(const Glib::ustring& dev_path);
	static bool update_dev_map_entry(const Partition& partition, OperationDetail& operationdetail);
	static bool is_member(const Glib::ustring& member_path);
	static bool is_member_active(const Glib::ustring& member_path);
	static const Glib::ustring& get_array(const Glib::ustring& member_path);

private:
	static void load_dmraid_cache();
	static void set_commands_found();
	static std::vector<Glib::ustring> get_dmraid_dir_entries(const Glib::ustring& dev_path);
	static int get_partition_number(const Glib::ustring& partition_name);
	static Glib::ustring get_udev_dm_name(const Glib::ustring& dev_path);
	static bool delete_dev_map_entry(const Partition& partition, OperationDetail& operationdetail);
	static std::vector<Glib::ustring> get_affected_dev_map_entries(const Partition& partition);
	static std::vector<Glib::ustring> get_partition_dev_map_entries(const Partition& partition);
	static std::vector<Glib::ustring> lookup_dmraid_members(const Glib::ustring& array);
	static const DMRaid_Member& get_cache_entry_by_member(const Glib::ustring& member_path);

	static bool dmraid_cache_initialized ;
	static bool dmraid_found ;
	static bool dmsetup_found ;
	static bool udevadm_found ;
	static std::vector<Glib::ustring> dmraid_devices ;
	static std::vector<DMRaid_Member> dmraid_member_cache;
};


}  // namespace GParted


#endif /* GPARTED_DMRAID_H */
