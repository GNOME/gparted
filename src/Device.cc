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
 
#include "../include/Device.h"

/*
 *	This is ridiculous!, if i use this one as a member function realpath starts complaining and won't compile :-S
 */
Glib::ustring get_sym_path( const Glib::ustring & real_path ) 
{ 
	int major,minor,size;
	char temp[4096], device_name[4096],short_path[4096] ;
	
	FILE*	proc_part_file = fopen ("/proc/partitions", "r");
	if (!proc_part_file)
		return real_path;
		
	//skip first 2 useless rules of /proc/partitions
	fgets (temp, 256, proc_part_file);fgets (temp, 256, proc_part_file);

	while (fgets (temp, 4096, proc_part_file) && sscanf (temp, "%d %d %d %255s", &major, &minor, &size, device_name) == 4)
	{
		strcpy(short_path, "/dev/"); strcat( short_path, device_name );
		realpath( short_path, device_name );
	
		if ( real_path == device_name ) { 
			fclose (proc_part_file);
		  	return (Glib::ustring(short_path));
		}
		
	}
	
	//paranoia modus :)
	fclose (proc_part_file);	
	return real_path;
}

//AFAIK it's not possible to use a C++ memberfunction as a callback for a C libary function (if you know otherwise, PLEASE contact me
Glib::ustring error_message;
bool show_libparted_message = true ;
PedExceptionOption PedException_Handler (PedException* ex)
{
	error_message = ex -> message ;
	
	if ( show_libparted_message )
	{
		Gtk::Label *label_temp;
		Gtk::HBox *hbox_filler;
		
		Glib::ustring message = "<span weight=\"bold\" size=\"larger\">" + (Glib::ustring) _( "Libparted message" ) + "</span>\n\n" ;
		message += (Glib::ustring) _( "This message directly comes from libparted and has little to do with GParted. However, the message might contain useful information, so you might decide to read it.") + "\n";
		message += _( "If you don't want to see these messages any longer, it's safe to disable them.") ;
				
		Gtk::MessageDialog dialog(  message,true,Gtk::MESSAGE_WARNING , Gtk::BUTTONS_OK, true); 
		Gtk::Frame frame_libparted_message ;
		frame_libparted_message .set_size_request( 400, -1 );
		label_temp = manage( new Gtk::Label("<b>" + (Glib::ustring) _( "Libparted message") + ": </b>" ) ) ;
		label_temp ->set_use_markup( true ) ;
		frame_libparted_message .set_label_widget( *label_temp ) ;
		
		frame_libparted_message .set_border_width( 5 ) ;
		label_temp = manage( new Gtk::Label( ) ) ;
		label_temp ->set_size_request( 55, -1 ) ; //used like a simple filler ( remember this html cursus :-P )
		hbox_filler = manage( new Gtk::HBox() ) ;
		hbox_filler ->pack_start ( *label_temp, Gtk::PACK_SHRINK ) ;
		hbox_filler ->pack_start ( frame_libparted_message, Gtk::PACK_SHRINK ) ;
		
		dialog .get_vbox() ->pack_start( *hbox_filler, Gtk::PACK_SHRINK ) ;
		
		Gtk::Label label_libparted_message( "<i>" + error_message + "</i>" ) ;
		label_libparted_message .set_selectable( true ) ;
		label_libparted_message .set_use_markup( true ) ;
		label_libparted_message .set_line_wrap( true ) ;
		frame_libparted_message.add ( label_libparted_message ) ;
			
		Gtk::CheckButton checkbutton_message( _( "Hide libparted messages for this session" ) ) ;
		checkbutton_message .set_border_width( 5 ) ;
		label_temp = manage( new Gtk::Label(  ) ) ;
		label_temp ->set_size_request( 55, -1 ) ;
		hbox_filler = manage( new Gtk::HBox() ) ;
		hbox_filler ->pack_start ( *label_temp, Gtk::PACK_SHRINK ) ;
		hbox_filler ->pack_start ( checkbutton_message, Gtk::PACK_SHRINK ) ;
		
		dialog .get_vbox() ->pack_start( *hbox_filler, Gtk::PACK_SHRINK ) ;
		
		dialog .show_all_children() ;
		dialog .run();
			
		show_libparted_message = ! checkbutton_message .get_active() ;
	}
		
	return PED_EXCEPTION_UNHANDLED ;
}


