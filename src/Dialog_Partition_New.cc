/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
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


#include "Dialog_Partition_New.h"

#include "Device.h"
#include "FileSystem.h"
#include "GParted_Core.h"
#include "Partition.h"
#include "Utils.h"

#include <gdkmm/rgba.h>
#include <glibmm/ustring.h>
#include <gtkmm/label.h>
#include <gtkmm/widget.h>
#include <atkmm/relation.h>
#include <sigc++/bind.h>
#include <sigc++/signal.h>
#include <glib.h>
#include <vector>


namespace GParted
{


Dialog_Partition_New::Dialog_Partition_New( const Device & device,
                                            const Partition & selected_partition,
                                            bool any_extended,
                                            unsigned short new_count,
                                            const std::vector<FS> & FILESYSTEMS )
 : Dialog_Base_Partition(device), default_fs(-1)
{
	/*TO TRANSLATORS: dialogtitle */
	this ->set_title( _("Create new Partition") ) ;
	Set_Resizer( false ) ;
	Set_Confirm_Button( NEW ) ;

	//set used (in pixels)...
	m_frame_resizer_base->set_used(0);

	set_data(device, selected_partition, any_extended, new_count, FILESYSTEMS );
}

Dialog_Partition_New::~Dialog_Partition_New()
{
	// Work around a Gtk issue fixed in 3.24.0.
	// https://gitlab.gnome.org/GNOME/gtk/issues/125
	hide();
}

void Dialog_Partition_New::set_data( const Device & device,
                                     const Partition & selected_partition,
                                     bool any_extended,
                                     unsigned short new_count,
                                     const std::vector<FS> & FILESYSTEMS )
{
	this ->new_count = new_count;
	m_new_partition.reset(selected_partition.clone());

	// Copy only supported file systems, excluding LUKS, from GParted_Core FILESYSTEMS
	// vector.  Add FS_CLEARED, FS_UNFORMATTED and FS_EXTENDED at the end.  This
	// decides the order of items in the file system menu built by
	// build_filesystems_combo().
	this->FILESYSTEMS.clear();
	for ( unsigned i = 0 ; i < FILESYSTEMS.size() ; i ++ )
	{
		if (GParted_Core::supported_filesystem(FILESYSTEMS[i].fstype) &&
		    FILESYSTEMS[i].fstype != FS_LUKS                            )
			this->FILESYSTEMS.push_back( FILESYSTEMS[i] );
	}

	FS fs_tmp ;
	//... add FS_CLEARED
	fs_tmp.fstype = FS_CLEARED;
	fs_tmp .create = FS::GPARTED ;
	this ->FILESYSTEMS .push_back( fs_tmp ) ;

	//... add FS_UNFORMATTED
	fs_tmp.fstype = FS_UNFORMATTED;
	fs_tmp .create = FS::GPARTED ;
	this ->FILESYSTEMS .push_back( fs_tmp ) ;

	// ... finally add FS_EXTENDED.  Needed so that when creating an extended
	// partition it is identified correctly before the operation is applied.
	fs_tmp = FS();
	fs_tmp.fstype = FS_EXTENDED;
	fs_tmp.create = FS::NONE;
	this ->FILESYSTEMS .push_back( fs_tmp ) ;

	// Add table with selection menu's...
	grid_create.set_border_width(10);
	grid_create.set_row_spacing(5);
	hbox_main.pack_start(grid_create, Gtk::PACK_SHRINK);

	/* TO TRANSLATORS: used as label for a list of choices.  Create as: <combo box with choices> */
	Gtk::Label *label_type = Utils::mk_label(Glib::ustring(_("Create as:")) + "\t");
	grid_create.attach(*label_type, 0, 0, 1, 1);
	combo_type.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_type->get_accessible());

	// Fill partition type combo.
	combo_type.items().push_back(_("Primary Partition"));
	combo_type.items().push_back(_("Logical Partition"));
	combo_type.items().push_back(_("Extended Partition"));

	//determine which PartitionType is allowed
	if ( device.disktype != "msdos" && device.disktype != "dvh" )
	{
		combo_type.items()[1].set_sensitive(false);
		combo_type.items()[2].set_sensitive(false);
		combo_type.set_active(0);
	}
	else if ( selected_partition.inside_extended )
	{
		combo_type.items()[0].set_sensitive(false);
		combo_type.items()[2].set_sensitive(false);
		combo_type.set_active(1);
	}
	else
	{
		combo_type.items()[1].set_sensitive(false);
		if ( any_extended )
			combo_type.items()[2].set_sensitive(false);
		combo_type.set_active(0);
	}

	//160 is the ideal width for this table column.
	//(when one widget is set, the rest wil take this width as well)
	combo_type.set_size_request(160, -1);

