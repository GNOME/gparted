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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DIALOG_PARTITION_INFO
#define DIALOG_PARTITION_INFO

//what kind of info would one prefer to see here?
//my guess is, it's best to keep the amount of info minimal and wait for users requests

#include "../include/Partition.h"

#include <gtkmm/dialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/label.h>
#include <gtkmm/frame.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/table.h>

#include <vector>
#include <fstream>

#define BORDER 8
 
namespace GParted
{ 

class Dialog_Partition_Info : public Gtk::Dialog
{
public:
	Dialog_Partition_Info( const Partition & );
	~Dialog_Partition_Info();

private:
	void init_drawingarea() ;
	void Display_Info();
	void Find_Status() ;
	Gtk::Label * mk_label( const Glib::ustring & text ) ;

	//signalhandlers
	void drawingarea_on_realize(  );
	bool drawingarea_on_expose( GdkEventExpose *  );

	Partition partition ;

	Gtk::HBox *hbox ;
	Gtk::DrawingArea drawingarea ;
	Gtk::Frame *frame ;
	Gtk::Label *label ;
	Gtk::Image *image;
	Gtk::Table *table;

	Glib::RefPtr<Gdk::GC> gc;
	Glib::RefPtr<Pango::Layout> pango_layout;

	Gdk::Color color_partition, color_used, color_unused, color_text ;

	int used,unused ;
	std::ostringstream os, os_percent;
	 
};

} //GParted

#endif //DIALOG_PARTITION_INFO
