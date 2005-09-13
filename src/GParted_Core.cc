#include "../include/GParted_Core.h"

namespace GParted
{
	
GParted_Core::GParted_Core( ) 
{
	device = NULL ;
	disk = NULL ;
	c_partition = NULL ;
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
	FS fs ; fs .filesystem = "unknown" ;
	FILESYSTEMS .push_back( fs ) ;
}

void GParted_Core::get_devices( std::vector<Device> & devices )
{
	devices .clear( ) ;
		
	//try to find all available devices and put these in a list
	ped_device_probe_all( );
	
	Device temp_device ;
	std::vector <Glib::ustring> device_paths ;
	
	device = ped_device_get_next( NULL );
	
	//in certain cases (e.g. when there's a cd in the cdrom-drive) ped_device_probe_all will find a 'ghost' device that has no name or contains
	//random garbage. Those 2 checks try to prevent such a ghostdevice from being initialized.. (tested over a 1000 times with and without cd)
	while ( device && strlen( device ->path ) > 6 && static_cast<Glib::ustring>( device ->path ) .is_ascii( ) )
	{
		if ( open_device( device ->path, device ) )
			device_paths .push_back( get_sym_path( device ->path ) ) ;
					
		device = ped_device_get_next( device ) ;
	}
	close_device_and_disk( device, disk ) ;
	
	for ( unsigned int t = 0 ; t < device_paths .size( ) ; t++ ) 
	{ 
		if ( open_device_and_disk( device_paths[ t ], device, disk, false ) )
		{
			temp_device .Reset( ) ;
			
			//device info..
			temp_device .path 	=	device_paths[ t ] ;
			temp_device .realpath	= 	device ->path ; 
			temp_device .model 	=	device ->model ;
			temp_device .heads 	=	device ->bios_geom .heads ;
			temp_device .sectors 	=	device ->bios_geom .sectors ;
			temp_device .cylinders	=	device ->bios_geom .cylinders ;
			temp_device .length 	=	temp_device .heads * temp_device .sectors * temp_device .cylinders ;
			temp_device .cylsize 	=	Sector_To_MB( temp_device .heads * temp_device .sectors ) ;
			
			//make sure cylsize is at least 1 MB
			if ( temp_device .cylsize < 1 )
				temp_device .cylsize = 1 ;
				
			//normal harddisk
			if ( disk )
			{
				temp_device .disktype =	disk ->type ->name ;
				temp_device .max_prims = ped_disk_get_max_primary_partition_count( disk ) ;
				
				set_device_partitions( temp_device ) ;
				
				if ( temp_device .highest_busy )
					temp_device .readonly = ! ped_disk_commit_to_os( disk ) ;
			}
			//harddisk without disklabel
			else
			{
				temp_device .disktype = _("unrecognized") ;
				temp_device .max_prims = -1 ;
				
				Partition partition_temp ;
				partition_temp .Set_Unallocated( 0, temp_device .length, false );
				temp_device .device_partitions .push_back( partition_temp );
			}
					
			devices .push_back( temp_device ) ;
			
			close_device_and_disk( device, disk ) ;
		}
	}
}

Glib::ustring GParted_Core::Get_Filesystem( ) 
{
	//standard libparted filesystems..
	if ( c_partition ->fs_type )
		return c_partition ->fs_type ->name ;
	
	//other filesystems libparted couldn't detect (i've send patches for these filesystems to the parted guys)
	char buf[512] ;
	ped_device_open( device );

	//reiser4
	ped_geometry_read ( & c_partition ->geom, buf, 128, 1) ;
	strcpy( buf, strcmp( buf, "ReIsEr4" ) == 0 ? "reiser4" : "" ) ;

	ped_device_close( device );
	if ( strlen( buf ) ) 
		return buf ;		
		
	//no filesystem found....
	partition_temp .error = _( "Unable to detect filesystem! Possible reasons are:" ) ;
	partition_temp .error += "\n-"; 
	partition_temp .error += _( "The filesystem is damaged" ) ;
	partition_temp .error += "\n-" ; 
	partition_temp .error += _( "The filesystem is unknown to libparted" ) ;
	partition_temp .error += "\n-"; 
	partition_temp .error += _( "There is no filesystem available (unformatted)" ) ; 
		
	return _("unknown") ;
}

void GParted_Core::set_device_partitions( Device & device ) 
{
	int EXT_INDEX = -1 ;
	
	//clear partitions
	device .device_partitions .clear( ) ;
	
	c_partition = ped_disk_next_partition( disk, NULL ) ;
	while ( c_partition )
	{
		partition_temp .Reset( ) ;
		
		switch ( c_partition ->type )
		{
			case PED_PARTITION_NORMAL:
			case PED_PARTITION_LOGICAL:             
				partition_temp .Set( 	device .path + num_to_str( c_partition ->num ),
							c_partition ->num,
							c_partition ->type == 0 ? GParted::PRIMARY : GParted::LOGICAL ,
							Get_Filesystem( ), c_partition ->geom .start,
							c_partition ->geom .end,
							c_partition ->type,
							ped_partition_is_busy( c_partition ) );
						
				if ( partition_temp .filesystem != "linux-swap" )
				{
					Set_Used_Sectors( partition_temp ) ;
					
					//the 'Unknown' filesystem warning overrules this one
					if ( partition_temp .sectors_used == -1 && partition_temp .error .empty( ) )
					{
						partition_temp .error = _("Unable to read the contents of this filesystem!") ;
						partition_temp .error += "\n" ;
						partition_temp .error += _("Because of this some operations may be unavailable.") ;
					}
				}
							
				partition_temp .flags = Get_Flags( c_partition ) ;									
				
				if ( partition_temp .busy && partition_temp .partition_number > device .highest_busy )
					device .highest_busy = partition_temp .partition_number ;
									
				break ;
		
			case PED_PARTITION_EXTENDED:
				partition_temp.Set( 	device .path + num_to_str( c_partition ->num ),
							c_partition ->num ,
							GParted::EXTENDED ,
							"extended" ,
							c_partition ->geom .start ,
							c_partition ->geom .end ,
							false ,
							ped_partition_is_busy( c_partition ) );
				
				partition_temp .flags = Get_Flags( c_partition ) ;
				EXT_INDEX = device .device_partitions .size ( ) ;
				break ;
		
			default:	break;
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
		c_partition = ped_disk_next_partition ( disk, c_partition ) ;
	}
	
	if ( EXT_INDEX > -1 )
		Insert_Unallocated( device .device_partitions[ EXT_INDEX ] .logicals, device .device_partitions[ EXT_INDEX ] .sector_start, device .device_partitions[ EXT_INDEX ] .sector_end, true ) ;
	
	Insert_Unallocated( device .device_partitions, 0, device .length -1, false ) ; 
}

void GParted_Core::Insert_Unallocated( std::vector<Partition> & partitions, Sector start, Sector end, bool inside_extended )
{
	partition_temp .Reset( ) ;
	partition_temp .Set_Unallocated( 0, 0, inside_extended ) ;
	
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
			if ( ! Delete( operation .device .path, operation .partition_original ) ) 
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
			if ( ! Convert_FS( operation .device .path, operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while converting filesystem of %1"), operation .partition_new .partition ) ) ;
										
			break;
		case COPY:
			if ( ! Copy( operation .device .path, operation .copied_partition_path, operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while copying %1"), operation .partition_new .partition ) ) ;
	}
}

bool GParted_Core::Create( const Device & device, Partition & new_partition ) 
{
	if ( new_partition .type == GParted::EXTENDED )   
		return Create_Empty_Partition( device .path, new_partition ) ;
	
	else if ( Create_Empty_Partition( device .path, new_partition, ( new_partition .Get_Length_MB( ) - device .cylsize ) < get_fs( new_partition .filesystem ) .MIN ) > 0 )
	{
		set_proper_filesystem( new_partition .filesystem ) ;
		
		//most likely this means the user created an unformatted partition, however in theory, it could also screw some errorhandling.
		if ( ! p_filesystem )
			return true ;
		
		return p_filesystem ->Create( new_partition ) ;
	}
	
	return false ;
}

bool GParted_Core::Convert_FS( const Glib::ustring & device_path, const Partition & partition )
{	
	//remove all filesystem signatures...
	if ( open_device_and_disk( device_path, device, disk ) )
	{
		c_partition = ped_disk_get_partition_by_sector( disk, (partition .sector_end + partition .sector_start) / 2 ) ;
		
		if ( c_partition )
			ped_file_system_clobber ( & c_partition ->geom ) ;
			
		close_device_and_disk( device, disk ) ;	
	}
	
	set_proper_filesystem( partition .filesystem ) ;

	return p_filesystem ->Create( partition ) ;
}

bool GParted_Core::Delete( const Glib::ustring & device_path, const Partition & partition ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( device_path, device, disk ) )
	{
		if ( partition .type == GParted::EXTENDED )
			c_partition = ped_disk_extended_partition( disk ) ;
		else
			c_partition = ped_disk_get_partition_by_sector( disk, (partition .sector_end + partition .sector_start) / 2 ) ;
		
		return_value = ( ped_disk_delete_partition( disk, c_partition ) && Commit( disk ) ) ;
		close_device_and_disk( device, disk ) ;
		
		sleep( 1 ) ;//paranoia give the OS some time to update nodes in /dev
	}
	
	return return_value ;
}

bool GParted_Core::Resize( const Device & device, const Partition & partition_old, const Partition & partition_new ) 
{
	if ( partition_old .type == GParted::EXTENDED )
		return Resize_Container_Partition( device .path, partition_old, partition_new, false ) ;
	
	//these 2 still use libparted's resizer.
	else if ( partition_old .filesystem == "fat16" || partition_old .filesystem == "fat32" )
		return Resize_Normal_Using_Libparted( device .path, partition_old, partition_new ) ;

	//use custom resize tools..
	else 
	{
		set_proper_filesystem( partition_new .filesystem ) ;
				
		if ( p_filesystem ->Check_Repair( partition_new ) )
		{		
			//shrinking
			if ( partition_new .Get_Length_MB( ) < partition_old .Get_Length_MB( ) )
			{
				p_filesystem ->cylinder_size = device .cylsize ;
				
				if ( p_filesystem ->Resize( partition_new ) )
					Resize_Container_Partition( device .path, partition_old, partition_new, ! get_fs( partition_new .filesystem ) .move ) ;
			}
			//growing/moving
			else
				Resize_Container_Partition( device .path, partition_old, partition_new, ! get_fs( partition_new .filesystem ) .move ) ;
					
			
			p_filesystem ->Check_Repair( partition_new ) ;
			
			p_filesystem ->Resize( partition_new, true ) ; //expand filesystem to fit exactly in partition
			
			return p_filesystem ->Check_Repair( partition_new ) ;
		}
	}
	
	return false ;
}

bool GParted_Core::Copy( const Glib::ustring & dest_device_path, const Glib::ustring & src_part_path, Partition & partition_dest ) 
{
	set_proper_filesystem( partition_dest .filesystem ) ;
	
	Partition src_partition ;
	src_partition .partition = src_part_path ;
	
	if ( p_filesystem ->Check_Repair( src_partition ) )
		if ( Create_Empty_Partition( dest_device_path, partition_dest, true ) > 0 )
			return p_filesystem ->Copy( src_part_path, partition_dest .partition ) ;
	
	return false ;
}

bool GParted_Core::Set_Disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( device_path, device, disk, false ) )
	{
		PedDiskType *type = NULL ;
		type = ped_disk_type_get( disklabel .c_str( ) ) ;
		
		if ( type )
		{
			disk = ped_disk_new_fresh ( device, type);
		
			return_value = Commit( disk ) ;
		}
		
		close_device_and_disk( device, disk ) ;
	}
	
	return return_value ;	
}

const std::vector<FS> & GParted_Core::get_filesystems( ) const 
{
	return FILESYSTEMS ;
}

const FS & GParted_Core::get_fs( const Glib::ustring & filesystem ) const 
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

std::vector<Glib::ustring>  GParted_Core::get_disklabeltypes( ) 
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


void GParted_Core::Set_Used_Sectors( Partition & partition )
{
	//because 'unknown' is translated we need to call get_fs instead of using partition .filesystem directly
	if ( get_fs( partition .filesystem ) .filesystem == "unknown" )
		partition .Set_Unused( -1 ) ;
	
	else if ( partition .busy )
	{
		system( ("df -k --sync " + partition .partition + " | grep " + partition .partition + " > /tmp/.tmp_gparted") .c_str( ) );
		std::ifstream file_input( "/tmp/.tmp_gparted" );
		
		//we need the 4th value
		file_input >> temp; file_input >> temp; file_input >> temp;file_input >> temp;
		if ( ! temp .empty( ) ) 
			partition .Set_Unused( atol( temp .c_str( ) ) * 2 ) ;//  1024/512
		
		file_input .close( );
		system( "rm -f /tmp/.tmp_gparted" );
	}
	
	else if ( get_fs( partition .filesystem ) .read )
	{
		set_proper_filesystem( partition .filesystem ) ;
		
		p_filesystem ->disk = disk ;
		p_filesystem ->Set_Used_Sectors( partition ) ;
	}
}

int GParted_Core::Create_Empty_Partition( const Glib::ustring & device_path, Partition & new_partition, bool copy )
{
	new_partition .partition_number = 0 ;
		
	if ( open_device_and_disk( device_path, device, disk ) )
	{
		PedPartitionType type;
		PedPartition *c_part = NULL ;
		PedConstraint *constraint = NULL ;
		
		//create new partition
		switch ( new_partition .type )
		{
			case 0	:	type = PED_PARTITION_NORMAL; 	break ;
			case 1	:	type = PED_PARTITION_LOGICAL; 	break ;
			case 2	:	type = PED_PARTITION_EXTENDED; 	break ;
			default	:	type = PED_PARTITION_FREESPACE;	break ; //will never happen ;)
		}
		
		c_part = ped_partition_new ( disk, type, NULL, new_partition .sector_start, new_partition .sector_end ) ;
		if ( c_part )
		{
			constraint = ped_constraint_any( device );
			
			if ( constraint )
			{
				if ( copy )
					constraint ->min_size = new_partition .sector_end - new_partition .sector_start ;
				
				if ( ped_disk_add_partition ( disk, c_part, constraint ) && Commit( disk ) )
				{
					//remove all filesystem signatures...
					ped_file_system_clobber ( & c_part ->geom ) ;
					
					sleep( 1 ) ;//the OS needs some time to create the devicenode in /dev
					
					new_partition .partition = ped_partition_get_path( c_part ) ;
					new_partition .partition_number = c_part ->num ;
				}
			
				ped_constraint_destroy ( constraint );
			}
			
		}
				
		close_device_and_disk( device, disk ) ;
	}
		
	return new_partition .partition_number ;
}

bool GParted_Core::Resize_Container_Partition( const Glib::ustring & device_path, const Partition & partition_old, const Partition & partition_new, bool fixed_start )
{
	bool return_value = false ;
	
	PedConstraint *constraint = NULL ;
	c_partition = NULL ;
		
	if ( open_device_and_disk( device_path, device, disk ) )
	{
		if ( partition_old .type == GParted::EXTENDED )
			c_partition = ped_disk_extended_partition( disk ) ;
		else		
			c_partition = ped_disk_get_partition_by_sector( disk, (partition_old .sector_end + partition_old .sector_start) / 2 ) ;
		
		if ( c_partition )
		{
			constraint = ped_constraint_any( device );
			
			if ( fixed_start && constraint ) //create a constraint which keeps de startpoint intact and rounds the end to a cylinderboundary
			{
				ped_disk_set_partition_geom ( disk, c_partition, constraint, partition_new .sector_start, partition_new .sector_end ) ;
				ped_constraint_destroy ( constraint );
				constraint = NULL ;
				
				ped_geometry_set_start ( & c_partition ->geom, partition_new .sector_start ) ;
				constraint = ped_constraint_exact ( & c_partition ->geom ) ;
			}
			
			if ( constraint )
			{
				if ( ped_disk_set_partition_geom ( disk, c_partition, constraint, partition_new .sector_start, partition_new .sector_end ) )
					return_value = Commit( disk ) ;
									
				ped_constraint_destroy ( constraint );
			}
		}
		
		close_device_and_disk( device, disk ) ;
	}
	
	sleep( 1 ) ; //the OS needs time to re-add the devicenode..
		
	return return_value ;
}

bool GParted_Core::Resize_Normal_Using_Libparted( const Glib::ustring & device_path, const Partition & partition_old, const Partition & partition_new )
{
	bool return_value = false ;
	
	PedFileSystem *fs = NULL ;
	PedConstraint *constraint = NULL ;
	c_partition = NULL ;
	
	if ( open_device_and_disk( device_path, device, disk ) )
	{
		c_partition = ped_disk_get_partition_by_sector( disk, (partition_old .sector_end + partition_old .sector_start) / 2 ) ;
		if ( c_partition )
		{
			fs = ped_file_system_open ( & c_partition ->geom );
			if ( fs )
			{
				constraint = ped_file_system_get_resize_constraint ( fs );
				if ( constraint )
				{
					if ( 	ped_disk_set_partition_geom ( disk, c_partition, constraint, partition_new .sector_start, partition_new .sector_end ) &&
						ped_file_system_resize ( fs, & c_partition ->geom, NULL )
                                           )
							return_value = Commit( disk ) ;
										
					ped_constraint_destroy ( constraint );
				}
				
				ped_file_system_close ( fs );
			}
		}
		
		close_device_and_disk( device, disk ) ;
	}
	
	return return_value ;
}

Glib::ustring GParted_Core::Get_Flags( PedPartition *c_partition )
{
	temp = "";
		
	for ( unsigned short t = 0; t < flags .size( ) ; t++ )
		if ( ped_partition_get_flag ( c_partition, flags[ t ] ) )
			temp += (Glib::ustring) ped_partition_flag_get_name ( flags[ t ] ) + " ";
		
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

void GParted_Core::set_proper_filesystem( const Glib::ustring & filesystem )
{
	//ugly, stupid, *aaargh*  :-)	
	
	if ( ! p_filesystem )
		delete p_filesystem ;
		
	if ( filesystem == "ext2" )	
		p_filesystem = new ext2( ) ; 
	else if ( filesystem == "ext3" )
		p_filesystem = new ext3( ) ;
	else if ( filesystem == "fat16" )
		p_filesystem = new fat16( ) ;
	else if ( filesystem == "fat32" )
		p_filesystem = new fat32( ) ;
	else if ( filesystem == "hfs" )
		p_filesystem = new hfs( ) ;
	else if ( filesystem == "hfs+" )
		p_filesystem = new hfsplus( ) ;
	else if ( filesystem == "jfs" )
		p_filesystem = new jfs( ) ;
	else if ( filesystem == "linux-swap" )
		p_filesystem = new linux_swap( ) ;
	else if ( filesystem == "reiser4" )
		p_filesystem = new reiser4( ) ;
	else if ( filesystem == "reiserfs" )
		p_filesystem = new reiserfs( ) ;
	else if ( filesystem == "ntfs" )
		p_filesystem = new ntfs( ) ;
	else if ( filesystem == "xfs" )
		p_filesystem = new xfs( ) ;
	else
		p_filesystem = NULL ;

	if ( p_filesystem )
		p_filesystem ->textbuffer = textbuffer ;
}

} //GParted
