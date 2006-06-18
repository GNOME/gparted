/* Copyright (C) 2004 Bart 'plors' Hakvoort
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
 
#include "../include/Win_GParted.h"
#include "../include/GParted_Core.h"
#include "../include/OperationCopy.h"
#include "../include/OperationCreate.h"
#include "../include/OperationDelete.h"
#include "../include/OperationFormat.h"
#include "../include/OperationResizeMove.h"

#include "../include/ext2.h"
#include "../include/ext3.h"
#include "../include/fat16.h"
#include "../include/fat32.h"
#include "../include/linux_swap.h"
#include "../include/reiserfs.h"
#include "../include/ntfs.h"
#include "../include/xfs.h"
#include "../include/jfs.h"
#include "../include/hfs.h"
#include "../include/hfsplus.h"
#include "../include/reiser4.h"
#include "../include/ufs.h"

#include <cerrno>
#include <sys/statvfs.h>	

Glib::ustring ped_error ; //see e.g. ped_exception_handler()

namespace GParted
{

GParted_Core::GParted_Core() 
{
	lp_device = NULL ;
	lp_disk = NULL ;
	lp_partition = NULL ;
	p_filesystem = NULL ;
	DISABLE_AUTOMOUNT = false ;

	ped_exception_set_handler( ped_exception_handler ) ; 

	//get valid flags ...
	for ( PedPartitionFlag flag = ped_partition_flag_next( static_cast<PedPartitionFlag>( NULL ) ) ;
	      flag ;
	      flag = ped_partition_flag_next( flag ) )
		flags .push_back( flag ) ;
	
	//throw libpartedversion to the stdout to see which version is actually used.
	std::cout << "======================" << std::endl ;
	std::cout << "libparted : " << ped_get_version() << std::endl ;

	//see if we can disable automounting
	if ( ! Glib::find_program_in_path( "hal-find-by-property" ) .empty() &&
	     ! Glib::find_program_in_path( "hal-set-property" ) .empty() )
	{
		DISABLE_AUTOMOUNT = true ;
		std::cout << "automounting disabled" << std::endl ;
	}

	std::cout << "======================" << std::endl ;
	
	//initialize filesystemlist
	find_supported_filesystems() ;
}

void GParted_Core::find_supported_filesystems()
{
	FILESYSTEMS .clear() ;
	
	ext2 fs_ext2;
	FILESYSTEMS .push_back( fs_ext2 .get_filesystem_support() ) ;
	
	ext3 fs_ext3;
	FILESYSTEMS .push_back( fs_ext3 .get_filesystem_support() ) ;
	
	fat16 fs_fat16;
	FILESYSTEMS .push_back( fs_fat16 .get_filesystem_support() ) ;
	
	fat32 fs_fat32;
	FILESYSTEMS .push_back( fs_fat32 .get_filesystem_support() ) ;
	
	hfs fs_hfs;
	FILESYSTEMS .push_back( fs_hfs .get_filesystem_support() ) ;
	
	hfsplus fs_hfsplus;
	FILESYSTEMS .push_back( fs_hfsplus .get_filesystem_support() ) ;
	
	jfs fs_jfs;
	FILESYSTEMS .push_back( fs_jfs .get_filesystem_support() ) ;
	
	linux_swap fs_linux_swap;
	FILESYSTEMS .push_back( fs_linux_swap .get_filesystem_support() ) ;
	
	ntfs fs_ntfs;
	FILESYSTEMS .push_back( fs_ntfs .get_filesystem_support() ) ;
	
	reiser4 fs_reiser4;
	FILESYSTEMS .push_back( fs_reiser4 .get_filesystem_support() ) ;
	
	reiserfs fs_reiserfs;
	FILESYSTEMS .push_back( fs_reiserfs .get_filesystem_support() ) ;
	
	ufs fs_ufs;
	FILESYSTEMS .push_back( fs_ufs .get_filesystem_support() ) ;
	
	xfs fs_xfs;
	FILESYSTEMS .push_back( fs_xfs .get_filesystem_support() ) ;
	
	//unknown filesystem (default when no match is found)
	FS fs ; fs .filesystem = GParted::FS_UNKNOWN ;
	FILESYSTEMS .push_back( fs ) ;
}
	
void GParted_Core::set_user_devices( const std::vector<Glib::ustring> & user_devices ) 
{
	this ->device_paths = user_devices ;
	this ->probe_devices = ! user_devices .size() ;
}
	
void GParted_Core::set_devices( std::vector<Device> & devices )
{
	devices .clear() ;
	Device temp_device ;
	
	init_maps() ;
	
	//only probe if no devices were specified as arguments..
	if ( probe_devices )
	{
		device_paths .clear() ;
		
		//try to find all available devices
		ped_device_probe_all();
		
		lp_device = ped_device_get_next( NULL );
		while ( lp_device ) 
		{
			device_paths .push_back( lp_device ->path ) ;
						
			lp_device = ped_device_get_next( lp_device ) ;
		}
		close_device_and_disk() ;

		std::sort( device_paths .begin(), device_paths .end() ) ;
	}
	
	//remove non-existing devices from disabled_automount_devices..
	for ( iter = disabled_automount_devices .begin() ; iter != disabled_automount_devices .end() ; ++iter )
		if ( std::find( device_paths .begin(), device_paths .end(), iter ->first ) == device_paths .end() )
			disabled_automount_devices .erase( iter ) ;

	for ( unsigned int t = 0 ; t < device_paths .size() ; t++ ) 
	{ 
		if ( check_device_path( device_paths[ t ] ) &&
		     open_device_and_disk( device_paths[ t ], false ) &&
		     lp_device )
		{
			temp_device .Reset() ;
			
			//device info..
			temp_device .add_path( device_paths[ t ] ) ;
			temp_device .add_paths( get_alternate_paths( temp_device .get_path() ) ) ;

			temp_device .model 	=	lp_device ->model ;
			temp_device .heads 	=	lp_device ->bios_geom .heads ;
			temp_device .sectors 	=	lp_device ->bios_geom .sectors ;
			temp_device .cylinders	=	lp_device ->bios_geom .cylinders ;
			temp_device .length 	=	temp_device .heads * temp_device .sectors * temp_device .cylinders ;
			temp_device .cylsize 	=	temp_device .heads * temp_device .sectors ; 
			
			//make sure cylsize is at least 1 MiB
			if ( temp_device .cylsize < MEBIBYTE )
				temp_device .cylsize = MEBIBYTE ;
				
			//normal harddisk
			if ( lp_disk )
			{
				temp_device .disktype =	lp_disk ->type ->name ;
				temp_device .max_prims = ped_disk_get_max_primary_partition_count( lp_disk ) ;
				
				disable_automount( temp_device ) ;
				set_device_partitions( temp_device ) ;
				set_mountpoints( temp_device .partitions ) ;
				set_used_sectors( temp_device .partitions ) ;
			
				if ( temp_device .highest_busy )
					temp_device .readonly = ! ped_disk_commit_to_os( lp_disk ) ;
			}
			//harddisk without disklabel
			else
			{
				temp_device .disktype = _("unrecognized") ;
				temp_device .max_prims = -1 ;
				
				Partition partition_temp ;
				partition_temp .Set_Unallocated( temp_device .get_path(),
								 0,
								 temp_device .length,
								 false );
				temp_device .partitions .push_back( partition_temp );
			}
					
			devices .push_back( temp_device ) ;
			
			close_device_and_disk() ;
		}
	}

	//clear leftover information...	
	//NOTE that we cannot clear mountinfo since it might be needed in get_all_mountpoints()
	alternate_paths .clear() ;
	fstab_info .clear() ;
}

bool GParted_Core::apply_operation_to_disk( Operation * operation )
{
	switch ( operation ->type )
	{
		case DELETE:
			return Delete( operation ->partition_original, operation ->operation_details .sub_details ) ;
		case CREATE:
			return create( operation ->device, 
				       operation ->partition_new,
				       operation ->operation_details .sub_details ) ;
		case RESIZE_MOVE:
			return resize_move( operation ->device,
				       	    operation ->partition_original,
					    operation ->partition_new,
					    operation ->operation_details .sub_details ) ;
		case FORMAT:
			return format( operation ->partition_new, operation ->operation_details .sub_details ) ;
		case COPY: 
			operation ->partition_new .add_path( operation ->partition_original .get_path(), true ) ;
			return copy( static_cast<OperationCopy*>( operation ) ->partition_copied,
				     operation ->partition_new,
				     static_cast<OperationCopy*>( operation ) ->partition_copied .get_length(),
				     static_cast<OperationCopy*>( operation ) ->block_size,
				     operation ->operation_details .sub_details ) ;
	}

	return false ;
}

bool GParted_Core::set_disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( device_path, false ) )
	{
		PedDiskType *type = NULL ;
		type = ped_disk_type_get( disklabel .c_str() ) ;
		
		if ( type )
		{
			lp_disk = ped_disk_new_fresh( lp_device, type );
		
			return_value = commit() ;
		}
		
		close_device_and_disk() ;
	}
	
	return return_value ;	
}
	
bool GParted_Core::toggle_flag( const Partition & partition, const Glib::ustring & flag, bool state ) 
{
	bool succes = false ;

	if ( open_device_and_disk( partition .device_path ) )
	{
		lp_partition = NULL ;
		if ( partition .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else
			lp_partition = ped_disk_get_partition_by_sector( 
						lp_disk,
						(partition .sector_end + partition .sector_start) / 2 ) ;
	
		if ( lp_partition )
		{
			PedPartitionFlag lp_flag = ped_partition_flag_get_by_name( flag .c_str() ) ;

			if ( lp_flag > 0 && ped_partition_set_flag( lp_partition, lp_flag, state ) )
				succes = commit() ;
		}
	
		close_device_and_disk() ;
	}

	return succes ;
}

const std::vector<FS> & GParted_Core::get_filesystems() const 
{
	return FILESYSTEMS ;
}

const FS & GParted_Core::get_fs( GParted::FILESYSTEM filesystem ) const 
{
	for ( unsigned int t = 0 ; t < FILESYSTEMS .size() ; t++ )
		if ( FILESYSTEMS[ t ] .filesystem == filesystem )
			return FILESYSTEMS[ t ] ;
	
	return FILESYSTEMS .back() ;
}

std::vector<Glib::ustring> GParted_Core::get_disklabeltypes() 
{
	std::vector<Glib::ustring> disklabeltypes ;
	
	//msdos should be first in the list
	disklabeltypes .push_back( "msdos" ) ;
	
	 PedDiskType *disk_type ;
	 for ( disk_type = ped_disk_type_get_next( NULL ) ; disk_type ; disk_type = ped_disk_type_get_next( disk_type ) ) 
		 if ( static_cast<Glib::ustring>( disk_type->name ) != "msdos" )
			disklabeltypes .push_back( disk_type->name ) ;

	 return disklabeltypes ;
}

std::vector<Glib::ustring> GParted_Core::get_all_mountpoints() 
{
	std::vector<Glib::ustring> mountpoints ;

	for ( iter_mp = mount_info .begin() ; iter_mp != mount_info .end() ; ++iter_mp )
		mountpoints .insert( mountpoints .end(), iter_mp ->second .begin(), iter_mp ->second .end() ) ;

	return mountpoints ;
}
	
std::map<Glib::ustring, bool> GParted_Core::get_available_flags( const Partition & partition ) 
{
	std::map<Glib::ustring, bool> flag_info ;

	if ( open_device_and_disk( partition .device_path ) )
	{
		lp_partition = NULL ;
		if ( partition .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else
			lp_partition = ped_disk_get_partition_by_sector( 
						lp_disk,
						(partition .sector_end + partition .sector_start) / 2 ) ;
	
		if ( lp_partition )
		{
			for ( unsigned int t = 0 ; t < flags .size() ; t++ )
				if ( ped_partition_is_flag_available( lp_partition, flags[ t ] ) )
					flag_info[ ped_partition_flag_get_name( flags[ t ] ) ] =
						ped_partition_get_flag( lp_partition, flags[ t ] ) ;
		}
	
		close_device_and_disk() ;
	}

	return flag_info ;
}

//private functions...

void GParted_Core::init_maps() 
{
	alternate_paths .clear() ;
	mount_info .clear() ;
	fstab_info .clear() ;

	read_mountpoints_from_file( "/proc/mounts", mount_info ) ;
	read_mountpoints_from_file( "/etc/mtab", mount_info ) ;
	read_mountpoints_from_file( "/etc/fstab", fstab_info ) ;
	
	//sort the mountpoints and remove duplicates.. (no need to do this for fstab_info)
	for ( iter_mp = mount_info .begin() ; iter_mp != mount_info .end() ; ++iter_mp )
	{
		std::sort( iter_mp ->second .begin(), iter_mp ->second .end() ) ;
		
		iter_mp ->second .erase( 
				std::unique( iter_mp ->second .begin(), iter_mp ->second .end() ),
				iter_mp ->second .end() ) ;
	}

	//initialize alternate_paths...
	std::string line ;
	std::ifstream proc_partitions( "/proc/partitions" ) ;
	if ( proc_partitions )
	{
		char c_str[255] ;
		
		while ( getline( proc_partitions, line ) )
			if ( sscanf( line .c_str(), "%*d %*d %*d %255s", c_str ) == 1 )
			{
				line = "/dev/" ; 
				line += c_str ;
				
				if ( realpath( line .c_str(), c_str ) && line != c_str )
				{
					//because we can make no assumption about which path libparted will detect
					//we add all combinations.
					alternate_paths[ c_str ] = line ;
					alternate_paths[ line ] = c_str ;
				}
			}

		proc_partitions .close() ;
	}
}

void GParted_Core::read_mountpoints_from_file( const Glib::ustring & filename,
					       std::map< Glib::ustring, std::vector<Glib::ustring> > & map ) 
{
	std::string line ;
	char node[255], mountpoint[255] ;
	unsigned int index ;
	
	std::ifstream file( filename .c_str() ) ;
	if ( file )
	{
		while ( getline( file, line ) )
			if ( Glib::str_has_prefix( line, "/" ) &&
			     sscanf( line .c_str(), "%255s %255s", node, mountpoint ) == 2 &&
			     static_cast<Glib::ustring>( node ) != "/dev/root" )
			{
				line = mountpoint ;
				
				//see if mountpoint contains spaces and deal with it
				index = line .find( "\\040" ) ;
				if ( index < line .length() )
					line .replace( index, 4, " " ) ;
				
				map[ node ] .push_back( line ) ;
			}
			
		file .close() ;
	}
}

bool GParted_Core::check_device_path( const Glib::ustring & device_path ) 
{
	return ( device_path .length() > 6 && device_path .is_ascii() ) ;
}

std::vector<Glib::ustring> GParted_Core::get_alternate_paths( const Glib::ustring & path ) 
{
	std::vector<Glib::ustring> paths ;
	
	iter = alternate_paths .find( path ) ;
	if ( iter != alternate_paths .end() )
		paths .push_back( iter ->second ) ;

	return paths ;
}

void GParted_Core::disable_automount( const Device & device ) 
{
	if ( DISABLE_AUTOMOUNT )
	{
		if ( disabled_automount_devices .find( device .get_path() ) == disabled_automount_devices .end() )
		{
			//get HAL device-id
			Glib::ustring hal_id, error ;
			bool found = false ;

			for ( unsigned int t = 0 ; t < device .get_paths() .size() && ! found ; t++ )
				found = ! Utils::execute_command(
						"hal-find-by-property --key 'block.device' --string '" +
						device .get_paths()[ t ] + "'",
						hal_id,
						error ) ;

			if ( found )
			{
				if ( ! hal_id .empty() && hal_id[ hal_id .length() -1 ] == '\n' )
					hal_id .erase( hal_id .length() -1, 1 ) ;
					
				Utils::execute_command( "hal-set-property --udi '" + hal_id + 
							"' --key 'storage.automount_enabled_hint' --bool false" ) ;
			}
			
			//found or not found.. we're done with this device..
			disabled_automount_devices[ device. get_path() ] = hal_id ;
		}
	}
}

void GParted_Core::set_device_partitions( Device & device ) 
{
	int EXT_INDEX = -1 ;
	char * lp_path ;//we have to free the result of ped_partition_get_path()..
	
	//clear partitions
	device .partitions .clear() ;
	
	lp_partition = ped_disk_next_partition( lp_disk, NULL ) ;
	while ( lp_partition )
	{
		partition_temp .Reset() ;
	
		switch ( lp_partition ->type )
		{
			case PED_PARTITION_NORMAL:
			case PED_PARTITION_LOGICAL:  
				lp_path = ped_partition_get_path( lp_partition ) ;
				partition_temp .Set( device .get_path(),
						     lp_path,
						     lp_partition ->num,
						     lp_partition ->type == 0 ?
						     		GParted::TYPE_PRIMARY : GParted::TYPE_LOGICAL,
						     get_filesystem(),
						     lp_partition ->geom .start,
						     lp_partition ->geom .end,
						     lp_partition ->type,
						     ped_partition_is_busy( lp_partition ) ) ;
				free( lp_path ) ;
						
				partition_temp .add_paths( get_alternate_paths( partition_temp .get_path() ) ) ;
				set_flags( partition_temp ) ;
				
				if ( partition_temp .busy && partition_temp .partition_number > device .highest_busy )
					device .highest_busy = partition_temp .partition_number ;
									
				break ;
			
			case PED_PARTITION_EXTENDED:
				lp_path = ped_partition_get_path( lp_partition ) ;
				partition_temp .Set( device .get_path(),
					 	     lp_path, 
						     lp_partition ->num,
						     GParted::TYPE_EXTENDED,
						     GParted::FS_EXTENDED,
						     lp_partition ->geom .start,
						     lp_partition ->geom .end,
						     false,
						     ped_partition_is_busy( lp_partition ) ) ;
				free( lp_path ) ;
				
				partition_temp .add_paths( get_alternate_paths( partition_temp .get_path() ) ) ;
				set_flags( partition_temp ) ;
				
				EXT_INDEX = device .partitions .size() ;
				break ;
		
			default:
				break;
		}
		
		//if there's an end, there's a partition ;)
		if ( partition_temp .sector_end > -1 )
		{
			if ( ! partition_temp .inside_extended )
				device .partitions .push_back( partition_temp );
			else
				device .partitions[ EXT_INDEX ] .logicals .push_back( partition_temp ) ;
		}
		
		//next partition (if any)
		lp_partition = ped_disk_next_partition( lp_disk, lp_partition ) ;
	}
	
	if ( EXT_INDEX > -1 )
		insert_unallocated( device .get_path(),
				    device .partitions[ EXT_INDEX ] .logicals,
				    device .partitions[ EXT_INDEX ] .sector_start,
				    device .partitions[ EXT_INDEX ] .sector_end,
				    true ) ;
	
	insert_unallocated( device .get_path(), device .partitions, 0, device .length -1, false ) ; 
}

GParted::FILESYSTEM GParted_Core::get_filesystem() 
{
	//standard libparted filesystems..
	if ( lp_partition && lp_partition ->fs_type )
	{
		if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "extended" )
			return GParted::FS_EXTENDED ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "ext2" )
			return GParted::FS_EXT2 ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "ext3" )
			return GParted::FS_EXT3 ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "linux-swap" )
			return GParted::FS_LINUX_SWAP ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "fat16" )
			return GParted::FS_FAT16 ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "fat32" )
			return GParted::FS_FAT32 ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "ntfs" )
			return GParted::FS_NTFS ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "reiserfs" )
			return GParted::FS_REISERFS ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "xfs" )
			return GParted::FS_XFS ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "jfs" )
			return GParted::FS_JFS ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "hfs" )
			return GParted::FS_HFS ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "hfs+" )
			return GParted::FS_HFSPLUS ;
		else if ( static_cast<Glib::ustring>( lp_partition ->fs_type ->name ) == "ufs" )
			return GParted::FS_UFS ;
	}
	
	
	//other filesystems libparted couldn't detect (i've send patches for these filesystems to the parted guys)
	char buf[512] ;

	ped_device_open( lp_device );

	//reiser4
	ped_geometry_read( & lp_partition ->geom, buf, 128, 1 ) ;
	ped_device_close( lp_device );
	
	if ( static_cast<Glib::ustring>( buf ) == "ReIsEr4" )
		return GParted::FS_REISER4 ;		
		
	//no filesystem found....
	partition_temp .error = _( "Unable to detect filesystem! Possible reasons are:" ) ;
	partition_temp .error += "\n-"; 
	partition_temp .error += _( "The filesystem is damaged" ) ;
	partition_temp .error += "\n-" ; 
	partition_temp .error += _( "The filesystem is unknown to GParted" ) ;
	partition_temp .error += "\n-"; 
	partition_temp .error += _( "There is no filesystem available (unformatted)" ) ; 

	return GParted::FS_UNKNOWN ;
}

void GParted_Core::insert_unallocated( const Glib::ustring & device_path,
				       std::vector<Partition> & partitions,
				       Sector start,
				       Sector end,
				       bool inside_extended )
{
	partition_temp .Reset() ;
	partition_temp .Set_Unallocated( device_path, 0, 0, inside_extended ) ;
	
	//if there are no partitions at all..
	if ( partitions .empty() )
	{
		partition_temp .sector_start = start ;
		partition_temp .sector_end = end ;
		
		partitions .push_back( partition_temp );
		
		return ;
	}
		
	//start <---> first partition start
	if ( (partitions .front() .sector_start - start) >= MEBIBYTE )
	{
		partition_temp .sector_start = start ;
		partition_temp .sector_end = partitions .front() .sector_start -1 ;
		
		partitions .insert( partitions .begin(), partition_temp );
	}
	
	//look for gaps in between
	for ( unsigned int t =0 ; t < partitions .size() -1 ; t++ )
		if ( ( partitions[ t +1 ] .sector_start - partitions[ t ] .sector_end ) >= MEBIBYTE )
		{
			partition_temp .sector_start = partitions[ t ] .sector_end +1 ;
			partition_temp .sector_end = partitions[ t +1 ] .sector_start -1 ;
		
			partitions .insert( partitions .begin() + ++t, partition_temp );
		}
		
	//last partition end <---> end
	if ( (end - partitions .back() .sector_end ) >= MEBIBYTE )
	{
		partition_temp .sector_start = partitions .back() .sector_end +1 ;
		partition_temp .sector_end = end ;
		
		partitions .push_back( partition_temp );
	}
}
	
void GParted_Core::set_mountpoints( std::vector<Partition> & partitions ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		if ( ( partitions[ t ] .type == GParted::TYPE_PRIMARY ||
		       partitions[ t ] .type == GParted::TYPE_LOGICAL ) &&
		     partitions[ t ] .filesystem != GParted::FS_LINUX_SWAP )
		{
			if ( partitions[ t ] .busy )
			{
				for ( unsigned int i = 0 ; i < partitions[ t ] .get_paths() .size() ; i++ )
				{
					iter_mp = mount_info .find( partitions[ t ] .get_paths()[ i ] ) ;
					if ( iter_mp != mount_info .end() )
					{
						partitions[ t ] .add_mountpoints( iter_mp ->second ) ;
						break ;
					}
				}

				if ( partitions[ t ] .get_mountpoints() .empty() )
					partitions[ t ] .error = _("Unable to find mountpoint") ;
			}
			else
			{
				iter_mp = fstab_info .find( partitions[ t ] .get_path() );
				if ( iter_mp != fstab_info .end() )
					partitions[ t ] .add_mountpoints( iter_mp ->second ) ;
			}
		}
		else if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			set_mountpoints( partitions[ t ] .logicals ) ;
	}
}
	
void GParted_Core::set_used_sectors( std::vector<Partition> & partitions ) 
{
	struct statvfs sfs ; 
	
	temp = _("Unable to read the contents of this filesystem!") ;
	temp += "\n" ;
	temp += _("Because of this some operations may be unavailable.") ;
	temp += "\n\n" ;
	temp += _("Did you install the correct plugin for this filesystem?") ;
	
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		if ( partitions[ t ] .filesystem != GParted::FS_LINUX_SWAP &&
		     partitions[ t ] .filesystem != GParted::FS_UNKNOWN )
		{
			if ( partitions[ t ] .type == GParted::TYPE_PRIMARY ||
			     partitions[ t ] .type == GParted::TYPE_LOGICAL ) 
			{
				if ( partitions[ t ] .busy )
				{
					if ( partitions[ t ] .get_mountpoints() .size() > 0  )
					{ 
						if ( statvfs( partitions[ t ] .get_mountpoint() .c_str(), &sfs ) == 0 )
							partitions[ t ] .Set_Unused( sfs .f_bfree * (sfs .f_bsize / 512) ) ;
						else
							partitions[ t ] .error = 
								"statvfs (" + 
								partitions[ t ] .get_mountpoint() + 
								"): " + 
								Glib::strerror( errno ) ;
					}
				}
				else
				{
					switch( get_fs( partitions[ t ] .filesystem ) .read )
					{
						case GParted::FS::EXTERNAL	:
							set_proper_filesystem( partitions[ t ] .filesystem ) ;
							p_filesystem ->Set_Used_Sectors( partitions[ t ] ) ;
							break ;
						case GParted::FS::LIBPARTED	:
							LP_set_used_sectors( partitions[ t ] ) ;
							break ;

						default:
							break ;
					}
				}

				if ( partitions[ t ] .sectors_used == -1 && partitions[ t ] .error .empty() )
					partitions[ t ] .error = temp ;
				
			}
			else if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
				set_used_sectors( partitions[ t ] .logicals ) ;
		}
	}
}

void GParted_Core::LP_set_used_sectors( Partition & partition )
{
	PedFileSystem *fs = NULL;
	PedConstraint *constraint = NULL;

	if ( lp_disk )
	{
		lp_partition = ped_disk_get_partition_by_sector( 
					lp_disk,
					(partition .sector_end + partition .sector_start) / 2 ) ;
		
		if ( lp_partition )
		{
			fs = ped_file_system_open( & lp_partition ->geom ); 	
					
			if ( fs )
			{
				constraint = ped_file_system_get_resize_constraint( fs ) ;
				if ( constraint )
				{
					partition .Set_Unused( partition .get_length() - constraint ->min_size ) ;
					
					ped_constraint_destroy( constraint );
				}
												
				ped_file_system_close( fs ) ;
			}
		}
	}
				
	if ( partition .sectors_used == -1 )
		partition .error = ped_error ;
}

void GParted_Core::set_flags( Partition & partition )
{
	for ( unsigned int t = 0 ; t < flags .size() ; t++ )
		if ( ped_partition_is_flag_available( lp_partition, flags[ t ] ) &&
		     ped_partition_get_flag( lp_partition, flags[ t ] ) )
			partition .flags .push_back( ped_partition_flag_get_name( flags[ t ] ) ) ;
}

bool GParted_Core::create( const Device & device,
			   Partition & new_partition,
			   std::vector<OperationDetails> & operation_details ) 
{
	
	if ( new_partition .type == GParted::TYPE_EXTENDED )   
	{
		return create_partition( new_partition, operation_details ) ;
	}
	else if ( create_partition( new_partition, operation_details, get_fs( new_partition .filesystem ) .MIN ) )
	{
		if ( new_partition .filesystem == GParted::FS_UNFORMATTED )
			return true ;
		else
			return set_partition_type( new_partition, operation_details ) &&
		       	       create_filesystem( new_partition, operation_details ) ;
	}
	
	return false ;
}

bool GParted_Core::create_partition( Partition & new_partition,
				     std::vector<OperationDetails> & operation_details,
				     Sector min_size )
{
	operation_details .push_back( OperationDetails( _("create empty partition") ) ) ;
	
	new_partition .partition_number = 0 ;
	ped_error .clear() ;
		
	if ( open_device_and_disk( new_partition .device_path ) )
	{
		PedPartitionType type;
		lp_partition = NULL ;
		PedConstraint *constraint = NULL ;
		PedFileSystemType* fs_type = NULL ;
		
		//create new partition
		switch ( new_partition .type )
		{
			case GParted::TYPE_PRIMARY:
				type = PED_PARTITION_NORMAL ;
				break ;
			case GParted::TYPE_LOGICAL:
				type = PED_PARTITION_LOGICAL ;
				break ;
			case GParted::TYPE_EXTENDED:
				type = PED_PARTITION_EXTENDED ;
				break ;
				
			default	:
				type = PED_PARTITION_FREESPACE;
		}

		if ( new_partition .type != GParted::TYPE_EXTENDED )
			fs_type = ped_file_system_type_get( "ext2" ) ;

		lp_partition = ped_partition_new( lp_disk,
						  type,
						  fs_type,
						  new_partition .sector_start,
						  new_partition .sector_end ) ;
	
		if ( lp_partition )
		{
			if ( new_partition .strict )
			{
				PedGeometry *geom = ped_geometry_new( lp_device,
								      new_partition .sector_start,
								      new_partition .get_length() ) ;

				if ( geom )
					constraint = ped_constraint_exact( geom ) ;
			}
			else
				constraint = ped_constraint_any( lp_device );
			
			if ( constraint )
			{
				if ( min_size > 0 )
					constraint ->min_size = min_size ;
			
				if ( ped_disk_add_partition( lp_disk, lp_partition, constraint ) && commit() )
				{
					//we have to free the result of ped_partition_get_path()..
					char * lp_path = ped_partition_get_path( lp_partition ) ;
					new_partition .add_path( lp_path, true ) ;
					free( lp_path ) ;

					new_partition .partition_number = lp_partition ->num ;
					new_partition .sector_start = lp_partition ->geom .start ;
					new_partition .sector_end = lp_partition ->geom .end ;
					
					operation_details .back() .sub_details .push_back( OperationDetails( 
						"<i>" +
						String::ucompose( _("path: %1"), new_partition .get_path() ) + "\n" +
						String::ucompose( _("start: %1"), new_partition .sector_start ) + "\n" +
						String::ucompose( _("end: %1"), new_partition .sector_end ) + "\n" +
						String::ucompose( _("size: %1"), Utils::format_size( new_partition .get_length() ) ) + 
						"</i>",
						OperationDetails::NONE ) ) ;
				}
			
				ped_constraint_destroy( constraint );
			}
		}
				
		close_device_and_disk() ;
	}


	if ( new_partition .partition_number > 0 &&
	     (	
		new_partition .type == GParted::TYPE_EXTENDED ||
	     	(
	     	   wait_for_node( new_partition .get_path() ) && 
	     	   erase_filesystem_signatures( new_partition )
	     	)
	     )
	   )
	{ 
		operation_details .back() .status = OperationDetails::SUCCES ;
		
		return true ;
	}
	else
	{		
		if ( ! ped_error .empty() )
			operation_details .back() .sub_details .push_back( 
				OperationDetails( "<i>" + ped_error + "</i>", OperationDetails::NONE ) ) ;
		
		operation_details .back() .status = OperationDetails::ERROR ;	
		
		return false ;
	} 
}
	
bool GParted_Core::create_filesystem( const Partition & partition, std::vector<OperationDetails> & operation_details ) 
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::get_filesystem_string( partition .filesystem ) ) ) ) ;
	
	set_proper_filesystem( partition .filesystem ) ;

	if ( p_filesystem && p_filesystem ->Create( partition, operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

bool GParted_Core::format( const Partition & partition, std::vector<OperationDetails> & operation_details )
{	
	//remove all filesystem signatures...
	erase_filesystem_signatures( partition ) ;
	
	return set_partition_type( partition, operation_details ) &&
				   create_filesystem( partition, operation_details ) ;
}

bool GParted_Core::Delete( const Partition & partition, std::vector<OperationDetails> & operation_details ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( partition .device_path ) )
	{
		if ( partition .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else
			lp_partition = ped_disk_get_partition_by_sector( 
						lp_disk,
						(partition .sector_end + partition .sector_start) / 2 ) ;
		
		return_value = ped_disk_delete_partition( lp_disk, lp_partition ) && commit() ;
		sleep( 1 ) ; //give the kernel some time to reread the partitiontable
	
		close_device_and_disk() ;
	}
	
	return return_value ;
}
	
bool GParted_Core::resize_move( const Device & device,
				const Partition & partition_old,
			  	Partition & partition_new,
			  	std::vector<OperationDetails> & operation_details ) 
{
	if ( partition_new .sector_start != partition_old .sector_start &&
	     get_fs( partition_old .filesystem ) .move == GParted::FS::GPARTED )
		return move( device, partition_old, partition_new, operation_details ) ;
	else
		return resize( device, partition_old, partition_new, operation_details ) ;
}

bool GParted_Core::move( const Device & device,
			 const Partition & partition_old,
		   	 Partition & partition_new,
		   	 std::vector<OperationDetails> & operation_details ) 
{
	//FIXME: look very carefully at this function and see where things (like checking and fsmaximizing) are 
	//missing.
	if ( partition_new .get_length() > partition_old .get_length() )
	{
		//first do the move 
		Partition temp = partition_new ;
		temp .sector_end = partition_new .sector_start + partition_old .get_length() ;
		if ( move_filesystem( partition_old, temp, operation_details ) )
		{
			//now move the partition
			if ( resize_move_partition( partition_old,
					       	     temp,
						     false,
						     operation_details,
						     temp .get_length() ) )
			{
				//now the partition and the filesystem are moved, we can grow it..
				partition_new .sector_start = temp .sector_start ;
				return resize( device, temp, partition_new, operation_details ) ;
			}
		}

		return false ;
	}
	else if ( partition_new .get_length() < partition_old .get_length() )
	{
		//first shrink the partition
		Partition temp = partition_old ;
		temp .sector_end = partition_old .sector_start + partition_new .get_length() -1 ;
		if ( resize( device, partition_old, temp, operation_details ) )
		{
			//now we can move it
			partition_new .sector_end = partition_new .sector_start + temp .get_length() -1 ;
			return move_filesystem( temp, partition_new, operation_details ) &&
			       resize_move_partition( temp,
			       			      partition_new,
						      false,
						      operation_details,
						      partition_new .get_length() ) ;
		}

		return false ;
	}
	else
		return move_filesystem( partition_old, partition_new, operation_details ) &&
		       resize_move_partition( partition_old,
		       			      partition_new,
					      false,
					      operation_details,
					      partition_new .get_length() ) ;
}

bool GParted_Core::move_filesystem( const Partition & partition_old,
		   		    Partition & partition_new,
				    std::vector<OperationDetails> & operation_details ) 
{
	operation_details .push_back( OperationDetails( _("move filesystem.") ) ) ;

	bool succes = false ;
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		//calculate correct geom voor new location (rounded to cylinder)
		lp_partition = NULL ;
		lp_partition = ped_disk_get_partition_by_sector(
					lp_disk,
					(partition_old .sector_end + partition_old .sector_start) / 2 ) ;
		
		if ( lp_partition )
		{
			//calculate a new partition just to get a correct geometry
			//(we could use lp_partition as it is, but this feels safer)
			lp_partition = ped_partition_new( lp_disk,
						    	  lp_partition ->type,
						    	  lp_partition ->fs_type,
						    	  partition_new .sector_start,
						    	  partition_new .sector_end ) ;

			if ( lp_partition )
			{
				PedConstraint *constraint = NULL ;
				constraint = ped_constraint_any( lp_device ) ;

				if ( constraint )
				{
					if ( ped_disk_set_partition_geom( lp_disk,
									  lp_partition,
									  constraint,
									  partition_new .sector_start,
									  partition_new .sector_end ) )
					{
						partition_new .sector_start = lp_partition ->geom .start ;
						partition_new .sector_end = lp_partition ->geom .end ;
						succes = true ;
					}
					
					ped_constraint_destroy( constraint );
				}
			}
		}
		//we don't need disk anymore..	
		close_disk() ;	
		
		//do the move..
		Sector blocksize = 32 ;
		Glib::ustring error_message ;

		if ( succes && ped_device_open( lp_device ) )
		{
			ped_device_sync( lp_device ) ;
		
			//add an empty sub which we will constantly update in the loop
			operation_details .back() .sub_details .push_back( 
				OperationDetails( "", OperationDetails::NONE ) ) ;

			if ( partition_new .sector_start < partition_old .sector_start )	
			{
				Sector t = 0 ;
				for ( ; t < partition_old .get_length() - blocksize ; t+=blocksize )
				{
					if ( ! copy_block( lp_device,
						    	   lp_device,
						    	   partition_old .sector_start +t,
						    	   partition_new .sector_start +t,
						    	   blocksize,
						    	   error_message ) )
					{
						succes = false ;
						break ;
					}
					
					if ( t % (blocksize * 100) == 0 )
					{
						operation_details .back() .sub_details .back() .progress_text =
							String::ucompose( _("%1 of %2 moved"),
							  		  Utils::format_size( t +1 ),
							  		  Utils::format_size( partition_old .get_length() ) ) ; 
				
						operation_details .back() .sub_details .back() .description =
							"<i>" + 
							operation_details .back() .sub_details .back() .progress_text + 
							"</i>" ;

						operation_details .back() .sub_details .back() .fraction =
							t / static_cast<double>( partition_old .get_length() ) ;
					}
				}

				//copy the last couple of sectors..
				if ( succes )
				{
					Sector last_sectors = partition_old .get_length() -1 - t + blocksize ;
					succes = copy_block( lp_device,
						    	     lp_device,
						    	     partition_old .sector_start +t,
						    	     partition_new .sector_start +t,
						    	     last_sectors,
						    	     error_message ) ;
				}
			}
			else //move to the right..
			{	
				Sector t = blocksize ;
				for ( ; t < partition_old .get_length() - blocksize  ; t+=blocksize )
				{
					if ( ! copy_block( lp_device,
						    	   lp_device,
						    	   partition_old .sector_start +1 - t,
						    	   partition_new .sector_start +1 - t,
						    	   blocksize,
						    	   error_message ) )
					{
						succes = false ;
						break ;
					}
				
					if ( t % (blocksize * 100) == 0 )
					{
						operation_details .back() .sub_details .back() .progress_text =
							String::ucompose( _("%1 of %2 moved"),
							  		  Utils::format_size( t +1 ),
							  		  Utils::format_size( partition_old .get_length() ) ) ; 
				
						operation_details .back() .sub_details .back() .description =
							"<i>" + 
							operation_details .back() .sub_details .back() .progress_text + 
							"</i>" ;

						operation_details .back() .sub_details .back() .fraction =
							t / static_cast<double>( partition_old .get_length() ) ;
					}
				}

				//copy the last couple of sectors..
				if ( succes )
				{
					Sector last_sectors = partition_old .get_length() -1 - t + blocksize ;
					succes = copy_block( lp_device,
						    	     lp_device,
						    	     partition_old .sector_start,
						    	     partition_new .sector_start,
						    	     last_sectors,
						    	     error_message ) ;
				}
			}

			//final description
			operation_details .back() .sub_details .back() .description =
				"<i>" +
				String::ucompose( _("%1 of %2 moved"),
						  Utils::format_size( partition_old .get_length() ),
						  Utils::format_size( partition_old .get_length() ) ) + 
				"</i>" ;
			
			//reset fraction to -1 to make room for a new one (or a pulsebar)
			operation_details .back() .sub_details .back() .fraction = -1 ;

			if( ! succes )
			{
				if ( ! error_message .empty() )
					operation_details .back() .sub_details .push_back( 
						OperationDetails( error_message, OperationDetails::NONE ) ) ;

				if ( ! ped_error .empty() )
					operation_details .back() .sub_details .push_back( 
						OperationDetails( "<i>" + ped_error + "</i>", OperationDetails::NONE ) ) ;
			}


		}

		close_device_and_disk() ;
	}

	operation_details .back() .status = succes ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return succes ;
}

bool GParted_Core::resize( const Device & device,
			   const Partition & partition_old, 
			   Partition & partition_new,
			   std::vector<OperationDetails> & operation_details ) 
{
	//extended partition
	if ( partition_old .type == GParted::TYPE_EXTENDED )
		return resize_move_partition( partition_old, partition_new, false, operation_details ) ;
	
	bool succes = false ;
	
	//resize using libparted..
	if ( get_fs( partition_old .filesystem ) .grow == GParted::FS::LIBPARTED ||
	     get_fs( partition_old .filesystem ) .shrink == GParted::FS::LIBPARTED ||
	     get_fs( partition_old .filesystem ) .move == GParted::FS::LIBPARTED )
	{
		if ( check_repair( partition_new, operation_details ) )
		{
			succes = LP_resize_partition_and_filesystem( partition_old, partition_new, operation_details ) ;

			//always check after a resize, but if it failes the whole operation failes 
			if ( ! check_repair( partition_new, operation_details ) )
				succes = false ;
		}
		
		return succes ;
	}

	//use custom resize tools..
	if ( check_repair( partition_new, operation_details ) )
	{
		succes = true ;
		//FIXME, find another way to resolve this cylsize problem...	
		if ( partition_new .get_length() < partition_old .get_length() )
			succes = resize_filesystem( partition_old, partition_new, operation_details, device .cylsize ) ;
						
		if ( succes )
			succes = resize_move_partition(
					partition_old,
			     		partition_new,
					! get_fs( partition_new .filesystem ) .move,
					operation_details ) ;
			
		//these 3 are always executed, however, if 1 of them fails the whole operation fails
		if ( ! check_repair( partition_new, operation_details ) )
			succes = false ;

		//expand filesystem to fit exactly in partition
		if ( ! maximize_filesystem( partition_new, operation_details ) )
			succes = false ;
			
		if ( ! check_repair( partition_new, operation_details ) )
			succes = false ;

		return succes ;
	}
		
	return false ;
}

bool GParted_Core::resize_move_partition( const Partition & partition_old,
				     	  Partition & partition_new,
					  bool fixed_start,
					  std::vector<OperationDetails> & operation_details,
					  Sector min_size )
{
	operation_details .push_back( OperationDetails( _("resize/move partition") ) ) ;
	/*FIXME: try to find fitting descriptions for all situations
	if ( partition_new .get_length() < partition_old .get_length() )
		operation_details .push_back( OperationDetails( _("shrink partition") ) ) ;
	else if ( partition_new .get_length() > partition_old .get_length() )
		operation_details .push_back( OperationDetails( _("grow partition") ) ) ;
	else if ( partition_new .sector_start != partition_old .sector_start )
		operation_details .push_back( OperationDetails( _("move partition") ) ) ;
	else
	{
		operation_details .push_back( OperationDetails( _("resize/move partition") ) ) ;
		operation_details .back() .sub_details .push_back( 
			"<i>" + 
			OperationDetails( _("new and old partition have the same size and positition. continuing anyway") ) +
			"</i>" ) ;
	}*/

	operation_details .back() .sub_details .push_back( 
		OperationDetails(
			"<i>" +
			String::ucompose( _("old start: %1"), partition_old .sector_start ) + "\n" +
			String::ucompose( _("old end: %1"), partition_old .sector_end ) + "\n" +
			String::ucompose( _("old size: %1"), Utils::format_size( partition_old .get_length() ) ) + 
			"</i>",
		OperationDetails::NONE ) ) ;
	
	bool return_value = false ;
	
	PedConstraint *constraint = NULL ;
	lp_partition = NULL ;
	ped_error .clear() ;
		
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		if ( partition_old .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else		
			lp_partition = ped_disk_get_partition_by_sector(
						lp_disk,
						(partition_old .sector_end + partition_old .sector_start) / 2 ) ;
		
		if ( lp_partition )
		{
			constraint = ped_constraint_any( lp_device );
			
			if ( fixed_start && constraint )
			{ 
				//create a constraint which keeps de startpoint intact and rounds the end to a cylinder
				ped_disk_set_partition_geom( lp_disk,
							     lp_partition,
							     constraint,
							     partition_new .sector_start,
							     partition_new .sector_end ) ;
				ped_constraint_destroy( constraint );
				constraint = NULL ;
				
				ped_geometry_set_start( & lp_partition ->geom, partition_new .sector_start ) ;
				constraint = ped_constraint_exact( & lp_partition ->geom ) ;
			}
			else if ( min_size > 0 )
				constraint ->min_size = min_size ;//at this moment min_size and fixed start are mut. excl.
								  //this might change in the (near) future.

			if ( constraint )
			{
				if ( ped_disk_set_partition_geom( lp_disk,
								  lp_partition,
								  constraint,
								  partition_new .sector_start,
								  partition_new .sector_end ) )
					return_value = commit() ;
									
				ped_constraint_destroy( constraint );
			}
		}
		
		close_device_and_disk() ;
	}
	
	if ( return_value )
	{
		partition_new .sector_start = lp_partition ->geom .start ;
		partition_new .sector_end = lp_partition ->geom .end ;
		
		operation_details .back() .sub_details .push_back( 
			OperationDetails(
				"<i>" +
				String::ucompose( _("new start: %1"), partition_new .sector_start ) + "\n" +
				String::ucompose( _("new end: %1"), partition_new .sector_end ) + "\n" +
				String::ucompose( _("new size: %1"), Utils::format_size( partition_new .get_length() ) ) + 
				"</i>",
			OperationDetails::NONE ) ) ;
	}
	else if ( ! ped_error .empty() )
		operation_details .back() .sub_details .push_back( 
			OperationDetails( "<i>" + ped_error + "</i>", OperationDetails::NONE ) ) ;
	
	if ( partition_old .type == GParted::TYPE_EXTENDED )
	{
		operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
		return return_value ;
	}
	else
	{
		return_value &= wait_for_node( partition_new .get_path() ) ;
		operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;

		return return_value ;
	}
}
	
