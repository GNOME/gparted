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

// Return the label of the encrypted file system within, or "" if no open mapping.
Glib::ustring PartitionLUKS::get_filesystem_label() const
{
	if ( busy )
		return encrypted.get_filesystem_label();
	return "";
}

} //GParted
