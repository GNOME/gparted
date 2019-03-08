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

#include <gtkmm/checkmenuitem.h>
#include <gtkmm/separatormenuitem.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/accellabel.h>
#include <gtkmm/stock.h>
#include <gtkmm/settings.h>

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
 : Gtk::MenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_accel_key(key);

	Gtk::Box *box_widget = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	box_widget->set_spacing(6);

	Gtk::AccelLabel *label_widget = Gtk::manage(new Gtk::AccelLabel(label));
	label_widget->set_use_underline(true);
	label_widget->set_accel_widget(*this);
#ifdef GTKMM_HAVE_LABEL_SET_XALIGN
	label_widget->set_xalign(0.0);
#else
	label_widget->set_alignment(0.0, 0.5);
#endif

	box_widget->add(image_widget);
	box_widget->pack_end(*label_widget, true, true, 0);
	add(*box_widget);

	show_all();

	g_object_bind_property(Gtk::Settings::get_default()->gobj(),
	                       "gtk-menu-images",
	                       image_widget.gobj(),
	                       "visible",
	                       (GBindingFlags) (G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE));
}


ImageMenuElem::ImageMenuElem(const Glib::ustring& label,
                             Gtk::Widget& image_widget,
                             const CallSlot& slot)
 : Gtk::MenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	Gtk::Box *box_widget = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	box_widget->set_spacing(6);

	Gtk::Label *label_widget = Gtk::manage(new Gtk::Label(label));
	label_widget->set_use_underline(true);

	box_widget->add(image_widget);
	box_widget->add(*label_widget);
	add(*box_widget);

	show_all();

	g_object_bind_property(Gtk::Settings::get_default()->gobj(),
	                       "gtk-menu-images",
	                       image_widget.gobj(),
	                       "visible",
	                       (GBindingFlags) (G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE));
}


ImageMenuElem::ImageMenuElem(const Glib::ustring& label,
                             Gtk::Widget& image_widget,
                             Gtk::Menu& submenu)
 : Gtk::MenuItem()
{
	set_submenu(submenu);

	Gtk::Box *box_widget = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	box_widget->set_spacing(6);

	Gtk::Label *label_widget = Gtk::manage(new Gtk::Label(label));
	label_widget->set_use_underline(true);

	box_widget->add(image_widget);
	box_widget->add(*label_widget);
	add(*box_widget);

	show_all();

	g_object_bind_property(Gtk::Settings::get_default()->gobj(),
	                       "gtk-menu-images",
	                       image_widget.gobj(),
	                       "visible",
	                       (GBindingFlags) (G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE));
}


SeparatorElem::SeparatorElem()
 : Gtk::SeparatorMenuItem()
{
	show_all();
}


StockMenuElem::StockMenuElem(const Gtk::StockID& stock_id,
                             const Gtk::AccelKey& key,
                             const CallSlot& slot)
 : Gtk::MenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	set_accel_key(key);

	Gtk::Box *box_widget = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	box_widget->set_spacing(6);

	Gtk::AccelLabel *label_widget = Gtk::manage(new Gtk::AccelLabel());
	Gtk::StockItem stock;
	if (Gtk::Stock::lookup(stock_id, stock))
		label_widget->set_label(stock.get_label());
	label_widget->set_use_underline(true);
	label_widget->set_accel_widget(*this);
#ifdef GTKMM_HAVE_LABEL_SET_XALIGN
	label_widget->set_xalign(0.0);
#else
	label_widget->set_alignment(0.0, 0.5);
#endif

	Gtk::Image *image_widget = Gtk::manage(new Gtk::Image(stock_id, Gtk::ICON_SIZE_MENU));

	box_widget->add(*image_widget);
	box_widget->pack_end(*label_widget, true, true, 0);
	add(*box_widget);

	show_all();

	g_object_bind_property(Gtk::Settings::get_default()->gobj(),
	                       "gtk-menu-images",
	                       image_widget->gobj(),
	                       "visible",
	                       (GBindingFlags) (G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE));
}


StockMenuElem::StockMenuElem(const Gtk::StockID& stock_id,
                             const CallSlot& slot)
 : Gtk::MenuItem()
{
	if (slot)
		signal_activate().connect(slot);

	Gtk::StockItem stock;
	if (Gtk::Stock::lookup(stock_id, stock))
		set_accel_key(Gtk::AccelKey(stock.get_keyval(),
		                            stock.get_modifier()));

	Gtk::Box *box_widget = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	box_widget->set_spacing(6);

	Gtk::Label *label_widget = Gtk::manage(new Gtk::Label(stock_id.get_string()));
	if (Gtk::Stock::lookup(stock_id, stock))
		label_widget->set_label(stock.get_label());
	label_widget->set_use_underline(true);

	Gtk::Image *image_widget = Gtk::manage(new Gtk::Image(stock_id, Gtk::ICON_SIZE_MENU));

	box_widget->add(*image_widget);
	box_widget->add(*label_widget);
	add(*box_widget);

	show_all();

	g_object_bind_property(Gtk::Settings::get_default()->gobj(),
	                       "gtk-menu-images",
	                       image_widget->gobj(),
	                       "visible",
	                       (GBindingFlags) (G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE));
}


StockMenuElem::StockMenuElem(const Gtk::StockID& stock_id,
                             Gtk::Menu& submenu)
 : Gtk::MenuItem()
{
	set_submenu(submenu);

	Gtk::Box *box_widget = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	box_widget->set_spacing(6);

	Gtk::Label *label_widget = Gtk::manage(new Gtk::Label(stock_id.get_string()));
	Gtk::StockItem stock;
	if (Gtk::Stock::lookup(stock_id, stock))
		label_widget->set_label(stock.get_label());
	label_widget->set_use_underline(true);

	Gtk::Image *image_widget = Gtk::manage(new Gtk::Image(stock_id, Gtk::ICON_SIZE_MENU));

	box_widget->add(*image_widget);
	box_widget->add(*label_widget);
	add(*box_widget);

	show_all();

	g_object_bind_property(Gtk::Settings::get_default()->gobj(),
	                       "gtk-menu-images",
	                       image_widget->gobj(),
	                       "visible",
	                       (GBindingFlags) (G_BINDING_DEFAULT | G_BINDING_SYNC_CREATE));
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
