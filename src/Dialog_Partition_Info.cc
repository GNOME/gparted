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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "Dialog_Partition_Info.h"
#include "LVM2_PV_Info.h"
#include "Partition.h"
#include "PartitionLUKS.h"
#include "Utils.h"
#include "btrfs.h"

#include <gtk/gtk.h>
#include <gtkmm/alignment.h>

namespace GParted
{

Dialog_Partition_Info::Dialog_Partition_Info( const Partition & partition ) : partition( partition )
{
	this ->set_has_separator( false ) ;
	// Set minimum dialog height so it fits on an 800x600 screen without too much
	// whitespace (~500 px max for GNOME desktop).  Allow extra space if have any
	// messages or for LVM2 PV or LUKS encryption.
	if ( partition.have_messages()          ||
	     partition.filesystem == FS_LVM2_PV ||
	     partition.filesystem == FS_LUKS       )
		this ->set_size_request( -1, 460) ;
	else
		this ->set_size_request( -1, 370 ) ;	//Minimum 370 to avoid scrolling on Fedora 20

	/*TO TRANSLATORS: dialogtitle, looks like Information about /dev/hda3 */
	this ->set_title( String::ucompose( _("Information about %1"), partition .get_path() ) );

	init_drawingarea() ;

	//place info and optional messages in scrollable window
	info_msg_vbox .set_border_width( 6 ) ;
	info_scrolled .set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ;
	info_scrolled .add( info_msg_vbox ) ;
	//As Gtk::VBox widget info_msg_vbox doesn't have a native scrolling capability a
	//  Gtk::Viewport is automatically created to contain it when it is added to the
	//  Gtk::ScrolledWindow widget info_scrolled.  The Viewport widget is created with
	//  shadow type GTK_SHADOW_IN by default.  Change to GTK_SHADOW_NONE.  Have to use
	//  GTK+ C API as the Viewport widget isn't exposed via gtkmm C++ API.
	//  https://developer.gnome.org/gtkmm/2.24/classGtk_1_1ScrolledWindow.html#a17c588d56d93923841d38f0d5e2ec9d7
	GtkWidget * scrollable_child = gtk_bin_get_child(GTK_BIN(info_scrolled.gobj()));
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(scrollable_child), GTK_SHADOW_NONE);
	//horizontally center the information scrolled window to match partition graphic
	Gtk::Alignment * center_widget = manage( new Gtk::Alignment(0.5, 0.5, 0.0, 1.0) ) ;
	center_widget ->add( info_scrolled ) ;
	this ->get_vbox() ->pack_start( *center_widget ) ;

	//add label for detail and fill with relevant info
	Display_Info() ;
	
