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

class OptionStoreItem;
class OptionStoreItemsCollection;
class OptionStore;
typedef const OptionStoreItem            OptionStoreItemConst;
typedef const OptionStoreItemsCollection OptionStoreItemsCollectionConst;


class OptionStoreItem
{
public:
	explicit OptionStoreItem( const Glib::RefPtr<OptionStore>& ref_model,
	                          const Gtk::TreeModel::iterator& iter );
	explicit OptionStoreItem( const Glib::RefPtr<OptionStore>& ref_model,
	                          const Gtk::TreeModel::Path& path );
	explicit OptionStoreItem( const Gtk::TreeModel::RowReference& rowreference );

	void set( const Glib::ustring& text,
	          bool sensitive = true );
	void set_text( const Glib::ustring& text );
	void set_sensitive( bool sensitive = true );

	Glib::ustring text() const;
	bool          sensitive() const;

	operator Gtk::TreeModel::iterator();
	operator Gtk::TreeModel::const_iterator() const;

private:
	Gtk::TreeModel::RowReference m_rowreference;
};


class OptionStoreItemsCollection
{
public:
	explicit OptionStoreItemsCollection( const Glib::RefPtr<OptionStore>& ref_model );

	void push_front( const Glib::ustring& text,
	                 bool sensitive = true );
	void push_back( const Glib::ustring& text,
	                bool sensitive = true );

	void insert( const OptionStoreItem& item,
	             const Glib::ustring& text,
	             bool sensitive = true );
	void insert( unsigned position,
	             const Glib::ustring& text,
	             bool sensitive = true );

	void pop_front();
	void pop_back();

	void erase( const OptionStoreItem& item );
	void erase( unsigned position );
	void clear();

	OptionStoreItem front();
	OptionStoreItem back();

	OptionStoreItem at( unsigned position );
	OptionStoreItem operator[] ( unsigned position );

	unsigned size() const;

	OptionStoreItemConst front() const;
	OptionStoreItemConst back() const;

	OptionStoreItemConst at( unsigned position ) const;
	OptionStoreItemConst operator[] ( unsigned position ) const;

private:
	Glib::RefPtr<OptionStore> m_ref_model;
};


class OptionStore
 : public Gtk::ListStore
{
public:
	typedef Gtk::ListStore Base;

	typedef OptionStoreItem Item;
	typedef const Item ItemConst;
	typedef OptionStoreItemsCollection ItemsCollection;
	typedef const ItemsCollection ItemsCollectionConst;

	static
	Glib::RefPtr<OptionStore> create();

	ItemsCollection      items();
	ItemsCollectionConst items() const;

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

	OptionStore::ItemsCollection items();
	OptionStore::ItemsCollectionConst items() const;

	// hide methods in base class Gtk::ComboBox
	void set_model( const Glib::RefPtr<OptionStore>& ref_model );

	Glib::RefPtr<OptionStore> get_model();
	Glib::RefPtr<const OptionStore> get_model() const;

	OptionStore::Item get_active();
	OptionStore::ItemConst get_active() const;

protected:
	void pack_cell_renderers();
	Glib::RefPtr<OptionStore> util_get_optionstore() const;
};


}//GParted

#endif /* GPARTED_OPTIONCOMBOBOX_H */
