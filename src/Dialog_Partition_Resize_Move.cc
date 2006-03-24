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
 
#include "../include/Dialog_Partition_Resize_Move.h"

namespace GParted
{

Dialog_Partition_Resize_Move::Dialog_Partition_Resize_Move( const FS & fs, Sector cylinder_size )
{
	this ->fs = fs ;
	
	BUF = cylinder_size * 2 ;
}

void Dialog_Partition_Resize_Move::Set_Data( const Partition & selected_partition,
					     const std::vector<Partition> & partitions )
{
	GRIP = true ; //prevents on spinbutton_changed from getting activated prematurely
	
	this ->selected_partition = selected_partition ;
	
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
	if ( ! selected_partition .error .empty() )
		fs .shrink = GParted::FS::NONE ;
	
	//see if we need a fixed_start
	if ( fs .move )
	{
		set_title( String::ucompose( _("Resize/Move %1"), selected_partition .get_path() ) ) ;
		frame_resizer_base ->set_fixed_start( false ) ;
	}
	else
	{
		set_title( String::ucompose( _("Resize %1"), selected_partition .get_path() ) ) ;
		this ->fixed_start = true;
		frame_resizer_base ->set_fixed_start( true ) ;
		spinbutton_before .set_sensitive( false ) ;
	}
	
	//calculate total size in MiB's of previous, current and next partition
	//first find index of partition
	unsigned int t;
	for ( t = 0 ; t < partitions .size() ; t++ )
		if ( partitions[ t ] == selected_partition )
			break;

	Sector previous, next ;
	previous = next = 0 ;
	//also check the partitions filesystem ( if this is a 'resize-only' then previous should be 0 )	
	if ( t >= 1 && partitions[t -1].type == GParted::TYPE_UNALLOCATED && ! this ->fixed_start )
	{ 
		previous = partitions[t -1] .get_length() ;
		START = partitions[t -1] .sector_start ;
	} 
	else
		START = selected_partition .sector_start ;
	
	if ( t +1 < partitions .size() && partitions[t +1] .type == GParted::TYPE_UNALLOCATED )
		next = partitions[t +1] .get_length() ;
	
	total_length = previous + selected_partition .get_length() + next;
	TOTAL_MB = Utils::Round( Utils::sector_to_unit( total_length, GParted::UNIT_MIB ) ) ;
	
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;
		
	//now calculate proportional length of partition 
	frame_resizer_base ->set_x_start( Utils::Round( previous / ( total_length / 500.00 ) ) ) ;
	frame_resizer_base ->set_x_end( 
		Utils::Round( selected_partition .get_length() / ( total_length / 500.00 ) ) + frame_resizer_base ->get_x_start() ) ;
	frame_resizer_base ->set_used( Utils::Round( selected_partition.sectors_used / ( total_length / 500.00 ) ) ) ;
		
	if ( fs .shrink )
	{
		//since some filesystems have lower limits we need to check for this
		if ( selected_partition .sectors_used > fs .MIN )
			fs .MIN = selected_partition .sectors_used ;
		
		//if fs. MIN is 0 here (means used == 0 as well) it's safe to have BUF / 2
		fs .MIN += fs .MIN ? BUF : BUF/2 ;
		
		//in certain (rare) cases fs .MIN is a bit too high...
		if ( fs .MIN > selected_partition .get_length() )
			fs .MIN = selected_partition .get_length() ;
	}
	else //only grow..
		fs .MIN = selected_partition .get_length() ;
		
	fs .MAX = ( ! fs .MAX || fs .MAX > (TOTAL_MB * MEBIBYTE) ) ? ( TOTAL_MB * MEBIBYTE ) : fs .MAX -= BUF/2 ;
	
	//set values of spinbutton_before
	if ( ! fixed_start )
	{
		spinbutton_before .set_range( 0, TOTAL_MB - Utils::Round( Utils::sector_to_unit( fs .MIN, GParted::UNIT_MIB ) ) ) ;
		spinbutton_before .set_value( Utils::Round( Utils::sector_to_unit( previous, GParted::UNIT_MIB ) ) ) ;
	}
	
	//set values of spinbutton_size 
	spinbutton_size .set_range( 
		Utils::Round( Utils::sector_to_unit( fs .MIN, GParted::UNIT_MIB ) ),
		Utils::Round( Utils::sector_to_unit( fs .MAX, GParted::UNIT_MIB ) ) ) ;
	spinbutton_size .set_value( 
		Utils::Round( Utils::sector_to_unit( selected_partition .get_length(), GParted::UNIT_MIB ) ) ) ;
	
	//set values of spinbutton_after
	spinbutton_after .set_range( 0, TOTAL_MB - Utils::Round( Utils::sector_to_unit( fs .MIN, GParted::UNIT_MIB ) ) ) ;
	spinbutton_after .set_value( Utils::Round( Utils::sector_to_unit( next, GParted::UNIT_MIB ) ) ) ;
	
	frame_resizer_base ->set_size_limits( Utils::Round( fs .MIN / (MB_PER_PIXEL * MEBIBYTE) ),
					      Utils::Round( fs .MAX / (MB_PER_PIXEL * MEBIBYTE) ) ) ;
	
	//set contents of label_minmax
	Set_MinMax_Text( 
		Utils::Round( Utils::sector_to_unit( fs .MIN, GParted::UNIT_MIB ) ),
		Utils::Round( Utils::sector_to_unit( fs .MAX, GParted::UNIT_MIB ) ) ) ;
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
		previous = partitions[t -1] .get_length() ;
		START = partitions[t -1] .sector_start ;
	} 
	else
		START = selected_partition .sector_start ;
	
