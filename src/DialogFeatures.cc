/* Copyright (C) 2004-2006 Bart 'plors' Hakvoort
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
 
#include "../include/DialogFeatures.h" 

#include <gtkmm/stock.h>

namespace GParted
{

DialogFeatures::DialogFeatures() 
{
	set_title( _("Features") ) ;
	set_has_separator( false ) ;
	set_resizable( false ) ;

	liststore_filesystems = Gtk::ListStore::create( treeview_filesystems_columns );
	treeview_filesystems .set_model( liststore_filesystems );
	treeview_filesystems .append_column( _("Filesystem"), treeview_filesystems_columns .filesystem );
	treeview_filesystems .append_column( _("Detect"), treeview_filesystems_columns .detect );
	treeview_filesystems .append_column( _("Read"), treeview_filesystems_columns .read );
	treeview_filesystems .append_column( _("Create"), treeview_filesystems_columns .create );
	treeview_filesystems .append_column( _("Grow"), treeview_filesystems_columns .grow );
	treeview_filesystems .append_column( _("Shrink"), treeview_filesystems_columns .shrink );
	treeview_filesystems .append_column( _("Move"), treeview_filesystems_columns .move );
	treeview_filesystems .append_column( _("Copy"), treeview_filesystems_columns .copy );
	treeview_filesystems .append_column( _("Check"), treeview_filesystems_columns .check );
	//FIXME: add info about the relevant project (e.g an url to the projectpage)
	//of course this url has to be selectable and (if possible) clickable
	treeview_filesystems .get_selection() ->set_mode( Gtk::SELECTION_NONE );
	treeview_filesystems .set_rules_hint( true ) ;

	hbox = manage( new Gtk::HBox() ) ;
	hbox ->set_border_width( 6 ) ;
	hbox ->pack_start( treeview_filesystems ) ;
	get_vbox() ->pack_start( *hbox ) ;
	
	//legend..
	hbox = manage( new Gtk::HBox() ) ;
	image = manage( new Gtk::Image( Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR ) );
	hbox ->pack_start( *image, Gtk::PACK_SHRINK ) ;
	hbox ->pack_start( * Utils::mk_label( _("Available") ), Gtk::PACK_EXPAND_WIDGET ) ;
	get_vbox() ->pack_start( *hbox ) ;

	hbox = manage( new Gtk::HBox() ) ;
	image = manage( new Gtk::Image( Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ) );
	hbox ->pack_start( *image, Gtk::PACK_SHRINK ) ;
	hbox ->pack_start( * Utils::mk_label( _("Not Available") ), Gtk::PACK_EXPAND_WIDGET ) ;
	get_vbox() ->pack_start( *hbox ) ;

	//initialize icons
	icon_yes = render_icon( Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ; 
	icon_no = render_icon( Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ; 
	
	add_button( Gtk::Stock::REFRESH, Gtk::RESPONSE_OK ) ;
	add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE ) ->grab_focus() ;
	show_all_children() ;
}

void DialogFeatures::load_filesystems( const std::vector<FS> & FILESYSTEMS )
{
	liststore_filesystems ->clear() ;
	
	for ( unsigned short t = 0; t < FILESYSTEMS .size() -1 ; t++ )
		show_filesystem( FILESYSTEMS[ t ] ) ;
}
		
void DialogFeatures::show_filesystem( const FS & fs )
{
	treerow = *( liststore_filesystems ->append() );
	treerow[ treeview_filesystems_columns .filesystem ] = Utils::get_filesystem_string( fs .filesystem ) ;
	
	treerow[ treeview_filesystems_columns .detect ] = icon_yes ; 
	treerow[ treeview_filesystems_columns .read ] = fs .read ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .create ] = fs .create ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .grow ] = fs .grow ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .shrink ] = fs .shrink ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .move ] = fs .move ? icon_yes : icon_no ;  
	treerow[ treeview_filesystems_columns .copy ] = fs .copy ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .check ] = fs .check ? icon_yes : icon_no ; 
}

DialogFeatures::~DialogFeatures() 
{
}

} //GParted


