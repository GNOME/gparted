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


#include "Frame_Resizer_Base.h"

#include "Utils.h"

#include <gdkmm/general.h>
#include <sigc++/signal.h>


namespace GParted
{


Frame_Resizer_Base::Frame_Resizer_Base()
 : m_color_used(Utils::get_color(FS_USED)),
   m_color_unused(Utils::get_color(FS_UNUSED)),
   m_color_arrow("black"),
   m_color_background(Utils::get_color(FS_UNALLOCATED)),
   m_color_arrow_rectangle("lightgrey")
{
	m_drawingarea.set_size_request(WIDTH + GRIPPER * 2 + BORDER * 2, HEIGHT);

	m_drawingarea.signal_realize().connect(
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_realize) ) ;
	m_drawingarea.signal_draw().connect(
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_draw));
	m_drawingarea.signal_motion_notify_event().connect(
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_mouse_motion) ) ;
	m_drawingarea.signal_button_press_event().connect(
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_button_press_event) ) ;
	m_drawingarea.signal_button_release_event().connect(
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_button_release_event) ) ;
	m_drawingarea.signal_leave_notify_event().connect(
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_leave_notify) ) ;

	this->add(m_drawingarea);

	m_cursor_resize = Gdk::Cursor::create(get_display(), "ew-resize");
	m_cursor_move   = Gdk::Cursor::create(get_display(), "fleur");  // FIXME: Replace with "all-resize"
	                                                                // when available on all distributions.

	set_size_limits(0, WIDTH);

	Gdk::Point p;
	p.set_y(15);  m_arrow_points.push_back(p);
	p.set_y(25);  m_arrow_points.push_back(p);
	p.set_y(35);  m_arrow_points.push_back(p);

	this ->show_all_children();
}


void Frame_Resizer_Base::set_rgb_partition_color(const Gdk::RGBA& color)
{
	m_color_partition = color;
}


void Frame_Resizer_Base::override_default_rgb_unused_color(const Gdk::RGBA& color)
{
	m_color_unused = color;
}


void Frame_Resizer_Base::set_x_start( int x_start ) 
{  
	m_x_start = x_start + GRIPPER;
} 


void Frame_Resizer_Base::set_x_min_space_before( int x_min_space_before ) 
{
	m_x_min_space_before = x_min_space_before;
}


void Frame_Resizer_Base::set_x_end( int x_end ) 
{ 
	m_x_end = x_end + GRIPPER + BORDER * 2;
}


void Frame_Resizer_Base::set_used( int used )
{   
	m_used = used;
}


void Frame_Resizer_Base::set_fixed_start( bool fixed_start ) 
{
	m_fixed_start = fixed_start;
}


void Frame_Resizer_Base::set_size_limits( int min_size, int max_size )
{
	this ->MIN_SIZE = min_size + BORDER * 2 ;
	this ->MAX_SIZE = max_size + BORDER * 2 ;
}


int Frame_Resizer_Base::get_used()
{
	return m_used;
}


int Frame_Resizer_Base::get_x_start() 
{
	return m_x_start - GRIPPER;
}


int Frame_Resizer_Base::get_x_end() 
{
	return m_x_end - GRIPPER - BORDER * 2;
}


void Frame_Resizer_Base::drawingarea_on_realize()
{
	m_drawingarea.add_events(Gdk::POINTER_MOTION_MASK);
	m_drawingarea.add_events(Gdk::BUTTON_PRESS_MASK);
	m_drawingarea.add_events(Gdk::BUTTON_RELEASE_MASK);
	m_drawingarea.add_events(Gdk::LEAVE_NOTIFY_MASK);
}


bool Frame_Resizer_Base::drawingarea_on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	cr->set_line_cap(Cairo::LINE_CAP_SQUARE);
	cr->set_line_width(1.0);

	draw_partition(cr);

	return true;
}

