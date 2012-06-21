/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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
#include "../include/LVM2_PV_Info.h"

namespace GParted
{

Dialog_Partition_Info::Dialog_Partition_Info( const Partition & partition )
{
	this ->partition = partition ;

	this ->set_resizable( false ) ;
	this ->set_has_separator( false ) ;
	
	/*TO TRANSLATORS: dialogtitle, looks like Information about /dev/hda3 */
	this ->set_title( String::ucompose( _("Information about %1"), partition .get_path() ) );
	 
	init_drawingarea() ;
	
	//add label for detail and fill with relevant info
	Display_Info() ;
	
	//display messages (if any)
	if ( partition .messages .size() > 0 )
	{
		frame = manage( new Gtk::Frame() );
		frame ->set_border_width( 10 );
		
		{
			Gtk::Image* image(manage(new Gtk::Image(Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON)));

			hbox = manage(new Gtk::HBox());
			hbox->pack_start(*image, Gtk::PACK_SHRINK);
		}

		hbox ->pack_start( * Utils::mk_label( "<b> " + Glib::ustring(_("Warning:") ) + " </b>" ),
				   Gtk::PACK_SHRINK ) ;


		frame ->set_label_widget( *hbox ) ;

		//FIXME: for more clarity we should add a listview here with alternating rowcolors..
		//that way it's easier to tell messages apart..
		{
			Gtk::VBox* vbox(manage(new Gtk::VBox(false,4)));
			vbox->set_border_width(5);

			for (unsigned int t = 0; t < partition.messages.size(); ++t)
				vbox->pack_start(*Utils::mk_label("<i>" + partition .messages[t] + "</i>",
								true,
								Gtk::ALIGN_LEFT,
								Gtk::ALIGN_CENTER,
								true),
						Gtk::PACK_SHRINK);
			frame->add(*vbox);
		}

		this ->get_vbox() ->pack_start( *frame, Gtk::PACK_SHRINK ) ;
	}
		
	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_OK ) ;
	this ->show_all_children() ;
}

void Dialog_Partition_Info::drawingarea_on_realize()
{
	gc = Gdk::GC::create( drawingarea .get_window() ) ;
	
	drawingarea .get_window() ->set_background( color_partition ) ;
}

bool Dialog_Partition_Info::drawingarea_on_expose( GdkEventExpose *ev )
{
	if ( partition .type != GParted::TYPE_UNALLOCATED ) 
	{
		//used
		gc ->set_foreground( color_used );
		drawingarea .get_window() ->draw_rectangle( gc, true, BORDER, BORDER, used, 44 ) ;
		
		//unused
		gc ->set_foreground( color_unused );
		drawingarea .get_window() ->draw_rectangle( gc, true, BORDER + used, BORDER, unused, 44 ) ;

		//unallocated
		gc ->set_foreground( color_unallocated );
		drawingarea .get_window() ->draw_rectangle( gc, true, BORDER + used + unused, BORDER, unallocated, 44 ) ;
	}
	
	//text
	gc ->set_foreground( color_text );
	drawingarea .get_window() ->draw_layout( gc, 180, BORDER + 6, pango_layout ) ;
	
	return true;
}

void Dialog_Partition_Info::init_drawingarea() 
{
	drawingarea .set_size_request( 400, 60 ) ;
	drawingarea .signal_realize().connect( sigc::mem_fun(*this, &Dialog_Partition_Info::drawingarea_on_realize) ) ;
	drawingarea .signal_expose_event().connect( sigc::mem_fun(*this, &Dialog_Partition_Info::drawingarea_on_expose) ) ;
	
	frame = manage( new Gtk::Frame() ) ;
	frame ->add( drawingarea ) ;
	
	frame ->set_shadow_type( Gtk::SHADOW_ETCHED_OUT ) ;
	frame ->set_border_width( 10 ) ;
	hbox = manage( new Gtk::HBox() ) ;
	hbox ->pack_start( *frame, Gtk::PACK_EXPAND_PADDING ) ;
	
	this ->get_vbox() ->pack_start( *hbox, Gtk::PACK_SHRINK ) ;
	
	//calculate proportional width of used, unused and unallocated
	if ( partition .type == GParted::TYPE_EXTENDED )
	{
		//Specifically show extended partitions as unallocated
		used        = 0 ;
		unused      = 0 ;
		unallocated = 400 - BORDER *2 ;
	}
	else if ( partition .sector_usage_known() )
	{
		partition .get_usage_triple( 400 - BORDER *2, used, unused, unallocated ) ;
	}
	else
	{
		//Specifically show unknown figures as unused
		used        = 0 ;
		unused      = 400 - BORDER *2 ;
		unallocated = 0 ;
	}
	
	//allocate some colors
	color_used.set( "#F8F8BA" );
	this ->get_colormap() ->alloc_color( color_used ) ;
	
	color_unused .set( "white" ) ;
	this ->get_colormap() ->alloc_color( color_unused ) ;

	color_unallocated .set( "darkgrey" ) ;
	this ->get_colormap() ->alloc_color( color_unallocated ) ;

	color_text .set( "black" );
	this ->get_colormap() ->alloc_color( color_text ) ;
	
	color_partition = partition .color ;
	this ->get_colormap() ->alloc_color( color_partition ) ;	 
	
	//set text of pangolayout
	pango_layout = drawingarea .create_pango_layout( 
		partition .get_path() + "\n" + Utils::format_size( partition .get_sector_length(), partition .sector_size ) ) ;
}