bool GParted_Core::resize_filesystem( const Partition & partition_old,
				      const Partition & partition_new,
				      std::vector<OperationDetails> & operation_details,
				      Sector cylinder_size,
				      bool fill_partition ) 
{
	if ( ! fill_partition )
	{
		if ( partition_new .get_length() < partition_old .get_length() )
			operation_details .push_back( OperationDetails( _("shrink filesystem") ) ) ;
		else if ( partition_new .get_length() > partition_old .get_length() )
			operation_details .push_back( OperationDetails( _("grow filesystem") ) ) ;
		else
			operation_details .push_back( 
				OperationDetails( _("new and old partition have the same size. continuing anyway") ) ) ;
	}

	set_proper_filesystem( partition_new .filesystem, cylinder_size ) ;
	if ( p_filesystem && p_filesystem ->Resize( partition_new, operation_details .back() .sub_details, fill_partition ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}
	
//FIXME: perform checks in maximize_filesystem(), resize_filesystem() and check_repair() to see if 
//the specified action is supported. if not, return true, but set status of operation to NOT_AVAILABLE
//(or maybe we should simply skip the entire suboperation?)
bool GParted_Core::maximize_filesystem( const Partition & partition,
					std::vector<OperationDetails> & operation_details ) 
{
	operation_details .push_back( OperationDetails( _("grow filesystem to fill the partition") ) ) ;
	
	return resize_filesystem( partition, partition, operation_details, 0, true ) ;
}

bool GParted_Core::LP_resize_partition_and_filesystem( const Partition & partition_old,
						       Partition & partition_new,
						       std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("resize partition and filesystem using libparted") ) ) ;
	
	bool return_value = false ;
	
	PedFileSystem *fs = NULL ;
	PedConstraint *constraint = NULL ;
	lp_partition = NULL ;
	ped_error .clear() ;
	
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		lp_partition = ped_disk_get_partition_by_sector( 
					lp_disk,
					(partition_old .sector_end + partition_old .sector_start) / 2 ) ;
		if ( lp_partition )
		{
			fs = ped_file_system_open( & lp_partition ->geom );
			if ( fs )
			{
				constraint = ped_file_system_get_resize_constraint( fs );
				if ( constraint )
				{
					if ( ped_disk_set_partition_geom( lp_disk,
									  lp_partition,
									  constraint,
									  partition_new .sector_start,
									  partition_new .sector_end ) 
					     &&
					     ped_file_system_resize( fs, & lp_partition ->geom, NULL )
                                           )
					{
						return_value = commit() ;

						if ( return_value )
						{
							partition_new .sector_start = lp_partition ->geom .start ;
							partition_new .sector_end = lp_partition ->geom .end ; 
						}
					}

					ped_constraint_destroy( constraint );
				}
				
				ped_file_system_close( fs );
			}
		}
		
		close_device_and_disk() ;
	}
	
	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
        
	if ( ! return_value && ! ped_error .empty() )
		operation_details .back() .sub_details .push_back( 
			OperationDetails( "<i>" + ped_error + "</i>", OperationDetails::NONE ) ) ;
	
	return return_value ;
}

