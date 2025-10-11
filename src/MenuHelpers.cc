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
#include "Utils.h"

#include <gtkmm/imagemenuitem.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/stock.h>


namespace GParted
{


namespace Menu_Helpers
{


MenuElem::MenuElem(const Glib::ustring& label,
                   const Gtk::AccelKey& key,
                   const CallSlot& slot)
 : Gtk::MenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_accel_key(key);

	set_label(label);
	set_use_underline(true);

	show_all();
}


MenuElem::MenuElem(const Glib::ustring & label,
                   const CallSlot & slot)
 : Gtk::MenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_label(label);
	set_use_underline(true);

	show_all();
}


MenuElem::MenuElem(const Glib::ustring & label,
                   Gtk::Menu & submenu)
 : Gtk::MenuItem()
{
	set_submenu(submenu);

	set_label(label);
	set_use_underline(true);

	show_all();
}


ImageMenuElem::ImageMenuElem(const Glib::ustring& label,
                             const Gtk::AccelKey& key,
                             Gtk::Widget& image_widget,
                             const CallSlot& slot)
 : ImageMenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_accel_key(key);

	set_image(image_widget);
	set_label(label);
	set_use_underline(true);
	set_always_show_image(true);

	show_all();
}


ImageMenuElem::ImageMenuElem(const Glib::ustring& label,
                             Gtk::Widget& image_widget,
                             const CallSlot& slot)
 : ImageMenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_image(image_widget);
	set_label(label);
	set_use_underline(true);
	set_always_show_image(true);

	show_all();
}


ImageMenuElem::ImageMenuElem(const Glib::ustring& label,
                             Gtk::Widget& image_widget,
                             Gtk::Menu& submenu)
 : ImageMenuItem()
{
	set_submenu(submenu);

	set_image(image_widget);
	set_label(label);
	set_use_underline(true);
	set_always_show_image(true);

	show_all();
}


SeparatorElem::SeparatorElem()
 : Gtk::SeparatorMenuItem()
{
	show_all();
}


StockMenuElem::StockMenuElem(const Gtk::StockID& stock_id,
                             const Gtk::AccelKey& key,
                             const CallSlot& slot)
 : Gtk::ImageMenuItem()
{
	Gtk::StockItem stock;

	if (slot)
		signal_activate().connect(slot);

	set_accel_key(key);

	Gtk::Image *image = Utils::mk_image(stock_id, Gtk::ICON_SIZE_MENU);
	set_image(*image);
	set_always_show_image(true);

	if (Gtk::Stock::lookup(stock_id, stock))
	{
		set_label(Utils::get_stock_label(stock_id));
		set_use_underline(true);
	}

	show_all();
}


StockMenuElem::StockMenuElem(const Gtk::StockID& stock_id,
                             const CallSlot& slot)
 : Gtk::ImageMenuItem()
{
	Gtk::StockItem stock;

	if (slot)
		signal_activate().connect(slot);

	Gtk::Image *image = Utils::mk_image(stock_id, Gtk::ICON_SIZE_MENU);
	set_image(*image);
	set_always_show_image(true);

	if (Gtk::Stock::lookup(stock_id, stock))
	{
		set_accel_key(Gtk::AccelKey(stock.get_keyval(),
		                            stock.get_modifier()));

		set_label(Utils::get_stock_label(stock_id));
		set_use_underline(true);
	}

	show_all();
}


StockMenuElem::StockMenuElem(const Gtk::StockID& stock_id,
                             Gtk::Menu& submenu)
 : Gtk::ImageMenuItem()
{
	Gtk::StockItem stock;

	set_submenu(submenu);

	Gtk::Image *image = Utils::mk_image(stock_id, Gtk::ICON_SIZE_MENU);
	set_image(*image);
	set_always_show_image(true);

	if (Gtk::Stock::lookup(stock_id, stock))
	{
		set_accel_key(Gtk::AccelKey(stock.get_keyval(),
		                            stock.get_modifier()));

		set_label(Utils::get_stock_label(stock_id));
		set_use_underline(true);
	}

	show_all();
}


CheckMenuElem::CheckMenuElem(const Glib::ustring& label,
                             const Gtk::AccelKey& key,
                             const CallSlot& slot)
 : Gtk::CheckMenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_accel_key(key);

	set_label(label);
	set_use_underline(true);

	show_all();
}


CheckMenuElem::CheckMenuElem(const Glib::ustring& label,
                             const CallSlot& slot)
 : Gtk::CheckMenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_label(label);
	set_use_underline(true);

	show_all();
}


}  // namespace Menu_Helpers


}  // namespace GParted
