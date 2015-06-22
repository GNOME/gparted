/* Copyright (C) 2004 Bart
 * Copyright (C) 2010 Curtis Gedak
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

#ifndef GPARTED_DEVICE_H
#define GPARTED_DEVICE_H

#include "../include/Partition.h"

namespace GParted
{
	
class Device
{
	 
public:
	Device() ;
	~Device() ;

	Device get_copy_without_partitions() const;
	void add_path( const Glib::ustring & path, bool clear_paths = false ) ;
	void add_paths( const std::vector<Glib::ustring> & paths, bool clear_paths = false ) ;
	Glib::ustring get_path() const ;
	std::vector<Glib::ustring> get_paths() const ;
	void enable_partition_naming( int length );  // > 0 => enable partition naming support
	bool partition_naming_supported() const;
	int get_max_partition_name_length() const;

	bool operator==( const Device & device ) const ;
	bool operator!=( const Device & device ) const ;
	
	void Reset() ;
	std::vector<Partition> partitions ;
	Sector length;
	Sector heads ;
	Sector sectors ;
	Sector cylinders ;
	Sector cylsize ;
	Glib::ustring model;
	Glib::ustring serial_number;
 	Glib::ustring disktype;
	int sector_size ;
	int max_prims ;
	int highest_busy ;
	bool readonly ; 

private:
	void sort_paths_and_remove_duplicates() ;

	static bool compare_paths( const Glib::ustring & A, const Glib::ustring & B ) ;
	
	std::vector<Glib::ustring> paths ;
	int max_partition_name_length;  // > 0 => naming of partitions is supported on this device
};
 
} //GParted

#endif /* GPARTED_DEVICE_H */
