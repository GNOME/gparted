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

#ifndef GPARTED_TREEVIEW_DETAIL_H
#define GPARTED_TREEVIEW_DETAIL_H

#include "Partition.h"
#include "PartitionVector.h"

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
	void load_partitions( const PartitionVector & partitions );
	void set_selected( const Partition * partition_ptr );
	void clear() ;

	// Signals for interclass communication
	sigc::signal<void, const Partition *, bool> signal_partition_selected;
	sigc::signal< void > signal_partition_activated ;
	sigc::signal< void, unsigned int, unsigned int > signal_popup_menu ;

private:
	void load_partitions( const PartitionVector & partitions,
	                      bool & show_names,
	                      bool & show_mountpoints,
	                      bool & show_labels,
	                      const Gtk::TreeRow & parent_row = Gtk::TreeRow() );
	bool set_selected( Gtk::TreeModel::Children rows,
	                   const Partition * partition_ptr, bool inside_extended = false );
	void create_row( const Gtk::TreeRow & treerow,
	                 const Partition & partition,
	                 bool & show_names,
	                 bool & show_mountpoints,
	                 bool & show_labels );

	//(overridden) signals
	bool on_button_press_event( GdkEventButton * event );
	void on_row_activated( const Gtk::TreeModel::Path & path, Gtk::TreeViewColumn * column ) ;
	void on_selection_changed() ;
	
	Glib::RefPtr<Gtk::TreeStore> treestore_detail;
	Glib::RefPtr<Gtk::TreeSelection> treeselection;

	bool block ;

	//columns for this treeview
	struct treeview_detail_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> path;
		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> filesystem;
		Gtk::TreeModelColumn<Glib::ustring> mountpoint;
		Gtk::TreeModelColumn<Glib::ustring> label ;
		Gtk::TreeModelColumn<Glib::ustring> size;
		Gtk::TreeModelColumn<Glib::ustring> used;
		Gtk::TreeModelColumn<Glib::ustring> unused;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > color ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon1 ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon2 ;
		Gtk::TreeModelColumn<Glib::ustring> flags;
		Gtk::TreeModelColumn<const Partition *> partition_ptr;  // Hidden column.  (Alias to element in
		                                                        // Win_GParted::display_partitions[] vector).

		treeview_detail_Columns()
		{
			add( path ); add( name ); add( filesystem ); add( mountpoint ); add( label );
			add( size ); add( used ); add( unused ); add( color );
			add( icon1 ); add( icon2 ); add( flags ); add( partition_ptr );
		}
	};
	
	treeview_detail_Columns treeview_detail_columns;
};

} //GParted

#endif /* GPARTED_TREEVIEW_DETAIL_H */
