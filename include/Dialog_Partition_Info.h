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

#include <gtkmm/dialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/frame.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/table.h>

#define BORDER 8

namespace GParted
{

class Dialog_Partition_Info : public Gtk::Dialog
{
public:
	Dialog_Partition_Info( const Partition & partition );
	~Dialog_Partition_Info();

private:
	void init_drawingarea() ;
	void Display_Info();

	//signalhandlers
	void drawingarea_on_realize();
	bool drawingarea_on_expose( GdkEventExpose *ev );

	const Partition & partition;  // (Alias to element in Win_GParted::display_partitions[] vector).

	Gtk::HBox *hbox ;
	Gtk::DrawingArea drawingarea ;
	Gtk::Frame *frame ;
	Gtk::VBox info_msg_vbox ;
	Gtk::ScrolledWindow info_scrolled ;

	Glib::RefPtr<Gdk::GC> gc;
	Glib::RefPtr<Pango::Layout> pango_layout;

	Gdk::Color color_partition, color_used, color_unused, color_unallocated, color_text ;

	int used, unused, unallocated ;
};

} //GParted

#endif /* GPARTED_DIALOG_PARTITION_INFO_H */