bool Frame_Resizer_Base::drawingarea_on_mouse_motion( GdkEventMotion * ev ) 
{
	if ( GRIP_LEFT || GRIP_RIGHT || GRIP_MOVE ) 
	{
		if ( GRIP_LEFT )
		{
			if (ev->x > (GRIPPER + m_x_min_space_before)                     &&
			    MIN_SIZE < (m_x_end - ev->x) && (m_x_end - ev->x) < MAX_SIZE   )
			{
				m_x_start = static_cast<int>(ev->x);

				signal_resize.emit(m_x_start - GRIPPER,
				                   m_x_end - GRIPPER - 2 * BORDER,
				                   ARROW_LEFT);
			}
			else if ((m_x_end - ev ->x) >= MAX_SIZE)
			{
				if ((m_x_end - m_x_start) < MAX_SIZE)
				{
					m_x_start = m_x_end - MAX_SIZE;

					if (m_x_start < (GRIPPER + m_x_min_space_before))
						m_x_start = GRIPPER + m_x_min_space_before;

					//-1 to force the spinbutton to its max.
					signal_resize.emit(m_x_start - GRIPPER - 1,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_LEFT);
				}
			}
			else if (ev->x <= (GRIPPER + m_x_min_space_before))
			{
				if (m_x_start             > (GRIPPER + m_x_min_space_before) &&
				    (m_x_end - m_x_start) < MAX_SIZE                           )
				{
					m_x_start = GRIPPER + m_x_min_space_before;

					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_LEFT);
				}
			}
			else if ((m_x_end - ev->x) <= MIN_SIZE)
			{
				if ((m_x_end - m_x_start) > MIN_SIZE)
				{
					m_x_start = m_x_end - MIN_SIZE;

					//+1 to force the spinbutton to its min.
					signal_resize.emit(m_x_start - GRIPPER +1,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_LEFT);
				}
			}
		}

		else if ( GRIP_RIGHT )
		{
			if (ev->x < (WIDTH + GRIPPER + BORDER * 2)                           &&
			    MIN_SIZE < (ev->x - m_x_start) && (ev->x - m_x_start < MAX_SIZE)   )
			{
				m_x_end = static_cast<int>(ev->x);

				signal_resize.emit(m_x_start - GRIPPER,
				                   m_x_end - GRIPPER - BORDER * 2,
				                   ARROW_RIGHT);
			}
			else if ((ev->x - m_x_start) >= MAX_SIZE)
			{
				if ((m_x_end - m_x_start) < MAX_SIZE)
				{
					m_x_end = m_x_start + MAX_SIZE;

					if (m_x_end > (WIDTH + GRIPPER + BORDER * 2))
						m_x_end = WIDTH + GRIPPER + BORDER * 2;

					//+1 to force the spinbutton to its min.
					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2 + 1,
					                   ARROW_RIGHT);
				}
			}
			else if (ev->x >= WIDTH + GRIPPER + BORDER * 2)
			{
				if (m_x_end               < (WIDTH + GRIPPER + BORDER * 2) &&
				    (m_x_end - m_x_start) < MAX_SIZE                         )
				{
					m_x_end = WIDTH + GRIPPER + BORDER * 2;

					signal_resize.emit(m_x_start -GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_RIGHT);
				}
			}
			else if ((ev->x - m_x_start) <= MIN_SIZE)
			{
				if ((m_x_end - m_x_start) > MIN_SIZE)
				{
					m_x_end = m_x_start + MIN_SIZE;

					//-1 to force the spinbutton to its min.
					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2 - 1,
					                   ARROW_RIGHT);
				}
			}
		}
		
		else if ( GRIP_MOVE )
		{
			int temp_x = m_x_start + static_cast<int>(ev->x - m_x_start_move);
			int temp_y = m_x_end - m_x_start;

			if (temp_x            > (GRIPPER + m_x_min_space_before) &&
			    (temp_x + temp_y) < (WIDTH + GRIPPER + BORDER * 2)     )
			{
				m_x_start = temp_x;
				m_x_end = m_x_start + temp_y;
			}
			else if (temp_x <= (GRIPPER + m_x_min_space_before))
			{
				if (m_x_start > (GRIPPER + m_x_min_space_before))
				{
					m_x_start = GRIPPER + m_x_min_space_before;
					m_x_end = m_x_start + temp_y;
				}
			}
			else if (temp_x + temp_y >= WIDTH + GRIPPER + BORDER * 2)
			{
				if (m_x_end < (WIDTH + GRIPPER + BORDER * 2))
				{
					m_x_end = WIDTH + GRIPPER + BORDER * 2;
					m_x_start = m_x_end - temp_y;
				}
			}

			m_x_start_move = static_cast<int>(ev->x);
			signal_move.emit(m_x_start - GRIPPER,
			                 m_x_end - GRIPPER - BORDER * 2);
		}

		redraw();
	}
	else
	{ 
		//check if pointer is over a gripper
		//left grip
		if (! m_fixed_start                                      &&
		    (m_x_start - GRIPPER) <= ev->x && ev->x <= m_x_start &&
		    5                     <= ev->y && ev->y <= 45          )
		{
			m_drawingarea.get_parent_window()->set_cursor(m_cursor_resize);
		}
		//right grip
		else if (m_x_end <= ev->x && ev->x <= (m_x_end + GRIPPER) &&
			 5       <= ev->y && ev->y <= 45                    )
		{
			m_drawingarea.get_parent_window()->set_cursor(m_cursor_resize);
		}
		//move grip
		else if (! m_fixed_start                        &&
			 m_x_start <= ev->x && ev->x <= m_x_end   )
		{
			m_drawingarea.get_parent_window()->set_cursor(m_cursor_move);
		}
		//normal pointer 
		else
		{
			m_drawingarea.get_parent_window()->set_cursor();
		}
	}

	return true;
}


