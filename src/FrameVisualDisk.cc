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

#include "../include/FrameVisualDisk.h"

#define BORDER 8
#define SEP 5
#define HEIGHT 70

namespace GParted
{

FrameVisualDisk::FrameVisualDisk()
{
	this ->set_border_width( 5 ) ;
	this ->set_shadow_type( Gtk::SHADOW_ETCHED_OUT ) ;
	
	//set and allocated some standard colors
	color_used .set( Utils::Get_Color( GParted::FS_USED ) );
	this ->get_colormap() ->alloc_color( color_used ) ;
	
	color_unused .set( Utils::Get_Color( GParted::FS_UNUSED ) );
	this ->get_colormap() ->alloc_color( color_unused ) ;
	
	color_text .set( "black" );
	this ->get_colormap() ->alloc_color( color_text ) ;

	//prepare drawingarea and frame and pack them	
	drawingarea .set_events( Gdk::BUTTON_PRESS_MASK );
	
	drawingarea .signal_realize() .connect( sigc::mem_fun(*this, &FrameVisualDisk::drawingarea_on_realize) ) ;
	drawingarea .signal_expose_event() .connect( sigc::mem_fun(*this, &FrameVisualDisk::drawingarea_on_expose) ) ;
	drawingarea .signal_button_press_event() .connect( sigc::mem_fun( *this, &FrameVisualDisk::on_drawingarea_button_press) ) ;
	
	drawingarea .set_size_request( -1, HEIGHT ) ;

	this ->add( drawingarea ) ;
}
	
void FrameVisualDisk::load_partitions( const std::vector<Partition> & partitions, Sector device_length )
{
	clear() ;	
	
	TOT_SEP = get_total_separator_px( partitions ) ;
	set_static_data( partitions, visual_partitions, device_length ) ;

	drawingarea .queue_resize() ;
}

void FrameVisualDisk::set_selected( const Partition & partition ) 
{
	set_selected( visual_partitions, partition ) ;
	
	draw_partitions( visual_partitions ) ;
}

void FrameVisualDisk::clear()
{
	free_colors( visual_partitions ) ;
	visual_partitions .clear() ;
	
	drawingarea .queue_resize() ;
}
	
int FrameVisualDisk::get_total_separator_px( const std::vector<Partition> & partitions ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
		if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			return ( partitions[ t ] .logicals .size() -1 + partitions .size() -1 ) * SEP ;


	return ( partitions .size() -1 ) * SEP ;
}	

void FrameVisualDisk::set_static_data( const std::vector<Partition> & partitions, std::vector<visual_partition> & visual_partitions, Sector length ) 
{
	Sector p_length ;
	visual_partition vp ;
	
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		visual_partitions .push_back( vp ) ;

		visual_partitions .back() .partition = partitions[ t ] ; 
		
		p_length = partitions[ t ] .sector_end - partitions[ t ] .sector_start ;
		visual_partitions .back() .fraction = p_length / static_cast<double>( length ) ;

		if ( partitions[ t ] .type == GParted::TYPE_UNALLOCATED || partitions[ t ] .type == GParted::TYPE_EXTENDED )
			visual_partitions .back() .fraction_used = -1 ;
		else if ( partitions[ t ] .sectors_used > 0 )
			visual_partitions .back() .fraction_used = partitions[ t ] .sectors_used / static_cast<double>( p_length ) ;
	
		visual_partitions .back() .color = partitions[ t ] .color; 
		this ->get_colormap( ) ->alloc_color( visual_partitions .back() .color );

		if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			set_static_data( partitions[ t ] .logicals, 
				      	 visual_partitions .back() .logicals,
	   			         partitions[ t ] .sector_end - partitions[ t ] .sector_start ) ;
		else
			visual_partitions .back() .pango_layout = drawingarea .create_pango_layout( 
				partitions[ t ] .partition + "\n" + Utils::format_size( partitions[ t ] .get_length() ) ) ; 
	}
}

int FrameVisualDisk::calc_length( std::vector<visual_partition> & visual_partitions, int length_px ) 
{
	int calced_length = 0 ;

	for ( int t = 0 ; t < static_cast<int>( visual_partitions .size() ) ; t++ )
	{
		visual_partitions[ t ] .length = Utils::Round( length_px * visual_partitions[ t ] .fraction ) ;
			
		if ( visual_partitions[ t ] .logicals .size() > 0 )
			visual_partitions[ t ] .length = 
				calc_length( visual_partitions[ t ] .logicals, visual_partitions[ t ] .length - (2 * BORDER) ) + (2 * BORDER) ;
		else if ( visual_partitions[ t ] .length < MIN_SIZE )
			visual_partitions[ t ] .length = MIN_SIZE ;
	
		calced_length += visual_partitions[ t ] .length ;
	}
	
	return calced_length + (visual_partitions .size() - 1) * SEP ;
}

