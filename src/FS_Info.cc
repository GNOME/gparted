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

#include "../include/FS_Info.h"

namespace GParted
{

//initialize static data elements
bool FS_Info::fs_info_cache_initialized = false ;
bool FS_Info::blkid_found  = false ;
// Assume workaround is needed just in case determination fails and as
// it only costs a fraction of a second to run blkid command again.
bool FS_Info::need_blkid_vfat_cache_update_workaround = true;
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
		if ( ! Utils::execute_command( "blkid", output, error, true ) )
			fs_info_cache = output ;
		else
			fs_info_cache = "" ;
	}
}

void FS_Info::set_commands_found()
{
	//Set status of commands found 
	blkid_found = (! Glib::find_program_in_path( "blkid" ) .empty() ) ;
	if ( blkid_found )
	{
		// Blkid from util-linux before 2.23 has a cache update bug which prevents
		// correct identification between FAT16 and FAT32 when overwriting one
		// with the other.  Detect the need for a workaround.
		Glib::ustring output, error;
		Utils::execute_command( "blkid -v", output, error, true );
		Glib::ustring blkid_version = Utils::regexp_label( output, "blkid.* ([0-9\\.]+) " );
		int blkid_major_ver = 0;
		int blkid_minor_ver = 0;
		if ( sscanf( blkid_version.c_str(), "%d.%d", &blkid_major_ver, &blkid_minor_ver ) == 2 )
		{
			need_blkid_vfat_cache_update_workaround =
					( blkid_major_ver < 2                              ||
					  ( blkid_major_ver == 2 && blkid_minor_ver < 23 )    );
		}
	}

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
	fs_type     = Utils::regexp_label( dev_path_line, " TYPE=\"([^\"]*)\"" );
	fs_sec_type = Utils::regexp_label( dev_path_line, " SEC_TYPE=\"([^\"]*)\"" );

	//If vfat, decide whether fat16 or fat32
	Glib::ustring output, error;
	if ( fs_type == "vfat" )
	{
		if ( need_blkid_vfat_cache_update_workaround )
		{
			// Blkid cache does not correctly add and remove SEC_TYPE when
			// overwriting FAT16 and FAT32 file systems with each other, so
			// prevents correct identification.  Run blkid command again,
			// bypassing the the cache to get the correct results.
			if ( ! Utils::execute_command( "blkid -c /dev/null " + path, output, error, true ) )
				fs_sec_type = Utils::regexp_label( output, " SEC_TYPE=\"([^\"]*)\"" );
		}
		if ( fs_sec_type == "msdos" )
			fs_type = "fat16" ;
		else
			fs_type = "fat32" ;
	}

	if ( fs_type .empty() && vol_id_found )
	{
		//Retrieve TYPE using vol_id command
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
	if ( Utils::regexp_label( temp, "( LABEL=\")") != "" )
		found = true ;

	//Retrieve LABEL
	label = Utils::regexp_label( temp, " LABEL=\"([^\"]*)\"" );
	return label ;
}

Glib::ustring FS_Info::get_uuid( const Glib::ustring & path )
{
	//Retrieve the line containing the device path
	Glib::ustring temp = get_device_entry( path ) ;

	//Retrieve the UUID
	Glib::ustring uuid = Utils::regexp_label( temp, " UUID=\"([^\"]*)\"" );

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
	Glib::ustring regexp = "^([^:]*):.* UUID=\"" + uuid + "\".*$";
	Glib::ustring path = Utils::regexp_label( fs_info_cache, regexp ) ;

	return path ;
}

Glib::ustring FS_Info::get_path_by_label( const Glib::ustring & label )
{
	//Retrieve the path given the label
	Glib::ustring regexp = "^([^:]*):.* LABEL=\"" + label + "\".*$";
	Glib::ustring path = Utils::regexp_label( fs_info_cache, regexp ) ;

	return path ;
}

}//GParted