bool Frame_Resizer_Base::drawingarea_on_button_press_event( GdkEventButton *ev ) 
{
	GRIP_LEFT = GRIP_RIGHT = GRIP_MOVE = false;
	
	//left grip
	if (! m_fixed_start                                      &&
	    (m_x_start - GRIPPER) <= ev->x && ev->x <= m_x_start &&
	    5                     <= ev->y && ev->y <= 45          )
	{
		GRIP_LEFT = true ;
	}
	//right grip
	else if (m_x_end <= ev->x && ev->x <= (m_x_end + GRIPPER) &&
		 5       <= ev->y && ev->y <= 45                    )
	{
		GRIP_RIGHT = true ;
	}
	//move grip
	else if (! m_fixed_start                        &&
		 m_x_start <= ev->x && ev->x <= m_x_end   )
	{
		 GRIP_MOVE = true ;
		 m_x_start_move = static_cast<int>(ev->x);
	}

	return true;
}


bool Frame_Resizer_Base::drawingarea_on_button_release_event( GdkEventButton *ev ) 
{
	GRIP_LEFT = GRIP_RIGHT = GRIP_MOVE = false;
	
	return true;
}


bool Frame_Resizer_Base::drawingarea_on_leave_notify( GdkEventCrossing *ev )
{
	if ( ev ->mode != GDK_CROSSING_GRAB && ! GRIP_LEFT && ! GRIP_RIGHT && ! GRIP_MOVE ) 
		m_drawingarea.get_parent_window()->set_cursor();

	return true;
}


