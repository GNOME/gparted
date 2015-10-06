/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Curtis Gedak
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

#ifndef GPARTED_GPARTED_CORE_H
#define GPARTED_GPARTED_CORE_H

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
	static Glib::Thread *mainthread;
	GParted_Core() ;
	~GParted_Core() ;

	static void find_supported_core();
	void find_supported_filesystems() ;
	void set_user_devices( const std::vector<Glib::ustring> & user_devices ) ;
	void set_devices( std::vector<Device> & devices ) ;
	void set_devices_thread( std::vector<Device> * pdevices );
	void guess_partition_table(const Device & device, Glib::ustring &buff);
	
	bool snap_to_cylinder( const Device & device, Partition & partition, Glib::ustring & error ) ;
	bool snap_to_mebibyte( const Device & device, Partition & partition, Glib::ustring & error ) ;
	bool snap_to_alignment( const Device & device, Partition & partition, Glib::ustring & error ) ;
	bool apply_operation_to_disk( Operation * operation );

	bool set_disklabel( const Device & device, const Glib::ustring & disklabel );
	bool new_disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel,
	                    bool recreate_dmraid_devs = true );

	bool toggle_flag( const Partition & partition, const Glib::ustring & flag, bool state ) ;
	
	const std::vector<FS> & get_filesystems() const ;
	const FS & get_fs( GParted::FILESYSTEM filesystem ) const ;
	static std::vector<Glib::ustring> get_disklabeltypes() ;
	static bool is_dev_mounted( const Glib::ustring & path ) ;
	static std::vector<Glib::ustring> get_all_mountpoints() ;
	std::map<Glib::ustring, bool> get_available_flags( const Partition & partition ) ;
	Glib::ustring get_libparted_version() ;
	Glib::ustring get_thread_status_message() ;

	static FileSystem * get_filesystem_object( FILESYSTEM filesystem );
	static bool supported_filesystem( FILESYSTEM fstype );
	static bool filesystem_resize_disallowed( const Partition & partition ) ;
private:
	//detectionstuff..
	static void init_maps() ;
	void set_thread_status_message( Glib::ustring msg ) ;
	static void read_mountpoints_from_file( const Glib::ustring & filename,
	                                        std::map< Glib::ustring, std::vector<Glib::ustring> > & map ) ;
	static void add_node_and_mountpoint( std::map< Glib::ustring, std::vector<Glib::ustring> > & map,
	                                     Glib::ustring & node,
	                                     Glib::ustring & mountpoint ) ;
	static void read_mountpoints_from_file_swaps( const Glib::ustring & filename,
	                                              std::map< Glib::ustring, std::vector<Glib::ustring> > & map ) ;
	static bool have_rootfs_dev( std::map< Glib::ustring, std::vector<Glib::ustring> > & map ) ;
	static void read_mountpoints_from_mount_command( std::map< Glib::ustring, std::vector<Glib::ustring> > & map ) ;
	static Glib::ustring get_partition_path( PedPartition * lp_partition );
	void set_device_serial_number( Device & device );
	void set_device_partitions( Device & device, PedDevice* lp_device, PedDisk* lp_disk ) ;
	void set_device_one_partition( Device & device, PedDevice * lp_device, FILESYSTEM fstype,
	                               std::vector<Glib::ustring> & messages );
	void set_partition_label_and_uuid( Partition & partition );
	static FILESYSTEM detect_filesystem_internal( PedDevice * lp_device, PedPartition * lp_partition );
	static FILESYSTEM detect_filesystem( PedDevice * lp_device, PedPartition * lp_partition,
	                                     std::vector<Glib::ustring> & messages );
	void read_label( Partition & partition ) ;
	void read_uuid( Partition & partition ) ;
	void insert_unallocated( const Glib::ustring & device_path,
				 std::vector<Partition> & partitions,
				 Sector start,
				 Sector end,
				 Byte_Value sector_size,
				 bool inside_extended ) ;
	void set_mountpoints( std::vector<Partition> & partitions ) ;
	bool set_mountpoints_helper( Partition & partitions, const Glib::ustring & path ) ;
	bool is_busy( FILESYSTEM fstype, const Glib::ustring & path ) ;
	void set_used_sectors( std::vector<Partition> & partitions, PedDisk* lp_disk ) ;
	void mounted_set_used_sectors( Partition & partition ) ;
#ifdef HAVE_LIBPARTED_FS_RESIZE
	void LP_set_used_sectors( Partition & partition, PedDisk* lp_disk ) ;
