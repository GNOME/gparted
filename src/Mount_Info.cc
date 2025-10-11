/* Copyright (C) 2016 Mike Fleetwood
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


#include "Mount_Info.h"
#include "BlockSpecial.h"
#include "FS_Info.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <glibmm/fileutils.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <mntent.h>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>


namespace GParted
{


// Associative array mapping currently mounted devices to one or more mount points.
// E.g.
//     //                                        readonly, mountpoints
//     mount_info[BlockSpecial("/dev/sda1")] -> {false   , ["/boot"]                       }
//     mount_info[BlockSpecial("/dev/sda2")] -> {false   , [""]                            }  // swap
//     mount_info[BlockSpecial("/dev/sda3")] -> {false   , ["/"]                           }
//     mount_info[BlockSpecial("/dev/sr0")]  -> {true    , ["/run/media/user/GParted-live"]}
static Mount_Info::MountMapping mount_info;

// Associative array mapping configured devices to one or more mount points read from
// /etc/fstab.  E.g.
//     //                                        readonly, mountpoints
//     fstab_info[BlockSpecial("/dev/sda1")] -> {false   ,["/boot"]}
//     fstab_info[BlockSpecial("/dev/sda3")] -> {false   ,["/"]    }
static Mount_Info::MountMapping fstab_info;

void Mount_Info::load_cache()
{
	mount_info.clear();
	fstab_info.clear();

	read_mountpoints_from_file( "/proc/mounts", mount_info );
	read_mountpoints_from_file_swaps( "/proc/swaps", mount_info );

	if ( ! have_rootfs_dev( mount_info ) )
		// Old distributions only contain 'rootfs' and '/dev/root' device names
		// for the / (root) file system in /proc/mounts with '/dev/root' being a
		// block device rather than a symlink to the true device.  This prevents
		// identification, and therefore busy detection, of the device containing
		// the / (root) file system.  Used to read /etc/mtab to get the root file
		// system device name, but this contains an out of date device name after
		// the mounting device has been dynamically removed from a multi-device
		// btrfs, thus identifying the wrong device as busy.  Instead fall back
		// to reading mounted file systems from the output of the mount command,
		// but only when required.
		read_mountpoints_from_mount_command( mount_info );

	read_mountpoints_from_file( "/etc/fstab", fstab_info );

	// Sort the mount points and remove duplicates ... (no need to do this for fstab_info)
	MountMapping::iterator iter_mp;
	for ( iter_mp = mount_info.begin() ; iter_mp != mount_info.end() ; ++ iter_mp )
	{
		std::sort( iter_mp->second.mountpoints.begin(), iter_mp->second.mountpoints.end() );

		iter_mp->second.mountpoints.erase(
				std::unique( iter_mp->second.mountpoints.begin(), iter_mp->second.mountpoints.end() ),
				iter_mp->second.mountpoints.end() );
	}
}

// Return whether the device path, such as /dev/sda3, is mounted or not
bool Mount_Info::is_dev_mounted( const Glib::ustring & path )
{
	return is_dev_mounted( BlockSpecial( path ) );
}

// Return whether the BlockSpecial object, such as {"/dev/sda3", 8, 3}, is mounted or not
bool Mount_Info::is_dev_mounted( const BlockSpecial & bs )
{
	MountMapping::const_iterator iter_mp = mount_info.find( bs );
	return iter_mp != mount_info.end();
}

// Return whether the device path, such as /dev/sda3, is mounted read-only or not
bool Mount_Info::is_dev_mounted_readonly( const Glib::ustring & path )
{
	return is_dev_mounted_readonly( BlockSpecial( path ) );
}

// Return whether the BlockSpecial object, such as {"/dev/sda3", 8, 3}, is mounted read-only or not
bool Mount_Info::is_dev_mounted_readonly( const BlockSpecial & bs )
{
	MountMapping::const_iterator iter_mp = mount_info.find( bs );
	if ( iter_mp == mount_info.end() )
		return false;
	return iter_mp->second.readonly;
}

std::vector<Glib::ustring> Mount_Info::get_all_mountpoints()
{
	MountMapping::const_iterator iter_mp;
	std::vector<Glib::ustring> mountpoints;

	for ( iter_mp = mount_info.begin() ; iter_mp != mount_info.end() ; ++ iter_mp )
		mountpoints.insert( mountpoints.end(),
		                    iter_mp->second.mountpoints.begin(), iter_mp->second.mountpoints.end() );

	return mountpoints;
}

const std::vector<Glib::ustring> & Mount_Info::get_mounted_mountpoints( const Glib::ustring & path )
{
	return find( mount_info, path ).mountpoints;
}

const std::vector<Glib::ustring> & Mount_Info::get_fstab_mountpoints( const Glib::ustring & path )
{
	return find( fstab_info, path ).mountpoints;
}


// Return whether the device path, such as /dev/sda3, is mounted at mount point or not
bool Mount_Info::is_dev_mounted_at(const Glib::ustring& path, const Glib::ustring& mountpoint)
{
	const std::vector<Glib::ustring>& mountpoints = get_mounted_mountpoints(path);
	for (unsigned i = 0; i < mountpoints.size(); i++)
		if (mountpoint == mountpoints[i])
			return true;
	return false;
}


// Private methods

void Mount_Info::read_mountpoints_from_file( const Glib::ustring & filename, MountMapping & map )
{
	FILE* fp = setmntent( filename .c_str(), "r" );
	if (fp == nullptr)
		return;

	struct mntent* p = nullptr;
	while ((p = getmntent(fp)) != nullptr)
	{
		Glib::ustring node = lookup_uuid_or_label(p->mnt_fsname);
		if (node.empty())
			node = p->mnt_fsname;

		Glib::ustring mountpoint = p->mnt_dir;

		add_mountpoint_entry(map, node, parse_readonly_flag(p->mnt_opts), mountpoint);
	}

	endmntent( fp );
}


void Mount_Info::add_mountpoint_entry(MountMapping& map,
                                      const Glib::ustring& node,
                                      bool readonly,
                                      const Glib::ustring& mountpoint)
{
	// Only add node path if mount point exists
	if (Glib::file_test(mountpoint, Glib::FILE_TEST_EXISTS))
	{
		// Map::operator[] default constructs MountEntry for new keys (nodes).
		MountEntry & mountentry = map[BlockSpecial( node )];
		mountentry.readonly = mountentry.readonly || readonly;
		mountentry.mountpoints.push_back( mountpoint );
	}
}

// Parse file system mount options string into read-only boolean
// E.g. "ro,relatime" -> true
//      "rw,seclabel,relatime,attr2,inode64,noquota" -> false
bool Mount_Info::parse_readonly_flag( const Glib::ustring & str )
{
	std::vector<Glib::ustring> mntopts;
	Utils::split( str, mntopts, "," );
	for ( unsigned int i = 0 ; i < mntopts.size() ; i ++ )
	{
		if ( mntopts[i] == "rw" )
			return false;
		else if ( mntopts[i] == "ro" )
			return true;
	}
	return false;  // Default is read-write mount
}

void Mount_Info::read_mountpoints_from_file_swaps( const Glib::ustring & filename, MountMapping & map )
{
	std::string line;
	std::string node;

	std::ifstream file( filename.c_str() );
	if ( file )
	{
		while ( getline( file, line ) )
		{
			node = Utils::regexp_label( line, "^(/[^ ]+)" );
			if ( node.size() > 0 )
				map[BlockSpecial( node )].mountpoints.push_back( "" /* no mountpoint for swap */ );
		}
		file.close();
	}
}

