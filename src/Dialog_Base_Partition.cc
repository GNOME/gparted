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


#include "Dialog_Base_Partition.h"

#include "Device.h"
#include "Frame_Resizer_Base.h"
#include "Frame_Resizer_Extended.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <gtkmm/box.h>
#include <gtkmm/dialog.h>
#include <gtkmm/enums.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/widget.h>
#include <atkmm/relation.h>
#include <sigc++/bind.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <glib.h>
#include <memory>


namespace GParted
{


Dialog_Base_Partition::Dialog_Base_Partition(const Device& device)
 : m_device(device)
{
	GRIP = false ;
	this ->fixed_start = false ;
	this ->set_resizable( false );
	ORIG_BEFORE = ORIG_SIZE = ORIG_AFTER = -1 ;
	MIN_SPACE_BEFORE_MB = -1 ;

	// Pack resizer hbox
	hbox_resizer.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
	this->get_content_area()->pack_start(hbox_resizer, Gtk::PACK_SHRINK);

	// Add label_minmax
	this->get_content_area()->pack_start(label_minmax, Gtk::PACK_SHRINK);

	// Pack hbox_main
	hbox_main.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
	this->get_content_area()->pack_start(hbox_main, Gtk::PACK_SHRINK);

	// Put the vbox with resizer stuff (cool widget and spinbuttons) in the hbox_main
	vbox_resize_move.set_orientation(Gtk::ORIENTATION_VERTICAL);
	hbox_main .pack_start( vbox_resize_move, Gtk::PACK_EXPAND_PADDING );

	// Fill grid
	grid_resize.set_border_width(5);
	grid_resize.set_row_spacing(5);
	hbox_grid.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
	hbox_grid.pack_start(grid_resize, Gtk::PACK_EXPAND_PADDING);

	hbox_grid.set_border_width(5);
	vbox_resize_move.pack_start(hbox_grid, Gtk::PACK_SHRINK);

	// Add spinbutton_before
	Gtk::Label *label_before = Utils::mk_label(Glib::ustring(_("Free space preceding (MiB):")) + " \t");
	grid_resize.attach(*label_before, 0, 0, 1, 1);
	spinbutton_before.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_before->get_accessible());

	spinbutton_before .set_numeric( true );
	spinbutton_before .set_increments( 1, 100 );
	spinbutton_before.set_width_chars(7);
	grid_resize.attach(spinbutton_before, 1, 0, 1, 1);

