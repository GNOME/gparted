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

Dialog_Partition_New::Dialog_Partition_New(  )
{
	/*TO TRANSLATORS: dialogtitle */
	this ->set_title( _("Create new Partition") ) ;
	Set_Resizer( false ) ;
	Set_Confirm_Button( NEW ) ;
	
	//set partition color
	color_temp .set( selected_partition .Get_Color( "ext2" ) ) ;
	frame_resizer_base ->set_rgb_partition_color( color_temp ) ;
	
	//set the resizer..
	frame_resizer_base ->set_x_start( 0 ) ;
	frame_resizer_base ->set_x_end( 500  ) ;
	frame_resizer_base ->set_used( 0 ) ;
 		
}

void Dialog_Partition_New::Set_Data( const Partition & partition, bool any_extended, unsigned short  new_count )
{
	this->new_count = new_count;
	this->selected_partition = partition;
	
	//add table with selection menu;s...
	table_create .set_border_width( 10 ) ;
	table_create.set_row_spacings( 5 ) ;
	hbox_main .pack_start( table_create, Gtk::PACK_SHRINK );
	
	/*TO TRANSLATORS: used as label for a list of choices.   Create as: <optionmenu with choices> */
	label_type.set_text( (Glib::ustring) _("Create as") + ":\t" );
	table_create.attach( label_type, 0,1,0,1,Gtk::SHRINK);
	
	//fill partitiontype menu
	menu_type.items().push_back(Gtk::Menu_Helpers::MenuElem( "Primary") ) ;
	menu_type.items().push_back(Gtk::Menu_Helpers::MenuElem( "Logical") ) ;
	menu_type.items().push_back(Gtk::Menu_Helpers::MenuElem( "Extended") ) ;
	
	//determine which PartitionType is allowed
	if ( partition.inside_extended )
	{
		menu_type.items()[0] . set_sensitive( false ); 
		menu_type.items()[2] . set_sensitive( false );
		menu_type.set_active( 1 );
	}
	else
	{
		menu_type.items()[1] . set_sensitive( false ); 
		if ( any_extended ) menu_type.items()[2] . set_sensitive( false );
	}
	
	optionmenu_type.set_menu( menu_type );
	optionmenu_type.set_size_request( 150, -1 ); //150 is the ideal width for this table column, (when one widget is set, the rest wil take this width as well)
	optionmenu_type.signal_changed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &Dialog_Partition_New::optionmenu_changed), true) );
	table_create.attach( optionmenu_type, 1,2,0,1,Gtk::FILL);
	
	//filesystems to choose from 
	label_filesystem.set_text( (Glib::ustring) _("Filesystem") + ":\t" );
	table_create.attach( label_filesystem, 0,1,1,2,Gtk::SHRINK);
	
	Build_Filesystems_Menu() ;
	 
	optionmenu_filesystem.set_menu( menu_filesystem );
	optionmenu_filesystem.signal_changed().connect( sigc::bind<bool>(sigc::mem_fun(*this, &Dialog_Partition_New::optionmenu_changed), false) );
	table_create.attach( optionmenu_filesystem, 1,2,1,2,Gtk::FILL);
	
	//set some widely used values...
	START = partition.sector_start ;
	total_length = partition.sector_end - partition.sector_start ;
	TOTAL_MB = this ->selected_partition .Get_Length_MB() ;
	MB_PER_PIXEL = (double) TOTAL_MB / 500    ;
	
	//set spinbuttons
	GRIP = true ; //prevents on spinbutton_changed from getting activated prematurely	
	
	spinbutton_before .set_range( 0, TOTAL_MB -1 ) ;//mind the -1  !!
	spinbutton_before .set_value( 0 ) ;
	
	spinbutton_size .set_range( 1, TOTAL_MB ) ;
	spinbutton_size .set_value( TOTAL_MB ) ;
	
	spinbutton_after .set_range( 0, TOTAL_MB  -1 ) ;//mind the -1  !!
	spinbutton_after .set_value( 0 ) ;
	
	GRIP = false ;
	
	//set contents of label_minmax
	os << _("Minimum Size") << ": " << 1 << " MB\t\t"; 
	os << _("Maximum Size") << ": " << TOTAL_MB << " MB" ; 
	label_minmax.set_text( os.str() ) ; os.str("") ;
	
	this ->show_all_children() ;
}