// Return true only if the map contains a device name for the / (root) file system other
// than 'rootfs' and '/dev/root'.
bool Mount_Info::have_rootfs_dev( MountMapping & map )
{
	MountMapping::const_iterator iter_mp;
	for ( iter_mp = mount_info.begin() ; iter_mp != mount_info.end() ; iter_mp ++ )
	{
		if ( ! iter_mp->second.mountpoints.empty() && iter_mp->second.mountpoints[0] == "/" )
		{
			if ( iter_mp->first.m_name != "rootfs" && iter_mp->first.m_name != "/dev/root" )
				return true;
		}
	}
	return false;
}

void Mount_Info::read_mountpoints_from_mount_command( MountMapping & map )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "mount", output, error, true ) )
	{
		std::vector<Glib::ustring> lines;
		Utils::split( output, lines, "\n");
		for ( unsigned int i = 0 ; i < lines .size() ; i ++ )
		{
			// Process line like "/dev/sda3 on / type ext4 (rw)"
			Glib::ustring node = Utils::regexp_label( lines[ i ], "^([^[:blank:]]+) on " );
			Glib::ustring mountpoint = Utils::regexp_label( lines[ i ], "^[^[:blank:]]+ on ([^[:blank:]]+) " );
			Glib::ustring mntopts = Utils::regexp_label( lines[i], " type [^[:blank:]]+ \\(([^\\)]*)\\)" );
			if ( ! node.empty() )
				add_mountpoint_entry(map, node, parse_readonly_flag(mntopts), mountpoint);
		}
	}
}


const MountEntry& Mount_Info::find(MountMapping& map, const Glib::ustring& path)
{
	BlockSpecial bs_path = BlockSpecial(path);

	// 1) Key look up by path.  E.g. BlockSpecial("/dev/sda1").
	MountMapping::const_iterator iter_mp = map.find(bs_path);
	if ( iter_mp != map.end() )
		return iter_mp->second;

	// 2) Not found so iterate over all mount entries resolving UUID= and LABEL=
	//    references; checking after each for the requested mount entry.
	//    (Unresolved UUID= and LABEL= references are added by
	//    read_mountpoints_from_file("/etc/fstab", fstab_info) for open encryption
	//    mappings as the file system details are only added later into the FS_Info
	//    cache by GParted_Core::detect_filesystem_in_encryption_mappings()).
	std::vector<BlockSpecial> ref_nodes;
	for (iter_mp = map.begin(); iter_mp != map.end(); ++iter_mp)
	{
		if (iter_mp->first.m_name.compare(0, 5, "UUID=")  == 0 ||
		    iter_mp->first.m_name.compare(0, 6, "LABEL=") == 0   )
		{
			ref_nodes.push_back(iter_mp->first);
		}
	}
	for (unsigned i = 0; i < ref_nodes.size(); i++)
	{
		Glib::ustring node = lookup_uuid_or_label(ref_nodes[i].m_name);
		if (! node.empty())
		{
			// Insert new mount entry and delete the old one.
			map[BlockSpecial(node)] = map[ref_nodes[i]];
			map.erase(ref_nodes[i]);

			if (BlockSpecial(node) == bs_path)
				// This resolved mount entry is the one being searched for.
				return map[bs_path];
		}
	}

	static MountEntry not_mounted = MountEntry();
	return not_mounted;
}


// Return file system's block device given a UUID=... or LABEL=... reference, or return
// the empty string when not found.
Glib::ustring Mount_Info::lookup_uuid_or_label(const Glib::ustring& ref)
{
	if (ref.compare(0, 5, "UUID=") == 0)
		return FS_Info::get_path_by_uuid(ref.substr(5));

	if (ref.compare(0, 6, "LABEL=") == 0)
		return FS_Info::get_path_by_label(ref.substr(6));

	return "";
}


}  // namespace GParted
