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


/* Mount_Info
 *
 * Cache of mounted file systems from /proc/mounts and cache of
 * configured mount points from /etc/fstab.
 */

#ifndef GPARTED_MOUNT_INFO_H
#define GPARTED_MOUNT_INFO_H

#include "BlockSpecial.h"

#include <glibmm/ustring.h>
#include <map>
#include <vector>

namespace GParted
{

class Mount_Info
{
public:
	typedef std::map<BlockSpecial, std::vector<Glib::ustring> > MountMapping;

	static void load_cache();
	static bool is_dev_mounted( const Glib::ustring & path );
	static bool is_dev_mounted( const BlockSpecial & bs );
	static std::vector<Glib::ustring> get_all_mountpoints();
	static const std::vector<Glib::ustring> & get_mounted_mountpoints( const Glib::ustring & path );
	static const std::vector<Glib::ustring> & get_fstab_mountpoints( const Glib::ustring & path );

private:
	static void read_mountpoints_from_file( const Glib::ustring & filename, MountMapping & map );
	static void add_node_and_mountpoint( MountMapping & map,
	                                     Glib::ustring & node,
	                                     Glib::ustring & mountpoint );
	static void read_mountpoints_from_file_swaps( const Glib::ustring & filename,
	                                              MountMapping & map );
	static bool have_rootfs_dev( MountMapping & map );
	static void read_mountpoints_from_mount_command( MountMapping & map );
	static const std::vector<Glib::ustring> & find( const MountMapping & map, const Glib::ustring & path );
};

} //GParted

#endif /* GPARTED_MOUNT_INFO_H */
