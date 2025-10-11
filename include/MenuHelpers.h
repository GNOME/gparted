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


#ifndef GPARTED_MENUHELPERS_H
#define GPARTED_MENUHELPERS_H

#include <glibmm/ustring.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/imagemenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/checkmenuitem.h>
#include <sigc++/slot.h>


namespace GParted
{


namespace Menu_Helpers
{


typedef sigc::slot<void> CallSlot;


class MenuElem
 : public Gtk::MenuItem
{
public:
	MenuElem(const Glib::ustring&  label,
	         const Gtk::AccelKey&  key,
	         const CallSlot&       slot = CallSlot());
	MenuElem(const Glib::ustring&  label,
	         const CallSlot&       slot = CallSlot());
	MenuElem(const Glib::ustring&  label,
	               Gtk::Menu&      submenu);
};


class ImageMenuElem
 : public Gtk::ImageMenuItem
{
public:
	ImageMenuElem(const Glib::ustring&  label,
	              const Gtk::AccelKey&  key,
	                    Gtk::Widget&    image_widget,
	              const CallSlot&       slot = CallSlot());
	ImageMenuElem(const Glib::ustring&  label,
	                    Gtk::Widget&    image_widget,
	              const CallSlot&       slot = CallSlot());
	ImageMenuElem(const Glib::ustring&  label,
	                    Gtk::Widget&    image_widget,
	                    Gtk::Menu&      submenu);
};


class SeparatorElem
 : public Gtk::SeparatorMenuItem
{
public:
	SeparatorElem();
};


class StockMenuElem
 : public Gtk::ImageMenuItem
{
public:
	StockMenuElem(const Gtk::StockID&   stock_id,
	              const Gtk::AccelKey&  key,
	              const CallSlot&       slot = CallSlot());
	StockMenuElem(const Gtk::StockID&   stock_id,
	              const CallSlot&       slot = CallSlot());
	StockMenuElem(const Gtk::StockID&   stock_id,
	                    Gtk::Menu&      submenu);
};


class CheckMenuElem
 : public Gtk::CheckMenuItem
{
public:
	CheckMenuElem(const Glib::ustring&  label,
	              const Gtk::AccelKey&  key,
	              const CallSlot&       slot = CallSlot());
	CheckMenuElem(const Glib::ustring&  label,
	              const CallSlot&       slot = CallSlot());
};


}  // namespace Menu_Helpers


}  // namespace GParted


#endif /* GPARTED_MENUHELPERS_H */
