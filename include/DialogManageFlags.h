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


#ifndef GPARTED_DIALOGMANAGEFLAGS_H
#define GPARTED_DIALOGMANAGEFLAGS_H

#include "Partition.h"

#include <gtkmm/dialog.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>

namespace GParted
{
	
class DialogManageFlags : public Gtk::Dialog
{
public:
	DialogManageFlags( const Partition & partition, std::map<Glib::ustring, bool> flag_info ) ;

	sigc::signal< std::map<Glib::ustring, bool>, const Partition & > signal_get_flags ;
	sigc::signal< bool, const Partition &, const Glib::ustring &, bool > signal_toggle_flag ;

	bool any_change ;
	
private:
	void load_treeview() ;
	void on_flag_toggled( const Glib::ustring & path ) ;
	
	
	Gtk::TreeView treeview_flags ;
	Gtk::TreeRow row ;
	
	Glib::RefPtr<Gtk::ListStore> liststore_flags ;
	
	struct treeview_flags_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> flag ;
		Gtk::TreeModelColumn<bool> status ;
		
		treeview_flags_Columns()
		{
			add( flag ) ;
			add( status ) ;
		}
	} ;
	treeview_flags_Columns treeview_flags_columns ;	

	const Partition & partition;  // (Alias to element in Win_GParted::display_partitions[] vector).
	std::map<Glib::ustring, bool> flag_info ;
};

} //GParted

#endif /* GPARTED_DIALOGMANAGEFLAGS_H */
