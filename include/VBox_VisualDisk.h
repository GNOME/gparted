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

#ifndef  VBOX_VISUALDISK
#define VBOX_VISUALDISK

#include "../include/Partition.h"
#include "../include/Device.h"

#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/tooltips.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/drawingarea.h>

#define BORDER 8

namespace GParted
{

class VBox_VisualDisk : public Gtk::VBox
{
	//struct which contains visual information about the partitions. This prevents recalculation of everything after an expose en therefore saves performance..
	struct Visual_Partition
	{
		//2 attributes used for selection ( see Set_Selected() an drawingarea_on_expose() for more info )
		Sector sector_start; 
		int index;
		
		int length; //pixels
		int used;  //pixels
		int height;  //pixels
		int text_y; //pixels
		Gdk::Color color_fs;
		Gtk::DrawingArea *drawingarea;
		Glib::RefPtr<Gdk::GC> gc;
		Glib::RefPtr<Pango::Layout> pango_layout;
	};
	
public:
	VBox_VisualDisk( const std::vector<Partition> & partitions, const Sector device_length );
	~VBox_VisualDisk();
	void Set_Selected( const Partition & );

	
	//public signal for interclass communication
	sigc::signal<void,GdkEventButton *, const Partition &> signal_mouse_click;
	
	
private:
	void Build_Visual_Disk( int ) ; //i still dream of some fully resizeable visualdisk.... 
	void Build_Legend( ) ;
	void Add_Legend_Item( const Glib::ustring & filesystem, const Gdk::Color & color );
	
	//signal handlers
	void drawingarea_on_realize( Visual_Partition *  );
	bool drawingarea_on_expose( GdkEventExpose *, Visual_Partition* );
	bool on_drawingarea_button_press( GdkEventButton *, const  Partition & );

	std::vector<Partition> partitions;
	Sector device_length;	

	Gtk::Frame *frame_disk_legend ;
	Gtk::HBox hbox_disk_main, *hbox_disk, *hbox_extended, hbox_legend_main, *hbox_legend;
	Gtk::CheckButton checkbutton_filesystem;
	Gtk::Tooltips tooltips;
		
	Visual_Partition *visual_partition;
	std::vector <Visual_Partition *> visual_partitions;
	Gtk::EventBox * eventbox_extended;
	Gdk::Color color_used, color_unused, color_text;


	Glib::ustring str_temp ;
	int temp,selected_partition;
	Gtk::Entry *entry_temp;
};

} //GParted
#endif //VBOX_VISUALDISK
