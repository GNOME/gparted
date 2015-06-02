/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
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

#include "../include/Dialog_Partition_New.h"
#include "../include/GParted_Core.h"

namespace GParted
{

Dialog_Partition_New::Dialog_Partition_New( const Device & device,
                                            const Partition & partition,
                                            bool any_extended,
                                            unsigned short new_count,
                                            const std::vector<FS> & FILESYSTEMS )
{
	/*TO TRANSLATORS: dialogtitle */
	this ->set_title( _("Create new Partition") ) ;
	Set_Resizer( false ) ;
	Set_Confirm_Button( NEW ) ;
	
	//set used (in pixels)...
	frame_resizer_base ->set_used( 0 ) ;

	set_data(device, partition, any_extended, new_count, FILESYSTEMS );
}

void Dialog_Partition_New::set_data( const Device & device,
                                     const Partition & partition,
                                     bool any_extended,
                                     unsigned short new_count,
                                     const std::vector<FS> & FILESYSTEMS )
{
	this ->new_count = new_count;
	new_partition = partition;

	// Copy only supported file systems from GParted_Core FILESYSTEMS vector.  Add
	// FS_CLEARED, FS_UNFORMATTED and FS_EXTENDED at the end.  This decides the order
	// of items in the file system menu built by Build_Filesystems_Menu().
	this->FILESYSTEMS.clear();
	for ( unsigned i = 0 ; i < FILESYSTEMS.size() ; i ++ )
	{
		if ( GParted_Core::supported_filesystem( FILESYSTEMS[i].filesystem ) )
			this->FILESYSTEMS.push_back( FILESYSTEMS[i] );
	}

	FS fs_tmp ;
	//... add FS_CLEARED
	fs_tmp .filesystem = FS_CLEARED ;
	fs_tmp .create = FS::GPARTED ;
	this ->FILESYSTEMS .push_back( fs_tmp ) ;

	//... add FS_UNFORMATTED
	fs_tmp .filesystem = GParted::FS_UNFORMATTED ;
	fs_tmp .create = FS::GPARTED ;
	this ->FILESYSTEMS .push_back( fs_tmp ) ;

	// ... finally add FS_EXTENDED.  Needed so that when creating an extended
	// partition it is identified correctly before the operation is applied.
	fs_tmp = FS();
	fs_tmp .filesystem = GParted::FS_EXTENDED ;
	fs_tmp .create = GParted::FS::NONE ;
	this ->FILESYSTEMS .push_back( fs_tmp ) ;
	
	//add table with selection menu's...
	table_create .set_border_width( 10 ) ;
	table_create .set_row_spacings( 5 ) ;
	hbox_main .pack_start( table_create, Gtk::PACK_SHRINK );
	
	/*TO TRANSLATORS: used as label for a list of choices.   Create as: <optionmenu with choices> */
	table_create .attach( * Utils::mk_label( static_cast<Glib::ustring>( _("Create as:") ) + "\t" ), 
			      0, 1, 0, 1,
			      Gtk::FILL );
	
	//fill partitiontype menu
	menu_type .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("Primary Partition") ) ) ;
	menu_type .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("Logical Partition") ) ) ;
	menu_type .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("Extended Partition") ) ) ;
	
	//determine which PartitionType is allowed
	if ( device.disktype != "msdos" && device.disktype != "dvh" )
	{
		menu_type .items()[ 1 ] .set_sensitive( false ); 
		menu_type .items()[ 2 ] .set_sensitive( false );
		menu_type .set_active( 0 );
	}
	else if ( partition .inside_extended )
	{
		menu_type .items()[ 0 ] .set_sensitive( false ); 
		menu_type .items()[ 2 ] .set_sensitive( false );
		menu_type .set_active( 1 );
	}
	else
	{
		menu_type .items()[ 1 ] .set_sensitive( false ); 
		if ( any_extended )
			menu_type .items()[ 2 ] .set_sensitive( false );
	}
	
	optionmenu_type .set_menu( menu_type );
	
	//160 is the ideal width for this table column.
	//(when one widget is set, the rest wil take this width as well)
	optionmenu_type .set_size_request( 160, -1 ); 
	
	optionmenu_type .signal_changed() .connect( 
		sigc::bind<bool>( sigc::mem_fun( *this, &Dialog_Partition_New::optionmenu_changed ), true ) );
	table_create .attach( optionmenu_type, 1, 2, 0, 1, Gtk::FILL );
	
	// Partition name
	table_create.attach( *Utils::mk_label( static_cast<Glib::ustring>( _("Partition name:") ) + "\t" ),
	                     0, 1, 1, 2, Gtk::FILL );
	// Initialise text entry box
	partition_name_entry.set_width_chars( 20 );
	partition_name_entry.set_sensitive( device.partition_naming_supported() );
	partition_name_entry.set_max_length( device.get_max_partition_name_length() );
	// Add entry box to table
	table_create .attach( partition_name_entry, 1, 2, 1, 2, Gtk::FILL );

	//file systems to choose from 
	table_create .attach( * Utils::mk_label( static_cast<Glib::ustring>( _("File system:") ) + "\t" ),
	                      0, 1, 2, 3, Gtk::FILL );

	Build_Filesystems_Menu( device.readonly );
	 
	optionmenu_filesystem .set_menu( menu_filesystem );
	optionmenu_filesystem .signal_changed() .connect( 
		sigc::bind<bool>( sigc::mem_fun( *this, &Dialog_Partition_New::optionmenu_changed ), false ) );
	table_create .attach( optionmenu_filesystem, 1, 2, 2, 3, Gtk::FILL );

	//Label
	table_create .attach( * Utils::mk_label( Glib::ustring( _("Label:") ) ),
	                      0, 1, 3, 4, Gtk::FILL );
	//Create Text entry box
	filesystem_label_entry.set_width_chars( 20 );
	//Add entry box to table
	table_create.attach( filesystem_label_entry, 1, 2, 3, 4, Gtk::FILL );

	//set some widely used values...
	MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record( new_partition );
	START = partition.sector_start ;
	total_length = partition.sector_end - partition.sector_start ;
	TOTAL_MB = Utils::round( Utils::sector_to_unit( new_partition.get_sector_length(), new_partition.sector_size, UNIT_MIB ) );
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;
	
	//set first enabled file system
	optionmenu_filesystem .set_history( first_creatable_fs ) ;
	optionmenu_changed( false ) ;
	
	//set spinbuttons initial values
	spinbutton_after .set_value( 0 ) ;
	spinbutton_size .set_value( ceil( fs .MAX / double(MEBIBYTE) ) ) ; 
	spinbutton_before .set_value( MIN_SPACE_BEFORE_MB ) ;
	
	//Disable resizing when the total area is less than two mebibytes
	if ( TOTAL_MB < 2 )
		frame_resizer_base ->set_sensitive( false ) ;

	//connect signal handler for Dialog_Base_Partiton optionmenu_alignment
	optionmenu_alignment .signal_changed() .connect( 
		sigc::bind<bool>( sigc::mem_fun( *this, &Dialog_Partition_New::optionmenu_changed ), false ) );

	this ->show_all_children() ;
}

