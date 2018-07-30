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


#include <glibmm/ustring.h>
#include <gtkmm/combobox.h>
#include <gtkmm/liststore.h>
#include <gtkmm/treerowreference.h>


namespace GParted
{

class OptionStore_Item;
class OptionStore_Item_Collection;
typedef const OptionStore_Item            OptionStore_Item_Const;
typedef const OptionStore_Item_Collection OptionStore_Item_Collection_Const;
class OptionStore;


class OptionStore_Item
{
public:
	explicit OptionStore_Item( const Glib::RefPtr<OptionStore>& ref_model,
	                           const Gtk::TreeModel::iterator& iter );
	explicit OptionStore_Item( const Glib::RefPtr<OptionStore>& ref_model,
	                           const Gtk::TreeModel::Path& path );
	explicit OptionStore_Item( const Gtk::TreeModel::RowReference& rowreference );

	void set( const Glib::ustring& text,
	          bool sensitive = true );
	void set_text( const Glib::ustring& text );
	void set_sensitive( bool sensitive = true );

	Glib::ustring text() const;
	bool          sensitive() const;

	operator Gtk::TreeModel::iterator() {
		return to_iterator_();
	}
	operator Gtk::TreeModel::const_iterator() const {
		return to_iterator_();
	}

private:
	Gtk::TreeModel::iterator to_iterator_() const;
	friend class OptionStore_Item_Collection;

private:
	mutable Gtk::TreeModel::RowReference m_rowreference;
};


class OptionStore_Item_Collection
{
public:
	explicit OptionStore_Item_Collection( const Glib::RefPtr<OptionStore>& ref_model );

	void push_front( const Glib::ustring& text,
	                 bool sensitive = true );
	void push_back( const Glib::ustring& text,
	                bool sensitive = true );

	void insert( const OptionStore_Item& item,
	             const Glib::ustring& text,
	             bool sensitive = true );
	void insert( unsigned position,
	             const Glib::ustring& text,
	             bool sensitive = true );

	void pop_front();
	void pop_back();

	void erase( const OptionStore_Item& item );
	void erase( unsigned position );
	void clear();

	OptionStore_Item front();
	OptionStore_Item back();

	OptionStore_Item at( unsigned position );
	OptionStore_Item operator[] ( unsigned position );

	unsigned size() const;

	OptionStore_Item_Const front() const;
	OptionStore_Item_Const back() const;

	OptionStore_Item_Const at( unsigned position ) const;
	OptionStore_Item_Const operator[] ( unsigned position ) const;

private:
	Glib::RefPtr<OptionStore> m_ref_model;
};


class OptionStore
 : public Gtk::ListStore
{
public:
	typedef Gtk::ListStore Base;

	typedef OptionStore_Item Item;
	typedef OptionStore_Item_Const Item_Const;
	typedef OptionStore_Item_Collection Item_Collection;
	typedef OptionStore_Item_Collection_Const Item_Collection_Const;

	static
	Glib::RefPtr<OptionStore> create();

	Item_Collection       items();
	Item_Collection_Const items() const;

	struct Slots
	{
		static Gtk::TreeModelColumn< Glib::ustring > Text;
		static Gtk::TreeModelColumn< bool > Sensitive;

	private:
		static Gtk::TreeModel::ColumnRecord record_;
		friend class OptionStore;
	};

protected:
	explicit OptionStore();
};


class OptionComboBox
 : public Gtk::ComboBox
{
public:
	typedef Gtk::ComboBox Base;

	explicit OptionComboBox( );
	explicit OptionComboBox( const Glib::RefPtr<OptionStore>& ref_model );

	void set_model( const Glib::RefPtr<OptionStore>& ref_model );

	Glib::RefPtr< OptionStore > get_model();
	OptionStore_Item_Collection items();
	OptionStore_Item            get_active();

	Glib::RefPtr< const OptionStore > get_model()  const;
	OptionStore_Item_Collection_Const items()      const;
	OptionStore_Item_Const            get_active() const;

protected:
	void pack_cell_renderers();

protected:
	Glib::RefPtr<OptionStore> m_ref_model;
};


}//GParted

#endif /* GPARTED_OPTIONCOMBOBOX_H */