	//display messages (if any)
	if ( partition.have_messages() )
	{
		frame = manage( new Gtk::Frame() );

		{
			Gtk::Image* image(manage(new Gtk::Image(Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON)));

			hbox = manage(new Gtk::HBox());
			hbox->pack_start(*image, Gtk::PACK_SHRINK);
		}

		hbox ->pack_start( * Utils::mk_label( "<b> " + Glib::ustring(_("Warning:") ) + " </b>" ),
				   Gtk::PACK_SHRINK ) ;


		frame ->set_label_widget( *hbox ) ;

		// Concatenate all messages for display so that they can be selected
		// together.  Use a blank line between individual messages so that each
		// message can be distinguished. Therefore each message should have been
		// formatted as one or more non-blank lines, with an optional trailing new
		// line.  This is true of GParted internal messages and probably all
		// external messages and errors from libparted and executed commands too.
		std::vector<Glib::ustring> messages = partition.get_messages();
		Glib::ustring concatenated_messages;
		for ( unsigned int i = 0; i < messages.size(); i ++ )
		{
			if ( concatenated_messages.size() > 0 )
				concatenated_messages += "\n\n";

			Glib::ustring::size_type take = messages[i].size();
			if ( take > 0 )
			{
				if ( messages[i][take-1] == '\n' )
					take -- ;  //Skip optional trailing new line
				concatenated_messages += "<i>" + messages[i].substr( 0, take ) + "</i>";
			}
		}
		Gtk::VBox *vbox( manage( new Gtk::VBox() ) ) ;
		vbox ->set_border_width( 5 ) ;
		vbox->pack_start( *Utils::mk_label( concatenated_messages, true, true, true ), Gtk::PACK_SHRINK );
		frame ->add( *vbox ) ;

		info_msg_vbox .pack_start( *frame, Gtk::PACK_EXPAND_WIDGET ) ;
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
	if ( partition.filesystem != FS_UNALLOCATED )
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

	color_partition.set( Utils::get_color( partition.get_filesystem_partition().filesystem ) );
	this ->get_colormap() ->alloc_color( color_partition ) ;	 
	
	//set text of pangolayout
	pango_layout = drawingarea .create_pango_layout( 
		partition .get_path() + "\n" + Utils::format_size( partition .get_sector_length(), partition .sector_size ) ) ;
}

void Dialog_Partition_Info::Display_Info()
{
	//The information in this area is in table format.
	//
	//For example:
	//<-------------------------- Column Numbers ----------------------------->
	//0 1            2             3             4            5               6
	//+-+------------+-------------+-------------+------------+---------------+
	//Section
	//  Field Left:  Value Left    Field Right:  Value Right  Optional Right
	//+-+------------+-------------+-------------+------------+---------------+
	//0 1            2             3             4            5               6

	// Refers to the Partition object containing the file system.  Either the same as
	// partition, or for an open LUKS mapping, the encrypted Partition object member
	// within containing the encrypted file system.
	const Partition & filesystem_ptn = partition.get_filesystem_partition();

	Sector ptn_sectors = partition .get_sector_length() ;

	Glib::ustring vgname = "" ;  //Also used in partition status message
	if ( filesystem_ptn.filesystem == FS_LVM2_PV )
		vgname = LVM2_PV_Info::get_vg_name( filesystem_ptn.get_path() );

	bool filesystem_accessible = false;
	if ( partition.filesystem != FS_LUKS || partition.busy )
		// As long as this is not a LUKS encrypted partition which is closed the
		// file system is accessible.
		filesystem_accessible = true;

	//initialize table top and bottom row number attach trackers
	int top = 0     , bottom = 1 ;      //Left field & value pairs
	int topright = 0, bottomright = 1 ; //Right field & value pairs

	Gtk::Table* table(manage(new Gtk::Table()));

	table ->set_border_width( 5 ) ;
	table ->set_col_spacings(10 ) ;
	info_msg_vbox .pack_start( *table, Gtk::PACK_SHRINK ) ;

	//FILE SYSTEM DETAIL SECTION
	//file system headline
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("File System") ) + "</b>" ),
			0, 6,
			top++, bottom++,
			Gtk::FILL ) ;

	//use current left row tracker position as anchor for right
	topright = top ;
	bottomright = bottom ;

	//Left field & value pair area
	//file system
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("File system:") ) + "</b>" ),
			1, 2,
			top, bottom,
			Gtk::FILL ) ;
	if ( filesystem_accessible )
	{
		table->attach( *Utils::mk_label( Utils::get_filesystem_string( filesystem_ptn.filesystem ), true, false, true ),
		               2, 3, top, bottom, Gtk::FILL );
	}
	top++, bottom++;

	//label
	if ( filesystem_ptn.filesystem != FS_UNALLOCATED && filesystem_ptn.type != TYPE_EXTENDED )
	{
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Label:") ) + "</b>"),
				1, 2,
				top, bottom,
				Gtk::FILL) ;
		if ( filesystem_accessible )
		{
			table->attach( *Utils::mk_label( filesystem_ptn.get_filesystem_label(), true, false, true ),
			               2, 3, top, bottom, Gtk::FILL);
		}
		top++, bottom++;
	}

	// file system uuid
	if ( filesystem_ptn.filesystem != FS_UNALLOCATED && filesystem_ptn.type != TYPE_EXTENDED )
	{
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("UUID:") ) + "</b>"),
				1, 2,
				top, bottom,
				Gtk::FILL) ;
		if ( filesystem_accessible )
		{
			table->attach( *Utils::mk_label( filesystem_ptn.uuid, true, false, true ),
			               2, 3, top, bottom, Gtk::FILL);
		}
		top++, bottom++;
	}

	/* TO TRANSLATORS:   Open
	 * means that the LUKS encryption is open and the encrypted data within is accessible.
	 */
	static Glib::ustring luks_open   = _("Open");
	/* TO TRANSLATORS:   Closed
	 * means that the LUKS encryption is closed and the encrypted data within is not accessible.
	 */
	static Glib::ustring luks_closed = _("Closed");

	//status
	if ( filesystem_ptn.filesystem != FS_UNALLOCATED && filesystem_ptn.status != STAT_NEW )
	{
		//status
		Glib::ustring str_temp ;
		table->attach( *Utils::mk_label( "<b>" + Glib::ustring( _("Status:") ) + "</b>",
		                                 true, false, false, 0.0 /* ALIGN_TOP */ ),
		               1, 2, top, bottom, Gtk::FILL );
		if ( ! filesystem_accessible )
		{
			/* TO TRANSLATORS:   Not accessible (Encrypted)
			 * means that the data in encrypted and hasn't been made
			 * accessible by opening it with the passphrase.
			 */
			str_temp = _("Not accessible (Encrypted)");
		}
		else if ( filesystem_ptn.busy )
		{
			if ( filesystem_ptn.type == TYPE_EXTENDED )
			{
				/* TO TRANSLATORS:  Busy (At least one logical partition is mounted)
				 * means that this extended partition contains at least one logical
				 * partition that is mounted or otherwise active.
				 */
				str_temp = _("Busy (At least one logical partition is mounted)") ;
			}
			else if ( filesystem_ptn.filesystem == FS_LINUX_SWAP   ||
			          filesystem_ptn.filesystem == FS_LINUX_SWRAID ||
			          filesystem_ptn.filesystem == FS_LVM2_PV         )
			{
				/* TO TRANSLATORS:  Active
				 * means that this linux swap, linux software raid partition, or
				 * LVM physical volume is enabled and being used by the operating system.
				 */
				str_temp = _("Active") ;
			}
			else if ( filesystem_ptn.filesystem == FS_LUKS )
			{
				// NOTE: LUKS within LUKS
				// Only ever display LUKS information in the file system
				// section when the contents of a LUKS encryption is
				// another LUKS encryption!
				str_temp = luks_open;
			}
			else if ( filesystem_ptn.get_mountpoints().size() )
			{
				str_temp = String::ucompose(
						/* TO TRANSLATORS: looks like   Mounted on /mnt/mymountpoint */
						_("Mounted on %1"),
						Glib::build_path( ", ", filesystem_ptn.get_mountpoints() ) );
			}
		}
		else if ( filesystem_ptn.type == TYPE_EXTENDED )
		{
			/* TO TRANSLATORS:  Not busy (There are no mounted logical partitions)
			 * means that this extended partition contains no mounted or otherwise
			 * active partitions.
			 */
			str_temp = _("Not busy (There are no mounted logical partitions)") ;
		}
		else if ( filesystem_ptn.filesystem == FS_LINUX_SWAP   ||
		          filesystem_ptn.filesystem == FS_LINUX_SWRAID    )
		{
			/* TO TRANSLATORS:  Not active
			 *  means that this linux swap or linux software raid partition
			 *  is not enabled and is not in use by the operating system.
			 */
			str_temp = _("Not active") ;
		}
		else if ( filesystem_ptn.filesystem == FS_LUKS )
		{
			// NOTE: LUKS within LUKS
			str_temp = luks_closed;
		}
		else if ( filesystem_ptn.filesystem == FS_LVM2_PV )
		{
			if ( vgname .empty() )
				/* TO TRANSLATORS:  Not active (Not a member of any volume group)
				 * means that the partition is not yet a member of an LVM volume
				 * group and therefore is not active and can not yet be used by
				 * the operating system.
				 */
				str_temp = _("Not active (Not a member of any volume group)") ;
			else if ( LVM2_PV_Info::is_vg_exported( vgname ) )
				/* TO TRANSLATORS:  Not active and exported
				 * means that the partition is a member of an LVM volume group but
				 * the volume group is not active and not being used by the operating system.
				 * The volume group has also been exported making the LVM physical volumes
				 * ready for moving to a different computer system.
				 */
				str_temp = _("Not active and exported") ;
			else
				/* TO TRANSLATORS:  Not active
				 * means that the partition is a member of an LVM volume group but
				 * the volume group is not active and not being used by the operating system.
				 */
				str_temp = _("Not active") ;
		}
		else
		{
			/* TO TRANSLATORS:  Not mounted
			 * means that this partition is not mounted.
			 */
			str_temp = _("Not mounted") ;
		}

		table->attach( *Utils::mk_label( str_temp, true, true, true ), 2, 3, top++, bottom++, Gtk::FILL );
	}

	//Optional, LVM2 Volume Group name
	if ( filesystem_ptn.filesystem == FS_LVM2_PV )
	{
		//Volume Group
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Volume Group:") ) + "</b>"),
		                1, 2, top, bottom, Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( vgname, true, false, true ),
		                2, 3, top++, bottom++, Gtk::FILL ) ;
	}

	//Optional, members of multi-device file systems
	if ( filesystem_ptn.filesystem == FS_LVM2_PV ||
	     filesystem_ptn.filesystem == FS_BTRFS      )
	{
		//Members
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Members:") ) + "</b>", true, false, false, 0.0 /* ALIGN_TOP */ ),
		                1, 2, top, bottom, Gtk::FILL ) ;

		std::vector<Glib::ustring> members ;
		switch ( filesystem_ptn.filesystem )
		{
			case FS_BTRFS:
				members = btrfs::get_members( filesystem_ptn.get_path() );
				break ;
			case FS_LVM2_PV:
				if ( ! vgname .empty() )
					members = LVM2_PV_Info::get_vg_members( vgname );
				break ;
			default:
				break ;
		}

		table->attach( *Utils::mk_label( Glib::build_path( "\n", members ), true, false, true ),
		               2, 3, top++, bottom++, Gtk::FILL );
	}

	if ( filesystem_ptn.filesystem == FS_LVM2_PV )
	{
		//Logical Volumes
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Logical Volumes:") ) + "</b>", true, false, false, 0.0 /* ALIGN_TOP */ ),
		                1, 2, top, bottom, Gtk::FILL );

		std::vector<Glib::ustring> lvs = LVM2_PV_Info::get_vg_lvs( vgname );
		table->attach( *Utils::mk_label( Glib::build_path( "\n", lvs ), true, false, true ),
		               2, 3, top++, bottom++, Gtk::FILL );
	}

	//Right field & value pair area
	if ( partition .sector_usage_known() )
	{
		//calculate relative diskusage
		int percent_used, percent_unused, percent_unallocated ;
		partition .get_usage_triple( 100, percent_used, percent_unused, percent_unallocated ) ;

		//Used
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Used:") ) + "</b>" ),
				3, 4,
				topright, bottomright,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( Utils::format_size( partition .get_sectors_used(), partition .sector_size ), true, false, true ),
				4, 5,
				topright, bottomright,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( "( " + Utils::num_to_str( percent_used ) + "% )"),
				5, 6,
				topright++, bottomright++,
				Gtk::FILL ) ;

		//unused
		table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Unused:") ) + "</b>" ),
				3, 4,
				topright, bottomright,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( Utils::format_size( partition .get_sectors_unused(), partition .sector_size ), true, false, true ),
				4, 5,
				topright, bottomright,
				Gtk::FILL ) ;
		table ->attach( * Utils::mk_label( "( " + Utils::num_to_str( percent_unused ) + "% )"),
				5, 6,
				topright++, bottomright++,
				Gtk::FILL ) ;

		//unallocated
		Sector sectors_unallocated = partition .get_sectors_unallocated() ;
		if ( sectors_unallocated > 0 )
		{
			table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Unallocated:") ) + "</b>" ),
					3, 4,
					topright, bottomright,
					Gtk::FILL ) ;
			table ->attach( * Utils::mk_label( Utils::format_size( sectors_unallocated, partition .sector_size ), true, false, true ),
					4, 5,
					topright, bottomright,
					Gtk::FILL ) ;
			table ->attach( * Utils::mk_label( "( " + Utils::num_to_str( percent_unallocated ) + "% )"),
					5, 6,
					topright++, bottomright++,
					Gtk::FILL ) ;
		}
	}

	//size
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Size:") ) + "</b>" ),
			3, 4,
			topright, bottomright,
			Gtk::FILL) ;
	table ->attach( * Utils::mk_label( Utils::format_size( ptn_sectors, partition .sector_size ), true, false, true ),
			4, 5,
			topright++, bottomright++,
			Gtk::FILL ) ;

	//ensure left row tracker set to largest side (left/right)
	top = std::max( top, topright );
	bottom = std::max( bottom, bottomright );

	//one blank line
	table ->attach( * Utils::mk_label( "" ), 0, 6, top++, bottom++, Gtk::FILL ) ;

	if ( partition.filesystem == FS_LUKS )
	{
		// ENCRYPTION DETAIL SECTION
		// encryption headline
		table->attach( *Utils::mk_label( "<b>" + Glib::ustring( _("Encryption") ) + "</b>" ),
		               0, 6, top++, bottom++, Gtk::FILL );

		// Encryption
		table->attach( *Utils::mk_label( "<b>" + Glib::ustring( _("Encryption:") ) + "</b>"),
		               1, 2, top, bottom, Gtk::FILL);
		table->attach( *Utils::mk_label( Utils::get_filesystem_string( partition.filesystem ), true, false, true ),
		               2, 3, top++, bottom++, Gtk::FILL);

		// LUKS path
		table->attach( *Utils::mk_label( "<b>" + Glib::ustring( _("Path:") ) + "</b>"),
		               1, 2, top, bottom, Gtk::FILL);
		if ( partition.busy )
			table->attach( *Utils::mk_label( partition.get_mountpoint(), true, false, true ),
			               2, 3, top, bottom, Gtk::FILL);
		top++, bottom++;

		// LUKS uuid
		table->attach( *Utils::mk_label( "<b>" + Glib::ustring( _("UUID:") ) + "</b>"),
		               1, 2, top, bottom, Gtk::FILL);
		table->attach( *Utils::mk_label( partition.uuid, true, false, true ),
		               2, 3, top++, bottom++, Gtk::FILL);

		// LUKS status
		table->attach( *Utils::mk_label( "<b>" + Glib::ustring( _("Status:") ) + "</b>" ),
		               1, 2, top, bottom, Gtk::FILL );
		Glib::ustring str_temp;
		if ( partition.busy )
			str_temp = luks_open;
		else
			str_temp = luks_closed;
		table->attach( *Utils::mk_label( str_temp, true, false, true ), 2, 3, top++, bottom++, Gtk::FILL );

		// one blank line
		table->attach( *Utils::mk_label( "" ), 0, 6, top++, bottom++, Gtk::FILL );
	}

	//PARTITION DETAIL SECTION
	//partition headline
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Partition") ) + "</b>" ),
			0, 6,
			top++, bottom++,
			Gtk::FILL ) ;

	//use current left row tracker position as anchor for right
	topright = top ;
	bottomright = bottom ;

	//Left field & value pair area
	//path
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Path:") ) + "</b>" ),
			1, 2,
			top, bottom,
			Gtk::FILL ) ;
	table->attach( * Utils::mk_label( partition.get_path(), true, false, true ),
	               2, 3,
	               top++, bottom++,
	               Gtk::FILL );

	if ( partition.filesystem != FS_UNALLOCATED && partition .status != GParted::STAT_NEW )
	{
		// name
		table->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Name:") ) + "</b>"),
		               1, 2,
		               top, bottom,
		               Gtk::FILL );
		table->attach( * Utils::mk_label( partition.name, true, false, true ),
		               2, 3,
		               top++, bottom++,
		               Gtk::FILL );

		// flags
		table->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Flags:") ) + "</b>" ),
		               1, 2,
		               top, bottom,
		               Gtk::FILL );
		table->attach( * Utils::mk_label( Glib::build_path( ", ", partition .flags ), true, false, true ),
		               2, 3,
		               top++, bottom++,
		               Gtk::FILL );
	}

	//Right field & value pair area
	//first sector
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("First sector:") ) + "</b>" ),
			3, 4,
			topright, bottomright,
			Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( Utils::num_to_str( partition .sector_start ), true, false, true ),
			4, 5,
			topright++, bottomright++,
			Gtk::FILL ) ;
	
	//last sector
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Last sector:") ) + "</b>" ),
			3, 4,
			topright, bottomright,
			Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( Utils::num_to_str( partition.sector_end ), true, false, true ),
			4, 5,
			topright++, bottomright++,
			Gtk::FILL ) ; 
	
	//total sectors
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( _("Total sectors:") ) + "</b>" ),
			3, 4,
			topright, bottomright,
			Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( Utils::num_to_str( ptn_sectors ), true, false, true ),
			4, 5,
			topright++, bottomright++,
			Gtk::FILL ) ;
}

Dialog_Partition_Info::~Dialog_Partition_Info()
{
	this ->get_colormap() ->free_color( color_used ) ;
	this ->get_colormap() ->free_color( color_unused ) ;
	this ->get_colormap() ->free_color( color_text ) ;
	this ->get_colormap() ->free_color( color_partition ) ;
}

} //GParted
