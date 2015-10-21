/* Copyright (C) 2015 Mike Fleetwood
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

#include "../include/SWRaid_Info.h"

#include "../include/Utils.h"

#include <glibmm/ustring.h>
#include <fstream>

namespace GParted
{

// Data model:
// mdadm_found       - Is the "mdadm" command available?
// swraid_info_cache - Vector of member information in Linux Software RAID arrays.
//                     E.g.
//                     //member     , active
//                     [{"/dev/sda1", true },
//                      {"/dev/sda2", true },
//                      {"/dev/sda6", false},
//                      {"/dev/sdb6", false}
//                     ]

// Initialise static data elements
bool SWRaid_Info::mdadm_found = false;
std::vector<SWRaid_Member> SWRaid_Info::swraid_info_cache;

void SWRaid_Info::load_cache()
{
	set_command_found();
	load_swraid_info_cache();
}

bool SWRaid_Info::is_member( const Glib::ustring & member_path )
{
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	if ( memb.member == member_path )
		return true;

	return false;
}

bool SWRaid_Info::is_member_active( const Glib::ustring & member_path )
{
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	if ( memb.member == member_path )
		return memb.active;

	return false;  // No such member
}

// Private methods

void SWRaid_Info::set_command_found()
{
	mdadm_found = ! Glib::find_program_in_path( "mdadm" ).empty();
}

void SWRaid_Info::load_swraid_info_cache()
{
	Glib::ustring output, error;

	swraid_info_cache.clear();

	if ( ! mdadm_found )
		return;

	// Load SWRaid members into the cache.
	Glib::ustring cmd = "mdadm --examine --scan --verbose";
	if ( ! Utils::execute_command( cmd, output, error, true ) )
	{
		// Extract member /dev entries from Linux Software RAID arrays only,
		// excluding IMSM and DDF arrays.  Example output:
		//     ARRAY metadata=imsm UUID=...
		//        devices=/dev/sdd,/dev/sdc,/dev/md/imsm0
		//     ARRAY /dev/md/MyRaid container=...
		//
		//     ARRAY /dev/md/1  level=raid1 metadata=1.0 num-devices=2 UUID=...
		//        devices=/dev/sda1,/dev/sdb1
		//     ARRAY /dev/md5 level=raid1 num-devices=2 UUID=...
		//        devices=/dev/sda6,/dev/sdb6
		std::vector<Glib::ustring> lines;
		Utils::split( output, lines, "\n" );
		enum LINE_TYPE
		{
			LINE_TYPE_OTHER   = 0,
			LINE_TYPE_ARRAY   = 1,
			LINE_TYPE_DEVICES = 2
		};
		LINE_TYPE line_type = LINE_TYPE_OTHER;
		for ( unsigned int i = 0 ; i < lines.size() ; i ++ )
		{
			Glib::ustring metadata_type;
			if ( lines[i].substr( 0, 6 ) == "ARRAY " )
			{
				line_type = LINE_TYPE_ARRAY;
				Glib::ustring metadata_type = Utils::regexp_label( lines[i],
				                                                   "metadata=([[:graph:]]+)" );
				// Mdadm with these flags doesn't seem to print the
				// metadata tag for 0.90 version arrays.  Accept no tag
				// (or empty version) as well as "0.90".
				if ( metadata_type != ""    && metadata_type != "0.90" &&
				     metadata_type != "1.0" && metadata_type != "1.1"  &&
				     metadata_type != "1.2"                               )
					// Skip mdadm reported non-Linux Software RAID arrays
					line_type = LINE_TYPE_OTHER;
			}
			else if ( line_type == LINE_TYPE_ARRAY                       &&
			          lines[i].find( "devices=" ) != Glib::ustring::npos    )
			{
				line_type = LINE_TYPE_DEVICES;
				Glib::ustring devices_str = Utils::regexp_label( lines[i],
				                                                 "devices=([[:graph:]]+)" );
				std::vector<Glib::ustring> devices;
				Utils::split( devices_str, devices, "," );
				for ( unsigned int j = 0 ; j < devices.size() ; j ++ )
				{
					SWRaid_Member memb;
					memb.member = devices[j];
					memb.active = false;
					swraid_info_cache.push_back( memb );
				}
			}
			else
				line_type = LINE_TYPE_OTHER;
		}
	}

	// Set which SWRaid members are active.
	std::string line;
	std::ifstream input( "/proc/mdstat" );
	if ( input )
	{
		// Read /proc/mdstat extracting members for active arrays, marking them
		// active in the cache.  Example fragment of /proc/mdstat:
		//     md1 : active raid1 sdb1[0] sdb2[1]
		//           1047552 blocks super 1.2 [2/2] [UU]
		while ( getline( input, line ) )
		{
			if ( line.find( " : active " ) != std::string::npos )
			{
				// Found a line for an active array.  Split into space
				// separated fields.
				std::vector<Glib::ustring> fields;
				Utils::tokenize( line, fields, " " );
				for ( unsigned int i = 0 ; i < fields.size() ; i ++ )
				{
					Glib::ustring::size_type index = fields[i].find( "[" );
					if ( index != Glib::ustring::npos )
					{
						// Field contains an "[" so got a short
						// kernel device name of a member.  Mark
						// as active.
						Glib::ustring mpath = "/dev/" +
						                      fields[i].substr( 0, index );
						SWRaid_Member & memb = get_cache_entry_by_member( mpath );
						if ( memb.member == mpath )
							memb.active = true;
					}
				}
			}
		}
		input.close();
	}
}

// Perform linear search of the cache to find the matching member.
// Returns found cache entry or not found substitute.
SWRaid_Member & SWRaid_Info::get_cache_entry_by_member( const Glib::ustring & member_path )
{
	for ( unsigned int i = 0 ; i < swraid_info_cache.size() ; i ++ )
	{
		if ( member_path == swraid_info_cache[i].member )
			return swraid_info_cache[i];
	}
	static SWRaid_Member memb = {"", false};
	return memb;
}

} //GParted
