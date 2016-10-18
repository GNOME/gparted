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

#include "Device.h"

namespace GParted
{

Device::Device()
{
	Reset() ;	
}

void Device::Reset()
{
	path.clear();
	partitions .clear() ;
	length = cylsize = 0 ;
	heads = sectors = cylinders = 0 ;
	model = "";
	serial_number = "";
	disktype = "";
	sector_size = max_prims = highest_busy = 0 ;
	readonly = false ; 	
	max_partition_name_length = 0;
}

Device Device::get_copy_without_partitions() const
{
	Device new_device;                                    // (1) Construct new Device object.
	new_device.length                    = this->length;  // (2) Copy all members except partitions.
	new_device.heads                     = this->heads;
	new_device.sectors                   = this->sectors;
	new_device.cylinders                 = this->cylinders;
	new_device.cylsize                   = this->cylsize;
	new_device.model                     = this->model;
	new_device.disktype                  = this->disktype;
	new_device.sector_size               = this->sector_size;
	new_device.max_prims                 = this->max_prims;
	new_device.highest_busy              = this->highest_busy;
	new_device.readonly                  = this->readonly;
	new_device.path                      = this->path;
	new_device.max_partition_name_length = this->max_partition_name_length;
	return new_device;                                    // (3) Return by value.
}

void Device::set_path( const Glib::ustring & path )
{
	this->path = path;
}

Glib::ustring Device::get_path() const
{
	return path;
}

void Device::enable_partition_naming( int max_length )
{
	if ( max_length > 0 )
		max_partition_name_length = max_length;
	else
		max_partition_name_length = 0;
}

bool Device::partition_naming_supported() const
{
	return max_partition_name_length > 0;
}

int Device::get_max_partition_name_length() const
{
	return max_partition_name_length;
}

bool Device::operator==( const Device & device ) const
{
	return this ->get_path() == device .get_path() ;
}
	
bool Device::operator!=( const Device & device ) const 
{
	return ! ( *this == device ) ;
}
	
Device::~Device()
{
}


} //GParted
