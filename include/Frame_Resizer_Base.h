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
 
 
#ifndef  FRAME_RESIZER_BASE
#define FRAME_RESIZER_BASE

#include <gtkmm/frame.h>
#include <gtkmm/drawingarea.h>
#include <gdkmm/cursor.h>
 
#define BORDER 8

class Frame_Resizer_Base : public Gtk::Frame, virtual public sigc::trackable
{
public:
	
	enum ArrowType {
		ARROW_LEFT		=	0,
		ARROW_RIGHT	=	1
	};
	
	Frame_Resizer_Base(  ) ;
	~Frame_Resizer_Base() ;

	void set_rgb_partition_color( const Gdk::Color & ) ;
	void override_default_rgb_unused_color( const Gdk::Color & ) ;
	
	void set_x_start( int ) ;
	void set_x_end( int ) ;
	void set_used( int );
	void set_fixed_start( bool ) ;
	void set_used_start( int used_start ) ;

	

	int get_used();
	int get_x_start() ;
	int get_x_end() ;

	virtual void Draw_Partition() ;


	//public signal  (emitted upon resize/move)
	sigc::signal<void,int,int, ArrowType> signal_resize;
	sigc::signal<void,int,int> signal_move;

protected:
	int X_START, USED, UNUSED, X_END, X_START_MOVE, USED_START;
	bool GRIP_LEFT, GRIP_RIGHT, GRIP_MOVE ;

	//signal handlers
	void drawingarea_on_realize(  );
	bool drawingarea_on_expose( GdkEventExpose *  );
	virtual bool drawingarea_on_mouse_motion( GdkEventMotion* ) ;
	bool drawingarea_on_button_press_event( GdkEventButton* ) ;
	bool drawingarea_on_button_release_event( GdkEventButton* ) ;
	bool drawingarea_on_leave_notify( GdkEventCrossing* ) ;
		
	void Draw_Resize_Grip( ArrowType ) ;

	Gtk::DrawingArea drawingarea;
	Glib::RefPtr<Gdk::GC> gc;

	Gdk::Color color_used, color_unused, color_arrow, color_background,color_partition,color_arrow_rectangle;

	std::vector <Gdk::Point> arrow_points;
	
	Gdk::Cursor *cursor_resize, *cursor_normal, *cursor_move;
	
	int temp_x, temp_y ;
	bool fixed_start; //a fixed start disables moving the start and thereby the whole move functionality..

private:
	void init() ;

};

#endif // FRAME_RESIZER_BASE
