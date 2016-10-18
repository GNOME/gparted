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

#include "Dialog_Base_Partition.h"
#include "Partition.h"

namespace GParted
{
	
Dialog_Base_Partition::Dialog_Base_Partition()
{
	this ->set_has_separator( false ) ;
	frame_resizer_base = NULL;
	GRIP = false ;
	this ->fixed_start = false ;
	this ->set_resizable( false );
	ORIG_BEFORE = ORIG_SIZE = ORIG_AFTER = -1 ;
	MIN_SPACE_BEFORE_MB = -1 ;
	
	//pack resizer hbox
	this ->get_vbox() ->pack_start( hbox_resizer, Gtk::PACK_SHRINK );
	
	//add label_minmax
	this ->get_vbox() ->pack_start( label_minmax, Gtk::PACK_SHRINK );
	
	//pack hbox_main
	this ->get_vbox() ->pack_start( hbox_main, Gtk::PACK_SHRINK );

	//put the vbox with resizer stuff (cool widget and spinbuttons) in the hbox_main
	hbox_main .pack_start( vbox_resize_move, Gtk::PACK_EXPAND_PADDING );
	
	//fill table
	table_resize .set_border_width( 5 ) ;
	table_resize .set_row_spacings( 5 ) ;
	hbox_table.pack_start( table_resize, Gtk::PACK_EXPAND_PADDING ) ;
	
	hbox_table .set_border_width( 5 ) ;
	vbox_resize_move .pack_start( hbox_table, Gtk::PACK_SHRINK );
	
	//add spinbutton_before
	table_resize .attach(
		* Utils::mk_label( static_cast<Glib::ustring>( _( "Free space preceding (MiB):") ) + " \t" ),
		0, 1, 0, 1,
		Gtk::SHRINK );
		
	spinbutton_before .set_numeric( true );
	spinbutton_before .set_increments( 1, 100 );
	table_resize.attach( spinbutton_before, 1, 2, 0, 1, Gtk::FILL );
	
	//add spinbutton_size
	table_resize.attach( * Utils::mk_label( _( "New size (MiB):" ) ), 0, 1, 1, 2 );

	spinbutton_size .set_numeric( true );
	spinbutton_size .set_increments( 1, 100 );
	table_resize.attach( spinbutton_size, 1, 2, 1, 2, Gtk::FILL );
	
	//add spinbutton_after
	table_resize.attach( * Utils::mk_label( _( "Free space following (MiB):") ), 0, 1, 2, 3 ) ;
	
	spinbutton_after .set_numeric( true );
	spinbutton_after .set_increments( 1, 100 );
	table_resize.attach( spinbutton_after, 1, 2, 2, 3, Gtk::FILL );
	
	if ( ! fixed_start )
		before_value = spinbutton_before .get_value() ;

	//connect signalhandlers of the spinbuttons
	if ( ! fixed_start )
		before_change_connection = spinbutton_before .signal_value_changed() .connect(
			sigc::bind<SPINBUTTON>( 
				sigc::mem_fun(*this, &Dialog_Base_Partition::on_spinbutton_value_changed), BEFORE ) ) ;
	else
		//Initialise empty connection object for use in the destructor
		before_change_connection = sigc::connection() ;
	
	size_change_connection = spinbutton_size .signal_value_changed() .connect(
		sigc::bind<SPINBUTTON>( 
			sigc::mem_fun(*this, &Dialog_Base_Partition::on_spinbutton_value_changed), SIZE ) ) ;
	after_change_connection = spinbutton_after .signal_value_changed() .connect(
		sigc::bind<SPINBUTTON>( 
			sigc::mem_fun(*this, &Dialog_Base_Partition::on_spinbutton_value_changed), AFTER ) ) ;

	//add alignment
	/*TO TRANSLATORS: used as label for a list of choices.   Align to: <optionmenu with choices> */
	table_resize .attach( * Utils::mk_label( static_cast<Glib::ustring>( _("Align to:") ) + "\t" ),
	                      0, 1, 3, 4, Gtk::FILL );

	//fill partition alignment menu
	/*TO TRANSLATORS: Menu option for drop down menu "Align to:" */
	menu_alignment .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("Cylinder") ) ) ;
	/*TO TRANSLATORS: Menu option for label "Align to:" */
	menu_alignment .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("MiB") ) ) ;
	/*TO TRANSLATORS: Menu option for drop down menu "Align to:" */
	menu_alignment .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("None") ) ) ;

	optionmenu_alignment .set_menu( menu_alignment );
	optionmenu_alignment .set_history( ALIGN_MEBIBYTE );  //Default setting

	table_resize .attach( optionmenu_alignment, 1, 2, 3, 4, Gtk::FILL );

	this->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	this ->show_all_children() ;
}