	// Add spinbutton_size
	Gtk::Label *label_size = Utils::mk_label(_("New size (MiB):"));
	grid_resize.attach(*label_size, 0, 1, 1, 1);
	spinbutton_size.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_size->get_accessible());

	spinbutton_size .set_numeric( true );
	spinbutton_size .set_increments( 1, 100 );
	spinbutton_size.set_width_chars(7);
	grid_resize.attach(spinbutton_size, 1, 1, 1, 1);

	// Add spinbutton_after
	Gtk::Label *label_after = Utils::mk_label(_("Free space following (MiB):"));
	grid_resize.attach(*label_after, 0, 2, 1, 1);
	spinbutton_after.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_after->get_accessible());

	spinbutton_after .set_numeric( true );
	spinbutton_after .set_increments( 1, 100 );
	spinbutton_after.set_width_chars(7);
	grid_resize.attach(spinbutton_after, 1, 2, 1, 1);

	if ( ! fixed_start )
		before_value = spinbutton_before .get_value() ;

	//connect signalhandlers of the spinbuttons
	if ( ! fixed_start )
		before_change_connection = spinbutton_before .signal_value_changed() .connect(
			sigc::bind<SPINBUTTON>( 
				sigc::mem_fun(*this, &Dialog_Base_Partition::on_spinbutton_value_changed), BEFORE ) ) ;
	else
		//Initialise empty connection object for use in the destructor
		before_change_connection = sigc::connection() ;
	
	size_change_connection = spinbutton_size .signal_value_changed() .connect(
		sigc::bind<SPINBUTTON>( 
			sigc::mem_fun(*this, &Dialog_Base_Partition::on_spinbutton_value_changed), SIZE ) ) ;
	after_change_connection = spinbutton_after .signal_value_changed() .connect(
		sigc::bind<SPINBUTTON>( 
			sigc::mem_fun(*this, &Dialog_Base_Partition::on_spinbutton_value_changed), AFTER ) ) ;

	// Add alignment
	/* TO TRANSLATORS: used as label for a list of choices.  Align to: <combo box with choices> */
	Gtk::Label *label_alignment = Utils::mk_label(Glib::ustring(_("Align to:")) + "\t");
	grid_resize.attach(*label_alignment, 0, 3, 1, 1);
	combo_alignment.get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY, label_alignment->get_accessible());

	// Fill partition alignment combo
	/* TO TRANSLATORS: Option for combo box "Align to:" */
	combo_alignment.items().push_back(_("Cylinder"));
	/* TO TRANSLATORS: Option for combo box "Align to:" */
	combo_alignment.items().push_back(_("MiB"));
	/* TO TRANSLATORS: Option for combo box "Align to:" */
	combo_alignment.items().push_back(_("None"));

	// Default setting for partition table alignment
	if (device.disktype == "amiga")
		combo_alignment.set_active(ALIGN_CYLINDER);
	else
		combo_alignment.set_active(ALIGN_MEBIBYTE);

	grid_resize.attach(combo_alignment, 1, 3, 1, 1);

	// Set vexpand on all grid_resize child widgets
	std::vector<Gtk::Widget*> children = grid_resize.get_children();
	for (std::vector<Gtk::Widget*>::iterator it = children.begin(); it != children.end(); ++it)
		(*it)->set_vexpand();

	this->add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	this ->show_all_children() ;
}


void Dialog_Base_Partition::Set_Resizer( bool extended )
{
	if ( extended )
		m_frame_resizer_base = std::make_unique<Frame_Resizer_Extended>();
	else
	{
		m_frame_resizer_base = std::make_unique<Frame_Resizer_Base>();
		m_frame_resizer_base->signal_move.connect(sigc::mem_fun(this, &Dialog_Base_Partition::on_signal_move));
	}

	m_frame_resizer_base->set_border_width(5);
	m_frame_resizer_base->set_shadow_type(Gtk::SHADOW_ETCHED_OUT);

	//connect signals
	m_frame_resizer_base->signal_resize.connect(sigc::mem_fun(this, &Dialog_Base_Partition::on_signal_resize));

	hbox_resizer.pack_start(*m_frame_resizer_base, Gtk::PACK_EXPAND_PADDING);

	this ->show_all_children() ;
}


const Partition & Dialog_Base_Partition::Get_New_Partition()
{
	g_assert(m_new_partition != nullptr);  // Bug: Not initialised by derived Dialog_Partition_*() constructor calling set_data()

	prepare_new_partition();
	return *m_new_partition;
}


