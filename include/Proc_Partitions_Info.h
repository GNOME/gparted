/* Copyright (C) 2010 Curtis Gedak
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


/* Proc_Partitions_Info
 * 
 * A persistent cache of information from the file /proc/partitions
 * that helps to minimize the number of required disk reads.
 */

#ifndef GPARTED_PROC_PARTITIONS_INFO_H
#define GPARTED_PROC_PARTITIONS_INFO_H

#include <glibmm/ustring.h>
#include <vector>

namespace GParted
{

class Proc_Partitions_Info
{
public:
	static void load_cache();
	static const std::vector<Glib::ustring> & get_device_paths();

private:
	static void initialize_if_required();
	static void load_proc_partitions_info_cache();
	static bool proc_partitions_info_cache_initialized ;
	static std::vector<Glib::ustring> device_paths_cache ;
};

}//GParted

#endif /* GPARTED_PROC_PARTITIONS_INFO_H */
