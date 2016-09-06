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


#ifndef GPARTED_PARTITIONLUKS_H
#define GPARTED_PARTITIONLUKS_H

#include "Partition.h"
#include "Utils.h"

#include <glibmm/ustring.h>

namespace GParted
{

class PartitionLUKS : public Partition
{
public:
	PartitionLUKS();
	virtual ~PartitionLUKS();
	virtual PartitionLUKS * clone() const;

	void set_luks( const Glib::ustring & path,
	               FILESYSTEM fstype,
	               Sector header_size,
	               Sector mapping_size,
	               Byte_Value sector_size,
	               bool busy );

	Partition & get_encrypted()              { return encrypted; };
	const Partition & get_encrypted() const  { return encrypted; };

	virtual bool sector_usage_known() const;
	virtual Sector estimated_min_size() const;
	virtual Sector get_sectors_used() const;
	virtual Sector get_sectors_unused() const;
	virtual Sector get_sectors_unallocated() const;
	virtual Glib::ustring get_filesystem_label() const;
	virtual bool have_messages() const;
	virtual std::vector<Glib::ustring> get_messages() const;
	virtual void clear_messages();

	virtual const Partition & get_filesystem_partition() const;

private:
	Partition encrypted;
	Sector header_size;  // Size of the LUKS header (everything up to the start of the mapping)
};

}//GParted

#endif /* GPARTED_PARTITIONLUKS_H */