	//calculate length of next
	if ( t +1 < partitions .size() && partitions[ t +1 ] .type == GParted::TYPE_UNALLOCATED )
		next = partitions[ t +1 ] .get_length() ;
		
	//now we have enough data to calculate some important values..
	total_length = previous + selected_partition .get_length() + next;
	TOTAL_MB = Utils::Round( Utils::sector_to_unit( total_length, UNIT_MIB ) ) ;
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;
	
	//calculate proportional length of partition ( in pixels )
	frame_resizer_base ->set_x_start( Utils::Round( previous / ( total_length / 500.00 ) ) ) ;
	frame_resizer_base ->set_x_end( Utils::Round( selected_partition .get_length() / ( total_length / 500.00 ) ) + frame_resizer_base ->get_x_start() ) ;
	
	//used is a bit different here... we consider start of first logical to end last logical as used space
	Sector first =0, used =0 ;
	for ( unsigned int i = 0 ; i < partitions[ t ] .logicals .size() ; i++ )
	{
		if ( partitions[ t ] .logicals[ i ] .type == GParted::TYPE_LOGICAL )
		{
			if ( first == 0 )
				first = partitions[ t ] .logicals[ i ] .sector_start ;
			
			used = partitions[ t ] .logicals[ i ] .sector_end - first;
		}
	}

	dynamic_cast<Frame_Resizer_Extended *>( frame_resizer_base ) ->
		set_used_start( Utils::Round( (first - START) / ( total_length / 500.00 ) ) ) ;
	frame_resizer_base ->set_used( Utils::Round( used / ( total_length / 500.00 ) ) ) ;
	
	//set values of spinbutton_before (we assume there is no fixed start.)
	if ( first == 0 ) //no logicals
		spinbutton_before .set_range( 0, TOTAL_MB - Utils::Round( Utils::sector_to_unit( BUF/2, GParted::UNIT_MIB ) ) ) ;
	else
		spinbutton_before .set_range( 0, Utils::Round( Utils::sector_to_unit( first - START, GParted::UNIT_MIB ) ) ) ;
	
	spinbutton_before .set_value( Utils::Round( Utils::sector_to_unit( previous, GParted::UNIT_MIB ) ) ) ;
	
	//set values of spinbutton_size
	if ( first == 0 ) //no logicals
		spinbutton_size .set_range( Utils::Round( Utils::sector_to_unit( BUF/2, GParted::UNIT_MIB ) ), TOTAL_MB ) ;
	else
		spinbutton_size .set_range( Utils::Round( Utils::sector_to_unit( used, GParted::UNIT_MIB ) ), TOTAL_MB ) ;
	
	spinbutton_size .set_value(
		Utils::Round( Utils::sector_to_unit( selected_partition .get_length(), GParted::UNIT_MIB ) ) ) ;
	
	//set values of spinbutton_after
	if ( first == 0 ) //no logicals
		spinbutton_after .set_range( 
			0, TOTAL_MB - Utils::Round( Utils::sector_to_unit( BUF/2, GParted::UNIT_MIB ) ) ) ;
	else
		spinbutton_after .set_range( 
			0, Utils::Round( Utils::sector_to_unit( total_length + START - first - used, GParted::UNIT_MIB ) ) ) ;
	
	spinbutton_after .set_value( Utils::Round( Utils::sector_to_unit( next, GParted::UNIT_MIB ) ) ) ;
	
	//set contents of label_minmax
	Set_MinMax_Text( Utils::Round( Utils::sector_to_unit( first == 0 ? BUF/2 : used, GParted::UNIT_MIB ) ),
			 Utils::Round( Utils::sector_to_unit( total_length, GParted::UNIT_MIB ) ) ) ;
}

} //GParted
