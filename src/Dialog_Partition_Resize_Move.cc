/* Copyright (C) 2004 Bart
 * Copyright (C) 2010 Curtis Gedak
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


#include "Dialog_Partition_Resize_Move.h"

#include "Device.h"
#include "FileSystem.h"
#include "Frame_Resizer_Extended.h"
#include "GParted_Core.h"
#include "Partition.h"
#include "PartitionVector.h"
#include "Utils.h"

#include <gdkmm/rgba.h>
#include <glibmm/ustring.h>
#include <glib.h>


namespace GParted
{


Dialog_Partition_Resize_Move::Dialog_Partition_Resize_Move(const Device& device, const FS & fs,
                                                           const FS_Limits & fs_limits,
                                                           const Partition & selected_partition,
                                                           const PartitionVector & partitions)
 : Dialog_Base_Partition(device)
{
	this ->fs = fs ;
	this->fs_limits = fs_limits;
	set_data( selected_partition, partitions );
}


void Dialog_Partition_Resize_Move::set_data( const Partition & selected_partition,
                                             const PartitionVector & partitions )
{
	GRIP = true ; //prevents on spinbutton_changed from getting activated prematurely

	m_new_partition.reset(selected_partition.clone());

	if (selected_partition.type == TYPE_EXTENDED)
	{
		Set_Resizer( true ) ;	
		Resize_Move_Extended( partitions ) ;
	}
	else
	{
		Set_Resizer( false ) ;	
		Resize_Move_Normal( partitions ) ;
	}

	// Set partition color
	Gdk::RGBA partition_color(Utils::get_color(selected_partition.fstype));
	m_frame_resizer_base->set_rgb_partition_color(partition_color);

	//store the original values
	ORIG_BEFORE 	= spinbutton_before .get_value_as_int() ;
	ORIG_SIZE	= spinbutton_size .get_value_as_int() ;
	ORIG_AFTER	= spinbutton_after .get_value_as_int() ;
	
	GRIP = false ;
	
	Set_Confirm_Button( RESIZE_MOVE ) ;	
	this ->show_all_children() ;
}


void Dialog_Partition_Resize_Move::Resize_Move_Normal( const PartitionVector & partitions )
{
	g_assert(m_new_partition != nullptr);  // Bug: Not initialised by constructor calling set_data()

	// Don't permit shrinking an existing file system (other than linux-swap) when the
	// usage is unknown as that sets the minimum resize.
	if (! m_new_partition->sector_usage_known()  &&
	    m_new_partition->status != STAT_NEW      &&
	    m_new_partition->fstype != FS_LINUX_SWAP   )
		fs.shrink = FS::NONE;

	//Disable resizing as it's currently disallowed for the file system in this partition.
	//  (Updates this class's copy of file system support information).
	if (GParted_Core::filesystem_resize_disallowed(*m_new_partition))
	{
		fs .shrink = FS::NONE ;
		fs .grow   = FS::NONE ;
	}

	// See if we can allow the start of the file system to move
	if (fs.move && ! m_new_partition->busy && m_new_partition->type != TYPE_UNPARTITIONED)
	{
		set_title(Glib::ustring::compose(_("Resize/Move %1"), m_new_partition->get_path()));
		m_frame_resizer_base->set_fixed_start(false);
	}
	else
	{
		set_title(Glib::ustring::compose(_("Resize %1"), m_new_partition->get_path()));
		this ->fixed_start = true;
		m_frame_resizer_base->set_fixed_start(true);
		spinbutton_before .set_sensitive( false ) ;
	}

	//calculate total size in MiB's of previous, current and next partition
	//first find index of partition
	unsigned int t;
	for ( t = 0 ; t < partitions .size() ; t++ )
		if (partitions[t] == *m_new_partition)
			break;

	Sector previous = 0;
	Sector next = 0;
	const Partition* prev_unalloc_partition = nullptr;
	//also check the partitions file system ( if this is a 'resize-only' then previous should be 0 )	
	if (t >= 1 && partitions[t-1].type == TYPE_UNALLOCATED && ! this->fixed_start)
	{ 
		previous = partitions[t -1] .get_sector_length() ;
		START = partitions[t -1] .sector_start ;
		prev_unalloc_partition = &partitions[t-1];
	} 
	else
		START = m_new_partition->sector_start;

	if (t+1 < partitions.size() && partitions[t+1].type == TYPE_UNALLOCATED)
	{
		next = partitions[t +1] .get_sector_length() ;

		//If this is a logical partition and there is extra free space
		//  then check if we need to reserve 1 MiB of space after for
		//  the next logical partition Extended Boot Record.
		if (m_new_partition->type == TYPE_LOGICAL                            &&
		    next                  >= MEBIBYTE / m_new_partition->sector_size   )
		{
			//Find maximum sector_end (allocated or unallocated) within list of
			//  partitions inside the extended partition
			Sector max_sector_end = 0 ;
			for (unsigned int k = 0 ; k < partitions .size() ; k++ )
			{
				if ( partitions[ k ] .sector_end > max_sector_end )
					max_sector_end = partitions[ k ] .sector_end ;
			}

			//If not within 1 MiB of the end of the extended partition, then reserve 1 MiB
			if (max_sector_end - partitions[t+1].sector_end > MEBIBYTE / m_new_partition->sector_size)
				next -= MEBIBYTE / m_new_partition->sector_size;
		}
	}

	// Only need to use MIN_SPACE_BEFORE_MB to reserve 1 MiB to protect the partition
	// table or EBR if there is a previous unallocated partition allowing the start of
	// this selected partition to be resize/moved to the left.
	if (prev_unalloc_partition == nullptr)
		MIN_SPACE_BEFORE_MB = 0 ;
	else
		MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record(*prev_unalloc_partition);

	total_length = previous + m_new_partition->get_sector_length() + next;
	TOTAL_MB = Utils::round( Utils::sector_to_unit(total_length, m_new_partition->sector_size, UNIT_MIB));

	MB_PER_PIXEL = TOTAL_MB / 500.00 ;

	//now calculate proportional length of partition 
	m_frame_resizer_base->set_x_min_space_before(Utils::round(MIN_SPACE_BEFORE_MB / MB_PER_PIXEL));
	m_frame_resizer_base->set_x_start(Utils::round(previous / (total_length / 500.00)));
	m_frame_resizer_base->set_x_end(
	                Utils::round(m_new_partition->get_sector_length() / (total_length / 500.00)) +
	                m_frame_resizer_base->get_x_start());
	Sector min_resize = m_new_partition->estimated_min_size();
	m_frame_resizer_base->set_used(Utils::round(min_resize / (total_length / 500.00)));

	//set MIN
	if ((fs.shrink        && ! m_new_partition->busy) ||
	    (fs.online_shrink && m_new_partition->busy  )   )
	{
		//since some file systems have lower limits we need to check for this
		if (min_resize > fs_limits.min_size / m_new_partition->sector_size)
			fs_limits.min_size = min_resize * m_new_partition->sector_size;

		//ensure that minimum size is at least one mebibyte
		if ( ! fs_limits.min_size || fs_limits.min_size < MEBIBYTE )
			fs_limits.min_size = MEBIBYTE;
	}
	else
	{
		fs_limits.min_size = m_new_partition->get_byte_length();
	}

	//set MAX
	if ( fs .grow )
		fs_limits.max_size = (TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE;
	else
		fs_limits.max_size = m_new_partition->get_byte_length();

	//set values of spinbutton_before
	if ( ! fixed_start )
	{
		spinbutton_before.set_range( MIN_SPACE_BEFORE_MB,
		                             TOTAL_MB - ceil( fs_limits.min_size / double(MEBIBYTE) ) );
		spinbutton_before .set_value(
			Utils::round(Utils::sector_to_unit(previous, m_new_partition->sector_size, UNIT_MIB)));
	}

	//set values of spinbutton_size 
	spinbutton_size.set_range( ceil( fs_limits.min_size / double(MEBIBYTE) ),
	                           ceil( fs_limits.max_size / double(MEBIBYTE) ) );
	spinbutton_size .set_value( 
	                Utils::round(Utils::sector_to_unit(m_new_partition->get_sector_length(),
	                                                   m_new_partition->sector_size,
	                                                   UNIT_MIB)));

	//set values of spinbutton_after
	Sector after_min = ( ! fs .grow && ! fs .move ) ? next : 0 ;
	spinbutton_after .set_range( 
	                Utils::round(Utils::sector_to_unit(after_min, m_new_partition->sector_size, UNIT_MIB)),
	                TOTAL_MB - MIN_SPACE_BEFORE_MB - ceil(fs_limits.min_size / double(MEBIBYTE)));
	spinbutton_after .set_value( 
	                Utils::round(Utils::sector_to_unit(next, m_new_partition->sector_size, UNIT_MIB)));

	m_frame_resizer_base->set_size_limits(Utils::round(fs_limits.min_size / (MB_PER_PIXEL * MEBIBYTE)),
	                                      Utils::round(fs_limits.max_size / (MB_PER_PIXEL * MEBIBYTE)));

	//set contents of label_minmax
	Set_MinMax_Text( ceil( fs_limits.min_size / double(MEBIBYTE) ),
	                 ceil( fs_limits.max_size / double(MEBIBYTE) ) );
}


void Dialog_Partition_Resize_Move::Resize_Move_Extended( const PartitionVector & partitions )
{
	g_assert(m_new_partition != nullptr);  // Bug: Not initialised by constructor calling set_data()

	set_title(Glib::ustring::compose(_("Resize/Move %1"), m_new_partition->get_path()));

	//calculate total size in MiB's of previous, current and next partition
	//first find index of partition
	unsigned int t = 0;
	while (t < partitions.size() && partitions[t].type != TYPE_EXTENDED)
		t++;

	Sector previous = 0;
	Sector next = 0;
	const Partition* prev_unalloc_partition = nullptr;
	//calculate length and start of previous
	if (t > 0 && partitions[t-1].type == TYPE_UNALLOCATED)
	{
		previous = partitions[t -1] .get_sector_length() ;
		START = partitions[t -1] .sector_start ;
		prev_unalloc_partition = &partitions[t-1];
	} 
	else
		START = m_new_partition->sector_start;

	//calculate length of next
	if (t+1 < partitions.size() && partitions[t+1].type == TYPE_UNALLOCATED)
		next = partitions[ t +1 ] .get_sector_length() ;
		
	//now we have enough data to calculate some important values..

	// Only need to use MIN_SPACE_BEFORE_MB to reserve 1 MiB to protect the partition
	// table or EBR if there is a previous unallocated partition allowing the start of
	// this selected partition to be resize/moved to the left.
	if (prev_unalloc_partition == nullptr)
		MIN_SPACE_BEFORE_MB = 0 ;
	else
		MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record(*prev_unalloc_partition);

	total_length = previous + m_new_partition->get_sector_length() + next;
	TOTAL_MB = Utils::round(Utils::sector_to_unit(total_length, m_new_partition->sector_size, UNIT_MIB));
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;

	//calculate proportional length of partition ( in pixels )
	m_frame_resizer_base->set_x_min_space_before(Utils::round(MIN_SPACE_BEFORE_MB / MB_PER_PIXEL));
	m_frame_resizer_base->set_x_start(Utils::round(previous / (total_length / 500.00)));
	m_frame_resizer_base->set_x_end(
	                Utils::round(m_new_partition->get_sector_length() / (total_length / 500.00)) +
	                m_frame_resizer_base->get_x_start());
	
	//used is a bit different here... we consider start of first logical to end last logical as used space
	Sector first = 0;
	Sector last = 0;
	Sector used = 0;
	if ( ! (m_new_partition->logicals.size()      == 1                &&
	        m_new_partition->logicals.back().type == TYPE_UNALLOCATED   ))
	{
		//logical partitions other than unallocated exist
		first = m_new_partition->sector_end;
		last = m_new_partition->sector_start;
		for ( unsigned int i = 0 ; i < partitions[ t ] .logicals .size() ; i++ )
		{
			if (partitions[t].logicals[i].type == TYPE_LOGICAL)
			{
				if ( partitions[ t ] .logicals[ i ] .sector_start < first )
					first =   partitions[t].logicals[i].sector_start
					        - (MEBIBYTE / m_new_partition->sector_size);
				if ( first < 0 )
					first = 0 ;
				if ( partitions[ t ] .logicals[ i ] .sector_end > last )
					last = partitions[ t ] .logicals[ i ] .sector_end ;
			}
		}
		used = last - first;
	}
	//set MIN
	if ( used == 0 )
		//Reasonable minimum of 1 MiB for EBR plus 1 MiB for small partition
		fs_limits.min_size = MEBIBYTE;
	else 
		fs_limits.min_size = used * m_new_partition->sector_size;

	//set MAX
	fs_limits.max_size = (TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE;

	dynamic_cast<Frame_Resizer_Extended*>(m_frame_resizer_base.get())->
		set_used_start( Utils::round( (first - START) / ( total_length / 500.00 ) ) ) ;
	m_frame_resizer_base->set_used(Utils::round(used / (total_length / 500.00)));

	//set values of spinbutton_before (we assume there is no fixed start.)
	if ( first == 0 ) //no logicals
		spinbutton_before.set_range( MIN_SPACE_BEFORE_MB,
		                             TOTAL_MB - MIN_SPACE_BEFORE_MB - ceil( fs_limits.min_size /
		                                                                    double(MEBIBYTE) ) );
	else
		spinbutton_before.set_range(MIN_SPACE_BEFORE_MB,
		                            Utils::round(Utils::sector_to_unit(first - START,
		                                                               m_new_partition->sector_size,
		                                                               UNIT_MIB)));

	spinbutton_before.set_value(Utils::round(Utils::sector_to_unit(previous,
	                                                               m_new_partition->sector_size, UNIT_MIB)));

	//set values of spinbutton_size
	spinbutton_size.set_range( ceil( fs_limits.min_size / double(MEBIBYTE) ),
	                           TOTAL_MB - MIN_SPACE_BEFORE_MB );

	spinbutton_size.set_value(Utils::round(Utils::sector_to_unit(m_new_partition->get_sector_length(),
	                                                             m_new_partition->sector_size, UNIT_MIB)));

	//set values of spinbutton_after
	if ( first == 0 ) //no logicals
		spinbutton_after.set_range( 0, TOTAL_MB -
		                               ceil( fs_limits.min_size / double(MEBIBYTE) ) -
		                               MIN_SPACE_BEFORE_MB );
	else
		spinbutton_after.set_range(0, Utils::round(Utils::sector_to_unit(total_length + START - first - used,
		                                                                 m_new_partition->sector_size,
		                                                                 UNIT_MIB)));

	spinbutton_after.set_value(Utils::round(Utils::sector_to_unit(next,
	                                                              m_new_partition->sector_size, UNIT_MIB)));

	//set contents of label_minmax
	Set_MinMax_Text(ceil(fs_limits.min_size / double(MEBIBYTE)),
	                Utils::round(Utils::sector_to_unit(total_length - MIN_SPACE_BEFORE_MB * (MEBIBYTE / m_new_partition->sector_size),
	                                                   m_new_partition->sector_size,
	                                                   UNIT_MIB)));
}


}  // namespace GParted
