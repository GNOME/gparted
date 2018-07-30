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

#include <stdexcept>


namespace GParted
{

/* NOTE: As per GTKMM3 documentation, slots can be made static and shared for all instances.
         See reference documentation for Gtk::TreeModelColumn and Gtk::TreeModelColumnRecord. */

Gtk::TreeModelColumn< Glib::ustring > OptionStore::Slots::Text;
Gtk::TreeModelColumn< bool >          OptionStore::Slots::Sensitive;

Gtk::TreeModel::ColumnRecord          OptionStore::Slots::record_;

}


namespace GParted
{


OptionStoreItem::OptionStoreItem( const Glib::RefPtr<OptionStore>& ref_model,
                                  const Gtk::TreeModel::iterator& iter )
 : m_rowreference ( ref_model, ref_model ->get_path( iter ) )
{}
OptionStoreItem::OptionStoreItem( const Glib::RefPtr<OptionStore>& ref_model,
                                  const Gtk::TreeModel::Path& path )
 : m_rowreference( ref_model, path )
{}
OptionStoreItem::OptionStoreItem( const Gtk::TreeModel::RowReference& rowreference )
 : m_rowreference( rowreference )
{}


void OptionStoreItem::set( const Glib::ustring& text,
                           bool sensitive )
{
	Gtk::TreeModel::iterator iter = *this;
	(*iter)[ OptionStore::Slots::Text ] = text;
	(*iter)[ OptionStore::Slots::Sensitive ] = sensitive;
}


void OptionStoreItem::set_text( const Glib::ustring& text )
{
	Gtk::TreeModel::iterator iter = *this;
	(*iter)[ OptionStore::Slots::Text ] = text;
}


void OptionStoreItem::set_sensitive( bool sensitive )
{
	Gtk::TreeModel::iterator iter = *this;
	(*iter)[ OptionStore::Slots::Sensitive ] = sensitive;
}


Glib::ustring OptionStoreItem::text() const
{
	Gtk::TreeModel::const_iterator iter = *this;
	return (*iter)[ OptionStore::Slots::Text ];
}


bool OptionStoreItem::sensitive() const
{
	Gtk::TreeModel::const_iterator iter = *this;
	return (*iter)[ OptionStore::Slots::Sensitive ];
}


Gtk::TreeModel::iterator OptionStoreItem::to_iterator()
{
	Gtk::TreeModel::Path path = m_rowreference.get_path();

	Glib::RefPtr< OptionStore > ref_model =
		Glib::RefPtr< OptionStore >::cast_dynamic( m_rowreference.get_model() );

	if ( !ref_model )
		throw std::runtime_error ( "incompatible Gtk::TreeModel type." );

	return ref_model->get_iter( path );
}


Gtk::TreeModel::const_iterator OptionStoreItem::to_iterator() const
{
	Gtk::TreeModel::Path path = m_rowreference.get_path();

	Glib::RefPtr< const OptionStore > ref_model =
		Glib::RefPtr< const OptionStore >::cast_dynamic( m_rowreference.get_model() );

	if ( !ref_model )
		throw std::runtime_error ( "incompatible Gtk::TreeModel type." );

	/* NOTE: To convert the Gtk::TreeModel::Path to a Gtk::TreeModel::iterator we cannot
	         just call Gtk::TreeModel::get_iter() because there is no 'get_iter() const'

	         Added in GTKMM4:
	         see bug report https://bugzilla.gnome.org/show_bug.cgi?id=134520
	         and commit https://gitlab.gnome.org/GNOME/gtkmm/commit/acd6dcfb

	         We use Gtk::TreeNodeChildren API here. */

	int position = path.front();

	// check conversion of signed int to unsigned
	if ( position < 0)
		throw std::runtime_error ( "invalid integral cast." );

	return ref_model->children()[static_cast< unsigned >( position )];
}


OptionStoreItemsCollection::OptionStoreItemsCollection( const Glib::RefPtr<OptionStore>& ref_model )
 : m_ref_model( ref_model )
{}


void OptionStoreItemsCollection::push_front( const Glib::ustring& text,
                                             bool sensitive )
{
	Gtk::TreeModel::iterator iter = m_ref_model ->prepend();
	(*iter)[ OptionStore::Slots::Text ] = text;
	(*iter)[ OptionStore::Slots::Sensitive ] = sensitive;
}


void OptionStoreItemsCollection::push_back( const Glib::ustring& text,
                                            bool sensitive )
{
	Gtk::TreeModel::iterator iter = m_ref_model ->append();
	(*iter)[ OptionStore::Slots::Text ] = text;
	(*iter)[ OptionStore::Slots::Sensitive ] = sensitive;
}


void OptionStoreItemsCollection::insert( const OptionStoreItem& item,
	                                     const Glib::ustring& text,
	                                     bool sensitive )
{
	Gtk::TreeModel::iterator previous_iter = item .to_iterator();
	Gtk::TreeModel::iterator iter = m_ref_model ->insert( previous_iter );
	(*iter)[ OptionStore::Slots::Text ] = text;
	(*iter)[ OptionStore::Slots::Sensitive ] = sensitive;
}


void OptionStoreItemsCollection::insert( unsigned position,
	                                     const Glib::ustring& text,
	                                     bool sensitive )
{
	Gtk::TreeModel::iterator previous_iter = m_ref_model ->children()[ position ];
	Gtk::TreeModel::iterator iter = m_ref_model ->insert( previous_iter );
	(*iter)[ OptionStore::Slots::Text ] = text;
	(*iter)[ OptionStore::Slots::Sensitive ] = sensitive;
}


void OptionStoreItemsCollection::pop_front()
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children() .begin();
	m_ref_model ->erase( iter );
}


