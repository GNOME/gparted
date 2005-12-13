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
 
#include "../include/Dialog_Base_Partition.h"

namespace GParted
{
	
Dialog_Base_Partition::Dialog_Base_Partition( )
{
	this ->set_has_separator( false ) ;
	
	frame_resizer_base = NULL;
	GRIP = false ;
	this ->fixed_start = false ;
	this ->set_resizable( false );
	ORIG_BEFORE = ORIG_SIZE = ORIG_AFTER = -1 ;
	
	//pack resizer hbox
	this ->get_vbox( ) ->pack_start( hbox_resizer, Gtk::PACK_SHRINK );
	
	//add label_minmax
	this ->get_vbox( ) ->pack_start( label_minmax, Gtk::PACK_SHRINK );
	
	//pack hbox_main
	this ->get_vbox( ) ->pack_start( hbox_main, Gtk::PACK_SHRINK );
	
	//put the vbox with resizer stuff (cool widget and spinbuttons) in the hbox_main
	hbox_main .pack_start( vbox_resize_move, Gtk::PACK_EXPAND_PADDING );
	
	//fill table
	table_resize .set_border_width( 5 ) ;
	table_resize .set_row_spacings( 5 ) ;
	hbox_table.pack_start( table_resize, Gtk::PACK_EXPAND_PADDING ) ;
	
	hbox_table .set_border_width( 5 ) ;
	vbox_resize_move .pack_start( hbox_table, Gtk::PACK_SHRINK );
	
	//add spinbutton_before
	table_resize.attach( * Utils::mk_label( (Glib::ustring) _( "Free Space Preceding (MB):") + " \t" ), 0, 1, 0, 1, Gtk::SHRINK );
		
	spinbutton_before .set_numeric( true );
	spinbutton_before .set_increments( 1, 100 );
	table_resize.attach( spinbutton_before, 1, 2, 0, 1, Gtk::FILL );
	
	//add spinbutton_size
	table_resize.attach( * Utils::mk_label( _( "New Size (MB):" ) ), 0, 1, 1, 2 );

	spinbutton_size .set_numeric( true );
	spinbutton_size .set_increments( 1, 100 );
	table_resize.attach( spinbutton_size, 1, 2, 1, 2, Gtk::FILL );
	
	//add spinbutton_after
	table_resize.attach( * Utils::mk_label( _( "Free Space Following (MB):") ), 0, 1, 2, 3 ) ;
	
	spinbutton_after .set_numeric( true );
	spinbutton_after .set_increments( 1, 100 );
	table_resize.attach( spinbutton_after, 1, 2, 2, 3, Gtk::FILL );
	
	if ( ! fixed_start )
		before_value = spinbutton_before .get_value( ) ;
	
	//connect signalhandlers of the spinbuttons
	if ( ! fixed_start )
		spinbutton_before .signal_value_changed( ) .connect( sigc::bind<SPINBUTTON>( sigc::mem_fun( *this, &Dialog_Base_Partition::on_spinbutton_value_changed), BEFORE) ) ;
	
	spinbutton_size .signal_value_changed( ) .connect( sigc::bind<SPINBUTTON>( sigc::mem_fun( *this, &Dialog_Base_Partition::on_spinbutton_value_changed), SIZE) ) ;
	spinbutton_after .signal_value_changed( ) .connect( sigc::bind<SPINBUTTON>( sigc::mem_fun( *this, &Dialog_Base_Partition::on_spinbutton_value_changed), AFTER) ) ;
	
	//pack warning about small differences in values..
	this ->get_vbox( ) ->pack_start( * Utils::mk_label( "\n <i>" + (Glib::ustring) _( "NOTE: values on disk may differ slightly from the values entered here.") + "</i>" ), Gtk::PACK_SHRINK );
	
	this ->get_vbox( ) ->pack_start( * Utils::mk_label( "" ), Gtk::PACK_SHRINK ); //filler :-P
		
	this->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	this ->show_all_children( ) ;
}

void Dialog_Base_Partition::Set_Resizer( bool extended )
{
	if ( extended )
		frame_resizer_base = new Frame_Resizer_Extended( ) ;
	else
	{
		frame_resizer_base = new Frame_Resizer_Base( ) ;
		frame_resizer_base ->signal_move .connect( sigc::mem_fun( this, &Dialog_Base_Partition::on_signal_move ) );
	}
	
	frame_resizer_base ->set_border_width( 5 ) ;
	frame_resizer_base ->set_shadow_type( Gtk::SHADOW_ETCHED_OUT );
		
	//connect signals
	frame_resizer_base ->signal_resize .connect( sigc::mem_fun( this, &Dialog_Base_Partition::on_signal_resize ) );
		
	hbox_resizer .pack_start( *frame_resizer_base, Gtk::PACK_EXPAND_PADDING );
	
	this ->show_all_children( ) ;
}

Partition Dialog_Base_Partition::Get_New_Partition( ) 
{ 	
	if ( ORIG_BEFORE != spinbutton_before .get_value_as_int( ) )
		selected_partition .sector_start = START + spinbutton_before .get_value_as_int( ) * MEGABYTE ;	
	
	if ( ORIG_AFTER != spinbutton_after .get_value_as_int( ) )
		selected_partition .sector_end = selected_partition .sector_start + spinbutton_size .get_value_as_int( ) * MEGABYTE ;

	//due to loss of precision during calcs from Sector -> MB and back, it is possible the new partition thinks it's bigger then it can be. Here we solve this.
	if ( selected_partition .sector_start < START )
		selected_partition .sector_start = START ;
	if ( selected_partition .sector_end > (START + total_length) ) 
		selected_partition .sector_end = START + total_length ;
	
	//grow a bit into small freespace ( < 1MB ) 
	if (  (selected_partition .sector_start - START) < MEGABYTE )
		selected_partition .sector_start = START ;
	if ( ( START + total_length - selected_partition .sector_end ) < MEGABYTE )
		selected_partition .sector_end = START + total_length ;
	
	//set new value of unused..
	if ( selected_partition .sectors_used != -1 )
		selected_partition .sectors_unused = ( selected_partition .sector_end - selected_partition .sector_start) - selected_partition .sectors_used ;
	
	return selected_partition ;
}

void Dialog_Base_Partition::Set_Confirm_Button( CONFIRMBUTTON button_type ) 
{ 
	switch( button_type )
	{
		case NEW	:	this ->add_button( Gtk::Stock::ADD, Gtk::RESPONSE_OK );
					break ;
		case RESIZE_MOVE:	image_temp = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON ) );
					hbox_resize_move .pack_start( *image_temp, Gtk::PACK_EXPAND_PADDING ) ;
					hbox_resize_move .pack_start( * Utils::mk_label( fixed_start ? _("Resize") : _("Resize/Move") ), Gtk::PACK_EXPAND_PADDING ) ;
					button_resize_move .add( hbox_resize_move ) ;
														
