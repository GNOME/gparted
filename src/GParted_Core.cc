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

std::vector<Glib::ustring> libparted_messages ; //see ped_exception_handler()

namespace GParted
{

GParted_Core::GParted_Core() 
{
	lp_device = NULL ;
	lp_disk = NULL ;
	lp_partition = NULL ;
	p_filesystem = NULL ;

	ped_exception_set_handler( ped_exception_handler ) ; 
	
	//disable automount //FIXME: temporary hack, till i find a better solution...
	std::ofstream fdi_file( "/usr/share/hal/fdi/policy/gparted-disable-automount.fdi" ) ;
	if ( fdi_file )
	{
		fdi_file << "<deviceinfo version='0.2'>" ;
		fdi_file << "<device>" ;
		fdi_file << "<match key='@block.storage_device:storage.hotpluggable' bool='true'>" ;
		fdi_file << "<merge key='volume.ignore' type='bool'>true</merge>" ;
		fdi_file << "</match>" ;
		fdi_file << "</device>" ;
		fdi_file << "</deviceinfo>" ;

		fdi_file .close() ;
	}	

	//get valid flags ...
	for ( PedPartitionFlag flag = ped_partition_flag_next( static_cast<PedPartitionFlag>( NULL ) ) ;
	      flag ;
	      flag = ped_partition_flag_next( flag ) )
		flags .push_back( flag ) ;
	
	//throw libpartedversion to the stdout to see which version is actually used.
	std::cout << "======================" << std::endl ;
	std::cout << "libparted : " << ped_get_version() << std::endl ;
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
	//FIXME: since parted 1.7.1 i detect an unusable /dev/md/0 device (voyager), take a look at this and
	//find out how to filter it from the list
		lp_device = ped_device_get_next( NULL );
		while ( lp_device ) 
		{
			device_paths .push_back( lp_device ->path ) ;

			lp_device = ped_device_get_next( lp_device ) ;
		}
		close_device_and_disk() ;

		std::sort( device_paths .begin(), device_paths .end() ) ;
	}
	
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

bool GParted_Core::snap_to_cylinder( const Device & device, Partition & partition, Glib::ustring & error ) 
{
	if ( ! partition .strict )
	{
		//FIXME: this doesn't work to well, when dealing with layouts which contain an extended partition
		//because of the metadata length % cylindersize isn't always 0
		Sector diff = partition .sector_start % device .cylsize ;
		if ( diff )
		{
			if ( diff < ( device .cylsize / 2 ) )
				partition .sector_start -= diff ;
			else
				partition .sector_start += (device .cylsize - diff ) ;
		}
	
		diff = (partition .sector_end +1) % device .cylsize ;
		if ( diff )
		{
			if ( diff < ( device .cylsize / 2 ) )
				partition .sector_end -= diff ;
			else
				partition .sector_end += (device .cylsize - diff ) ;
		}
	
		if ( partition .sector_start < 0 )
			partition .sector_start = 0 ;
		if ( partition .sector_end > device .length )
			partition .sector_end = device .length -1 ;

		//ok, do some basic checks on the partition..
		if ( partition .get_length() <= 0 )
		{
			error = String::ucompose( _("A partition cannot have a length of %1 sectors"),
						  partition .get_length() ) ;
			return false ;
		}

		if ( partition .get_length() < partition .sectors_used )
		{
			error = String::ucompose( 
				_("A partition with used sectors (%1) greater than it's length (%2) is not valid"),
				partition .sectors_used,
				partition .get_length() ) ;
			return false ;
		}

		//FIXME: it would be perfect if we could check for overlapping with adjacent partitions as well,
		//however, finding the adjacent partitions is not as easy as it seems and at this moment all the dialogs
		//already perform these checks. A perfect 'fixme-later' ;)
	}

	return true ;
}

bool GParted_Core::apply_operation_to_disk( Operation * operation )
{
	bool succes = true ;
	libparted_messages .clear() ;
	
	if ( operation ->partition_original .status == GParted::STAT_COPY )
	{
		succes = false ;
		operation ->operation_detail .add_child( 
			OperationDetail( String::ucompose( _("find real path of %1"),
							   operation ->partition_original .get_path() ) ) ) ;

		if ( open_device_and_disk( operation ->partition_original .device_path ) )
		{
			lp_partition = ped_disk_get_partition_by_sector( lp_disk,
									 operation ->partition_original .get_sector() ) ;
		
			if ( lp_partition )
			{
				char * lp_path = ped_partition_get_path( lp_partition ) ;
				
				operation ->partition_original .add_path( lp_path, true ) ;
				operation ->partition_new .add_path( lp_path, true ) ;

				free( lp_path ) ;
				succes = true ;

				operation ->operation_detail .get_last_child() .add_child( 
					OperationDetail( String::ucompose( _("path: %1"),
									   operation ->partition_new .get_path() ),
							 STATUS_NONE,
							 FONT_ITALIC ) ) ;
			}

			close_device_and_disk() ;
		}

		operation ->operation_detail .get_last_child() .set_status(  succes ? STATUS_SUCCES : STATUS_ERROR ) ;
	}

	if ( succes )
		switch ( operation ->type )
		{	     
			case OPERATION_DELETE:
				succes = Delete( operation ->partition_original, operation ->operation_detail ) ;
				break ;
			case OPERATION_CREATE:
				succes = create( operation ->device, 
					         operation ->partition_new,
					         operation ->operation_detail ) ;
				break ;
			case OPERATION_RESIZE_MOVE:
				succes = resize_move( operation ->device,
					       	      operation ->partition_original,
						      operation ->partition_new,
						      operation ->operation_detail ) ;
				break ;
			case OPERATION_FORMAT:
				succes = format( operation ->partition_new, operation ->operation_detail ) ;
				break ;
			case OPERATION_COPY: 
				//when copying to an existing partition we have to change the 'copy of..' to a valid path
				operation ->partition_new .add_path( operation ->partition_original .get_path(), true ) ;
				succes = copy( static_cast<OperationCopy*>( operation ) ->partition_copied,
					       operation ->partition_new,
					       static_cast<OperationCopy*>( operation ) ->partition_copied .get_length(),
					       operation ->operation_detail ) ;
				break ;
		}

	if ( libparted_messages .size() > 0 )
	{
		operation ->operation_detail .add_child( OperationDetail( _("libparted messages"), STATUS_INFO ) ) ;

		for ( unsigned int t = 0 ; t < libparted_messages .size() ; t++ )
			operation ->operation_detail .get_last_child() .add_child(
				OperationDetail( libparted_messages[ t ], STATUS_NONE, FONT_ITALIC ) ) ;
	}

	return succes ;
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
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition .get_sector() ) ;
	
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
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition .get_sector() ) ;
	
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