Partition Dialog_Partition_New::Get_New_Partition( Byte_Value sector_size )
{
	Partition part_temp ;
	PartitionType part_type ;
	Sector new_start, new_end;
		
	switch ( optionmenu_type .get_history() )
	{
		case 0	:	part_type = GParted::TYPE_PRIMARY;  break;
		case 1	:	part_type = GParted::TYPE_LOGICAL;  break;
		case 2	:	part_type = GParted::TYPE_EXTENDED; break;

		default	:	part_type = GParted::TYPE_UNALLOCATED ;
	}

	//FIXME:  Partition size is limited to just less than 1024 TeraBytes due
	//        to the maximum value of signed 4 byte integer.
	new_start = START + (Sector(spinbutton_before .get_value_as_int()) * (MEBIBYTE / sector_size)) ;
	new_end  = new_start + (Sector(spinbutton_size .get_value_as_int()) * (MEBIBYTE / sector_size)) - 1 ;
	
	/* due to loss of precision during calcs from Sector -> MiB and back, it is possible the new 
	 * partition thinks it's bigger then it can be. Here we try to solve this.*/
	if ( new_start < new_partition.sector_start )
		new_start = new_partition.sector_start;
	if  ( new_end > new_partition.sector_end )
		new_end = new_partition.sector_end;

	part_temp .status = GParted::STAT_NEW ;
	part_temp.Set( new_partition.device_path,
	               String::ucompose( _("New Partition #%1"), new_count ),
	               new_count, part_type, new_partition.whole_device,
	               FILESYSTEMS[optionmenu_filesystem.get_history()].filesystem,
	               new_start, new_end,
	               sector_size,
	               new_partition.inside_extended, false );

	// Retrieve partition name
	part_temp.name = Utils::trim( partition_name_entry.get_text() );

	//Retrieve Label info
	part_temp.set_filesystem_label( Utils::trim( filesystem_label_entry.get_text() ) );
	
	//grow new partition a bit if freespaces are < 1 MiB
	if ( (part_temp.sector_start - new_partition.sector_start) < (MEBIBYTE / sector_size) )
		part_temp.sector_start = new_partition.sector_start;
	if ( (new_partition.sector_end - part_temp.sector_end) < (MEBIBYTE / sector_size) )
		part_temp.sector_end = new_partition.sector_end;
	
	//if new is extended...
	if ( part_temp .type == GParted::TYPE_EXTENDED )
	{
		Partition UNALLOCATED ;
		UNALLOCATED.Set_Unallocated( part_temp.device_path,
		                             part_temp.whole_device,
		                             part_temp.sector_start,
		                             part_temp.sector_end,
		                             sector_size,
		                             true );
		part_temp .logicals .push_back( UNALLOCATED ) ;
	}

	//set alignment
	switch ( optionmenu_alignment .get_history() )
	{
		case  0 :  part_temp .alignment = GParted::ALIGN_CYLINDER;  break;
		case  1 :  part_temp .alignment = GParted::ALIGN_MEBIBYTE;
		           {
		               //if start sector not MiB aligned and free space available then add ~1 MiB to partition so requested size is kept
		               Sector diff = ( MEBIBYTE / part_temp .sector_size ) - ( part_temp .sector_end + 1 ) % ( MEBIBYTE / part_temp .sector_size ) ;
		               if (   diff
		                   && ( part_temp .sector_start % (MEBIBYTE / part_temp .sector_size ) ) > 0
		                   && ( ( part_temp .sector_end - START +1 + diff ) < total_length )
		                  )
		                   part_temp .sector_end += diff ;
		           }
		           break;
		case  2 :  part_temp .alignment = GParted::ALIGN_STRICT;  break;

		default :  part_temp .alignment = GParted::ALIGN_MEBIBYTE ;
	}

	part_temp .free_space_before = Sector(spinbutton_before .get_value_as_int()) * (MEBIBYTE / sector_size) ;

	return part_temp;
}