					this ->add_action_widget ( button_resize_move, Gtk::RESPONSE_OK ) ;
					button_resize_move .set_sensitive( false ) ;
					break ;
			
		case PASTE	:	this ->add_button( Gtk::Stock::PASTE, Gtk::RESPONSE_OK );
					break ;
	}
}

void Dialog_Base_Partition::Set_MinMax_Text( long min, long max )
{
	str_temp = String::ucompose( _("Minimum Size: %1 MB"), min ) + "\t\t" ;
	str_temp += String::ucompose( _("Maximum Size: %1 MB"), max ) ;
	label_minmax .set_text( str_temp ) ; 
}

void Dialog_Base_Partition::on_signal_move( int x_start, int x_end )
{  
	GRIP = true ;

	spinbutton_before .set_value( x_start == 0 ? 0 : x_start * MB_PER_PIXEL ) ;
	
	if ( x_end == 500 )
	{
		spinbutton_after .set_value( 0 ) ;
		spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value( ) ) ;
	}
	else
		spinbutton_after .set_value( TOTAL_MB - spinbutton_before .get_value( ) - spinbutton_size .get_value( ) ) ;
	
	Check_Change( ) ;
	
	GRIP = false ;
}

void Dialog_Base_Partition::on_signal_resize( int x_start, int x_end, Frame_Resizer_Base::ArrowType arrow )
{  
	GRIP = true ;
		
	spinbutton_size .set_value( ( x_end - x_start ) * MB_PER_PIXEL ) ;
	
	fixed_start ? before_value = 0 : before_value = spinbutton_before .get_value( ) ;
	
	if ( arrow == Frame_Resizer_Base::ARROW_RIGHT ) //don't touch freespace before, leave it as it is
	{
		if ( x_end == 500 )
		{
			spinbutton_after .set_value( 0 ) ;
			spinbutton_size .set_value( TOTAL_MB - before_value ) ;
		}
		else
			spinbutton_after .set_value( TOTAL_MB - before_value - spinbutton_size .get_value( ) ) ;
	}
	else if ( arrow == Frame_Resizer_Base::ARROW_LEFT ) //don't touch freespace after, leave it as it is
	{
		if ( x_start == 0 )
		{
			spinbutton_before .set_value( 0 );
			spinbutton_size .set_value( TOTAL_MB - spinbutton_after.get_value( ) ) ;
		}
		else
			spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value( ) - spinbutton_after .get_value( ) ) ;
	}
		
	Check_Change( ) ;
	
	GRIP = false ;
}

