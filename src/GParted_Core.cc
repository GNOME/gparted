#include "../include/GParted_Core.h"

#include <sys/statvfs.h>	

namespace GParted
{
	
GParted_Core::GParted_Core( ) 
{
	lp_device = NULL ;
	lp_disk = NULL ;
	lp_partition = NULL ;
	p_filesystem = NULL ;

	//get valid flags ...
	for ( PedPartitionFlag flag = ped_partition_flag_next( (PedPartitionFlag) NULL ) ; flag ; flag = ped_partition_flag_next( flag ) )
		flags .push_back( flag ) ;	
	
	//throw libpartedversion to the stdout to see which version is actually used.
	std::cout << "======================" << std::endl ;
	std::cout << "libparted : " << ped_get_version( ) << std::endl ;
	std::cout << "======================" << std::endl ;
	
	//initialize filesystemlist
	find_supported_filesystems( ) ;
}

void GParted_Core::find_supported_filesystems( )
{
	FILESYSTEMS .clear( ) ;
	
	ext2 fs_ext2;
	FILESYSTEMS .push_back( fs_ext2 .get_filesystem_support( ) ) ;
	
	ext3 fs_ext3;
	FILESYSTEMS .push_back( fs_ext3 .get_filesystem_support( ) ) ;
	
	fat16 fs_fat16;
	FILESYSTEMS .push_back( fs_fat16 .get_filesystem_support( ) ) ;
	
	fat32 fs_fat32;
	FILESYSTEMS .push_back( fs_fat32 .get_filesystem_support( ) ) ;
	
	hfs fs_hfs;
	FILESYSTEMS .push_back( fs_hfs .get_filesystem_support( ) ) ;
	
	hfsplus fs_hfsplus;
	FILESYSTEMS .push_back( fs_hfsplus .get_filesystem_support( ) ) ;
	
	jfs fs_jfs;
	FILESYSTEMS .push_back( fs_jfs .get_filesystem_support( ) ) ;
	
	linux_swap fs_linux_swap;
	FILESYSTEMS .push_back( fs_linux_swap .get_filesystem_support( ) ) ;
	
	ntfs fs_ntfs;
	FILESYSTEMS .push_back( fs_ntfs .get_filesystem_support( ) ) ;
	
	reiser4 fs_reiser4;
	FILESYSTEMS .push_back( fs_reiser4 .get_filesystem_support( ) ) ;
	
	reiserfs fs_reiserfs;
	FILESYSTEMS .push_back( fs_reiserfs .get_filesystem_support( ) ) ;
	
	xfs fs_xfs;
	FILESYSTEMS .push_back( fs_xfs .get_filesystem_support( ) ) ;
	
	//unknown filesystem (default when no match is found)
	FS fs ; fs .filesystem = GParted::FS_UNKNOWN ;
	FILESYSTEMS .push_back( fs ) ;
}
	
void GParted_Core::set_user_devices( const std::vector<Glib::ustring> & user_devices ) 
{
	this ->device_paths = user_devices ;
	this ->probe_devices = ! user_devices .size() ;
}
	
bool GParted_Core::check_device_path( const Glib::ustring & device_path ) 
{
	if ( device_path .length() > 6 && device_path .is_ascii() && open_device( device_path ) )
	{
		close_device_and_disk() ;
		return true ;
	}

	return false ;
}

void GParted_Core::get_devices( std::vector<Device> & devices )
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
	}

	
	for ( unsigned int t = 0 ; t < device_paths .size() ; t++ ) 
	{ 
		if ( check_device_path( device_paths[ t ] ) && open_device_and_disk( device_paths[ t ], false ) )
		{
			temp_device .Reset() ;
			
			//device info..
			temp_device .path 	=	get_short_path( device_paths[ t ] ) ;
			temp_device .realpath	= 	lp_device ->path ; 
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
				set_short_paths( temp_device .partitions ) ;

				if ( temp_device .highest_busy )
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
				partition_temp .Set_Unallocated( temp_device .path, 0, temp_device .length, false );
				temp_device .partitions .push_back( partition_temp );
			}
					
			devices .push_back( temp_device ) ;
			
			close_device_and_disk() ;
		}
	}

	//clear leftover information...	
	//NOTE that we cannot clear mountinfo since it might be needed in get_all_mountpoints()
	short_paths .clear() ;
}