Partition Dialog_Partition_New::Get_New_Partition()
{
	Partition part_temp ;
	PartitionType part_type = GParted::UNALLOCATED; //paranoia ;P
	Sector new_start, new_end;
		
	switch ( optionmenu_type.get_history() )
	{
		case 0	:	part_type = GParted::PRIMARY;  	break;
		case 1	:	part_type = GParted::LOGICAL; 	break;
		case 2	:	part_type = GParted::EXTENDED; 	break;
	}
	
	new_start = START +  (Sector) (spinbutton_before .get_value() * MEGABYTE) ;
	new_end  = new_start + (Sector) (spinbutton_size .get_value() * MEGABYTE) ;
	
	//due to loss of precision during calcs from Sector -> MB and back, it is possible the new partition thinks it's bigger then it can be. Here we try to solve this.
	if ( new_start < selected_partition.sector_start )
		new_start = selected_partition.sector_start ;
	if  ( new_end > selected_partition.sector_end )
		new_end = selected_partition.sector_end ;
	
	os << "New Partition #" << new_count;
	part_temp.Set( os.str(), new_count, part_type , filesystems[ optionmenu_filesystem.get_history() ], new_start, new_end, -1,  selected_partition.inside_extended, false) ; os.str("") ;
	
	//THIS SHOULD PROBABLY BE A SETTING IN USERSPACE! ( grow new partition a bit if freespaces are < 1 MB )
	if ( (part_temp.sector_start - selected_partition.sector_start) < MEGABYTE ) 
		part_temp.sector_start = selected_partition.sector_start ;
	if ( (selected_partition.sector_end - part_temp.sector_end) < MEGABYTE ) 
		part_temp.sector_end = selected_partition.sector_end ;

	return part_temp;
}



void Dialog_Partition_New::optionmenu_changed( bool type  )
{
	//optionmenu_type
	if ( type )
	{
		if (optionmenu_type.get_history() == GParted::EXTENDED )
		{
			menu_filesystem.items().push_back(Gtk::Menu_Helpers::MenuElem( "extended") ) ;
			optionmenu_filesystem.set_history(  5  )   ;
			optionmenu_filesystem.set_sensitive( false );
		}
		else if ( menu_filesystem.items() .size() > 5 ) 
		{
			menu_filesystem.items() .remove( menu_filesystem.items() .back() );
			optionmenu_filesystem.set_sensitive( true );
			optionmenu_filesystem.set_history(  0  )  ;
		}
	
	}
	
	//optionmenu_filesystem
	if ( ! type )
	{
		selected_partition .filesystem = filesystems[ optionmenu_filesystem .get_history() ] ; //needed vor upper limit check (see also Dialog_Base_Partition::on_signal_resize )
		
		//set new spinbutton ranges
		long MIN, MAX;
		switch ( optionmenu_filesystem .get_history() )
		{
			case 1	:	MIN = 32 ;
							TOTAL_MB > 1023 ? MAX = 1023 : MAX = TOTAL_MB ;
							break;
			case 2	:	MIN = 256 ;
							MAX = TOTAL_MB ;
							break;
			default	:	MIN = 1 ;
							MAX = TOTAL_MB ;
		}
		
		spinbutton_before .set_range( 0, TOTAL_MB - MIN ) ;
		spinbutton_size .set_range( MIN, MAX ) ;
		spinbutton_after .set_range( 0, TOTAL_MB  - MIN ) ;
		
		//set contents of label_minmax
		os << _("Minimum Size") << ": " << MIN << " MB\t\t"; 
		os << _("Maximum Size") << ": " << MAX << " MB" ; 
		label_minmax.set_text( os.str() ) ; os.str("") ;
		
	}
	
	
	//set fitting resizer colors
	//backgroundcolor..
	optionmenu_type.get_history() == 2 ? color_temp .set( "darkgrey" )  : color_temp .set( "white" )  ;
	frame_resizer_base ->override_default_rgb_unused_color( color_temp );
	
	//partitioncolor..
	color_temp .set( selected_partition .Get_Color( filesystems[ optionmenu_filesystem.get_history() ]  ) ) ;
	frame_resizer_base ->set_rgb_partition_color( color_temp ) ;
	
	frame_resizer_base ->Draw_Partition() ;
}

void Dialog_Partition_New::Build_Filesystems_Menu() 
{
	//those filesystems can be created by libparted (NOTE: to create reiserfs you need libreiserfs, i'll look into that later )
	filesystems.push_back( "ext2" );
	filesystems.push_back( "fat16" );
	filesystems.push_back( "fat32" );
	filesystems.push_back( "linux-swap" );
	filesystems.push_back( "ReiserFS" ); //libreiserfs needed
	filesystems.push_back( "extended" ); //convenient ;)
	
	//fill the filesystem menu with those filesystems (except for extended) 
	for ( unsigned int t=0; t< filesystems.size() -1 ; t++ )
		menu_filesystem.items().push_back(Gtk::Menu_Helpers::MenuElem( filesystems[t] ) ) ;
	
	//check if selected unallocated is big enough for fat fs'es
	if ( this ->selected_partition .Get_Length_MB() < 32 )
		menu_filesystem.items()[ 1 ]  .set_sensitive( false ) ;
	if ( this ->selected_partition .Get_Length_MB() < 256 )
		menu_filesystem.items()[ 2 ]  .set_sensitive( false ) ;
	
	
	//disable reiserfs for the time being...
	menu_filesystem.items()[ 4 ]  .set_sensitive( false ) ;
	
}



} //GParted
