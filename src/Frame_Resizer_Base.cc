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
 
#include "../include/Frame_Resizer_Base.h"

Frame_Resizer_Base::Frame_Resizer_Base()
{
	this ->fixed_start = false ;
	init( ) ;
}

void Frame_Resizer_Base::init() 
{
	drawingarea .set_size_request( 536, 50 );

	drawingarea .signal_realize( ) .connect( sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_realize) ) ;
	drawingarea .signal_expose_event( ) .connect( sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_expose) ) ;
	drawingarea .signal_motion_notify_event( ) .connect( sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_mouse_motion) ) ;
	drawingarea .signal_button_press_event( ) .connect( sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_button_press_event) ) ;
	drawingarea .signal_button_release_event( ) .connect( sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_button_release_event) ) ;
	drawingarea .signal_leave_notify_event( ) .connect( sigc::mem_fun(*this, &Frame_Resizer_Base::drawingarea_on_leave_notify) ) ;
		
	this ->add( drawingarea ) ;
	
	color_used .set( "#F8F8BA" );			this ->get_colormap( ) ->alloc_color( color_used ) ;
	color_unused .set( "white" );			this ->get_colormap( ) ->alloc_color( color_unused ) ;
	color_arrow .set( "black" );			this ->get_colormap( ) ->alloc_color( color_arrow ) ;
	color_background .set( "darkgrey" );		this ->get_colormap( ) ->alloc_color( color_background ) ;
	color_arrow_rectangle .set( "lightgrey" );	this ->get_colormap( ) ->alloc_color( color_arrow_rectangle ) ;
	
	cursor_resize = new Gdk::Cursor( Gdk::SB_H_DOUBLE_ARROW ) ; 
	cursor_normal = new Gdk::Cursor( Gdk::LEFT_PTR ) ;
	cursor_move   = new Gdk::Cursor( Gdk::FLEUR ) ; 
	  
	GRIP_MOVE = GRIP_LEFT = GRIP_RIGHT = false;
	X_END = 0;
	set_size_limits( 0, 500 ) ;
	
	Gdk::Point p;
	p .set_y( 15 ); 	arrow_points .push_back( p ) ;
	p .set_y( 25 ); 	arrow_points .push_back( p ) ;
	p .set_y( 35 ); 	arrow_points .push_back( p ) ;
	
	this ->show_all_children( );
}


void Frame_Resizer_Base::set_rgb_partition_color( const Gdk::Color & color )
{
	this ->get_colormap() ->free_colors( color_partition, 1 ) ;
	this ->color_partition = color ;
	this ->get_colormap() ->alloc_color( color_partition ) ;
}

void Frame_Resizer_Base::override_default_rgb_unused_color( const Gdk::Color & color ) 
{
	this ->get_colormap() ->free_colors( color_unused, 1 ) ;
	this ->color_unused = color ;
	this ->get_colormap() ->alloc_color( color_unused ) ;
}

void Frame_Resizer_Base::set_x_start( int x_start) 
{  
	this ->X_START = x_start +10;//space for leftgripper
} 

void Frame_Resizer_Base::set_x_end( int x_end ) 
{ 
	this ->X_END = x_end +26 ; //space for leftgripper + 2 * BORDER
}

void Frame_Resizer_Base::set_used( int  used)
{   
	this  ->USED = used ;
}

void Frame_Resizer_Base::set_fixed_start( bool fixed_start ) 
{
	this ->fixed_start = fixed_start ;
}

void Frame_Resizer_Base::set_used_start( int used_start ) 
{
	if ( used_start <= 0 )
		this ->USED_START = 10	;
	else
		this ->USED_START = used_start +10;
}

void Frame_Resizer_Base::set_size_limits( int min_size, int max_size )
{
	this ->MIN_SIZE = min_size + 16 ;
	this ->MAX_SIZE = max_size + 16 ; 
}

int Frame_Resizer_Base::get_used()
{
	return USED ;
}

int Frame_Resizer_Base::get_x_start() 
{
	return X_START -10  ;
}

int Frame_Resizer_Base::get_x_end() 
{
	return X_END -26 ;
}

