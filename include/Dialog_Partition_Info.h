/* Copyright (C) 2004 Bart
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

#ifndef GPARTED_DIALOG_PARTITION_INFO_H
#define GPARTED_DIALOG_PARTITION_INFO_H

//what kind of info would one prefer to see here?
//my guess is, it's best to keep the amount of info minimal and wait for users requests

#include "Partition.h"
#include "i18n.h"

#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/scrolledwindow.h>


#define BORDER 8


namespace GParted
{


class Dialog_Partition_Info : public Gtk::Dialog
{
public:
	Dialog_Partition_Info( const Partition & partition );

private:
	void init_drawingarea() ;
	void Display_Info();

	// Signal handler
	bool drawingarea_on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

	const Partition& m_partition;  // (Alias to element in Win_GParted::m_display_device.partitions[] vector).

	Gtk::ScrolledWindow m_info_scrolled;
	Gtk::Box            m_info_msg_vbox;
	Gtk::DrawingArea    m_drawingarea;

	Glib::RefPtr<Pango::Layout> m_text_overlay;

	Gdk::RGBA m_color_partition;
	Gdk::RGBA m_color_used;
	Gdk::RGBA m_color_unused;
	Gdk::RGBA m_color_unallocated;
	Gdk::RGBA m_color_text;

	int used        = 0;
	int unused      = 0;
	int unallocated = 0;
};


}  // namespace GParted


#endif /* GPARTED_DIALOG_PARTITION_INFO_H */
