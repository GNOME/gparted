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

#include "Frame_Resizer_Extended.h"

#include <gdkmm/general.h>


namespace GParted
{


void Frame_Resizer_Extended::set_used_start( int used_start ) 
{
	if ( used_start <= 0 )
		USED_START = GRIPPER ;
	else
		USED_START = used_start + GRIPPER ;
}

bool Frame_Resizer_Extended::drawingarea_on_mouse_motion( GdkEventMotion * ev ) 
{
	if ( GRIP_LEFT || GRIP_RIGHT ) 
	{
		if ( GRIP_LEFT )
		{
			if ((GRIPPER + m_x_min_space_before) < ev->x && ev->x < (m_x_end - MIN_SIZE - BORDER * 2) &&
			    (ev->x < USED_START || m_used == 0)                                                     )
			{
				m_x_start = static_cast<int>(ev->x);

				signal_resize.emit(m_x_start - GRIPPER,
				                   m_x_end - GRIPPER - BORDER * 2,
				                   ARROW_LEFT);
			}
			else if (ev->x <= (GRIPPER + m_x_min_space_before))
			{
				if (m_x_start > (GRIPPER + m_x_min_space_before))
				{
					m_x_start = GRIPPER + m_x_min_space_before;

					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_LEFT);
				}
			}
			else if (m_used != 0 && ev->x >= USED_START)
			{
				if (m_x_start < USED_START)
				{
					m_x_start = USED_START;

					//+1 to force the spinbutton to its min.
					signal_resize.emit(m_x_start - GRIPPER +1,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_LEFT);
				}
			}
			else if (m_used == 0 && ev->x >= (m_x_end - MIN_SIZE - BORDER * 2))
			{
				if (m_x_start < (m_x_end - BORDER * 2))
				{
					m_x_start = m_x_end - MIN_SIZE - BORDER * 2;

					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_LEFT);
				}
			}
		}
		else if ( GRIP_RIGHT )
		{
			if ((m_x_start + MIN_SIZE + BORDER * 2) < ev->x && ev->x < (WIDTH + GRIPPER + BORDER * 2) &&
			    (ev->x > (USED_START + m_used + BORDER * 2) || m_used == 0)                             )
			{
				m_x_end = static_cast<int>(ev->x);

				signal_resize.emit(m_x_start - GRIPPER,
				                   m_x_end - GRIPPER - BORDER * 2,
				                   ARROW_RIGHT);
			}
			else if (ev->x >= WIDTH + GRIPPER + BORDER * 2)
			{
				if (m_x_end < (WIDTH + GRIPPER + BORDER * 2))
				{
					m_x_end = WIDTH + GRIPPER + BORDER * 2;

					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_RIGHT);
				}
			}
			else if (m_used != 0 && ev->x <= (USED_START + m_used + BORDER * 2))
			{
				if (m_x_end > (USED_START + m_used + BORDER * 2))
				{
					m_x_end = USED_START + m_used + BORDER * 2;

					//-1 to force the spinbutton to its min.
					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2 - 1,
					                   ARROW_RIGHT);
				}
			}
			else if (m_used == 0 && ev->x <= (m_x_start + MIN_SIZE + BORDER * 2))
			{
				if (m_x_end > (m_x_start + MIN_SIZE + BORDER * 2))
				{
					m_x_end = m_x_start + MIN_SIZE + BORDER * 2;

					signal_resize.emit(m_x_start - GRIPPER,
					                   m_x_end - GRIPPER - BORDER * 2,
					                   ARROW_RIGHT);
				}
			}
		}

		redraw();
	}
	
	//check if pointer is over a gripper
	else
	{ 
		//left grip
		if (! m_fixed_start              &&
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
		//normal pointer
		else
		{
			m_drawingarea.get_parent_window()->set_cursor();
		}
	}

	return true ;
}


void Frame_Resizer_Extended::draw_partition(const Cairo::RefPtr<Cairo::Context>& cr)
{
	// Extended partition resize/move widget is drawn like this:
	//     ...     ##################################               ...
	//     ...     ##         11111111111111       ##               ...
	//     ...    <##         11111111111111       ##>              ...
	//     ...   <<##         11111111111111       ##>>             ...
	//     ...    <##         11111111111111       ##>              ...
	//     ...     ##         11111111111111       ##               ...
	//     ...     ##################################               ...
	//
	//                        |<- m_used ->|
	//            /\         /\                     /\.
	//             m_x_start  USED_START             m_x_end
	// Legend:
	//                - Unallocated space.  Shares portion of WIDTH (500 pixels).
	//     .          - Light grey gripper landing area at each end.  GRIPPER (10)
	//                  pixels wide.
	//     <          - Left arrow.  GRIPPER (10 pixels) wide at start of partition.
	//     >          - Right arrow.  GRIPPER (10 pixels) wide at end of partition.
	//     #          - Partition border.  BORDER (8 pixels) wide around partition.
	//     1 / m_used - Used space within partition.  m_used pixels wide.  Shares
	//                  portion of WIDTH (500 pixels).
	//     m_x_start  - Pixel X-position to start of partition with widget.
	//     USED_START - Pixel X-position to start of used space within widget.
	//     m_x_end    - Pixel X-position to end of partition with widget.

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

	// Unallocated space filling extended partition
	Gdk::Cairo::set_source_rgba(cr, m_color_background);
	cr->rectangle(m_x_start + BORDER, BORDER, m_x_end - m_x_start - BORDER * 2, HEIGHT - BORDER * 2);
	cr->fill();

	// Used
	Gdk::Cairo::set_source_rgba(cr, m_color_used);
	cr->rectangle(USED_START + BORDER, BORDER, m_used, HEIGHT - BORDER * 2);
	cr->fill();

	// Resize grips
	draw_resize_grip(cr, ARROW_LEFT);
	draw_resize_grip(cr, ARROW_RIGHT);
}


}  // namespace GParted