void GParted_Core::set_device_partitions( Device & device ) 
{
	int EXT_INDEX = -1 ;
	char * lp_path ;//we have to free the result of ped_partition_get_path()..
	
	//clear partitions
	device .partitions .clear() ;
	
	lp_partition = ped_disk_next_partition( lp_disk, NULL ) ;
	while ( lp_partition )
	{
		libparted_messages .clear() ;
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

		partition_temp .messages .insert( partition_temp .messages .end(),
						  libparted_messages. begin(),
						  libparted_messages .end() ) ;
		
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
		if ( Glib::ustring( lp_partition ->fs_type ->name ) == "extended" )
			return GParted::FS_EXTENDED ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "ext2" )
			return GParted::FS_EXT2 ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "ext3" )
			return GParted::FS_EXT3 ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "linux-swap" )
			return GParted::FS_LINUX_SWAP ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "fat16" )
			return GParted::FS_FAT16 ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "fat32" )
			return GParted::FS_FAT32 ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "ntfs" )
			return GParted::FS_NTFS ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "reiserfs" )
			return GParted::FS_REISERFS ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "xfs" )
			return GParted::FS_XFS ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "jfs" )
			return GParted::FS_JFS ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "hfs" )
			return GParted::FS_HFS ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "hfs+" )
			return GParted::FS_HFSPLUS ;
		else if ( Glib::ustring( lp_partition ->fs_type ->name ) == "ufs" )
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
	temp = _( "Unable to detect filesystem! Possible reasons are:" ) ;
	temp += "\n-"; 
	temp += _( "The filesystem is damaged" ) ;
	temp += "\n-" ; 
	temp += _( "The filesystem is unknown to GParted" ) ;
	temp += "\n-"; 
	temp += _( "There is no filesystem available (unformatted)" ) ; 
	
	partition_temp .messages .push_back( temp ) ;
	
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
					partitions[ t ] .messages .push_back( _("Unable to find mountpoint") ) ;
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
							partitions[ t ] .messages .push_back( 
								"statvfs (" + 
								partitions[ t ] .get_mountpoint() + 
								"): " + 
								Glib::strerror( errno ) ) ;
					}
				}
				else
				{
					switch( get_fs( partitions[ t ] .filesystem ) .read )
					{
						case GParted::FS::EXTERNAL	:
							if ( set_proper_filesystem( partitions[ t ] .filesystem ) )
								p_filesystem ->set_used_sectors( partitions[ t ] ) ;
							break ;
						case GParted::FS::LIBPARTED	:
							LP_set_used_sectors( partitions[ t ] ) ;
							break ;

						default:
							break ;
					}
				}

				if ( partitions[ t ] .sectors_used == -1 )
					partitions[ t ] .messages .push_back( temp ) ;
				
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
		lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition .get_sector() ) ;
		
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
}

void GParted_Core::set_flags( Partition & partition )
{
	for ( unsigned int t = 0 ; t < flags .size() ; t++ )
		if ( ped_partition_is_flag_available( lp_partition, flags[ t ] ) &&
		     ped_partition_get_flag( lp_partition, flags[ t ] ) )
			partition .flags .push_back( ped_partition_flag_get_name( flags[ t ] ) ) ;
}

bool GParted_Core::create( const Device & device, Partition & new_partition, OperationDetail & operationdetail ) 
{
	if ( new_partition .type == GParted::TYPE_EXTENDED )   
	{
		return create_partition( new_partition, operationdetail ) ;
	}
	else if ( create_partition( new_partition, operationdetail, get_fs( new_partition .filesystem ) .MIN ) )
	{
		if ( new_partition .filesystem == GParted::FS_UNFORMATTED )
			return true ;
		else
			return set_partition_type( new_partition, operationdetail ) &&
		       	       create_filesystem( new_partition, operationdetail ) ;
	}
	
	return false ;
}