	combo_type.signal_changed().connect(
		sigc::bind<bool>(sigc::mem_fun(*this, &Dialog_Partition_New::combobox_changed), true));
	grid_create.attach(combo_type, 1, 0, 1, 1);

	// Partition name
	Gtk::Label *partition_name_label = Utils::mk_label(Glib::ustring(_("Partition name:")) + "\t");
	grid_create.attach(*partition_name_label, 0, 1, 1, 1);
	partition_name_entry.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                        partition_name_label->get_accessible());
	// Initialise text entry box
	partition_name_entry.set_width_chars( 20 );
	partition_name_entry.set_sensitive( device.partition_naming_supported() );
	partition_name_entry.set_max_length( device.get_max_partition_name_length() );
	// Add entry box to table
	grid_create.attach(partition_name_entry, 1, 1, 1, 1);

	// File systems to choose from
	Gtk::Label *label_filesystem = Utils::mk_label(Glib::ustring(_("File system:")) + "\t");
	grid_create.attach(*label_filesystem, 0, 1, 2, 3);
	combo_filesystem.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                    label_filesystem->get_accessible());

	build_filesystems_combo(device.readonly);

	combo_filesystem.signal_changed().connect(
		sigc::bind<bool>(sigc::mem_fun(*this, &Dialog_Partition_New::combobox_changed), false));
	grid_create.attach(combo_filesystem, 1, 2, 1, 1);

	// Label
	Gtk::Label *filesystem_label_label = Utils::mk_label(_("Label:"));
	grid_create.attach(*filesystem_label_label, 0, 3, 1, 1);
	filesystem_label_entry.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                          filesystem_label_label->get_accessible());
	//Create Text entry box
	filesystem_label_entry.set_width_chars( 20 );
	// Add entry box to table
	grid_create.attach(filesystem_label_entry, 1, 3, 1, 1);

	// Set vexpand on all grid_create child widgets
	std::vector<Gtk::Widget*> children = grid_create.get_children();
	for (std::vector<Gtk::Widget*>::iterator it = children.begin(); it != children.end(); ++it)
		(*it)->set_vexpand();

	//set some widely used values...
	MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record( selected_partition );
	START = selected_partition.sector_start;
	total_length = selected_partition.get_sector_length();
	TOTAL_MB = Utils::round( Utils::sector_to_unit( selected_partition.get_sector_length(),
	                                                selected_partition.sector_size, UNIT_MIB ) );
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;

	// Set default creatable file system.
	// (As the change signal for combo_filesystem has already been connected,
	// combobox_changed(false) is automatically called by setting the active
	// selection.  This is needed to initialise everything correctly).
	combo_filesystem.set_active(default_fs);

	//set spinbuttons initial values
	spinbutton_after .set_value( 0 ) ;
	spinbutton_size.set_value( ceil( fs_limits.max_size / double(MEBIBYTE) ) );
	spinbutton_before .set_value( MIN_SPACE_BEFORE_MB ) ;

	//Disable resizing when the total area is less than two mebibytes
	if ( TOTAL_MB < 2 )
		m_frame_resizer_base->set_sensitive(false);

	// Connect signal handler for Dialog_Base_Partition combo_alignment.
	combo_alignment.signal_changed().connect(
		sigc::bind<bool>(sigc::mem_fun(*this, &Dialog_Partition_New::combobox_changed), false));

	this ->show_all_children() ;
}