void Dialog_Base_Partition::Set_Resizer( bool extended )
{
	if ( extended )
		frame_resizer_base = new Frame_Resizer_Extended() ;
	else
	{
		frame_resizer_base = new Frame_Resizer_Base() ;
		frame_resizer_base ->signal_move .connect( sigc::mem_fun( this, &Dialog_Base_Partition::on_signal_move ) );
	}
	
	frame_resizer_base ->set_border_width( 5 ) ;
	frame_resizer_base ->set_shadow_type( Gtk::SHADOW_ETCHED_OUT );
		
	//connect signals
	frame_resizer_base ->signal_resize .connect( sigc::mem_fun( this, &Dialog_Base_Partition::on_signal_resize ) );
		
	hbox_resizer .pack_start( *frame_resizer_base, Gtk::PACK_EXPAND_PADDING );
	
	this ->show_all_children() ;
}

const Partition & Dialog_Base_Partition::Get_New_Partition()
{
	g_assert( new_partition != NULL );  // Bug: Not initialised by derived Dialog_Partition_*() constructor calling set_data()

	prepare_new_partition();
	return *new_partition;
}

void Dialog_Base_Partition::prepare_new_partition()
{
	g_assert( new_partition != NULL );  // Bug: Not initialised by derived Dialog_Partition_*() constructor calling set_data()

	Sector old_size = new_partition->get_sector_length();

	//FIXME:  Partition size is limited to just less than 1024 TeraBytes due
	//        to the maximum value of signed 4 byte integer.
	if ( ORIG_BEFORE != spinbutton_before .get_value_as_int() )
		new_partition->sector_start = START + Sector(spinbutton_before.get_value_as_int()) * (MEBIBYTE / new_partition->sector_size);

	if ( ORIG_AFTER != spinbutton_after .get_value_as_int() )
		new_partition->sector_end =
			new_partition->sector_start
			+ Sector(spinbutton_size.get_value_as_int()) * (MEBIBYTE / new_partition->sector_size)
			- 1 /* one sector short of exact mebibyte multiple */;

	//due to loss of precision during calcs from Sector -> MiB and back, it is possible
	//the new partition thinks it's bigger then it can be. Here we solve this.
	if ( new_partition->sector_start < START )
		new_partition->sector_start = START;
	if ( new_partition->sector_end > ( START + total_length - 1 ) )
		new_partition->sector_end = START + total_length - 1;

	//grow a bit into small freespace ( < 1MiB ) 
	if ( (new_partition->sector_start - START) < (MEBIBYTE / new_partition->sector_size) )
		new_partition->sector_start = START;
	if ( ( START + total_length - 1 - new_partition->sector_end ) < (MEBIBYTE / new_partition->sector_size) )
		new_partition->sector_end = START + total_length - 1;

	//set alignment
	switch ( optionmenu_alignment .get_history() )
	{
		case 0:
			new_partition->alignment = ALIGN_CYLINDER;
			break;
		case 1:
			new_partition->alignment = ALIGN_MEBIBYTE;
			{
				// If partition size is not an integer multiple of MiB or
				// the start or end sectors are not MiB aligned, and space
				// is available, then add 1 MiB to partition so requested
				// size is kept after GParted_Core::snap_to_mebibyte
				// method rounding.
				Sector partition_size = new_partition->sector_end - new_partition->sector_start + 1;
				Sector sectors_in_mib = MEBIBYTE / new_partition->sector_size;
				if (    (    ( partition_size % sectors_in_mib                    > 0 )
				          || ( new_partition->sector_start % sectors_in_mib       > 0 )
				          || ( ( new_partition->sector_end + 1 ) % sectors_in_mib > 0 )
				        )
				     && ( partition_size + sectors_in_mib < total_length )
				   )
					new_partition->sector_end += sectors_in_mib;
			}
			break;
		case 2:
			new_partition->alignment = ALIGN_STRICT;
			break;

		default:
			new_partition->alignment = ALIGN_MEBIBYTE;
			break;
	}

	//update partition usage
	if ( new_partition->sectors_used != -1 && new_partition->sectors_unused != -1 )
	{
		Sector new_size = new_partition->get_sector_length();
		if ( old_size == new_size )
		{
			//Pasting into new same sized partition or moving partition keeping the same size,
			//  therefore only block copy operation will be performed maintaining file system size.
			new_partition->set_sector_usage(
					new_partition->sectors_used + new_partition->sectors_unused,
					new_partition->sectors_unused );
		}
		else
		{
			//Pasting into new larger partition or (moving and) resizing partition larger or smaller,
			//  therefore block copy followed by file system grow or shrink operations will be
			//  performed making the file system fill the partition.
			new_partition->set_sector_usage( new_size, new_size - new_partition->sectors_used );
		}
	}

	new_partition->free_space_before = Sector(spinbutton_before.get_value_as_int()) * (MEBIBYTE / new_partition->sector_size);

	//if the original before value has not changed, then set indicator to keep start sector unchanged
	if ( ORIG_BEFORE == spinbutton_before .get_value_as_int() )
		new_partition->strict_start = TRUE;
}