void Dialog_Base_Partition::on_spinbutton_value_changed( SPINBUTTON spinbutton )
{  
	if ( ! GRIP )
	{
		fixed_start ? before_value = 0 : before_value = spinbutton_before .get_value( ) ;
			
		//Balance the spinbuttons
		switch ( spinbutton )
		{
			case BEFORE	:	spinbutton_after .set_value( TOTAL_MB - spinbutton_size .get_value( ) - before_value) ;
						spinbutton_size .set_value( TOTAL_MB - before_value - spinbutton_after .get_value( ) ) ;
							
						break;
			case SIZE	:	spinbutton_after .set_value( TOTAL_MB - before_value - spinbutton_size .get_value( ) );
						if ( ! fixed_start )
							spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value( ) - spinbutton_after .get_value( ) );
				
						break;
			case AFTER	:	if ( ! fixed_start )
							spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value( ) - spinbutton_after .get_value( ) );
			
						spinbutton_size .set_value( TOTAL_MB - before_value - spinbutton_after .get_value( ) ) ;
				
						break;
		}
			
		
		//And apply the changes to the visual view...
		if ( ! fixed_start )
			frame_resizer_base ->set_x_start( Utils::Round( spinbutton_before .get_value( ) / MB_PER_PIXEL ) ) ;
		
		frame_resizer_base ->set_x_end( 500 - Utils::Round( spinbutton_after .get_value( ) / MB_PER_PIXEL ) ) ;
		
		frame_resizer_base ->Draw_Partition( ) ;
		
		Check_Change( ) ;
	}
}

void Dialog_Base_Partition::Check_Change( )
{
	if ( 	ORIG_BEFORE	 == spinbutton_before .get_value_as_int( )	&&
		ORIG_SIZE	 == spinbutton_size .get_value_as_int( )	&& 
		ORIG_AFTER	 == spinbutton_after .get_value_as_int( ) 
	)
		button_resize_move .set_sensitive( false ) ;
	else
		button_resize_move .set_sensitive( true ) ;
}

Dialog_Base_Partition::~Dialog_Base_Partition() 
{
	if ( frame_resizer_base )
		delete frame_resizer_base ;
}

} //GParted
