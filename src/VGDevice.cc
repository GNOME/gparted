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


#include "Device.h"
#include "VGDevice.h"


namespace GParted
{


VGDevice* VGDevice::clone() const
{
	return new VGDevice(*this);
}


VGDevice* VGDevice::clone_without_partitions() const
{
	VGDevice* new_vg = new VGDevice();
	new_vg->length       = this->length;
	new_vg->heads        = this->heads;
	new_vg->sectors      = this->sectors;
	new_vg->cylinders    = this->cylinders;
	new_vg->cylsize      = this->cylsize;
	new_vg->model        = this->model;
	new_vg->disktype     = this->disktype;
	new_vg->sector_size  = this->sector_size;
	new_vg->max_prims    = this->max_prims;
	new_vg->highest_busy = this->highest_busy;
	new_vg->readonly     = this->readonly;
	new_vg->set_path(this->get_path());
	if (this->partition_naming_supported())
		new_vg->enable_partition_naming(this->get_max_partition_name_length());
	new_vg->vg_name      = this->vg_name;
	new_vg->pe_size      = this->pe_size;
	new_vg->total_pe     = this->total_pe;
	new_vg->exported     = this->exported;
	new_vg->partial      = this->partial;
	return new_vg;
}


}  // namespace GParted
