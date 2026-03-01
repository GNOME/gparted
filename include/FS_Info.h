/* Copyright (C) 2008, 2009, 2010 Curtis Gedak
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


#ifndef GPARTED_FS_INFO_H
#define GPARTED_FS_INFO_H


#include "BlockSpecial.h"
#include "Proc_Partitions_Info.h"

#include <glibmm/ustring.h>
#include <vector>


namespace GParted
{


struct FS_Entry
{
	BlockSpecial  path;
	Glib::ustring type;
	Glib::ustring sec_type;
	Glib::ustring uuid;
	bool          have_label;
	Glib::ustring label;
};

class FS_Info
{
public:
	static const Glib::ustring& get_blkid_version_string();
	static void clear_cache();
	static void load_cache_for_device_and_partition_names(
	                        const std::vector<DeviceAndPartitionNames>& dev_ptn_names);
	static void load_cache_for_one_device_name(const Glib::ustring& device_name);
	static Glib::ustring get_fs_type( const Glib::ustring & path );
	static Glib::ustring get_label( const Glib::ustring & path, bool & found );
	static Glib::ustring get_uuid( const Glib::ustring & path );
	static Glib::ustring get_path_by_uuid( const Glib::ustring & uuid );
	static Glib::ustring get_path_by_label( const Glib::ustring & label );

private:
	static bool not_initialised_then_error();
	static void set_command_found();
	static FS_Entry& get_cache_entry_by_path(const Glib::ustring& path);
	static void run_blkid_load_cache(const std::vector<Glib::ustring>& paths);
	static void apply_blkid_whole_drive_zfs_detection_workaround(
	                        const std::vector<DeviceAndPartitionNames>& dev_ptn_names);
	static void update_fs_info_cache_all_labels();
	static bool run_blkid_update_cache_one_label( FS_Entry & fs_entry );

	static bool fs_info_cache_initialized ;
	static bool blkid_found ;
	static Glib::ustring full_blkid_version;
	static bool need_blkid_vfat_cache_update_workaround;
	static std::vector<FS_Entry> fs_info_cache;
};


}  // namespace GParted


#endif /* GPARTED_FS_INFO_H */