void Dialog_Base_Partition::Set_Confirm_Button( CONFIRMBUTTON button_type ) 
{ 
	switch( button_type )
	{
		case NEW	:
			this ->add_button( Gtk::Stock::ADD, Gtk::RESPONSE_OK );
			
			break ;
		case RESIZE_MOVE:
			{
				Gtk::Image* image_temp(manage(new Gtk::Image(Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON)));
				Gtk::HBox* hbox_resize_move(manage(new Gtk::HBox()));

				hbox_resize_move->pack_start(*image_temp, Gtk::PACK_EXPAND_PADDING);
				hbox_resize_move->pack_start(*Utils::mk_label(fixed_start ? _("Resize") : _("Resize/Move")),
							Gtk::PACK_EXPAND_PADDING);
				button_resize_move.add(*hbox_resize_move);
			}

			this ->add_action_widget ( button_resize_move, Gtk::RESPONSE_OK ) ;
			button_resize_move .set_sensitive( false ) ;
			
			break ;
		case PASTE	:
			this ->add_button( Gtk::Stock::PASTE, Gtk::RESPONSE_OK );
			
			break ;
	}
}

void Dialog_Base_Partition::Set_MinMax_Text( Sector min, Sector max )
{
	Glib::ustring str_temp(String::ucompose( _("Minimum size: %1 MiB"), min ) + "\t\t");
	str_temp += String::ucompose( _("Maximum size: %1 MiB"), max ) ;
	label_minmax .set_text( str_temp ) ; 
}

int Dialog_Base_Partition::MB_Needed_for_Boot_Record( const Partition & partition )
{
	//Determine if space is needed for the Master Boot Record or
	//  the Extended Boot Record.  Generally an an additional track or MEBIBYTE
	//  is required so for our purposes reserve a MEBIBYTE in front of the partition.
	//  NOTE:  This logic also contained in Win_GParted::set_valid_operations
	if (   (   partition .inside_extended
	        && partition .type == TYPE_UNALLOCATED
	       )
	    || ( partition .type == TYPE_LOGICAL )
	                                     /* Beginning of disk device */
	    || ( partition .sector_start <= (MEBIBYTE / partition .sector_size) )
	   )
		return 1 ;
	else
		return 0 ;
}

