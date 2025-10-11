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


#include "FS_Info.h"
#include "BlockSpecial.h"
#include "Utils.h"

#include <iostream>
#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <vector>


namespace GParted
{


//initialize static data elements
bool FS_Info::fs_info_cache_initialized = false ;
bool FS_Info::blkid_found  = false ;
// Assume workaround is needed just in case determination fails and as
// it only costs a fraction of a second to run blkid command again.
bool FS_Info::need_blkid_vfat_cache_update_workaround = true;

// Vector of file system information.
// E.g.
// (Note BS(path) is a short hand for constructor BlockSpecial(path)).
//     //path           , type         , sec_type, uuid                                    , have_label, label
//     [{BS("/dev/sda") , ""           , ""      , ""                                      , false     , ""          },
//      {BS("/dev/sda1"), "xfs"        , ""      , "f828ee8c-1e16-4ca9-b234-e4949dcd4bd1"  , false     , ""          },
//      {BS("/dev/sda2"), "LVM2_member", ""      , "p31pR5-qPLm-YICz-O09i-sB4u-mAH2-GVSNWG", false     , ""          },
//      {BS("/dev/sdb") , ""           , ""      , ""                                      , false     , ""          },
//      {BS("/dev/sdb1"), "ext3"       , "ext2"  , "f218c3b8-237e-4fbe-92c5-76623bba4062"  , true      , "test-ext3" },
//      {BS("/dev/sdb2"), "vfat"       , "msdos" , "9F87-1061"                             , true      , "TEST-FAT16"},
//      {BS("/dev/sdb3"), ""           , ""      , ""                                      , false     , ""          }
//     ]
std::vector<FS_Entry> FS_Info::fs_info_cache;


void FS_Info::clear_cache()
{
	set_commands_found();
	fs_info_cache.clear();
	fs_info_cache_initialized = true;
}


void FS_Info::load_cache_for_paths(const std::vector<Glib::ustring>& paths)
{
	if (not_initialised_then_error())
		return;

	run_blkid_load_cache(paths);
}


// Retrieve the file system type for the path
Glib::ustring FS_Info::get_fs_type( const Glib::ustring & path )
{
	if (not_initialised_then_error())
		return "";

	const FS_Entry & fs_entry = get_cache_entry_by_path( path );
	Glib::ustring fs_type = fs_entry.type;
	Glib::ustring fs_sec_type = fs_entry.sec_type;

	// If vfat, decide whether fat16 or fat32
	if ( fs_type == "vfat" )
	{
		if ( need_blkid_vfat_cache_update_workaround )
		{
			// Blkid cache does not correctly add and remove SEC_TYPE when
			// overwriting FAT16 and FAT32 file systems with each other, so
			// prevents correct identification.  Run blkid command again,
			// bypassing the the cache to get the correct results.
			Glib::ustring output;
			Glib::ustring error;
			if ( ! Utils::execute_command( "blkid -c /dev/null " + Glib::shell_quote( path ),
			                               output, error, true )                              )
				fs_sec_type = Utils::regexp_label( output, " SEC_TYPE=\"([^\"]*)\"" );
		}
		if ( fs_sec_type == "msdos" )
			fs_type = "fat16";
		else
			fs_type = "fat32";
	}

	return fs_type;
}

// Retrieve the label and set found indicator for the path
Glib::ustring FS_Info::get_label( const Glib::ustring & path, bool & found )
{
	if (not_initialised_then_error())
	{
		found = false;
		return "";
	}

	BlockSpecial bs = BlockSpecial( path );
	for ( unsigned int i = 0 ; i < fs_info_cache.size() ; i ++ )
		if ( bs == fs_info_cache[i].path )
		{
			if ( fs_info_cache[i].have_label || fs_info_cache[i].type == "" )
			{
				// Already have the label or this is a blank cache entry
				// for a whole disk device containing a partition table,
				// so no label.
				found = fs_info_cache[i].have_label;
				return fs_info_cache[i].label;
			}

			// Run blkid to get the label for this one partition, update the
			// cache and return the found label.
			found = run_blkid_update_cache_one_label( fs_info_cache[i] );
			return fs_info_cache[i].label;
		}
	found = false;
	return "";
}

// Retrieve the uuid given for the path
Glib::ustring FS_Info::get_uuid( const Glib::ustring & path )
{
	if (not_initialised_then_error())
		return "";

	const FS_Entry & fs_entry = get_cache_entry_by_path( path );
	return fs_entry.uuid;
}

// Retrieve the path given the uuid
Glib::ustring FS_Info::get_path_by_uuid( const Glib::ustring & uuid )
{
	if (not_initialised_then_error())
		return "";

	for ( unsigned int i = 0 ; i < fs_info_cache.size() ; i ++ )
		if ( uuid == fs_info_cache[i].uuid )
			return fs_info_cache[i].path.m_name;

	return "";
}

// Retrieve the path given the label
Glib::ustring FS_Info::get_path_by_label( const Glib::ustring & label )
{
	if (not_initialised_then_error())
		return "";

	update_fs_info_cache_all_labels();
	for ( unsigned int i = 0 ; i < fs_info_cache.size() ; i ++ )
		if ( label == fs_info_cache[i].label )
			return fs_info_cache[i].path.m_name;

	return "";
}

// Private methods

bool FS_Info::not_initialised_then_error()
{
	if (! fs_info_cache_initialized)
		std::cerr << "GParted Bug: FS_Info (blkid) cache not loaded before use" << std::endl;
	return ! fs_info_cache_initialized;
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
}

const FS_Entry & FS_Info::get_cache_entry_by_path( const Glib::ustring & path )
{
	BlockSpecial bs = BlockSpecial( path );
	for ( unsigned int i = 0 ; i < fs_info_cache.size() ; i ++ )
		if ( bs == fs_info_cache[i].path )
			return fs_info_cache[i];

	static FS_Entry not_found = {BlockSpecial(), "", "", "", false, ""};
	return not_found;
}


void FS_Info::run_blkid_load_cache(const std::vector<Glib::ustring>& paths)
{
	// Parse blkid output line by line extracting mandatory field: path and optional
	// fields: type, sec_type, uuid.  Label is not extracted here because of blkid's
	// default non-reversible encoding of non printable ASCII bytes.
	// Example command:
	//     blkid /dev/sda /dev/sda1 /dev/sda2 /dev/sdb /dev/sdb1 /dev/sdb2 /dev/sdb3
	// Example output:
	//     /dev/sda: PTUUID="5012fb1f" PTTYPE="dos"
	//     /dev/sda1: UUID="f828ee8c-1e16-4ca9-b234-e4949dcd4bd1" TYPE="xfs"
	//     /dev/sda2: UUID="p31pR5-qPLm-YICz-O09i-sB4u-mAH2-GVSNWG" TYPE="LVM2_member"
	//     /dev/sdb: PTUUID="f57595e1-c0ae-40ee-be64-00851b2a9977" PTTYPE="gpt"
	//     /dev/sdb1: LABEL="test-ext3" UUID="f218c3b8-237e-4fbe-92c5-76623bba4062" SEC_TYPE="ext2" TYPE="ext3" PARTUUID="71b3e059-30c5-492e-a526-9251dff7bbeb"
	//     /dev/sdb2: SEC_TYPE="msdos" LABEL="TEST-FAT16" UUID="9F87-1061" TYPE="vfat" PARTUUID="9d07ad9a-d468-428f-9bfd-724f5efae4fb"
	//     /dev/sdb3: PARTUUID="bb8438e1-d9f1-45d3-9888-e990b598900d"

	if (! blkid_found)
		return;

	Glib::ustring cmd = "blkid";
	for (unsigned int i = 0; i < paths.size(); i++)
		cmd.append(" " + Glib::shell_quote(paths[i]));

	Glib::ustring output;
	Glib::ustring error;
	if (Utils::execute_command(cmd, output, error, true) != 0)
		return;

	std::vector<Glib::ustring> lines;
	Utils::split(output, lines, "\n");
	for (unsigned int i = 0; i < lines.size(); i++)
	{
		FS_Entry fs_entry = {BlockSpecial(), "", "", "", false, ""};
		Glib::ustring entry_path = Utils::regexp_label(lines[i], "^(.*): ");
		if (entry_path.length() > 0)
		{
			fs_entry.path     = BlockSpecial(entry_path);
			fs_entry.type     = Utils::regexp_label(lines[i], " TYPE=\"([^\"]*)\"");
			fs_entry.sec_type = Utils::regexp_label(lines[i], " SEC_TYPE=\"([^\"]*)\"");
			fs_entry.uuid     = Utils::regexp_label(lines[i], " UUID=\"([^\"]*)\"");
			fs_info_cache.push_back(fs_entry);
		}
	}

	return;
}


void FS_Info::update_fs_info_cache_all_labels()
{
	if ( ! blkid_found )
		return;

	// For all cache entries which are file systems but don't yet have a label load it
	// now.
	for ( unsigned int i = 0 ; i < fs_info_cache.size() ; i ++ )
		if ( fs_info_cache[i].type != "" && ! fs_info_cache[i].have_label )
			run_blkid_update_cache_one_label( fs_info_cache[i] );
}

bool FS_Info::run_blkid_update_cache_one_label( FS_Entry & fs_entry )
{
	if ( ! blkid_found )
		return false;

	// (#786502) Run a separate blkid execution for a single partition to get the
	// label without blkid's default non-reversible encoding.
	Glib::ustring output;
	Glib::ustring error;
	bool success = ! Utils::execute_command( "blkid -o value -s LABEL " + Glib::shell_quote( fs_entry.path.m_name ),
	                                         output, error, true );
	if ( ! success )
		return false;

	// Output from blkid is either the label with a trailing new line character or
	// zero bytes when the file system has no label.  Update the cache entry in both
	// cases as the label was successfully read even if it didn't exist so is zero
	// characters long.
	fs_entry.have_label = true;
	fs_entry.label = Utils::trim_trailing_new_line(output);
	return true;
}


}  // namespace GParted
