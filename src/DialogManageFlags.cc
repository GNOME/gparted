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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "DialogManageFlags.h"
#include "Partition.h"

#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gdkmm/cursor.h>

namespace GParted
{

DialogManageFlags::DialogManageFlags( const Partition & partition, std::map<Glib::ustring, bool> flag_info ) :
                                    partition( partition )
{
	any_change = false ;

	set_title( String::ucompose( _("Manage flags on %1"), partition .get_path() ) );
	set_has_separator( false ) ;
	set_resizable( false ) ;

	Glib::ustring str_temp = "<span weight=\"bold\" size=\"larger\">" ;
	str_temp += String::ucompose( _("Manage flags on %1"), partition .get_path() ) ;
	str_temp += "</span>\n" ;
	get_vbox() ->pack_start( * Utils::mk_label( str_temp ), Gtk::PACK_SHRINK );
	
	//setup treeview
	liststore_flags = Gtk::ListStore::create( treeview_flags_columns ) ;
	treeview_flags .set_model( liststore_flags ) ;
	treeview_flags .set_headers_visible( false ) ;

	treeview_flags .append_column( "", treeview_flags_columns .status ) ;
	treeview_flags .append_column( "", treeview_flags_columns .flag ) ;
	static_cast<Gtk::CellRendererToggle *>( treeview_flags .get_column_cell_renderer( 0 ) ) 
		->property_activatable() = true ;
	static_cast<Gtk::CellRendererToggle *>( treeview_flags .get_column_cell_renderer( 0 ) ) 
		->signal_toggled() .connect( sigc::mem_fun( *this, &DialogManageFlags::on_flag_toggled ) ) ;

	treeview_flags .set_size_request( 300, -1 ) ;
	get_vbox() ->pack_start( treeview_flags, Gtk::PACK_SHRINK ) ;

	this ->flag_info = flag_info ;
	
	load_treeview() ;
	add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_OK ) ->grab_focus() ;
		
	show_all_children() ;
}

void DialogManageFlags::load_treeview()
{
	liststore_flags ->clear() ;

	for ( std::map<Glib::ustring, bool>::iterator iter = flag_info .begin() ; iter != flag_info .end() ; ++iter )
	{
		row = *( liststore_flags ->append() ) ;
		row[ treeview_flags_columns .flag ] = iter ->first ;
		row[ treeview_flags_columns .status ] = iter ->second ;
	}
}

void DialogManageFlags::on_flag_toggled( const Glib::ustring & path ) 
{
	get_window() ->set_cursor( Gdk::Cursor( Gdk::WATCH ) ) ;
	set_sensitive( false ) ;
	while ( Gtk::Main::events_pending() )
		Gtk::Main::iteration() ;
	
	any_change = true ;

	row = *( liststore_flags ->get_iter( path ) ) ;
	row[ treeview_flags_columns .status ] = ! row[ treeview_flags_columns .status ] ;

	signal_toggle_flag .emit( partition, row[ treeview_flags_columns .flag ], row[ treeview_flags_columns .status ] ) ;

	flag_info = signal_get_flags .emit( partition ) ;
	load_treeview() ;
	
	set_sensitive( true ) ;
	get_window() ->set_cursor() ;
}

}//GParted
