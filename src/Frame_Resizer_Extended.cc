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
			if ((GRIPPER + m_x_min_space_before) < ev->x && ev->x < (X_END - MIN_SIZE - BORDER * 2) &&
			    (ev->x < USED_START || USED == 0)                                                     )
			{
				X_START = static_cast<int>( ev ->x ) ;
				
				signal_resize .emit( X_START - GRIPPER, X_END - GRIPPER - BORDER * 2, ARROW_LEFT ) ; 
			}
			else if (ev->x <= (GRIPPER + m_x_min_space_before))
			{
				if (X_START > (GRIPPER + m_x_min_space_before))
				{
					X_START = GRIPPER + m_x_min_space_before;

					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_LEFT ) ; 
				}
			}
			else if ( USED != 0 && ev ->x >= USED_START )
			{
				if ( X_START < USED_START )
				{
					X_START = USED_START ;

					//+1 to force the spinbutton to its min.
					signal_resize .emit( X_START - GRIPPER +1,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_LEFT ) ;
				}
			}
			else if ( USED == 0 && ev ->x >= (X_END - MIN_SIZE - BORDER * 2) )
			{
				if ( X_START < X_END - BORDER * 2 )
				{
					X_START = X_END - MIN_SIZE - BORDER * 2 ;
					
					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_LEFT ) ;
				}
			}
		}
		else if ( GRIP_RIGHT )
		{
			if (ev->x < WIDTH + GRIPPER + BORDER * 2                  &&
			    ev->x > X_START + MIN_SIZE + BORDER * 2               &&
			    (ev->x > USED_START + USED + BORDER * 2 || USED == 0)   )
			{
				X_END = static_cast<int>( ev ->x ) ;

				signal_resize .emit( X_START - GRIPPER, X_END - GRIPPER - BORDER * 2, ARROW_RIGHT ) ; 
			}
			else if (ev->x >= WIDTH + GRIPPER + BORDER * 2)
			{
				if (X_END < WIDTH + GRIPPER + BORDER * 2)
				{
					X_END = WIDTH + GRIPPER + BORDER * 2;

					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_RIGHT ) ; 
				}
			}
			else if ( USED != 0 && ev ->x <= USED_START + USED + BORDER *2 )
			{
				if ( X_END > USED_START + USED + BORDER *2 )
				{
					X_END = USED_START + USED + BORDER *2 ;

					//-1 to force the spinbutton to its min.
					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2 -1, ARROW_RIGHT ) ;
				}
			}
			else if ( USED == 0 && ev ->x <= (X_START + MIN_SIZE + BORDER * 2) )
			{
				if ( X_END > (X_START + MIN_SIZE + BORDER * 2) )
				{
					X_END = X_START + MIN_SIZE + BORDER *2 ;
					
					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_RIGHT ) ;
				}
			}
		}

		redraw();
	}
	
	//check if pointer is over a gripper
	else
	{ 
		//left grip
		if (! m_fixed_start            &&
		    ev->x >= X_START - GRIPPER &&
		    ev->x <= X_START           &&
		    ev->y >= 5                 &&
		    ev->y <= 45                  )
		{
			m_drawingarea.get_parent_window()->set_cursor(m_cursor_resize);
		}
		//right grip
		else if (  ev ->x >= X_END &&
			   ev ->x <= X_END + GRIPPER &&
			   ev ->y >= 5 &&
			   ev ->y <= 45 ) 
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
	//     ...     ##       11111111111111         ##               ...
	//     ...    <##       11111111111111         ##>              ...
	//     ...   <<##       11111111111111         ##>>             ...
	//     ...    <##       11111111111111         ##>              ...
	//     ...     ##       11111111111111         ##               ...
	//     ...     ##################################               ...
	//
	//                      |<-- USED -->|
	//            /\       /\                       /\.
	//             X_START  USED_START               X_END
	// Legend:
	//                - Unallocated space.  Shares portion of WIDTH (500 pixels).
	//     .          - Light grey gripper landing area at each end.  GRIPPER (10)
	//                  pixels wide.
	//     <          - Left arrow.  GRIPPER (10 pixels) wide at start of partition.
	//     >          - Right arrow.  GRIPPER (10 pixels) wide at end of partition.
	//     #          - Partition border.  BORDER (8 pixels) wide around partition.
	//     1 / USED   - Used space within partition.  USED pixels wide.  Shares
	//                  portion of WIDTH (500 pixels).
	//     X_START    - Pixel X-position to start of partition with widget.
	//     USED_START - Pixel X-position to start of used space within widget.
	//     X_END      - Pixel X-position to end of partition with widget.

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
	cr->rectangle(X_START, 0, X_END - X_START, HEIGHT);
	cr->fill();

	// Unallocated space filling extended partition
	Gdk::Cairo::set_source_rgba(cr, m_color_background);
	cr->rectangle(X_START + BORDER, BORDER, X_END - X_START - BORDER * 2, HEIGHT - BORDER * 2);
	cr->fill();

	// Used
	Gdk::Cairo::set_source_rgba(cr, m_color_used);
	cr->rectangle(USED_START + BORDER, BORDER, USED, HEIGHT - BORDER * 2);
	cr->fill();

	// Resize grips
	draw_resize_grip(cr, ARROW_LEFT);
	draw_resize_grip(cr, ARROW_RIGHT);
}


}  // namespace GParted