void Dialog_Partition_New::optionmenu_changed( bool type )
{
	//optionmenu_type
	if ( type )
	{
		if ( optionmenu_type .get_history() == GParted::TYPE_EXTENDED &&
		     menu_filesystem .items() .size() < FILESYSTEMS .size() )
		{
			menu_filesystem .items() .push_back( 
				Gtk::Menu_Helpers::MenuElem( Utils::get_filesystem_string( GParted::FS_EXTENDED ) ) ) ;
			optionmenu_filesystem .set_history( menu_filesystem .items() .size() -1 ) ;
			optionmenu_filesystem .set_sensitive( false ) ;
		}
		else if ( optionmenu_type .get_history() != GParted::TYPE_EXTENDED &&
			  menu_filesystem .items() .size() == FILESYSTEMS .size() ) 
		{
			menu_filesystem .items() .remove( menu_filesystem .items() .back() ) ;
			optionmenu_filesystem .set_sensitive( true ) ;
			optionmenu_filesystem .set_history( first_creatable_fs ) ;
		}
	}
	
	//optionmenu_filesystem and optionmenu_alignment
	if ( ! type )
	{
		fs = FILESYSTEMS[ optionmenu_filesystem .get_history() ] ;

		if ( fs .MIN < MEBIBYTE )
			fs .MIN = MEBIBYTE ;

		if ( new_partition.get_byte_length() < fs.MIN )
			fs .MIN = new_partition.get_byte_length();

		if ( ! fs .MAX || ( fs .MAX > ((TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE) ) )
			fs .MAX = ((TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE) ;

		frame_resizer_base ->set_x_min_space_before( Utils::round( MIN_SPACE_BEFORE_MB / MB_PER_PIXEL ) ) ;
		frame_resizer_base ->set_size_limits( Utils::round( fs .MIN / (MB_PER_PIXEL * MEBIBYTE) ),
						      Utils::round( fs .MAX / (MB_PER_PIXEL * MEBIBYTE) ) ) ;

		//set new spinbutton ranges
		spinbutton_before .set_range( MIN_SPACE_BEFORE_MB
		                            , TOTAL_MB - ceil( fs .MIN / double(MEBIBYTE) )
		                            ) ;
		spinbutton_size .set_range( ceil( fs .MIN / double(MEBIBYTE) )
		                          , ceil( fs .MAX / double(MEBIBYTE) )
		                          ) ;
		spinbutton_after .set_range( 0
		                           , TOTAL_MB - MIN_SPACE_BEFORE_MB
		                             - ceil( fs .MIN / double(MEBIBYTE) )
		                           ) ;

		//set contents of label_minmax
		Set_MinMax_Text( ceil( fs .MIN / double(MEBIBYTE) )
		               , ceil( fs .MAX / double(MEBIBYTE) )
		               ) ;
	}

	//set fitting resizer colors
	{
		Gdk::Color color_temp;
		//Background color
		color_temp.set((optionmenu_type.get_history() == 2) ? "darkgrey" : "white");
		frame_resizer_base->override_default_rgb_unused_color(color_temp);

		//Partition color
		color_temp.set(Utils::get_color(fs.filesystem));
		frame_resizer_base->set_rgb_partition_color(color_temp);
	}

	// Maximum length of the file system label varies according to the selected file system type.
	filesystem_label_entry.set_max_length( Utils::get_filesystem_label_maxlength( fs.filesystem ) );

	frame_resizer_base ->Draw_Partition() ;
}

void Dialog_Partition_New::Build_Filesystems_Menu( bool only_unformatted ) 
{
	bool set_first=false;
	//fill the file system menu with the file systems (except for extended) 
	for ( unsigned int t = 0 ; t < FILESYSTEMS .size( ) ; t++ ) 
	{
		//skip extended
		if( FILESYSTEMS[ t ] .filesystem == GParted::FS_EXTENDED )
			continue ;
		menu_filesystem .items() .push_back( 
			Gtk::Menu_Helpers::MenuElem( Utils::get_filesystem_string( FILESYSTEMS[ t ] .filesystem ) ) ) ;
		menu_filesystem .items() .back() .set_sensitive(
			! only_unformatted && FILESYSTEMS[ t ] .create &&
			new_partition.get_byte_length() >= FILESYSTEMS[t].MIN );
		//use ext4/3/2 as first/second/third choice default file system
		//(Depends on ordering in FILESYSTEMS for preference)
		if ( ( FILESYSTEMS[ t ] .filesystem == FS_EXT2 ||
		       FILESYSTEMS[ t ] .filesystem == FS_EXT3 ||
		       FILESYSTEMS[ t ] .filesystem == FS_EXT4    ) &&
		     menu_filesystem .items() .back() .sensitive()     )
		{
			first_creatable_fs=menu_filesystem .items() .size() - 1;
			set_first=true;
		}
	}
	
	//unformatted is always available
	menu_filesystem .items() .back() .set_sensitive( true ) ;
	
	if(!set_first)
	{
		//find and set first enabled file system as last choice default
		for ( unsigned int t = 0 ; t < menu_filesystem .items() .size() ; t++ )
			if ( menu_filesystem .items()[ t ] .sensitive() )
			{
				first_creatable_fs = t ;
				break ;
			}
	}
}

} //GParted