bool GParted_Core::copy( const Partition & partition_src,
			 Partition & partition_dest,
			 Sector min_size,
			 Sector block_size,
			 std::vector<OperationDetails> & operation_details ) 
{
	if ( check_repair( partition_src, operation_details ) )
	{
		bool succes = true ;
		if ( partition_dest .status == GParted::STAT_NEW )
			succes = create_partition( partition_dest, operation_details, min_size ) ;

		if ( succes && set_partition_type( partition_dest, operation_details ) )
		{
			operation_details .push_back( OperationDetails( 
				String::ucompose( _("copy filesystem of %1 to %2"),
						  partition_src .get_path(),
						  partition_dest .get_path() ) ) ) ;
						

			switch ( get_fs( partition_dest .filesystem ) .copy )
			{
				case  GParted::FS::GPARTED :
						succes = copy_filesystem( partition_src,
									  partition_dest,
									  operation_details,
									  block_size ) ;
						break ;
				
				case  GParted::FS::LIBPARTED :
						//FIXME: see if copying through libparted has any advantages
						break ;

				case  GParted::FS::EXTERNAL :
						set_proper_filesystem( partition_dest .filesystem ) ;
						succes = p_filesystem &&
							 p_filesystem ->Copy( partition_src .get_path(),
								     	      partition_dest .get_path(),
									      operation_details .back() .sub_details ) ;
						break ;

				default :
						succes = false ;
						break ;
			}

			operation_details .back() .status = succes ? OperationDetails::SUCCES : OperationDetails::ERROR ;

			return ( succes &&	
			     	 check_repair( partition_dest, operation_details ) &&
			     	 maximize_filesystem( partition_dest, operation_details ) &&
			     	 check_repair( partition_dest, operation_details ) ) ;
		}
	}

	return false ;
}

