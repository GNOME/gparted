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
                             const Glib::ustring& icon_name,
                             const Gtk::AccelKey& key,
                             const CallSlot& slot)
 : ImageMenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_accel_key(key);

	set_image(*Utils::mk_image(icon_name, Gtk::ICON_SIZE_MENU));
	set_label(label);
	set_use_underline(true);
	set_always_show_image(true);

	show_all();
}


ImageMenuElem::ImageMenuElem(const Glib::ustring& label,
                             const Glib::ustring& icon_name,
                             const CallSlot& slot)
 : ImageMenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_image(*Utils::mk_image(icon_name, Gtk::ICON_SIZE_MENU));
	set_label(label);
	set_use_underline(true);
	set_always_show_image(true);

	show_all();
}


ImageMenuElem::ImageMenuElem(const Glib::ustring& label,
                             const Glib::ustring& icon_name,
                             Gtk::Menu& submenu)
 : ImageMenuItem()
{
	set_submenu(submenu);

	set_image(*Utils::mk_image(icon_name, Gtk::ICON_SIZE_MENU));
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


} //Menu_Helpers
} //GParted
