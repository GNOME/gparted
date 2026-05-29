/* Copyright (C) 2026 Patrick Verner
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


#ifndef GPARTED_VGDEVICE_H
#define GPARTED_VGDEVICE_H


#include "Device.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <vector>


namespace GParted
{


class VGDevice : public Device
{
public:
	VGDevice() = default;
	virtual VGDevice* clone() const;
	virtual VGDevice* clone_without_partitions() const;

	virtual bool is_partition_table_device() const  { return false; }

	Glib::ustring              vg_name;
	Byte_Value                 pe_size      = -1;
	Sector                     total_pe     = -1;
	Sector                     allocated_pe = -1;
	Sector                     free_pe      = -1;
	Glib::ustring              uuid;
	std::vector<Glib::ustring> pv_paths;
	std::vector<Glib::ustring> lv_paths;
	bool                       exported     = false;
	bool                       partial      = false;
};


}  // namespace GParted


#endif /* GPARTED_VGDEVICE_H */