void GParted_Core::init_maps() 
{
	std::string line ;

	short_paths .clear() ;
	mount_info .clear() ;
	
	//initialize mountpoints..
	char node[255], mountpoint[255] ;
	unsigned int index ;
	std::ifstream proc_mounts( "/proc/mounts" ) ;
	if ( proc_mounts )
	{
		while ( getline( proc_mounts, line ) )
			if ( Glib::str_has_prefix( line, "/" ) &&
			     sscanf( line .c_str(), "%255s %255s", node, mountpoint ) == 2 &&
			     static_cast<Glib::ustring>( node ) != "/dev/root" )
			{
				//see if mountpoint contains spaces and deal with it
				line = mountpoint ;
				index = line .find( "\\040" ) ;
				if ( index < line .length() )
					line .replace( index, 4, " " ) ;
				
				mount_info[ node ] .push_back( line ) ;
			}
		
		proc_mounts .close() ;
	}
	
	//above list lacks the root mountpoint, try to get it from /etc/mtab
	std::ifstream etc_mtab( "/etc/mtab" ) ;
	if ( etc_mtab )
	{
		while ( getline( etc_mtab, line ) )
			if ( Glib::str_has_prefix( line, "/" ) &&
			     sscanf( line .c_str(), "%255s %255s", node, mountpoint ) == 2 &&
			     static_cast<Glib::ustring>( mountpoint ) == "/" )
			{
				mount_info[ node ] .push_back( "/" ) ;
				break ;
			}
			
		etc_mtab .close() ;
	}

	//initialize shortpaths...
	std::ifstream proc_partitions( "/proc/partitions" ) ;
	if ( proc_partitions )
	{
		char c_str[255] ;
		
		while ( getline( proc_partitions, line ) )
			if ( sscanf( line .c_str(), "%*d %*d %*d %255s", c_str ) == 1 )
			{
				line = "/dev/" ; 
				line += c_str ;

				if ( realpath( line .c_str(), c_str ) )
					short_paths[ c_str ] = line ;
			}

		proc_partitions .close() ;
	}
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
	partition_temp .error += _( "The filesystem is unknown to libparted" ) ;
	partition_temp .error += "\n-"; 
	partition_temp .error += _( "There is no filesystem available (unformatted)" ) ; 

	return GParted::FS_UNKNOWN ;
}

void GParted_Core::set_device_partitions( Device & device ) 
{
	int EXT_INDEX = -1 ;
	
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
				partition_temp .Set( device .path,
						     ped_partition_get_path( lp_partition ),
						     lp_partition ->num,
						     lp_partition ->type == 0 ? GParted::TYPE_PRIMARY : GParted::TYPE_LOGICAL,
						     get_filesystem(),
						     lp_partition ->geom .start,
						     lp_partition ->geom .end,
						     lp_partition ->type,
						     ped_partition_is_busy( lp_partition ) );
						
				set_flags( partition_temp ) ;
				
				if ( partition_temp .busy && partition_temp .partition_number > device .highest_busy )
					device .highest_busy = partition_temp .partition_number ;
									
				break ;
		
			case PED_PARTITION_EXTENDED:
				partition_temp.Set( device .path,
						    ped_partition_get_path( lp_partition ), 
						    lp_partition ->num,
						    GParted::TYPE_EXTENDED,
						    GParted::FS_EXTENDED,
						    lp_partition ->geom .start,
						    lp_partition ->geom .end,
						    false,
						    ped_partition_is_busy( lp_partition ) );
				
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
		insert_unallocated( device .path,
				    device .partitions[ EXT_INDEX ] .logicals,
				    device .partitions[ EXT_INDEX ] .sector_start,
				    device .partitions[ EXT_INDEX ] .sector_end,
				    true ) ;
	
	insert_unallocated( device .path, device .partitions, 0, device .length -1, false ) ; 
}

