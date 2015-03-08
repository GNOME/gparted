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

#include "../include/Utils.h"

namespace GParted
{

class FS_Info
{
public:
	FS_Info() ;
	FS_Info( bool do_refresh ) ;
	~FS_Info() ;
	Glib::ustring get_fs_type( const Glib::ustring & path ) ;
	Glib::ustring get_label( const Glib::ustring & path, bool & found ) ;
	Glib::ustring get_uuid( const Glib::ustring & path ) ;
	Glib::ustring get_path_by_uuid( const Glib::ustring & uuid ) ;
	Glib::ustring get_path_by_label( const Glib::ustring & label ) ;
private:
	void load_fs_info_cache() ;
	void set_commands_found() ;
	Glib::ustring get_device_entry( const Glib::ustring & path ) ;
	static bool fs_info_cache_initialized ;
	static bool blkid_found ;
	static bool need_blkid_vfat_cache_update_workaround;
	static bool vol_id_found ;
	static Glib::ustring fs_info_cache ;
};

}//GParted

#endif /* GPARTED_FS_INFO_H */
