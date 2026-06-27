/* Copyright (C) 2012 Mike Fleetwood
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


/* LVM2_Info
 *
 * A persistent cache of information about LVM2 PVs, VGs and LVs that
 * helps to minimize the number of executions of lvm commands used to
 * query their attributes.
 */

#ifndef GPARTED_LVM2_INFO_H
#define GPARTED_LVM2_INFO_H

#include "BlockSpecial.h"
#include "Utils.h"
#include "VGDevice.h"

#include <glibmm/ustring.h>
#include <memory>
#include <vector>


namespace GParted
{


struct LVM2_PV
{
	BlockSpecial  pv_name;
	Byte_Value    pv_size = -1;
	Byte_Value    pv_free = -1;
	Glib::ustring vg_name;
};


struct LVM2_VG
{
	Glib::ustring vg_name;
	Glib::ustring vg_attr;
	Byte_Value    pe_size  = -1;
	Sector        total_pe = -1;
	Sector        free_pe  = -1;
	Glib::ustring uuid;
};


struct LVM2_LV
{
	Glib::ustring vg_name;
	Glib::ustring lv_name;
	Glib::ustring lv_path;
	Byte_Value    lv_size = -1;
	Byte_Value    lv_metadata_size = -1;  // Thin-pool metadata (tmeta) size; -1 if not a thin pool.
	bool          active  = false;
	char          segtype = '\0';
};


class LVM2_Info
{
public:
	static bool is_lvm2_supported();
	static void clear_cache();
	static const Glib::ustring& get_vg_name_for_pv(const Glib::ustring& path);
	static Byte_Value get_pv_size_bytes(const Glib::ustring& path);
	static Byte_Value get_pv_free_bytes(const Glib::ustring& path);
	static bool pv_has_active_lvs(const Glib::ustring& path);
	static std::vector<Glib::ustring> get_vgnames();
	static bool is_useable_vg(const Glib::ustring& vgname);
	static bool is_vg_exported( const Glib::ustring & vgname );
	static bool is_vg_name(const Glib::ustring& vgname);
	static std::vector<Glib::ustring> get_vg_members( const Glib::ustring & vgname );
	static std::vector<Glib::ustring> get_vg_lvs( const Glib::ustring & vgname );
	static Byte_Value get_lv_size_bytes(const Glib::ustring& lv_path);
	static Byte_Value get_lv_metadata_size_bytes(const Glib::ustring& lv_path);
	static bool is_lv_active(const Glib::ustring& lv_path);
	static char get_lv_segtype(const Glib::ustring& lv_path);
	static std::unique_ptr<VGDevice> make_vgdevice(const Glib::ustring& vgname);
	static std::vector<Glib::ustring> get_error_messages( const Glib::ustring & path );

private:
	static void initialize_if_required();
	static void set_command_found();
	static void load_lvm2_info_cache();
	static const LVM2_PV & get_pv_cache_entry_by_name( const Glib::ustring & pvname );
	static const LVM2_VG & get_vg_cache_entry_by_name( const Glib::ustring & vgname );
	static Byte_Value lvm2_size_to_num(const Glib::ustring& str);
	static bool bit_set( const Glib::ustring & attr, unsigned int bit ) ;
	static bool lvm2_info_cache_initialized;
	static bool lvm_found ;
	static std::vector<LVM2_PV> lvm2_pv_cache;
	static std::vector<LVM2_VG> lvm2_vg_cache;
	static std::vector<LVM2_LV> lvm2_lv_cache;
	static std::vector<Glib::ustring> error_messages ;
};


}  // namespace GParted


#endif /* GPARTED_LVM2_INFO_H */
