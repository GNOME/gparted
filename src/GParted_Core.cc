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
	textbuffer = Gtk::TextBuffer::create( ) ;

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
	
	reiser4 fs_reiser4;
	FILESYSTEMS .push_back( fs_reiser4 .get_filesystem_support( ) ) ;
	
	reiserfs fs_reiserfs;
	FILESYSTEMS .push_back( fs_reiserfs .get_filesystem_support( ) ) ;
	
	ntfs fs_ntfs;
	FILESYSTEMS .push_back( fs_ntfs .get_filesystem_support( ) ) ;
	
	xfs fs_xfs;
	FILESYSTEMS .push_back( fs_xfs .get_filesystem_support( ) ) ;
	
	//unknown filesystem (default when no match is found)
	FS fs ; fs .filesystem = GParted::FS_UNKNOWN ;
	FILESYSTEMS .push_back( fs ) ;
}

void GParted_Core::get_devices( std::vector<Device> & devices )
{
	devices .clear( ) ;
		
	//try to find all available devices and put these in a list
	ped_device_probe_all( );
	
	Device temp_device ;
	std::vector <Glib::ustring> device_paths ;
	
	lp_device = ped_device_get_next( NULL );
	
	//in certain cases (e.g. when there's a cd in the cdrom-drive) ped_device_probe_all will find a 'ghost' device that has no name or contains
	//random garbage. Those 2 checks try to prevent such a ghostdevice from being initialized.. (tested over a 1000 times with and without cd)
	while ( lp_device && strlen( lp_device ->path ) > 6 && static_cast<Glib::ustring>( lp_device ->path ) .is_ascii( ) )
	{
		if ( open_device( lp_device ->path ) )
			device_paths .push_back( get_sym_path( lp_device ->path ) ) ;
					
		lp_device = ped_device_get_next( lp_device ) ;
	}
	close_device_and_disk( ) ;
	
	for ( unsigned int t = 0 ; t < device_paths .size( ) ; t++ ) 
	{ 
		if ( open_device_and_disk( device_paths[ t ], false ) )
		{
			temp_device .Reset( ) ;
			
			//device info..
			temp_device .path 	=	device_paths[ t ] ;
			temp_device .realpath	= 	lp_device ->path ; 
			temp_device .model 	=	lp_device ->model ;
			temp_device .heads 	=	lp_device ->bios_geom .heads ;
			temp_device .sectors 	=	lp_device ->bios_geom .sectors ;
			temp_device .cylinders	=	lp_device ->bios_geom .cylinders ;
			temp_device .length 	=	temp_device .heads * temp_device .sectors * temp_device .cylinders ;
			temp_device .cylsize 	=	Sector_To_MB( temp_device .heads * temp_device .sectors ) ;
			
			//make sure cylsize is at least 1 MB
			if ( temp_device .cylsize < 1 )
				temp_device .cylsize = 1 ;
				
			//normal harddisk
			if ( lp_disk )
			{
				temp_device .disktype =	lp_disk ->type ->name ;
				temp_device .max_prims = ped_disk_get_max_primary_partition_count( lp_disk ) ;
				
				set_device_partitions( temp_device ) ;

				if ( temp_device .highest_busy )
					set_mountpoints( temp_device .device_partitions ) ;
				
				set_used_sectors( temp_device .device_partitions ) ;
				
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
				temp_device .device_partitions .push_back( partition_temp );
			}
					
			devices .push_back( temp_device ) ;
			
			close_device_and_disk( ) ;
		}
	}
}