const Partition & Dialog_Partition_New::Get_New_Partition()
{
	g_assert(m_new_partition != nullptr);  // Bug: Not initialised by constructor calling set_data()

	PartitionType part_type ;
	Sector new_start, new_end;

	switch (combo_type.get_active_row_number())
	{
		case 0:	  part_type = TYPE_PRIMARY;   break;
		case 1:	  part_type = TYPE_LOGICAL;   break;
		case 2:	  part_type = TYPE_EXTENDED;  break;

		default:  part_type = TYPE_UNALLOCATED;
	}

	//FIXME:  Partition size is limited to just less than 1024 TeraBytes due
	//        to the maximum value of signed 4 byte integer.
	new_start = START + Sector(spinbutton_before.get_value_as_int()) *
	                    (MEBIBYTE / m_new_partition->sector_size);
	new_end  = new_start + Sector(spinbutton_size.get_value_as_int()) *
	                       (MEBIBYTE / m_new_partition->sector_size)
	                     - 1;
	
	/* due to loss of precision during calcs from Sector -> MiB and back, it is possible the new 
	 * partition thinks it's bigger then it can be. Here we try to solve this.*/
	if (new_start < m_new_partition->sector_start)
		new_start = m_new_partition->sector_start;
	if  (new_end > m_new_partition->sector_end)
		new_end = m_new_partition->sector_end;

	// Grow new partition a bit if freespaces are < 1 MiB
	if (new_start - m_new_partition->sector_start < MEBIBYTE / m_new_partition->sector_size)
		new_start = m_new_partition->sector_start;
	if (m_new_partition->sector_end - new_end < MEBIBYTE / m_new_partition->sector_size)
		new_end = m_new_partition->sector_end;

	// Copy a final few values needed from the original unallocated partition before
	// resetting the Partition object and populating it as the new partition.
	Glib::ustring device_path = m_new_partition->device_path;
	Sector sector_size = m_new_partition->sector_size;
	bool inside_extended = m_new_partition->inside_extended;
	m_new_partition->Reset();
	m_new_partition->Set(device_path,
	                     Glib::ustring::compose(_("New Partition #%1"), new_count),
	                     new_count,
	                     part_type,
	                     FILESYSTEMS[combo_filesystem.get_active_row_number()].fstype,
	                     new_start,
	                     new_end,
	                     sector_size,
	                     inside_extended,
	                     false);
	m_new_partition->status = STAT_NEW;

	// Retrieve partition name
	m_new_partition->name = Utils::trim(partition_name_entry.get_text());

	//Retrieve Label info
	m_new_partition->set_filesystem_label(Utils::trim(filesystem_label_entry.get_text()));

	//set alignment
	switch (combo_alignment.get_active_row_number())
	{
		case 0:
			m_new_partition->alignment = ALIGN_CYLINDER;
			break;
		case 1:
			m_new_partition->alignment = ALIGN_MEBIBYTE;
			{
				// If start sector not MiB aligned and free space available
				// then add ~1 MiB to partition so requested size is kept
				Sector diff = (MEBIBYTE / m_new_partition->sector_size) -
				              (m_new_partition->sector_end + 1) % (MEBIBYTE / m_new_partition->sector_size);
				if (    diff
				     && m_new_partition->sector_start % (MEBIBYTE / m_new_partition->sector_size) > 0
				     && m_new_partition->sector_end - START + 1 + diff < total_length
				   )
					m_new_partition->sector_end += diff;
			}
			break;
		case 2:
			m_new_partition->alignment = ALIGN_STRICT;
			break;

		default:
			m_new_partition->alignment = ALIGN_MEBIBYTE;
			break;
	}

	// Set partition flag so preview matches what is applied by
	// GParted_Core::set_partition_type() for LVM2 PV file systems.
	if (m_new_partition->fstype == FS_LVM2_PV)
		m_new_partition->set_flag("lvm");

	m_new_partition->free_space_before =   Sector(spinbutton_before.get_value_as_int())
	                                     * (MEBIBYTE / m_new_partition->sector_size);

	// Create unallocated space within this new extended partition
	//
	// FIXME:
	// Even after moving creation of the unallocated space within this new extended
	// partition to here after the above alignment adjustment, the boundaries of the
	// extended partition may be further adjusted by snap_to_alignment().  However the
	// UI when creating logical partitions within this extended partition will use the
	// boundaries as defined now by this unallocated space.  Hence boundaries of
	// logical partitions may be wrong or overlapping.
	//
	// Test case:
	// On an empty MSDOS formatted disk, create a cylinder aligned extended partition.
	// Then create a default MiB aligned logical partition filling the extended
	// partition.  Apply the operations.  Creation of logical partition fails with
	// libparted message "Can't have overlapping partitions."
	//
	// To fix this properly all the alignment constraints need to be applied here in
	// the dialogs which create and modify partition boundaries.  The logic in
	// snap_to_alignment() needs including in it.  It will need abstracting into a set
	// of methods so that it can be used in each dialog which creates and modified
	// partition boundaries.
	if (m_new_partition->type == TYPE_EXTENDED)
	{
		Partition * unallocated = new Partition();
		unallocated->Set_Unallocated(m_new_partition->device_path,
		                             m_new_partition->sector_start,
		                             m_new_partition->sector_end,
		                             m_new_partition->sector_size,
		                             true);
		m_new_partition->logicals.push_back_adopt(unallocated);
	}

	Dialog_Base_Partition::snap_to_alignment(m_device, *m_new_partition);

	return *m_new_partition;
}


