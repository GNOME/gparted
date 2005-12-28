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

#include "../include/VBox_VisualDisk.h"

#include <gtkmm/image.h>

#define BORDER 8
#define SEP 5
#define HEIGHT 70

namespace GParted
{

VBox_VisualDisk::VBox_VisualDisk()
{
	this ->set_border_width( 10 );
	this ->set_spacing( 5 );
	
	//set and allocated some standard colors
	color_used .set( Utils::Get_Color( GParted::FS_USED ) );
	this ->get_colormap() ->alloc_color( color_used ) ;
	
	color_unused .set( Utils::Get_Color( GParted::FS_UNUSED ) );
	this ->get_colormap() ->alloc_color( color_unused ) ;
	
	color_text .set( "black" );
	this ->get_colormap() ->alloc_color( color_text ) ;

	//prepare drawingarea and frame and pack them	
	drawingarea .set_events( Gdk::BUTTON_PRESS_MASK );
	
	drawingarea .signal_realize() .connect( sigc::mem_fun(*this, &VBox_VisualDisk::drawingarea_on_realize) ) ;
	drawingarea .signal_expose_event() .connect( sigc::mem_fun(*this, &VBox_VisualDisk::drawingarea_on_expose) ) ;
	drawingarea .signal_button_press_event() .connect( sigc::mem_fun( *this, &VBox_VisualDisk::on_drawingarea_button_press) ) ;
	
	drawingarea .set_size_request( -1, HEIGHT ) ;

	frame = manage( new Gtk::Frame() ) ;
	frame ->add( drawingarea );
	frame ->set_shadow_type( Gtk::SHADOW_ETCHED_OUT ) ;
	this ->pack_start( *frame, Gtk::PACK_EXPAND_WIDGET ) ;

	//pack hbox which holds the legend
	this ->pack_start( hbox_legend );
}
	
void VBox_VisualDisk::load_partitions( const std::vector<Partition> & partitions, Sector device_length )
{
	clear() ;	

	TOT_SEP = get_total_separator_px( partitions ) ;
	set_static_data( partitions, visual_partitions, device_length ) ;

	drawingarea .queue_resize() ;

	build_legend( partitions ) ;
}

void VBox_VisualDisk::set_selected( const Partition & partition ) 
{
	set_selected( visual_partitions, partition ) ;
	
	draw_partitions( visual_partitions ) ;
}

void VBox_VisualDisk::clear()
{
	free_colors( visual_partitions ) ;
	visual_partitions .clear() ;
	hbox_legend .children() .clear() ;
	
	drawingarea .queue_resize() ;
}
	
int VBox_VisualDisk::get_total_separator_px( const std::vector<Partition> & partitions ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
		if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			return ( partitions[ t ] .logicals .size() -1 + partitions .size() -1 ) * SEP ;


	return ( partitions .size() -1 ) * SEP ;
}	

void VBox_VisualDisk::set_static_data( const std::vector<Partition> & partitions, std::vector<visual_partition> & visual_partitions, Sector length ) 
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
				partitions[ t ] .partition + "\n" + String::ucompose( _("%1 MB"), partitions[ t ] .Get_Length_MB() ) ) ;
	}
}

int VBox_VisualDisk::calc_length( std::vector<visual_partition> & visual_partitions, int length_px ) 
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

void VBox_VisualDisk::calc_position_and_height( std::vector<visual_partition> & visual_partitions, int start, int border ) 
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

void VBox_VisualDisk::calc_used_unused( std::vector<visual_partition> & visual_partitions ) 
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
	
void VBox_VisualDisk::calc_text( std::vector<visual_partition> & visual_partitions ) 
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

