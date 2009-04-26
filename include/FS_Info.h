/* Copyright (C) 2008, 2009 Curtis Gedak
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

#ifndef FS_INFO_H_
#define FS_INFO_H_

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
private:
	void load_fs_info_cache() ;
	void set_commands_found() ;
	Glib::ustring get_device_entry( const Glib::ustring & path ) ;
	static bool fs_info_cache_initialized ;
	static bool blkid_found ;
	static bool vol_id_found ;
	static Glib::ustring fs_info_cache ;
};

}//GParted

#endif /*FS_INFO_H_*/
