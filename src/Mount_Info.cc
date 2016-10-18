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
#include "FS_Info.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <mntent.h>
#include <string>
#include <fstream>

namespace GParted
{

// Associative array mapping currently mounted devices to one or more mount points.
// E.g.
//     mount_info[BlockSpecial("/dev/sda1")] -> ["/boot"]
//     mount_info[BlockSpecial("/dev/sda2")] -> [""]  (swap)
//     mount_info[BlockSpecial("/dev/sda3")] -> ["/"]
static Mount_Info::MountMapping mount_info;

// Associative array mapping configured devices to one or more mount points read from
// /etc/fstab.  E.g.
//     fstab_info[BlockSpecial("/dev/sda1")] -> ["/boot"]
//     fstab_info[BlockSpecial("/dev/sda3")] -> ["/"]
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
		std::sort( iter_mp->second.begin(), iter_mp->second.end() );

		iter_mp->second.erase(
				std::unique( iter_mp->second.begin(), iter_mp->second.end() ),
				iter_mp->second.end() );
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

std::vector<Glib::ustring> Mount_Info::get_all_mountpoints()
{
	MountMapping::const_iterator iter_mp;
	std::vector<Glib::ustring> mountpoints;

	for ( iter_mp = mount_info.begin() ; iter_mp != mount_info.end() ; ++ iter_mp )
		mountpoints.insert( mountpoints.end(), iter_mp->second.begin(), iter_mp->second.end() );

	return mountpoints;
}

const std::vector<Glib::ustring> & Mount_Info::get_mounted_mountpoints( const Glib::ustring & path )
{
	return find( mount_info, path );
}

const std::vector<Glib::ustring> & Mount_Info::get_fstab_mountpoints( const Glib::ustring & path )
{
	return find( fstab_info, path );
}

// Private methods

void Mount_Info::read_mountpoints_from_file( const Glib::ustring & filename, MountMapping & map )
{
	FILE* fp = setmntent( filename .c_str(), "r" );
	if ( fp == NULL )
		return;

	struct mntent* p = NULL;
	while ( ( p = getmntent( fp ) ) != NULL )
	{
		Glib::ustring node = p->mnt_fsname;
		Glib::ustring mountpoint = p->mnt_dir;

		Glib::ustring uuid = Utils::regexp_label( node, "^UUID=(.*)" );
		if ( ! uuid.empty() )
			node = FS_Info::get_path_by_uuid( uuid );

		Glib::ustring label = Utils::regexp_label( node, "^LABEL=(.*)" );
		if ( ! label.empty() )
			node = FS_Info::get_path_by_label( label );

		if ( ! node.empty() )
			add_node_and_mountpoint( map, node, mountpoint );
	}

	endmntent( fp );
}

void Mount_Info::add_node_and_mountpoint( MountMapping & map,
                                          Glib::ustring & node,
                                          Glib::ustring & mountpoint )
{
	// Only add node path if mount point exists
	if ( file_test( mountpoint, Glib::FILE_TEST_EXISTS ) )
		map[BlockSpecial( node )].push_back( mountpoint );
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
				map[BlockSpecial( node )].push_back( "" /* no mountpoint for swap */ );
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
		if ( ! iter_mp->second.empty() && iter_mp->second[0] == "/" )
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
			if ( ! node.empty() )
				add_node_and_mountpoint( map, node, mountpoint );
		}
	}
}

const std::vector<Glib::ustring> & Mount_Info::find( const MountMapping & map, const Glib::ustring & path )
{
	MountMapping::const_iterator iter_mp = map.find( BlockSpecial( path ) );
	if ( iter_mp != map.end() )
		return iter_mp->second;

	static std::vector<Glib::ustring> empty;
	return empty;
}
} //GParted
