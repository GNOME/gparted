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

#include "Proc_Partitions_Info.h"
#include "BlockSpecial.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <vector>
#include <fstream>
#include <stdio.h>

namespace GParted
{

//Initialize static data elements
bool Proc_Partitions_Info::proc_partitions_info_cache_initialized = false ;
std::vector<Glib::ustring> Proc_Partitions_Info::device_paths_cache ;

void Proc_Partitions_Info::load_cache()
{
	load_proc_partitions_info_cache();
	proc_partitions_info_cache_initialized = true;
}

const std::vector<Glib::ustring> & Proc_Partitions_Info::get_device_paths()
{
	initialize_if_required();
	return device_paths_cache;
}

// Private Methods

void Proc_Partitions_Info::initialize_if_required()
{
	if ( ! proc_partitions_info_cache_initialized )
	{
		load_proc_partitions_info_cache();
		proc_partitions_info_cache_initialized = true;
	}
}

void Proc_Partitions_Info::load_proc_partitions_info_cache()
{
	device_paths_cache .clear() ;

	std::ifstream proc_partitions( "/proc/partitions" ) ;
	if ( proc_partitions )
	{
		std::string line ;

		while ( getline( proc_partitions, line ) )
		{
			Glib::ustring name = Utils::regexp_label( line,
			        "^[[:blank:]]*[[:digit:]]+[[:blank:]]+[[:digit:]]+[[:blank:]]+[[:digit:]]+[[:blank:]]+([[:graph:]]+)$" );
			if ( name == "" )
				continue;

			// Pre-populate BlockSpecial cache to avoid stat() call for all
			// entries from /proc/partitions.
			unsigned long maj;
			unsigned long min;
			if ( sscanf( line.c_str(), "%lu %lu", &maj, &min ) != 2 )
				continue;
			BlockSpecial::register_block_special( "/dev/" + name, maj, min );

			// Save recognised whole disk device names for later returning as
			// the default GParted partitionable devices.
			if (is_whole_disk_device_name(name))
				device_paths_cache.push_back("/dev/" + name);
		}
		proc_partitions .close() ;
	}
}


// True only for whole disk device names which GParted will present by default as
// partitionable.
bool Proc_Partitions_Info::is_whole_disk_device_name(const Glib::ustring& name)
{
	// Match ordinary whole disk device names.
	// E.g.: device = sda (partition = sda1)
	if (Utils::regexp_label(name, "^([^0-9]+)$") != "")
		return true;

	// Match Linux software RAID (mdadm) device names.
	// E.g.: device = md127 (partition = md127p1)
	if (Utils::regexp_label(name, "^(md[0-9]+)$") != "")

	// Match SD/MMC card whole disk devices names.
	// E.g.: device = mmcblk0 (partition = mmcblk0p1)
	if (Utils::regexp_label(name, "^(md[0-9]+)$") != "")
		return true;

	// Match NVME (Non-Volatile Memory Express) whole disk device names.
	// E.g.: device = nvme0n1 (partition = nvme0n1p1)
	if (Utils::regexp_label(name, "^(nvme[0-9]+n[0-9]+)$") != "")
		return true;

	// Match hardware RAID controller whole disk device names.
	// * HP Smart Array (linux-x.y.z/Documentation/scsi/hpsa.rst).
	//   E.g.: device = cciss/c0d0 (partition = cciss/c0d0p1)
	// * Compaq SMART2 Intelligent Disk Array
	//   (linux-x.y.z/Documentation/admin-guide/devices.txt)
	//   E.g.: device = ida/c0d0 (partition = ida/c0d0p1)
	// * Mylex DAC960/AcceleRAID/eXtremeRAID PCI RAID
	//   (linux-x.y.z/Documentation/admin-guide/devices.txt)
	//   E.g.: device = rd/c0d0 (partition = rd/c0d0p1)
	if (Utils::regexp_label(name, "^([a-z]+/c[0-9]+d[0-9]+)$") != "")
		return true;

	return false;
}


} //GParted
