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

Dialog_About::Dialog_About()
{
	/*TO TRANSLATORS: this is the dialog title */
	this ->set_title( _( "About GParted" ) );
	this ->set_size_request( -1, 220 ) ;
	this ->set_resizable( false );
	this ->set_has_separator( false ) ;
	
	label_temp = manage( new Gtk::Label() ) ;
	label_temp -> set_markup( "\n<span size='small'>logo here ;)</span>\n" ) ;
	this ->get_vbox()->pack_start( *label_temp ,Gtk::PACK_SHRINK );
	
	label_temp = manage( new Gtk::Label() ) ;
	label_temp -> set_markup( "<span size='xx-large'><b>" + (Glib::ustring) _( "GParted" ) + " " + VERSION + "</b></span>" ) ;
	this ->get_vbox()->pack_start( *label_temp ,Gtk::PACK_SHRINK );
	
	label_temp = manage( new Gtk::Label() ) ;
	label_temp -> set_text( "\n" + (Glib::ustring) _( "Gnome Partition Editor based on libparted" ) + "\n" ) ;
	this ->get_vbox()->pack_start( *label_temp ,Gtk::PACK_SHRINK );
	
	label_temp = manage( new Gtk::Label() ) ;
	label_temp -> set_markup( "<span size='small'>" + (Glib::ustring) _( "Copyright (c)" ) + " 2004 Bart Hakvoort</span>" ) ;
	this ->get_vbox()->pack_start( *label_temp ,Gtk::PACK_SHRINK );
	
	label_temp = manage( new Gtk::Label() ) ;
	label_temp -> set_markup( "<span size='small'>http://gparted.sourceforge.net</span>" ) ;
	label_temp -> set_selectable( true ) ;
	this ->get_vbox()->pack_start( *label_temp ,Gtk::PACK_SHRINK );
	
	button_credits.add_pixlabel( "/usr/share/icons/hicolor/16x16/stock/generic/stock_about.png", "Credits", 0, 0.5 ) ;
	button_credits.signal_clicked() .connect( sigc::mem_fun( this, &Dialog_About::Show_Credits ) ) ;
	
	this ->get_action_area() ->set_layout( Gtk::BUTTONBOX_EDGE   ) ;
	this ->get_action_area() ->pack_start( button_credits ) ;
	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE );
	
	this ->show_all_children() ;
	
}

void Dialog_About::Show_Credits()
{
	Gtk::Dialog dialog( _("Credits"), *this ) ;
	Gtk::Notebook notebook_credits;
	Gtk::VBox vbox_written, vbox_documented, vbox_translators ;
	Gtk::Label label_writers, label_translators ;
		
	//written by
	vbox_written .set_border_width( 5 ) ;
	label_writers .set_text( "Bart Hakvoort <gparted@users.sf.net>");
	label_writers .set_alignment( Gtk::ALIGN_LEFT ) ;
	vbox_written .pack_start( label_writers, Gtk::PACK_SHRINK ) ;
	notebook_credits .set_size_request( -1, 200 ) ;
	/*TO TRANSLATORS: tablabel in aboutdialog */
	notebook_credits .append_page( vbox_written, _("Written by") ) ;
	
	//documented by
	/*TO TRANSLATORS: tablabel in aboutdialog */
//	notebook_credits .append_page( vbox_documented, _("Documented by") ) ;
	
	//translated by
	/*TO TRANSLATORS: your name(s) here please, if there are more translators put newlines (\n) between the names */
	label_translators .set_text( _( "translator_credits") ) ; 
	
	if ( label_translators .get_text() != "translator_credits" )
	{
		label_translators .set_alignment( Gtk::ALIGN_LEFT ) ;
		vbox_translators .pack_start( label_translators, Gtk::PACK_SHRINK ) ;
		/*TO TRANSLATORS: tablabel in aboutdialog */
		notebook_credits .append_page( vbox_translators, _("Translated by") ) ;
	}
	
	dialog .get_vbox()->pack_start(  notebook_credits, Gtk::PACK_SHRINK );
	dialog .add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE );
	
	dialog .set_resizable( false );
	dialog .show_all_children() ;
	dialog .run() ;
}
