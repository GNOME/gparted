/* Copyright (C) 2009 Curtis Gedak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* READ THIS!
 * This class was created in an effort to reduce the complexity of the
 * GParted_Core class.
 * This class provides support for Linux software RAID devices (mdadm).
 * Static elements are used in order to reduce the disk accesses required to
 * load the data structures upon each initialization of the class.
 */

#ifndef SWRAID_H_
#define SWRAID_H_

#include "../include/Utils.h"

#include <vector>

namespace GParted
{


class SWRaid
{
public:
	SWRaid() ;
	SWRaid( const bool & do_refresh ) ;
	~SWRaid() ;
	bool is_swraid_supported() ;
	void get_devices( std::vector<Glib::ustring> & swraid_devices ) ;
private:
	void load_swraid_cache() ;
	void set_commands_found() ;
	static bool swraid_cache_initialized ;
	static bool mdadm_found ;
	static std::vector<Glib::ustring> swraid_devices ;
};

}//GParted

#endif /* SWRAID_H_ */