void GParted_Core::set_mountpoints( std::vector<Partition> & partitions ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		if ( partitions[ t ] .busy && partitions[ t ] .filesystem != GParted::FS_LINUX_SWAP )
		{
			if ( partitions[ t ] .type == GParted::TYPE_PRIMARY || partitions[ t ] .type == GParted::TYPE_LOGICAL ) 
			{
				iter_mp = mount_info .find( partitions[ t ] .partition );
				if ( iter_mp != mount_info .end() )
					partitions[ t ] .mountpoints = iter_mp ->second ;
			}
			else if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
				set_mountpoints( partitions[ t ] .logicals ) ;
		}
	}
}

void GParted_Core::set_short_paths( std::vector<Partition> & partitions ) 
{
	for ( unsigned int t =0 ; t < partitions .size() ; t++ )
	{
		partitions[ t ] .partition = get_short_path( partitions[ t ] .partition ) ;

		if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			set_short_paths( partitions[ t ] .logicals ) ;
	}
}

Glib::ustring GParted_Core::get_short_path( const Glib::ustring & real_path ) 
{
	iter = short_paths .find( real_path );
	if ( iter != short_paths .end() )
		return iter ->second ;

	return real_path ;
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
		if ( partitions[ t ] .filesystem != GParted::FS_LINUX_SWAP && partitions[ t ] .filesystem != GParted::FS_UNKNOWN )
		{
			if ( partitions[ t ] .type == GParted::TYPE_PRIMARY || partitions[ t ] .type == GParted::TYPE_LOGICAL ) 
			{
				if ( partitions[ t ] .busy )
				{
					if ( statvfs( partitions[ t ] .mountpoints .back() .c_str(), &sfs ) == 0 )
						partitions[ t ] .Set_Unused( sfs .f_bfree * (sfs .f_bsize / 512) ) ;
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
							LP_Set_Used_Sectors( partitions[ t ] ) ;
							break ;

						default:
							break ;
					}
				}

				if ( partitions[ t ] .sectors_used == -1 )
					partitions[ t ] .error = temp ;
				
			}
			else if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
				set_used_sectors( partitions[ t ] .logicals ) ;
		}
	}
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
	if ( (partitions .front( ) .sector_start - start) >= MEBIBYTE )
	{
		partition_temp .sector_start = start ;
		partition_temp .sector_end = partitions .front( ) .sector_start -1 ;
		
		partitions .insert( partitions .begin( ), partition_temp );
	}
	
	//look for gaps in between
	for ( unsigned int t =0 ; t < partitions .size( ) -1 ; t++ )
		if ( ( partitions[ t +1 ] .sector_start - partitions[ t ] .sector_end ) >= MEBIBYTE )
		{
			partition_temp .sector_start = partitions[ t ] .sector_end +1 ;
			partition_temp .sector_end = partitions[ t +1 ] .sector_start -1 ;
		
			partitions .insert( partitions .begin( ) + ++t, partition_temp );
		}
		
	//last partition end <---> end
	if ( (end - partitions .back( ) .sector_end ) >= MEBIBYTE )
	{
		partition_temp .sector_start = partitions .back( ) .sector_end +1 ;
		partition_temp .sector_end = end ;
		
		partitions .push_back( partition_temp );
	}
}

bool GParted_Core::apply_operation_to_disk( Operation & operation )
{
	switch ( operation .operationtype )
	{
		case DELETE:
			return Delete( operation .partition_original, operation .operation_details .sub_details ) ;
		case CREATE:
			return create( operation .device, 
				       operation .partition_new,
				       operation .operation_details .sub_details ) ;
		case RESIZE_MOVE:
			return resize( operation .device,
				       operation .partition_original,
				       operation .partition_new,
				       operation .operation_details .sub_details ) ;
		case FORMAT:
			return format( operation .partition_new, operation .operation_details .sub_details ) ;
		case COPY: 
			return copy( operation .copied_partition_path,
				     operation .partition_new,
				     operation .operation_details .sub_details ) ; 
	}
	
	return false ;
}

