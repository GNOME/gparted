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

#include "../include/Dialog_Partition_New.h"

namespace GParted
{

Dialog_Partition_New::Dialog_Partition_New( )
{
	/*TO TRANSLATORS: dialogtitle */
	this ->set_title( _("Create new Partition") ) ;
	Set_Resizer( false ) ;
	Set_Confirm_Button( NEW ) ;
	
	//set partition color
	color_temp .set( Get_Color( "ext2" ) ) ;
	frame_resizer_base ->set_rgb_partition_color( color_temp ) ;
	
	//set the resizer..
	frame_resizer_base ->set_x_start( 0 ) ;
	frame_resizer_base ->set_x_end( 500 ) ;
	frame_resizer_base ->set_used( 0 ) ;
}

void Dialog_Partition_New::Set_Data( const Partition & partition, bool any_extended, unsigned short new_count, const std::vector <FS> & FILESYSTEMS, bool only_unformatted )
{
	this ->new_count = new_count;
	this ->selected_partition = partition;
	this ->FILESYSTEMS = FILESYSTEMS ;
	this ->FILESYSTEMS .back( ) .filesystem = _("Unformatted") ;
	this ->FILESYSTEMS .back( ) .create = true ;
	
	FS fs ; fs.filesystem = "extended" ;
	this ->FILESYSTEMS .push_back( fs ) ;
	
	//add table with selection menu;s...
	table_create .set_border_width( 10 ) ;
	table_create.set_row_spacings( 5 ) ;
	hbox_main .pack_start( table_create, Gtk::PACK_SHRINK );
	
	/*TO TRANSLATORS: used as label for a list of choices.   Create as: <optionmenu with choices> */
	table_create.attach( * mk_label( (Glib::ustring) _("Create as:") + "\t" ), 0, 1, 0, 1, Gtk::FILL);
	
	//fill partitiontype menu
	menu_type.items().push_back(Gtk::Menu_Helpers::MenuElem( _("Primary Partition") ) ) ;
	menu_type.items().push_back(Gtk::Menu_Helpers::MenuElem( _("Logical Partition") ) ) ;
	menu_type.items().push_back(Gtk::Menu_Helpers::MenuElem( _("Extended Partition") ) ) ;
	
	//determine which PartitionType is allowed
	if ( partition.inside_extended )
	{
		menu_type.items()[0] .set_sensitive( false ); 
		menu_type.items()[2] .set_sensitive( false );
		menu_type.set_active( 1 );
	}
	else
	{
		menu_type.items()[1] .set_sensitive( false ); 
		if ( any_extended )
			menu_type.items()[2] .set_sensitive( false );
	}
	
	optionmenu_type.set_menu( menu_type );
	optionmenu_type.set_size_request( 160, -1 ); //160 is the ideal width for this table column, (when one widget is set, the rest wil take this width as well)
	optionmenu_type.signal_changed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &Dialog_Partition_New::optionmenu_changed), true) );
	table_create.attach( optionmenu_type, 1,2,0,1,Gtk::FILL);
	
	//filesystems to choose from 
	table_create.attach( * mk_label( (Glib::ustring) _("Filesystem:") + "\t" ), 0,1,1,2,Gtk::FILL);
	
	Build_Filesystems_Menu( only_unformatted ) ;
	 
	optionmenu_filesystem .set_menu( menu_filesystem );
	optionmenu_filesystem .signal_changed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &Dialog_Partition_New::optionmenu_changed), false) );
	table_create.attach( optionmenu_filesystem, 1,2,1,2,Gtk::FILL);
	
	//set some widely used values...
	START = partition.sector_start ;
	total_length = partition.sector_end - partition.sector_start ;
	TOTAL_MB = this ->selected_partition .Get_Length_MB() ;
	MB_PER_PIXEL = (double) TOTAL_MB / 500 ;
	
	//set spinbuttons initial values( seems a bit redundant )
	GRIP = true ; //prevents on spinbutton_changed from getting activated prematurely	
	
	spinbutton_before .set_range( 0, TOTAL_MB -1 ) ;
	spinbutton_before .set_value( 0 ) ;
	
	spinbutton_size .set_range( 1, TOTAL_MB ) ;
	spinbutton_size .set_value( TOTAL_MB ) ;
	
	spinbutton_after .set_range( 0, TOTAL_MB -1 ) ;
	spinbutton_after .set_value( 0 ) ;
	
	GRIP = false ;
	
	//set first enabled filesystem
	optionmenu_filesystem .set_history( first_creatable_fs ) ;
	optionmenu_changed( false ) ;
		
	this ->show_all_children( ) ;
}

