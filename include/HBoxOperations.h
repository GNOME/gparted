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

#ifndef GPARTED_HBOXOPERATIONS_H
#define GPARTED_HBOXOPERATIONS_H

#include "Operation.h"

#include <gtkmm/box.h>
#include <gtkmm/liststore.h>
#include <gtkmm/menu.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/treeview.h>

namespace GParted
{

class HBoxOperations : public Gtk::HBox
{
public:
	HBoxOperations() ;
	~HBoxOperations() ;

	void load_operations( const std::vector<Operation *> operations ) ;
	void clear() ;

	sigc::signal< void > signal_undo ;
	sigc::signal< void > signal_clear ;
	sigc::signal< void > signal_apply ;
	sigc::signal< void > signal_close ;

private:
	bool on_signal_button_press_event( GdkEventButton * event ) ;
	void on_undo() ;
	void on_clear() ;
	void on_apply() ;
	void on_close() ;

	Gtk::Menu menu_popup ;
	Gtk::ScrolledWindow scrollwindow ;
	Gtk::TreeView treeview_operations ;
	Glib::RefPtr<Gtk::ListStore> liststore_operations ;
	
	struct treeview_operations_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> operation_description;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > operation_icon;
				
		treeview_operations_Columns() 
		{ 
			add( operation_description );
			add( operation_icon );
		} 
	};
	treeview_operations_Columns treeview_operations_columns;
};

} //GParted

#endif /* GPARTED_HBOXOPERATIONS_H */
