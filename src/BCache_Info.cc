/* Copyright (C) 2022 Mike Fleetwood
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


#include "BCache_Info.h"

#include <libgen.h>  // POSIX version of basename()
#include <unistd.h>
#include <glibmm/ustring.h>
#include <glibmm/fileutils.h>


namespace GParted
{


// Return true if this device or partition contains an active bcache component, false
// otherwise.  Equivalent to does the directory /sys/block/DEV[/PTN]/bcache exist?
bool BCache_Info::is_active(const Glib::ustring& device_path, const Glib::ustring& partition_path)
{
	return Glib::file_test(get_sysfs_bcache_path(device_path, partition_path), Glib::FILE_TEST_IS_DIR);
}


// Return the bcache device name for a registered bcache backing device (active
// component), or an empty string.
// E.g.: ("/dev/sdb", "/dev/sdb1") -> "/dev/bcache0"
Glib::ustring BCache_Info::get_bcache_device(const Glib::ustring& device_path, const Glib::ustring& partition_path)
{
	Glib::ustring sysfs_path = get_sysfs_bcache_path(device_path, partition_path) + "/dev";

	char buf[128];  // Large enough for link target
	                // "../../../../../../../../../../virtual/block/bcache0".
	ssize_t len = readlink(sysfs_path.c_str(), buf, sizeof(buf)-1);
	if (len < 0)
		return Glib::ustring("");
	buf[len] = '\0';

	return "/dev/" + Glib::ustring(basename(buf));
}


// Private methods

Glib::ustring BCache_Info::get_sysfs_bcache_path(const Glib::ustring& device_path, const Glib::ustring& partition_path)
{
	Glib::ustring dev_name = device_path.substr(5);  // Remove leading "/dev/".

	if (device_path == partition_path)
	{
		// Whole drive
		return "/sys/block/" + dev_name + "/bcache";
	}
	else
	{
		// Partition on drive
		Glib::ustring ptn_name = partition_path.substr(5);  // Remove leading "/dev/".
		return "/sys/block/" + dev_name + "/" + ptn_name + "/bcache";
	}
}


}  // namespace GParted
