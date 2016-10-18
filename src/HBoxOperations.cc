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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "HBoxOperations.h"

#include <gtkmm/stock.h>

namespace GParted
{

HBoxOperations::HBoxOperations() 
{
	//create listview for pending operations
	liststore_operations = Gtk::ListStore::create( treeview_operations_columns );
	treeview_operations .set_model( liststore_operations );
	treeview_operations .set_headers_visible( false );
	treeview_operations .append_column( "", treeview_operations_columns .operation_icon );
	treeview_operations .append_column( "", treeview_operations_columns .operation_description );
	treeview_operations .get_selection() ->set_mode( Gtk::SELECTION_NONE ) ;
	treeview_operations .signal_button_press_event() .connect( 
		sigc::mem_fun( *this, &HBoxOperations::on_signal_button_press_event ), false ) ;

	//init scrollwindow_operations
	scrollwindow .set_shadow_type( Gtk::SHADOW_ETCHED_IN );
	scrollwindow .set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
	scrollwindow .add ( treeview_operations ) ;
	pack_start( scrollwindow, Gtk::PACK_EXPAND_WIDGET );

	//create popupmenu
	menu_popup .items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( 
		_("_Undo Last Operation"), 
		* manage( new Gtk::Image( Gtk::Stock::UNDO, Gtk::ICON_SIZE_MENU ) ), 
		sigc::mem_fun(*this, &HBoxOperations::on_undo) ) );

	menu_popup .items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( 
		_("_Clear All Operations"), 
		* manage( new Gtk::Image( Gtk::Stock::CLEAR, Gtk::ICON_SIZE_MENU ) ), 
		sigc::mem_fun(*this, &HBoxOperations::on_clear) ) );

	menu_popup .items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( 
		_("_Apply All Operations"), 
		* manage( new Gtk::Image( Gtk::Stock::APPLY, Gtk::ICON_SIZE_MENU ) ), 
		sigc::mem_fun(*this, &HBoxOperations::on_apply) ) );

	menu_popup .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	menu_popup .items() .push_back( Gtk::Menu_Helpers::StockMenuElem( 
		Gtk::Stock::CLOSE, sigc::mem_fun(*this, &HBoxOperations::on_close) ) );
}

void HBoxOperations::load_operations( const std::vector<Operation *> operations ) 
{
	liststore_operations ->clear();

	Gtk::TreeRow treerow ;
	for ( unsigned int t = 0 ; t < operations .size(); t++ )
	{	
		treerow = *( liststore_operations ->append() );
		treerow[ treeview_operations_columns .operation_description ] = operations[ t ] ->description ;
		treerow[ treeview_operations_columns .operation_icon ] = operations[ t ] ->icon ;
	}
		
	//make scrollwindow focus on the last operation in the list	
	if ( liststore_operations ->children() .size() > 0 )
		treeview_operations .set_cursor( static_cast<Gtk::TreePath>( static_cast<Gtk::TreeRow>( 
					*(--liststore_operations ->children() .end()) ) ) ) ;
}

void HBoxOperations::clear()
{
	liststore_operations ->clear();
}

bool HBoxOperations::on_signal_button_press_event( GdkEventButton * event )
{
	//right-click
	if ( event ->button == 3 )
	{
		menu_popup .items()[0] .set_sensitive( liststore_operations ->children() .size() > 0 ) ;
		menu_popup .items()[1] .set_sensitive( liststore_operations ->children() .size() > 0 ) ;
		menu_popup .items()[2] .set_sensitive( liststore_operations ->children() .size() > 0 ) ;

		menu_popup .popup( event ->button, event ->time ) ;
	}

	return true ;
}

void HBoxOperations::on_undo() 
{
	signal_undo .emit() ;
}

void HBoxOperations::on_clear() 
{
	signal_clear .emit() ;
}

void HBoxOperations::on_apply() 
{
	signal_apply .emit() ;
}

void HBoxOperations::on_close() 
{
	signal_close .emit() ;
}

HBoxOperations::~HBoxOperations() 
{
}

} //GParted


