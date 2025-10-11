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

#include <glibmm/miscutils.h>
#include <gtkmm/alignment.h>
#include <gtkmm/viewport.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <atkmm/relation.h>
#include <gdkmm/general.h>
#include <sigc++/signal.h>


namespace GParted
{


Dialog_Partition_Info::Dialog_Partition_Info( const Partition & partition ) : partition( partition )
{
	// Set minimum dialog height so it fits on an 800x600 screen without too much
	// whitespace (~500 px max for GNOME desktop).  Allow extra space if have any
	// messages or for LVM2 PV or LUKS encryption.
	if (partition.have_messages()      ||
	    partition.fstype == FS_LVM2_PV ||
	    partition.fstype == FS_LUKS      )
		this ->set_size_request( -1, 460) ;
	else
		this ->set_size_request( -1, 370 ) ;	//Minimum 370 to avoid scrolling on Fedora 20

	/*TO TRANSLATORS: dialogtitle, looks like Information about /dev/hda3 */
	this ->set_title( Glib::ustring::compose( _("Information about %1"), partition .get_path() ) );

	init_drawingarea() ;

	// Place info and optional messages in scrollable window
	info_msg_vbox.set_orientation(Gtk::ORIENTATION_VERTICAL);
	info_msg_vbox.set_border_width(5);
	info_scrolled .set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ;
#if HAVE_SET_PROPAGATE_NATURAL_WIDTH
	info_scrolled.set_propagate_natural_width(true);
#endif
	info_scrolled .add( info_msg_vbox ) ;
	//  As Gtk::Box widget info_msg_vbox doesn't have a native scrolling capability a
	//  Gtk::Viewport is automatically created to contain it when it is added to the
	//  Gtk::ScrolledWindow widget info_scrolled.  The Viewport widget is created with
	//  shadow type GTK_SHADOW_IN by default.  Change to GTK_SHADOW_NONE.
	Gtk::Viewport * child_viewport = dynamic_cast<Gtk::Viewport *>(info_scrolled.get_child());
	if (child_viewport)
		child_viewport->set_shadow_type(Gtk::SHADOW_NONE);
	//horizontally center the information scrolled window to match partition graphic
	Gtk::Alignment* center_widget = Gtk::manage(new Gtk::Alignment(0.5, 0.5, 0.0, 1.0));
	center_widget ->add( info_scrolled ) ;
	this->get_content_area()->pack_start(*center_widget);

	//add label for detail and fill with relevant info
	Display_Info() ;
	
	//display messages (if any)
	if ( partition.have_messages() )
	{
		frame = Gtk::manage(new Gtk::Frame());

		{
			Gtk::Image* image = Utils::mk_image(Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON);

			hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
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

			concatenated_messages += "<i>" + Utils::trim_trailing_new_line(messages[i]) + "</i>";
		}
		Gtk::Box* vbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
		vbox ->set_border_width( 5 ) ;
		vbox->pack_start( *Utils::mk_label( concatenated_messages, true, true, true ), Gtk::PACK_SHRINK );
		frame ->add( *vbox ) ;

		info_msg_vbox .pack_start( *frame, Gtk::PACK_EXPAND_WIDGET ) ;
	}

	this->add_button(Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE);
	this ->show_all_children() ;
}


bool Dialog_Partition_Info::drawingarea_on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
	Gdk::Cairo::set_source_rgba(cr, color_partition);
	cr->rectangle(0, 0, 400, 60);
	cr->fill();

	if (partition.fstype != FS_UNALLOCATED)
	{
		// Used
		Gdk::Cairo::set_source_rgba(cr, color_used);
		cr->rectangle(BORDER,
		              BORDER,
		              used,
		              60 - 2 * BORDER);
		cr->fill();

		// Unused
		Gdk::Cairo::set_source_rgba(cr, color_unused);
		cr->rectangle(BORDER + used,
		              BORDER,
		              unused,
		              60 - 2 * BORDER);
		cr->fill();

		// Unallocated
		Gdk::Cairo::set_source_rgba(cr, color_unallocated);
		cr->rectangle(BORDER + used + unused,
		              BORDER,
		              unallocated,
		              60 - 2 * BORDER);
		cr->fill();
	}

