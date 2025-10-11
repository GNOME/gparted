/* Copyright (C) 2008 Curtis Gedak
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

#include "Dialog_FileSystem_Label.h"
#include "Partition.h"

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/label.h>
#include <atkmm/relation.h>
#include <gtkmm/stock.h>
#include <gtkmm/entry.h>


namespace GParted
{


Dialog_FileSystem_Label::Dialog_FileSystem_Label( const Partition & partition )
{
	this ->set_resizable( false ) ;
	this->set_size_request( 400, -1 );

	/* TO TRANSLATORS: dialog title, looks like   Set file system label on /dev/hda3 */
	this->set_title( Glib::ustring::compose( _("Set file system label on %1"), partition.get_path() ) );

	// Horizontal box to hold the label and entry box line
	Gtk::Box* hbox(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));
	hbox->set_border_width( 5 );
	hbox->set_spacing( 10 );
	get_content_area()->pack_start(*hbox, Gtk::PACK_SHRINK);

	// Only line: "Label: [EXISTINGLABEL ]"
	Gtk::Label *label_label = Utils::mk_label("<b>" + Glib::ustring(_("Label:")) + "</b>");
	hbox->pack_start(*label_label, Gtk::PACK_SHRINK);
	entry = Gtk::manage(new Gtk::Entry());
	entry->set_max_length(Utils::get_filesystem_label_maxlength(partition.fstype));
	entry->set_width_chars( 30 );
	entry->set_activates_default( true );
	entry->set_text( partition.get_filesystem_label() );
	entry->select_region( 0, entry->get_text_length() );
	entry->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_label->get_accessible());
	hbox->pack_start( *entry, Gtk::PACK_SHRINK );

	this ->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL ) ;
	this ->add_button( Gtk::Stock::OK, Gtk::RESPONSE_OK ) ;
	this ->set_default_response( Gtk::RESPONSE_OK ) ;
	this ->show_all_children() ;
}


Glib::ustring Dialog_FileSystem_Label::get_new_label()
{
	return Utils::trim( Glib::ustring( entry ->get_text() ) );
}


}  // namespace GParted
