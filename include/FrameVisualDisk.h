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

#ifndef FRAME_VISUALDISK
#define FRAME_VISUALDISK

#include "../include/Partition.h"

#include <gtkmm/frame.h>
#include <gtkmm/drawingarea.h>


namespace GParted
{

class FrameVisualDisk : public Gtk::Frame
{
public:
	FrameVisualDisk();
	~FrameVisualDisk();
	
	void load_partitions( const std::vector<Partition> & partitions, Sector device_length );
	void set_selected( const Partition & partition ) ;
	void clear() ;

	//public signal for interclass communication
	sigc::signal< void, const Partition &, bool > signal_partition_selected ;
	sigc::signal< void > signal_partition_activated ;
	sigc::signal< void, unsigned int, unsigned int > signal_popup_menu ;
	
private:
	struct visual_partition ; 

	//private functions	
	int get_total_separator_px( const std::vector<Partition> & partitions ) ;
	
	void set_static_data( const std::vector<Partition> & partitions, std::vector<visual_partition> & visual_partitions, Sector length ) ;
	int calc_length( std::vector<visual_partition> & visual_partitions, int length_px ) ;
	void calc_position_and_height( std::vector<visual_partition> & visual_partitions, int start, int border ) ;
	void calc_used_unused( std::vector<visual_partition> & visual_partitions ) ;
	void calc_text( std::vector<visual_partition> & visual_partitions ) ;
	
	void draw_partitions( const std::vector<visual_partition> & visual_partitions ) ;
	
	bool set_selected( std::vector<visual_partition> & visual_partitions, int x, int y ) ;
	void set_selected( std::vector<visual_partition> & visual_partitions, const Partition & partition ) ;
	
	int spreadout_leftover_px( std::vector<visual_partition> & visual_partitions, int pixels ) ;
	void free_colors( std::vector<visual_partition> & visual_partitions ) ;
	
	//signalhandlers
	void drawingarea_on_realize();
	bool drawingarea_on_expose( GdkEventExpose * event );
	bool on_drawingarea_button_press( GdkEventButton * event );
	void on_resize( Gtk::Allocation & allocation ) ;

	//variables
	struct visual_partition
	{
		double fraction ;
		double fraction_used ;

		int x_start, length ;
		int y_start, height ;
		int x_used_start, used_length ;
		int x_unused_start, unused_length ;
		int y_used_unused_start, used_unused_height ;
		int x_text, y_text ;

		bool selected ;

		Gdk::Color color ;
		Glib::RefPtr<Pango::Layout> pango_layout;

		//real partition
		Partition partition ;
		
		std::vector<visual_partition> logicals ;

		visual_partition()
		{
			fraction = fraction_used =
			x_start = length =
			y_start = height =
			x_used_start = used_length =
			x_unused_start = unused_length =
			y_used_unused_start = used_unused_height =
			x_text = y_text = 0 ;
			
			selected = false ;

			pango_layout .clear() ;
			logicals .clear() ;
		}

		~visual_partition()
		{
			pango_layout .clear() ;
			logicals .clear() ;
		}
	};

	std::vector<visual_partition> visual_partitions ;
	visual_partition selected_vp ;
	int TOT_SEP, MIN_SIZE ;

	Gtk::DrawingArea drawingarea;
	Glib::RefPtr<Gdk::GC> gc;
	Gdk::Color color_used, color_unused, color_text;
};

} //GParted
#endif //FRAME_VISUALDISK
