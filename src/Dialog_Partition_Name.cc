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

#include "../include/Dialog_Partition_Name.h"

namespace GParted
{

// NOTE: Limiting the Gtk::Entry to 36 UTF-8 characters doesn't guarantee that the
// partition name won't be too long as GPT works in UTF-16LE code units.  Therefore
// any character taking more that 1 UTF-16LE code unit won't be correctly counted.
//     http://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_entries
static const int GPT_NAME_LENGTH = 36;

Dialog_Partition_Name::Dialog_Partition_Name( const Partition & partition )
{
	this->set_resizable( false );
	this->set_has_separator( false );
	this->set_size_request( 300, 80 );

	/* TO TRANSLATORS: dialog title, looks like   Set partition name on /dev/hda3 */
	this->set_title( String::ucompose( _("Set partition name on %1"), partition.get_path() ) );

	{
		int top = 0, bottom = 1;

		// Create table to hold name and entry box
		Gtk::Table *table( manage( new Gtk::Table() ) );

		table->set_border_width( 5 );
		table->set_col_spacings( 10 );
		get_vbox()->pack_start( *table, Gtk::PACK_SHRINK );
		table->attach( *Utils::mk_label( "<b>" + Glib::ustring(_("Name:")) + "</b>" ),
		               0, 1,
		               top, bottom,
		               Gtk::FILL );
		// Create text entry box
		entry = manage( new Gtk::Entry() );
		entry->set_max_length( GPT_NAME_LENGTH ) ;
		entry->set_width_chars( 20 );
		entry->set_activates_default( true );
		entry->set_text( partition.name );
		entry->select_region( 0, entry->get_text_length() );
		// Add entry box to table
		table->attach( *entry,
		               1, 2,
		               top++, bottom++,
		               Gtk::FILL );
	}

	this->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	this->add_button( Gtk::Stock::OK, Gtk::RESPONSE_OK );
	this->set_default_response( Gtk::RESPONSE_OK );
	this->show_all_children() ;
}

Dialog_Partition_Name::~Dialog_Partition_Name()
{
}

Glib::ustring Dialog_Partition_Name::get_new_name()
{
	return Utils::trim( Glib::ustring( entry->get_text() ) );
}

} //GParted