void FrameVisualDisk::calc_position_and_height( std::vector<visual_partition> & visual_partitions, int start, int border ) 
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		visual_partitions[ t ] .x_start = start ;
		visual_partitions[ t ] .y_start = border ;
		visual_partitions[ t ] .height = HEIGHT - ( border * 2 ) ;

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			calc_position_and_height( visual_partitions[ t ] .logicals, 
						     visual_partitions[ t ] .x_start + BORDER,
  						     BORDER ) ;

		start += visual_partitions[ t ] .length + SEP ;
	}
}

void FrameVisualDisk::calc_used_unused( std::vector<visual_partition> & visual_partitions ) 
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		if ( visual_partitions[ t ] .fraction_used > -1 )
		{ 
			//used
			visual_partitions[ t ] .x_used_start = visual_partitions[ t ] .x_start + BORDER ;
		
			if ( visual_partitions[ t ] .fraction_used )
				visual_partitions[ t ] .used_length =
					Utils::Round( ( visual_partitions[ t ] .length - (2*BORDER) ) * visual_partitions[ t ] .fraction_used ) ;

			//unused
			visual_partitions[ t ] .x_unused_start = visual_partitions[ t ] .x_used_start + visual_partitions[ t ] .used_length  ;
			visual_partitions[ t ] .unused_length
				= visual_partitions[ t ] .length - (2 * BORDER) - visual_partitions[ t ] .used_length ;

			//y position and height
			visual_partitions[ t ] .y_used_unused_start = visual_partitions[ t ] .y_start + BORDER ;
			visual_partitions[ t ] .used_unused_height = visual_partitions[ t ] .height - (2 * BORDER) ;
		}

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			calc_used_unused( visual_partitions[ t ] .logicals ) ;
	}
}
	
void FrameVisualDisk::calc_text( std::vector<visual_partition> & visual_partitions ) 
{
	int length, height ;
	
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		if ( visual_partitions[ t ] .pango_layout )
		{
			//see if the text fits in the partition... (and if so, center the text..)
			visual_partitions[ t ] .pango_layout ->get_pixel_size( length, height ) ;
			if ( length < visual_partitions[ t ] .length - (2 * BORDER) - 2 )
			{
				visual_partitions[ t ] .x_text = visual_partitions[ t ] .x_start + 
					Utils::Round( (visual_partitions[ t ] .length / 2) - (length / 2) ) ;

				visual_partitions[ t ] .y_text = visual_partitions[ t ] .y_start + 
					Utils::Round( (visual_partitions[ t ] .height / 2) - (height / 2) ) ;
			}
			else
				visual_partitions[ t ] .x_text = visual_partitions[ t ] .y_text = 0 ;
		}

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			calc_text( visual_partitions[ t ] .logicals ) ;
	}
}

void FrameVisualDisk::draw_partitions( const std::vector<visual_partition> & visual_partitions ) 
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		//partition...
		gc ->set_foreground( visual_partitions[ t ] .color );
		drawingarea .get_window() ->draw_rectangle( gc, 
						 	    true, 
							    visual_partitions[ t ] .x_start, 
							    visual_partitions[ t ] .y_start, 
							    visual_partitions[ t ] .length,
							    visual_partitions[ t ] .height );
	
		//used..
		if ( visual_partitions[ t ] .used_length > 0 )
		{
			gc ->set_foreground( color_used );
			drawingarea .get_window() ->draw_rectangle( gc, 
								    true, 
								    visual_partitions[ t ] .x_used_start, 
								    visual_partitions[ t ] .y_used_unused_start, 
								    visual_partitions[ t ] .used_length,
								    visual_partitions[ t ] .used_unused_height );
		}
		
		//unused
		if ( visual_partitions[ t ] .unused_length > 0 )
		{
			gc ->set_foreground( color_unused );
			drawingarea .get_window() ->draw_rectangle( gc, 
								    true, 
							     	    visual_partitions[ t ] .x_unused_start, 
								    visual_partitions[ t ] .y_used_unused_start, 
								    visual_partitions[ t ] .unused_length,
								    visual_partitions[ t ] .used_unused_height );
		}

		//text
		if ( visual_partitions[ t ] .x_text > 0 )
		{
			gc ->set_foreground( color_text );
			drawingarea .get_window() ->draw_layout( gc,
								 visual_partitions[ t ] .x_text,
								 visual_partitions[ t ] .y_text,
								 visual_partitions[ t ] .pango_layout ) ;
		}

		//selection rectangle...
		if ( visual_partitions[ t ] .selected )
		{
			gc ->set_foreground( color_used );
			//selection start and ends at 4px from the borders, hence the >8 restriction
			if ( visual_partitions[ t ] .length > 8 )
				drawingarea .get_window() ->draw_rectangle( gc,
									    false, 
									    visual_partitions[ t ] .x_start +4, 
									    visual_partitions[ t ] .y_start +4, 
									    visual_partitions[ t ] .length -9, 
									    visual_partitions[ t ] .height -9 );
			else
				drawingarea .get_window() ->draw_rectangle( gc,
									    true, 
									    visual_partitions[ t ] .x_start, 
									    visual_partitions[ t ] .y_start +9, 
									    visual_partitions[ t ] .length, 
									    visual_partitions[ t ] .height -19 );
		}

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			draw_partitions( visual_partitions[ t ] .logicals ) ;
	}
}

