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
 
#ifndef TREEVIEW_DETAIL
#define TREEVIEW_DETAIL

#include "../include/Partition.h"

#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/entry.h>

#include <gtkmm/stock.h>
#include <gdkmm/pixbuf.h>

#include <vector>

namespace GParted
{

class TreeView_Detail : public Gtk::TreeView
{
public:
	TreeView_Detail( );
	void Load_Partitions( std::vector<Partition> & partitions ) ;
	void Set_Selected( const Partition & partition );

	//signals for interclass communication
	sigc::signal<void,GdkEventButton *,const Partition &> signal_mouse_click;

private:
	void Create_Row( const Gtk::TreeRow &, Partition &);

	//overridden signal
	virtual bool on_button_press_event(GdkEventButton *);
	
	Gtk::TreeRow row,childrow;
	Gtk::TreeIter iter,iter_child;

	Glib::RefPtr<Gtk::TreeStore> treestore_detail;
	Glib::RefPtr<Gtk::TreeSelection> treeselection;

	//columns for this treeview
	struct treeview_detail_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> partition;
		Gtk::TreeModelColumn<Glib::ustring> type;
		Gtk::TreeModelColumn<Glib::ustring> type_square;
		Gtk::TreeModelColumn<Glib::ustring> size;
		Gtk::TreeModelColumn<Glib::ustring> used;
		Gtk::TreeModelColumn<Glib::ustring> unused;
		Gtk::TreeModelColumn<Glib::ustring> color;
		Gtk::TreeModelColumn<Glib::ustring> text_color;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >  status_icon;
		Gtk::TreeModelColumn<Glib::ustring> flags;
		Gtk::TreeModelColumn< Partition >  partition_struct; //hidden column ( see also on_button_press_event )
		
		treeview_detail_Columns() {
			add( partition ); add( type ); add( type_square ); add( size );  add( used ); add( unused ); add( color ); add( text_color ); add( status_icon ); add( flags ); add(partition_struct);
		}
	};
	
	treeview_detail_Columns treeview_detail_columns;
	Partition partition_temp ; //used in Set_Selected to make the check a bit more readable

};

} //GParted

#endif //TREEVIEW_DETAIL