bool GParted_Core::create( const Device & device,
			   Partition & new_partition,
			   std::vector<OperationDetails> & operation_details ) 
{
	
	if ( new_partition .type == GParted::TYPE_EXTENDED )   
	{
		return create_empty_partition( new_partition, operation_details ) ;
	}
	else if ( create_empty_partition(
		  	new_partition,
			operation_details,
			( new_partition .get_length() - device .cylsize ) < get_fs( new_partition .filesystem ) .MIN * MEBIBYTE ) > 0 )
	{
		set_proper_filesystem( new_partition .filesystem ) ;
		
		//most likely this means the user created an unformatted partition,
		//however in theory, it could also screw some errorhandling.
		if ( ! p_filesystem )
			return true ;
		
		return set_partition_type( new_partition, operation_details ) &&
		       p_filesystem ->Create( new_partition, operation_details ) ;
	}
	
	return false ;
}

bool GParted_Core::format( const Partition & partition, std::vector<OperationDetails> & operation_details )
{	
	//remove all filesystem signatures...
	erase_filesystem_signatures( partition ) ;
	
	set_proper_filesystem( partition .filesystem ) ;
	
	return set_partition_type( partition, operation_details ) &&
				   p_filesystem ->Create( partition, operation_details ) ;
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

bool GParted_Core::resize( const Device & device,
			   const Partition & partition_old, 
			   const Partition & partition_new,
			   std::vector<OperationDetails> & operation_details ) 
{
	//extended partition
	if ( partition_old .type == GParted::TYPE_EXTENDED )
		return resize_container_partition( partition_old, partition_new, false, operation_details ) ;
	
	bool succes = false ;
	set_proper_filesystem( partition_new .filesystem ) ;
	
	//resize using libparted..
	if ( get_fs( partition_old .filesystem ) .grow == GParted::FS::LIBPARTED )
	{
		if ( p_filesystem && p_filesystem ->Check_Repair( partition_new, operation_details ) )
		{
			succes = resize_normal_using_libparted( partition_old, partition_new, operation_details ) ;

			//always check after a resize, but if it failes the whole operation failes 
			if ( ! p_filesystem ->Check_Repair( partition_new, operation_details ) )
				succes = false ;
		}
		
		return succes ;
	}

	//use custom resize tools..
	if ( p_filesystem && p_filesystem ->Check_Repair( partition_new, operation_details ) )
	{
		succes = true ;
			
		if ( partition_new .get_length() < partition_old .get_length() )
		{
			p_filesystem ->cylinder_size = device .cylsize ;
			succes = p_filesystem ->Resize( partition_new, operation_details ) ;
		}
						
		if ( succes )
			succes = resize_container_partition(
					partition_old,
			     		partition_new,
					! get_fs( partition_new .filesystem ) .move,
					operation_details ) ;
			
		//these 3 are always executed, however, if 1 of them fails the whole operation fails
		if ( ! p_filesystem ->Check_Repair( partition_new, operation_details ) )
			succes = false ;

		//expand filesystem to fit exactly in partition
		if ( ! p_filesystem ->Resize( partition_new, operation_details, true ) )
			succes = false ;
			
		if ( ! p_filesystem ->Check_Repair( partition_new, operation_details ) )
			succes = false ;

		return succes ;
	}
		
	return false ;
}

bool GParted_Core::copy( const Glib::ustring & src_part_path,
			 Partition & partition_dest,
			 std::vector<OperationDetails> & operation_details ) 
{
	set_proper_filesystem( partition_dest .filesystem ) ;
	
	Partition src_partition ;
	src_partition .partition = src_part_path ;
	
	return ( p_filesystem &&
		 p_filesystem ->Check_Repair( src_partition, operation_details ) &&
	     	 create_empty_partition( partition_dest, operation_details, true ) > 0 &&
	     	 set_partition_type( partition_dest, operation_details ) &&
	     	 p_filesystem ->Copy( src_part_path, partition_dest .partition, operation_details ) &&
	     	 p_filesystem ->Check_Repair( partition_dest, operation_details ) ) ;
}

bool GParted_Core::Set_Disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel ) 
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