bool GParted_Core::create_partition( Partition & new_partition, OperationDetail & operationdetail, Sector min_size )
{
	operationdetail .add_child( OperationDetail( _("create empty partition") ) ) ;
	
	new_partition .partition_number = 0 ;
		
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
					
					operationdetail .get_last_child() .add_child( OperationDetail( 
						String::ucompose( _("path: %1"), new_partition .get_path() ) + "\n" +
						String::ucompose( _("start: %1"), new_partition .sector_start ) + "\n" +
						String::ucompose( _("end: %1"), new_partition .sector_end ) + "\n" +
						String::ucompose( _("size: %1 (%2)"),
					  			  new_partition .get_length(),
					  			  Utils::format_size( new_partition .get_length() ) ),
						STATUS_NONE,
						FONT_ITALIC ) ) ;
				}
			
				ped_constraint_destroy( constraint );
			}
		}
				
		close_device_and_disk() ;
	}

	if ( new_partition .partition_number > 0 &&
	     erase_filesystem_signatures( new_partition ) &&
	     ( new_partition .type == GParted::TYPE_EXTENDED || wait_for_node( new_partition .get_path() ) ) )
	{ 
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		return true ;
	}
	else
	{		
		operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;	
		return false ;
	} 
}
	
bool GParted_Core::create_filesystem( const Partition & partition, OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( String::ucompose(
							_("create new %1 filesystem"),
							Utils::get_filesystem_string( partition .filesystem ) ) ) ) ;
	
	bool succes = false ;
	switch ( get_fs( partition .filesystem ) .create )
	{
		case GParted::FS::NONE:
			break ;
		case GParted::FS::GPARTED:
			break ;
		case GParted::FS::LIBPARTED:
			break ;
		case GParted::FS::EXTERNAL:
			succes = set_proper_filesystem( partition .filesystem ) &&
				 p_filesystem ->create( partition, operationdetail .get_last_child() ) ;

			break ;
	}

	operationdetail .get_last_child() .set_status(  succes ? STATUS_SUCCES : STATUS_ERROR ) ;
	return succes ;
}

bool GParted_Core::format( const Partition & partition, OperationDetail & operationdetail )
{	
	//remove all filesystem signatures...
	erase_filesystem_signatures( partition ) ;

	return set_partition_type( partition, operationdetail ) && create_filesystem( partition, operationdetail ) ;
}

bool GParted_Core::Delete( const Partition & partition, OperationDetail & operationdetail ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( partition .device_path ) )
	{
		if ( partition .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition .get_sector() ) ;
		
		return_value = ped_disk_delete_partition( lp_disk, lp_partition ) && commit() ;
	
		close_device_and_disk() ;
	}

	return return_value ;
}
	
bool GParted_Core::resize_move( const Device & device,
				const Partition & partition_old,
			  	Partition & partition_new,
			  	OperationDetail & operationdetail ) 
{
	if ( calculate_exact_geom( partition_old, partition_new, operationdetail ) )
	{
		//extended is a special case..
		if ( partition_old .type == GParted::TYPE_EXTENDED )
			return resize_move_partition( partition_old, partition_new, operationdetail ) ;
	
		//see if we need move or resize..
		if ( partition_new .sector_start == partition_old .sector_start )
			return resize( partition_old, partition_new, operationdetail ) ;
		else if ( partition_new .get_length() > partition_old .get_length() )
		{
			//first move, then grow...
			Partition temp = partition_new ;
			temp .sector_end = temp .sector_start + partition_old .get_length() -1 ;
			
			return calculate_exact_geom( partition_old, temp, operationdetail, temp .get_length() ) &&
			       move( device, partition_old, temp, operationdetail ) &&
			       resize( temp, partition_new, operationdetail ) ;
		}
		else if ( partition_new .get_length() < partition_old .get_length() )
		{
			//first shrink, then move..
			Partition temp = partition_old ;
			temp .sector_end = partition_old .sector_start + partition_new .get_length() -1 ;
		//FIXME: shrink and move of fat32 fs in 1 operation throws path not found errors	
			return calculate_exact_geom( partition_old, temp, operationdetail ) &&
			       resize( partition_old, temp, operationdetail ) &&
			       calculate_exact_geom( temp, partition_new, operationdetail, temp .get_length() ) &&
			       move( device, temp, partition_new, operationdetail ) ;
		}
		else
			return calculate_exact_geom( 
					partition_old, partition_new, operationdetail, partition_old .get_length() ) &&
			       move( device, partition_old, partition_new, operationdetail ) ;
	}

	return false ;
}

bool GParted_Core::move( const Device & device,
			 const Partition & partition_old,
		   	 const Partition & partition_new,
		   	 OperationDetail & operationdetail ) 
{
	return check_repair_filesystem( partition_old, operationdetail ) &&
	       move_filesystem( partition_old, partition_new, operationdetail ) &&
	       resize_move_partition( partition_old, partition_new, operationdetail ) &&
	       check_repair_filesystem( partition_new, operationdetail ) &&
	       maximize_filesystem( partition_new, operationdetail ) ;
}

