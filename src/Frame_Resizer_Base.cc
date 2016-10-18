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

Frame_Resizer_Base::Frame_Resizer_Base()
{
	BORDER = 8 ;
	GRIPPER = 10 ;
	X_MIN_SPACE_BEFORE = 0 ;

	fixed_start = false ;
	init() ;
}

void Frame_Resizer_Base::init() 
{
	drawingarea .set_size_request( 500 + GRIPPER * 2 + BORDER *2, 50 );

	drawingarea .signal_realize() .connect( 
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_realize) ) ;
	drawingarea .signal_expose_event() .connect( 
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_expose) ) ;
	drawingarea .signal_motion_notify_event() .connect( 
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_mouse_motion) ) ;
	drawingarea .signal_button_press_event() .connect( 
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_button_press_event) ) ;
	drawingarea .signal_button_release_event() .connect( 
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_button_release_event) ) ;
	drawingarea .signal_leave_notify_event() .connect( 
			sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_leave_notify) ) ;
		
	this ->add( drawingarea ) ;
	
	color_used .set( "#F8F8BA" );			this ->get_colormap() ->alloc_color( color_used ) ;
	color_unused .set( "white" );			this ->get_colormap() ->alloc_color( color_unused ) ;
	color_arrow .set( "black" );			this ->get_colormap() ->alloc_color( color_arrow ) ;
	color_background .set( "darkgrey" );		this ->get_colormap() ->alloc_color( color_background ) ;
	color_arrow_rectangle .set( "lightgrey" );	this ->get_colormap() ->alloc_color( color_arrow_rectangle ) ;
	
	cursor_resize = new Gdk::Cursor( Gdk::SB_H_DOUBLE_ARROW ) ; 
	cursor_move   = new Gdk::Cursor( Gdk::FLEUR ) ; 
	  
	GRIP_MOVE = GRIP_LEFT = GRIP_RIGHT = false;
	X_END = 0;
	set_size_limits( 0, 500 ) ;
	
	Gdk::Point p;
	p .set_y( 15 ); 	arrow_points .push_back( p ) ;
	p .set_y( 25 ); 	arrow_points .push_back( p ) ;
	p .set_y( 35 ); 	arrow_points .push_back( p ) ;
	
	this ->show_all_children();
}

void Frame_Resizer_Base::set_rgb_partition_color( const Gdk::Color & color )
{
	this ->get_colormap() ->free_color( color_partition ) ;
	this ->color_partition = color ;
	this ->get_colormap() ->alloc_color( color_partition ) ;
}

void Frame_Resizer_Base::override_default_rgb_unused_color( const Gdk::Color & color ) 
{
	this ->get_colormap() ->free_color( color_unused ) ;
	this ->color_unused = color ;
	this ->get_colormap() ->alloc_color( color_unused ) ;
}

void Frame_Resizer_Base::set_x_start( int x_start ) 
{  
	this ->X_START = x_start + GRIPPER ;
} 

void Frame_Resizer_Base::set_x_min_space_before( int x_min_space_before ) 
{
	this ->X_MIN_SPACE_BEFORE = x_min_space_before ;
}

void Frame_Resizer_Base::set_x_end( int x_end ) 
{ 
	this ->X_END = x_end + GRIPPER + BORDER * 2 ; 
}

void Frame_Resizer_Base::set_used( int used )
{   
	this ->USED = used ;
}

void Frame_Resizer_Base::set_fixed_start( bool fixed_start ) 
{
	this ->fixed_start = fixed_start ;
}

void Frame_Resizer_Base::set_size_limits( int min_size, int max_size )
{
	this ->MIN_SIZE = min_size + BORDER * 2 ;
	this ->MAX_SIZE = max_size + BORDER * 2 ;
}

int Frame_Resizer_Base::get_used()
{
	return USED ;
}

int Frame_Resizer_Base::get_x_start() 
{
	return X_START - GRIPPER ;
}

int Frame_Resizer_Base::get_x_end() 
{
	return X_END - GRIPPER - BORDER * 2 ;
}

void Frame_Resizer_Base::drawingarea_on_realize()
{
	gc_drawingarea = Gdk::GC::create( drawingarea .get_window() );
	pixmap = Gdk::Pixmap::create( drawingarea .get_window(),
				      drawingarea .get_allocation() .get_width(),
				      drawingarea .get_allocation() .get_height() );
	gc_pixmap = Gdk::GC::create( pixmap );
	
	drawingarea .add_events( Gdk::POINTER_MOTION_MASK );
	drawingarea .add_events( Gdk::BUTTON_PRESS_MASK );
	drawingarea .add_events( Gdk::BUTTON_RELEASE_MASK );
	drawingarea .add_events( Gdk::LEAVE_NOTIFY_MASK );
}

bool Frame_Resizer_Base::drawingarea_on_expose( GdkEventExpose * ev )
{ 
	Draw_Partition() ;
	return true;
}

