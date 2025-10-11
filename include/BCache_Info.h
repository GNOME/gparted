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


/* BCache_Info
 *
 * Simple module to query very basic information about bcache devices
 * (components).  No caching is performed by this module.
 */

#ifndef GPARTED_BCACHE_INFO_H
#define GPARTED_BCACHE_INFO_H


#include <glibmm/ustring.h>


namespace GParted
{


class BCache_Info
{
public:
	static bool is_active(const Glib::ustring& device_path, const Glib::ustring& partition_path);
	static Glib::ustring get_bcache_device(const Glib::ustring& device_path, const Glib::ustring& partition_path);

private:
	static Glib::ustring get_sysfs_bcache_path(const Glib::ustring& device_path,
	                                           const Glib::ustring& partition_path);
};


}  // namespace GParted


#endif /* GPARTED_BCACHE_INFO_H */
