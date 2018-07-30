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

#include "OptionComboBox.h"

#include <gtkmm/cellrenderertext.h>

#include <climits>
#include <stdexcept>

namespace GParted
{

OptionComboBox::OptionComboBox()
{
	Gtk::TreeModelColumnRecord slots;
	slots .add( slot_text );
	slots .add( slot_sensitive );
	m_ref_liststore = Gtk::ListStore::create( slots );

	set_model( m_ref_liststore );

	Gtk::CellRendererText *cell = manage( new Gtk::CellRendererText() );
	pack_start( *cell );
	add_attribute( *cell, "text", slot_text );
	add_attribute( *cell, "sensitive", slot_sensitive );
}

OptionComboBox::~OptionComboBox()
{
}

void OptionComboBox::prepend( const Glib::ustring& text,
                              bool sensitive )
{
	Gtk::TreeModel::iterator iter = m_ref_liststore ->prepend();
	(*iter)[ slot_text ] = text;
	(*iter)[ slot_sensitive ] = sensitive;
}

void OptionComboBox::append( const Glib::ustring& text,
                             bool sensitive )
{
	Gtk::TreeModel::iterator iter = m_ref_liststore ->append();
	(*iter)[ slot_text ] = text;
	(*iter)[ slot_sensitive ] = sensitive;
}

void OptionComboBox::insert( int index,
                             const Glib::ustring& text,
                             bool sensitive )
{
	Gtk::TreeModel::iterator iter
		= m_ref_liststore ->insert( make_nth_iter( index ) );
	(*iter)[ slot_text ] = text;
	(*iter)[ slot_sensitive ] = sensitive;
}

void OptionComboBox::erase( int index )
{
	m_ref_liststore ->erase ( make_nth_iter( index ) );
}

void OptionComboBox::clear()
{
	m_ref_liststore ->clear();
}

void OptionComboBox::set_row( int index,
                              const Glib::ustring& text,
                              bool sensitive )
{
	Gtk::TreeModel::iterator iter = make_nth_iter( index );
	(*iter)[ slot_text ] = text;
	(*iter)[ slot_sensitive ] = sensitive;
}

void OptionComboBox::set_row_text( int index,
                                   const Glib::ustring& text )
{
	Gtk::TreeModel::iterator iter = make_nth_iter( index );
	(*iter)[ slot_text ] = text;
}

void OptionComboBox::set_row_sensitive( int index,
                                        bool sensitive )
{
	Gtk::TreeModel::iterator iter = make_nth_iter( index );
	(*iter)[ slot_sensitive ] = sensitive;
}

void OptionComboBox::set_active_row( int index )
{
	set_active( index );
}

Glib::ustring OptionComboBox::get_row_text( int index ) const
{
	Gtk::TreeModel::const_iterator iter = make_nth_iter( index );
	return (*iter)[ slot_text ];
}

bool OptionComboBox::get_row_sensitive( int index ) const
{
	Gtk::TreeModel::const_iterator iter = make_nth_iter( index );
	return (*iter)[ slot_sensitive ];
}

int OptionComboBox::get_n_rows() const
{
	if (m_ref_liststore ->children() .size() > INT_MAX)
		throw std::runtime_error("invalid integral conversion.");

	return static_cast<int>( m_ref_liststore ->children() .size() );
}

int OptionComboBox::get_active_row_index() const
{
	return get_active_row_number();
}

Glib::ustring OptionComboBox::get_active_row_text() const
{
	Gtk::TreeModel::const_iterator iter = get_active();
	return (*iter)[ slot_text ];
}

Gtk::TreeModel::iterator OptionComboBox::make_nth_iter( int index )
{
	Gtk::TreeModel::Path path;
	path .push_back( index );
	return m_ref_liststore ->get_iter( path );
}

Gtk::TreeModel::const_iterator OptionComboBox::make_nth_iter( int index ) const
{
	// there is no const_iterator get_iter() in gtkmm3 (added in gtkmm4)
	// have to use children() STL-like API
	return m_ref_liststore ->children() [ index ];
}

}//GParted