namespace GParted
{

Device::Device()
{
}
	
Device::Device( const Glib::ustring & device_path )
{
	ped_exception_set_handler( PedException_Handler ) ; 
		
	this ->realpath 	=	device_path ; //this one is used by open_device_and_disk
	
	this ->length = 0;//lazy check.. if something goes wrong while reading the device, length will stay zero and i will know it ( see Win_GParted::Find_Devices )
	
	if ( ! open_device_and_disk() )
		return ;
		
	this ->model 		=	device ->model ;
	this ->path 		=	get_sym_path( device ->path ) ;
	this ->disktype 	= 	disk ->type ->name ;
	this ->heads 		=	device ->bios_geom.heads ;
	this ->sectors 	=	device ->bios_geom.sectors ;
	this ->cylinders	=	device ->bios_geom.cylinders ;
	this ->length 		=	heads * sectors * cylinders ;
	
	//get valid flags for this device
	for ( PedPartitionFlag flag = ped_partition_flag_next ( (PedPartitionFlag) 0 ) ; flag ; flag = ped_partition_flag_next ( flag ) )
		flags .push_back( flag ) ;
	
	Read_Disk_Layout() ; 
	
}

void Device::Read_Disk_Layout()
{
	//clear partitions
	this ->device_partitions .clear () ;
	
	c_partition = ped_disk_next_partition (  disk, NULL) ;
	while ( c_partition )
	{
		os << this ->path << c_partition ->num ;
			
		switch( c_partition ->type )
		{ 
			//NORMAL (PRIMARY)
			case 0 :	if ( c_partition ->fs_type )
								temp = c_partition ->fs_type ->name ;
							else
								{temp = "unknown" ; this ->error = (Glib::ustring) _( "Unable to detect filesystem! Possible reasons are:" ) + "\n-" + (Glib::ustring) _( "The filesystem is damaged" ) + "\n-"  + (Glib::ustring) _( "The filesystem is unknown to libparted" ) + "\n-" + (Glib::ustring) _( "There is no filesystem available (unformatted)" ) ; }
							
							partition_temp.Set( os.str(),c_partition ->num , GParted::PRIMARY, temp, c_partition ->geom .start, c_partition ->geom .end, Get_Used_Sectors( c_partition , os.str() ) , false, ped_partition_is_busy(  c_partition ) );
							
							partition_temp .flags = Get_Flags( c_partition ) ;									
							partition_temp .error = this ->error ;
							device_partitions.push_back( partition_temp );
											
							break;	
			//LOGICAL
			case 1:	if ( c_partition ->fs_type )
								temp = c_partition ->fs_type ->name ;
							else
								{temp = "unknown" ; this ->error = (Glib::ustring) _( "Unable to detect filesystem! Possible reasons are:" ) + "\n-" + (Glib::ustring) _( "The filesystem is damaged" ) + "\n-"  + (Glib::ustring) _( "The filesystem is unknown to libparted" ) + "\n-" + (Glib::ustring) _( "There is no filesystem available (unformatted)" ) ; }
								
							partition_temp.Set( os.str(), c_partition ->num, GParted::LOGICAL, temp, c_partition ->geom .start, c_partition ->geom .end, Get_Used_Sectors( c_partition , os.str() ) , true, ped_partition_is_busy( c_partition ) );
																
							partition_temp .flags = Get_Flags( c_partition ) ;									
							partition_temp .error = this ->error ;
							device_partitions.push_back( partition_temp );
												
							break;
			//EXTENDED
			case 2:	partition_temp.Set( os.str(), c_partition ->num, GParted::EXTENDED, "extended", c_partition ->geom .start, c_partition ->geom .end , -1, false, ped_partition_is_busy(  c_partition ) );
											
							partition_temp .flags = Get_Flags( c_partition ) ;									
							partition_temp .error = this ->error ;
							device_partitions.push_back( partition_temp );
				
							break;
			//FREESPACE OUTSIDE EXTENDED
			case 4:	if ( (c_partition ->geom .end - c_partition ->geom .start) > MEGABYTE )
							{
								partition_temp.Set_Unallocated( c_partition ->geom .start , c_partition ->geom .end, false );
								device_partitions.push_back( partition_temp );
							}
					
							break ;
			//FREESPACE INSIDE EXTENDED
			case 5:	if ( (c_partition ->geom .end - c_partition ->geom .start) > MEGABYTE )
							{
								partition_temp.Set_Unallocated( c_partition ->geom .start , c_partition ->geom .end, true );
								device_partitions.push_back( partition_temp );
							}
							
							break ;
			//METADATA  (do nothing)
			default:	break;
		}
			
		//reset stuff..
		this ->error = ""; error_message = ""; os.str("");
		
		//next partition (if any)
		c_partition = ped_disk_next_partition (  disk, c_partition ) ;
	}
	
}

bool Device::Delete_Partition( const Partition & partition ) 
{
	if ( partition .type == GParted::EXTENDED )
		c_partition = ped_disk_extended_partition( disk ) ;
	else
		c_partition = ped_disk_get_partition_by_sector( disk, (partition .sector_end + partition .sector_start) / 2 ) ;
	
	if ( ! ped_disk_delete_partition( disk, c_partition )  )
		return false;
	
	return Commit() ;
}

int Device::Create_Empty_Partition( const Partition & new_partition, PedConstraint * constraint ) 
{
	PedPartitionType type;
	PedPartition *c_part = NULL ;
	
	//create new partition
	switch ( new_partition .type )
	{
		case 0	:	type = PED_PARTITION_NORMAL; 		break;
		case 1	:	type = PED_PARTITION_LOGICAL; 		break;
		case 2	:	type = PED_PARTITION_EXTENDED; 	break;
		default	:	type = PED_PARTITION_FREESPACE;	break ; //will never happen ;)
	}
	c_part = ped_partition_new ( disk, type, NULL, new_partition .sector_start, new_partition .sector_end ) ;
	
	//create constraint
	if ( ! constraint )
		constraint = ped_constraint_any ( device);
	
	//and add the whole thingy to disk..
	if ( ! constraint || ! ped_disk_add_partition (disk, c_part, constraint) || ! Commit() )
		return 0 ;
	
	ped_constraint_destroy (constraint);
	return c_part ->num;
}

bool Device::Create_Partition_With_Filesystem( Partition & new_part, PedTimer * timer) 
{
	//no need to create a filesystem on an extended partition	
	if ( new_part .type == GParted::EXTENDED )   
		return  Create_Empty_Partition( new_part ) ;
	else
	{
		new_part .partition_number = Create_Empty_Partition( new_part ) ;
		return Set_Partition_Filesystem( new_part, timer ) ;
	}
	
}

bool Device::Move_Resize_Partition( const Partition & partition_original, const Partition & partition_new , PedTimer *timer )
{
	PedPartition *c_part = NULL ;
	PedFileSystem *fs = NULL ;
	PedConstraint *constraint = NULL ;
	
	//since resizing extended is a bit different i handle it elsewhere..
	if ( partition_new.type == GParted::EXTENDED )
		return Resize_Extended( partition_new, timer ) ;
	
	//normal partitions....
	c_part = ped_disk_get_partition_by_sector( disk, (partition_original .sector_end + partition_original .sector_start) / 2 ) ;
	if ( ! c_part )
		return false;
		
	fs = ped_file_system_open (& c_part->geom );
	if ( ! fs )
		return false ;
	
	constraint = ped_file_system_get_resize_constraint (fs);
	if ( ! constraint )
	{
		ped_file_system_close (fs);
		return false;
	}
	
	if ( ! ped_disk_set_partition_geom (disk, c_part, constraint,  partition_new .sector_start, partition_new .sector_end ) )
	{
		ped_constraint_destroy (constraint);
		ped_file_system_close (fs);
		return false ;
	}
	
	if ( ! ped_file_system_resize (fs, & c_part->geom, timer) )
	{
		ped_constraint_destroy (constraint);
		ped_file_system_close (fs);
		return false ;
	}
	
	ped_constraint_destroy (constraint);
	ped_file_system_close (fs);
	
	return Commit() ;
}

bool Device::Set_Partition_Filesystem( const Partition & new_partition, PedTimer * timer)
{
	if ( new_partition .partition_number <= 0 )
		return false ;
	
	PedPartition *c_part = NULL ;
	PedFileSystemType *fs_type = NULL ;
	PedFileSystem *fs = NULL ;
	
	c_part = ped_disk_get_partition_by_sector( disk, (new_partition .sector_end + new_partition .sector_start) / 2 ) ;
	if ( ! c_part )
		return false;
		
	fs_type = ped_file_system_type_get ( new_partition .filesystem .c_str()   ) ;
	if ( ! fs_type )
		return false;
		
	fs = ped_file_system_create ( & c_part ->geom, fs_type, timer);
	if ( ! fs )
		return false;
	
	ped_file_system_close (fs);
	
	if ( ! ped_partition_set_system (c_part, fs_type ) )
		return false;
	
	return Commit() ;
}

bool Device::Copy_Partition( Device *source_device, const Partition & source_partition, PedTimer * timer) 
{
	PedPartition *c_part_src = NULL;
	PedFileSystem *src_fs = NULL;
	PedPartition *c_part = NULL;
	PedFileSystem *dst_fs = NULL;
	PedFileSystemType*	dst_fs_type = NULL;
	
	//prepare source for reading
	if ( source_device ->Get_Path() == this ->path )
		c_part_src = ped_disk_get_partition( disk, source_partition .partition_number ) ;
	else
		c_part_src = source_device ->Get_c_partition( source_partition .partition_number ) ;
	
	if ( ! c_part_src )
		return false;
	
	src_fs = ped_file_system_open ( &c_part_src ->geom );
	if ( ! src_fs )
		return false ;
		
	//create new empty partition
	c_part = ped_disk_get_partition( disk,Create_Empty_Partition( source_partition, ped_file_system_get_copy_constraint ( src_fs, device )  )  ) ;
	if ( ! c_part )
		return false ;
	
	//and do the copy
	dst_fs = ped_file_system_copy (src_fs, &c_part ->geom, timer);
	if ( ! dst_fs )
	{
		ped_file_system_close (src_fs);
		return false ;
	}
	
	dst_fs_type = dst_fs ->type; // may be different to src_fs->type 
	
	ped_file_system_close (src_fs);
	ped_file_system_close (dst_fs);
	
	if ( !ped_partition_set_system (c_part, dst_fs_type) )
		return false ;
	
	return Commit() ;
}

bool Device::Resize_Extended( const Partition & partition , PedTimer *timer) 
{
	PedPartition *c_part = NULL ;
	PedConstraint *constraint = NULL ;
	
	c_part = ped_disk_extended_partition( disk ) ;
	if ( ! c_part )
		return false;
		
	constraint = ped_constraint_any ( device );
	if ( ! constraint )
		return false;

	if ( ! ped_disk_set_partition_geom (disk, c_part, constraint,  partition.sector_start, partition.sector_end ) )
	{
		ped_constraint_destroy (constraint);
		return false ;
	}
	
	ped_partition_set_system ( c_part, NULL);
	
	ped_constraint_destroy (constraint);
		
	return Commit() ;
}

bool Device::Commit() 
{
	bool return_value =  ped_disk_commit_to_dev( disk ) ;
	
	//i don't want this annoying "warning couldn't reread blabla" message all the time. I throw one myself if necessary )
	ped_exception_fetch_all() ;
	ped_disk_commit_to_os( disk ) ;
	ped_exception_leave_all() ;
	
	return return_value ;
}

PedPartition * Device::Get_c_partition( int part_num )
{
	return ped_disk_get_partition( disk, part_num ) ;
}

std::vector<Partition> Device::Get_Partitions()
{
	return device_partitions;
}

Sector Device::Get_Length() 
{
	return length ;
}

long Device::Get_Heads() 
{
	return heads ;
}

long Device::Get_Sectors() 
{
	return sectors ;
}

long Device:: Get_Cylinders() 
{
	return cylinders ;
}

Glib::ustring Device::Get_Model() 
{
	return model ;
}

Glib::ustring Device::Get_Path() 
{
	return path ;
}

Glib::ustring Device::Get_RealPath() 
{
	return realpath ;
}

Glib::ustring Device::Get_DiskType() 
{
	return disktype ;
}

bool Device::Get_any_busy() 
{
	for ( unsigned int t=0;t<device_partitions.size(); t++ )
		if ( device_partitions[t].busy )
			return true;
		
	return false;
}

int Device::Get_Max_Amount_Of_Primary_Partitions() 
{
	return ped_disk_get_max_primary_partition_count( disk ) ;
}

Sector Device::Get_Used_Sectors( PedPartition *c_partition, const Glib::ustring & sym_path)
{
	/* This is quite an unreliable process, atm i try two different methods, but since both are far from perfect the results are 
	 * questionable. 
	 * - first i try geometry.get_used() in libpartedpp, i implemented this function to check the minimal size when resizing a partition. Disadvantage 
     *   of this method is the fact it won't work on mounted filesystems. Besides that, its SLOW
	 * - if the former method fails ( result is -1 ) i'll try to read the output from the df command ( 	df -k --sync <partition path> )
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
	if ( ! ped_partition_is_busy( c_partition ) && (Glib::ustring) c_partition ->fs_type ->name != "ntfs" )
	{ 		
		PedFileSystem *fs = NULL;
		PedConstraint *constraint = NULL;
		
		fs = ped_file_system_open( & c_partition ->geom ); //opening a filesystem is *SLOOOWW* :-(
		
		if ( fs )
		{	
			constraint = ped_file_system_get_resize_constraint (fs);
			ped_file_system_close (fs);
			if ( constraint && constraint->min_size != -1 ) //apperantly the compiler evaluates constraint != NULL before the min_size check. Very nice! :D
				return constraint->min_size ;
		}
		else 
			this ->error = error_message ;

	}
		
	//METHOD #2
	//this ony works for mounted ( and therefore known to the OS ) filesystems. My method is quite crude, keep in mind it's only temporarely ;-)
	Glib::ustring buf;
	system( ("df -k --sync " + sym_path + " | grep " + sym_path + " > /tmp/.tmp_gparted").c_str() );
	std::ifstream file_input( "/tmp/.tmp_gparted" );
	
	file_input >> buf; //skip first value
	file_input >> buf;
	if ( buf != "0" && ! buf.empty() ) 
	{ 
		file_input >> buf;
		file_input.close(); system( "rm -f /tmp/.tmp_gparted" );		
		return atoi( buf.c_str() ) * 1024/512 ;
	}
	file_input.close(); system( "rm -f /tmp/.tmp_gparted" );
	
	
	return -1 ; //all methods were unsuccesfull

}

Glib::ustring Device::Get_Flags( PedPartition *c_partition )
{
	temp = "";
		
	for ( unsigned short t = 0; t < flags.size()  ; t++ )
		if ( ped_partition_get_flag ( c_partition,  flags[ t ] ) )
			temp += (Glib::ustring) ped_partition_flag_get_name ( flags[ t ] ) + " ";
		
	return temp ;
}

bool Device::open_device_and_disk() 
{
	device = ped_device_get( this->realpath .c_str() );
	if ( device )
		disk 	= 	ped_disk_new( device );
	
	if ( ! device || ! disk  )
	{
		if ( device ) 
			{ ped_device_destroy( device ) ;	device = NULL ; }
				
		return false;
	}
	
	return true ;
}

void Device::close_device_and_disk()
{
	if ( device )
	{
		ped_device_destroy( device ) ;
		device = NULL ;
	}
	
	if ( disk )
	{
		ped_disk_destroy( disk ) ;
		disk = NULL ;
	}
}

Device::~Device()
{
	close_device_and_disk() ;	
}


} //GParted
