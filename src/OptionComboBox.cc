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

#include <glibmm/ustring.h>
#include <gtkmm/cellrenderertext.h>
#include <stdexcept>


namespace GParted
{


// NOTE: As stated in Gtkmm3 documentation, slots can be shared for all model instances.
// See: Class Reference for Gtk::TreeModelColumn and Gtk::TreeModelColumnRecord.
//     https://developer.gnome.org/gtkmm/3.22/classGtk_1_1TreeModelColumnRecord.html#details
OptionStore::Slots* OptionStore::m_slots = nullptr;


OptionStore_Item::OptionStore_Item(const Glib::RefPtr<OptionStore>& ref_model,
                                   const Gtk::TreeModel::iterator& iter)
 : m_rowreference(ref_model, ref_model->get_path(iter))
{}


OptionStore_Item::OptionStore_Item(const Glib::RefPtr<OptionStore>& ref_model,
                                   const Gtk::TreeModel::Path& path)
 : m_rowreference(ref_model, path)
{}


OptionStore_Item::OptionStore_Item(const Gtk::TreeModel::RowReference& rowreference)
 : m_rowreference(rowreference)
{}


void OptionStore_Item::set(const Glib::ustring& text,
                           bool sensitive)
{
	Gtk::TreeModel::iterator iter = *this;
	(*iter)[OptionStore::m_slots->m_text] = text;
	(*iter)[OptionStore::m_slots->m_sensitive] = sensitive;
}


void OptionStore_Item::set_text(const Glib::ustring& text)
{
	Gtk::TreeModel::iterator iter = *this;
	(*iter)[OptionStore::m_slots->m_text] = text;
}


void OptionStore_Item::set_sensitive(bool sensitive)
{
	Gtk::TreeModel::iterator iter = *this;
	(*iter)[OptionStore::m_slots->m_sensitive] = sensitive;
}


Glib::ustring OptionStore_Item::text() const
{
	Gtk::TreeModel::const_iterator iter = *this;
	return (*iter)[OptionStore::m_slots->m_text];
}


bool OptionStore_Item::sensitive() const
{
	Gtk::TreeModel::const_iterator iter = *this;
	return (*iter)[OptionStore::m_slots->m_sensitive];
}


Gtk::TreeModel::iterator OptionStore_Item::to_iterator_() const
{
	Gtk::TreeModel::Path path = m_rowreference.get_path();

	Glib::RefPtr<OptionStore> ref_model =
		Glib::RefPtr<OptionStore>::cast_dynamic(m_rowreference.get_model());

	if (!ref_model)
		throw std::runtime_error ("incompatible Gtk::TreeModel type.");

	return ref_model->get_iter(path);
}


OptionStore_Item_Collection::OptionStore_Item_Collection(const Glib::RefPtr<OptionStore>& ref_model)
 : m_ref_model(ref_model)
{}


void OptionStore_Item_Collection::push_front(const Glib::ustring& text,
                                             bool sensitive)
{
	Gtk::TreeModel::iterator iter = m_ref_model->prepend();
	(*iter)[OptionStore::m_slots->m_text] = text;
	(*iter)[OptionStore::m_slots->m_sensitive] = sensitive;
}


void OptionStore_Item_Collection::push_back(const Glib::ustring& text,
                                            bool sensitive)
{
	Gtk::TreeModel::iterator iter = m_ref_model->append();
	(*iter)[OptionStore::m_slots->m_text] = text;
	(*iter)[OptionStore::m_slots->m_sensitive] = sensitive;
}


void OptionStore_Item_Collection::insert(const OptionStore_Item& item,
                                         const Glib::ustring& text,
                                         bool sensitive)
{
	Gtk::TreeModel::iterator previous_iter = item.to_iterator_();
	Gtk::TreeModel::iterator iter = m_ref_model->insert(previous_iter);
	(*iter)[OptionStore::m_slots->m_text] = text;
	(*iter)[OptionStore::m_slots->m_sensitive] = sensitive;
}


void OptionStore_Item_Collection::insert(unsigned position,
                                         const Glib::ustring& text,
                                         bool sensitive )
{
	Gtk::TreeModel::iterator previous_iter = m_ref_model->children()[position];
	Gtk::TreeModel::iterator iter = m_ref_model->insert(previous_iter);
	(*iter)[OptionStore::m_slots->m_text] = text;
	(*iter)[OptionStore::m_slots->m_sensitive] = sensitive;
}


void OptionStore_Item_Collection::pop_front()
{
	Gtk::TreeModel::iterator iter = m_ref_model->children().begin();
	m_ref_model->erase(iter);
}


void OptionStore_Item_Collection::pop_back()
{
	// NOTE: To get an iterator to the last item we do not use Gtk::TreeNodeChildren::rbegin()
	// because Gtk::TreeModel::reverse_iterator is broken and has been deprecated in Gtkmm3.
	//     https://bugzilla.gnome.org/show_bug.cgi?id=554889

	Gtk::TreeModel::iterator iter = m_ref_model->children().begin();

	for (unsigned i = 1; i < m_ref_model->children().size(); ++i)
		++iter;

	m_ref_model->erase(iter);
}


void OptionStore_Item_Collection::erase(const OptionStore_Item& item)
{
	Gtk::TreeModel::iterator iter = item.to_iterator_();
	m_ref_model->erase(iter);
}


void OptionStore_Item_Collection::erase(unsigned position)
{
	Gtk::TreeModel::iterator iter = m_ref_model->children()[position];
	m_ref_model->erase(iter);
}


void OptionStore_Item_Collection::clear()
{
	m_ref_model->clear();
}


OptionStore_Item OptionStore_Item_Collection::front()
{
	Gtk::TreeModel::iterator iter = m_ref_model->children().begin();
	return OptionStore_Item(m_ref_model, iter);
}


OptionStore_Item OptionStore_Item_Collection::back()
{
	// NOTE: To get an iterator to the last item we do not use Gtk::TreeNodeChildren::rbegin()
	// because Gtk::TreeModel::reverse_iterator is broken and has been deprecated in Gtkmm3.
	//     https://bugzilla.gnome.org/show_bug.cgi?id=554889

	Gtk::TreeModel::iterator iter = m_ref_model->children().begin();

	for (unsigned i = 1; i < m_ref_model->children().size(); ++i)
		++iter;

	return OptionStore_Item(m_ref_model, iter);
}


OptionStore_Item OptionStore_Item_Collection::at(unsigned position)
{
	Gtk::TreeModel::iterator iter = m_ref_model->children()[position];
	return OptionStore_Item(m_ref_model, iter);
}


OptionStore_Item OptionStore_Item_Collection::operator[](unsigned position)
{
	Gtk::TreeModel::iterator iter = m_ref_model->children()[position];
	return OptionStore_Item(m_ref_model, iter);
}


unsigned OptionStore_Item_Collection::size() const
{
	return m_ref_model->children().size();
}


OptionStore_Item_Const OptionStore_Item_Collection::front() const
{
	Gtk::TreeModel::iterator iter = m_ref_model->children().begin();
	return OptionStore_Item(m_ref_model, iter);
}


OptionStore_Item_Const OptionStore_Item_Collection::back() const
{
	// NOTE: To get an iterator to the last item we do not use Gtk::TreeNodeChildren::rbegin()
	// because Gtk::TreeModel::reverse_iterator is broken and has been deprecated in Gtkmm3.
	//     https://bugzilla.gnome.org/show_bug.cgi?id=554889

	Gtk::TreeModel::iterator iter = m_ref_model->children().begin();

	for (unsigned i = 1; i < m_ref_model->children().size(); ++i)
		++iter;

	return OptionStore_Item(m_ref_model, iter);
}


OptionStore_Item_Const OptionStore_Item_Collection::at(unsigned position) const
{
	Gtk::TreeModel::iterator iter = m_ref_model->children()[position];
	return OptionStore_Item(m_ref_model, iter);
}


OptionStore_Item_Const OptionStore_Item_Collection::operator[](unsigned position) const
{
	Gtk::TreeModel::iterator iter = m_ref_model->children()[position];
	return OptionStore_Item(m_ref_model, iter);
}


OptionStore::OptionStore()
 : Glib::ObjectBase("GParted_OptionStore")
{
	if (!m_slots)
		m_slots = new Slots();

	set_column_types(*m_slots);
}


Glib::RefPtr<OptionStore> OptionStore::create()
{
	return Glib::RefPtr<OptionStore>(new OptionStore());
}


OptionStore_Item_Collection OptionStore::items()
{
	return OptionStore_Item_Collection(Glib::RefPtr<OptionStore>(this));
}


OptionStore_Item_Collection_Const OptionStore::items() const
{
	OptionStore *this_ = const_cast<OptionStore*>(this);

	return OptionStore_Item_Collection(Glib::RefPtr<OptionStore>(this_));
}


OptionComboBox::OptionComboBox()
 : Glib::ObjectBase("GParted_OptionComboBox")
{
	OptionComboBox::set_model(OptionStore::create());

	pack_cell_renderers();
}


OptionComboBox::OptionComboBox(const Glib::RefPtr<OptionStore>& ref_model)
 : Glib::ObjectBase("GParted_OptionComboBox")
{
	OptionComboBox::set_model(ref_model);

	pack_cell_renderers();
}


void OptionComboBox::pack_cell_renderers()
{
	Gtk::CellLayout::clear();

	Gtk::CellRendererText* cell = Gtk::manage(new Gtk::CellRendererText());
	pack_start(*cell);
	add_attribute(*cell, "text", OptionStore::m_slots->m_text);
	add_attribute(*cell, "sensitive", OptionStore::m_slots->m_sensitive);
}


OptionStore_Item_Collection OptionComboBox::items()
{
	return OptionStore_Item_Collection(m_ref_model);
}


OptionStore_Item_Collection_Const OptionComboBox::items() const
{
	return OptionStore_Item_Collection(m_ref_model);
}


void OptionComboBox::set_model(const Glib::RefPtr<OptionStore>& ref_model)
{
	Base::set_model(ref_model);
	m_ref_model = ref_model;
}


Glib::RefPtr<OptionStore> OptionComboBox::get_model()
{
	return m_ref_model;
}


Glib::RefPtr<const OptionStore> OptionComboBox::get_model() const
{
	return m_ref_model;
}


OptionStore_Item OptionComboBox::get_active()
{
	return OptionStore_Item(m_ref_model, Base::get_active());
}


OptionStore_Item_Const OptionComboBox::get_active() const
{
	// NOTE: (for Gtkmm4) having a const_iterator, you can get an iterator with
	//     iter = m_ref_model->get_iter(m_ref_model->get_path(const_iter));

	return OptionStore_Item(m_ref_model, Base::get_active());
}


}  // namespace GParted
