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
 
#include "../include/Frame_Resizer_Extended.h"

Frame_Resizer_Extended::Frame_Resizer_Extended(  )
{
}

bool Frame_Resizer_Extended::drawingarea_on_mouse_motion( GdkEventMotion *ev ) 
{
	if ( ! GRIP_LEFT && ! GRIP_RIGHT  ) //no need to check this while resizing 
	{ 
		//check if pointer is over a gripper
		if ( ! fixed_start && ev ->x >= X_START -10 && ev ->x <= X_START && ev ->y >= 5 && ev ->y <= 45 ) //left grip
			drawingarea .get_parent_window( ) ->set_cursor( *cursor_resize ) ;
		else if (  ev ->x >= X_END && ev ->x <= X_END + 10 && ev ->y >= 5 && ev ->y <= 45 ) //right grip
			drawingarea .get_parent_window( ) ->set_cursor( *cursor_resize ) ;
		else																																						//normal pointer 
			drawingarea .get_parent_window( ) ->set_cursor( *cursor_normal ) ;		
	}
	
	else if ( GRIP_LEFT || GRIP_RIGHT ) 
	{
		if ( GRIP_LEFT && ev ->x >= 10 && ev ->x <= 510 && ev->x <= X_END - BORDER *2 && ( ev ->x <= USED_START || USED == 0 ) ) 
		{
			X_START = static_cast<int> ( ev ->x ) ;
			signal_resize .emit( X_START -10, X_END -26, ARROW_LEFT ) ; //-10/-26 to get the real value ( this way gripper calculations are invisible outside this class )
		}
		
		else if ( GRIP_RIGHT && ev ->x <= 526 && ev->x >= X_START + BORDER *2 && ev ->x >= USED_START + USED + BORDER *2 )
		{
			X_END = static_cast<int> ( ev ->x ) ;
			signal_resize .emit( X_START -10, X_END -26, ARROW_RIGHT ) ;//-10/-26 to get the real value ( this way gripper calculations are invisible outside this class )
		}
		
		Draw_Partition( ) ;
	}
	
	return true ;
}

void Frame_Resizer_Extended::Draw_Partition( ) 
{
	drawingarea .get_window( ) ->clear( ) ;
	
	//the two rectangles on each side of the partition
	gc ->set_foreground( color_arrow_rectangle );
	drawingarea .get_window( ) ->draw_rectangle( gc, true, 0, 0, 10, 50 );
	drawingarea .get_window( ) ->draw_rectangle( gc, true, 526, 0, 10, 50 );
	
	//used
	gc ->set_foreground( color_used );
	drawingarea .get_window( ) ->draw_rectangle( gc, true, USED_START + BORDER, BORDER, USED, 34 );
	
	//partition
	gc ->set_foreground( color_partition );
	for( short t = 0; t < 9 ; t++ )
		drawingarea .get_window( ) ->draw_rectangle( gc, false, X_START +t, t, X_END - X_START -t*2, 50 - t*2 );
			
	//resize grips
	Draw_Resize_Grip( ARROW_LEFT ) ;
	Draw_Resize_Grip( ARROW_RIGHT ) ;
}
