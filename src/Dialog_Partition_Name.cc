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

namespace GParted
{

Dialog_Partition_Name::Dialog_Partition_Name( const Partition & partition, int max_length )
{
	this->set_resizable( false );
	this->set_has_separator( false );
	this->set_size_request( 400, -1 );

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

		// NOTE: This limits the Gtk::Entry size in UTF-8 characters but partition
		// names are defined in terms of ASCII characters, or for GPT, UTF-16LE
		// code points.  See Utils::get_max_partition_name_length().  So for
		// certain extended characters this limit will be too generous.
		entry->set_max_length( max_length );

		entry->set_width_chars( 30 );
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
