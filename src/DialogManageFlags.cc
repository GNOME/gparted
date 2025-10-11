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

#include "GParted_Core.h"
#include "Partition.h"

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/main.h>
#include <gtkmm/stock.h>
#include <gdkmm/cursor.h>
#include <sigc++/signal.h>
#include <map>
#include <utility>


namespace GParted
{


DialogManageFlags::DialogManageFlags(const Partition& partition, std::map<Glib::ustring, bool> flag_info)
 : m_changed(false),
   m_warning_message(std::move(*Utils::mk_label("", false, true, true, Gtk::ALIGN_START))),
   m_partition(partition), m_flag_info(flag_info)
{
	set_title( Glib::ustring::compose( _("Manage flags on %1"), partition .get_path() ) );
	set_resizable( false ) ;

	// WH (Widget Hierarchy): this->get_content_area() / vbox
	Gtk::Box* vbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL)));
	vbox->set_border_width(5);
	vbox->set_spacing(5);
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

	vbox->pack_start(m_treeview_flags, Gtk::PACK_SHRINK);

	// Reserve space in the dialog so that it doesn't change size when the warning
	// frame is shown or hidden.
	// WH: this->get_content_area() / vbox / reserve_vbox
	Gtk::Box* reserve_vbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL)));
	reserve_vbox->set_size_request(360, 90);
	vbox->pack_start(*reserve_vbox, Gtk::PACK_EXPAND_WIDGET);

	// WH: this->get_content_area() / vbox / reserve_vbox / m_warning_frame
	reserve_vbox->pack_start(m_warning_frame, Gtk::PACK_EXPAND_WIDGET);

	// WH: ... / vbox / reserve_vbox / m_warning_frame / label_hbox
	Gtk::Box* label_hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));
	m_warning_frame.set_label_widget(*label_hbox);

	// WH: ... / vbox / reserve_vbox / m_warning_frame / label_hbox / warning_icon
	Gtk::Image* warning_icon = Utils::mk_image(Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON);
	label_hbox->pack_start(*warning_icon, Gtk::PACK_SHRINK);

	// WH: ... / vbox / reserve_vbox / m_warning_frame / label_hbox / "Warning:"
	label_hbox->pack_start(*Utils::mk_label("<b> " + Glib::ustring(_("Warning:")) + "</b>"),
	                       Gtk::PACK_SHRINK);

	// WH: ... / vbox / reserve_vbox / m_warning_frame / warning_vbox
	Gtk::Box* warning_vbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL)));
	warning_vbox->set_border_width(5);  // Padding inside the frame
	m_warning_frame.add(*warning_vbox);

	// WH: ... / vbox / reserve_vbox / m_warning_frame / warning_vbox / m_warning_message
	warning_vbox->pack_start(m_warning_message, Gtk::PACK_EXPAND_WIDGET);

	add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE)->grab_focus();

	show_all_children() ;

	load_treeview();
	update_warning();
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
	update_warning();

	set_sensitive( true ) ;
	get_window() ->set_cursor() ;
}


void DialogManageFlags::update_warning()
{
	bool esp_flag = false;
	const std::map<Glib::ustring, bool>::const_iterator it = m_flag_info.find("esp");
	if (it != m_flag_info.end())
		esp_flag = it->second;

	Glib::ustring warning = GParted_Core::check_logical_esp_warning(m_partition.type, esp_flag);

	m_warning_message.set_label(warning);
	m_warning_frame.set_visible(warning.size() > 0);
}


}  // namespace GParted
