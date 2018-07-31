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

#include "MenuHelpers.h"

#include <gtkmm/imagemenuitem.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/separatormenuitem.h>

namespace GParted
{
namespace Menu_Helpers
{

MenuElem::MenuElem( const Glib::ustring & label,
                         const Gtk::AccelKey & key,
                         const CallSlot & slot )
 : Gtk::MenuItem( label, true )
{
	if (slot)
		signal_activate() .connect( slot );

	set_accel_key( key );
	show();
}
MenuElem::MenuElem( const Glib::ustring & label,
                         const CallSlot & slot )
 : Gtk::MenuItem( label, true )
{
	if (slot)
		signal_activate() .connect( slot );
	show();
}
MenuElem::MenuElem( const Glib::ustring & label,
                         Gtk::Menu & submenu )
 : Gtk::MenuItem( label, true )
{
	set_submenu( submenu );
	show();
}

ImageMenuElem::ImageMenuElem( const Glib::ustring & label,
                              const Gtk::AccelKey & key,
                              Gtk::Widget & image_widget,
                              const CallSlot & slot )
 : ImageMenuItem( image_widget, label, true )
{
	if (slot)
		signal_activate() .connect( slot );
	
	set_accel_key( key );
	show();
}
ImageMenuElem::ImageMenuElem( const Glib::ustring & label,
                              Gtk::Widget & image_widget,
                              const CallSlot & slot )
 : ImageMenuItem( image_widget, label, true )
{
	if (slot)
		signal_activate() .connect( slot );
	show();
}
ImageMenuElem::ImageMenuElem( const Glib::ustring & label,
                              Gtk::Widget & image_widget,
                              Gtk::Menu & submenu )
 : ImageMenuItem( image_widget, label, true )
{
	set_submenu( submenu );
	show();
}

SeparatorElem::SeparatorElem( )
 : Gtk::SeparatorMenuItem( )
{
	show();
}

StockMenuElem::StockMenuElem( const Gtk::StockID & stock_id,
                              const Gtk::AccelKey & key,
                              const CallSlot & slot )
 : Gtk::ImageMenuItem( stock_id )
{
	if (slot)
		signal_activate() .connect( slot );
	
	set_accel_key( key );
	show();
}
StockMenuElem::StockMenuElem( const Gtk::StockID & stock_id,
                              const CallSlot & slot )
 : Gtk::ImageMenuItem( stock_id )
{
	if (slot)
		signal_activate() .connect( slot );
}
StockMenuElem::StockMenuElem( const Gtk::StockID & stock_id,
                              Gtk::Menu & submenu )
 : Gtk::ImageMenuItem( stock_id )
{
	set_submenu( submenu );
	show();
}

CheckMenuElem::CheckMenuElem( const Glib::ustring & label,
                              const Gtk::AccelKey & key,
                              const CallSlot & slot )
 : Gtk::CheckMenuItem( label, true )
{
	if (slot)
		signal_activate() .connect( slot );
	
	set_accel_key( key );
	show();
}
CheckMenuElem::CheckMenuElem( const Glib::ustring & label,
                              const CallSlot & slot )
 : Gtk::CheckMenuItem( label, true )
{
	if (slot)
		signal_activate() .connect( slot );
	show();
}

}
}//GParted