void VBox_VisualDisk::build_legend( const std::vector<Partition> & partitions ) 
{
	std::vector<GParted::FILESYSTEM> legend ;
	prepare_legend( partitions, legend ) ;

	frame = manage( new Gtk::Frame() ) ;
	hbox_legend .pack_start( *frame, Gtk::PACK_EXPAND_PADDING );
	
	Gtk::HBox * hbox = manage( new Gtk::HBox( false, 15 ) );
	hbox ->set_border_width( 3 ) ;
	frame ->add( *hbox ) ;
	
	bool hide_used_unused = true;
	for ( unsigned int t = 0 ; t < legend .size() ; t++ )
	{
		hbox ->pack_start( * create_legend_item( legend[ t ] ), Gtk::PACK_SHRINK ) ;
		
		if ( legend[ t ] != GParted::FS_UNALLOCATED && legend[ t ] != GParted::FS_EXTENDED && legend[ t ] != GParted::FS_LINUX_SWAP )
			hide_used_unused = false ;
	}

	if ( ! hide_used_unused )
	{
		frame = manage( new Gtk::Frame() ) ;
		hbox_legend .pack_start( *frame, Gtk::PACK_EXPAND_PADDING );
	
		hbox = manage( new Gtk::HBox( false, 15 ) );
		hbox ->set_border_width( 3 ) ;
		frame ->add( *hbox ) ;

		hbox ->pack_start( * create_legend_item( GParted::FS_USED ), Gtk::PACK_SHRINK ) ;
		hbox ->pack_start( * create_legend_item( GParted::FS_UNUSED ), Gtk::PACK_SHRINK ) ;
	}

	hbox_legend .show_all_children() ;
}
	
void VBox_VisualDisk::prepare_legend( const std::vector<Partition> & partitions, std::vector<GParted::FILESYSTEM> & legend ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		if ( std::find( legend .begin(), legend .end(), partitions[ t ] .filesystem ) == legend .end() )
			legend .push_back( partitions[ t ] .filesystem );
		
		if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			prepare_legend( partitions[ t ] .logicals, legend ) ;
	}
}

Gtk::HBox * VBox_VisualDisk::create_legend_item( GParted::FILESYSTEM fs ) 
{
	Gtk::HBox * hbox = manage( new Gtk::HBox( false, 5 ) ) ;
	hbox ->pack_start( * manage( new Gtk::Image( Utils::get_color_as_pixbuf( fs, 16, 16 ) ) ), Gtk::PACK_SHRINK );
		
	hbox ->pack_start( * Utils::mk_label( Utils::Get_Filesystem_String( fs ) ), Gtk::PACK_SHRINK );

	return hbox ;
}
	
void VBox_VisualDisk::draw_partitions( const std::vector<visual_partition> & visual_partitions ) 
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

bool VBox_VisualDisk::set_selected( std::vector<visual_partition> & visual_partitions, int x, int y ) 
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

void VBox_VisualDisk::set_selected( std::vector<visual_partition> & visual_partitions, const Partition & partition ) 
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

void VBox_VisualDisk::drawingarea_on_realize()
{
	gc = Gdk::GC::create( drawingarea .get_window() );
	
	//connect here to prevent premature signalling (only relevant at startup)
	drawingarea .signal_size_allocate() .connect( sigc::mem_fun( *this, &VBox_VisualDisk::on_resize ) ) ;
}

bool VBox_VisualDisk::drawingarea_on_expose( GdkEventExpose * event )
{
	draw_partitions( visual_partitions ) ;

	return true ;
}

bool VBox_VisualDisk::on_drawingarea_button_press( GdkEventButton * event )
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

void VBox_VisualDisk::on_resize( Gtk::Allocation & allocation ) 
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
		
	calc_position_and_height( visual_partitions, 0, 0 ) ;
	calc_used_unused( visual_partitions ) ;
	calc_text( visual_partitions ) ;
}

void VBox_VisualDisk::free_colors( std::vector<visual_partition> & visual_partitions ) 
{
	for ( unsigned int t = 0 ; t < visual_partitions .size() ; t++ )
	{
		this ->get_colormap() ->free_color( visual_partitions[ t ] .color ) ;

		if ( visual_partitions[ t ] .logicals .size() > 0 )
			free_colors( visual_partitions[ t ] .logicals ) ;
	}
}

VBox_VisualDisk::~VBox_VisualDisk()
{
	clear() ;

	//free the allocated colors
	this ->get_colormap() ->free_colors( color_used, 1 ) ;
	this ->get_colormap() ->free_colors( color_unused, 1 ) ;
	this ->get_colormap() ->free_colors( color_text, 1 ) ;
}

} //GParted
