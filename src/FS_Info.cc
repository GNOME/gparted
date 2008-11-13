/* Copyright (C) 2008 Curtis Gedak
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

#include "../include/FS_Info.h"

namespace GParted
{
	
FS_Info::FS_Info()
{
	load_fs_info_cache() ;
}

FS_Info::~FS_Info()
{
}

void FS_Info::load_fs_info_cache()
{
	Glib::ustring output, error ;
	if ( ! Glib::find_program_in_path( "blkid" ) .empty() )
	{
		if ( ! Utils::execute_command( "blkid -c /dev/null", output, error, true ) )
			fs_info_cache = output ;
		else
			fs_info_cache = "" ;
	}
}

Glib::ustring FS_Info::get_label( const Glib::ustring & path, bool & found )
{
	Glib::ustring label = "" ;
	found = false ;

	//Retrieve the line containing the device path
	Glib::ustring regexp = "^" + path + ":([^\n]*)$" ;
	Glib::ustring temp = Utils::regexp_label( fs_info_cache, regexp ) ;
	
	//Set indicator if LABEL found
	if ( Utils::regexp_label( temp, "(LABEL=\")") != "" )
		found = true ;

	//Retrieve LABEL
	label = Utils::regexp_label( temp, "LABEL=\"([^\"]*)\"" ) ;
	return label ;
}

Glib::ustring FS_Info::get_uuid( const Glib::ustring & path )
{
	//Retrieve the line containing the device path
	Glib::ustring regexp = "^" + path + ":([^\n]*)$" ;
	Glib::ustring temp = Utils::regexp_label( fs_info_cache, regexp ) ;

	//Retrieve the UUID
	Glib::ustring uuid = Utils::regexp_label( temp, "UUID=\"([^\"]*)\"" ) ;
	return uuid ;
}

}//GParted