bool GParted_Core::copy_filesystem( const Partition & partition_src,
				    const Partition & partition_dest,
				    std::vector<OperationDetails> & operation_details,
				    Sector block_size ) 
{
	operation_details .back() .sub_details .push_back( OperationDetails( 
		"<i>" + String::ucompose( _("Use a blocksize of %1 (%2 sectors)"),
				  	  Utils::format_size( block_size ),
				  	  block_size ) + "</i>",
		OperationDetails::NONE ) ) ;
	
	bool succes = false ;
	char buf[block_size * 512] ; 
	PedDevice *lp_device_src, *lp_device_dest ;
//FIXME: adapt open_device() so we don't have to call ped_device_get() here
//(same goes for close_device() and ped_device_destroy()
	lp_device_src = ped_device_get( partition_src .device_path .c_str() );

	if ( partition_src .device_path != partition_dest .device_path )
		lp_device_dest = ped_device_get( partition_dest .device_path .c_str() );
	else
		lp_device_dest = lp_device_src ;

	if ( lp_device_src && lp_device_dest && ped_device_open( lp_device_src ) && ped_device_open( lp_device_dest ) )
	{
		ped_device_sync( lp_device_dest ) ;

		//add an empty sub which we will constantly update in the loop
		operation_details .back() .sub_details .push_back( 
			OperationDetails( "", OperationDetails::NONE ) ) ;

		Glib::ustring error_message ;
		Sector t = 0 ;
		for ( ; t < partition_src .get_length() - block_size ; t+=block_size )
		{
			if ( ! ped_device_read( lp_device_src, buf, partition_src .sector_start + t, block_size ) )
			{
				error_message = "<i>" + String::ucompose( _("Error while reading block at sector %1"),
							  	  	  partition_src .sector_start + t ) + "</i>" ;

				break ;
			}
				
			if ( ! ped_device_write( lp_device_dest, buf, partition_dest .sector_start + t, block_size ) )
			{
				error_message = "<i>" + String::ucompose( _("Error while writing block at sector %1"),
							  	  	  partition_src .sector_start + t ) + "</i>" ;

				break ;
			}

			if ( t % ( block_size * 100 ) == 0 )
			{
				operation_details .back() .sub_details .back() .progress_text =
					String::ucompose( _("%1 of %2 copied"),
							  Utils::format_size( t +1 ),
							  Utils::format_size( partition_src .get_length() ) ) ; 
				
				operation_details .back() .sub_details .back() .description =
					"<i>" + operation_details .back() .sub_details .back() .progress_text + "</i>" ;

				operation_details .back() .sub_details .back() .fraction =
					t / static_cast<double>( partition_src .get_length() ) ;
			}
		}

		//copy the last couple of sectors..
		Sector last_sectors = partition_src .get_length() - t ;//FIXME: most likely this is incorrect, look at move_filesystem to see how they do it.
		if ( ped_device_read( lp_device_src, buf, partition_src .sector_start + t, last_sectors ) )
		{
			if ( ped_device_write( lp_device_dest, buf, partition_dest .sector_start + t , last_sectors ) )
				t += last_sectors ;
			else
				error_message = "<i>" + String::ucompose( _("Error while writing block at sector %1"),
							  	  	  partition_src .sector_start + t ) + "</i>" ;
		}
		else
			error_message = "<i>" + String::ucompose( _("Error while reading block at sector %1"),
						  	  	  partition_src .sector_start + t ) + "</i>" ;
				
		//final description
		operation_details .back() .sub_details .back() .description =
			"<i>" +
			String::ucompose( _("%1 of %2 copied"),
					  Utils::format_size( t +1 ),
					  Utils::format_size( partition_src .get_length() ) ) + 
			"</i>" ;
		//reset fraction to -1 to make room for a new one (or a pulsebar)
		operation_details .back() .sub_details .back() .fraction = -1 ;

		if ( t == partition_src .get_length() )
		{
			succes = true ;
		}
		else
		{
			if ( ! error_message .empty() )
				operation_details .back() .sub_details .push_back( 
					OperationDetails( error_message, OperationDetails::NONE ) ) ;

			if ( ! ped_error .empty() )
				operation_details .back() .sub_details .push_back( 
					OperationDetails( "<i>" + ped_error + "</i>", OperationDetails::NONE ) ) ;
		}
		
		//close the devices..
		ped_device_close( lp_device_src ) ;

		if ( partition_src .device_path != partition_dest .device_path )
			ped_device_close( lp_device_dest ) ;

		//detroy the devices..
		ped_device_destroy( lp_device_src ) ; 

		if ( partition_src .device_path != partition_dest .device_path )
			ped_device_destroy( lp_device_dest ) ;
	}
	else
		operation_details .back() .sub_details .push_back( 
			OperationDetails( _("An error occurred while opening the devices"), OperationDetails::NONE ) ) ;


	operation_details .back() .status = succes ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return succes ;
}
	
