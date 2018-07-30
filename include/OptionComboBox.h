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

#include <vector>

/*TODO review gtkmm includes */
#include <glibmm/ustring.h>
#include "gtkmm/combobox.h"
#include "gtkmm/treemodel.h"
#include "gtkmm/liststore.h"

namespace GParted
{

class OptionComboBox
 : public Gtk::ComboBox
{
public:
	class Row
	{
	public:
		Row ( OptionComboBox *parent, Gtk::TreeModel::iterator iter);
		~Row( );
		Row( const Row & );
		Row & operator=( const Row & );

		Glib::ustring get_text();
		void set_text( const Glib::ustring & text );
		bool get_sensitive();
		void set_sensitive( bool sensitive );

	private:
		Gtk::TreeModel::iterator m_iter;
		OptionComboBox *m_parent;
	
	friend class OptionComboBox;
	};
	friend class Row; /* for pre c++11 */

	OptionComboBox();
	~OptionComboBox();

	void append(const Glib::ustring & text, bool sensitive = true);
	void remove( const Row & );
	void clear();
	
	std::vector< Row > items();
	
	Glib::ustring get_active_text(); /*TODO make const */
	int           get_active_index(); /*TODO make const */

private:
	OptionComboBox( const OptionComboBox & src );              // Not implemented copy constructor
	OptionComboBox & operator=( const OptionComboBox & rhs );  // Not implemented copy assignment operator
	
	class ModelColumns : public Gtk::TreeModel::ColumnRecord
	{
	public:
		ModelColumns() { add(m_col_text); add(m_col_sensitive); }

		Gtk::TreeModelColumn<Glib::ustring> m_col_text;
		Gtk::TreeModelColumn<bool> m_col_sensitive;
	private:
		ModelColumns( const ModelColumns& );
		ModelColumns & operator=( const ModelColumns & );
	};

	ModelColumns                   m_columns;
	Glib::RefPtr<Gtk::ListStore>   m_model;
	Gtk::CellRendererText          m_cellrenderer;
};

}//GParted

#endif /* GPARTED_COMBOBOX_H */