void Dialog_Partition_New::combobox_changed(bool combo_type_changed)
{
	g_assert(m_new_partition != nullptr);  // Bug: Not initialised by constructor calling set_data()

	// combo_type
	if (combo_type_changed)
	{
		if (combo_type.get_active_row_number() == TYPE_EXTENDED      &&
		    combo_filesystem.items().size()    <  FILESYSTEMS.size()   )
		{
			combo_filesystem.items().push_back(Utils::get_filesystem_string(FS_EXTENDED));
			combo_filesystem.set_active(combo_filesystem.items().back());
			combo_filesystem.set_sensitive(false);
		}
		else if (combo_type.get_active_row_number() != TYPE_EXTENDED      &&
		         combo_filesystem.items().size()    == FILESYSTEMS.size()   )
		{
			combo_filesystem.set_active(default_fs);
			combo_filesystem.items().erase(combo_filesystem.items().back());
			combo_filesystem.set_sensitive(true);
		}
	}

	// combo_filesystem and combo_alignment
	if (! combo_type_changed)
	{
		fs = FILESYSTEMS[combo_filesystem.get_active_row_number()];
		fs_limits = GParted_Core::get_filesystem_limits(fs.fstype, *m_new_partition);

		if ( fs_limits.min_size < MEBIBYTE )
			fs_limits.min_size = MEBIBYTE;

		if (m_new_partition->get_byte_length() < fs_limits.min_size)
			fs_limits.min_size = m_new_partition->get_byte_length();

		if ( ! fs_limits.max_size || ( fs_limits.max_size > ((TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE) ) )
			fs_limits.max_size = (TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE;

		m_frame_resizer_base->set_x_min_space_before(Utils::round(MIN_SPACE_BEFORE_MB / MB_PER_PIXEL));
		m_frame_resizer_base->set_size_limits(Utils::round(fs_limits.min_size / (MB_PER_PIXEL * MEBIBYTE)),
		                                      Utils::round(fs_limits.max_size / (MB_PER_PIXEL * MEBIBYTE)));

		//set new spinbutton ranges
		spinbutton_before.set_range( MIN_SPACE_BEFORE_MB,
		                             TOTAL_MB - ceil( fs_limits.min_size / double(MEBIBYTE) ) );
		spinbutton_size.set_range( ceil( fs_limits.min_size / double(MEBIBYTE) ),
		                           ceil( fs_limits.max_size / double(MEBIBYTE) ) );
		spinbutton_after.set_range( 0,
		                            TOTAL_MB - MIN_SPACE_BEFORE_MB
		                            - ceil( fs_limits.min_size / double(MEBIBYTE) ) );

		//set contents of label_minmax
		Set_MinMax_Text( ceil( fs_limits.min_size / double(MEBIBYTE) ),
		                 ceil( fs_limits.max_size / double(MEBIBYTE) ) );
	}

	//set fitting resizer colors
	{
		Gdk::RGBA color_temp;
		//Background color
		color_temp.set((combo_type.get_active_row_number() == 2) ? "darkgrey" : "white");
		m_frame_resizer_base->override_default_rgb_unused_color(color_temp);

		//Partition color
		color_temp.set(Utils::get_color(fs.fstype));
		m_frame_resizer_base->set_rgb_partition_color(color_temp);
	}

	// Maximum length of the file system label varies according to the selected file system type.
	filesystem_label_entry.set_max_length(Utils::get_filesystem_label_maxlength(fs.fstype));

	m_frame_resizer_base->redraw();
}


void Dialog_Partition_New::build_filesystems_combo(bool only_unformatted)
{
	g_assert(m_new_partition != nullptr);  // Bug: Not initialised by constructor calling set_data()

	combo_filesystem.items().clear();

	// Fill the file system combobox
	for ( unsigned int t = 0 ; t < FILESYSTEMS .size( ) ; t++ ) 
	{
		// Skip extended which is only added by combobox_changed() while partition
		// type = extended.
		if (FILESYSTEMS[t].fstype == FS_EXTENDED)
			continue ;

		combo_filesystem.items().push_back(Utils::get_filesystem_string(FILESYSTEMS[t].fstype));

		if (FILESYSTEMS[t].fstype == FS_UNFORMATTED)
		{
			// Unformatted is always available
			combo_filesystem.items().back().set_sensitive(true);
		}
		else
		{
			combo_filesystem.items().back().set_sensitive(
				! only_unformatted                                                                    &&
				FILESYSTEMS[t].create                                                                 &&
				m_new_partition->get_byte_length() >= get_filesystem_min_limit(FILESYSTEMS[t].fstype)   );
		}

		//use ext4/3/2 as first/second/third choice default file system
		//(Depends on ordering in FILESYSTEMS for preference)
		if ((FILESYSTEMS[t].fstype == FS_EXT2 ||
		     FILESYSTEMS[t].fstype == FS_EXT3 ||
		     FILESYSTEMS[t].fstype == FS_EXT4   )      &&
		    combo_filesystem.items().back().sensitive()  )
		{
			default_fs = combo_filesystem.items().size() - 1;
		}
	}

	if (default_fs < 0)
	{
		// Find and set first enabled file system as last choice default.  Note
		// that unformatted will always be available.
		for (unsigned int t = 0; t < combo_filesystem.items().size(); t++)
			if (combo_filesystem.items()[t].sensitive())
			{
				default_fs = t;
				break ;
			}
	}
}


Byte_Value Dialog_Partition_New::get_filesystem_min_limit( FSType fstype )
{
	return GParted_Core::get_filesystem_limits(fstype, *m_new_partition).min_size;
}


}  // namespace GParted
