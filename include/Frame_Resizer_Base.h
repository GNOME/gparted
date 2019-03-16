/* Copyright (C) 2004 Bart
 * Copyright (C) 2010 Curtis Gedak
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

#ifndef GPARTED_FRAME_RESIZER_BASE_H
#define GPARTED_FRAME_RESIZER_BASE_H

#include <gtkmm/frame.h>
#include <gtkmm/drawingarea.h>
#include <gdkmm/cursor.h>
 

namespace GParted
{


class Frame_Resizer_Base : public Gtk::Frame
{
public:
	enum ArrowType {
		ARROW_LEFT	= 0,
		ARROW_RIGHT	= 1
	};

	Frame_Resizer_Base() ;
	~Frame_Resizer_Base() ;

	void set_rgb_partition_color(const Gdk::RGBA& color);
	void override_default_rgb_unused_color(const Gdk::RGBA& color);

	void set_x_min_space_before( int x_min_space_before ) ;
	void set_x_start( int x_start ) ;
	void set_x_end( int x_end ) ;
	void set_used( int used );
	void set_fixed_start( bool fixed_start ) ;
	void set_size_limits( int min_size, int max_size ) ;

	int get_used();
	int get_x_start() ;
	int get_x_end() ;

	virtual void draw_partition(const Cairo::RefPtr<Cairo::Context>& cr);

	void redraw();

	//public signals  (emitted upon resize/move)
	sigc::signal<void,int,int, ArrowType> signal_resize;
	sigc::signal<void,int,int> signal_move;

protected:
	int BORDER, GRIPPER ;
	int X_MIN_SPACE_BEFORE ;
	int X_START, USED, UNUSED, X_END, X_START_MOVE, MIN_SIZE, MAX_SIZE;
	bool GRIP_LEFT, GRIP_RIGHT, GRIP_MOVE ;

	//signal handlers
	void drawingarea_on_realize();
	bool drawingarea_on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
	virtual bool drawingarea_on_mouse_motion( GdkEventMotion * ev ) ;
	bool drawingarea_on_button_press_event( GdkEventButton * ev ) ;
	bool drawingarea_on_button_release_event( GdkEventButton * ev ) ;
	bool drawingarea_on_leave_notify( GdkEventCrossing * ev ) ;

	void draw_resize_grip(const Cairo::RefPtr<Cairo::Context>& cr, ArrowType);

	Gtk::DrawingArea drawingarea ;

	Gdk::RGBA color_used;
	Gdk::RGBA color_unused;
	Gdk::RGBA color_arrow;
	Gdk::RGBA color_background;
	Gdk::RGBA color_partition;
	Gdk::RGBA color_arrow_rectangle;

	std::vector<Gdk::Point> arrow_points;
	
	Glib::RefPtr<Gdk::Cursor> cursor_resize;
	Glib::RefPtr<Gdk::Cursor> cursor_move;
	
	int temp_x, temp_y ;
	bool fixed_start; //a fixed start disables moving the start and thereby the whole move functionality..

private:
	void init() ;

};


} //GParted

#endif /* GPARTED_FRAME_RESIZER_BASE_H */