bool GParted_Core::move_filesystem( const Partition & partition_old,
		   		    const Partition & partition_new,
				    OperationDetail & operationdetail ) 
{
	if ( partition_new .sector_start < partition_old .sector_start )
		operationdetail .add_child( OperationDetail( _("move filesystem to the left") ) ) ;
	else if ( partition_new .sector_start > partition_old .sector_start )
		operationdetail .add_child( OperationDetail( _("move filesystem to the right") ) ) ;
	else
	{
		operationdetail .add_child( OperationDetail( _("move filesystem") ) ) ;
		operationdetail .get_last_child() .add_child( 
			OperationDetail( _("new and old filesystem have the same positition. skipping this operation"),
					  STATUS_NONE,
					  FONT_ITALIC ) ) ;

		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		return true ;
	}

	bool succes = false ;
	switch ( get_fs( partition_old .filesystem ) .move )
	{
		case GParted::FS::NONE:
			break ;
		case GParted::FS::GPARTED:
			succes = move_filesystem_using_gparted( partition_old,
								partition_new,
								operationdetail .get_last_child() ) ;
			break ;
		case GParted::FS::LIBPARTED:
			succes = resize_move_filesystem_using_libparted( partition_old,
									 partition_new,
								  	 operationdetail .get_last_child() ) ;
			break ;
		case GParted::FS::EXTERNAL:
			break ;
	}

	operationdetail .get_last_child() .set_status(  succes ? STATUS_SUCCES : STATUS_ERROR ) ;
	return succes ;
}


bool GParted_Core::move_filesystem_using_gparted( const Partition & partition_old,
		      		    		  const Partition & partition_new,
				    		  OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( _("using internal algorithm"), STATUS_NONE ) ) ;

	CopyType copytype = partition_new .sector_start < partition_old .sector_start ? START_TO_END : END_TO_START ;
	Sector optimal_blocksize, offset ;

	return find_optimal_blocksize( partition_old, partition_new, copytype, optimal_blocksize, offset, operationdetail )
	       &&
	       copy_blocks( partition_old .device_path,
	  		    partition_new .device_path,
	  		    partition_old .sector_start + offset,
	  		    partition_new .sector_start + offset,
	  		    optimal_blocksize,
	  		    partition_old .get_length() - offset,
	  		    operationdetail, 
	  		    copytype ) ; 
}

bool GParted_Core::resize_move_filesystem_using_libparted( const Partition & partition_old,
		  	      		            	   const Partition & partition_new,
						    	   OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( _("using libparted"), STATUS_NONE ) ) ;

	bool return_value = false ;
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		PedPartition * lp_partition = NULL ;
		PedFileSystem *	fs = NULL ;
		PedGeometry * lp_geom = NULL ;	
		
		lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition_old .get_sector() ) ;
		if ( lp_partition )
		{
			fs = ped_file_system_open( & lp_partition ->geom );
			if ( fs )
			{
				lp_geom = ped_geometry_new( lp_device,
							    partition_new .sector_start,
							    partition_new .get_length() ) ;
				if ( lp_geom )
					return_value = ped_file_system_resize( fs, lp_geom, NULL ) &&
						       commit( partition_new .get_path() ) ;

				ped_file_system_close( fs );
			}
		}

		close_device_and_disk() ;
	}

	return return_value ;
}

bool GParted_Core::resize( const Partition & partition_old, 
			   const Partition & partition_new,
			   OperationDetail & operationdetail )
{
	bool succes = false ;
	if ( check_repair_filesystem( partition_new, operationdetail ) )
	{
		succes = true ;

		if ( succes && partition_new .get_length() < partition_old .get_length() )
			succes = resize_filesystem( partition_old, partition_new, operationdetail ) ;
						
		if ( succes )
			succes = resize_move_partition( partition_old, partition_new, operationdetail ) ;
			
		//these 2 are always executed, however, if 1 of them fails the whole operation fails
		if ( ! check_repair_filesystem( partition_new, operationdetail ) )
			succes = false ;

		//expand filesystem to fit exactly in partition
		if ( ! maximize_filesystem( partition_new, operationdetail ) )
			succes = false ;
			
		return succes ;
	}
		
	return false ;
}