bool GParted_Core::check_repair( const Partition & partition, std::vector<OperationDetails> & operation_details ) 
{
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("check filesystem on %1 for errors and (if possible) fix them"),
						  partition .get_path() ) ) ) ;
	
	set_proper_filesystem( partition .filesystem ) ;

	if ( p_filesystem && p_filesystem ->Check_Repair( partition, operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

bool GParted_Core::set_partition_type( const Partition & partition,
	   			       std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("set partitiontype") ) ) ;
	
	bool return_value = false ;
	
	if ( open_device_and_disk( partition .device_path ) )
	{
		PedFileSystemType * fs_type = 
			ped_file_system_type_get( Utils::get_filesystem_string( partition .filesystem ) .c_str() ) ;

		//default is Linux (83)
		if ( ! fs_type )
			fs_type = ped_file_system_type_get( "ext2" ) ;

		if ( fs_type )
		{
			lp_partition = ped_disk_get_partition_by_sector( 
						lp_disk,
						(partition .sector_end + partition .sector_start) / 2 ) ;

			if ( lp_partition && ped_partition_set_system( lp_partition, fs_type ) && commit() )
				return_value = wait_for_node( partition .get_path() ) ;
		}

		close_device_and_disk() ;
	}

	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return return_value ;
}
	