void Frame_Resizer_Base::draw_partition(const Cairo::RefPtr<Cairo::Context>& cr)
{
	m_unused = m_x_end - m_x_start - BORDER * 2 - m_used;
	if (m_unused < 0)
		m_unused = 0;

	// Normal partition resize/move widget is drawn like this:
	//     ...     ##################################               ...
	//     ...     ##111111111111110000000000000000##               ...
	//     ...    <##111111111111110000000000000000##>              ...
	//     ...   <<##111111111111110000000000000000##>>             ...
	//     ...    <##111111111111110000000000000000##>              ...
	//     ...     ##111111111111110000000000000000##               ...
	//     ...     ##################################               ...
	//
	//               |<- m_used ->||<- m_unused ->|
	//            /\                                /\.
	//             m_x_start                         m_x_end
	// Legend:
	//                  - Unallocated space.  Shares portion of WIDTH (500 pixels).
	//     .            - Light grey gripper landing area at each end.  GRIPPER (10)
	//                    pixels wide.
	//     <            - Left arrow.  GRIPPER (10 pixels) wide at start of partition.
	//     >            - Right arrow.  GRIPPER (10 pixels) wide at end of partition.
	//     #            - Partition border.  BORDER (8 pixels) wide around partition.
	//     1 / m_used   - Used space within partition.  m_used pixels wide.  Shares
	//                    portion of WIDTH (500 pixels).
	//     0 / m_unused - Unused space within partition.  m_unused pixels wide.
	//                    Shares portion of WIDTH (500 pixels).
	//     m_x_start    - Pixel X-position to start of partition with widget.
	//     m_x_end      - Pixel X-position to end of partition with widget.

	// Background color
	Gdk::Cairo::set_source_rgba(cr, m_color_background);
	cr->rectangle(0, 0, WIDTH + GRIPPER * 2 + BORDER * 2, HEIGHT);
	cr->fill();

	// The two rectangles on each side of the partition
	Gdk::Cairo::set_source_rgba(cr, m_color_arrow_rectangle);
	cr->rectangle(0, 0, GRIPPER, HEIGHT);
	cr->fill();
	cr->rectangle(WIDTH + GRIPPER + BORDER * 2, 0, GRIPPER, HEIGHT);
	cr->fill();

	// Partition
	Gdk::Cairo::set_source_rgba(cr, m_color_partition);
	cr->rectangle(m_x_start, 0, m_x_end - m_x_start, HEIGHT);
	cr->fill();

	// Used
	Gdk::Cairo::set_source_rgba(cr, m_color_used);
	cr->rectangle(m_x_start + BORDER, BORDER, m_used, HEIGHT - BORDER * 2);
	cr->fill();

	// Unused
	Gdk::Cairo::set_source_rgba(cr, m_color_unused);
	cr->rectangle(m_x_start + BORDER + m_used, BORDER, m_unused, HEIGHT - BORDER * 2);
	cr->fill();

	// Resize grips
	if (! m_fixed_start)
		draw_resize_grip(cr, ARROW_LEFT);

	draw_resize_grip(cr, ARROW_RIGHT);
}


void Frame_Resizer_Base::draw_resize_grip(const Cairo::RefPtr<Cairo::Context>& cr, ArrowType arrow_type)
{
	if ( arrow_type == ARROW_LEFT )
	{
		m_arrow_points[0].set_x(m_x_start);
		m_arrow_points[1].set_x(m_x_start - GRIPPER);
		m_arrow_points[2].set_x(m_x_start);
	}
	else
	{
		m_arrow_points[0].set_x(m_x_end);
		m_arrow_points[1].set_x(m_x_end + GRIPPER);
		m_arrow_points[2].set_x(m_x_end);
	}

	// Attach resize arrows to the partition
	Gdk::Cairo::set_source_rgba(cr, m_color_arrow_rectangle);
	cr->rectangle((arrow_type == ARROW_LEFT ? m_x_start - GRIPPER : m_x_end) + 0.5,
	              5 + 0.5,
	              9,
	              40);
	cr->stroke();

	Gdk::Cairo::set_source_rgba(cr, m_color_arrow);
	cr->move_to(m_arrow_points[0].get_x(), m_arrow_points[0].get_y());
	cr->line_to(m_arrow_points[1].get_x(), m_arrow_points[1].get_y());
	cr->line_to(m_arrow_points[2].get_x(), m_arrow_points[2].get_y());
	cr->close_path();
	cr->fill();
}


void Frame_Resizer_Base::redraw()
{
	m_drawingarea.queue_draw();
}


}  // namespace GParted