GParted::FILESYSTEM GParted_Core::Get_Filesystem( ) 
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
	device .device_partitions .clear( ) ;
	
	lp_partition = ped_disk_next_partition( lp_disk, NULL ) ;
	while ( lp_partition )
	{
		partition_temp .Reset( ) ;
		
		switch ( lp_partition ->type )
		{
			case PED_PARTITION_NORMAL:
			case PED_PARTITION_LOGICAL:             
				partition_temp .Set( 	device .path,
							device .path + num_to_str( lp_partition ->num ),
							lp_partition ->num,
							lp_partition ->type == 0 ? GParted::TYPE_PRIMARY : GParted::TYPE_LOGICAL,
							Get_Filesystem( ), lp_partition ->geom .start,
							lp_partition ->geom .end,
							lp_partition ->type,
							ped_partition_is_busy( lp_partition ) );
						
				partition_temp .flags = Get_Flags( ) ;									
				
				if ( partition_temp .busy && partition_temp .partition_number > device .highest_busy )
					device .highest_busy = partition_temp .partition_number ;
									
				break ;
		
			case PED_PARTITION_EXTENDED:
				partition_temp.Set( 	device .path,
							device .path + num_to_str( lp_partition ->num ),
							lp_partition ->num ,
							GParted::TYPE_EXTENDED ,
							GParted::FS_EXTENDED ,
							lp_partition ->geom .start ,
							lp_partition ->geom .end ,
							false ,
							ped_partition_is_busy( lp_partition ) );
				
				partition_temp .flags = Get_Flags( ) ;
				EXT_INDEX = device .device_partitions .size ( ) ;
				break ;
		
			default:
				break;
		}
		
		//if there's an end, there's a partition ;)
		if ( partition_temp .sector_end > -1 )
		{
			if ( ! partition_temp .inside_extended )
				device .device_partitions .push_back( partition_temp );
			else
				device .device_partitions[ EXT_INDEX ] .logicals .push_back( partition_temp ) ;
		}
		
		//next partition (if any)
		lp_partition = ped_disk_next_partition( lp_disk, lp_partition ) ;
	}
	
	if ( EXT_INDEX > -1 )
		Insert_Unallocated( device .path, device .device_partitions[ EXT_INDEX ] .logicals, device .device_partitions[ EXT_INDEX ] .sector_start, device .device_partitions[ EXT_INDEX ] .sector_end, true ) ;
	
	Insert_Unallocated( device .path, device .device_partitions, 0, device .length -1, false ) ; 
}

void GParted_Core::set_mountpoints( std::vector<Partition> & partitions, bool first_time ) 
{
	if ( first_time )
	{
		char node[255], mountpoint[255] ;
		std::string line ;
		std::ifstream input( "/proc/mounts" ) ;

		while ( getline( input, line ) )
			if ( line .length() > 0 && line[ 0 ] == '/' && sscanf( line .c_str(),"%s %s", node, mountpoint ) == 2 )
				mount_info[ node ] = mountpoint ;
		
		input .close() ;
	}


	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		if ( partitions[ t ] .busy && partitions[ t ] .filesystem != GParted::FS_LINUX_SWAP )
		{
			if ( partitions[ t ] .type == GParted::TYPE_PRIMARY || partitions[ t ] .type == GParted::TYPE_LOGICAL ) 
			{
				iter = mount_info .find( partitions[ t ] .partition );
				if ( iter != mount_info .end() )
				{
					partitions[ t ] .mountpoint = iter ->second ;
					mount_info .erase( iter ) ;
				}
				else 
					partitions[ t ] .mountpoint = "/" ;
			}
			else if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
				set_mountpoints( partitions[ t ] .logicals, false ) ;
		}
	}

	if ( first_time )
		mount_info .clear() ;
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
					if ( statvfs( partitions[ t ] .mountpoint .c_str(), &sfs ) == 0 )
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
	
