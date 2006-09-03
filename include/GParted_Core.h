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
	void set_devices( std::vector<Device> & devices ) ;
	
	bool snap_to_cylinder( const Device & device, Partition & partition, Glib::ustring & error ) ;
	bool apply_operation_to_disk( Operation * operation );
	
	bool set_disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel ) ;

	bool toggle_flag( const Partition & partition, const Glib::ustring & flag, bool state ) ;
	
	const std::vector<FS> & get_filesystems() const ;
	const FS & get_fs( GParted::FILESYSTEM filesystem ) const ;
	std::vector<Glib::ustring> get_disklabeltypes() ;
	std::vector<Glib::ustring> get_all_mountpoints() ;
	std::map<Glib::ustring, bool> get_available_flags( const Partition & partition ) ;
	
private:
	//detectionstuff..
	void init_maps() ;
	void read_mountpoints_from_file( const Glib::ustring & filename,
					 std::map< Glib::ustring, std::vector<Glib::ustring> > & map ) ;
	bool check_device_path( const Glib::ustring & device_path ) ;
	std::vector<Glib::ustring> get_alternate_paths( const Glib::ustring & path ) ;
	void set_device_partitions( Device & device ) ;
	GParted::FILESYSTEM get_filesystem() ; 
	void insert_unallocated( const Glib::ustring & device_path,
				 std::vector<Partition> & partitions,
				 Sector start,
				 Sector end,
				 bool inside_extended ) ;
	void set_mountpoints( std::vector<Partition> & partitions ) ;
	void set_used_sectors( std::vector<Partition> & partitions ) ;
	void LP_set_used_sectors( Partition & partition );
	void set_flags( Partition & partition ) ;

	//operationstuff...
	bool create( const Device & device, Partition & new_partition, OperationDetail & operationdetail ) ;
	bool create_partition( Partition & new_partition, OperationDetail & operationdetail, Sector min_size = 0 ) ;
	bool create_filesystem( const Partition & partition, OperationDetail & operationdetail ) ;

	bool format( const Partition & partition, OperationDetail & operationdetail ) ;

	bool Delete( const Partition & partition, OperationDetail & operationdetail ) ;
	
	bool resize_move( const Device & device,
			  const Partition & partition_old,
			  Partition & partition_new,
			  OperationDetail & operationdetail ) ;
	bool move( const Device & device, 
		   const Partition & partition_old,
		   const Partition & partition_new,
		   OperationDetail & operationdetail ) ;
	bool move_filesystem( const Partition & partition_old,
			      const Partition & partition_new,
			      OperationDetail & operationdetail ) ;
	bool resize_move_filesystem_using_libparted( const Partition & partition_old,
				      		     const Partition & partition_new,
					      	     OperationDetail & operationdetail ) ;
	bool resize( const Partition & partition_old,
		     const Partition & partition_new,
		     OperationDetail & operationdetail ) ;
	bool resize_move_partition( const Partition & partition_old,
			       	    const Partition & partition_new,
				    OperationDetail & operationdetail ) ;
	bool resize_filesystem( const Partition & partition_old,
				const Partition & partition_new,
				OperationDetail & operationdetail,
				bool fill_partition = false ) ;
	bool maximize_filesystem( const Partition & partition, OperationDetail & operationdetail ) ;
				
	bool copy( const Partition & partition_src,
		   Partition & partition_dst,
		   Sector min_size,
		   OperationDetail & operationdetail ) ; 
	bool copy_filesystem( const Partition & partition_src,
			      const Partition & partition_dest,
			      OperationDetail & operationdetail ) ;

	bool check_repair_filesystem( const Partition & partition, OperationDetail & operationdetail ) ;

	bool set_partition_type( const Partition & partition, OperationDetail & operationdetail ) ;

	void set_progress_info( Sector total, Sector done, const Glib::Timer & timer, OperationDetail & operationdetail ) ;

	bool copy_blocks( const Glib::ustring & src_device,
			  const Glib::ustring & dst_device,
			  Sector src_start,
			  Sector dst_start,
			  Sector length,
			  Sector blocksize,
			  OperationDetail & operationdetail ) ;

	bool copy_block( PedDevice * lp_device_src,
			 PedDevice * lp_device_dst,
			 Sector offset_src,
			 Sector offset_dst,
			 Sector blocksize,
			 Glib::ustring & error_message ) ; 
	bool calibrate_partition( Partition & partition, OperationDetail & operationdetail ) ;
	bool calculate_exact_geom( const Partition & partition_old,
			           Partition & partition_new,
				   OperationDetail & operationdetail ) ;
	bool set_proper_filesystem( const FILESYSTEM & filesystem ) ;
	bool wait_for_node( const Glib::ustring & node ) ;
	bool erase_filesystem_signatures( const Partition & partition ) ;

	//general..	
	bool open_device( const Glib::ustring & device_path ) ;
	bool open_device_and_disk( const Glib::ustring & device_path, bool strict = true ) ;
	void close_disk() ;
	void close_device_and_disk() ;
	bool commit( const Glib::ustring & node = "" ) ;

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
	
	PedDevice *lp_device ;
	PedDisk *lp_disk ;
	PedPartition *lp_partition ;
};

} //GParted


#endif //GPARTED_CORE
