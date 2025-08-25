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

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gdkmm/cursor.h>
#include <sigc++/signal.h>
#include <map>


namespace GParted
{


DialogManageFlags::DialogManageFlags(const Partition& partition, std::map<Glib::ustring, bool> flag_info)
 : m_changed(false), m_partition(partition), m_flag_info(flag_info)
{
	set_title( Glib::ustring::compose( _("Manage flags on %1"), partition .get_path() ) );
	set_resizable( false ) ;

	// WH (Widget Hierarchy): this->get_content_area() / vbox
	Gtk::Box* vbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL)));
	vbox->set_border_width(5);
	this->get_content_area()->pack_start(*vbox, Gtk::PACK_SHRINK);

	// WH: this->get_content_area() / vbox / m_treeview_flags
	m_liststore_flags = Gtk::ListStore::create(m_treeview_flags_columns);
	m_treeview_flags.set_model(m_liststore_flags);
	m_treeview_flags.set_headers_visible(false);

	m_treeview_flags.append_column("", m_treeview_flags_columns.status);
	m_treeview_flags.append_column("", m_treeview_flags_columns.flag);
	static_cast<Gtk::CellRendererToggle*>(m_treeview_flags.get_column_cell_renderer(0))
		->property_activatable() = true ;
	static_cast<Gtk::CellRendererToggle*>(m_treeview_flags.get_column_cell_renderer(0))
		->signal_toggled() .connect( sigc::mem_fun( *this, &DialogManageFlags::on_flag_toggled ) ) ;

	m_treeview_flags.set_size_request(300, -1);
	vbox->pack_start(m_treeview_flags, Gtk::PACK_SHRINK);

	add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_OK ) ->grab_focus() ;
		
	show_all_children() ;

	load_treeview();
}


void DialogManageFlags::load_treeview()
{
	m_liststore_flags->clear();

	for (std::map<Glib::ustring, bool>::iterator iter = m_flag_info.begin(); iter != m_flag_info.end(); ++iter)
	{
		m_row = *(m_liststore_flags->append());
		m_row[m_treeview_flags_columns.flag]   = iter->first;
		m_row[m_treeview_flags_columns.status] = iter->second;
	}
}


void DialogManageFlags::on_flag_toggled( const Glib::ustring & path ) 
{
	get_window()->set_cursor(Gdk::Cursor::create(Gdk::WATCH));
	set_sensitive( false ) ;
	while ( Gtk::Main::events_pending() )
		Gtk::Main::iteration() ;

	m_changed = true;

	m_row = *(m_liststore_flags->get_iter(path));
	m_row[m_treeview_flags_columns.status] = ! m_row[m_treeview_flags_columns.status];

	signal_toggle_flag.emit(m_partition, m_row[m_treeview_flags_columns.flag], m_row[m_treeview_flags_columns.status]);

	m_flag_info = signal_get_flags.emit(m_partition);
	load_treeview() ;
	
	set_sensitive( true ) ;
	get_window() ->set_cursor() ;
}


}//GParted