bool GParted_Core::copy_block( PedDevice * lp_device_src,
			       PedDevice * lp_device_dst,
			       Sector offset_src,
			       Sector offset_dst,
			       Sector blocksize,
			       Glib::ustring & error_message ) 
{
	char buf[blocksize * 512] ;

	if ( ! ped_device_read( lp_device_src, buf, offset_src, blocksize ) )
	{
		error_message = 
			"<i>" + String::ucompose( _("Error while reading block at sector %1"), offset_src ) + "</i>" ;

		return false ;
	}
				
	if ( ! ped_device_write( lp_device_dst, buf, offset_dst, blocksize ) )
	{
		error_message =
			"<i>" + String::ucompose( _("Error while writing block at sector %1"), offset_dst ) + "</i>" ;

		return false ;
	}

	return true ;
}

void GParted_Core::set_proper_filesystem( const FILESYSTEM & filesystem, Sector cylinder_size )
{
	if ( p_filesystem )
		delete p_filesystem ;
		
	switch( filesystem )
	{
		case FS_EXT2		: p_filesystem = new ext2() ;	 	break ;
		case FS_EXT3		: p_filesystem = new ext3() ; 		break ;
		case FS_LINUX_SWAP	: p_filesystem = new linux_swap() ; 	break ;
		case FS_FAT16		: p_filesystem = new fat16() ; 		break ;
		case FS_FAT32		: p_filesystem = new fat32() ; 		break ;
		case FS_NTFS		: p_filesystem = new ntfs() ; 		break ;
		case FS_REISERFS	: p_filesystem = new reiserfs() ; 	break ;
		case FS_REISER4		: p_filesystem = new reiser4() ;	break ;
		case FS_XFS		: p_filesystem = new xfs() ; 		break ;
		case FS_JFS		: p_filesystem = new jfs() ; 		break ;
		case FS_HFS		: p_filesystem = new hfs() ; 		break ;
		case FS_HFSPLUS		: p_filesystem = new hfsplus() ; 	break ;
		case FS_UFS		: p_filesystem = new ufs() ;	 	break ;

		default			: p_filesystem = NULL ;
	}

	if ( p_filesystem )
		p_filesystem ->cylinder_size = cylinder_size ;
}
	