void Dialog_Base_Partition::prepare_new_partition()
{
	g_assert(m_new_partition != nullptr);  // Bug: Not initialised by derived Dialog_Partition_*() constructor calling set_data()

	Sector old_size = m_new_partition->get_sector_length();

	//FIXME:  Partition size is limited to just less than 1024 TeraBytes due
	//        to the maximum value of signed 4 byte integer.
	if ( ORIG_BEFORE != spinbutton_before .get_value_as_int() )
		m_new_partition->sector_start = START + Sector(spinbutton_before.get_value_as_int()) * (MEBIBYTE / m_new_partition->sector_size);

	if ( ORIG_AFTER != spinbutton_after .get_value_as_int() )
		m_new_partition->sector_end =
			m_new_partition->sector_start
			+ Sector(spinbutton_size.get_value_as_int()) * (MEBIBYTE / m_new_partition->sector_size)
			- 1 /* one sector short of exact mebibyte multiple */;

	//due to loss of precision during calcs from Sector -> MiB and back, it is possible
	//the new partition thinks it's bigger then it can be. Here we solve this.
	if (m_new_partition->sector_start < START)
		m_new_partition->sector_start = START;
	if (m_new_partition->sector_end > (START + total_length - 1))
		m_new_partition->sector_end = START + total_length - 1;

	//grow a bit into small freespace ( < 1MiB ) 
	if ((m_new_partition->sector_start - START) < (MEBIBYTE / m_new_partition->sector_size))
		m_new_partition->sector_start = START;
	if ((START + total_length - 1 - m_new_partition->sector_end) < (MEBIBYTE / m_new_partition->sector_size))
		m_new_partition->sector_end = START + total_length - 1;

	//set alignment
	switch (combo_alignment.get_active_row_number())
	{
		case 0:
			m_new_partition->alignment = ALIGN_CYLINDER;
			break;
		case 1:
			m_new_partition->alignment = ALIGN_MEBIBYTE;
			{
				// If partition start or end sectors are not MiB aligned,
				// and space is available, then add 1 MiB to partition so
				// requesting size is kept after snap_to_mebibyte() method
				// rounding.
				Sector partition_size = m_new_partition->sector_end - m_new_partition->sector_start + 1;
				Sector sectors_in_mib = MEBIBYTE / m_new_partition->sector_size;
				if (    (    (m_new_partition->sector_start % sectors_in_mib     > 0)
				          || ((m_new_partition->sector_end + 1) % sectors_in_mib > 0)
				        )
				     && ( partition_size + sectors_in_mib < total_length )
				   )
				{
					m_new_partition->sector_end += sectors_in_mib;
				}
			}
			break;
		case 2:
			m_new_partition->alignment = ALIGN_STRICT;
			break;

		default:
			m_new_partition->alignment = ALIGN_MEBIBYTE;
			break;
	}

	m_new_partition->free_space_before =   Sector(spinbutton_before.get_value_as_int())
	                                     * (MEBIBYTE / m_new_partition->sector_size);

	// If the original before value has not changed, then set indicator to keep start sector unchanged.
	if ( ORIG_BEFORE == spinbutton_before .get_value_as_int() )
		m_new_partition->strict_start = true;

	snap_to_alignment(m_device, *m_new_partition);

	//update partition usage
	if (m_new_partition->sector_usage_known())
	{
		Sector new_size = m_new_partition->get_sector_length();
		if ( old_size == new_size )
		{
			//Pasting into new same sized partition or moving partition keeping the same size,
			//  therefore only block copy operation will be performed maintaining file system size.
			m_new_partition->set_sector_usage(
					m_new_partition->sectors_used + m_new_partition->sectors_unused,
					m_new_partition->sectors_unused);
		}
		else
		{
			//Pasting into new larger partition or (moving and) resizing partition larger or smaller,
			//  therefore block copy followed by file system grow or shrink operations will be
			//  performed making the file system fill the partition.
			m_new_partition->set_sector_usage(new_size, new_size - m_new_partition->sectors_used);
		}
	}
}


void Dialog_Base_Partition::snap_to_alignment(const Device& device, Partition& partition)
{
	if (partition.alignment == ALIGN_CYLINDER)
		snap_to_cylinder(device, partition);
	else if (partition.alignment == ALIGN_MEBIBYTE)
		snap_to_mebibyte(device, partition);
}


void Dialog_Base_Partition::snap_to_cylinder(const Device& device, Partition& partition)
{
	Sector diff = 0;

	// Determine if partition size is less than half a disk cylinder.
	bool less_than_half_cylinder = false;
	if (partition.sector_end - partition.sector_start < device.cylsize / 2)
		less_than_half_cylinder = true;

	if (partition.type         == TYPE_LOGICAL   ||
	    partition.sector_start == device.sectors   )
	{
		// Must account the relative offset between:
		// (A) the Extended Boot Record sector and the next track of the
		//     logical partition (usually 63 sectors), and
		// (B) the Master Boot Record sector and the next track of the first
		//     primary partition.
		diff = (partition.sector_start - device.sectors) % device.cylsize;
	}
	else if (partition.sector_start == 34)
	{
		// (C) the GUID Partition Table (GPT) and the start of the data
		//     partition at sector 34.
		diff = (partition.sector_start - 34) % device.cylsize;
	}
	else
	{
		diff = partition.sector_start % device.cylsize;
	}
	if (diff && ! partition.strict_start)
	{
		if (diff < device.cylsize / 2 || less_than_half_cylinder)
			partition.sector_start -= diff;
		else
			partition.sector_start += device.cylsize - diff;
	}

	diff = (partition.sector_end + 1) % device.cylsize;
	if (diff)
	{
		if (diff < device.cylsize / 2 && ! less_than_half_cylinder)
			partition.sector_end -= diff;
		else
			partition.sector_end += device.cylsize - diff;
	}
}


