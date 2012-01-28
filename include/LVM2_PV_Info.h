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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


/* LVM2_PV_Info
 *
 * A persistent cache of information about LVM2 PVs that helps to
 * minimize the number of executions of lvm commands used to query
 * their attributes.
 */

#ifndef LVM2_PV_INFO_H_
#define LVM2_PV_INFO_H_

#include "../include/Utils.h"

namespace GParted
{

class LVM2_PV_Info
{
public:
	LVM2_PV_Info() ;
	LVM2_PV_Info( bool do_refresh ) ;
	~LVM2_PV_Info() ;
	bool is_lvm2_pv_supported() ;
	Glib::ustring get_vg_name( const Glib::ustring & path ) ;
	Byte_Value get_free_bytes( const Glib::ustring & path ) ;
	bool has_active_lvs( const Glib::ustring & path ) ;
private:
	void set_commands_found() ;
	void load_lvm2_pv_info_cache() ;
	Glib::ustring get_pv_attr( const Glib::ustring & path, unsigned int entry ) ;
	static bool lvm2_pv_info_cache_initialized ;
	static bool lvm_found ;
	static bool dmsetup_found ;
	static std::vector<Glib::ustring> lvm2_pv_cache ;
	static Glib::ustring dmsetup_info_cache ;
};

}//GParted

#endif /*LVM2_PV_INFO_H_*/