bool GParted_Core::wait_for_node( const Glib::ustring & node ) 
{
	//we'll loop for 10 seconds or till 'node' appeares...
	for( short t = 0 ; t < 50 ; t++ )
	{
		//FIXME: find a better way to check if a file exists
		//the current way is suboptimal (at best)
		if ( Glib::file_test( node, Glib::FILE_TEST_EXISTS ) )
		{
			//same issue
			sleep( 2 ) ; 
			return true ;
		}
		else
			usleep( 200000 ) ; //200 milliseconds
	}

	return false ;
}

bool GParted_Core::erase_filesystem_signatures( const Partition & partition ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( partition .device_path ) )
	{
		lp_partition = ped_disk_get_partition_by_sector(
					lp_disk,
					(partition .sector_end + partition .sector_start) / 2 ) ;

		if ( lp_partition && ped_file_system_clobber( & lp_partition ->geom ) )
		{
			//filesystems not yet supported by libparted
			if ( ped_device_open( lp_device ) )
			{
				//reiser4 stores "ReIsEr4" at sector 128
				return_value = ped_geometry_write( & lp_partition ->geom, "0000000", 128, 1 ) ;

				ped_device_close( lp_device ) ;
			}
		}
		
		close_device_and_disk() ;	
	}

	return return_value ;
}
	
