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
#include <ctype.h>


namespace GParted
{


//Initialize static data elements
bool Proc_Partitions_Info::proc_partitions_info_cache_initialized = false ;

// Cache of all entries from /proc/partitions.
// E.g. [BlockSpecial("/dev/sda"), BlockSpecial("/dev/sda1"), BlockSpecial("/dev/sda2"), ...]
std::vector<BlockSpecial> Proc_Partitions_Info::all_entries_cache;

// Cache of device names read from /proc/partitions that GParted will present by default
// as partitionable.
// E.g. ["/dev/sda", "/dev/sdb", "/dev/md1"]
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


// E.g. ["/dev/sda", "/tmp/fs.img"] -> ["/dev/sda", "/dev/sda1", "/dev/sda2", "/tmp/fs.img"]
std::vector<Glib::ustring> Proc_Partitions_Info::get_device_and_partition_paths_for(
                const std::vector<Glib::ustring>& device_paths)
{
	initialize_if_required();
	std::vector<Glib::ustring> all_paths;
	for (unsigned int i = 0; i < device_paths.size(); i++)
	{
		all_paths.push_back(device_paths[i]);
		const std::vector<Glib::ustring>& part_paths = get_partition_paths_for(device_paths[i]);
		all_paths.insert(all_paths.end(), part_paths.begin(), part_paths.end());
	}
	return all_paths;
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
	all_entries_cache.clear();
	device_paths_cache.clear();

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

			// Pre-populate BlockSpecial cache to avoid stat() call and save
			// all entries from /proc/partitions for device to partition
			// mapping.
			unsigned long maj;
			unsigned long min;
			if ( sscanf( line.c_str(), "%lu %lu", &maj, &min ) != 2 )
				continue;
			BlockSpecial::register_block_special( "/dev/" + name, maj, min );
			all_entries_cache.push_back(BlockSpecial("/dev/" + name));

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
		return true;

	// Match SD/MMC card whole disk devices names.
	// E.g.: device = mmcblk0 (partition = mmcblk0p1)
	if (Utils::regexp_label(name, "^(mmcblk[0-9]+)$") != "")
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

	// Match block layer cache (bcache) device names.
	// E.g.: device = bcache0 (partition = bcache0p1)
	if (Utils::regexp_label(name, "^(bcache[0-9]+)$") != "")
		return true;

	// Match Network Block Device names.
	// E.g.: device = nbd0 (partition = nbd0p1)
	if (Utils::regexp_label(name, "^(nbd[0-9]+)$") != "")
		return true;

	return false;
}


// Return all partitions for a whole disk device.
// E.g.  get_partitions_for("/dev/sda") -> ["/dev/sda1", "/dev/sda2"]
std::vector<Glib::ustring> Proc_Partitions_Info::get_partition_paths_for(const Glib::ustring& device)
{
	// Assumes that entries from /proc/partitions are:
	// 1. Always ordered with whole device followed by any partitions for that device.
	// 2. Partition names are the whole device names appended with optional "p" and
	//    partition number.
	// This is confirmed by checking Linux kernel block/genhd.c:show_partition()
	// function which prints the content of /proc/partitions.
	// https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/block/genhd.c?h=v5.10.12#n1175

	std::vector<Glib::ustring> partitions;
	BlockSpecial bs = BlockSpecial(device);

	// Find matching whole disk device entry from /proc/partitions cache.
	unsigned int i = 0;
	bool found_device = false;
	for (; i < all_entries_cache.size(); i++)
	{
		if (bs == all_entries_cache[i])
		{
			found_device = true;
			break;
		}
	}
	if (! found_device)
		return partitions;  // Empty vector

	// Find following partition entries from /proc/partitions cache.
	for (i++; i < all_entries_cache.size(); i++)
	{
		if (is_partition_of_device(all_entries_cache[i].m_name, bs.m_name))
			partitions.push_back(all_entries_cache[i].m_name);
		else
			// No more partitions for this whole disk device.
			break;
	}

	return partitions;
}


// True if and only if devname is a leading substring of partname and the remainder
// consists of optional "p" and digits.
// E.g. ("/dev/sda1", "/dev/sda") -> true   ("/dev/md1p2", "/dev/md1") -> true
bool Proc_Partitions_Info::is_partition_of_device(const Glib::ustring& partname, const Glib::ustring& devname)
{
	// Linux kernel function block/genhd.c:disk_name() formats device and partition
	// names written to /proc/partitions.
	// https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/block/genhd.c?h=v5.10.12#n73
	size_t devname_len = devname.length();
	if (partname.compare(0, devname_len, devname) != 0)
		return false;
	size_t i = devname_len;
	if (partname.length() <= i)
		return false;
	if (partname[i] == 'p')
		i ++;
	if (partname.length() <= i)
		return false;
	for (i++; i < partname.length(); i++)
	{
		if (! isdigit(partname[i]))
			return false;
	}

	return true;
}


}  // namespace GParted