bool GParted_Core::resize_move_partition( const Partition & partition_old,
				     	  const Partition & partition_new,
					  OperationDetail & operationdetail )
{
	//i'm not too happy with this, but i think it is the correct way from a i18n POV
	enum Action
	{
		NONE			= 0,
		MOVE_RIGHT	 	= 1,
		MOVE_LEFT		= 2,
		GROW 			= 3,
		SHRINK			= 4,
		MOVE_RIGHT_GROW		= 5,
		MOVE_RIGHT_SHRINK	= 6,
		MOVE_LEFT_GROW		= 7,
		MOVE_LEFT_SHRINK	= 8
	} ;
	Action action = NONE ;

	if ( partition_new .get_length() > partition_old .get_length() )
		action = GROW ;
	else if ( partition_new .get_length() < partition_old .get_length() )
		action = SHRINK ;

	if ( partition_new .sector_start > partition_old .sector_start &&
	     partition_new .sector_end > partition_old .sector_end )
		action = action == GROW ? MOVE_RIGHT_GROW : action == SHRINK ? MOVE_RIGHT_SHRINK : MOVE_RIGHT ;
	else if ( partition_new .sector_start < partition_old .sector_start &&
	     partition_new .sector_end < partition_old .sector_end )
		action = action == GROW ? MOVE_LEFT_GROW : action == SHRINK ? MOVE_LEFT_SHRINK : MOVE_LEFT ;

	Glib::ustring description ;	
	switch ( action )
	{
		case NONE		:
			description = _("resize/move partition") ;
			break ;
		case MOVE_RIGHT		:
			description = _("move partition to the right") ;
			break ;
		case MOVE_LEFT		:
			description = _("move partition to the left") ;
			break ;
		case GROW 		:
			description = _("grow partition from %1 to %2") ;
			break ;
		case SHRINK		:
			description = _("shrink partition from %1 to %2") ;
			break ;
		case MOVE_RIGHT_GROW	:
			description = _("move partition to the right and grow it from %1 to %2") ;
			break ;
		case MOVE_RIGHT_SHRINK	:
			description = _("move partition to the right and shrink it from %1 to %2") ;
			break ;
		case MOVE_LEFT_GROW	:
			description = _("move partition to the left and grow it from %1 to %2") ;
			break ;
		case MOVE_LEFT_SHRINK	:
			description = _("move partition to the left and shrink it from %1 to %2") ;
			break ;
	}

	if ( ! description .empty() && action != NONE && action != MOVE_LEFT && action != MOVE_RIGHT )
		description = String::ucompose( description,
						Utils::format_size( partition_old .get_length() ),
						Utils::format_size( partition_new .get_length() ) ) ;

	operationdetail .add_child( OperationDetail( description ) ) ;

	
	if ( action == NONE )
		operationdetail .get_last_child() .add_child( 
			OperationDetail( _("new and old partition have the same size and positition. continuing anyway"),
					  STATUS_NONE,
					  FONT_ITALIC ) ) ;

	operationdetail .get_last_child() .add_child( 
		OperationDetail(
			String::ucompose( _("old start: %1"), partition_old .sector_start ) + "\n" +
			String::ucompose( _("old end: %1"), partition_old .sector_end ) + "\n" +
			String::ucompose( _("old size: %1 (%2)"),
					  partition_old .get_length(),
					  Utils::format_size( partition_old .get_length() ) ),
		STATUS_NONE, 
		FONT_ITALIC ) ) ;
	
	//finally the actual resize/move
	bool return_value = false ;
	
	PedConstraint *constraint = NULL ;
	lp_partition = NULL ;

	//sometimes the lp_partition ->geom .start,end and length values display random numbers
	//after going out of the 'if ( lp_partition)' scope. That's why we use some variables here.
	Sector new_start = -1, new_end = -1 ;
		
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		if ( partition_old .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else		
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition_old .get_sector() ) ;
		
		if ( lp_partition )
		{
			PedGeometry *geom = ped_geometry_new( lp_device,
							      partition_new .sector_start,
							      partition_new .get_length() ) ;
			constraint = ped_constraint_exact( geom ) ;

			if ( constraint )
			{
				if ( ped_disk_set_partition_geom( lp_disk,
								  lp_partition,
								  constraint,
								  partition_new .sector_start,
								  partition_new .sector_end ) )
				{
					new_start = lp_partition ->geom .start ;
					new_end = lp_partition ->geom .end ;

					return_value = commit( partition_new .type == TYPE_EXTENDED ? 
									"" : partition_new .get_path() ) ;
				}
									
				ped_constraint_destroy( constraint );
			}
		}
		
		close_device_and_disk() ;
	}
	
	if ( return_value )
	{
		operationdetail .get_last_child() .add_child( 
			OperationDetail(
				String::ucompose( _("new start: %1"), new_start ) + "\n" +
				String::ucompose( _("new end: %1"), new_end ) + "\n" +
				String::ucompose( _("new size: %1 (%2)"),
					 	  new_end - new_start + 1,
					  	  Utils::format_size( new_end - new_start + 1 ) ),
			STATUS_NONE, 
			FONT_ITALIC ) ) ;
	}
	
	operationdetail .get_last_child() .set_status(  return_value ? STATUS_SUCCES : STATUS_ERROR ) ;
	
	return return_value ;
}

bool GParted_Core::resize_filesystem( const Partition & partition_old,
				      const Partition & partition_new,
				      OperationDetail & operationdetail,
				      bool fill_partition ) 
{
	//by default 'grow' to accomodate expand_filesystem()
	GParted::FS::Support action = get_fs( partition_old .filesystem ) .grow ;

	if ( ! fill_partition )
	{
		if ( partition_new .get_length() < partition_old .get_length() )
		{
			operationdetail .add_child( OperationDetail( _("shrink filesystem") ) ) ;
			action = get_fs( partition_old .filesystem ) .shrink ;
		}
		else if ( partition_new .get_length() > partition_old .get_length() )
			operationdetail .add_child( OperationDetail( _("grow filesystem") ) ) ;
		else
		{
			operationdetail .add_child( OperationDetail( _("resize filesystem") ) ) ;
			operationdetail .get_last_child() .add_child( 
				OperationDetail( 
					_("new and old filesystem have the same size and positition. continuing anyway"),
					STATUS_NONE,
					FONT_ITALIC ) ) ;
		}
	}

	bool succes = false ;
	switch ( action )
	{
		case GParted::FS::NONE:
			break ;
		case GParted::FS::GPARTED:
			break ;
		case GParted::FS::LIBPARTED:
			succes = resize_move_filesystem_using_libparted( partition_old,
									 partition_new,
								  	 operationdetail .get_last_child() ) ;
			break ;
		case GParted::FS::EXTERNAL:
			succes = set_proper_filesystem( partition_new .filesystem ) && 
				 p_filesystem ->resize( partition_new,
							operationdetail .get_last_child(), 
							fill_partition ) ;
			break ;
	}

	operationdetail .get_last_child() .set_status(  succes ? STATUS_SUCCES : STATUS_ERROR ) ;
	return succes ;
}
	
bool GParted_Core::maximize_filesystem( const Partition & partition, OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( _("grow filesystem to fill the partition") ) ) ;

	if ( get_fs( partition .filesystem ) .grow == GParted::FS::NONE )
	{
		operationdetail .get_last_child() .add_child( 
			OperationDetail( _("growing is not available for this filesystem"),
					  STATUS_NONE,
					  FONT_ITALIC ) ) ;

		operationdetail .get_last_child() .set_status( STATUS_N_A ) ;
		return true ;
	}
	
	return resize_filesystem( partition, partition, operationdetail, true ) ;
}