//FIXME open_device( _and_disk) and the close functions should take an PedDevice * and PedDisk * as argument
//basicly we should get rid of these global lp_device and lp_disk
bool GParted_Core::open_device( const Glib::ustring & device_path )
{
	lp_device = ped_device_get( device_path .c_str() );
	
	return lp_device ;
}
	
bool GParted_Core::open_device_and_disk( const Glib::ustring & device_path, bool strict ) 
{
	lp_device = NULL ;
	lp_disk = NULL ;

	if ( open_device( device_path ) )
	{
		lp_disk = ped_disk_new( lp_device );
	
		//if ! disk and writeable it's probably a HD without disklabel.
		//We return true here and deal with them in GParted_Core::get_devices
		if ( lp_disk || ( ! strict && ! lp_device ->read_only ) )
			return true ;
		
		close_device_and_disk() ;
	}

	return false ;
}

void GParted_Core::close_disk()
{
	if ( lp_disk )
		ped_disk_destroy( lp_disk ) ;
	
	lp_disk = NULL ;
}

void GParted_Core::close_device_and_disk()
{
	close_disk() ;

	if ( lp_device )
		ped_device_destroy( lp_device ) ;
		
	lp_device = NULL ;
}	

bool GParted_Core::commit() 
{
	bool return_value = ped_disk_commit_to_dev( lp_disk ) ;
	
	ped_disk_commit_to_os( lp_disk ) ;
	
	return return_value ;
}
	
PedExceptionOption GParted_Core::ped_exception_handler( PedException * e ) 
{
        std::cout << e ->message << std::endl ;
	ped_error = e ->message ;
        
	return PED_EXCEPTION_UNHANDLED ;
}

GParted_Core::~GParted_Core() 
{
	if ( p_filesystem )
		delete p_filesystem ;
}
	
} //GParted