void GParted_Core::Insert_Unallocated( const Glib::ustring & device_path, std::vector<Partition> & partitions, Sector start, Sector end, bool inside_extended )
{
	partition_temp .Reset( ) ;
	partition_temp .Set_Unallocated( device_path, 0, 0, inside_extended ) ;
	
	//if there are no partitions at all..
	if ( partitions .empty( ) )
	{
		partition_temp .sector_start = start ;
		partition_temp .sector_end = end ;
		
		partitions .push_back( partition_temp );
		
		return ;
	}
		
	//start <---> first partition start
	if ( (partitions .front( ) .sector_start - start) >= MEGABYTE )
	{
		partition_temp .sector_start = start ;
		partition_temp .sector_end = partitions .front( ) .sector_start -1 ;
		
		partitions .insert( partitions .begin( ), partition_temp );
	}
	
	//look for gaps in between
	for ( unsigned int t =0 ; t < partitions .size( ) -1 ; t++ )
		if ( ( partitions[ t +1 ] .sector_start - partitions[ t ] .sector_end ) >= MEGABYTE )
		{
			partition_temp .sector_start = partitions[ t ] .sector_end +1 ;
			partition_temp .sector_end = partitions[ t +1 ] .sector_start -1 ;
		
			partitions .insert( partitions .begin( ) + ++t, partition_temp );
		}
		
	//last partition end <---> end
	if ( (end - partitions .back( ) .sector_end ) >= MEGABYTE )
	{
		partition_temp .sector_start = partitions .back( ) .sector_end +1 ;
		partition_temp .sector_end = end ;
		
		partitions .push_back( partition_temp );
	}
}

int GParted_Core::get_estimated_time( const Operation & operation )
{
	switch ( operation .operationtype )
	{
		case GParted::DELETE:
			return 2 ; //i guess it'll never take more then 2 secs to delete a partition ;)
		
		case GParted::CREATE:
		case GParted::CONVERT:
			set_proper_filesystem( operation .partition_new .filesystem ) ;
			if ( p_filesystem )
				return p_filesystem ->get_estimated_time( operation .partition_new .Get_Length_MB( ) ) ;
			
			break ;
			
		case GParted::RESIZE_MOVE:
			set_proper_filesystem( operation .partition_new .filesystem ) ;
			if ( p_filesystem )
				return p_filesystem ->get_estimated_time( std::abs( operation .partition_original .Get_Length_MB( ) - operation .partition_new .Get_Length_MB( ) ) ) ;
			
			break ;
		
		case GParted::COPY:
			//lets take 10MB/s for the moment..
			return operation .partition_new .Get_Length_MB( ) / 10 ;
	}
	
	return -1 ; //pulsing
}