void OptionStoreItemsCollection::pop_back()
{
	/* NOTE: To get an iterator to the last item we do not use Gtk::TreeNodeChildren::rbegin()
	         because Gtk::TreeModel::reverse_iterator has been deprecated in GTKMM3.
	         For that, see https://bugzilla.gnome.org/show_bug.cgi?id=554889 */

	Gtk::TreeModel::iterator iter = m_ref_model ->children() .begin();

	for (unsigned i = 1; i < m_ref_model->children().size(); ++i)
		++iter;

	m_ref_model ->erase( iter );
}


void OptionStoreItemsCollection::erase( const OptionStoreItem& item )
{
	Gtk::TreeModel::iterator iter = item.to_iterator();
	m_ref_model ->erase( iter );
}


void OptionStoreItemsCollection::erase( unsigned position )
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children()[ position ];
	m_ref_model ->erase( iter );
}


void OptionStoreItemsCollection::clear()
{
	m_ref_model ->clear();
}


OptionStoreItem OptionStoreItemsCollection::front()
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children() .begin();
	return OptionStoreItem(m_ref_model, iter);
}


OptionStoreItem OptionStoreItemsCollection::back()
{
	/* NOTE: To get an iterator to the last item we do not use Gtk::TreeNodeChildren::rbegin()
	         because Gtk::TreeModel::reverse_iterator has been deprecated in GTKMM3.
	         For that, see https://bugzilla.gnome.org/show_bug.cgi?id=554889 */

	Gtk::TreeModel::iterator iter = m_ref_model ->children() .begin();

	for (unsigned i = 1; i < m_ref_model->children().size(); ++i)
		++iter;

	return OptionStoreItem(m_ref_model, iter);
}


OptionStoreItem OptionStoreItemsCollection::at(unsigned position)
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children()[ position ];
	return OptionStoreItem(m_ref_model, iter);
}


OptionStoreItem OptionStoreItemsCollection::operator[](unsigned position)
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children()[ position ];
	return OptionStoreItem(m_ref_model, iter);
}


unsigned OptionStoreItemsCollection::size() const
{
	return m_ref_model ->children() .size();
}


OptionStoreItemConst OptionStoreItemsCollection::front() const
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children() .begin();
	return OptionStoreItem(m_ref_model, iter);
}


OptionStoreItemConst OptionStoreItemsCollection::back() const
{
	/* NOTE: To get an iterator to the last item we do not use Gtk::TreeNodeChildren::rbegin()
	         because Gtk::TreeModel::reverse_iterator has been deprecated in GTKMM3.
	         For that, see https://bugzilla.gnome.org/show_bug.cgi?id=554889 */

	Gtk::TreeModel::iterator iter = m_ref_model ->children() .begin();

	for (unsigned i = 1; i < m_ref_model->children().size(); ++i)
		++iter;

	return OptionStoreItem(m_ref_model, iter);
}