void Frame_Resizer_Base::drawingarea_on_realize(  )
{
	gc = Gdk::GC::create( drawingarea .get_window() );
	
	drawingarea .get_window() ->set_background(color_background);
	
	drawingarea.add_events(Gdk::POINTER_MOTION_MASK );
	drawingarea.add_events(Gdk::BUTTON_PRESS_MASK );
	drawingarea.add_events(Gdk::BUTTON_RELEASE_MASK );
	drawingarea.add_events(Gdk::LEAVE_NOTIFY_MASK );
}

bool Frame_Resizer_Base::drawingarea_on_expose( GdkEventExpose * ev  )
{ 
	Draw_Partition() ;
	return true;
}

bool Frame_Resizer_Base::drawingarea_on_mouse_motion( GdkEventMotion *ev ) 
{
	if ( ! GRIP_LEFT && ! GRIP_RIGHT && ! GRIP_MOVE ) //no need to check this while resizing or moving
	{ 
		//check if pointer is over a gripper
		if ( ! fixed_start && ev ->x >= X_START -10 && ev ->x <= X_START && ev ->y >= 5 && ev ->y <= 45  )	//left grip
			drawingarea .get_parent_window() ->set_cursor( *cursor_resize ) ;
		else if (  ev ->x >= X_END && ev ->x <= X_END + 10 && ev ->y >= 5 && ev ->y <= 45 )			//right grip
			drawingarea .get_parent_window() ->set_cursor( *cursor_resize ) ;
		else if ( ! fixed_start && ev ->x >= X_START && ev ->x <= X_END )					//move grip
			drawingarea .get_parent_window() ->set_cursor( *cursor_move ) ;
		else																																						//normal pointer 
			drawingarea .get_parent_window() ->set_cursor( *cursor_normal ) ;		
	}

	
	//here's where the real work is done ;-)
	if ( GRIP_LEFT || GRIP_RIGHT || GRIP_MOVE) 
	{ 
		if ( GRIP_LEFT && ev ->x >= 10 && ev ->x <= X_END - USED - BORDER * 2 && (X_END - ev ->x) <= MAX_SIZE && (X_END - ev ->x) >= MIN_SIZE )
		{
			X_START =(int) ev -> x ;
			signal_resize.emit( X_START -10, X_END -26, ARROW_LEFT) ; //-10/-26 to get the real value ( this way gripper calculations are invisible outside this class )
		}
		
		else if ( GRIP_RIGHT && ev ->x <= 526 && ev ->x >= X_START + USED + BORDER *2 && (ev ->x - X_START) <= MAX_SIZE && (ev ->x - X_START) >= MIN_SIZE )
		{
			X_END = (int) ev ->x ;
			signal_resize.emit( X_START -10, X_END -26, ARROW_RIGHT) ; //-10/-26 to get the real value ( this way gripper calculations are invisible outside this class )
		}
		
		else if ( GRIP_MOVE )
		{
			temp_x = X_START + ((int) ev ->x - X_START_MOVE);
			temp_y = X_END + ( (int) ev ->x - X_START_MOVE);
			
			if ( temp_x >= 10 && temp_y <= 526 )
			{
				X_START = temp_x ;
				X_END =  temp_y ;
			}
			
			X_START_MOVE = (int) ev ->x ;		
			
			signal_move.emit( X_START -10, X_END -26) ;	//-10/-26 to get the real value ( this way gripper calculations are invisible outside this class )
		}
		
		Draw_Partition() ;
	}

	return true;
	
}

bool Frame_Resizer_Base::drawingarea_on_button_press_event( GdkEventButton *ev ) 
{
	GRIP_MOVE = false; GRIP_RIGHT = false; GRIP_LEFT = false ;
	
	if ( ! fixed_start && ev ->x >= X_START -10 && ev ->x <= X_START && ev ->y >= 5 && ev ->y <= 45  ) //left grip
		GRIP_LEFT = true ;
	else if (  ev ->x >= X_END && ev ->x <= X_END + 10 && ev ->y >= 5 && ev ->y <= 45 ) //right grip
		GRIP_RIGHT = true ;
	else if ( ! fixed_start && ev ->x >= X_START && ev ->x <= X_END )															//move grip
		{ GRIP_MOVE = true ; X_START_MOVE = (int)ev ->x; }
	
	return true;
}

