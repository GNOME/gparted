#include "../include/GParted_Core.h"

namespace GParted
{
	
//AFAIK it's not possible to use a C++ memberfunction as a callback for a C libary function (if you know otherwise, PLEASE contact me)
Glib::ustring error_message;
PedExceptionOption PedException_Handler (PedException* ex)
{
	error_message = Glib::locale_to_utf8( ex ->message ) ;
	
	std::cout << "\nLIBPARTED MESSAGE ----------------------\n" << error_message << "\n---------------------------\n";
	
	return PED_EXCEPTION_UNHANDLED ;
}
	
	
	
GParted_Core::GParted_Core( ) 
{
	device = NULL ;
	disk = NULL ;
	c_partition = NULL ;
	ped_exception_set_handler( PedException_Handler ) ; 
	
	p_filesystem = NULL ;
	textbuffer = Gtk::TextBuffer::create( ) ;

	//get valid flags ...
	for ( PedPartitionFlag flag = ped_partition_flag_next ( (PedPartitionFlag) 0 ) ; flag ; flag = ped_partition_flag_next ( flag ) )
		flags .push_back( flag ) ;	
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
	
	linux_swap fs_linux_swap;
	FILESYSTEMS .push_back( fs_linux_swap .get_filesystem_support( ) ) ;
	
	reiserfs fs_reiserfs;
	FILESYSTEMS .push_back( fs_reiserfs .get_filesystem_support( ) ) ;
}

void GParted_Core::get_devices( std::vector<Device> & devices, bool deep_scan )
{
	devices .clear( ) ;
		
	//try to find all available devices and put these in a list
	ped_device_probe_all( );
	
	Device temp_device ;
	std::vector <Glib::ustring> device_paths ;
	
	device = ped_device_get_next ( NULL );
	
	//in certain cases (e.g. when there's a cd in the cdrom-drive) ped_device_probe_all will find a 'ghost' device that has no name or contains
	//random garbage. Those 2 checks try to prevent such a ghostdevice from being initialized.. (tested over a 1000 times with and without cd)
	while ( device && strlen( device ->path ) > 6 && ( (Glib::ustring) device ->path ). is_ascii( ) )
	{
		if ( open_device( device ->path, device ) )
			device_paths .push_back( get_sym_path( device ->path ) ) ;
			
		device = ped_device_get_next ( device ) ;
	}
	close_device_and_disk( device, disk ) ;
	
	//and sort the devices on name.. (this prevents some very weird errors ;) )
	sort( device_paths .begin( ), device_paths .end( ) ) ;
	
	
	for ( unsigned int t = 0 ; t < device_paths .size( ) ; t++ ) 
	{ 
		if ( open_device_and_disk( device_paths[ t ], device, disk ) )
		{
			temp_device .Reset( ) ;
			
			temp_device .path 	=	device_paths[ t ] ;
			temp_device .realpath	= 	device ->path ; 
			temp_device .model 	=	device ->model ;
			temp_device .disktype	= 	disk ->type ->name ;
			temp_device .heads 	=	device ->bios_geom .heads ;
			temp_device .sectors 	=	device ->bios_geom .sectors ;
			temp_device .cylinders	=	device ->bios_geom .cylinders ;
			temp_device .length 	=	temp_device .heads * temp_device .sectors * temp_device .cylinders ;
			temp_device .max_prims	= 	ped_disk_get_max_primary_partition_count( disk ) ;
			
			set_device_partitions( temp_device, deep_scan ) ;
			
			devices .push_back( temp_device ) ;
			
			close_device_and_disk( device, disk ) ;
		}
	}
		
}

void GParted_Core::set_device_partitions( Device & device, bool deep_scan ) 
{
	Partition partition_temp ;
	Glib::ustring part_path, error ;
	int EXT_INDEX = -1 ;
	
	//clear partitions
	device .device_partitions .clear( ) ;
	
	c_partition = ped_disk_next_partition( disk, NULL ) ;
	while ( c_partition )
	{
		partition_temp .Reset( ) ;
		part_path = device .path + num_to_str( c_partition ->num ) ;
		
		switch ( c_partition ->type )
		{
			case PED_PARTITION_NORMAL:
			case PED_PARTITION_LOGICAL:
				if ( c_partition ->fs_type )
					temp = c_partition ->fs_type ->name ;
				else
				{
					temp = "unknown" ;
					error = _( "Unable to detect filesystem! Possible reasons are:" ) ;
					error += "\n-"; 
					error += _( "The filesystem is damaged" ) ;
					error += "\n-" ; 
					error += _( "The filesystem is unknown to libparted" ) ;
					error += "\n-"; 
					error += _( "There is no filesystem available (unformatted)" ) ; 
							
				}				
				partition_temp .Set( 	part_path,
							c_partition ->num,
							c_partition ->type == 0 ? GParted::PRIMARY : GParted::LOGICAL ,
							temp, c_partition ->geom .start,
							c_partition ->geom .end,
							c_partition ->type,
							ped_partition_is_busy( c_partition ) );
					
				if ( deep_scan )
					partition_temp .Set_Used( Get_Used_Sectors( c_partition, part_path ) ) ;
							
				partition_temp .flags = Get_Flags( c_partition ) ;									
				partition_temp .error = error .empty( ) ? error_message : error ;
				
				if ( partition_temp .busy )
					device .busy = true ;
					
				break ;
		
			case PED_PARTITION_EXTENDED:
				partition_temp.Set( 	part_path ,
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
		
			case PED_PARTITION_FREESPACE:
			case 5: //freespace inside extended (there's no enumvalue for that..)
				partition_temp .Set_Unallocated( c_partition ->geom .start, c_partition ->geom .end, c_partition ->type == 4 ? false : true );
				break ;
			
			case PED_PARTITION_METADATA:
				if ( device .device_partitions .size( ) && device .device_partitions .back( ) .type == GParted::UNALLOCATED )
					device .device_partitions .back( ) .sector_end = c_partition ->geom .end ;
				
				break ;
				
			case 9: //metadata inside extended (there's no enumvalue for that..)
				if ( 	device .device_partitions[ EXT_INDEX ] .logicals .size( ) &&
					device .device_partitions[ EXT_INDEX ] .logicals .back( ) .type == GParted::UNALLOCATED
				   )
					device .device_partitions[ EXT_INDEX ] .logicals .back( ) .sector_end = c_partition ->geom .end ;
		
				break ;
			
			default:	break;
		}
		
		if ( partition_temp .Get_Length_MB( ) >= 1 ) // check for unallocated < 1MB, and metadatasituations (see PED_PARTITION_METADATA and 9 )
		{
			if ( ! partition_temp .inside_extended )
				device .device_partitions .push_back( partition_temp );
			else
				device .device_partitions[ EXT_INDEX ] .logicals .push_back( partition_temp ) ;
		}
		
		//reset stuff..
		error = error_message = "" ;
		
		//next partition (if any)
		c_partition = ped_disk_next_partition ( disk, c_partition ) ;
	}
	
}

int GParted_Core::get_estimated_time( Operation & operation )
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
				return p_filesystem ->get_estimated_time( Abs( operation .partition_original .Get_Length_MB( ) - operation .partition_new .Get_Length_MB( ) ) ) ;
			
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
			if ( ! Delete( operation .device_path, operation .partition_original ) ) 
				Show_Error( String::ucompose( _("Error while deleting %1"), operation .partition_original .partition ) ) ;
														
			break;
		case CREATE:
			if ( ! Create( operation .device_path, operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while creating %1"), operation .partition_new .partition ) );
											
			break;
		case RESIZE_MOVE:
			if ( ! Resize( operation .device_path, operation .partition_original, operation .partition_new ) )
				Show_Error( String::ucompose( _("Error while resizing/moving %1"), operation .partition_new .partition ) ) ;
											
			break;
		case CONVERT:
			if ( ! Convert_FS( operation .device_path, operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while converting filesystem of %1"), operation .partition_new .partition ) ) ;
										
			break;
		case COPY:
			if ( ! Copy( operation .device_path, operation .copied_partition_path, operation .partition_new ) ) 
				Show_Error( String::ucompose( _("Error while copying %1"), operation .partition_new .partition ) ) ;
									
	}
	
	
}

	
bool GParted_Core::Create( const Glib::ustring & device_path, Partition & new_partition ) 
{
	if ( new_partition .type == GParted::EXTENDED )   
		return Create_Empty_Partition( device_path, new_partition ) ;
	
	else if ( Create_Empty_Partition( device_path, new_partition ) > 0 )
	{
		set_proper_filesystem( new_partition .filesystem ) ;
		
		return p_filesystem ->Create( device_path, new_partition ) ;
	}
	
	return false ;
}

bool GParted_Core::Convert_FS( const Glib::ustring & device_path, const Partition & partition )
{
	set_proper_filesystem( partition .filesystem ) ;
		
	return p_filesystem ->Create( device_path, partition ) ;
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
	}
	
	return return_value ;
}

bool GParted_Core::Resize(const Glib::ustring & device_path, const Partition & partition_old, const Partition & partition_new ) 
{
	if ( partition_old .type == GParted::EXTENDED )
		return Resize_Extended( device_path, partition_new ) ;
	
	else 
	{
		set_proper_filesystem( partition_new .filesystem ) ;
		
		return p_filesystem ->Resize( device_path, partition_old, partition_new ) ;
	}
	
	return false ;
}

bool GParted_Core::Copy( const Glib::ustring & dest_device_path, const Glib::ustring & src_part_path, Partition & partition_dest ) 
{
	if ( Create_Empty_Partition( dest_device_path, partition_dest, true ) > 0 )
	{
		set_proper_filesystem( partition_dest .filesystem ) ;
		
		return p_filesystem ->Copy( src_part_path, partition_dest .partition ) ;
	}
	
	return false ;
}

std::vector<FS> GParted_Core::get_fs( )
{
	return FILESYSTEMS ;
}

Glib::RefPtr<Gtk::TextBuffer> GParted_Core::get_textbuffer( )
{
	return textbuffer ;
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


Sector GParted_Core::Get_Used_Sectors( PedPartition *c_partition, const Glib::ustring & sym_path)
{
	/* This is quite an unreliable process, atm i try two different methods, but since both are far from perfect the results are 
	 * questionable. 
	 * - first i try geometry.get_used() in libpartedpp, i implemented this function to check the minimal size when resizing a partition. Disadvantage 
	*   of this method is the fact it won't work on mounted filesystems. Besides that, its SLOW
	 * - if the former method fails ( result is -1 ) i'll try to read the output from the df command ( df -k --sync <partition path> )
	 * - if this fails the filesystem on the partition is ( more or less ) unknown to the operating system and therefore the unused sectors cannot be calcualted
	 * - as soon as i have my internetconnection back i should ask people with more experience on this stuff for advice !
	*/
	
	//check if there is a (known) filesystem
	if ( ! c_partition ->fs_type )
		return -1;
	
	//used sectors are not relevant for swapspace
	if ( (Glib::ustring) c_partition ->fs_type ->name == "linux-swap" )
		return -1;
	
	//METHOD #1           
	//the getused method doesn't work on mounted partitions and for some filesystems ( i *guess* this is called check by andrew )
	if ( ! ped_partition_is_busy( c_partition ) && Get_FS( c_partition ->fs_type ->name, FILESYSTEMS ) .create )
	{ 		
		PedFileSystem *fs = NULL;
		PedConstraint *constraint = NULL;
	
		fs = ped_file_system_open( & c_partition ->geom ); 	
				
		if ( fs )
		{
			constraint = ped_file_system_get_resize_constraint (fs);
			ped_file_system_close (fs);
			if ( constraint && constraint->min_size != -1 ) 
				return constraint->min_size ;
		}
	}
		
	//METHOD #2
	//this ony works for mounted ( and therefore known to the OS ) filesystems. My method is quite crude, keep in mind it's only temporary ;-)
	if ( ped_partition_is_busy( c_partition ) )
	{
		Glib::ustring buf;
		system( ("df -k --sync " + sym_path + " | grep " + sym_path + " > /tmp/.tmp_gparted") .c_str( ) );
		std::ifstream file_input( "/tmp/.tmp_gparted" );
		
		file_input >> buf; //skip first value
		file_input >> buf;
		if ( buf != "0" && ! buf .empty( ) ) 
		{ 
			file_input >> buf;
			file_input .close( ); system( "rm -f /tmp/.tmp_gparted" );		
			return atoi( buf .c_str( ) ) * 1024/512 ;
		}
		file_input .close( ); system( "rm -f /tmp/.tmp_gparted" );
	}
	
	
	return -1 ; //all methods were unsuccesfull
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
			constraint = ped_constraint_any ( device );
			
			if ( constraint )
			{
				if ( copy )
					constraint ->min_size = new_partition .sector_end - new_partition .sector_start ;
				
				if ( ped_disk_add_partition ( disk, c_part, constraint ) && Commit( disk ) )
				{
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

bool GParted_Core::Resize_Extended( const Glib::ustring & device_path, const Partition & extended ) 
{
	bool return_value = false ;
	
	if ( open_device_and_disk( device_path, device, disk ) )
	{
		PedPartition *c_part = NULL ;
		PedConstraint *constraint = NULL ;
		
		c_part = ped_disk_extended_partition( disk ) ;
		if ( c_part )
		{
			constraint = ped_constraint_any ( device );
			if ( constraint )
			{
				if ( ped_disk_set_partition_geom ( disk, c_part, constraint, extended .sector_start, extended .sector_end ) )
				{
					ped_partition_set_system ( c_part, NULL );
					return_value = Commit( disk ) ;
				}		
					
				ped_constraint_destroy (constraint);
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
	else if ( filesystem == "linux-swap" )
		p_filesystem = new linux_swap( ) ;
	else if ( filesystem == "reiserfs" )
		p_filesystem = new reiserfs( ) ;
	else
		p_filesystem = NULL ;

	if ( p_filesystem )
		p_filesystem ->textbuffer = textbuffer ;
}

} //GParted
