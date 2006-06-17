/* Copyright (C) 2004 Bart
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
 
#ifndef GPARTED_CORE
#define GPARTED_CORE

#include "../include/FileSystem.h"
#include "../include/Operation.h"

#include <parted/parted.h>
#include <vector>
#include <fstream>

namespace GParted
{

class GParted_Core
{
public:
	GParted_Core() ;
	~GParted_Core() ;
	void find_supported_filesystems() ;
	void set_user_devices( const std::vector<Glib::ustring> & user_devices ) ;
	void get_devices( std::vector<Device> & devices ) ;
	
	bool apply_operation_to_disk( Operation * operation );
	
	bool Set_Disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel ) ;
	
	const std::vector<FS> & get_filesystems() const ;
	const FS & get_fs( GParted::FILESYSTEM filesystem ) const ;
	std::vector<Glib::ustring> get_disklabeltypes() ;
	std::vector<Glib::ustring> get_all_mountpoints() ;
	std::map<Glib::ustring, bool> get_available_flags( const Partition & partition ) ;
	bool toggle_flag( const Partition & partition, const Glib::ustring & flag, bool state ) ;
	
private:
	bool create( const Device & device,
		     Partition & new_partition,
		     std::vector<OperationDetails> & operation_details ) ;
	bool format( const Partition & partition, std::vector<OperationDetails> & operation_details ) ;
	bool Delete( const Partition & partition, std::vector<OperationDetails> & operation_details ) ;
	bool resize_move( const Device & device,
			  const Partition & partition_old,
			  Partition & partition_new,
			  std::vector<OperationDetails> & operation_details ) ;
	bool move_partition( const Partition & partition_old,
			     Partition & partition_new,
			     std::vector<OperationDetails> & operation_details ) ;
	bool resize( const Device & device, 
		     const Partition & partition_old,
		     Partition & partition_new,
		     std::vector<OperationDetails> & operation_detail ) ; 
	bool copy( const Partition & partition_src,
		   Partition & partition_dest,
		   Sector min_size,
		   Sector block_size,
		   std::vector<OperationDetails> & operation_details ) ; 

	GParted::FILESYSTEM get_filesystem() ; 
	bool check_device_path( const Glib::ustring & device_path ) ;
	void set_device_partitions( Device & device ) ;
	void disable_automount( const Device & device ) ;
	void read_mountpoints_from_file( const Glib::ustring & filename,
					 std::map< Glib::ustring, std::vector<Glib::ustring> > & map ) ;
	void init_maps() ;
	void set_mountpoints( std::vector<Partition> & partitions ) ;
	void set_used_sectors( std::vector<Partition> & partitions ) ;
	void insert_unallocated( const Glib::ustring & device_path,
				 std::vector<Partition> & partitions,
				 Sector start,
				 Sector end,
				 bool inside_extended ) ;
	std::vector<Glib::ustring> get_alternate_paths( const Glib::ustring & path ) ;
	void LP_Set_Used_Sectors( Partition & partition );
	void set_flags( Partition & partition ) ;
	bool create_partition( Partition & new_partition,
			       std::vector<OperationDetails> & operation_details,	    
			       Sector min_size = 0 ) ;
	bool create_filesystem( const Partition & partition, std::vector<OperationDetails> & operation_details ) ;
	bool resize_container_partition( const Partition & partition_old,
					 Partition & partition_new,
					 bool fixed_start,
					 std::vector<OperationDetails> & operation_details,
					 Sector min_size = 0 ) ;
	bool resize_normal_using_libparted( const Partition & partition_old,
					    Partition & partition_new,
					    std::vector<OperationDetails> & operation_details ) ;
	bool copy_filesystem( const Partition & partition_src,
			      const Partition & partition_dest,
			      std::vector<OperationDetails> & operation_details,
			      Sector block_size ) ;
	void set_proper_filesystem( const FILESYSTEM & filesystem ) ;
	bool set_partition_type( const Partition & partition,
				 std::vector<OperationDetails> & operation_details ) ;
	bool wait_for_node( const Glib::ustring & node ) ;
	bool erase_filesystem_signatures( const Partition & partition ) ;
		
	bool open_device( const Glib::ustring & device_path ) ;
	bool open_device_and_disk( const Glib::ustring & device_path, bool strict = true ) ;
	void close_disk() ;
	void close_device_and_disk() ;
	bool commit() ;

	static PedExceptionOption ped_exception_handler( PedException * e ) ;

	std::vector<FS> FILESYSTEMS ;
	FileSystem * p_filesystem ;
	std::vector<PedPartitionFlag> flags;
	Glib::ustring temp ;
	Partition partition_temp ;
	FS fs ;
	std::vector<Glib::ustring> device_paths ;
	bool probe_devices ;
	
	std::map< Glib::ustring, std::vector<Glib::ustring> > mount_info ;
	std::map< Glib::ustring, std::vector<Glib::ustring> > fstab_info ;
	std::map< Glib::ustring, Glib::ustring > alternate_paths ;
	std::map< Glib::ustring, Glib::ustring >::iterator iter ;
	std::map< Glib::ustring, std::vector<Glib::ustring> >::iterator iter_mp ;

	//disabling automount stuff
	bool DISABLE_AUTOMOUNT ;
	std::map<Glib::ustring, Glib::ustring> disabled_automount_devices ;

	PedDevice *lp_device ;
	PedDisk *lp_disk ;
	PedPartition *lp_partition ;
};

} //GParted


#endif //GPARTED_CORE