#endif
	void set_flags( Partition & partition, PedPartition* lp_partition ) ;
	
	//operationstuff...
	bool create( Partition & new_partition, OperationDetail & operationdetail );
	bool create_partition( Partition & new_partition, OperationDetail & operationdetail, Sector min_size = 0 ) ;
	bool create_filesystem( const Partition & partition, OperationDetail & operationdetail ) ;

	bool format( const Partition & partition, OperationDetail & operationdetail ) ;

	bool Delete( const Partition & partition, OperationDetail & operationdetail ) ;

	bool remove_filesystem( const Partition & partition, OperationDetail & operationdetail ) ;

	bool label_filesystem( const Partition & partition, OperationDetail & operationdetail );

	bool name_partition( const Partition & partition, OperationDetail & operationdetail );

	bool change_uuid( const Partition & partition, OperationDetail & operation_detail ) ;

	bool resize_move( const Partition & partition_old,
	                  Partition & partition_new,
	                  OperationDetail & operationdetail );
	bool move( const Partition & partition_old,
	           const Partition & partition_new,
	           OperationDetail & operationdetail );
	bool move_filesystem( const Partition & partition_old,
			      const Partition & partition_new,
			      OperationDetail & operationdetail ) ;
#ifdef HAVE_LIBPARTED_FS_RESIZE
	bool resize_move_filesystem_using_libparted( const Partition & partition_old,
				      		     const Partition & partition_new,
					      	     OperationDetail & operationdetail ) ;
	void thread_lp_ped_file_system_resize( PedFileSystem * fs,
	                                       PedGeometry * lp_geom,
	                                       bool * return_value );
#endif
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
		   Byte_Value min_size,
		   OperationDetail & operationdetail ) ; 
	bool copy_filesystem( const Partition & partition_src,
			      const Partition & partition_dst,
			      OperationDetail & operationdetail,
			      bool cancel_safe );
	bool copy_filesystem( const Partition & partition_src,
			      const Partition & partition_dst,
			      OperationDetail & operationdetail,
			      Byte_Value & total_done,
			      bool cancel_safe );
	bool copy_filesystem( const Glib::ustring & src_device,
			      const Glib::ustring & dst_device,
			      Sector src_start,
			      Sector dst_start,
			      Byte_Value src_sector_size,
			      Byte_Value dst_sector_size,
			      Byte_Value src_length,
			      OperationDetail & operationdetail,
			      Byte_Value & total_done,
			      bool cancel_safe ) ;
	void rollback_transaction( const Partition & partition_src,
				   const Partition & partition_dst,
				   OperationDetail & operationdetail,
				   Byte_Value total_done ) ;

	bool check_repair_filesystem( const Partition & partition, OperationDetail & operationdetail ) ;

	bool set_partition_type( const Partition & partition, OperationDetail & operationdetail ) ;

	bool calibrate_partition( Partition & partition, OperationDetail & operationdetail ) ;
	bool calculate_exact_geom( const Partition & partition_old,
			           Partition & partition_new,
				   OperationDetail & operationdetail ) ;
	bool erase_filesystem_signatures( const Partition & partition, OperationDetail & operationdetail ) ;
	bool update_bootsector( const Partition & partition, OperationDetail & operationdetail ) ;

	//general..	
	static void init_filesystems();
	static void fini_filesystems();

	static bool flush_device( PedDevice * lp_device );
	static bool get_device( const Glib::ustring & device_path, PedDevice *& lp_device, bool flush = false );
	static bool get_disk( PedDevice *& lp_device, PedDisk *& lp_disk, bool strict = true );
	static bool get_device_and_disk( const Glib::ustring & device_path,
	                                 PedDevice*& lp_device, PedDisk*& lp_disk, bool strict = true, bool flush = false );
	static void destroy_device_and_disk( PedDevice*& lp_device, PedDisk*& lp_disk );
	static bool commit( PedDisk* lp_disk );
	static bool commit_to_os( PedDisk* lp_disk, std::time_t timeout );
	static void settle_device( std::time_t timeout );
	static bool useable_device( PedDevice * lp_device );

	static PedExceptionOption ped_exception_handler( PedException * e ) ;

	std::vector<FS> FILESYSTEMS ;
	static std::map< FILESYSTEM, FileSystem * > FILESYSTEM_MAP ;
	std::vector<PedPartitionFlag> flags;
	std::vector<Glib::ustring> device_paths ;
	bool probe_devices ;
	Glib::ustring thread_status_message;  //Used to pass data to show_pulsebar method
	Glib::RefPtr<Glib::IOChannel> iocInput, iocOutput; // Used to send data to gpart command
};

} //GParted

#endif /* GPARTED_GPARTED_CORE_H */
