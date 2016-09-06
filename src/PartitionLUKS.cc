/* Copyright (C) 2015 Mike Fleetwood
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

#include "PartitionLUKS.h"
#include "Utils.h"

namespace GParted
{

PartitionLUKS::PartitionLUKS() : header_size( 0 )
{
	// Nothing further to do here as the base class Partition default constructor,
	// which calls Partition::Reset(), is called for this object and also to
	// initialise the encrypted member variable of type Partition.
}

PartitionLUKS::~PartitionLUKS()
{
}

PartitionLUKS * PartitionLUKS::clone() const
{
	// Virtual copy constructor method
	return new PartitionLUKS( *this );
}

// Mostly a convenience method calling Partition::Set() on the encrypted Partition but
// also sets private header_size.  Makes encrypted Partition object look like a whole disk
// device as /dev/mapper/CRYPTNAME contains no partition table and the file system starts
// from sector 0 going to the very end.
void PartitionLUKS::set_luks( const Glib::ustring & path,
                              FILESYSTEM fstype,
                              Sector header_size,
                              Sector mapping_size,
                              Byte_Value sector_size,
                              bool busy )
{
	encrypted.Set( path,
	               path,
	               1,
	               TYPE_PRIMARY,
	               true,
	               fstype,
	               0LL,
	               mapping_size - 1LL,
	               sector_size,
	               false,
	               busy );
	this->header_size = header_size;
}

bool PartitionLUKS::sector_usage_known() const
{
	if ( busy )
		// For an open dm-crypt mapping the usage of both the LUKS and encrypted
		// file system must be known.
		return Partition::sector_usage_known() && encrypted.sector_usage_known();
	return Partition::sector_usage_known();
}

// An encrypted partition is laid out, and usage figures calculated like this:
//
//     Partition
//     |<---------------------------------------->| <- this PartitionLUKS object
//     LUKS   LUKS
//     Header Mapping
//     |<--->||<----------------------->|           <- encrypted Partition object member
//     hhhhhhh                                      <- this->header_size
//     1111111111111111111111111111111111uuuuuuuuuu <- this->sectors_{used,unused,unallocated}
//            Encrypted file system
//            |<----------------->|
//            111111111111110000000uuuuuu           <- encrypted.sectors_{used,unused,unallocated}
//                                                     encrypted.sector_start = 0
//                                                     encrypted.sector_end = (last sector in LUKS mapping)
//
//     1111111111111111111110000000uuuuuuuuuuuuuuuu <- Overall usage figures as used in the following
//                                                     usage related methods.
// Legend:
// 1 - used sectors
// 0 - unused sectors
// u - unallocated sectors
// h - LUKS header sectors
//
// Calculations:
//     total_used        = LUKS Header size + Encrypted file system used
//                       = this->header_size + encrypted.sectors_used
//     total_unallocated = LUKS unallocated + Encrypted file system unallocated
//                       = this->sectors_unallocated + encrypted.sectors_unallocated
//     total_unused      = LUKS unused + Encrypted file system unused
//                       = 0 + encrypted.sectors_unused
//
//     By definition LUKS unused is 0 (See luks::set_used_sectors()).

// Return estimated minimum size to which the partition can be resized.
// See Partition::estimated_min_size() for unallocated threshold reasoning.
Sector PartitionLUKS::estimated_min_size() const
{
	if ( busy )
	{
		// For an open dm-crypt mapping work with above described totals.
		if ( sectors_used >= 0 && encrypted.sectors_used >= 0 )
		{
			Sector total_used        = header_size + encrypted.sectors_used;
			Sector total_unallocated = sectors_unallocated + encrypted.sectors_unallocated;
			return total_used + std::min( total_unallocated, significant_threshold );
		}
		return -1;
	}
	return Partition::estimated_min_size();
}

// Return user displayable used sectors.
// See Partition::get_sectors_used() for unallocated threshold reasoning.
Sector PartitionLUKS::get_sectors_used() const
{
	if ( busy )
	{
		// For an open dm-crypt mapping work with above described totals.
		if ( sectors_used >= 0 && encrypted.sectors_used >= 0 )
		{
			Sector total_used        = header_size + encrypted.sectors_used;
			Sector total_unallocated = sectors_unallocated + encrypted.sectors_unallocated;
			if ( total_unallocated < significant_threshold )
				return total_used + total_unallocated;
			else
				return total_used;
		}
		return -1;
	}
	return Partition::get_sectors_used();
}

// Return user displayable unused sectors.
// See above described totals.
Sector PartitionLUKS::get_sectors_unused() const
{
	if ( busy )
		return encrypted.get_sectors_unused();
	return Partition::get_sectors_unused();
}

// Return user displayable unallocated sectors.
// See Partition::get_sectors_unallocated() for unallocated threshold reasoning.
Sector PartitionLUKS::get_sectors_unallocated() const
{
	if ( busy )
	{
		// For an open dm-crypt mapping work with above described totals.
		Sector total_unallocated = sectors_unallocated + encrypted.sectors_unallocated;
		if ( total_unallocated < significant_threshold )
			return 0;
		else
			return total_unallocated;
	}
	return Partition::get_sectors_unallocated();
}

// Return the label of the encrypted file system within, or "" if no open mapping.
Glib::ustring PartitionLUKS::get_filesystem_label() const
{
	if ( busy )
		return encrypted.get_filesystem_label();
	return "";
}

bool PartitionLUKS::have_messages() const
{
	if ( busy )
		return Partition::have_messages() || encrypted.have_messages();
	return Partition::have_messages();
}

std::vector<Glib::ustring> PartitionLUKS::get_messages() const
{
	if ( busy )
	{
		std::vector<Glib::ustring> msgs  = Partition::get_messages();
		std::vector<Glib::ustring> msgs2 = encrypted.get_messages();
		msgs.insert( msgs.end(), msgs2.begin(), msgs2.end() );
		return msgs;
	}
	return Partition::get_messages();
}

void PartitionLUKS::clear_messages()
{
	Partition::clear_messages();
	encrypted.clear_messages();
}

const Partition & PartitionLUKS::get_filesystem_partition() const
{
	if ( busy )
		return encrypted;
	return *this;
}

} //GParted