void Dialog_Base_Partition::snap_to_mebibyte(const Device& device, Partition& partition)
{
	Sector diff = 0;
	if (partition.sector_start < 2 || partition.type == TYPE_LOGICAL)
	{
		// Must account the relative offset between:
		// (A) the Master Boot Record sector and the first primary/extended partition, and
		// (B) the Extended Boot Record sector and the logical partition.

		// If moving the starting sector and no preceding space then add minimum
		// space to force alignment to next mebibyte.
		if (! partition.strict_start && partition.free_space_before == 0)
		{
			// Unless specifically told otherwise, the Linux kernel considers extended
			// boot records to be two sectors long, in order to "leave room for LILO".
			partition.sector_start += 2;
		}
	}

	// Calculate difference offset from Mebibyte boundary.
	diff = Sector(partition.sector_start % (MEBIBYTE / partition.sector_size));

	// Align start sector only if permitted to change start sector.
	if (diff && (! partition.strict_start                                      ||
	            (partition.strict_start && (partition.status == STAT_NEW  ||
	                                        partition.status == STAT_COPY   ))   ))
	{
		partition.sector_start += MEBIBYTE / partition.sector_size - diff;

		// If this is an extended partition then check to see if sufficient space is
		// available for any following logical partition Extended Boot Record.
		if (partition.type == TYPE_EXTENDED)
		{
			// If there is logical partition that starts less than 2 sectors from
			// the start of this partition, then reserve a mebibyte for the EBR.
			int index_extended = find_extended_partition(device.partitions);
			if (index_extended >= 0)
			{
				for (unsigned int i = 0; i < device.partitions[index_extended].logicals.size(); i++)
				{
					if (device.partitions[index_extended].logicals[i].type == TYPE_LOGICAL &&
					    // Unless specifically told otherwise, the Linux kernel considers extended
					    // boot records to be two sectors long, in order to "leave room for LILO".
					    device.partitions[index_extended].logicals[i].sector_start
					                                          - partition.sector_start < 2   )
					{
						partition.sector_start -= MEBIBYTE / partition.sector_size;
					}
				}
			}
		}
	}

	// Align end sector.
	diff = (partition.sector_end + 1) % (MEBIBYTE / partition.sector_size);
	if (diff)
		partition.sector_end -= diff;

	// If this is a logical partition not at end of drive then check to see if space
	// is required for a following logical partition Extended Boot Record.
	if (partition.type == TYPE_LOGICAL)
	{
		// If there is a following logical partition that starts less than 2 sectors
		// from the end of this partition, then reserve at least a mebibyte for the EBR.
		int index_extended = find_extended_partition(device.partitions);
		if (index_extended >= 0)
		{
			for (unsigned int i = 0; i < device.partitions[index_extended].logicals.size(); i++)
			{
				if (device.partitions[index_extended].logicals[i].type         == TYPE_LOGICAL         &&
				    device.partitions[index_extended].logicals[i].sector_start  > partition.sector_end &&
				    // Unless specifically told otherwise, the Linux kernel considers extended
				    // boot records to be two sectors long, in order to "leave room for LILO".
				    device.partitions[index_extended].logicals[i].sector_start
				                                                            - partition.sector_end < 2   )
				{
					partition.sector_end -= MEBIBYTE / partition.sector_size;
				}
			}
		}

		// If the logical partition end is beyond the end of the extended partition
		// then reduce logical partition end by a mebibyte to address the overlap.
		if (index_extended        != -1                                           &&
		    partition.sector_end   > device.partitions[index_extended].sector_end   )
		{
			partition.sector_end -= MEBIBYTE / partition.sector_size;
		}
	}

	// If this is a primary or an extended partition and the partition overlaps
	// the start of the next primary or extended partition then subtract a
	// mebibyte from the end of the partition to address the overlap.
	if (partition.type == TYPE_PRIMARY || partition.type == TYPE_EXTENDED)
	{
		for (unsigned int i = 0; i < device.partitions.size(); i++)
		{
			if ((device.partitions[i].type == TYPE_PRIMARY ||
			     device.partitions[i].type == TYPE_EXTENDED  )                      &&
			     // For a change to an existing partition, (e.g., move or resize)
			     // skip comparing to original partition and only compare to
			     // other existing partitions.
			    partition.status           == STAT_REAL                             &&
			    partition.partition_number != device.partitions[i].partition_number &&
			    device.partitions[i].sector_start  > partition.sector_start         &&
			    device.partitions[i].sector_start <= partition.sector_end             )
			{
				partition.sector_end -= MEBIBYTE / partition.sector_size;
			}
		}
	}

	// If this is an extended partition then check to see if the end of the
	// extended partition encompasses the end of the last logical partition.
	if (partition.type == TYPE_EXTENDED)
	{
		// If there is logical partition that has an end sector beyond the
		// end of the extended partition, then set the extended partition
		// end sector to be the same as the end of the logical partition.
		for (unsigned int i = 0; i < partition.logicals.size(); i++)
		{
			if (partition.logicals[i].type       == TYPE_LOGICAL         &&
			    partition.logicals[i].sector_end  > partition.sector_end   )
			{
				partition.sector_end = partition.logicals[i].sector_end;
			}
		}
	}

	// If this is a GPT partition table and the partition ends less than 34 sectors
	// from the end of the device, then reserve at least a mebibyte for the backup
	// partition table.
	if (device.disktype == "gpt" && device.length - partition.sector_end < 34)
		partition.sector_end -= MEBIBYTE / partition.sector_size;
}


