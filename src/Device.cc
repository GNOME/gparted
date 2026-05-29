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

#include <glibmm/ustring.h>


namespace GParted
{


Device::Device()
{
	Reset() ;	
}


Device* Device::clone() const
{
	return new Device(*this);
}


void Device::Reset()
{
	partitions.clear();
	length                    = 0;
	heads                     = 0;
	sectors                   = 0;
	cylinders                 = 0;
	cylsize                   = 0;
	model                     = "";
	serial_number             = "";
	disktype                  = "";
	sector_size               = 0;
	max_prims                 = 0;
	highest_busy              = 0;
	readonly                  = false;
	path                      = "";
	max_partition_name_length = 0;
}


Device* Device::clone_without_partitions() const
{
	Device* new_device = new Device();
	copy_fields_without_partitions(*new_device);
	return new_device;
}


void Device::copy_fields_without_partitions(Device& dest) const
{
	dest.length                    = this->length;
	dest.heads                     = this->heads;
	dest.sectors                   = this->sectors;
	dest.cylinders                 = this->cylinders;
	dest.cylsize                   = this->cylsize;
	dest.model                     = this->model;
	dest.disktype                  = this->disktype;
	dest.sector_size               = this->sector_size;
	dest.max_prims                 = this->max_prims;
	dest.highest_busy              = this->highest_busy;
	dest.readonly                  = this->readonly;
	dest.path                      = this->path;
	dest.max_partition_name_length = this->max_partition_name_length;
}


void Device::set_path( const Glib::ustring & path )
{
	this->path = path;
}


const Glib::ustring& Device::get_path() const
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
	

}  // namespace GParted
