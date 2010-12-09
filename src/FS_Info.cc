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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../include/FS_Info.h"

namespace GParted
{

//initialize static data elements
bool FS_Info::fs_info_cache_initialized = false ;
bool FS_Info::blkid_found  = false ;
bool FS_Info::vol_id_found  = false ;
Glib::ustring FS_Info::fs_info_cache = "";

FS_Info::FS_Info()
{
	//Ensure that cache has been loaded at least once
	if ( ! fs_info_cache_initialized )
	{
		fs_info_cache_initialized = true ;
		set_commands_found() ;
		load_fs_info_cache() ;
	}
}

FS_Info:: FS_Info( bool do_refresh )
{
	//Ensure that cache has been loaded at least once
	if ( ! fs_info_cache_initialized )
	{
		fs_info_cache_initialized = true ;
		set_commands_found() ;
		if ( do_refresh == false )
			load_fs_info_cache() ;
	}

	if ( do_refresh )
		load_fs_info_cache() ;
}

FS_Info::~FS_Info()
{
}

void FS_Info::load_fs_info_cache()
{
	Glib::ustring output, error ;
	if ( blkid_found )
	{
		if ( ! Utils::execute_command( "blkid -c /dev/null", output, error, true ) )
			fs_info_cache = output ;
		else
			fs_info_cache = "" ;
	}
}

void FS_Info::set_commands_found()
{
	//Set status of commands found 
	blkid_found = (! Glib::find_program_in_path( "blkid" ) .empty() ) ;
	vol_id_found = (! Glib::find_program_in_path( "vol_id" ) .empty() ) ;
}

Glib::ustring FS_Info::get_device_entry( const Glib::ustring & path )
{
	//Retrieve the line containing the device path
	Glib::ustring regexp = "^" + path + ":([^\n]*)$" ;
	Glib::ustring entry = Utils::regexp_label( fs_info_cache, regexp ) ;
	return entry;
}

Glib::ustring FS_Info::get_fs_type( const Glib::ustring & path )
{
	Glib::ustring fs_type = "" ;
	Glib::ustring fs_sec_type = "" ;

	//Retrieve the line containing the device path
	Glib::ustring dev_path_line = get_device_entry( path ) ;
	
	//Retrieve TYPE
	fs_type     = Utils::regexp_label( dev_path_line, "[^_]TYPE=\"([^\"]*)\"" ) ;
	fs_sec_type = Utils::regexp_label( dev_path_line, "SEC_TYPE=\"([^\"]*)\"" ) ;

	//If vfat, decide whether fat16 or fat32
	if ( fs_type == "vfat" )
	{
		if ( fs_sec_type == "msdos" )
			fs_type = "fat16" ;
		else
			fs_type = "fat32" ;
	}

	if ( fs_type .empty() && vol_id_found )
	{
		//Retrieve TYPE using vol_id command
		Glib::ustring output, error ;
		if ( ! Utils::execute_command( "vol_id " + path, output, error, true ) )
			fs_type = Utils::regexp_label( output, "ID_FS_TYPE=([^\n]*)" ) ;
	}

	return fs_type ;
}

Glib::ustring FS_Info::get_label( const Glib::ustring & path, bool & found )
{
	Glib::ustring label = "" ;
	found = false ;

	//Retrieve the line containing the device path
	Glib::ustring temp = get_device_entry( path ) ;
	
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
	Glib::ustring temp = get_device_entry( path ) ;

	//Retrieve the UUID
	Glib::ustring uuid = Utils::regexp_label( temp, "UUID=\"([^\"]*)\"" ) ;

	if ( uuid .empty() && vol_id_found )
	{
		//Retrieve UUID using vol_id command
		Glib::ustring output, error ;
		if ( ! Utils::execute_command( "vol_id " + path, output, error, true ) )
		{
			uuid = Utils::regexp_label( output, "ID_FS_UUID=([^\n]*)" ) ;
		}
	}

	return uuid ;
}

Glib::ustring FS_Info::get_path_by_uuid( const Glib::ustring & uuid )
{
	//Retrieve the path given the uuid
	Glib::ustring regexp = "^([^:]*):.*UUID=\"" + uuid + "\".*$" ;
	Glib::ustring path = Utils::regexp_label( fs_info_cache, regexp ) ;

	return path ;
}

Glib::ustring FS_Info::get_path_by_label( const Glib::ustring & label )
{
	//Retrieve the path given the label
	Glib::ustring regexp = "^([^:]*):.*LABEL=\"" + label + "\".*$" ;
	Glib::ustring path = Utils::regexp_label( fs_info_cache, regexp ) ;

	return path ;
}

}//GParted