bool GParted_Core::copy( const Partition & partition_src,
			 Partition & partition_dest,
			 Sector min_size,
			 OperationDetail & operationdetail ) 
{
	if ( check_repair_filesystem( partition_src, operationdetail ) )
	{
		bool succes = true ;
		if ( partition_dest .status == GParted::STAT_COPY )
			succes = create_partition( partition_dest, operationdetail, min_size ) ;

		if ( succes && set_partition_type( partition_dest, operationdetail ) )
		{
			operationdetail .add_child( OperationDetail( 
				String::ucompose( _("copy filesystem of %1 to %2"),
						  partition_src .get_path(),
						  partition_dest .get_path() ) ) ) ;
						

			switch ( get_fs( partition_dest .filesystem ) .copy )
			{
				case  GParted::FS::GPARTED :
						succes = copy_filesystem( partition_src,
									  partition_dest,
									  operationdetail .get_last_child() ) ;
						break ;
				
				case  GParted::FS::LIBPARTED :
						//FIXME: see if copying through libparted has any advantages
						break ;

				case  GParted::FS::EXTERNAL :
						succes = set_proper_filesystem( partition_dest .filesystem ) &&
							 p_filesystem ->copy( partition_src .get_path(),
								     	      partition_dest .get_path(),
									      operationdetail .get_last_child() ) ;
						break ;

				default :
						succes = false ;
						break ;
			}

			operationdetail .get_last_child() .set_status(  succes ? STATUS_SUCCES : STATUS_ERROR ) ;

			return ( succes &&	
			     	 check_repair_filesystem( partition_dest, operationdetail ) &&
			     	 maximize_filesystem( partition_dest, operationdetail ) ) ;
		}
	}

	return false ;
}

bool GParted_Core::copy_filesystem( const Partition & partition_src,
				    const Partition & partition_dst,
				    OperationDetail & operationdetail )
{
	Sector optimal_blocksize, offset ;

	return find_optimal_blocksize( partition_src,
				       partition_dst,
				       START_TO_END,
				       optimal_blocksize,
				       offset,
				       operationdetail )
	       &&
	       copy_blocks( partition_src .device_path,
	  		    partition_dst .device_path,
	  		    partition_src .sector_start + offset,
	  		    partition_dst .sector_start + offset,
	  		    optimal_blocksize,
	  		    partition_src .get_length() - offset,
	  		    operationdetail,
			    START_TO_END ) ; 
}
	
bool GParted_Core::check_repair_filesystem( const Partition & partition, OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( 
				String::ucompose( _("check filesystem on %1 for errors and (if possible) fix them"),
						  partition .get_path() ) ) ) ;
	
	bool succes = false ;
	switch ( get_fs( partition .filesystem ) .check )
	{
		case GParted::FS::NONE:
			operationdetail .get_last_child() .add_child(
				OperationDetail( _("checking is not available for this filesystem"),
						 STATUS_NONE,
						 FONT_ITALIC ) ) ;

			operationdetail .get_last_child() .set_status( STATUS_N_A ) ;
			return true ;	

			break ;
		case GParted::FS::GPARTED:
			break ;
		case GParted::FS::LIBPARTED:
			break ;
		case GParted::FS::EXTERNAL:
			succes = set_proper_filesystem( partition .filesystem ) &&
				 p_filesystem ->check_repair( partition, operationdetail .get_last_child() ) ;

			break ;
	}

	operationdetail .get_last_child() .set_status(  succes ? STATUS_SUCCES : STATUS_ERROR ) ;
	return succes ;
}

bool GParted_Core::set_partition_type( const Partition & partition, OperationDetail & operationdetail )
{
	operationdetail .add_child( OperationDetail(
				String::ucompose( _("set partitiontype on %1"), partition .get_path() ) ) ) ;
	
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
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition .get_sector() ) ;

			if ( lp_partition &&
			     ped_partition_set_system( lp_partition, fs_type ) && 
			     commit( partition .get_path() ) )
			{
				operationdetail .get_last_child() .add_child( 
					OperationDetail( String::ucompose( _("new partitiontype: %1"),
									   lp_partition ->fs_type ->name ),
							 STATUS_NONE,
							 FONT_ITALIC ) ) ;

				return_value = true ;
			}
		}

		close_device_and_disk() ;
	}

	operationdetail .get_last_child() .set_status(  return_value ? STATUS_SUCCES : STATUS_ERROR ) ;
	return return_value ;
}
	
void GParted_Core::set_progress_info( Sector total,
				      Sector done,
				      std::time_t time_start,
				      OperationDetail & operationdetail ) 
{
	operationdetail .fraction = done / static_cast<double>( total ) ;

	double sec_per_frac = (std::time( NULL ) - time_start) / static_cast<double>( operationdetail .fraction ) ;

	std::time_t time_remaining = Utils::round( (1.0 - operationdetail .fraction) * sec_per_frac ) ;

	//format it a bit..
	Glib::ustring time ;
	
	int unit = static_cast<int>( time_remaining / 3600 ) ;
	if ( unit < 10 )
		time += "0" ;
	time += Utils::num_to_str( unit ) + ":" ;
	time_remaining %= 3600 ;

	unit = static_cast<int>( time_remaining / 60 ) ;
	if ( unit < 10 )
		time += "0" ;
	time += Utils::num_to_str( unit ) + ":" ;
	time_remaining %= 60 ;

	if ( time_remaining < 10 )
		time += "0" ;
	time += Utils::num_to_str( time_remaining ) ;

	operationdetail .progress_text = String::ucompose( _("%1 of %2 copied (%3 remaining)"),
							   Utils::format_size( done ),
							   Utils::format_size( total ),
							   time ) ; 
			
	operationdetail .set_description( String::ucompose( _("%1 of %2 copied"), done, total ), FONT_ITALIC ) ;
}
	