const std::vector<FS> & GParted_Core::get_filesystems() const 
{
	return FILESYSTEMS ;
}

const FS & GParted_Core::get_fs( GParted::FILESYSTEM filesystem ) const 
{
	for ( unsigned int t = 0 ; t < FILESYSTEMS .size( ) ; t++ )
		if ( FILESYSTEMS[ t ] .filesystem == filesystem )
			return FILESYSTEMS[ t ] ;
	
	return FILESYSTEMS .back( ) ;
}

std::vector<Glib::ustring> GParted_Core::get_disklabeltypes( ) 
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

void GParted_Core::LP_Set_Used_Sectors( Partition & partition )
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
					partition .Set_Unused( 
						(partition .sector_end - partition .sector_start) - constraint ->min_size ) ;
					
					ped_constraint_destroy( constraint );
				}
												
				ped_file_system_close( fs ) ;
			}
		}
	}
}

int GParted_Core::create_empty_partition( Partition & new_partition,
					  std::vector<OperationDetails> & operation_details,
					  bool copy )
{
	operation_details .push_back( OperationDetails( _("create empty partition") ) ) ;
	
	new_partition .partition_number = 0 ;
		
	if ( open_device_and_disk( new_partition .device_path ) )
	{
		PedPartitionType type;
		lp_partition = NULL ;
		PedConstraint *constraint = NULL ;
		
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
		
		lp_partition = ped_partition_new( lp_disk,
						  type,
						  NULL,
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
				if ( copy )
					constraint ->min_size = new_partition .get_length() ;
				
				if ( ped_disk_add_partition( lp_disk, lp_partition, constraint ) && commit() )
				{
					new_partition .partition = ped_partition_get_path( lp_partition ) ;
					new_partition .partition_number = lp_partition ->num ;

					Sector start = lp_partition ->geom .start ;
					Sector end = lp_partition ->geom .end ;
					
					operation_details .back() .sub_details .push_back( 
						OperationDetails( 
							"<i>" +
							String::ucompose( _("path: %1"), new_partition .partition ) + "\n" +
							String::ucompose( _("start: %1"), start ) + "\n" +
							String::ucompose( _("end: %1"), end ) + "\n" +
							String::ucompose( _("size: %1"), Utils::format_size( end - start + 1 ) ) + 
							"</i>",
							OperationDetails::NONE ) ) ;
				}
			
				ped_constraint_destroy( constraint );
			}
			
		}
				
		close_device_and_disk() ;
	}

	if ( new_partition .type == GParted::TYPE_EXTENDED ||
	     (
	     	new_partition .partition_number > 0 &&
	     	wait_for_node( new_partition .partition ) &&
	     	erase_filesystem_signatures( new_partition )
	     )
	   )
	{
		operation_details .back() .status = new_partition .partition_number > 0 ? 
			OperationDetails::SUCCES : OperationDetails::ERROR ;
		
		return new_partition .partition_number ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;	
		return 0 ;
	}
}

