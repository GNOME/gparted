/* Copyright (C) 2017 Mike Fleetwood
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

#include "DialogPasswordEntry.h"
#include "Partition.h"

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <atkmm/relation.h>
#include <gtkmm/stock.h>
#include <gtkmm/button.h>
#include <gtk/gtk.h>
#include <sigc++/signal.h>


namespace GParted
{


DialogPasswordEntry::DialogPasswordEntry(const Partition& partition, const Glib::ustring& reason)
{
	this->set_resizable( false );
	this->set_size_request( 400, -1 );

	/* TO TRANSLATORS: dialog title, looks like   LUKS Passphrase /dev/sda1 */
	this->set_title( Glib::ustring::compose( _("LUKS Passphrase %1"), partition.get_path() ) );

	// Separate vertical box to hold lines in the dialog.
	Gtk::Box* vbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL)));
	vbox->set_border_width( 5 );
	vbox->set_spacing( 5 );
	this->get_content_area()->pack_start(*vbox, Gtk::PACK_SHRINK);

	// Line 1: Reason message, e.g. "Enter LUKS passphrase to open /dev/sda1"
	vbox->pack_start(*Utils::mk_label(reason), Gtk::PACK_SHRINK);

	// Line 2: "Passphrase: [              ]"
	// (Horizontal box holding prompt and entry box)
	Gtk::Box* entry_hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));
	Gtk::Label *label_passphrase = Utils::mk_label("<b>" + Glib::ustring(_("Passphrase:")) + "</b>");
	entry_hbox->pack_start(*label_passphrase);
	m_entry = Gtk::manage(new Gtk::Entry());
	m_entry->set_width_chars(30);
	m_entry->set_visibility(false);
	m_entry->set_activates_default(true);
	m_entry->grab_focus();
	m_entry->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_passphrase->get_accessible());
	entry_hbox->pack_start(*m_entry);
	vbox->pack_start( *entry_hbox );

	// Line 3: blank
	vbox->pack_start( *Utils::mk_label( "" ) );

	// Line 4: error message
	m_error_message = Utils::mk_label("");
	vbox->pack_start(*m_error_message);

	this->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	Gtk::Button *unlock_button = this->add_button( _("Unlock"), Gtk::RESPONSE_OK );
	unlock_button->signal_clicked().connect( sigc::mem_fun( *this, &DialogPasswordEntry::on_button_unlock ) );
	this->set_default_response( Gtk::RESPONSE_OK );
	this->show_all_children();
}


const char * DialogPasswordEntry::get_password()
{
	// Avoid using the gtkmm C++ entry->get_text() because that constructs a
	// Glib::ustring, copying the password from the underlying C GtkEntry object into
	// an unsecured malloced chunk of memory.
	return (const char *)gtk_entry_get_text(GTK_ENTRY(m_entry->gobj()));
}


void DialogPasswordEntry::set_error_message( const Glib::ustring & message )
{
	m_error_message->set_label(message);
	// After unlock failure, also select failed password and make the password entry
	// box have focus ready for retyping the correct password.
	m_entry->select_region(0, -1);
	m_entry->grab_focus();
}


// Private methods

void DialogPasswordEntry::on_button_unlock()
{
	// Clear any previous unlock failure message before starting new attempt.
	m_error_message->set_label("");
}


}  // namespace GParted
