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
	TreeView_Detail();
	void load_partitions( const std::vector<Partition> & partitions ) ;
	void set_selected( const Partition & partition );
	void clear() ;

	//signals for interclass communication
	sigc::signal< void, const Partition &, bool > signal_partition_selected ;
	sigc::signal< void > signal_partition_activated ;
	sigc::signal< void, unsigned int, unsigned int > signal_popup_menu ;

private:
	bool set_selected( Gtk::TreeModel::Children rows, const Partition & partition, bool inside_extended = false ) ;
	void create_row( const Gtk::TreeRow & treerow, const Partition & partition );

	//(overridden) signals
	bool on_button_press_event( GdkEventButton * event );
	void on_row_activated( const Gtk::TreeModel::Path & path, Gtk::TreeViewColumn * column ) ;
	void on_selection_changed() ;
	
	Gtk::TreeRow row, childrow;

	Glib::RefPtr<Gtk::TreeStore> treestore_detail;
	Glib::RefPtr<Gtk::TreeSelection> treeselection;

	bool block ;

	//columns for this treeview
	struct treeview_detail_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> partition;
		Gtk::TreeModelColumn<Glib::ustring> filesystem;
		Gtk::TreeModelColumn<Glib::ustring> mountpoint;
		Gtk::TreeModelColumn<Glib::ustring> size;
		Gtk::TreeModelColumn<Glib::ustring> used;
		Gtk::TreeModelColumn<Glib::ustring> unused;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > color;
		Gtk::TreeModelColumn<Glib::ustring> text_color;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > status_icon;
		Gtk::TreeModelColumn<Glib::ustring> flags;
		Gtk::TreeModelColumn<Partition> partition_struct; //hidden column ( see also on_button_press_event )
		
		treeview_detail_Columns( )
		{
			add( partition ); add( filesystem ); add( mountpoint ) ;
			add( size ); add( used ); add( unused );
			add( color ); add( text_color ); add( status_icon );
			add( flags ); add(partition_struct);
		}
	};
	
	treeview_detail_Columns treeview_detail_columns;
};

} //GParted

#endif //TREEVIEW_DETAIL