void Dialog_Partition_Info::Display_Info()
{  
	int top = 0, bottom = 1 ;
	Sector ptn_sectors = partition .get_sector_length() ;
	
	Gtk::Table* table(manage(new Gtk::Table()));

	table ->set_border_width( 5 ) ;
	table ->set_col_spacings(10 ) ;
	this ->get_vbox() ->pack_start( *table, Gtk::PACK_SHRINK ) ;
	
	//filesystem
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("File system:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( Utils::get_filesystem_string( partition .filesystem ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
			1, 2,
			top++, bottom++,
			Gtk::FILL ) ;

	//size
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Size:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL) ;
	table ->attach( * Utils::mk_label( Utils::format_size( ptn_sectors, partition .sector_size ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
			1, 2,
			top++, bottom++,
			Gtk::FILL ) ;
	
	if ( partition .sector_usage_known() )
	{
		Sector used        = partition .get_sectors_used() ;
		Sector unused      = partition .get_sectors_unused() ;
		Sector unallocated = partition .get_sectors_unallocated() ;
		//calculate relative diskusage
		int percent_used, percent_unused, percent_unallocated ;
		partition .get_usage_triple( 100, percent_used, percent_unused, percent_unallocated ) ;

		//Used
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Used:") ) + "</b>" ),
				0, 1,
				top, bottom,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( Utils::format_size( used, partition .sector_size ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
				1, 2,
				top, bottom,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( "\t\t\t( " + Utils::num_to_str( percent_used ) + "% )"),
				1, 2,
				top++, bottom++,
				Gtk::FILL ) ;

		//unused
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Unused:") ) + "</b>" ),
				0, 1,
				top, bottom,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( Utils::format_size( unused, partition .sector_size ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
				1, 2,
				top, bottom,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( "\t\t\t( " + Utils::num_to_str( percent_unused ) + "% )"),
				1, 2,
				top++, bottom++,
				Gtk::FILL ) ;

		//unallocated
		if ( unallocated > 0 )
		{
			table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Unallocated:") ) + "</b>" ),
					0, 1,
					top, bottom,
					Gtk::FILL ) ;
			table ->attach( * Utils::mk_label( Utils::format_size( unallocated, partition .sector_size ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
					1, 2,
					top, bottom,
					Gtk::FILL ) ;
			table ->attach( * Utils::mk_label( "\t\t\t( " + Utils::num_to_str( percent_unallocated ) + "% )"),
					1, 2,
					top++, bottom++,
					Gtk::FILL ) ;
		}
	}
	
	//flags
	if ( partition.type != GParted::TYPE_UNALLOCATED )
	{
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Flags:") ) + "</b>" ),
				0, 1,
				top, bottom,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( Glib::build_path( ", ", partition .flags ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
				1, 2, 
				top++, bottom++,
				Gtk::FILL ) ;
	}
	
	//one blank line
	table ->attach( * Utils::mk_label( "" ), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	if ( partition .type != GParted::TYPE_UNALLOCATED && partition .status != GParted::STAT_NEW )
	{
		//path
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Path:") ) + "</b>",
						   true,
						   Gtk::ALIGN_LEFT,
						   Gtk::ALIGN_TOP ),
				0, 1,
				top, bottom,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( Glib::build_path( "\n", partition .get_paths() ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
				1, 2,
				top++, bottom++,
				Gtk::FILL ) ;
		
		//status
		Glib::ustring str_temp ;
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Status:") ) + "</b>" ),
				0, 1,
				top, bottom,
				Gtk::FILL ) ;
		if ( partition .busy )
		{
			if ( partition .type == GParted::TYPE_EXTENDED )
			{
				/* TO TRANSLATORS:  Busy (At least one logical partition is mounted)
				 * means that this extended partition contains at least one logical
				 * partition that is mounted or otherwise active.
				 */
				str_temp =  _("Busy (At least one logical partition is mounted)") ;
			}
			else if ( partition .filesystem == FS_LINUX_SWAP )
			{
				/* TO TRANSLATORS:  Active
				 * means that this linux swap partition is enabled and being used by
				 * the operating system.
				 */
				str_temp = _("Active") ;
			}
			else if ( partition .filesystem == FS_LVM2_PV )
			{
				/* TO TRANSLATORS:  myvgname active
				 * means that the partition is a member of an LVM volume group and the
				 * volume group is active and being used by the operating system.
				 */
				str_temp = String::ucompose( _("%1 active"), partition .get_mountpoint() ) ;
			}
			else if ( partition .get_mountpoints() .size() )
			{
				str_temp = String::ucompose( 
						/* TO TRANSLATORS: looks like   Mounted on /mnt/mymountpoint */
						_("Mounted on %1"),
						Glib::build_path( ", ", partition .get_mountpoints() ) ) ;
			}
		}
		else if ( partition.type == GParted::TYPE_EXTENDED )
		{
			/* TO TRANSLATORS:  Not busy (There are no mounted logical partitions)
			 * means that this extended partition contains no mounted or otherwise
			 * active partitions.
			 */
			str_temp = _("Not busy (There are no mounted logical partitions)") ;
		}
		else if ( partition.filesystem == GParted::FS_LINUX_SWAP )
		{
			/* TO TRANSLATORS:  Not active
			 *  means that this linux swap partition is not enabled and is not
			 *  in use by the operating system.
			 */
			str_temp = _("Not active") ;
		}
		else if ( partition .filesystem == FS_LVM2_PV )
		{
			LVM2_PV_Info lvm2_pv_info ;
			Glib::ustring mount_point = partition .get_mountpoint() ;
			if ( mount_point .empty() )
				/* TO TRANSLATORS:  Not active (Not a member of any volume group)
				 * means that the partition is not yet a member of an LVM volume
				 * group and therefore is not active and can not yet be used by
				 * the operating system.
				 */
				str_temp = _("Not active (Not a member of any volume group)") ;
			else if ( lvm2_pv_info .is_vg_exported( mount_point ) )
				/* TO TRANSLATORS:  myvgname not active and exported
				 * means that the partition is a member of an LVM volume group but
				 * the volume group is not active and not being used by the operating system.
				 * The volume group has also been exported making the LVM physical volumes
				 * ready for moving to a different computer system.
				 */
				str_temp = String::ucompose( _("%1 not active and exported"), mount_point ) ;
			else
				/* TO TRANSLATORS:  myvgname not active
				 * means that the partition is a member of an LVM volume group but
				 * the volume group is not active and not being used by the operating system.
				 */
				str_temp = String::ucompose( _("%1 not active"), mount_point ) ;
		}
		else
		{
			/* TO TRANSLATORS:  Not mounted
			 * means that this partition is not mounted.
			 */
			str_temp = _("Not mounted") ;
		}

		table ->attach( * Utils::mk_label( str_temp, true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ), 1, 2, top++, bottom++, Gtk::FILL ) ;
	}

	//label
	if ( partition.type != GParted::TYPE_UNALLOCATED && partition.type != GParted::TYPE_EXTENDED )
	{
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Label:") ) + "</b>"),
				0, 1,
				top, bottom,
				Gtk::FILL) ;
		table ->attach( * Utils::mk_label( partition .label, true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
				1, 2,
				top++, bottom++,
				Gtk::FILL) ;
	}

	//uuid
	if ( partition.type != GParted::TYPE_UNALLOCATED && partition.type != GParted::TYPE_EXTENDED )
	{
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("UUID:") ) + "</b>"),
				0, 1,
				top, bottom,
				Gtk::FILL) ;
		table ->attach( * Utils::mk_label( partition .uuid, true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
				1, 2,
				top++, bottom++,
				Gtk::FILL) ;
	}

	//one blank line
	table ->attach( * Utils::mk_label( "" ), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	//first sector
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("First sector:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( Utils::num_to_str( partition .sector_start ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
			1, 2,
			top++, bottom++,
			Gtk::FILL ) ;
	
	//last sector
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Last sector:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( Utils::num_to_str( partition.sector_end ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
			1, 2,
			top++, bottom++,
			Gtk::FILL ) ; 
	
	//total sectors
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Total sectors:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( Utils::num_to_str( ptn_sectors ), true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
			1, 2,
			top++, bottom++,
			Gtk::FILL ) ;
}

Dialog_Partition_Info::~Dialog_Partition_Info()
{
	this ->get_colormap() ->free_colors( color_used, 1 ) ;
	this ->get_colormap() ->free_colors( color_unused, 1 ) ;
	this ->get_colormap() ->free_colors( color_text, 1 ) ;
	this ->get_colormap() ->free_colors( color_partition, 1 ) ;
}

} //GParted
