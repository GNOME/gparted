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
// cache_initialised - Has the cache been loaded?
// mdadm_found       - Is the "mdadm" command available?
// swraid_info_cache - Vector of member information in Linux Software RAID arrays.
//                     Only active arrays have /dev entries.
//                     E.g.
//                     //member     , array     , uuid                                  , label      , active
//                     [{"/dev/sda1", "/dev/md1", "15224a42-c25b-bcd9-15db-60004e5fe53a", "chimney:1", true },
//                      {"/dev/sda2", "/dev/md1", "15224a42-c25b-bcd9-15db-60004e5fe53a", "chimney:1", true },
//                      {"/dev/sda6", ""        , "8dc7483c-d74e-e0a8-b6a8-dc3ca57e43f8", ""         , false},
//                      {"/dev/sdb6", ""        , "8dc7483c-d74e-e0a8-b6a8-dc3ca57e43f8", ""         , false}
//                     ]

// Initialise static data elements
bool SWRaid_Info::cache_initialised = false;
bool SWRaid_Info::mdadm_found = false;
std::vector<SWRaid_Member> SWRaid_Info::swraid_info_cache;

void SWRaid_Info::load_cache()
{
	set_command_found();
	load_swraid_info_cache();
	cache_initialised = true;
}

bool SWRaid_Info::is_member( const Glib::ustring & member_path )
{
	initialise_if_required();
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	if ( memb.member == member_path )
		return true;

	return false;
}

bool SWRaid_Info::is_member_active( const Glib::ustring & member_path )
{
	initialise_if_required();
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	if ( memb.member == member_path )
		return memb.active;

	return false;  // No such member
}

// Return array /dev entry (e.g. "/dev/md1") containing the specified member, or "" if the
// array is not running or there is no such member.
Glib::ustring SWRaid_Info::get_array( const Glib::ustring & member_path )
{
	initialise_if_required();
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	return memb.array;
}

// Return array UUID for the specified member, or "" when failed to parse the UUID or
// there is no such member.
Glib::ustring SWRaid_Info::get_uuid( const Glib::ustring & member_path )
{
	initialise_if_required();
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	return memb.uuid;
}

// Return array label (array name in mdadm terminology) for the specified member, or ""
// when the array has no label or there is no such member.
// (Metadata 0.90 arrays don't have names.  Metata 1.x arrays are always named, getting a
// default of hostname ":" array number when not otherwise specified).
Glib::ustring SWRaid_Info::get_label( const Glib::ustring & member_path )
{
	initialise_if_required();
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	return memb.label;
}

// Private methods

void SWRaid_Info::initialise_if_required()
{
	if ( ! cache_initialised )
	{
		set_command_found();
		load_swraid_info_cache();
		cache_initialised = true;
	}
}

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

	// Load SWRaid members into the cache.  Load member device, array UUID and array
	// label (array name in mdadm terminology).
	Glib::ustring cmd = "mdadm --examine --scan --verbose";
	if ( ! Utils::execute_command( cmd, output, error, true ) )
	{
		// Extract information from Linux Software RAID arrays only, excluding
		// IMSM and DDF arrays.  Example output:
		//     ARRAY metadata=imsm UUID=9a5e3477:e1e668ea:12066a1b:d3708608
		//        devices=/dev/sdd,/dev/sdc,/dev/md/imsm0
		//     ARRAY /dev/md/MyRaid container=9a5e3477:e1e668ea:12066a1b:d3708608 member=0 UUID=47518beb:cc6ef9e7:c80cd1c7:5f6ecb28
		//
		//     ARRAY /dev/md/1  level=raid1 metadata=1.0 num-devices=2 UUID=15224a42:c25bbcd9:15db6000:4e5fe53a name=chimney:1
		//        devices=/dev/sda1,/dev/sdb1
		//     ARRAY /dev/md5 level=raid1 num-devices=2 UUID=8dc7483c:d74ee0a8:b6a8dc3c:a57e43f8
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
		Glib::ustring uuid;
		Glib::ustring label;
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
				{
					// Skip mdadm reported non-Linux Software RAID arrays
					line_type = LINE_TYPE_OTHER;
					continue;
				}

				uuid = mdadm_to_canonical_uuid(
						Utils::regexp_label( lines[i], "UUID=([[:graph:]]+)" ) );
				label = Utils::regexp_label( lines[i], "name=(.*)$" );
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
					memb.array = "";
					memb.uuid = uuid;
					memb.label = label;
					memb.active = false;
					swraid_info_cache.push_back( memb );
				}
				uuid.clear();
				label.clear();
			}
			else
			{
				line_type = LINE_TYPE_OTHER;
			}
		}
	}

	// For active SWRaid members, set array and active flag.
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
						// kernel device name of a member.  Set
						// array and active flag.
						Glib::ustring mpath = "/dev/" +
						                      fields[i].substr( 0, index );
						SWRaid_Member & memb = get_cache_entry_by_member( mpath );
						if ( memb.member == mpath )
						{
							memb.array = "/dev/" + fields[0];
							memb.active = true;
						}
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
	static SWRaid_Member memb = {"", "", "", "", false};
	return memb;
}

// Reformat mdadm printed UUID into canonical format.  Returns "" if source not correctly
// formatted.
// E.g. "15224a42:c25bbcd9:15db6000:4e5fe53a" -> "15224a42-c25b-bcd9-15db-60004e5fe53a"
Glib::ustring SWRaid_Info::mdadm_to_canonical_uuid( const Glib::ustring & mdadm_uuid )
{
	Glib::ustring verified_uuid = Utils::regexp_label( mdadm_uuid,
			"^([[:xdigit:]]{8}:[[:xdigit:]]{8}:[[:xdigit:]]{8}:[[:xdigit:]]{8})$" );
	if ( verified_uuid.empty() )
		return verified_uuid;

	Glib::ustring canonical_uuid = verified_uuid.substr( 0, 8) + "-" +
	                               verified_uuid.substr( 9, 4) + "-" +
	                               verified_uuid.substr(13, 4) + "-" +
	                               verified_uuid.substr(18, 4) + "-" +
	                               verified_uuid.substr(22, 4) + verified_uuid.substr(27, 8);
	return canonical_uuid;
}

} //GParted
