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
 
#include "../include/Dialog_Partition_Resize_Move.h"
#include "../include/GParted_Core.h"

namespace GParted
{

Dialog_Partition_Resize_Move::Dialog_Partition_Resize_Move( const FS & fs, const Partition & selected_partition,
                                                            const std::vector<Partition> & partitions )
{
	this ->fs = fs ;
	set_data( selected_partition, partitions );
}

void Dialog_Partition_Resize_Move::set_data( const Partition & selected_partition,
                                             const std::vector<Partition> & partitions )
{
	GRIP = true ; //prevents on spinbutton_changed from getting activated prematurely

	new_partition = selected_partition;

	if ( selected_partition .type == GParted::TYPE_EXTENDED )
	{
		Set_Resizer( true ) ;	
		Resize_Move_Extended( partitions ) ;
	}
	else
	{
		Set_Resizer( false ) ;	
		Resize_Move_Normal( partitions ) ;
	}
	
	//set partition color
	frame_resizer_base ->set_rgb_partition_color( selected_partition .color ) ;
	
	//store the original values
	ORIG_BEFORE 	= spinbutton_before .get_value_as_int() ;
	ORIG_SIZE	= spinbutton_size .get_value_as_int() ;
	ORIG_AFTER	= spinbutton_after .get_value_as_int() ;
	
	GRIP = false ;
	
	Set_Confirm_Button( RESIZE_MOVE ) ;	
	this ->show_all_children() ;
}

void Dialog_Partition_Resize_Move::Resize_Move_Normal( const std::vector<Partition> & partitions )
{
	//little bit of paranoia ;)
	if ( ! new_partition.sector_usage_known() &&
	     new_partition.status != STAT_NEW &&
	     new_partition.filesystem != FS_LINUX_SWAP )
		fs .shrink = GParted::FS::NONE ;
	
	//Disable resizing as it's currently disallowed for the file system in this partition.
	//  (Updates this class's copy of file system support information).
	if ( GParted_Core::filesystem_resize_disallowed( new_partition ) )
	{
		fs .shrink = FS::NONE ;
		fs .grow   = FS::NONE ;
	}

	// See if we can allow the start of the file system to move
	if ( fs.move && ! new_partition.busy && ! new_partition.whole_device )
	{
		set_title( String::ucompose( _("Resize/Move %1"), new_partition.get_path() ) );
		frame_resizer_base ->set_fixed_start( false ) ;
	}
	else
	{
		set_title( String::ucompose( _("Resize %1"), new_partition.get_path() ) );
		this ->fixed_start = true;
		frame_resizer_base ->set_fixed_start( true ) ;
		spinbutton_before .set_sensitive( false ) ;
	}
	
	//calculate total size in MiB's of previous, current and next partition
	//first find index of partition
	unsigned int t;
	for ( t = 0 ; t < partitions .size() ; t++ )
		if ( partitions[t] == new_partition )
			break;

	Sector previous, next ;
	previous = next = 0 ;
	//also check the partitions file system ( if this is a 'resize-only' then previous should be 0 )	
	if ( t >= 1 && partitions[t -1].type == GParted::TYPE_UNALLOCATED && ! this ->fixed_start )
	{ 
		previous = partitions[t -1] .get_sector_length() ;
		START = partitions[t -1] .sector_start ;
	} 
	else
		START = new_partition.sector_start;
	
	if ( t +1 < partitions .size() && partitions[t +1] .type == GParted::TYPE_UNALLOCATED )
	{
		next = partitions[t +1] .get_sector_length() ;

		//If this is a logical partition and there is extra free space
		//  then check if we need to reserve 1 MiB of space after for
		//  the next logical partition Extended Boot Record.
		if (   new_partition.type == TYPE_LOGICAL
		    && next >= (MEBIBYTE / new_partition.sector_size)
		   )
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
			if ( ( max_sector_end - partitions[t+1].sector_end ) > ( MEBIBYTE / new_partition.sector_size ) )
				next -= MEBIBYTE / new_partition.sector_size;
		}
	}

	//Only calculate MIN_SPACE_BEFORE_MB if we have a previous (unallocated) partition.
	//  Otherwise there will not be enough graphical space to reserve a full 1 MiB for MBR/EBR.
	//  Since this is an existing partition, if an MBR/EBR was needed then it already exists with enough space.
	if ( previous <= 0 )
		MIN_SPACE_BEFORE_MB = 0 ;
	else
	{
		if ( START <= (MEBIBYTE / new_partition.sector_size) )
			MIN_SPACE_BEFORE_MB = 1 ;
		else
			MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record( new_partition );
	}
	total_length = previous + new_partition.get_sector_length() + next;
	TOTAL_MB = Utils::round( Utils::sector_to_unit( total_length, new_partition.sector_size, UNIT_MIB ) );
	
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;
		