	// Text
	Gdk::Cairo::set_source_rgba(cr, color_text);
	cr->move_to(180, BORDER + 6);
	pango_layout->show_in_cairo_context(cr);

	return true;
}

void Dialog_Partition_Info::init_drawingarea() 
{
	drawingarea .set_size_request( 400, 60 ) ;
	drawingarea.signal_draw().connect(sigc::mem_fun(*this, &Dialog_Partition_Info::drawingarea_on_draw));

	frame = Gtk::manage(new Gtk::Frame());
	frame ->add( drawingarea ) ;

	frame ->set_shadow_type( Gtk::SHADOW_ETCHED_OUT ) ;
	frame ->set_border_width( 10 ) ;
	hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	hbox ->pack_start( *frame, Gtk::PACK_EXPAND_PADDING ) ;

	this->get_content_area()->pack_start(*hbox, Gtk::PACK_SHRINK);

	//calculate proportional width of used, unused and unallocated
	if (partition.type == TYPE_EXTENDED)
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
	
	// Colors
	color_used.set("#F8F8BA");
	color_unused.set("white");
	color_unallocated.set("darkgrey");
	color_text.set("black");
	color_partition.set(Utils::get_color(partition.get_filesystem_partition().fstype));

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

	Glib::ustring vgname;
	if (filesystem_ptn.fstype == FS_LVM2_PV)
		vgname = LVM2_PV_Info::get_vg_name( filesystem_ptn.get_path() );

	bool filesystem_accessible = false;
	if (partition.fstype != FS_LUKS || partition.busy)
		// As long as this is not a LUKS encrypted partition which is closed the
		// file system is accessible.
		filesystem_accessible = true;

	Gtk::Grid* grid(Gtk::manage(new Gtk::Grid()));

	grid->set_border_width(5);
	grid->set_column_spacing(10);
	info_msg_vbox.pack_start(*grid, Gtk::PACK_SHRINK);

	// FILE SYSTEM DETAIL SECTION
	// File system headline
	grid->attach(*Utils::mk_label("<b>" + Glib::ustring(_("File System")) + "</b>"), 0, 0, 6, 1);

	// Initialize grid top attach tracker (left side of the grid)
	int top = 1;

	// Left field & value pair area
	// File system
	Gtk::Label *label_filesystem = Utils::mk_label("<b>" + Glib::ustring(_("File system:")) + "</b>");
	grid->attach(*label_filesystem, 1, top, 1, 1);
	if ( filesystem_accessible )
	{
		Gtk::Label *value_filesystem = Utils::mk_label(Utils::get_filesystem_string(filesystem_ptn.fstype),
		                                               true, false, true);
		grid->attach(*value_filesystem, 2, top, 1, 1);
		value_filesystem->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                     label_filesystem->get_accessible());
	}
	top++;

	//label
	if (filesystem_ptn.fstype != FS_UNALLOCATED && filesystem_ptn.type != TYPE_EXTENDED)
	{
		Gtk::Label *label_label = Utils::mk_label("<b>" + Glib::ustring(_("Label:")) + "</b>");
		grid->attach(*label_label, 1, top, 1, 1);
		if ( filesystem_accessible )
		{
			Gtk::Label *value_label = Utils::mk_label(filesystem_ptn.get_filesystem_label(),
			                                          true, false, true);
			grid->attach(*value_label, 2, top, 1, 1);
			value_label->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
			                                                label_label->get_accessible());
		}
		top++;
	}

	// file system uuid
	if (filesystem_ptn.fstype != FS_UNALLOCATED && filesystem_ptn.type != TYPE_EXTENDED)
	{
		Gtk::Label *label_uuid = Utils::mk_label("<b>" + Glib::ustring(_("UUID:")) + "</b>");
		grid->attach(*label_uuid, 1, top, 1, 1);
		if ( filesystem_accessible )
		{
			Gtk::Label *value_uuid = Utils::mk_label(filesystem_ptn.uuid, true, false, true);
			grid->attach(*value_uuid, 2, top, 1, 1);
			value_uuid->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
			                                               label_uuid->get_accessible());
		}
		top++;
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
	if (filesystem_ptn.fstype != FS_UNALLOCATED && filesystem_ptn.status != STAT_NEW)
	{
		//status
		Glib::ustring str_temp ;
		Gtk::Label *label_status = Utils::mk_label("<b>" + Glib::ustring(_("Status:")) + "</b>",
		                                           true, false, false, Gtk::ALIGN_START);
		grid->attach(*label_status, 1, top, 1, 1);
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
			else if (filesystem_ptn.fstype == FS_LINUX_SWAP   ||
			         filesystem_ptn.fstype == FS_LINUX_SWRAID ||
			         filesystem_ptn.fstype == FS_ATARAID      ||
			         filesystem_ptn.fstype == FS_LVM2_PV      ||
			         filesystem_ptn.fstype == FS_BCACHE       ||
			         filesystem_ptn.fstype == FS_JBD            )
			{
				/* TO TRANSLATORS:  Active
				 * means that this linux swap, linux software raid partition, or
				 * LVM physical volume is enabled and being used by the operating system.
				 */
				str_temp = _("Active") ;
			}
			else if (filesystem_ptn.fstype == FS_LUKS)
			{
				// NOTE: LUKS within LUKS
				// Only ever display LUKS information in the file system
				// section when the contents of a LUKS encryption is
				// another LUKS encryption!
				str_temp = luks_open;
			}
			else if ( filesystem_ptn.get_mountpoints().size() )
			{
				str_temp = Glib::ustring::compose(
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
		else if (filesystem_ptn.fstype == FS_LINUX_SWAP   ||
		         filesystem_ptn.fstype == FS_LINUX_SWRAID ||
		         filesystem_ptn.fstype == FS_ATARAID      ||
		         filesystem_ptn.fstype == FS_BCACHE       ||
		         filesystem_ptn.fstype == FS_JBD            )
		{
			/* TO TRANSLATORS:  Not active
			 *  means that this linux swap or linux software raid partition
			 *  is not enabled and is not in use by the operating system.
			 */
			str_temp = _("Not active") ;
		}
		else if (filesystem_ptn.fstype == FS_LUKS)
		{
			// NOTE: LUKS within LUKS
			str_temp = luks_closed;
		}
		else if (filesystem_ptn.fstype == FS_LVM2_PV)
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

		Gtk::Label *value_status = Utils::mk_label(str_temp, true, true, true);
		grid->attach(*value_status, 2, top++, 1, 1);
		value_status->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                 label_status->get_accessible());
	}

	//Optional, LVM2 Volume Group name
	if (filesystem_ptn.fstype == FS_LVM2_PV)
	{
		// Volume Group
		Gtk::Label *label_volume_group = Utils::mk_label("<b>" + Glib::ustring(_("Volume Group:")) + "</b>");
		grid->attach(*label_volume_group, 1, top, 1, 1);
		Gtk::Label *value_volume_group = Utils::mk_label(vgname, true, false, true);
		grid->attach(*value_volume_group, 2, top++, 1, 1);
		value_volume_group->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                       label_volume_group->get_accessible());
	}

	//Optional, members of multi-device file systems
	if (filesystem_ptn.fstype == FS_LVM2_PV ||
	    filesystem_ptn.fstype == FS_BTRFS     )
	{
		// Members
		Gtk::Label *label_members = Utils::mk_label("<b>" + Glib::ustring(_("Members:")) + "</b>",
		                                            true, false, false, Gtk::ALIGN_START);
		grid->attach(*label_members, 1, top, 1, 1);

		std::vector<Glib::ustring> members ;
		switch (filesystem_ptn.fstype)
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

		Gtk::Label *value_members = Utils::mk_label(Glib::build_path("\n", members), true, false, true);
		grid->attach(*value_members, 2, top++, 1, 1);
		value_members->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                  label_members->get_accessible());
	}

	if (filesystem_ptn.fstype == FS_LVM2_PV)
	{
		// Logical Volumes
		Gtk::Label *label_logical_volumes = Utils::mk_label("<b>" + Glib::ustring(_("Logical Volumes:")) + "</b>",
		                                                    true, false, false, Gtk::ALIGN_START);
		grid->attach(*label_logical_volumes, 1, top, 1, 1);

		std::vector<Glib::ustring> lvs = LVM2_PV_Info::get_vg_lvs( vgname );
		Gtk::Label *value_logical_volumes = Utils::mk_label(Glib::build_path("\n", lvs), true, false, true);
		grid->attach(*value_logical_volumes, 2, top++, 1, 1);
		value_logical_volumes->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                          label_logical_volumes->get_accessible());
	}

	// Initialize grid top attach tracker (right side of the grid)
	int topright = 1;

	//Right field & value pair area
	if ( partition .sector_usage_known() )
	{
		//calculate relative diskusage
		int percent_used, percent_unused, percent_unallocated ;
		partition .get_usage_triple( 100, percent_used, percent_unused, percent_unallocated ) ;

		// Used
		Gtk::Label *label_used = Utils::mk_label("<b>" + Glib::ustring(_("Used:")) + "</b>");
		grid->attach(*label_used, 3, topright, 1, 1);
		Gtk::Label *value_used = Utils::mk_label(Utils::format_size(partition.get_sectors_used(), partition.sector_size),
		                                         true, false, true);
		grid->attach(*value_used, 4, topright, 1, 1);
		grid->attach(*Utils::mk_label("( " + Utils::num_to_str(percent_used) + "% )"),
		             5, topright++, 1, 1);
		value_used->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_used->get_accessible());

		// Unused
		Gtk::Label *label_unused = Utils::mk_label("<b>" + Glib::ustring(_("Unused:")) + "</b>");
		grid->attach(*label_unused, 3, topright, 1, 1);
		Gtk::Label *value_unused = Utils::mk_label(Utils::format_size(partition.get_sectors_unused(), partition.sector_size),
		                                           true, false, true);
		grid->attach(*value_unused, 4, topright, 1, 1);
		grid->attach(*Utils::mk_label("( " + Utils::num_to_str(percent_unused) + "% )"),
		             5, topright++, 1, 1);
		value_unused->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                 label_unused->get_accessible());

		//unallocated
		Sector sectors_unallocated = partition .get_sectors_unallocated() ;
		if ( sectors_unallocated > 0 )
		{
			Gtk::Label *label_unallocated = Utils::mk_label("<b>" + Glib::ustring(_("Unallocated:")) + "</b>");
			grid->attach(*label_unallocated, 3, topright, 1, 1);
			Gtk::Label *value_unallocated = Utils::mk_label(Utils::format_size(sectors_unallocated, partition.sector_size),
			                                                true, false, true);
			grid->attach(*value_unallocated, 4, topright, 1, 1);
			grid->attach(*Utils::mk_label("( " + Utils::num_to_str(percent_unallocated) + "% )"),
			             5, topright++, 1, 1);
			value_unallocated->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
			                                                      label_unallocated->get_accessible());
		}
	}

	// Size
	Gtk::Label *label_size = Utils::mk_label("<b>" + Glib::ustring(_("Size:")) + "</b>");
	grid->attach(*label_size, 3, topright, 1, 1);
	Gtk::Label *value_size = Utils::mk_label(Utils::format_size(ptn_sectors, partition.sector_size),
	                                         true, false, true);
	grid->attach(*value_size, 4, topright++, 1, 1);
	value_size->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_size->get_accessible());

	//ensure left row tracker set to largest side (left/right)
	top = std::max( top, topright );

	// One blank line
	grid->attach(*Utils::mk_label(""), 0, top++, 6, 1);

	if (partition.fstype == FS_LUKS)
	{
		// ENCRYPTION DETAIL SECTION
		// Encryption headline
		grid->attach(*Utils::mk_label("<b>" + Glib::ustring(_("Encryption")) + "</b>"),
		             0, top++, 6, 1);

		// Encryption
		Gtk::Label *label_encryption = Utils::mk_label("<b>" + Glib::ustring(_("Encryption:")) + "</b>");
		grid->attach(*label_encryption, 1, top, 1, 1);
		Gtk::Label *value_encryption = Utils::mk_label(Utils::get_filesystem_string(partition.fstype),
		                                               true, false, true);
		grid->attach(*value_encryption, 2, top++, 1, 1);
		value_encryption->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                     label_encryption->get_accessible());

		// LUKS path
		Gtk::Label *label_path = Utils::mk_label("<b>" + Glib::ustring(_("Path:")) + "</b>");
		grid->attach(*label_path, 1, top, 1, 1);
		if ( partition.busy ) {
			Gtk::Label *value_path = Utils::mk_label(partition.get_mountpoint(), true, false, true);
			grid->attach(*value_path, 2, top, 1, 1);
			value_path->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
			                                               label_path->get_accessible());
		}
		top++;

		// LUKS uuid
		Gtk::Label *label_uuid = Utils::mk_label("<b>" + Glib::ustring(_("UUID:")) + "</b>");
		grid->attach(*label_uuid, 1, top, 1, 1);
		Gtk::Label *value_uuid = Utils::mk_label(partition.uuid, true, false, true);
		grid->attach(*value_uuid, 2, top++, 1, 1);
		value_uuid->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                               label_uuid->get_accessible());

		// LUKS status
		Gtk::Label *label_status = Utils::mk_label("<b>" + Glib::ustring(_("Status:")) + "</b>");
		grid->attach(*label_status, 1, top, 1, 1);
		Glib::ustring str_temp;
		if ( partition.busy )
			str_temp = luks_open;
		else
			str_temp = luks_closed;
		Gtk::Label *value_status = Utils::mk_label(str_temp, true, false, true);
		grid->attach(*value_status, 2, top++, 1, 1);
		value_status->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                 label_status->get_accessible());

		// One blank line
		grid->attach(*Utils::mk_label(""), 0, top++, 6, 1);
	}

	// PARTITION DETAIL SECTION
	// Partition headline
	grid->attach(*Utils::mk_label("<b>" + Glib::ustring(_("Partition")) + "</b>"),
	             0, top++, 6, 1);

	//use current left row tracker position as anchor for right
	topright = top ;

	// Left field & value pair area
	// Path
	Gtk::Label *label_path = Utils::mk_label("<b>" + Glib::ustring(_("Path:")) + "</b>");
	grid->attach(*label_path, 1, top, 1, 1);
	Gtk::Label *value_path = Utils::mk_label(partition.get_path(), true, false, true);
	grid->attach(*value_path, 2, top++, 1, 1);
	value_path->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_path->get_accessible());

	if (partition.fstype != FS_UNALLOCATED && partition.status != STAT_NEW)
	{
		// Name
		Gtk::Label *label_name = Utils::mk_label("<b>" + Glib::ustring(_("Name:")) + "</b>");
		grid->attach(*label_name, 1, top, 1, 1);
		Gtk::Label *value_name = Utils::mk_label(partition.name, true, false, true);
		grid->attach(*value_name, 2, top++, 1, 1);
		value_name->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_name->get_accessible());

		// Flags
		Gtk::Label *label_flags = Utils::mk_label("<b>" + Glib::ustring(_("Flags:")) + "</b>");
		grid->attach(*label_flags, 1, top, 1, 1);
		Gtk::Label *value_flags = Utils::mk_label(Glib::build_path(", ", partition.flags), true, false, true);
		grid->attach(*value_flags, 2, top++, 1, 1);
		value_flags->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
		                                                label_flags->get_accessible());
	}

	// Right field & value pair area
	// First sector
	Gtk::Label *label_first_sector = Utils::mk_label("<b>" + Glib::ustring(_("First sector:")) + "</b>");
	grid->attach(*label_first_sector, 3, topright, 1, 1);
	Gtk::Label *value_first_sector = Utils::mk_label(Utils::num_to_str(partition.sector_start), true, false, true);
	grid->attach(*value_first_sector, 4, topright++, 1, 1);
	value_first_sector->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_first_sector->get_accessible());

	// Last sector
	Gtk::Label *label_last_sector = Utils::mk_label("<b>" + Glib::ustring(_("Last sector:")) + "</b>");
	grid->attach(*label_last_sector, 3, topright, 1, 1);
	Gtk::Label *value_last_sector = Utils::mk_label(Utils::num_to_str(partition.sector_end), true, false, true);
	grid->attach(*value_last_sector, 4, topright++, 1, 1);
	value_last_sector->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                      label_last_sector->get_accessible());

	// Total sectors
	Gtk::Label *label_total_sectors = Utils::mk_label("<b>" + Glib::ustring(_("Total sectors:")) + "</b>");
	grid->attach(*label_total_sectors, 3, topright, 1, 1);
	Gtk::Label *value_total_sectors = Utils::mk_label(Utils::num_to_str(ptn_sectors), true, false, true);
	grid->attach(*value_total_sectors, 4, topright++, 1, 1);
	value_total_sectors->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                        label_total_sectors->get_accessible());
}


}  // namespace GParted
