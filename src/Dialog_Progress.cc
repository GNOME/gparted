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

namespace GParted
{

Dialog_Progress::Dialog_Progress( int count_operations, const Glib::ustring & first_operation )
{
	this ->set_size_request( 600, 275 ) ;
	this ->set_resizable( false ) ;
	this ->set_has_separator( false ) ;
	this ->set_title( _("Applying pending operations") ) ;
	
	this ->count_operations = count_operations ;
	current_operation_number = 0;
	
	fraction = (double) 1 / count_operations ;
	fraction -= 1E-8 ; //minus 1E-8 is to prevent fraction from ever reaching >=1, it needs to be 0.0 < fraction < 1.0
	
	Glib::ustring str_temp = "<span weight=\"bold\" size=\"larger\">" ;
	str_temp += _( "Applying pending operations" ) ;
	str_temp += "</span>\n\n" ;
	str_temp += _("Applying all listed operations.") ;
	str_temp += "\n";
	str_temp += _("Clicking Cancel will prevent the next operations from being applied.") ;
	str_temp += "\n";
	this->get_vbox() ->pack_start( * mk_label( str_temp ), Gtk::PACK_SHRINK );
	
	progressbar_current.set_text( _("initializing...") );
	this->get_vbox() ->pack_start( progressbar_current , Gtk::PACK_SHRINK);
	
	label_current.set_alignment( Gtk::ALIGN_LEFT   );
	label_current.set_markup( "<i>" + first_operation + "</i>" ) ;
	this->get_vbox() ->pack_start( label_current, Gtk::PACK_SHRINK );
		
	this->get_vbox() ->pack_start( * mk_label( "<b>\n" + (Glib::ustring) _( "Completed Operations" ) + ":</b>" ), Gtk::PACK_SHRINK );
	
	progressbar_all.set_text( String::ucompose( _("%1 of %2 operations completed"), 0, count_operations ) ) ;

	this->get_vbox() ->pack_start( progressbar_all, Gtk::PACK_SHRINK );
	
	this->get_vbox() ->set_spacing( 5 ) ;
	this->get_vbox() ->set_border_width( 15 ) ;
	
	this->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	this->show_all_children() ;
}

void Dialog_Progress::Set_Next_Operation( )
{ 
	progressbar_all.set_fraction( progressbar_all.get_fraction() + fraction );
	
	progressbar_all.set_text( String::ucompose( _("%1 of %2 operations completed"), ++current_operation_number, count_operations ) ) ;

	label_current.set_markup( "<i>" + current_operation + "</i>" ) ;
	progressbar_current.set_fraction( 0 );
	progressbar_current.set_text( "initializing..." );
}

void Dialog_Progress::Set_Progress_Current_Operation( )
{
	progressbar_current.set_fraction( fraction_current );

	if ( time_left > 59 && time_left < 120 )
		progressbar_current.set_text( String::ucompose( _("about %1 minute and %2 seconds left"), time_left/60, time_left % 60 ) ) ;
	else
		progressbar_current.set_text( String::ucompose( _("about %1 minutes and %2 seconds left"), time_left/60, time_left % 60 ) ) ;
}

Dialog_Progress::~Dialog_Progress()
{
}


}//GParted