bool Frame_Resizer_Base::drawingarea_on_button_release_event( GdkEventButton *ev ) 
{
	GRIP_LEFT = false ; GRIP_RIGHT = false ; GRIP_MOVE = false;
	
	return true;
}

bool Frame_Resizer_Base::drawingarea_on_leave_notify( GdkEventCrossing *ev )
{
	if ( ev ->mode != GDK_CROSSING_GRAB && ! GRIP_LEFT && ! GRIP_RIGHT && ! GRIP_MOVE ) 
		drawingarea .get_parent_window() ->set_cursor( *cursor_normal ) ;	
	
	return true;
}

void Frame_Resizer_Base::Draw_Partition( )   
{
	UNUSED = X_END - X_START - BORDER * 2 - USED ;
	if ( UNUSED < 0 )
		UNUSED = 0 ;
	
	if ( drawingarea .get_window( ) )
	{
		drawingarea .get_window() ->clear() ;
		
		//the two rectangles on each side of the partition
		gc ->set_foreground( color_arrow_rectangle );
		drawingarea .get_window() ->draw_rectangle( gc, true, 0,0,10,50 );
		drawingarea .get_window() ->draw_rectangle( gc, true, 526,0,10,50 );
		
		//partition
		gc ->set_foreground( color_partition );
		drawingarea .get_window() ->draw_rectangle( gc, true, X_START,0,X_END - X_START,50 );
		
		//used
		gc ->set_foreground( color_used );
		drawingarea .get_window() ->draw_rectangle( gc, true, X_START + BORDER, BORDER, USED ,34 );
		
		//unused
		gc ->set_foreground( color_unused );
		drawingarea .get_window() ->draw_rectangle( gc, true, X_START + BORDER +USED, BORDER, UNUSED,34 );
		
		//resize grips
		if ( ! fixed_start )
			Draw_Resize_Grip( ARROW_LEFT ) ;
		
		Draw_Resize_Grip( ARROW_RIGHT ) ;
	}
	
}

void Frame_Resizer_Base::Draw_Resize_Grip( ArrowType arrow_type ) 
{
	if ( arrow_type == ARROW_LEFT )
	{
		arrow_points[0] .set_x( X_START ) ;
		arrow_points[1] .set_x( X_START -10 ) ;
		arrow_points[2] .set_x( X_START ) ;
	}
	else
	{
		arrow_points[0] .set_x( X_END )  ;
		arrow_points[1] .set_x( X_END +10 ) ; 
		arrow_points[2] .set_x( X_END )  ;
	}
	
	//attach resize arrows to the partition
	gc ->set_foreground(  color_arrow_rectangle );
	
	if ( arrow_type == ARROW_LEFT )
		drawingarea .get_window() ->draw_rectangle( gc, false, X_START -10 , 5, 9, 40 );
	else
		drawingarea .get_window() ->draw_rectangle( gc, false, X_END +1, 5, 9, 40 );
	
	gc ->set_foreground( color_arrow );
	drawingarea .get_window() ->draw_polygon( gc , true, arrow_points  );
	
}


Frame_Resizer_Base::~Frame_Resizer_Base()
{ 
	this ->get_colormap() ->free_colors( color_used, 1 ) ;
	this ->get_colormap() ->free_colors( color_unused, 1 ) ;
	this ->get_colormap() ->free_colors( color_arrow, 1 ) ;
	this ->get_colormap() ->free_colors( color_background, 1 ) ;
	this ->get_colormap() ->free_colors( color_partition, 1 ) ;
	this ->get_colormap() ->free_colors( color_arrow_rectangle, 1 ) ;
	
	if ( cursor_resize )
		delete cursor_resize ;
	if ( cursor_normal )
		delete cursor_normal ;
	if ( cursor_move )
		delete cursor_move ;
}