bool Frame_Resizer_Base::drawingarea_on_mouse_motion( GdkEventMotion * ev ) 
{
	if ( GRIP_LEFT || GRIP_RIGHT || GRIP_MOVE ) 
	{
		if ( GRIP_LEFT )
		{
			if ( ev ->x > (GRIPPER + X_MIN_SPACE_BEFORE) &&
			     (X_END - ev ->x) < MAX_SIZE &&
			     (X_END - ev ->x) > MIN_SIZE )
			{
				X_START = static_cast<int>( ev ->x ) ;
			
				signal_resize .emit( X_START - GRIPPER, X_END - GRIPPER - 2 * BORDER, ARROW_LEFT ) ; 
			}
			else if ( X_END - ev ->x >= MAX_SIZE )
			{
				if ( X_END - X_START < MAX_SIZE )
				{
					X_START = X_END - MAX_SIZE ;

					if ( X_START < (GRIPPER + X_MIN_SPACE_BEFORE) )
						X_START = GRIPPER + X_MIN_SPACE_BEFORE ;
			
					//-1 to force the spinbutton to its max.
					signal_resize .emit( X_START - GRIPPER -1,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_LEFT ) ; 
				}
			}
			else if ( ev ->x <= (GRIPPER + X_MIN_SPACE_BEFORE) )
			{
				if ( X_START > (GRIPPER + X_MIN_SPACE_BEFORE) && X_END - X_START < MAX_SIZE )
				{
					X_START = GRIPPER + X_MIN_SPACE_BEFORE ;
				
					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_LEFT ) ;
				}
			}
			else if ( X_END - ev ->x <= MIN_SIZE )
			{
				if ( X_END - X_START > MIN_SIZE )
				{
					X_START = X_END - MIN_SIZE ;
			
					//+1 to force the spinbutton to its min.
					signal_resize .emit( X_START - GRIPPER +1,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_LEFT ) ;
				}
			}
		}

		else if ( GRIP_RIGHT )
		{
			if ( ev ->x < 500 + GRIPPER + BORDER * 2 &&
			     (ev ->x - X_START) < MAX_SIZE &&
			     (ev ->x - X_START) > MIN_SIZE )
			{
				X_END = static_cast<int>( ev ->x ) ;
			
				signal_resize .emit( X_START - GRIPPER, X_END - GRIPPER - BORDER * 2, ARROW_RIGHT ) ;
			}
			else if ( ev ->x - X_START >= MAX_SIZE )
			{
				if ( X_END - X_START < MAX_SIZE )
				{
					X_END = X_START + MAX_SIZE ;

					if ( X_END > 500 + GRIPPER + BORDER * 2 )
						X_END = 500 + GRIPPER + BORDER * 2 ;
		
					//+1 to force the spinbutton to its min.
					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2 +1,
							     ARROW_RIGHT ) ;
				}
			}
			else if ( ev ->x >= 500 + GRIPPER + BORDER * 2 )
			{
				if ( X_END < 500 + GRIPPER + BORDER * 2 && X_END - X_START < MAX_SIZE )
				{
					X_END = 500 + GRIPPER + BORDER * 2 ;
		
					signal_resize .emit( X_START -GRIPPER,
							     X_END - GRIPPER - BORDER * 2,
							     ARROW_RIGHT ) ;
				}
			}
			else if ( ev ->x - X_START <= MIN_SIZE )
			{
				if ( X_END - X_START > MIN_SIZE )
				{
					X_END = X_START + MIN_SIZE ;
		
					//-1 to force the spinbutton to its min.
					signal_resize .emit( X_START - GRIPPER,
							     X_END - GRIPPER - BORDER * 2 -1,
							     ARROW_RIGHT ) ;
				}
			}
		}
		
		else if ( GRIP_MOVE )
		{
			temp_x = X_START + static_cast<int>( ev ->x - X_START_MOVE );
			temp_y = X_END - X_START ;

			if ( temp_x > (GRIPPER + X_MIN_SPACE_BEFORE) && temp_x + temp_y < 500 + GRIPPER + BORDER * 2 )
			{
				X_START = temp_x ;
				X_END = X_START + temp_y ;
			}
			else if ( temp_x <= (GRIPPER + X_MIN_SPACE_BEFORE) )
			{
				if ( X_START > (GRIPPER + X_MIN_SPACE_BEFORE) )
				{
					X_START = GRIPPER + X_MIN_SPACE_BEFORE;
					X_END = X_START + temp_y ;
				}
			}
			else if ( temp_x + temp_y >= 500 + GRIPPER + BORDER * 2 )
			{
				if ( X_END < 500 + GRIPPER + BORDER * 2 )
				{
					X_END = 500 + GRIPPER + BORDER * 2 ;
					X_START = X_END - temp_y ;
				}
			}

			X_START_MOVE = static_cast<int>( ev ->x ) ;
			signal_move .emit( X_START - GRIPPER, X_END - GRIPPER - BORDER * 2 ) ;
		}
		
		Draw_Partition() ;
	}
	else
	{ 
		//check if pointer is over a gripper
		//left grip
		if ( ! fixed_start &&
		     ev ->x >= X_START - GRIPPER &&
		     ev ->x <= X_START &&
		     ev ->y >= 5 &&
		     ev ->y <= 45 )
			drawingarea .get_parent_window() ->set_cursor( *cursor_resize ) ;
		//right grip
		else if ( ev ->x >= X_END &&
			  ev ->x <= X_END + GRIPPER &&
			  ev ->y >= 5 && 
			  ev ->y <= 45 )
			drawingarea .get_parent_window() ->set_cursor( *cursor_resize ) ;
		//move grip
		else if ( ! fixed_start && 
			  ev ->x >= X_START && 
			  ev ->x <= X_END )
			drawingarea .get_parent_window() ->set_cursor( *cursor_move ) ;
		//normal pointer 
		else								
			drawingarea .get_parent_window() ->set_cursor() ;		
	}

	return true;
}