Partition Dialog_Partition_New::Get_New_Partition()
{
	Partition part_temp ;
	PartitionType part_type = GParted::UNALLOCATED; //paranoia ;P
	Sector new_start, new_end;
		
	switch ( optionmenu_type .get_history( ) )
	{
		case 0:	part_type = GParted::PRIMARY;  break;
		case 1:	part_type = GParted::LOGICAL;  break;
		case 2:	part_type = GParted::EXTENDED; break;
	}
	
	new_start = START + (Sector) (spinbutton_before .get_value( ) * MEGABYTE) ;
	new_end  = new_start + (Sector) (spinbutton_size .get_value( ) * MEGABYTE) ;
	
	//due to loss of precision during calcs from Sector -> MB and back, it is possible the new partition thinks it's bigger then it can be. Here we try to solve this.
	if ( new_start < selected_partition.sector_start )
		new_start = selected_partition.sector_start ;
	if  ( new_end > selected_partition.sector_end )
		new_end = selected_partition.sector_end ;
	
	part_temp .status = GParted::STAT_NEW ;
	part_temp .Set(	String::ucompose( _("New Partition #%1"), new_count ),
			new_count, part_type,
			FILESYSTEMS[ optionmenu_filesystem .get_history( ) ] .filesystem,
			new_start, new_end, 
			selected_partition .inside_extended, false) ; 
	
	//grow new partition a bit if freespaces are < 1 MB
	if ( (part_temp.sector_start - selected_partition.sector_start) < MEGABYTE ) 
		part_temp.sector_start = selected_partition.sector_start ;
	if ( (selected_partition.sector_end - part_temp.sector_end) < MEGABYTE ) 
		part_temp.sector_end = selected_partition.sector_end ;
	
	//if new is extended...
	if ( part_temp .type == GParted::EXTENDED )
	{
		Partition UNALLOCATED ;
		UNALLOCATED .Set_Unallocated( part_temp .sector_start, part_temp .sector_end, true ) ;
		part_temp .logicals .push_back( UNALLOCATED ) ;
	}

	return part_temp;
}

void Dialog_Partition_New::optionmenu_changed( bool type )
{
	//optionmenu_type
	if ( type )
	{
		if ( optionmenu_type .get_history( ) == GParted::EXTENDED && menu_filesystem .items( ) .size( ) < FILESYSTEMS .size( ) )
		{
			menu_filesystem .items( ) .push_back( Gtk::Menu_Helpers::MenuElem( "extended" ) ) ;
			optionmenu_filesystem .set_history( menu_filesystem .items( ) .size( ) -1 ) ;
			optionmenu_filesystem .set_sensitive( false ) ;
		}
		else if ( optionmenu_type .get_history( ) != GParted::EXTENDED && menu_filesystem .items( ) .size( ) == FILESYSTEMS .size( ) ) 
		{
			menu_filesystem .items( ) .remove( menu_filesystem .items( ) .back( ) ) ;
			optionmenu_filesystem .set_sensitive( true ) ;
			optionmenu_filesystem .set_history( first_creatable_fs ) ;
		}
	}
	
	//optionmenu_filesystem
	if ( ! type )
	{
		//needed vor upper limit check (see also Dialog_Base_Partition::on_signal_resize )
		selected_partition .filesystem = FILESYSTEMS[ optionmenu_filesystem .get_history( ) ] .filesystem ; 
		
		//set new spinbutton ranges
		long MIN, MAX;
		switch ( optionmenu_filesystem .get_history() )
		{
			case 2:		MIN = 32 ;
					TOTAL_MB > 1023 ? MAX = 1023 : MAX = TOTAL_MB ;
					break;
			case 3:		MIN = 256 ;
					MAX = TOTAL_MB ;
					break;
			case 5:		MIN = 40 ;
					MAX = TOTAL_MB ;
					break;
			default:	MIN = 1 ;
					MAX = TOTAL_MB ;
		}
		
		spinbutton_before .set_range( 0, TOTAL_MB - MIN ) ;
		spinbutton_size .set_range( MIN, MAX ) ;
		spinbutton_after .set_range( 0, TOTAL_MB  - MIN ) ;
		
		//set contents of label_minmax
		Set_MinMax_Text( MIN, MAX ) ;
	}
	
	//set fitting resizer colors
	//backgroundcolor..
	optionmenu_type.get_history() == 2 ? color_temp .set( "darkgrey" ) : color_temp .set( "white" ) ;
	frame_resizer_base ->override_default_rgb_unused_color( color_temp );
	
	//partitioncolor..
	color_temp .set( Get_Color( FILESYSTEMS[ optionmenu_filesystem.get_history() ] .filesystem ) ) ;
	frame_resizer_base ->set_rgb_partition_color( color_temp ) ;
	
	if ( this ->is_realized( ) )
		frame_resizer_base ->Draw_Partition( ) ;
}

void Dialog_Partition_New::Build_Filesystems_Menu( bool only_unformatted ) 
{
	//fill the filesystem menu with the filesystems (except for extended) 
	for ( unsigned int t = 0 ; t < FILESYSTEMS .size( ) -1 ; t++ ) 
	{
		menu_filesystem .items( ) .push_back( Gtk::Menu_Helpers::MenuElem( FILESYSTEMS[ t ] .filesystem ) ) ;
		menu_filesystem .items( )[ t ] .set_sensitive( FILESYSTEMS[ t ] .create && ! only_unformatted ) ;	
	}
	
	//unformatted is always available
	menu_filesystem .items( ) .back( ) .set_sensitive( true ) ;
	
	//find and set first enabled filesystem
	for ( unsigned int t = 0 ; t < menu_filesystem .items( ) .size( ) ; t++ )
		if ( menu_filesystem .items( )[ t ] .sensitive( ) )
		{
			first_creatable_fs = t ;
			break ;
		}
		
	//check if selected unallocated is big enough for fs'es with min. size
	//fat16
	if ( this ->selected_partition .Get_Length_MB() < 32 )
		menu_filesystem .items()[ 2 ] .set_sensitive( false ) ;
	//fat32
	if ( this ->selected_partition .Get_Length_MB() < 256 )
		menu_filesystem .items()[ 3 ] .set_sensitive( false ) ;
	//reiserfs
	if ( this ->selected_partition .Get_Length_MB() < 40 )
		menu_filesystem .items()[ 5 ] .set_sensitive( false ) ;
	
}



} //GParted