bool GParted_Core::find_optimal_blocksize( const Partition & partition_old,
		      		     	   const Partition & partition_new,
					   CopyType copytype,
					   Sector & optimal_blocksize,
					   Sector & offset,
					   OperationDetail & operationdetail ) 
{//FIXME, this probing is actually quite clumsy and suboptimal..
//find out if there is a better way to determine a (close to) optimal blocksize...
	bool succes = true ;

	optimal_blocksize = 32 ;//sensible default?
	offset = 0 ;

	if ( partition_old .get_length() > 100000 )
	{
		operationdetail .add_child( OperationDetail( _("finding optimal blocksize"), STATUS_NONE ) ) ;
		
		std::clock_t clockticks_start, smallest_ticks = -1  ;

		std::vector<Sector> blocksizes ;
		blocksizes .push_back( 1 ) ;
		blocksizes .push_back( 32 ) ;
		blocksizes .push_back( 128 ) ;
		blocksizes .push_back( 256 ) ;
		blocksizes .push_back( 512 ) ;
		
		for ( unsigned int t = 0 ; t < blocksizes .size() && succes ; t++ )
		{
			clockticks_start = std::clock() ;

			succes = copy_blocks( partition_old .device_path,
					      partition_new .device_path,
					      partition_old .sector_start + offset,
					      partition_new .sector_start + offset,
					      blocksizes[ t ],
					      20000,
					      operationdetail .get_last_child(),
					      copytype ) ;

			if ( (std::clock() - clockticks_start) < smallest_ticks || smallest_ticks == -1 )
			{
				smallest_ticks = std::clock() - clockticks_start ;
				optimal_blocksize = blocksizes[ t ] ;
			}

			operationdetail .get_last_child() .get_last_child() .add_child( OperationDetail( 
				String::ucompose( _("%1 clockticks"), std::clock() - clockticks_start ),
				STATUS_NONE,
				FONT_ITALIC ) ) ;

			offset += 20000 ;
		}
		
		operationdetail .get_last_child() .add_child( OperationDetail( 
			String::ucompose( _("optimal blocksize is %1 sectors (%2)"),
					  optimal_blocksize,
					  Utils::format_size( optimal_blocksize ) ) ,
			STATUS_NONE ) ) ;
	}
	
	return succes ; 
}

bool GParted_Core::copy_blocks( const Glib::ustring & src_device,
		  	        const Glib::ustring & dst_device,
		  	        Sector src_start,
		  	        Sector dst_start,
		  	        Sector blocksize,
		  	        Sector sectors,
		  	        OperationDetail & operationdetail,
		  	        CopyType copytype )
{
	operationdetail .add_child( OperationDetail( 
		String::ucompose( _("copy %1 sectors using a blocksize of %2 sectors"), sectors, blocksize ) ) ) ;
	
	bool succes = false ;
	PedDevice *lp_device_src, *lp_device_dst ;

	lp_device_src = ped_device_get( src_device .c_str() );

	if ( src_device != dst_device )
		lp_device_dst = ped_device_get( dst_device .c_str() );
	else
		lp_device_dst = lp_device_src ;
	Glib::ustring error_message ;
	if ( lp_device_src && lp_device_dst && ped_device_open( lp_device_src ) && ped_device_open( lp_device_dst ) )
	{
		ped_device_sync( lp_device_dst ) ;
		//add an empty sub which we will constantly update in the loop
		operationdetail .get_last_child() .add_child( OperationDetail( "", STATUS_NONE ) ) ;
	
		std::time_t time_start, time_last_progress_update ;
		time_start = time_last_progress_update = std::time(NULL) ;
		Sector t ;
		if ( copytype == START_TO_END )
		{
			Sector rest_sectors = sectors % blocksize ;
			for ( t = 0 ; t < sectors - rest_sectors ; t+=blocksize )
			{
				if ( ! copy_block( lp_device_src,
						   lp_device_dst,
						   src_start +t,
						   dst_start +t,
						   blocksize,
						   error_message ) ) 
					break ;

				if ( (std::time(NULL) - time_last_progress_update) >= 1 )
				{
					set_progress_info( sectors,
							   t + blocksize,
							   time_start,
							   operationdetail .get_last_child() .get_last_child() ) ;

					time_last_progress_update = std::time(NULL) ;
				}
			}

			if ( rest_sectors > 0 &&
			     sectors -t == rest_sectors &&
			     copy_block( lp_device_src,
					 lp_device_dst,
					 src_start +t,
					 dst_start +t,
					 rest_sectors,
					 error_message ) )
				t += rest_sectors ;
		}
		else if ( copytype == END_TO_START )
		{
			Sector rest_sectors = sectors % blocksize ;
			for ( t = 0 ; t < sectors - rest_sectors ; t+=blocksize )
			{
				if ( ! copy_block( lp_device_src,
						   lp_device_dst,
						   src_start + sectors -blocksize -t,
						   dst_start + sectors -blocksize -t,
						   blocksize,
						   error_message ) )
					break ;

				if ( (std::time(NULL) - time_last_progress_update) >= 1 )
				{
					set_progress_info( sectors,
							   t + blocksize,
							   time_start,
							   operationdetail .get_last_child() .get_last_child() ) ;

					time_last_progress_update = std::time(NULL) ;
				}
			}

			if ( rest_sectors > 0 &&
			     sectors -t == rest_sectors &&
			     copy_block( lp_device_src,
					 lp_device_dst,
					 src_start,
					 dst_start,
					 rest_sectors,
					 error_message ) )
				t += rest_sectors ;
		}
	

		//final description
		operationdetail .get_last_child() .get_last_child() .set_description( 
			String::ucompose( _("%1 of %2 copied"), t, sectors ), FONT_ITALIC ) ;
			
		//reset fraction to -1 to make room for a new one (or a pulsebar)
		operationdetail .get_last_child() .get_last_child() .fraction = -1 ;

		if ( t == sectors )
			succes = true ;
	
		if ( ! succes && ! error_message .empty() )
			operationdetail .get_last_child() .add_child( 
				OperationDetail( error_message, STATUS_NONE, FONT_ITALIC ) ) ;

		//close and destroy the devices..
		ped_device_close( lp_device_src ) ;
		ped_device_destroy( lp_device_src ) ; 

		if ( src_device != dst_device )
		{
			ped_device_close( lp_device_dst ) ;
			ped_device_destroy( lp_device_dst ) ;
		}
	}

	operationdetail .get_last_child() .set_status( succes ? STATUS_SUCCES : STATUS_ERROR ) ;

	return succes ;
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
		error_message = String::ucompose( _("Error while reading block at sector %1"), offset_src ) ;

		return false ;
	}
				
	if ( ! ped_device_write( lp_device_dst, buf, offset_dst, blocksize ) )
	{
		error_message = String::ucompose( _("Error while writing block at sector %1"), offset_dst ) ;

		return false ;
	}

	return true ;
}