bool Frame_Resizer_Base::drawingarea_on_button_press_event( GdkEventButton *ev ) 
{
	GRIP_LEFT = GRIP_RIGHT = GRIP_MOVE = false;
	
	//left grip
	if ( ! fixed_start && 
	     ev ->x >= X_START - GRIPPER &&
	     ev ->x <= X_START && 
	     ev ->y >= 5 && 
	     ev ->y <= 45 ) 
		GRIP_LEFT = true ;
	//right grip
	else if ( ev ->x >= X_END && 
		  ev ->x <= X_END + GRIPPER && 
		  ev ->y >= 5 && 
		  ev ->y <= 45 ) 
		GRIP_RIGHT = true ;
	//move grip
	else if ( ! fixed_start && 
		  ev ->x >= X_START && 
		  ev ->x <= X_END )
	{
		 GRIP_MOVE = true ;
		 X_START_MOVE = static_cast<int>( ev ->x );
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
		drawingarea .get_parent_window() ->set_cursor() ;	
	
	return true;
}

void Frame_Resizer_Base::Draw_Partition()   
{
	UNUSED = X_END - X_START - BORDER * 2 - USED ;
	if ( UNUSED < 0 )
		UNUSED = 0 ;
	
	if ( drawingarea .get_window() )
	{
		//i couldn't find a clear() for a pixmap, that's why ;)
		gc_pixmap ->set_foreground( color_background );
		pixmap ->draw_rectangle( gc_pixmap, true, 0, 0, 536, 50 );
		
		//the two rectangles on each side of the partition
		gc_pixmap ->set_foreground( color_arrow_rectangle );
		pixmap ->draw_rectangle( gc_pixmap, true, 0, 0, 10, 50 );
		pixmap ->draw_rectangle( gc_pixmap, true, 526, 0, 10, 50 );
		
		//partition
		gc_pixmap ->set_foreground( color_partition );
		pixmap ->draw_rectangle( gc_pixmap, true, X_START, 0, X_END - X_START, 50 );
		
		//used
		gc_pixmap ->set_foreground( color_used );
		pixmap ->draw_rectangle( gc_pixmap, true, X_START +BORDER, BORDER, USED, 34 );
		
		//unused
		gc_pixmap ->set_foreground( color_unused );
		pixmap ->draw_rectangle( gc_pixmap, true, X_START +BORDER +USED, BORDER, UNUSED, 34 );
		
		//resize grips
		if ( ! fixed_start )
			Draw_Resize_Grip( ARROW_LEFT ) ;
		
		Draw_Resize_Grip( ARROW_RIGHT ) ;
		
		//and draw everything to "real" screen..
		drawingarea .get_window() ->draw_drawable( gc_drawingarea, pixmap, 0, 0, 0, 0 ) ;
	}
}

void Frame_Resizer_Base::Draw_Resize_Grip( ArrowType arrow_type ) 
{
	if ( arrow_type == ARROW_LEFT )
	{
		arrow_points[ 0 ] .set_x( X_START ) ;
		arrow_points[ 1 ] .set_x( X_START - GRIPPER ) ;
		arrow_points[ 2 ] .set_x( X_START ) ;
	}
	else
	{
		arrow_points[ 0 ] .set_x( X_END )  ;
		arrow_points[ 1 ] .set_x( X_END + GRIPPER ) ; 
		arrow_points[ 2 ] .set_x( X_END )  ;
	}
	
	//attach resize arrows to the partition
	gc_pixmap ->set_foreground( color_arrow_rectangle );
	pixmap ->draw_rectangle( gc_pixmap,
				 false,
				 arrow_type == ARROW_LEFT ? X_START - GRIPPER : X_END +1,
				 5,
				 9,
				 40 ) ;
	
	gc_pixmap ->set_foreground( color_arrow );
	pixmap ->draw_polygon( gc_pixmap, true, arrow_points );
}

Frame_Resizer_Base::~Frame_Resizer_Base()
{ 
	this ->get_colormap() ->free_color( color_used ) ;
	this ->get_colormap() ->free_color( color_unused ) ;
	this ->get_colormap() ->free_color( color_arrow ) ;
	this ->get_colormap() ->free_color( color_background ) ;
	this ->get_colormap() ->free_color( color_partition ) ;
	this ->get_colormap() ->free_color( color_arrow_rectangle ) ;
	
	delete cursor_resize;
	delete cursor_move;
}