	//now calculate proportional length of partition 
	frame_resizer_base ->set_x_min_space_before( Utils::round( MIN_SPACE_BEFORE_MB / MB_PER_PIXEL ) ) ;
	frame_resizer_base ->set_x_start( Utils::round( previous / ( total_length / 500.00 ) ) ) ;
	frame_resizer_base ->set_x_end( 
		Utils::round( new_partition.get_sector_length() / ( total_length / 500.00 ) ) + frame_resizer_base->get_x_start() );
	Sector min_resize = new_partition.estimated_min_size();
	frame_resizer_base ->set_used( Utils::round( min_resize / ( total_length / 500.00 ) ) ) ;

	//set MIN
	if (   ( fs.shrink && ! new_partition.busy )
	    || ( fs.online_shrink && new_partition.busy )
	   )
	{
		//since some file systems have lower limits we need to check for this
		if ( min_resize > (fs.MIN / new_partition.sector_size) )
			fs.MIN = min_resize * new_partition.sector_size;

		//ensure that minimum size is at least one mebibyte
		if ( ! fs .MIN || fs .MIN < MEBIBYTE )
			fs .MIN = MEBIBYTE ;
	}
	else
		fs.MIN = new_partition.get_byte_length();

	//set MAX
	if ( fs .grow )
		fs .MAX = (TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE ;
	else
		fs.MAX = new_partition.get_byte_length();

	//set values of spinbutton_before
	if ( ! fixed_start )
	{
		spinbutton_before .set_range( MIN_SPACE_BEFORE_MB
		                            , TOTAL_MB - ceil( fs .MIN / double(MEBIBYTE) )
		                            ) ;
		spinbutton_before .set_value(
			Utils::round( Utils::sector_to_unit( previous, new_partition.sector_size, UNIT_MIB ) ) );
	}

	//set values of spinbutton_size 
	spinbutton_size .set_range( ceil( fs .MIN / double(MEBIBYTE) )
	                          , ceil( fs .MAX / double(MEBIBYTE) )
	                          ) ;
	spinbutton_size .set_value( 
		Utils::round( Utils::sector_to_unit( new_partition.get_sector_length(), new_partition.sector_size, UNIT_MIB ) ) );
	
	//set values of spinbutton_after
	Sector after_min = ( ! fs .grow && ! fs .move ) ? next : 0 ;
	spinbutton_after .set_range( 
		Utils::round( Utils::sector_to_unit( after_min, new_partition.sector_size, UNIT_MIB ) ),
		TOTAL_MB - MIN_SPACE_BEFORE_MB - ceil( fs .MIN / double(MEBIBYTE) ) ) ;
	spinbutton_after .set_value( 
		Utils::round( Utils::sector_to_unit( next, new_partition.sector_size, UNIT_MIB ) ) );

	frame_resizer_base ->set_size_limits( Utils::round( fs .MIN / (MB_PER_PIXEL * MEBIBYTE) ),
					      Utils::round( fs .MAX / (MB_PER_PIXEL * MEBIBYTE) ) ) ;

	//set contents of label_minmax
	Set_MinMax_Text( ceil( fs .MIN / double(MEBIBYTE) )
	               , ceil( fs .MAX / double(MEBIBYTE) )
	               ) ;
}

void Dialog_Partition_Resize_Move::Resize_Move_Extended( const std::vector<Partition> & partitions )
{
	//calculate total size in MiB's of previous, current and next partition
	//first find index of partition
	unsigned int t = 0;
	while ( t < partitions .size() && partitions[ t ] .type != GParted::TYPE_EXTENDED ) t++ ;
	
	Sector previous, next ;
	previous = next = 0 ;
	//calculate length and start of previous
	if ( t > 0 && partitions[t -1] .type == GParted::TYPE_UNALLOCATED )
	{
		previous = partitions[t -1] .get_sector_length() ;
		START = partitions[t -1] .sector_start ;
	} 
	else
		START = new_partition.sector_start;
	
	//calculate length of next
	if ( t +1 < partitions .size() && partitions[ t +1 ] .type == GParted::TYPE_UNALLOCATED )
		next = partitions[ t +1 ] .get_sector_length() ;
		
	//now we have enough data to calculate some important values..
	//Only calculate MIN_SPACE_BEFORE_MB if we have a previous (unallocated) partition.
	//  Otherwise there will not be enough graphical space to reserve a full 1 MiB for MBR/EBR.
	//  Since this is an existing partition, if an MBR/EBR was needed then it already exists with enough space.
	if ( previous <= 0 )
		MIN_SPACE_BEFORE_MB = 0 ;
	else
		MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record( new_partition );
	total_length = previous + new_partition.get_sector_length() + next;
	TOTAL_MB = Utils::round( Utils::sector_to_unit( total_length, new_partition.sector_size, UNIT_MIB ) );
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;
	
	//calculate proportional length of partition ( in pixels )
	frame_resizer_base ->set_x_min_space_before( Utils::round( MIN_SPACE_BEFORE_MB / MB_PER_PIXEL ) ) ;
	frame_resizer_base ->set_x_start( Utils::round( previous / ( total_length / 500.00 ) ) ) ;
	frame_resizer_base ->set_x_end( Utils::round( new_partition.get_sector_length() / ( total_length / 500.00 ) ) + frame_resizer_base ->get_x_start() );
	
	//used is a bit different here... we consider start of first logical to end last logical as used space
	Sector first =0, last = 0, used =0 ;
	if ( ! (   new_partition.logicals.size() == 1
	        && new_partition.logicals.back().type == TYPE_UNALLOCATED
	       )
	   )
	{
		//logical partitions other than unallocated exist
		first = new_partition.sector_end;
		last = new_partition.sector_start;
		for ( unsigned int i = 0 ; i < partitions[ t ] .logicals .size() ; i++ )
		{
			if ( partitions[ t ] .logicals[ i ] .type == GParted::TYPE_LOGICAL )
			{
				if ( partitions[ t ] .logicals[ i ] .sector_start < first )
					first = partitions[ t ] .logicals[ i ] .sector_start - (MEBIBYTE / new_partition.sector_size);
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
	{
		//Reasonable minimum of 1 MiB for EBR plus 1 MiB for small partition
		fs .MIN = MEBIBYTE ;
	}
	else 
		fs.MIN = used * new_partition.sector_size;

	//set MAX
	fs .MAX = (TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE ;

	dynamic_cast<Frame_Resizer_Extended *>( frame_resizer_base ) ->
		set_used_start( Utils::round( (first - START) / ( total_length / 500.00 ) ) ) ;
	frame_resizer_base ->set_used( Utils::round( used / ( total_length / 500.00 ) ) ) ;
	
	//set values of spinbutton_before (we assume there is no fixed start.)
	if ( first == 0 ) //no logicals
		spinbutton_before .set_range( MIN_SPACE_BEFORE_MB, TOTAL_MB - MIN_SPACE_BEFORE_MB - ceil( fs .MIN / double(MEBIBYTE) ) ) ;
	else
		spinbutton_before .set_range( MIN_SPACE_BEFORE_MB, Utils::round( Utils::sector_to_unit( first - START, new_partition.sector_size, UNIT_MIB ) ) );
	
	spinbutton_before .set_value( Utils::round( Utils::sector_to_unit( previous, new_partition.sector_size, UNIT_MIB ) ) );
	
	//set values of spinbutton_size
	spinbutton_size .set_range( ceil( fs .MIN / double(MEBIBYTE) ), TOTAL_MB - MIN_SPACE_BEFORE_MB ) ;
	
	spinbutton_size .set_value(
		Utils::round( Utils::sector_to_unit( new_partition.get_sector_length(), new_partition.sector_size, UNIT_MIB ) ) );
	
	//set values of spinbutton_after
	if ( first == 0 ) //no logicals
		spinbutton_after .set_range( 
			0, TOTAL_MB - ceil( fs .MIN / double(MEBIBYTE) ) - MIN_SPACE_BEFORE_MB ) ;
	else
		spinbutton_after .set_range( 
			0, Utils::round( Utils::sector_to_unit( total_length + START - first - used, new_partition.sector_size, UNIT_MIB ) ) );
	
	spinbutton_after .set_value( Utils::round( Utils::sector_to_unit( next, new_partition.sector_size, UNIT_MIB ) ) );
	
	//set contents of label_minmax
	Set_MinMax_Text( ceil( fs .MIN / double(MEBIBYTE) )
	               , Utils::round( Utils::sector_to_unit( total_length - (MIN_SPACE_BEFORE_MB * (MEBIBYTE / new_partition.sector_size)), new_partition.sector_size, UNIT_MIB ) ) );
}

} //GParted