void Dialog_Base_Partition::Set_Confirm_Button( CONFIRMBUTTON button_type ) 
{ 
	switch( button_type )
	{
		case NEW	:
			this ->add_button( Gtk::Stock::ADD, Gtk::RESPONSE_OK );
			
			break ;
		case RESIZE_MOVE:
			{
				Gtk::Image* image_temp = Utils::mk_image(Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON);
				Gtk::Box* hbox_resize_move(Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)));

				hbox_resize_move->pack_start(*image_temp, Gtk::PACK_EXPAND_PADDING);
				hbox_resize_move->pack_start(*Utils::mk_label(fixed_start ? _("Resize") : _("Resize/Move")),
							Gtk::PACK_EXPAND_PADDING);
				button_resize_move.add(*hbox_resize_move);
			}

			this ->add_action_widget ( button_resize_move, Gtk::RESPONSE_OK ) ;
			button_resize_move .set_sensitive( false ) ;
			
			break ;
		case PASTE	:
			this ->add_button( Gtk::Stock::PASTE, Gtk::RESPONSE_OK );
			
			break ;
	}
}

void Dialog_Base_Partition::Set_MinMax_Text( Sector min, Sector max )
{
	Glib::ustring str_temp(Glib::ustring::compose( _("Minimum size: %1 MiB"), min ) + "\t\t");
	str_temp += Glib::ustring::compose( _("Maximum size: %1 MiB"), max ) ;
	label_minmax .set_text( str_temp ) ; 
}


int Dialog_Base_Partition::MB_Needed_for_Boot_Record( const Partition & partition )
{
	// Determine if space needs reserving for the partition table or the EBR (Extended
	// Boot Record).  Generally a track or MEBIBYTE is reserved.  For our purposes
	// reserve a MEBIBYTE at the start of the partition.
	// NOTE: This logic also contained in Win_GParted::set_valid_operations()
	if (partition.inside_extended                                 ||
	    partition.sector_start < MEBIBYTE / partition.sector_size   )
		return 1 ;
	else
		return 0 ;
}

