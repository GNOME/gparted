/* Copyright (C) 2015 Michael Zimmermann
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

#include "Dialog_Partition_Name.h"
#include "Partition.h"

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/stock.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <atkmm/relation.h>


namespace GParted
{


Dialog_Partition_Name::Dialog_Partition_Name( const Partition & partition, int max_length )
{
	this->set_resizable( false );
	this->set_size_request( 400, -1 );

	/* TO TRANSLATORS: dialog title, looks like   Set partition name on /dev/hda3 */
	this->set_title( Glib::ustring::compose( _("Set partition name on %1"), partition.get_path() ) );

	// Horizontal box to hold the label and entry box line
	Gtk::Box* hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));
	hbox->set_border_width( 5 );
	hbox->set_spacing( 10 );
	get_content_area()->pack_start(*hbox, Gtk::PACK_SHRINK);

	// Only line: "Name: [EXISTINGNAME  ]"
	Gtk::Label *label_name = Utils::mk_label("<b>" + Glib::ustring(_("Name:")) + "</b>");
	hbox->pack_start(*label_name, Gtk::PACK_SHRINK);

	entry = Gtk::manage(new Gtk::Entry());
	// NOTE: This limits the Gtk::Entry size in UTF-8 characters but partition names
	// are defined in terms of ASCII characters, or for GPT, UTF-16LE code points.
	// See Utils::get_max_partition_name_length().  So for certain extended characters
	// this limit will be too generous.
	entry->set_max_length( max_length );

	entry->set_width_chars( 30 );
	entry->set_activates_default( true );
	entry->set_text( partition.name );
	entry->select_region( 0, entry->get_text_length() );
	entry->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_name->get_accessible());
	hbox->pack_start( *entry, Gtk::PACK_SHRINK );

	this->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	this->add_button( Gtk::Stock::OK, Gtk::RESPONSE_OK );
	this->set_default_response( Gtk::RESPONSE_OK );
	this->show_all_children() ;
}


Glib::ustring Dialog_Partition_Name::get_new_name()
{
	return Utils::trim( Glib::ustring( entry->get_text() ) );
}


}  // namespace GParted
