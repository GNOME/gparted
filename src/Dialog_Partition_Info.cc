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
 
#include "../include/Dialog_Partition_Info.h"

namespace GParted
{

Dialog_Partition_Info::Dialog_Partition_Info( const Partition & partition )
{
	this -> partition = partition ;

	this->set_resizable( false );
	
	/*TO TRANSLATORS: dialogtitle, looks like Information about /dev/hda3 */
	this->set_title( String::ucompose( _( "Information about %1"), partition.partition ) );
	 
	init_drawingarea() ;
	
	//add label for detail and fill with relevant info
	Display_Info() ;
	
	//display error (if any)
	if ( partition .error != "" )
	{
		frame = manage( new Gtk::Frame() );
		frame ->set_border_width( 10 );
		
		image = manage( new Gtk::Image( Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON ) );
		label = manage( new Gtk::Label( ) ) ;
		label ->set_markup( "<b> " + (Glib::ustring) _( "Libparted message:" ) + " </b>" ) ;
		
		hbox = manage( new Gtk::HBox() );
		hbox ->pack_start( *image, Gtk::PACK_SHRINK ) ;
		hbox ->pack_start( *label, Gtk::PACK_SHRINK ) ;
		
		frame ->set_label_widget( *hbox ) ;
		
		label = manage( new Gtk::Label( ) ) ;
		label ->set_markup( "<i>" +  partition.error + "</i>") ;
		label ->set_selectable( true ) ;
		label ->set_line_wrap( true ) ;
		frame ->add( *label );
		
		this ->get_vbox() ->pack_start( *frame, Gtk::PACK_SHRINK ) ;
	}
		
		
	this->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_OK );
	this->show_all_children();
}

void Dialog_Partition_Info::drawingarea_on_realize(  )
{
	gc = Gdk::GC::create( drawingarea .get_window() );
	
	drawingarea .get_window() ->set_background( color_partition );
	
}

bool Dialog_Partition_Info::drawingarea_on_expose( GdkEventExpose *ev  )
{
	if ( partition.type != GParted::UNALLOCATED ) 
	{
		//used
		gc ->set_foreground(  color_used );
		drawingarea .get_window() ->draw_rectangle( gc, true, BORDER, BORDER, used ,34 );
		
		//unused
		gc ->set_foreground(  color_unused );
		drawingarea .get_window() ->draw_rectangle( gc, true, BORDER + used, BORDER, unused,34 );
	}
	
	//text
	gc ->set_foreground(  color_text );
	drawingarea .get_window() ->draw_layout( gc, BORDER +5, BORDER +1 , pango_layout ) ;
	
	return true;
}

void Dialog_Partition_Info::init_drawingarea() 
{
	drawingarea .set_size_request( 375, 50 ) ;
	drawingarea.signal_realize().connect( sigc::mem_fun(*this, &Dialog_Partition_Info::drawingarea_on_realize) ) ;
	drawingarea.signal_expose_event().connect( sigc::mem_fun(*this, &Dialog_Partition_Info::drawingarea_on_expose) ) ;
	
	frame = manage( new Gtk::Frame() );
	frame ->add ( drawingarea ) ;
	
	frame ->set_shadow_type(Gtk::SHADOW_ETCHED_OUT);
	frame ->set_border_width( 10 );
	hbox = manage( new Gtk::HBox() ) ;
	hbox ->pack_start( *frame, Gtk::PACK_EXPAND_PADDING ) ;
	
	this ->get_vbox() ->pack_start( *hbox, Gtk::PACK_SHRINK ) ;
	
	//calculate proportional width of used and unused
	used = unused = 0 ;
	used 		= (int ) ( (  (375 - BORDER *2) / ( (double)  (partition.sector_end - partition.sector_start) / partition.sectors_used ) )+0.5 ) ;
	unused  	= 375 - used - (BORDER *2) ;
	
	//allocate some colors
	color_used.set( "#F8F8BA" );						this ->get_colormap() ->alloc_color( color_used ) ;
	
	partition .type == GParted::EXTENDED ?  color_unused.set( "darkgrey" ) : color_unused.set( "white" ) ;
	this ->get_colormap() ->alloc_color( color_unused ) ;

	color_text.set( "black" );								this ->get_colormap() ->alloc_color( color_text ) ;
	color_partition = partition.color ;							this ->get_colormap() ->alloc_color( color_partition ) ;	 
	
	//set text of pangolayout
	os << partition .partition <<"\n" << String::ucompose( _("%1 MB"), partition .Get_Length_MB() ) ;
	pango_layout = drawingarea .create_pango_layout ( os.str() ) ;os.str("");
	
}

