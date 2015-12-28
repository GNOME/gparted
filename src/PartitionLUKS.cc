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

#include "../include/PartitionLUKS.h"
#include "../include/Utils.h"

namespace GParted
{

PartitionLUKS::PartitionLUKS()
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
//     1111111111111111111111111111111111uuuuuuuuuu <- this->sectors_{used,unused,unallocated}
//            Encrypted file system
//            |<----------------->|
//            111111111111110000000uuuuuu           <- encrypted.sectors_{used,unused,unallocated}
//            ^                         ^
//            |                         `------------- encrypted.sector_end
//            `--------------------------------------- encrypted.sector_start (== size of LUKS header)
//     1111111111111111111110000000uuuuuuuuuuuuuuuu <- Overall usage figures as used in the following
//                                                     usage related methods.
// Legend:
// 1 - used sectors
// 0 - unused sectors
// u - unallocated sectors
//
// Calculations:
//     total_used        = LUKS Header size + Encrypted file system used
//                       = encrypted.sector_start + encrypted.sectors_used
//     total_unallocated = LUKS unallocated + Encrypted file system unallocated
//                       = this->sectors_unallocated + encrypted_unallocated
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
			Sector total_used        = encrypted.sector_start + encrypted.sectors_used;
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
			Sector total_used        = encrypted.sector_start + encrypted.sectors_used;
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

} //GParted
