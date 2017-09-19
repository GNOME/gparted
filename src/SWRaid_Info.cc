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

#include "SWRaid_Info.h"
#include "BlockSpecial.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <fstream>

namespace GParted
{

// Data model:
// cache_initialised - Has the cache been loaded?
// mdadm_found       - Is the "mdadm" command available?
// swraid_info_cache - Vector of member information in Linux Software RAID arrays.
//                     Only active arrays have /dev entries.
//                     Notes:
//                     *   BS(member) is short hand for constructor BlockSpecial(member).
//                     *   Array is only displayed as the mount point to the user and
//                         never compared so not constructing BlockSpecial object for it.
//                     E.g.
//                     //member         , array     , uuid                                  , label      , active
//                     [{BS("/dev/sda1)", "/dev/md1", "15224a42-c25b-bcd9-15db-60004e5fe53a", "chimney:1", true },
//                      {BS("/dev/sda2"), "/dev/md1", "15224a42-c25b-bcd9-15db-60004e5fe53a", "chimney:1", true },
//                      {BS("/dev/sda6"), ""        , "8dc7483c-d74e-e0a8-b6a8-dc3ca57e43f8", ""         , false},
//                      {BS("/dev/sdb6"), ""        , "8dc7483c-d74e-e0a8-b6a8-dc3ca57e43f8", ""         , false}
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
	if ( memb.member.m_name.length() > 0 )
		return true;

	return false;
}