bool GParted_Core::resize_container_partition( const Partition & partition_old,
					       const Partition & partition_new,
					       bool fixed_start,
					       std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("resize partition") ) ) ;

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
		//use start/end vars since lp_partition ->geom loses his values after a functioncall :/
		//this is actually quite weird, but i don't have time to investigate it more thorough.
		Sector start = lp_partition ->geom .start ;
		Sector end = lp_partition ->geom .end ;
		
		operation_details .back() .sub_details .push_back( 
			OperationDetails(
				"<i>" +
				String::ucompose( _("new start: %1"), start ) + "\n" +
				String::ucompose( _("new end: %1"), end ) + "\n" +
				String::ucompose( _("new size: %1"), Utils::format_size( end - start + 1 ) ) + 
				"</i>",
			OperationDetails::NONE ) ) ;
	}
	
	if ( partition_old .type == GParted::TYPE_EXTENDED )
	{
		operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
		return return_value ;
	}
	else
	{
		return_value &= wait_for_node( partition_new .partition ) ;
		operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;

		return return_value ;
	}
}

bool GParted_Core::resize_normal_using_libparted( const Partition & partition_old,
						  const Partition & partition_new,
						  std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( _("resize partition and filesystem using libparted") ) ) ;
	
	bool return_value = false ;
	
	PedFileSystem *fs = NULL ;
	PedConstraint *constraint = NULL ;
	lp_partition = NULL ;
	
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
							return_value = commit() ;
										
					ped_constraint_destroy( constraint );
				}
				
				ped_file_system_close( fs );
			}
		}
		
		close_device_and_disk() ;
	}
	
	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return return_value ;
}

void GParted_Core::set_flags( Partition & partition )
{
	for ( unsigned int t = 0 ; t < flags .size() ; t++ )
		if ( ped_partition_get_flag( lp_partition, flags[ t ] ) )
			partition .flags .push_back( ped_partition_flag_get_name( flags[ t ] ) ) ;
}

void GParted_Core::set_proper_filesystem( const FILESYSTEM & filesystem )
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

		default			: p_filesystem = NULL ;
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
			ped_file_system_type_get( Utils::Get_Filesystem_String( partition .filesystem ) .c_str() ) ;

		//default is Linux (83)
		if ( ! fs_type )
			fs_type = ped_file_system_type_get( "ext2" ) ;

		if ( fs_type )
		{
			lp_partition = ped_disk_get_partition_by_sector( 
						lp_disk,
						(partition .sector_end + partition .sector_start) / 2 ) ;

			if ( lp_partition && ped_partition_set_system( lp_partition, fs_type ) && commit() )
				return_value = wait_for_node( partition .partition ) ;
		}

		close_device_and_disk() ;
	}

	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return return_value ;
}
	
bool GParted_Core::wait_for_node( const Glib::ustring & node ) 
{
	//we'll loop for 10 seconds or till 'node' appeares...
	for( short t = 0 ; t < 50 ; t++ )
	{
		if ( Glib::file_test( node, Glib::FILE_TEST_EXISTS ) )
		{ 
			sleep( 1 ) ; //apperantly the node isn't available immediatly after file_test returns succesfully :/
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

bool GParted_Core::open_device( const Glib::ustring & device_path )
{
	lp_device = ped_device_get( device_path .c_str() );
	
	return lp_device ;
}
	
bool GParted_Core::open_device_and_disk( const Glib::ustring & device_path, bool strict ) 
{
	if ( open_device( device_path ) )
		lp_disk = ped_disk_new( lp_device );
	
	//if ! disk and writeable it's probably a HD without disklabel.
	//We return true here and deal with them in GParted_Core::get_devices
	if ( ! lp_disk && ( strict || lp_device ->read_only ) )
	{
		ped_device_destroy( lp_device ) ;
		lp_device = NULL ; 
		
		return false;
	}	
		
	return true ;
}

void GParted_Core::close_device_and_disk()
{
	if ( lp_device )
		ped_device_destroy( lp_device ) ;
		
	if ( lp_disk )
		ped_disk_destroy( lp_disk ) ;
	
	lp_device = NULL ;
	lp_disk = NULL ;
}	

bool GParted_Core::commit() 
{
	bool return_value = ped_disk_commit_to_dev( lp_disk ) ;
	
	ped_disk_commit_to_os( lp_disk ) ;
		
	return return_value ;
}


} //GParted