void Dialog_Base_Partition::on_signal_move( int x_start, int x_end )
{  
	GRIP = true ;

	spinbutton_before .set_value( x_start * MB_PER_PIXEL ) ;

	if ( x_end == 500 )
	{
		spinbutton_after .set_value( 0 ) ;
		spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value() ) ;
	}
	else
		spinbutton_after .set_value( TOTAL_MB - spinbutton_before .get_value() - spinbutton_size .get_value() ) ;
	
	Check_Change() ;
	
	GRIP = false ;
}

void Dialog_Base_Partition::on_signal_resize( int x_start, int x_end, Frame_Resizer_Base::ArrowType arrow )
{  
	GRIP = true ;
		
	spinbutton_size .set_value( ( x_end - x_start ) * MB_PER_PIXEL ) ;
	
	before_value = fixed_start ? MIN_SPACE_BEFORE_MB : spinbutton_before .get_value() ;

	if ( arrow == Frame_Resizer_Base::ARROW_RIGHT ) //don't touch freespace before, leave it as it is
	{
		if ( x_end == 500 )
		{
			spinbutton_after .set_value( 0 ) ;
			spinbutton_size .set_value( TOTAL_MB - before_value ) ;
		}
		else
			spinbutton_after .set_value( TOTAL_MB - before_value - spinbutton_size .get_value() ) ;
	}
	else if ( arrow == Frame_Resizer_Base::ARROW_LEFT ) //don't touch freespace after, leave it as it is
		spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value() - spinbutton_after .get_value() ) ;

	Check_Change() ;
	
	GRIP = false ;
}

void Dialog_Base_Partition::on_spinbutton_value_changed( SPINBUTTON spinbutton )
{  
	if ( ! GRIP )
	{
		before_value = fixed_start ? MIN_SPACE_BEFORE_MB : spinbutton_before .get_value() ;
			
		//Balance the spinbuttons
		switch ( spinbutton )
		{
			case BEFORE	:
				spinbutton_after .set_value( TOTAL_MB - spinbutton_size .get_value() - before_value ) ;
				spinbutton_size .set_value( TOTAL_MB - before_value - spinbutton_after .get_value() ) ;
							
				break ;
			case SIZE	:
				spinbutton_after .set_value( TOTAL_MB - before_value - spinbutton_size .get_value() );
				if ( ! fixed_start )
					spinbutton_before .set_value( 
						TOTAL_MB - spinbutton_size .get_value() - spinbutton_after .get_value() );
				
				break;
			case AFTER	:
				if ( ! fixed_start )
					spinbutton_before .set_value( 
						TOTAL_MB - spinbutton_size .get_value() - spinbutton_after .get_value() );
			
				spinbutton_size .set_value( TOTAL_MB - before_value - spinbutton_after .get_value() ) ;
				
				break;
		}
		
		//And apply the changes to the visual view...
		if ( ! fixed_start )
			frame_resizer_base ->set_x_start( Utils::round( spinbutton_before .get_value() / MB_PER_PIXEL ) ) ;
		
		frame_resizer_base ->set_x_end( 500 - Utils::round( spinbutton_after .get_value() / MB_PER_PIXEL ) ) ;
		
		frame_resizer_base ->Draw_Partition() ;
		
		Check_Change() ;
	}
}

void Dialog_Base_Partition::Check_Change()
{
	button_resize_move .set_sensitive(
		ORIG_BEFORE != spinbutton_before .get_value_as_int()	||
		ORIG_SIZE   != spinbutton_size .get_value_as_int()	|| 
		ORIG_AFTER  != spinbutton_after .get_value_as_int() ) ; 
}

Dialog_Base_Partition::~Dialog_Base_Partition() 
{
	before_change_connection .disconnect() ;
	size_change_connection .disconnect() ;
	after_change_connection .disconnect() ;
	delete frame_resizer_base;
}

} //GParted