bool FrameVisualDisk::set_selected( std::vector<visual_partition> & visual_partitions, int x, int y ) 
{
	bool found = false ;
	
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		if ( visual_partitions[ t ] .x_start <= x && x < visual_partitions[ t ] .x_start + visual_partitions[ t ] .length &&
		     visual_partitions[ t ] .y_start <= y && y < visual_partitions[ t ] .y_start + visual_partitions[ t ] .height )
		{
			visual_partitions[ t ] .selected = true ;
			selected_vp = visual_partitions[ t ] ;
			found = true ;
		}
		else
			visual_partitions[ t ] .selected = false ;

		if ( visual_partitions[ t ] .logicals .size() > 0  ) 
			visual_partitions[ t ] .selected &= ! set_selected( visual_partitions[ t ] .logicals, x, y ) ;
	}

	return found ;
}

void FrameVisualDisk::set_selected( std::vector<visual_partition> & visual_partitions, const Partition & partition ) 
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		if ( visual_partitions[ t ] .partition == partition )
		{
			visual_partitions[ t ] .selected = true ;
			selected_vp = visual_partitions[ t ] ;
		}
		else
			visual_partitions[ t ] .selected = false ;

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			set_selected( visual_partitions[ t ] .logicals, partition ) ;
	}
}

void FrameVisualDisk::drawingarea_on_realize()
{
	gc = Gdk::GC::create( drawingarea .get_window() );
	
	//connect here to prevent premature signalling (only relevant at startup)
	drawingarea .signal_size_allocate() .connect( sigc::mem_fun( *this, &FrameVisualDisk::on_resize ) ) ;
}

bool FrameVisualDisk::drawingarea_on_expose( GdkEventExpose * event )
{
	draw_partitions( visual_partitions ) ;

	return true ;
}

bool FrameVisualDisk::on_drawingarea_button_press( GdkEventButton * event )
{
	set_selected( visual_partitions, static_cast<int>( event ->x ), static_cast<int>( event ->y ) ) ;
	draw_partitions( visual_partitions ) ;
	
	signal_partition_selected .emit( selected_vp .partition, false ) ;	

	if ( event ->type == GDK_2BUTTON_PRESS  )
		signal_partition_activated .emit() ;
	else if ( event ->button == 3 )  
		signal_popup_menu .emit( event ->button, event ->time ) ;

	return true ;
}

void FrameVisualDisk::on_resize( Gtk::Allocation & allocation ) 
{
	MIN_SIZE = 20 ;

	int calced, TOTAL ;
	do
	{
		TOTAL = allocation .get_width() - TOT_SEP ;
		calced = allocation .get_width() ; //for first time :)
		do 
		{
			TOTAL -= ( calced - allocation .get_width() ) ;
			calced = calc_length( visual_partitions, TOTAL ) ;
		}
		while ( calced > allocation .get_width() && TOTAL > 0 ) ; 

		MIN_SIZE-- ;
	}
	while ( TOTAL <= 0 && MIN_SIZE > 0 ) ; 

	//due to rounding a few px may be lost (max. 2), lets add these to the last partition.
	if ( allocation .get_width() > calced )
		visual_partitions .back() .length += ( allocation .get_width() - calced ) ;
	
	calc_position_and_height( visual_partitions, 0, 0 ) ;
	calc_used_unused( visual_partitions ) ;
	calc_text( visual_partitions ) ;
}

void FrameVisualDisk::free_colors( std::vector<visual_partition> & visual_partitions ) 
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		this ->get_colormap() ->free_colors( visual_partitions[ t ] .color, 1 ) ;

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			free_colors( visual_partitions[ t ] .logicals ) ;
	}
}

FrameVisualDisk::~FrameVisualDisk()
{
	clear() ;

	//free the allocated colors
	this ->get_colormap() ->free_colors( color_used, 1 ) ;
	this ->get_colormap() ->free_colors( color_unused, 1 ) ;
	this ->get_colormap() ->free_colors( color_text, 1 ) ;
}

} //GParted