void Dialog_Base_Partition::on_signal_move( int x_start, int x_end )
{  
	GRIP = true ;

	spinbutton_before .set_value( x_start * MB_PER_PIXEL ) ;

	if ( x_end == 500 )
	{
		spinbutton_after .set_value( 0 ) ;
		spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value() ) ;
	}
	else
		spinbutton_after .set_value( TOTAL_MB - spinbutton_before .get_value() - spinbutton_size .get_value() ) ;

	update_button_resize_move_sensitivity();

	GRIP = false ;
}


void Dialog_Base_Partition::on_signal_resize( int x_start, int x_end, Frame_Resizer_Base::ArrowType arrow )
{  
	GRIP = true ;
		
	spinbutton_size .set_value( ( x_end - x_start ) * MB_PER_PIXEL ) ;
	
	before_value = fixed_start ? MIN_SPACE_BEFORE_MB : spinbutton_before .get_value() ;

	if ( arrow == Frame_Resizer_Base::ARROW_RIGHT ) //don't touch freespace before, leave it as it is
	{
		if ( x_end == 500 )
		{
			spinbutton_after .set_value( 0 ) ;
			spinbutton_size .set_value( TOTAL_MB - before_value ) ;
		}
		else
			spinbutton_after .set_value( TOTAL_MB - before_value - spinbutton_size .get_value() ) ;
	}
	else if ( arrow == Frame_Resizer_Base::ARROW_LEFT ) //don't touch freespace after, leave it as it is
		spinbutton_before .set_value( TOTAL_MB - spinbutton_size .get_value() - spinbutton_after .get_value() ) ;

	update_button_resize_move_sensitivity();

	GRIP = false ;
}


void Dialog_Base_Partition::on_spinbutton_value_changed( SPINBUTTON spinbutton )
{
	g_assert(m_frame_resizer_base != nullptr);  // Bug: Not initialised by call to Set_Resizer()

	if ( ! GRIP )
	{
		before_value = fixed_start ? MIN_SPACE_BEFORE_MB : spinbutton_before .get_value() ;
			
		//Balance the spinbuttons
		switch ( spinbutton )
		{
			case BEFORE	:
				spinbutton_after .set_value( TOTAL_MB - spinbutton_size .get_value() - before_value ) ;
				spinbutton_size .set_value( TOTAL_MB - before_value - spinbutton_after .get_value() ) ;
							
				break ;
			case SIZE	:
				spinbutton_after .set_value( TOTAL_MB - before_value - spinbutton_size .get_value() );
				if ( ! fixed_start )
					spinbutton_before .set_value( 
						TOTAL_MB - spinbutton_size .get_value() - spinbutton_after .get_value() );
				
				break;
			case AFTER	:
				if ( ! fixed_start )
					spinbutton_before .set_value( 
						TOTAL_MB - spinbutton_size .get_value() - spinbutton_after .get_value() );
			
				spinbutton_size .set_value( TOTAL_MB - before_value - spinbutton_after .get_value() ) ;
				
				break;
		}
		
		//And apply the changes to the visual view...
		if ( ! fixed_start )
			m_frame_resizer_base->set_x_start(Utils::round(spinbutton_before.get_value() / MB_PER_PIXEL));

		m_frame_resizer_base->set_x_end(500 - Utils::round(spinbutton_after.get_value() / MB_PER_PIXEL));

		m_frame_resizer_base->redraw();

		update_button_resize_move_sensitivity();
	}
}


void Dialog_Base_Partition::update_button_resize_move_sensitivity()
{
	button_resize_move .set_sensitive(
		ORIG_BEFORE != spinbutton_before .get_value_as_int()	||
		ORIG_SIZE   != spinbutton_size .get_value_as_int()	|| 
		ORIG_AFTER  != spinbutton_after .get_value_as_int() ) ; 
}


Dialog_Base_Partition::~Dialog_Base_Partition() 
{
	before_change_connection .disconnect() ;
	size_change_connection .disconnect() ;
	after_change_connection .disconnect() ;

	// Work around a Gtk issue fixed in 3.24.0.
	// https://gitlab.gnome.org/GNOME/gtk/issues/125
	hide();
}


}  // namespace GParted