// Return member/array active status, or false when there is no such member.
bool SWRaid_Info::is_member_active( const Glib::ustring & member_path )
{
	initialise_if_required();
	const SWRaid_Member & memb = get_cache_entry_by_member( member_path );
	return memb.active;
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

	// Load SWRaid members into the cache.  Load member device, array UUID and array
	// label (array name in mdadm terminology).
	if ( mdadm_found                                                                         &&
	     ! Utils::execute_command( "mdadm --examine --scan --verbose", output, error, true )    )
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
		enum MDADM_LINE_TYPE
		{
			MDADM_LT_OTHER   = 0,
			MDADM_LT_ARRAY   = 1,
			MDADM_LT_DEVICES = 2
		};
		MDADM_LINE_TYPE mdadm_line_type = MDADM_LT_OTHER;
		Glib::ustring uuid;
		Glib::ustring label;
		for ( unsigned int i = 0 ; i < lines.size() ; i ++ )
		{
			Glib::ustring metadata_type;
			if ( lines[i].substr( 0, 6 ) == "ARRAY " )
			{
				mdadm_line_type = MDADM_LT_ARRAY;
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
					mdadm_line_type = MDADM_LT_OTHER;
					continue;
				}

				uuid = mdadm_to_canonical_uuid(
						Utils::regexp_label( lines[i], "UUID=([[:graph:]]+)" ) );
				label = Utils::regexp_label( lines[i], "name=(.*)$" );
			}
			else if ( mdadm_line_type == MDADM_LT_ARRAY                  &&
			          lines[i].find( "devices=" ) != Glib::ustring::npos    )
			{
				mdadm_line_type = MDADM_LT_DEVICES;
				Glib::ustring devices_str = Utils::regexp_label( lines[i],
				                                                 "devices=([[:graph:]]+)" );
				std::vector<Glib::ustring> devices;
				Utils::split( devices_str, devices, "," );
				for ( unsigned int j = 0 ; j < devices.size() ; j ++ )
				{
					SWRaid_Member memb;
					memb.member = BlockSpecial( devices[j] );
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
				mdadm_line_type = MDADM_LT_OTHER;
			}
		}
	}

	// For active SWRaid members, set array and active flag.
	std::string line;
	std::ifstream input( "/proc/mdstat" );
	if ( input )
	{
		// Read /proc/mdstat extracting information for Linux Software RAID arrays
		// only, excluding external IMSM and DDF arrays.  Example /proc/mdstat:
		//     Personalities : [raid1]
		//     md127 : inactive sdd[1](S) sdc[0](S)
		//           6306 blocks super external:imsm
		//
		//     md126 : active raid1 sdc[1] sdd[0]
		//           8383831 blocks super external:/md127/0 [2/2] [UU]
		//
		//     md1 : active raid1 sdb1[3] sda1[2]
		//           524224 blocks super 1.0 [2/2] [UU]
		//
		//     md5 : active raid1 sda6[0] sdb6[1]
		//           524224 blocks [2/2] [UU]
		//
		//     unused devices: <none>
		enum MDSTAT_LINE_TYPE
		{
			MDSTAT_LT_OTHER  = 0,
			MDSTAT_LT_ACTIVE = 1,
			MDSTAT_LT_BLOCKS = 2
		};
		MDSTAT_LINE_TYPE mdstat_line_type = MDSTAT_LT_OTHER;
		Glib::ustring array;
		std::vector<Glib::ustring> members;
		while ( getline( input, line ) )
		{
			if ( line.find( " : active " ) != std::string::npos )
			{
				mdstat_line_type = MDSTAT_LT_ACTIVE;
				// Found a line for an active array.  Split into space
				// separated fields.
				std::vector<Glib::ustring> fields;
				Utils::tokenize( line, fields, " " );
				array = "/dev/" + fields[0];
				members.clear();
				for ( unsigned int i = 0 ; i < fields.size() ; i ++ )
				{
					Glib::ustring::size_type index = fields[i].find( "[" );
					if ( index != Glib::ustring::npos )
					{
						// Field contains an "[" so got a short
						// kernel device name of a member.
						members.push_back( "/dev/" + fields[i].substr( 0, index ) );
					}
				}
			}
			else if ( mdstat_line_type == MDSTAT_LT_ACTIVE         &&
			          line.find( " blocks " ) != std::string::npos    )
			{
				mdstat_line_type = MDSTAT_LT_BLOCKS;
				// Found a blocks line for an array.
				Glib::ustring super_type = Utils::regexp_label( line, "super ([[:graph:]]+)" );

				// Kernel doesn't seem to print the super block type for
				// 0.90 version arrays.  Accept no tag (or empty version)
				// as well as "0.90".
				if ( super_type != ""    && super_type != "0.90" &&
				     super_type != "1.0" && super_type != "1.1"  &&
				     super_type != "1.2"                            )
				{
					// Skip /proc/mdstat reported non-Linux Software RAID arrays
					mdstat_line_type = MDSTAT_LT_OTHER;
					continue;
				}

				for ( unsigned int i = 0 ; i < members.size() ; i ++ )
				{
					SWRaid_Member & memb = get_cache_entry_by_member( members[i] );
					if ( memb.member.m_name.length() > 0 )
					{
						// Update existing cache entry, setting
						// array and active flag.
						memb.array = array;
						memb.active = true;
					}
					else
					{
						// Member not already found in the cache.
						// (Mdadm command possibly missing).
						// Insert cache entry.
						SWRaid_Member new_memb;
						new_memb.member = BlockSpecial( members[i] );
						new_memb.array = array;
						new_memb.uuid = "";
						new_memb.label = "";
						new_memb.active = true;
						swraid_info_cache.push_back( new_memb );
					}
				}
				array.clear();
				members.clear();
			}
			else
			{
				mdstat_line_type = MDSTAT_LT_OTHER;
			}
		}
		input.close();
	}
}

// Perform linear search of the cache to find the matching member.
// Returns found cache entry or not found substitute.
SWRaid_Member & SWRaid_Info::get_cache_entry_by_member( const Glib::ustring & member_path )
{
	BlockSpecial bs = BlockSpecial( member_path );
	for ( unsigned int i = 0 ; i < swraid_info_cache.size() ; i ++ )
	{
		if ( bs == swraid_info_cache[i].member )
			return swraid_info_cache[i];
	}
	static SWRaid_Member memb = {BlockSpecial(), "", "", "", false};
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
