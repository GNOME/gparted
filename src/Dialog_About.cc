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
 
#include "../include/Dialog_About.h"

namespace GParted
{

Dialog_About::Dialog_About()
{
	/*TO TRANSLATORS: this is the dialog title */
	this ->set_title( _( "About GParted" ) );
	this ->set_size_request( 250, 220 ) ;
	this ->set_resizable( false );
	this ->set_has_separator( false ) ;
	
	this ->get_vbox() ->pack_start( * mk_label( "\n<span size='small'>logo here ;)</span>\n", true, false ), Gtk::PACK_SHRINK );
	this ->get_vbox() ->pack_start( * mk_label( "<span size='xx-large'><b>" + (Glib::ustring) _( "GParted" ) + " " + VERSION + "</b></span>", true, false ) ,Gtk::PACK_SHRINK );
	this ->get_vbox() ->pack_start( * mk_label( "\n" + (Glib::ustring) _( "Gnome Partition Editor" ) + "\n", false, false ) ,Gtk::PACK_SHRINK );
	this ->get_vbox() ->pack_start( * mk_label( "<span size='small'>Copyright © 2004 Bart Hakvoort</span>", true, false ), Gtk::PACK_SHRINK );
	this ->get_vbox() ->pack_start( * mk_label( "<span size='small'>http://gparted.sourceforge.net</span>", true, false ), Gtk::PACK_SHRINK );
	
	button_credits.add_pixlabel( "/usr/share/icons/hicolor/16x16/stock/generic/stock_about.png", "Credits", 0, 0.5 ) ;
	button_credits.signal_clicked() .connect( sigc::mem_fun( this, &Dialog_About::Show_Credits ) ) ;
	
	this ->get_action_area() ->set_layout( Gtk::BUTTONBOX_EDGE ) ;
	this ->get_action_area() ->pack_start( button_credits ) ;
	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE );
	
	this ->show_all_children() ;
}

void Dialog_About::Show_Credits()
{
	Gtk::Dialog dialog( _("Credits"), *this ) ;
	Gtk::Notebook notebook_credits;
	Gtk::VBox vbox_written, vbox_translators ;
			
	//written by
	vbox_written .set_border_width( 5 ) ;
	vbox_written .pack_start( * mk_label( "Bart Hakvoort <gparted@users.sf.net>", false ), Gtk::PACK_SHRINK ) ;
	notebook_credits .set_size_request( -1, 200 ) ;
	
	/*TO TRANSLATORS: tablabel in aboutdialog */
	notebook_credits .append_page( vbox_written, _("Written by") ) ;
	
	/*TO TRANSLATORS: your name(s) here please, if there are more translators put newlines (\n) between the names.
	    It's a good idea to provide the url of your translationteam as well. Thanks! */
	Glib::ustring str_credits = _("translator-credits") ;
	if ( str_credits != "translator-credits" )
	{
		vbox_translators .set_border_width( 5 ) ;
		vbox_translators .pack_start( * mk_label( str_credits, false ), Gtk::PACK_SHRINK ) ;
		/*TO TRANSLATORS: tablabel in aboutdialog */
		notebook_credits .append_page( vbox_translators, _("Translated by") ) ;
	}
	
	dialog .get_vbox() ->pack_start( notebook_credits, Gtk::PACK_SHRINK );
	dialog .add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE );
	
	dialog .set_resizable( false );
	dialog .show_all_children() ;
	dialog .run() ;
}

}//GParted
