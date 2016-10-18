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

Frame_Resizer_Extended::Frame_Resizer_Extended()
{
}

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
			if (  ev ->x > (GRIPPER + X_MIN_SPACE_BEFORE) &&
			      ev ->x < (X_END - MIN_SIZE - BORDER * 2) &&
			      ( ev ->x < USED_START || USED == 0 )
			   )
			{
				X_START = static_cast<int>( ev ->x ) ;
				
				signal_resize .emit( X_START - GRIPPER, X_END - GRIPPER - BORDER * 2, ARROW_LEFT ) ; 
			}
			else if ( ev ->x <= (GRIPPER + X_MIN_SPACE_BEFORE) )
			{
				if ( X_START > (GRIPPER + X_MIN_SPACE_BEFORE) )
				{
					X_START = GRIPPER + X_MIN_SPACE_BEFORE ;

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
			if ( ev ->x < 500 + GRIPPER + BORDER * 2 && 
			     ev ->x > (X_START + MIN_SIZE + BORDER * 2) &&
			     ( ev ->x > USED_START + USED + BORDER *2 || USED == 0 ) )
			{
				X_END = static_cast<int>( ev ->x ) ;
				
				signal_resize .emit( X_START - GRIPPER, X_END - GRIPPER - BORDER * 2, ARROW_RIGHT ) ; 
			}
			else if ( ev ->x >= 500 + GRIPPER + BORDER * 2 )
			{
				if ( X_END < 500 + GRIPPER + BORDER * 2 )
				{
					X_END = 500 + GRIPPER + BORDER * 2 ;

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
		
		Draw_Partition() ;
	}
	
	//check if pointer is over a gripper
	else
	{ 
		//left grip
		if ( ! fixed_start &&
		     ev ->x >= X_START - GRIPPER &&
		     ev ->x <= X_START &&
		     ev ->y >= 5 &&
		     ev ->y <= 45 ) 
			drawingarea .get_parent_window() ->set_cursor( *cursor_resize ) ;
		//right grip
		else if (  ev ->x >= X_END &&
			   ev ->x <= X_END + GRIPPER &&
			   ev ->y >= 5 &&
			   ev ->y <= 45 ) 
			drawingarea .get_parent_window() ->set_cursor( *cursor_resize ) ;
		//normal pointer
		else 
			drawingarea .get_parent_window() ->set_cursor() ;		
	}
	
	return true ;
}

void Frame_Resizer_Extended::Draw_Partition() 
{
	//i couldn't find a clear() for a pixmap, that's why ;)
	gc_pixmap ->set_foreground( color_background );
	pixmap ->draw_rectangle( gc_pixmap, true, 0, 0, 536, 50 );
	
	//the two rectangles on each side of the partition
	gc_pixmap ->set_foreground( color_arrow_rectangle );
	pixmap ->draw_rectangle( gc_pixmap, true, 0, 0, 10, 50 );
	pixmap ->draw_rectangle( gc_pixmap, true, 526, 0, 10, 50 );
	
	//used
	gc_pixmap ->set_foreground( color_used );
	pixmap ->draw_rectangle( gc_pixmap, true, USED_START + BORDER, BORDER, USED, 34 );
	
	//partition
	gc_pixmap ->set_foreground( color_partition );
	for( short t = 0; t < 9 ; t++ )
		pixmap ->draw_rectangle( gc_pixmap, false, X_START +t, t, X_END - X_START -t*2, 50 - t*2 );
			
	//resize grips
	Draw_Resize_Grip( ARROW_LEFT ) ;
	Draw_Resize_Grip( ARROW_RIGHT ) ;
	
	//and draw everything to "real" screen..
	drawingarea .get_window() ->draw_drawable( gc_drawingarea, pixmap, 0, 0, 0, 0 ) ;
}
