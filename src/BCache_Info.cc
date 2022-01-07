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

#include <glibmm/ustring.h>
#include <glibmm/fileutils.h>


namespace GParted
{


// Return true if this device or partition contains an active bcache component, false
// otherwise.  Equivalent to does the directory /sys/block/DEV[/PTN]/bcache exist?
bool BCache_Info::is_active(const Glib::ustring& device_path, const Glib::ustring& partition_path)
{
	Glib::ustring bcache_path;
	Glib::ustring dev_name = device_path.substr(5);  // Remove leading "/dev/".

	if (device_path == partition_path)
	{
		// Whole drive
		bcache_path = "/sys/block/" + dev_name + "/bcache";
	}
	else
	{
		// Partition on drive
		Glib::ustring ptn_name = partition_path.substr(5);  // Remove leading "/dev/".
		bcache_path = "/sys/block/" + dev_name + "/" + ptn_name + "/bcache";
	}

	return file_test(bcache_path, Glib::FILE_TEST_IS_DIR);
}


} //GParted
