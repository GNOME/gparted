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

#include <glibmm/ustring.h>
#include <gtkmm/dialog.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <sigc++/signal.h>
#include <map>


namespace GParted
{


class DialogManageFlags : public Gtk::Dialog
{
public:
	DialogManageFlags( const Partition & partition, std::map<Glib::ustring, bool> flag_info ) ;

	sigc::signal< std::map<Glib::ustring, bool>, const Partition & > signal_get_flags ;
	sigc::signal< bool, const Partition &, const Glib::ustring &, bool > signal_toggle_flag ;

	bool m_changed;

private:
	void load_treeview() ;
	void on_flag_toggled( const Glib::ustring & path ) ;
	void update_warning();

	Gtk::TreeView m_treeview_flags;
	Gtk::Frame    m_warning_frame;
	Gtk::Label    m_warning_message;

	Gtk::TreeRow  m_row;

	Glib::RefPtr<Gtk::ListStore> m_liststore_flags;

	struct TreeView_Flags_Columns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn<Glib::ustring> flag ;
		Gtk::TreeModelColumn<bool> status ;
		
		TreeView_Flags_Columns()
		{
			add( flag ) ;
			add( status ) ;
		}
	} ;
	TreeView_Flags_Columns m_treeview_flags_columns;

	const Partition& m_partition;  // (Alias to element in Win_GParted::m_display_device.partitions[] vector).
	std::map<Glib::ustring, bool> m_flag_info;
};


}  // namespace GParted


#endif /* GPARTED_DIALOGMANAGEFLAGS_H */
