/* Copyright (C) 2009 Curtis Gedak
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

#include "../include/SWRaid.h"

namespace GParted
{

//Initialize static data elements
bool SWRaid::swraid_cache_initialized = false ;
bool SWRaid::mdadm_found  = false ;
std::vector<Glib::ustring> SWRaid::swraid_devices ;

SWRaid::SWRaid()
{
	//Ensure that cache has been loaded at least once
	if ( ! swraid_cache_initialized )
	{
		swraid_cache_initialized = true ;
		set_commands_found() ;
		load_swraid_cache() ;
	}
}

SWRaid::SWRaid( const bool & do_refresh )
{
	//Ensure that cache has been loaded at least once
	if ( ! swraid_cache_initialized )
	{
		swraid_cache_initialized = true ;
		set_commands_found() ;
		if ( do_refresh == false )
			load_swraid_cache() ;
	}

	if ( do_refresh )
		load_swraid_cache() ;
}

SWRaid::~SWRaid()
{
}

void SWRaid::load_swraid_cache()
{
	//Load data into swraid structures
	Glib::ustring output, error ;
	swraid_devices .clear() ;

	if ( mdadm_found )
	{
		if ( ! Utils::execute_command( "mdadm --examine --scan", output, error, true ) )
		{
			if ( output .size() > 0 )
			{
				std::vector<Glib::ustring> temp_arr ;
				Utils::tokenize( output, temp_arr, "\n" ) ;
				for ( unsigned int k = 0; k < temp_arr .size(); k++ )
				{
					Glib::ustring temp = Utils::regexp_label( output, "^[^/]*(/dev/[^\t ]*)" ) ;
					if ( temp .size() > 0 )
						swraid_devices .push_back( temp ) ;
				}
			}
		}
	}
}

void SWRaid::set_commands_found()
{
	//Set status of commands found 
	mdadm_found = (! Glib::find_program_in_path( "mdadm" ) .empty() ) ;
}

bool SWRaid::is_swraid_supported()
{
	//Determine if Linux software RAID is supported
	return ( mdadm_found ) ;
}

void SWRaid::get_devices( std::vector<Glib::ustring> & device_list )
{
	//Retrieve list of Linux software RAID devices
	device_list .clear() ;

	for ( unsigned int k=0; k < swraid_devices .size(); k++ )
		device_list .push_back( swraid_devices[k] ) ;
}

}//GParted
