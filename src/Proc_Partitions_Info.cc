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
		Glib::ustring device;

		while ( getline( proc_partitions, line ) )
		{
			// Pre-populate BlockSpecial cache with major, minor numbers of
			// all names found from /proc/partitions.
			Glib::ustring name = Utils::regexp_label( line,
			        "^[[:blank:]]*[[:digit:]]+[[:blank:]]+[[:digit:]]+[[:blank:]]+[[:digit:]]+[[:blank:]]+([[:graph:]]+)$" );
			if ( name == "" )
				continue;
			unsigned long maj;
			unsigned long min;
			if ( sscanf( line.c_str(), "%lu %lu", &maj, &min ) != 2 )
				continue;
			BlockSpecial::register_block_special( "/dev/" + name, maj, min );

			// Recognise only whole disk device names, excluding partitions,
			// from /proc/partitions and save in this cache.

			//Whole disk devices are the ones we want.
			//Device names without a trailing digit refer to the whole disk.
			device = Utils::regexp_label( name, "^([^0-9]+)$" );

			//Recognize /dev/md* devices (Linux software RAID - mdadm).
			//E.g., device = /dev/md127, partition = /dev/md127p1
			if ( device == "" )
				device = Utils::regexp_label( name, "^(md[0-9]+)$" );

			//Recognize /dev/mmcblk* devices.
			//E.g., device = /dev/mmcblk0, partition = /dev/mmcblk0p1
			if ( device == "" )
				device = Utils::regexp_label( name, "^(mmcblk[0-9]+)$" );

			// Recognise /dev/nvme*n* devices
			// (Non-Volatile Memory Express devices.  SSD type devices which
			// plug directly into PCIe sockets).
			// E.g., device = /dev/nvme0n1, partition = /dev/nvme0n1p1
			if ( device == "" )
				device = Utils::regexp_label( name, "^(nvme[0-9]+n[0-9]+)$" );

			//Device names that end with a #[^p]# are HP Smart Array Devices (disks)
			//  E.g., device = /dev/cciss/c0d0, partition = /dev/cciss/c0d0p1
			//  (linux-x.y.z/Documentation/blockdev/cciss.txt)
			//Device names for Compaq SMART2 Intelligent Disk Array
			//  E.g., device = /dev/ida/c0d0, partition = /dev/ida/c0d0p1
			//  (linux-x.y.z/Documentation/blockdev/cpqarray.txt)
			//Device names for Mylex DAC960/AcceleRAID/eXtremeRAID PCI RAID
			//  E.g., device = /dev/rd/c0d0, partition = /dev/rd/c0d0p1
			//  (linux-x.y.z/Documentation/blockdev/README.DAC960)
			if ( device == "" )
				device = Utils::regexp_label( name, "^([a-z]+/c[0-9]+d[0-9]+)$" );

			if ( device != "" )
			{
				//add potential device to the list
				device_paths_cache.push_back( "/dev/" + device );
			}
		}
		proc_partitions .close() ;
	}
}

}//GParted
