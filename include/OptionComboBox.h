/* Copyright (C) 2018 Luca Bacci
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

#ifndef GPARTED_OPTIONCOMBOBOX_H
#define GPARTED_OPTIONCOMBOBOX_H

#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>

namespace GParted
{

class OptionComboBox
 : public Gtk::ComboBox
{
public:
	explicit OptionComboBox();
	virtual ~OptionComboBox();

	void prepend( const Glib::ustring& text, bool sensitive = true );
	void append( const Glib::ustring& text, bool sensitive = true );
	void insert( int index, const Glib::ustring& text, bool sensitive = true );

	void erase( int index );
	void clear();

	void set_row( int index, const Glib::ustring& text, bool sensitive );
	void set_row_text( int index, const Glib::ustring& text );
	void set_row_sensitive( int index, bool sensitive = true );
	
	void set_active_row( int index );

	Glib::ustring get_row_text( int index ) const;
	bool get_row_sensitive( int index ) const;
	
	int get_n_rows() const;

	int get_active_row_index() const;
	Glib::ustring get_active_row_text() const;

	static Glib::RefPtr<OptionComboBox> create() {
		return Glib::RefPtr<OptionComboBox>( new OptionComboBox() );
	}

protected:
	Glib::RefPtr<Gtk::ListStore> m_ref_liststore;
	Gtk::TreeModelColumn<Glib::ustring> slot_text;
	Gtk::TreeModelColumn<bool> slot_sensitive;

// utils
	Gtk::TreeModel::iterator make_nth_iter( int index );
	Gtk::TreeModel::const_iterator make_nth_iter( int index ) const;
};

}//GParted

#endif /* GPARTED_COMBOBOX_H */