bool GParted_Core::calculate_exact_geom( const Partition & partition_old,
			       	         Partition & partition_new,
				         OperationDetail & operationdetail,
				         Sector min_size ) 
{
	operationdetail .add_child( OperationDetail( 
		String::ucompose( _("calculate new size and position of %1"), partition_new .get_path() ) ) ) ;

	if ( min_size >= 0 )
		operationdetail .get_last_child() .add_child( 
			OperationDetail( String::ucompose( _("minimum size: %1"), min_size ), STATUS_NONE ) ) ;
	
	operationdetail .get_last_child() .add_child( 
		OperationDetail(
			String::ucompose( _("requested start: %1"), partition_new .sector_start ) + "\n" +
			String::ucompose( _("requested end: %1"), partition_new .sector_end ) + "\n" +
			String::ucompose( _("requested size: %1 (%2)"),
					  partition_new .get_length(),
					  Utils::format_size( partition_new .get_length() ) ),
		STATUS_NONE,
		FONT_ITALIC ) ) ;
	
	bool succes = false ;
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		lp_partition = NULL ;

		if ( partition_old .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else		
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition_old .get_sector() ) ;

		if ( lp_partition )
		{
			PedConstraint *constraint = NULL ;
			constraint = ped_constraint_any( lp_device ) ;
			
			if ( constraint )
			{
				if ( min_size >= 0 )
					constraint ->min_size = min_size ;

				//FIXME: if we insert a weird partitionnew geom here (e.g. start > end) 
				//ped_disk_set_partition_geom() will still return true (althoug an lp exception is written
				//to stdout.. see if this also affect create_partition and resize_move_partition
				//sended a patch to fix this to libparted list. will probably be in 1.7.2
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

		close_device_and_disk() ;	
	}

	if ( succes ) 
	{
		operationdetail .get_last_child() .add_child( 
			OperationDetail(
				String::ucompose( _("new start: %1"), partition_new .sector_start ) + "\n" +
				String::ucompose( _("new end: %1"), partition_new .sector_end ) + "\n" +
				String::ucompose( _("new size: %1 (%2)"),
					 	  partition_new .get_length(),
					  	  Utils::format_size( partition_new .get_length() ) ),
			STATUS_NONE,
			FONT_ITALIC ) ) ;
	}

	operationdetail .get_last_child() .set_status(  succes ? STATUS_SUCCES : STATUS_ERROR ) ;
	return succes ;
}

bool GParted_Core::set_proper_filesystem( const FILESYSTEM & filesystem )
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

	return p_filesystem ;
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
		lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition .get_sector() ) ;

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

bool GParted_Core::commit( const Glib::ustring & node ) 
{
	bool return_value = ped_disk_commit_to_dev( lp_disk ) ;
	
	ped_disk_commit_to_os( lp_disk ) ;

	if ( ! node .empty() && return_value )
		return_value = wait_for_node( node ) ;

	return return_value ;
}
	
PedExceptionOption GParted_Core::ped_exception_handler( PedException * e ) 
{
        std::cout << e ->message << std::endl ;

        libparted_messages .push_back( e->message ) ;

	return PED_EXCEPTION_UNHANDLED ;
}

GParted_Core::~GParted_Core() 
{
	if ( p_filesystem )
		delete p_filesystem ;

	//remove .fdi file..
	remove( "/usr/share/hal/fdi/policy/gparted-disable-automount.fdi" ) ;
}
	
} //GParted