void Dialog_Partition_Info::Display_Info()
{  
	int top =0, bottom = 1;
	
	table = manage( new Gtk::Table() ) ;
	table ->set_border_width( 5 ) ;
	table ->set_col_spacings(10 ) ;
	this ->get_vbox() ->pack_start( *table , Gtk::PACK_SHRINK ) ;
	
	//filesystem
	table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Filesystem:" ) + "</b>" ) , 0,1,top, bottom ,Gtk::FILL);
	table ->attach( * mk_label( partition .filesystem ), 1,2, top++, bottom++, Gtk::FILL);
	
	//size
	table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Size:" ) + "</b>" ), 0,1,top, bottom,Gtk::FILL);
	table ->attach( * mk_label( String::ucompose( _("%1 MB"), this ->partition .Get_Length_MB() ) ) , 1,2, top++, bottom++,Gtk::FILL);
	
	if ( partition.sectors_used != -1 )
	{
		//calculate relative diskusage
		int percent_used =(int)  (  (double) partition.Get_Used_MB()  / partition .Get_Length_MB() *100 +0.5 ) ;
				
		//used
		table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Used:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
		table ->attach( * mk_label( String::ucompose( _("%1 MB"), this ->partition .Get_Used_MB() ) ) , 1,2, top, bottom,Gtk::FILL);
		os << percent_used ;
		table ->attach( * mk_label( "\t\t\t( " + os.str() + "% )"), 1,2, top++, bottom++,Gtk::FILL); os.str("") ;
		
		//unused
		table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Unused:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
		table ->attach( * mk_label( String::ucompose( _("%1 MB"), this ->partition .Get_Unused_MB() ) ) , 1,2, top, bottom,Gtk::FILL);
		os << 100 - percent_used ;
		table ->attach( * mk_label( "\t\t\t( " + os.str() + "% )"), 1,2, top++, bottom++,Gtk::FILL); os.str("") ;
	}
	
	//flags
	if ( partition.type != GParted::UNALLOCATED )
	{
		table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Flags:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
		table ->attach( * mk_label( partition .flags ), 1,2, top++, bottom++,Gtk::FILL);
	}
	
	//one blank line
	table ->attach( * mk_label( "" ), 1,2, top++, bottom++,Gtk::FILL);
	
	if ( partition.type != GParted::UNALLOCATED && partition .status != GParted::STAT_NEW )
	{
		//path
		table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Path:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
		table ->attach( * mk_label( partition .partition ), 1,2, top++, bottom++,Gtk::FILL);
		
		//realpath (this sucks)
		char real_path[4096] ;
		realpath( partition.partition.c_str() , real_path );
		
		//only show realpath if it's diffent from the short path...
		if ( partition.partition != real_path )
		{
			table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Real Path:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
			table ->attach( * mk_label( real_path ), 1,2, top++, bottom++,Gtk::FILL);
		}
		
		//status
		table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Status:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
		if ( partition.busy )
			Find_Status() ;
		else if ( partition.type == GParted::EXTENDED )
			os << (Glib::ustring) _("Not busy (There are no mounted logical partitions)" ) ;
		else if ( partition.filesystem == "linux-swap" )
			os << (Glib::ustring) _("Not active" ) ;
		else 
			os << (Glib::ustring) _("Not mounted" ) ;
		
		table ->attach( * mk_label( os.str()  ), 1,2, top++, bottom++,Gtk::FILL); os.str( "") ;
	}
	
	//one blank line
	table ->attach( * mk_label( "" ), 1,2, top++, bottom++,Gtk::FILL);
	
	//first sector
	table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "First Sector:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
	os << partition.sector_start ;
	table ->attach( * mk_label( os.str()  ), 1,2, top++, bottom++,Gtk::FILL); os.str( "") ;
	
	//last sector
	table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Last Sector:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
	os << partition.sector_end ;
	table ->attach( * mk_label( os.str()  ), 1,2, top++, bottom++,Gtk::FILL); os.str( "") ;
	
	//total sectors
	table ->attach( * mk_label( "<b>" + (Glib::ustring) _( "Total Sectors:" ) + "</b>" ), 0,1, top, bottom,Gtk::FILL);
	os << partition.sector_end - partition.sector_start ;
	table ->attach( * mk_label( os.str()  ), 1,2, top++, bottom++,Gtk::FILL); os.str( "") ;
}

void Dialog_Partition_Info::Find_Status() 
{
	if ( partition.type == GParted::EXTENDED )
	{
		os <<   _("Busy  (At least one logical partition is mounted)" ) ;
		return ;
	}
	
	
	if ( partition.filesystem == "linux-swap" )
	{
		os << _("Active") ;
		return ;
	}
	
	//try to find the mountpoint in /proc/mounts
	//get realpath
	char real_path[4096] ;
	realpath( partition.partition.c_str() , real_path );
	Glib::ustring mountpoint, partition_real_path = real_path ; //because root partition is listed as /dev/root we need te compare against te real path..
	
	
	std::ifstream file_mounts( "/proc/mounts" ) ;
	std::string line ;
	
	while ( getline( file_mounts, line ) )
	{
		realpath( line.substr( 0, line.find( ' ' ) ) .c_str() , real_path );
		
		if ( partition_real_path == real_path )
		{
			mountpoint = line.substr( line.find( ' ' ) +1, line .length()  ) ;
			
			os << String::ucompose( _("Mounted on %1"), mountpoint .substr( 0, mountpoint .find( ' ' ) ) ) ;
			break ;
		}
		
		
	}
	
	file_mounts .close() ;

	//sometimes rootdevices are not listed as paths. I'll take a guess and just enter / here...( we'll look into this when complaints start coming in :P )
	if ( mountpoint.empty() )
		os << String::ucompose( _("Mounted on %1"), "/") ;
}

Dialog_Partition_Info::~Dialog_Partition_Info()
{
	this ->get_colormap() ->free_colors( color_used, 1 ) ;
	this ->get_colormap() ->free_colors( color_unused, 1 ) ;
	this ->get_colormap() ->free_colors( color_text, 1 ) ;
	this ->get_colormap() ->free_colors( color_partition, 1 ) ;
}


} //GParted