void GParted_Core::Apply_Operation_To_Disk( Operation & operation )
{
	switch ( operation .operationtype )
	{
		case DELETE:
			if ( ! Delete( operation .partition_original ) ) 
				Show_Error( String::ucompose( _("Error while deleting %1"), operation .partition_original .partition ) ) ;
														
			break;
		case CREATE:
			if ( ! Create( operation .device, operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while creating %1"), operation .partition_new .partition ) );
											
			break;
		case RESIZE_MOVE:
			if ( ! Resize( operation .device, operation .partition_original, operation .partition_new ) )
				Show_Error( String::ucompose( _("Error while resizing/moving %1"), operation .partition_new .partition ) ) ;
											
			break;
		case CONVERT:
			if ( ! Convert_FS( operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while converting filesystem of %1"), operation .partition_new .partition ) ) ;
										
			break;
		case COPY:
			if ( ! Copy( operation .copied_partition_path, operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while copying %1"), operation .partition_new .partition ) ) ;
	}
}

bool GParted_Core::Create( const Device & device, Partition & new_partition ) 
{
	if ( new_partition .type == GParted::TYPE_EXTENDED )   
		return Create_Empty_Partition( new_partition ) ;
	
	else if ( Create_Empty_Partition( new_partition, ( new_partition .Get_Length_MB( ) - device .cylsize ) < get_fs( new_partition .filesystem ) .MIN ) > 0 )
	{
		set_proper_filesystem( new_partition .filesystem ) ;
		
		//most likely this means the user created an unformatted partition,
		//however in theory, it could also screw some errorhandling.
		if ( ! p_filesystem )
			return true ;

		return set_partition_type( new_partition ) && p_filesystem ->Create( new_partition ) ;
	}
	
	return false ;
}

bool GParted_Core::Convert_FS( const Partition & partition )
{	
	//remove all filesystem signatures...
	erase_filesystem_signatures( partition ) ;
	
	set_proper_filesystem( partition .filesystem ) ;
	
	return set_partition_type( partition ) && p_filesystem ->Create( partition ) ;
}

bool GParted_Core::Delete( const Partition & partition ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( partition .device_path ) )
	{
		if ( partition .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, (partition .sector_end + partition .sector_start) / 2 ) ;
		
		return_value = ( ped_disk_delete_partition( lp_disk, lp_partition ) && commit( ) ) ;
	
		close_device_and_disk( ) ;
	}
	
	return return_value ;
}

bool GParted_Core::Resize( const Device & device, const Partition & partition_old, const Partition & partition_new ) 
{
	if ( partition_old .type == GParted::TYPE_EXTENDED )
		return Resize_Container_Partition( partition_old, partition_new, false ) ;
	
	//lazy check (only grow). it's possbile one day this should be separated in checks for grow,shrink,move ..
	if ( get_fs( partition_old .filesystem ) .grow == GParted::FS::LIBPARTED )
		return Resize_Normal_Using_Libparted( partition_old, partition_new ) ;
	else //use custom resize tools..
	{
		set_proper_filesystem( partition_new .filesystem ) ;
				
		if ( p_filesystem ->Check_Repair( partition_new ) )
		{		
			//shrinking
			if ( partition_new .Get_Length_MB( ) < partition_old .Get_Length_MB( ) )
			{
				p_filesystem ->cylinder_size = device .cylsize ;
				
				if ( p_filesystem ->Resize( partition_new ) )
					Resize_Container_Partition( partition_old, partition_new, ! get_fs( partition_new .filesystem ) .move ) ;
			}
			//growing/moving
			else
				Resize_Container_Partition( partition_old, partition_new, ! get_fs( partition_new .filesystem ) .move ) ;
					
			
			p_filesystem ->Check_Repair( partition_new ) ;
			
			p_filesystem ->Resize( partition_new, true ) ; //expand filesystem to fit exactly in partition
			
			return p_filesystem ->Check_Repair( partition_new ) ;
		}
	}
	
	return false ;
}

bool GParted_Core::Copy( const Glib::ustring & src_part_path, Partition & partition_dest ) 
{
	set_proper_filesystem( partition_dest .filesystem ) ;
	
	Partition src_partition ;
	src_partition .partition = src_part_path ;
	
	if ( p_filesystem ->Check_Repair( src_partition ) )
		if ( Create_Empty_Partition( partition_dest, true ) > 0 )
			return p_filesystem ->Copy( src_part_path, partition_dest .partition ) ;
	
	return false ;
}

bool GParted_Core::Set_Disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( device_path, false ) )
	{
		PedDiskType *type = NULL ;
		type = ped_disk_type_get( disklabel .c_str( ) ) ;
		
		if ( type )
		{
			lp_disk = ped_disk_new_fresh ( lp_device, type);
		
			return_value = commit( ) ;
		}
		
		close_device_and_disk( ) ;
	}
	
	return return_value ;	
}

const std::vector<FS> & GParted_Core::get_filesystems( ) const 
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

Glib::RefPtr<Gtk::TextBuffer> GParted_Core::get_textbuffer( )
{
	return textbuffer ;
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

Glib::ustring GParted_Core::get_sym_path( const Glib::ustring & real_path ) 
{ 
	int major, minor, size;
	char temp[4096], device_name[4096], short_path[4096] ;
	
	FILE* proc_part_file = fopen ( "/proc/partitions", "r" );
	if ( ! proc_part_file )
		return real_path;
		
	//skip first 2 useless rules of /proc/partitions
	fgets( temp, 256, proc_part_file ); fgets( temp, 256, proc_part_file );

	while ( fgets( temp, 4096, proc_part_file ) && sscanf(temp, "%d %d %d %255s", &major, &minor, &size, device_name ) == 4 )
	{
		strcpy( short_path, "/dev/" ); strcat( short_path, device_name );
		realpath( short_path, device_name );
	
		if ( real_path == device_name ) { 
			fclose ( proc_part_file );
		  	return ( Glib::ustring( short_path ) );
		}
		
	}
	
	//paranoia modus :)
	fclose ( proc_part_file );	
	return real_path;
}	

void GParted_Core::LP_Set_Used_Sectors( Partition & partition )
{
	PedFileSystem *fs = NULL;
	PedConstraint *constraint = NULL;
	
	if ( lp_disk )
	{
		if ( lp_partition )
		{
			fs = ped_file_system_open( & lp_partition ->geom ); 	
					
			if ( fs )
			{
				constraint = ped_file_system_get_resize_constraint( fs ) ;
				if ( constraint )
				{
					partition .Set_Unused( (partition .sector_end - partition .sector_start) - constraint ->min_size ) ;
					
					ped_constraint_destroy( constraint );
				}
												
				ped_file_system_close( fs ) ;
			}
		}
	}
}

int GParted_Core::Create_Empty_Partition( Partition & new_partition, bool copy )
{
	new_partition .partition_number = 0 ;
		
	if ( open_device_and_disk( new_partition .device_path ) )
	{
		PedPartitionType type;
		PedPartition *c_part = NULL ;
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
		
		c_part = ped_partition_new( lp_disk, type, NULL, new_partition .sector_start, new_partition .sector_end ) ;
		if ( c_part )
		{
			constraint = ped_constraint_any( lp_device );
			
			if ( constraint )
			{
				if ( copy )
					constraint ->min_size = new_partition .sector_end - new_partition .sector_start ;
				
				if ( ped_disk_add_partition( lp_disk, c_part, constraint ) && commit( ) )
				{
					new_partition .partition = ped_partition_get_path( c_part ) ;
					new_partition .partition_number = c_part ->num ;
					
					if ( ! wait_for_node( new_partition .partition ) )
						return 0 ;
				}
			
				ped_constraint_destroy( constraint );
			}
			
		}
				
		close_device_and_disk( ) ;
	}

	//remove all filesystem signatures...
	if ( new_partition .partition_number > 0 )
		erase_filesystem_signatures( new_partition ) ;
		
	return new_partition .partition_number ;
}

bool GParted_Core::Resize_Container_Partition( const Partition & partition_old, const Partition & partition_new, bool fixed_start )
{
	bool return_value = false ;
	
	PedConstraint *constraint = NULL ;
	lp_partition = NULL ;
		
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		if ( partition_old .type == GParted::TYPE_EXTENDED )
			lp_partition = ped_disk_extended_partition( lp_disk ) ;
		else		
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, (partition_old .sector_end + partition_old .sector_start) / 2 ) ;
		
		if ( lp_partition )
		{
			constraint = ped_constraint_any( lp_device );
			
			if ( fixed_start && constraint ) //create a constraint which keeps de startpoint intact and rounds the end to a cylinderboundary
			{
				ped_disk_set_partition_geom( lp_disk, lp_partition, constraint, partition_new .sector_start, partition_new .sector_end ) ;
				ped_constraint_destroy( constraint );
				constraint = NULL ;
				
				ped_geometry_set_start( & lp_partition ->geom, partition_new .sector_start ) ;
				constraint = ped_constraint_exact( & lp_partition ->geom ) ;
			}
			
			if ( constraint )
			{
				if ( ped_disk_set_partition_geom( lp_disk, lp_partition, constraint, partition_new .sector_start, partition_new .sector_end ) )
					return_value = commit( ) ;
									
				ped_constraint_destroy( constraint );
			}
		}
		
		close_device_and_disk( ) ;
	}
	
	return wait_for_node( partition_new .partition ) && return_value ;
}

bool GParted_Core::Resize_Normal_Using_Libparted( const Partition & partition_old, const Partition & partition_new )
{
	bool return_value = false ;
	
	PedFileSystem *fs = NULL ;
	PedConstraint *constraint = NULL ;
	lp_partition = NULL ;
	
	if ( open_device_and_disk( partition_old .device_path ) )
	{
		lp_partition = ped_disk_get_partition_by_sector( lp_disk, (partition_old .sector_end + partition_old .sector_start) / 2 ) ;
		if ( lp_partition )
		{
			fs = ped_file_system_open ( & lp_partition ->geom );
			if ( fs )
			{
				constraint = ped_file_system_get_resize_constraint ( fs );
				if ( constraint )
				{
					if ( ped_disk_set_partition_geom ( lp_disk, lp_partition, constraint, partition_new .sector_start, partition_new .sector_end ) &&
						ped_file_system_resize ( fs, & lp_partition ->geom, NULL )
                                           )
							return_value = commit( ) ;
										
					ped_constraint_destroy ( constraint );
				}
				
				ped_file_system_close ( fs );
			}
		}
		
		close_device_and_disk( ) ;
	}
	
	return return_value ;
}

Glib::ustring GParted_Core::Get_Flags( )
{
	temp = "";
		
	for ( unsigned short t = 0; t < flags .size( ) ; t++ )
		if ( ped_partition_get_flag ( lp_partition, flags[ t ] ) )
			temp += static_cast<Glib::ustring>( ped_partition_flag_get_name( flags[ t ] ) ) + " ";
		
	return temp ;
}

void GParted_Core::Show_Error( Glib::ustring message ) 
{
	message = "<span weight=\"bold\" size=\"larger\">" + message + "</span>\n\n" ;
	message += _( "Be aware that the failure to apply this operation could affect other operations on the list." ) ;
	Gtk::MessageDialog dialog( message ,true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true );
	
	gdk_threads_enter( );
	dialog .run( );
	gdk_threads_leave( );
}

void GParted_Core::set_proper_filesystem( const FILESYSTEM & filesystem )
{
	if ( ! p_filesystem )
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
	
	if ( p_filesystem )
		p_filesystem ->textbuffer = textbuffer ;
}


bool GParted_Core::set_partition_type( const Partition & partition ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( partition .device_path ) )
	{
		PedFileSystemType * fs_type = ped_file_system_type_get( Get_Filesystem_String( partition .filesystem ) .c_str() ) ;

		//default is Linux (83)
		if ( ! fs_type )
			fs_type = ped_file_system_type_get( "ext2" ) ;

		if ( fs_type )
		{
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, (partition .sector_end + partition .sector_start) / 2 ) ;

			if ( lp_partition && ped_partition_set_system( lp_partition, fs_type ) && commit( ) )
				return_value = wait_for_node( partition .partition ) ;
		}

		close_device_and_disk( ) ;
	}

	return return_value ;
}
	
bool GParted_Core::wait_for_node( const Glib::ustring & node ) 
{
	//we'll loop for 10 seconds or till 'node' appeares...
	for( short t = 0 ; t < 50 ; t++ )
	{
		if ( access( node .c_str(), F_OK ) == 0 )
		{ 
			sleep( 1 ) ; //apperantly the node isn't available immediatly after access returns succesfully :/
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
		lp_partition = ped_disk_get_partition_by_sector( lp_disk, (partition .sector_end + partition .sector_start) / 2 ) ;

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
		
		close_device_and_disk( ) ;	
	}

	return return_value ;
}

bool GParted_Core::open_device( const Glib::ustring & device_path )
{
	lp_device = ped_device_get( device_path .c_str( ) );
	
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

void GParted_Core::close_device_and_disk( )
{
	if ( lp_device )
		ped_device_destroy( lp_device ) ;
		
	if ( lp_disk )
		ped_disk_destroy( lp_disk ) ;
	
	lp_device = NULL ;
	lp_disk = NULL ;
}	

bool GParted_Core::commit( ) 
{
	bool return_value = ped_disk_commit_to_dev( lp_disk ) ;
	
	ped_disk_commit_to_os( lp_disk ) ;
		
	return return_value ;
}


} //GParted
