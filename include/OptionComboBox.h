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


// OptionStore_Item is the class that represents a single item (row).
// It lets you manipulate the data of a single item with a convenient
// high level interface.
class OptionStore_Item
{
public:
	explicit OptionStore_Item(const Glib::RefPtr<OptionStore>& ref_model,
	                          const Gtk::TreeModel::iterator& iter);
	explicit OptionStore_Item(const Glib::RefPtr<OptionStore>& ref_model,
	                          const Gtk::TreeModel::Path& path);
	explicit OptionStore_Item(const Gtk::TreeModel::RowReference& rowreference);

	void set(const Glib::ustring& text, bool sensitive = true);
	void set_text(const Glib::ustring& text);
	void set_sensitive(bool sensitive = true);

	Glib::ustring text() const;
	bool          sensitive() const;

	operator Gtk::TreeModel::iterator()
	{
		return to_iterator_();
	}
	operator Gtk::TreeModel::const_iterator() const
	{
		return to_iterator_();
	}

private:
	Gtk::TreeModel::iterator to_iterator_() const;
	friend class OptionStore_Item_Collection;

private:
	mutable Gtk::TreeModel::RowReference m_rowreference;
};


// OptionStore_Item_Collection lets you operate on OptionStore model
// with an STL-like container interface.  You can act on it like a
// sequence of items.
// Usually you get an OptionStore_Item_Collection by calling the items()
// method on a OptionStore model.  You can also call the items() method
// on an OptionComboBox; it simply redirects the call to the associated
// OptionStore model.
class OptionStore_Item_Collection
{
public:
	explicit OptionStore_Item_Collection(const Glib::RefPtr<OptionStore>& ref_model);

	void push_front(const Glib::ustring& text, bool sensitive = true);
	void push_back(const Glib::ustring& text, bool sensitive = true);

	void insert(const OptionStore_Item& item,
	            const Glib::ustring& text,
	            bool sensitive = true);
	void insert(unsigned position,
	            const Glib::ustring& text,
	            bool sensitive = true);

	void pop_front();
	void pop_back();

	void erase(const OptionStore_Item& item);
	void erase(unsigned position);
	void clear();

	OptionStore_Item front();
	OptionStore_Item back();

	OptionStore_Item at(unsigned position);
	OptionStore_Item operator[](unsigned position);

	unsigned size() const;

	OptionStore_Item_Const front() const;
	OptionStore_Item_Const back() const;

	OptionStore_Item_Const at(unsigned position) const;
	OptionStore_Item_Const operator[] (unsigned position) const;

private:
	Glib::RefPtr<OptionStore> m_ref_model;
};


// OptionStore is the model that backs the data for OptionComboBox.
// It's a specialized Gtk::ListStore with a string slot for row text
// and a boolean slot for row sensitivity (if it's false the row is
// "grayed out").
// Although you can call any Gtk::ListStore method (from which
// OptionStore inherits) it is usually convenient to call the items()
// method to get a OptionStore_Item_Collection object that provides
// a nice, high level interface.
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

	class Slots
	 : public Gtk::TreeModelColumnRecord
	{
	public:
		Gtk::TreeModelColumn<Glib::ustring> m_text;
		Gtk::TreeModelColumn<bool>          m_sensitive;

		Slots()
		{
			add(m_text);
			add(m_sensitive);
		}
	};
	static Slots *m_slots;

protected:
	explicit OptionStore();
};


// OptionComboBox is a specialized ComboBox that shows a list of rows,
// some of which may be selectively grayed out.  It is commonly used to
// display a list of options where not all of the options can be
// selected in all cases.
class OptionComboBox
 : public Gtk::ComboBox
{
public:
	typedef Gtk::ComboBox Base;

	explicit OptionComboBox();
	explicit OptionComboBox(const Glib::RefPtr<OptionStore>& ref_model);

	void set_model(const Glib::RefPtr<OptionStore>& ref_model);

	Glib::RefPtr<OptionStore>   get_model();
	OptionStore_Item_Collection items();
	OptionStore_Item            get_active();

	Glib::RefPtr<const OptionStore>   get_model()  const;
	OptionStore_Item_Collection_Const items()      const;
	OptionStore_Item_Const            get_active() const;

protected:
	void pack_cell_renderers();

protected:
	Glib::RefPtr<OptionStore> m_ref_model;
};


}  // namespace GParted


#endif /* GPARTED_OPTIONCOMBOBOX_H */
