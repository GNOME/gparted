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
	virtual ~Frame_Resizer_Base() = default;

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
	const int WIDTH   = 500;
	const int HEIGHT  = 50;
	const int BORDER  = 8;
	const int GRIPPER = 10;

	int  m_x_min_space_before = 0;
	int  m_x_start            = 0;
	int  m_x_end              = 0;
	int  m_used               = 0;
	int  m_unused             = 0;
	int  m_x_start_move       = 0;
	int  m_min_size           = 0;
	int  m_max_size           = 0;
	bool m_grip_left          = false;
	bool m_grip_right         = false;
	bool m_grip_move          = false;

	//signal handlers
	void drawingarea_on_realize();
	bool drawingarea_on_draw(const Cairo::RefPtr<Cairo::Context>& cr);
	virtual bool drawingarea_on_mouse_motion( GdkEventMotion * ev ) ;
	bool drawingarea_on_button_press_event( GdkEventButton * ev ) ;
	bool drawingarea_on_button_release_event( GdkEventButton * ev ) ;
	bool drawingarea_on_leave_notify( GdkEventCrossing * ev ) ;

	void draw_resize_grip(const Cairo::RefPtr<Cairo::Context>& cr, ArrowType);

	Gtk::DrawingArea m_drawingarea;

	Gdk::RGBA m_color_used;
	Gdk::RGBA m_color_unused;
	Gdk::RGBA m_color_arrow;
	Gdk::RGBA m_color_background;
	Gdk::RGBA m_color_partition;
	Gdk::RGBA m_color_arrow_rectangle;

	std::vector<Gdk::Point> m_arrow_points;

	Glib::RefPtr<Gdk::Cursor> m_cursor_resize;
	Glib::RefPtr<Gdk::Cursor> m_cursor_move;

	bool m_fixed_start = false;  // A fixed start disables moving the start and
	                             // therefore the whole move functionality.
};


}  // namespace GParted


#endif /* GPARTED_FRAME_RESIZER_BASE_H */
