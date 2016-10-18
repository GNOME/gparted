/* Copyright (C) 2009, 2010, 2011 Curtis Gedak
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

/* READ THIS!
 * This class was created in an effort to reduce the complexity of the
 * GParted_Core class.
 * This class provides support for motherboard based RAID devices, also know
 * as Fake RAID.
 * Static elements are used in order to reduce the disk accesses required to
 * load the data structures upon each initialization of the class.
 */

#ifndef GPARTED_DMRAID_H
#define GPARTED_DMRAID_H

#include "Utils.h"
#include "Partition.h"
#include "OperationDetail.h"

#include <vector>

namespace GParted
{


class DMRaid
{
public:
	DMRaid() ;
	DMRaid( const bool & do_refresh ) ;
	~DMRaid() ;
	bool is_dmraid_supported() ;
	bool is_dmraid_device( const Glib::ustring & dev_path ) ;
	int execute_command( const Glib::ustring & command, OperationDetail & operationdetail ) ;
	void get_devices( std::vector<Glib::ustring> & dmraid_devices ) ;
	Glib::ustring get_dmraid_name( const Glib::ustring & dev_path ) ;
	int get_partition_number( const Glib::ustring & partition_name ) ;
	Glib::ustring get_udev_dm_name( const Glib::ustring & dev_path ) ;
	Glib::ustring make_path_dmraid_compatible( Glib::ustring partition_path ) ;
	bool create_dev_map_entries( const Partition & partition, OperationDetail & operationdetail ) ;
	bool create_dev_map_entries( const Glib::ustring & dev_path ) ;
	bool delete_affected_dev_map_entries( const Partition & partition, OperationDetail & operationdetail ) ;
	bool delete_dev_map_entry( const Partition & partition, OperationDetail & operationdetail ) ;
	bool purge_dev_map_entries( const Glib::ustring & dev_path ) ;
	bool update_dev_map_entry( const Partition & partition, OperationDetail & operationdetail ) ;
private:
	void load_dmraid_cache() ;
	void set_commands_found() ;
	void get_dmraid_dir_entries( const Glib::ustring & dev_path, std::vector<Glib::ustring> & dir_list ) ;
	void get_affected_dev_map_entries( const Partition & partition, std::vector<Glib::ustring> & affected_entries ) ;
	void get_partition_dev_map_entries( const Partition & partition, std::vector<Glib::ustring> & partition_entries ) ;
	static bool dmraid_cache_initialized ;
	static bool dmraid_found ;
	static bool dmsetup_found ;
	static bool udevinfo_found ;
	static bool udevadm_found ;
	static std::vector<Glib::ustring> dmraid_devices ;
};

}//GParted

#endif /* GPARTED_DMRAID_H */
