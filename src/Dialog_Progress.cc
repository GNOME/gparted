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
 
#include "../include/Dialog_Progress.h"
#include <iostream>
namespace GParted
{

Dialog_Progress::Dialog_Progress( int count_operations, Glib::RefPtr<Gtk::TextBuffer> textbuffer )
{
	this ->set_size_request( 600, 275 ) ;
	this ->set_resizable( false ) ;
	this ->set_has_separator( false ) ;
	this ->set_title( _("Applying pending operations") ) ;
	
	this ->count_operations = count_operations ;
	current_operation_number = 0 ;
	fraction = 1.00 / count_operations ;
			
	Glib::ustring str_temp = "<span weight=\"bold\" size=\"larger\">" ;
	str_temp += _( "Applying pending operations" ) ;
	str_temp += "</span>\n\n" ;
	str_temp += _("Applying all listed operations.") ;
	str_temp += "\n";
	str_temp += _("Clicking Cancel will prevent the next operations from being applied.") ;
	str_temp += "\n";
	this ->get_vbox( ) ->pack_start( * mk_label( str_temp ), Gtk::PACK_SHRINK );
	
	progressbar_current .set_pulse_step( 0.01 ) ;
	this->get_vbox( ) ->pack_start( progressbar_current, Gtk::PACK_SHRINK );
	
	label_current .set_alignment( Gtk::ALIGN_LEFT );
	this ->get_vbox( ) ->pack_start( label_current, Gtk::PACK_SHRINK );
	
	textview_details .set_sensitive( false ) ;
	textview_details .set_size_request( -1, 100 ) ;
	textview_details .set_wrap_mode( Gtk::WRAP_WORD ) ;
	
	textbuffer ->signal_insert( ) .connect( sigc::mem_fun( this, &Dialog_Progress::signal_textbuffer_insert ) ) ;
	textview_details .set_buffer( textbuffer ) ;
		
	scrolledwindow .set_shadow_type( Gtk::SHADOW_ETCHED_IN ) ;
	scrolledwindow .set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ;
	scrolledwindow .add( textview_details ) ;
	
	this ->get_vbox( ) ->pack_start( scrolledwindow, Gtk::PACK_SHRINK );
		
	this ->get_vbox( ) ->pack_start( * mk_label( "<b>\n" + (Glib::ustring) _( "Completed Operations" ) + ":</b>" ), Gtk::PACK_SHRINK );
	this ->get_vbox( ) ->pack_start( progressbar_all, Gtk::PACK_SHRINK );
	
	this ->get_vbox( ) ->set_spacing( 5 ) ;
	this ->get_vbox( ) ->set_border_width( 15 ) ;
	
	tglbtn_details .set_label( _("Details") ) ;
	tglbtn_details .signal_toggled( ) .connect( sigc::mem_fun( this, &Dialog_Progress::tglbtn_details_toggled ) ) ;
	
	this ->get_action_area( ) ->set_layout( Gtk::BUTTONBOX_EDGE ) ;
	this ->get_action_area( ) ->pack_start( tglbtn_details ) ;
	this ->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	
	this ->show_all_children( ) ;
	scrolledwindow .hide( ) ;
}

void Dialog_Progress::Set_Operation( )
{
	//all operations
	if ( current_operation_number && (progressbar_all .get_fraction( ) + fraction) <= 1.0 )
		progressbar_all .set_fraction( progressbar_all .get_fraction( ) + fraction );
	
	progressbar_all .set_text( String::ucompose( _("%1 of %2 operations completed"), current_operation_number++, count_operations ) ) ;

	//new operation
	conn .disconnect( ) ;
		
	label_current .set_markup( "<i>" + current_operation + "</i>" ) ;
	
	progressbar_current .set_fraction( 0 );
	progressbar_current .set_text( "initializing..." );
	
	if ( TIME_LEFT > 0 )
	{
		fraction_current = 1.00 / TIME_LEFT ;
		conn = Glib::signal_timeout( ) .connect( sigc::mem_fun( *this, &Dialog_Progress::Show_Progress ), 1000 );
	}
	else
		conn = Glib::signal_timeout( ) .connect( sigc::mem_fun( *this, &Dialog_Progress::Pulse ), 10 );
}

bool Dialog_Progress::Show_Progress( ) 
{
	if ( (progressbar_current .get_fraction( ) + fraction_current) <= 1.0 )
	{
		progressbar_current .set_fraction( progressbar_current .get_fraction( ) + fraction_current );
	
		if ( TIME_LEFT > 59 && TIME_LEFT < 120 )
			progressbar_current .set_text( String::ucompose( _("about %1 minute and %2 seconds left"), TIME_LEFT/60, TIME_LEFT % 60 ) ) ;
		else
			progressbar_current .set_text( String::ucompose( _("about %1 minutes and %2 seconds left"), TIME_LEFT/60, TIME_LEFT % 60 ) ) ;
		
		TIME_LEFT-- ;
	}
		
	return true ;
}

void Dialog_Progress::tglbtn_details_toggled( ) 
{
	if ( tglbtn_details .get_active( ) )
	{
		scrolledwindow .show( ) ;
		this ->set_size_request( 600, 375 ) ;
	}
	else
	{
		scrolledwindow .hide( ) ;
		this ->set_size_request( 600, 275 ) ;
	}
}

void Dialog_Progress::signal_textbuffer_insert( const Gtk::TextBuffer::iterator & iter, const Glib::ustring & text, int ) 
{
	Gtk::TextBuffer::iterator temp = iter ;
	textview_details .scroll_to( temp, 0 ) ;
}

Dialog_Progress::~Dialog_Progress()
{
	conn .disconnect( ) ;
}


}//GParted