OptionStoreItemConst OptionStoreItemsCollection::at(unsigned position) const
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children()[ position ];
	return OptionStoreItem(m_ref_model, iter);
}


OptionStoreItemConst OptionStoreItemsCollection::operator[](unsigned position) const
{
	Gtk::TreeModel::iterator iter = m_ref_model ->children()[ position ];
	return OptionStoreItem(m_ref_model, iter);
}


OptionStore::OptionStore()
 : Glib::ObjectBase("GPartedOptionStore")
{
	if (!Slots::record_.size())
	{
		Slots::record_.add( Slots::Text );
		Slots::record_.add( Slots::Sensitive );
	}

	set_column_types( Slots::record_ );
}


Glib::RefPtr<OptionStore> OptionStore::create()
{
	return Glib::RefPtr<OptionStore>( new OptionStore() );
}


OptionStoreItemsCollection OptionStore::items()
{
	return OptionStoreItemsCollection( Glib::RefPtr<OptionStore>( this ) );
}


OptionStoreItemsCollectionConst OptionStore::items() const
{
	/* NOTE: This const_cast<> is safe because OptionStore objects are all originally
	         instantiated as non-const. OptionStore objects can only be allocated by
	         OptionStore::create() static member function and it allocates non const
	         instances. Also, even if we cast away constness, it is taken read only
	         and never modified. */

	return OptionStoreItemsCollection( Glib::RefPtr<OptionStore>( const_cast<OptionStore*>( this ) ) );
}


OptionComboBox::OptionComboBox()
 : Glib::ObjectBase("GPartedOptionComboBox")
{
	set_model( OptionStore::create() );

	pack_cell_renderers();
}


OptionComboBox::OptionComboBox(const Glib::RefPtr<OptionStore>& ref_optionstore)
 : Glib::ObjectBase("GPartedOptionComboBox")
{
	set_model( ref_optionstore );

	pack_cell_renderers();
}


void OptionComboBox::pack_cell_renderers()
{
	Gtk::CellLayout::clear();

	Gtk::CellRendererText *cell = manage( new Gtk::CellRendererText() );
	pack_start( *cell );
	add_attribute( *cell, "text", OptionStore::Slots::Text );
	add_attribute( *cell, "sensitive", OptionStore::Slots::Sensitive );
}


OptionStoreItemsCollection OptionComboBox::items()
{
	return OptionStoreItemsCollection( util_get_optionstore() );
}


OptionStoreItemsCollectionConst OptionComboBox::items() const
{
	return OptionStoreItemsCollection( util_get_optionstore() );
}


void OptionComboBox::set_model( const Glib::RefPtr<OptionStore>& ref_model )
{
	Base::set_model( ref_model );
}


Glib::RefPtr<OptionStore> OptionComboBox::get_model()
{
	return util_get_optionstore();
}


Glib::RefPtr<const OptionStore> OptionComboBox::get_model() const
{
	return util_get_optionstore();
}


OptionStore::Item OptionComboBox::get_active()
{
	return OptionStore::Item( util_get_optionstore(), Base::get_active() );
}


OptionStore::ItemConst OptionComboBox::get_active() const
{
	return OptionStore::Item( util_get_optionstore(), Base::get_active() );
}


Glib::RefPtr<OptionStore> OptionComboBox::util_get_optionstore() const
{
	// this cast_const is safe
	Glib::RefPtr<Gtk::TreeModel> ref_model =
		Glib::RefPtr<Gtk::TreeModel>::cast_const( Base::get_model() );

	if (!ref_model)
		return Glib::RefPtr<OptionStore>( NULL );

	Glib::RefPtr<OptionStore> ref_optionstore =
		Glib::RefPtr<OptionStore>::cast_dynamic( ref_model );

	if (!ref_optionstore)
		throw std::runtime_error( "incompatible Gtk::TreeModel type." );

	return ref_optionstore;
}

}//GParted
