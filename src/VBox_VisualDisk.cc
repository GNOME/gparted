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

namespace GParted
{

VBox_VisualDisk::VBox_VisualDisk( const std::vector<Partition> & partitions, const Sector device_length)
{
	this ->partitions = partitions ;
	this ->device_length = device_length ;
	
	selected_partition = -1;
	//set locale for this stream to standard. this prevent weird locals from f*cking up my conversions :^)
	os.imbue(std::locale(""));

	//create frame which contains the visual disk
	frame_disk_legend = manage( new Gtk::Frame() ) ;
	frame_disk_legend ->set_shadow_type(Gtk::SHADOW_ETCHED_OUT);
	frame_disk_legend ->set_border_width( 5 );
	hbox_disk_main.pack_start( *frame_disk_legend, Gtk::PACK_EXPAND_PADDING );
	
	hbox_disk = NULL ;
	hbox_extended = NULL ;
	
	this->set_border_width( 10 );
	this->set_spacing( 8 );
	this->pack_start( hbox_disk_main, Gtk::PACK_SHRINK );
	
	//set and allocated some standard colors
	color_used.set( "#F8F8BA" ); 	this ->get_colormap() ->alloc_color( color_used ) ;//bleach yellow ;)
	color_unused.set( "white" );  	this ->get_colormap() ->alloc_color( color_unused ) ;
	color_text.set( "black" );  		this ->get_colormap() ->alloc_color( color_text ) ;
	
	
	//since disksegments have minimal sizes ( unallocated 15 and partitions 20 pixels ) i do some checking to prevent the visual disk from growing to much
	Sector sectors_per_pixel = Round(  (double) device_length / 750 ) ;
	
	double width, extra_pixels = 0 ;
	
	for ( unsigned int t=0; t<partitions.size() ; t++ )
	{
		width = (double)  (partitions[ t ].sector_end -  partitions[ t ].sector_start) / sectors_per_pixel ;
		
		if ( (partitions[ t ] .type == GParted::PRIMARY || partitions[ t ] .type == GParted::LOGICAL) && width < 20  )
			extra_pixels += (20 - width) ;
		else if ( partitions[ t ] .type == GParted::UNALLOCATED && width < 15 )
			extra_pixels += (15 - width) ;
	}
	
	//draw visual disk and its legend
	Build_Visual_Disk( 750 - Round( extra_pixels ) ) ;

	Build_Legend(  ) ;		
	
}

void VBox_VisualDisk::Build_Visual_Disk( int SCREEN_WIDTH ) 
{
	//since there is a 5 pixel space between every partition.... ( extended adds 2 *5 more, but that doesn't matter that much.
	//NOTE that a part < 20 will also grow to 20, so length in pixels may vary across different devices.. 
	SCREEN_WIDTH -= ( (partitions.size() -1) *5 ) ;
	
	//create hbox_disk and add to frame
	hbox_disk = manage( new Gtk::HBox() ) ;
	hbox_disk ->set_spacing( 5 );
	frame_disk_legend ->add ( *hbox_disk );
	
	
	//walk through all partitions....
	int x,y;//for pango_layout check
	for ( unsigned int i=0;i<partitions.size();i++ )
	{
		visual_partition = new Visual_Partition() ;
		visual_partitions.push_back( visual_partition ) ;
		visual_partitions.back()  ->index = i ;
		visual_partitions.back()  ->sector_start = partitions[i].sector_start ;
		
		if ( partitions[i].type == GParted::EXTENDED )
		{
			visual_partitions.back()  ->drawingarea = NULL ;//it's just a dummy ;-)  ( see Set_Selected() )
			visual_partitions.back()  ->length = 0 ; //keeps total_length clean
						
			eventbox_extended = manage( new Gtk::EventBox() ) ;
			eventbox_extended ->set_size_request(  -1 , 60 );
			eventbox_extended ->modify_bg( eventbox_extended ->get_state(), partitions[i].color  );
			hbox_disk ->pack_start( *eventbox_extended, Gtk::PACK_SHRINK ) ;
			
			hbox_extended = manage( new Gtk::HBox() ) ;
			hbox_extended ->set_border_width( 5 );
			hbox_extended ->set_spacing( 5 );
			eventbox_extended ->add( *hbox_extended ) ;
			continue;
		}
		
		visual_partitions.back()  ->length =  (int) (SCREEN_WIDTH / (double) ( device_length / (double)  ( partitions[i].sector_end - partitions[i].sector_start)  ) );
		if ( visual_partitions.back()  ->length < 20 )//we need a min. size. Otherwise some small partitions wouldn't be visible
			partitions[i].type == GParted::UNALLOCATED ? visual_partitions.back()  ->length = 15 : visual_partitions.back()  ->length = 20 ; 
		
		if ( partitions[i].inside_extended )
			{ visual_partitions.back()  ->height = 34 ; visual_partitions.back()  ->text_y = 10 ; }
		else
			{ visual_partitions.back()  ->height = 44 ;visual_partitions.back()  ->text_y = 15 ; }
		
		if (  partitions[i].type == GParted::UNALLOCATED )  
			visual_partitions.back()  ->used = -1;
		else 
			visual_partitions.back()  ->used = (int) ( (visual_partitions.back()  ->length  - (BORDER *2)) / (double) ( ( partitions[i].sector_end - partitions[i].sector_start) / (double)partitions[i].sectors_used ) ) ;
	
	
		visual_partitions.back()  ->color_fs =  partitions[i].color; 
		this ->get_colormap() ->alloc_color(visual_partitions.back()  ->color_fs);
		
		visual_partitions.back()  ->drawingarea = manage( new Gtk::DrawingArea() ) ;
		visual_partitions.back()  ->drawingarea ->set_size_request(  visual_partitions.back()  ->length+1  ,60 );
		visual_partitions.back()  ->drawingarea ->set_events(Gdk::BUTTON_PRESS_MASK);

		//connect signal handlers		
		visual_partitions.back()  ->drawingarea ->signal_button_press_event().connect(  sigc::bind<Partition>(sigc::mem_fun(*this, &VBox_VisualDisk::on_drawingarea_button_press), partitions[i] ) );
		visual_partitions.back()  ->drawingarea->signal_realize().connect( sigc::bind<Visual_Partition *>(sigc::mem_fun(*this, &VBox_VisualDisk::drawingarea_on_realize), visual_partitions.back()  ) );
		visual_partitions.back()  ->drawingarea ->signal_expose_event().connect(  sigc::bind<Visual_Partition *>(sigc::mem_fun(*this, &VBox_VisualDisk::drawingarea_on_expose), visual_partitions.back() ));
		
		//create  pangolayout and see if it fits in the visual partition
		os << partitions[i].partition << "\n" << String::ucompose( _("%1 MB"), partitions[i] .Get_Length_MB() ) ;
		visual_partitions.back()  ->pango_layout = visual_partitions.back()  ->drawingarea ->create_pango_layout ( os.str() ) ;os.str("");
		
		visual_partitions.back()  ->pango_layout ->get_pixel_size( x, y ) ;
		if ( visual_partitions.back()  ->length - BORDER * 2 -2 < x )
			visual_partitions.back()  ->pango_layout ->set_text( "" ) ;
		
		//tooltip
		if ( partitions[i].type != GParted::UNALLOCATED )
			os << partitions[i].filesystem << "\n" ;

		os << partitions[i].partition + "\n" + String::ucompose( _("%1 MB"), partitions[i] .Get_Length_MB() ) ;
		tooltips.set_tip( *(visual_partitions.back()  ->drawingarea) ,os.str() ) ; os.str("");
		
		partitions[i].inside_extended ? hbox_extended ->pack_start( *(visual_partitions.back()  ->drawingarea) , Gtk::PACK_SHRINK ) : hbox_disk ->pack_start( *(visual_partitions.back()  ->drawingarea) , Gtk::PACK_SHRINK ) ;

	}
	this ->show_all_children() ;
}


void VBox_VisualDisk::Build_Legend(  ) 
{ 
	//add the hboxes and frame_legenda
	frame_disk_legend = manage( new Gtk::Frame() ) ;
	hbox_legend_main.pack_start( *frame_disk_legend, Gtk::PACK_EXPAND_PADDING   );
	
	hbox_legend = manage( new Gtk::HBox() );
	frame_disk_legend ->add( *hbox_legend ) ;
	this->pack_start( hbox_legend_main  );
	
	std::vector<Glib::ustring> legende;
	bool legende_allow = true, only_unallocated = true;
		
	for ( unsigned int i=0;i<partitions.size();i++ )
	{
		if ( partitions[i].type != GParted::UNALLOCATED && partitions[i].type != GParted::EXTENDED )
			only_unallocated = false;
		
		//check if filesystem is already in legende
		for ( unsigned int t=0;t<legende.size();t++) //not very nice, but very effective :P
			if ( partitions[i].filesystem == legende[t] )
				{ legende_allow = false; break; }
			
		if ( legende_allow )
		{
			Add_Legend_Item( partitions[ i ] .filesystem + "    " , partitions[ i ] .color ) ;			
		
			//make sure this color isn't added to the legende again.
			legende.push_back( partitions[i].filesystem );
		}
			
		legende_allow = true;
	}
	
	//remove trailing whitespaces of last label_temp...
	if ( label_temp )
		label_temp ->set_text( label_temp ->get_text() .substr( 0, label_temp ->get_text() .length() -3)  ) ;
	
	
	//if there are any partitions add used/unused
	if ( ! only_unallocated )
	{
		frame_disk_legend = manage( new Gtk::Frame() ) ;
		hbox_legend_main.pack_start( *frame_disk_legend, Gtk::PACK_EXPAND_PADDING   );
		
		hbox_legend = manage( new Gtk::HBox() );
		frame_disk_legend ->add( *hbox_legend );
	
		Add_Legend_Item( (Glib::ustring) _( "used" ) + "    ", color_used  ) ;
		Add_Legend_Item( (Glib::ustring) _( "unused") + " ", color_unused ) ;
	}
		
}

void VBox_VisualDisk::Add_Legend_Item( const Glib::ustring & filesystem, const Gdk::Color & color )
{
	//the colored square
	entry_temp =  manage ( new Gtk::Entry() );
	entry_temp->set_sensitive( false );
	entry_temp->set_size_request( 15,15);
	entry_temp->modify_base( entry_temp->get_state(), color );
	hbox_legend ->pack_start( *entry_temp, Gtk::PACK_SHRINK   );
						
	//and the label
	label_temp = manage( new Gtk::Label( " " + filesystem ) );
	hbox_legend ->pack_start( *label_temp, Gtk::PACK_SHRINK   );
}

void VBox_VisualDisk::Set_Selected( const Partition & partition )
{
	//clean previously selected one
	temp = selected_partition ;
	selected_partition = -1;
	if ( temp >= 0 ) //prevent segfault at firsttime selection
		drawingarea_on_expose( NULL, visual_partitions[ temp ] ) ; 	
		
	
	if ( partition.type == GParted::EXTENDED )
		return; //extended can not be selected in visualdisk (yet )
	 
	//now set new selected one
	for ( unsigned int t=0;t< visual_partitions.size() ; t++ )
	{
		if ( visual_partitions[t] ->sector_start == partition.sector_start && visual_partitions[t] ->drawingarea != NULL )
		{
			selected_partition = t;
			drawingarea_on_expose( NULL, visual_partitions[ t ] ) ; 	
			return;
		}
	}
	
	
}


void VBox_VisualDisk::drawingarea_on_realize(  Visual_Partition* vp  )
{
	vp ->gc = Gdk::GC::create( vp ->drawingarea ->get_window() );
	
	vp ->drawingarea ->get_window() ->set_background( vp ->color_fs );
	//eventmasks necessary for tooltips
	vp ->drawingarea ->add_events(Gdk::ENTER_NOTIFY_MASK   );
	vp ->drawingarea ->add_events(Gdk::LEAVE_NOTIFY_MASK   );
	
}

bool VBox_VisualDisk::drawingarea_on_expose( GdkEventExpose * ev, Visual_Partition* vp  )
{ 
	vp ->drawingarea ->get_window() ->clear() ;
	
	if ( vp ->used != -1  ) //if not unknown or unallocated
	{
		vp ->gc ->set_foreground( color_used );
		vp ->drawingarea ->get_window() ->draw_rectangle( vp ->gc, true, BORDER,BORDER,vp ->used , vp ->height );
		
		vp ->gc ->set_foreground( color_unused );
		vp ->drawingarea ->get_window() ->draw_rectangle( vp ->gc, true, BORDER + vp ->used, BORDER, vp ->length - vp ->used - BORDER *2 , vp ->height );
	}
		
	vp ->gc ->set_foreground( color_text );
	vp ->drawingarea ->get_window() ->draw_layout( vp ->gc, BORDER +2, vp ->text_y, vp ->pango_layout ) ;
	
	//if partition is selected one..
	if ( vp ->index == selected_partition )
	{
		vp ->gc ->set_foreground( color_used );
		vp ->drawingarea ->get_window() ->draw_rectangle( vp ->gc, false, 4,4,vp ->length-8, vp ->height +8 );
	}
	
	return true;
}

bool VBox_VisualDisk::on_drawingarea_button_press( GdkEventButton *event, const Partition & partition )
{
	signal_mouse_click.emit( event, partition );
	return true;
}

VBox_VisualDisk::~VBox_VisualDisk()
{
	for ( unsigned int t=0;t< visual_partitions.size() ; t++ ) 
	{
		this ->get_colormap() ->free_colors( visual_partitions[t] ->color_fs , 1 ) ;
		delete visual_partitions[t] ;
	}
		
	visual_partitions.clear() ;
	
	//free the allocated colors
	this ->get_colormap() ->free_colors( color_used, 1 ) ;
	this ->get_colormap() ->free_colors( color_unused, 1 ) ;
	this ->get_colormap() ->free_colors( color_text, 1 ) ;

}




} //GParted
