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

#include "../include/Dialog_Partition_Copy.h"

namespace GParted
{

Dialog_Partition_Copy::Dialog_Partition_Copy( const FS & fs, long cylinder_size )
{
	this ->fs = fs ;
	
	BUF = cylinder_size ;
	
	Set_Resizer( false ) ;	
	Set_Confirm_Button( PASTE ) ;
}

void Dialog_Partition_Copy::Set_Data( const Partition & selected_partition, const Partition & copied_partition )
{
	this ->set_title( String::ucompose( _("Paste %1"), copied_partition .partition ) ) ;
	
	//set partition color
	frame_resizer_base ->set_rgb_partition_color( copied_partition .color ) ;
	
	//set some widely used values...
	START = selected_partition .sector_start ;
	total_length = selected_partition .sector_end - selected_partition .sector_start ;
	TOTAL_MB = selected_partition .Get_Length_MB( ) ;
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;
	
	long COPIED_LENGTH_MB = copied_partition .Get_Length_MB( ) ;
	
	//now calculate proportional length of partition 
	frame_resizer_base ->set_x_start( 0 ) ;
	int x_end = Round( COPIED_LENGTH_MB / ( TOTAL_MB/500.00 ) ) ; //> 500 px only possible with xfs...
	frame_resizer_base ->set_x_end( x_end > 500 ? 500 : x_end ) ;
	frame_resizer_base ->set_used( Round( copied_partition .Get_Used_MB( ) / ( TOTAL_MB/500.00) ) ) ;
	
	fs .MAX = ( ! fs .MAX || fs .MAX > TOTAL_MB ) ? TOTAL_MB : fs .MAX -= BUF ;
	
	if ( fs .filesystem == "xfs" ) //bit hackisch, but most effective, since it's a unique situation
		fs .MIN = copied_partition .Get_Used_MB( ) + (BUF * 2) ;
	else
		fs .MIN = COPIED_LENGTH_MB +1 ;
	
	GRIP = true ;
	//set values of spinbutton_before
	spinbutton_before .set_range( 0, TOTAL_MB - fs .MIN ) ;
	spinbutton_before .set_value( 0 ) ;
		
	//set values of spinbutton_size
	spinbutton_size .set_range( fs .MIN, fs .MAX ) ;
	spinbutton_size .set_value( COPIED_LENGTH_MB ) ;
	
	//set values of spinbutton_after
	spinbutton_after .set_range( 0, TOTAL_MB - fs .MIN ) ;
	spinbutton_after .set_value( TOTAL_MB - COPIED_LENGTH_MB ) ; 
	GRIP = false ;
	
	frame_resizer_base ->set_size_limits( Round(fs .MIN / MB_PER_PIXEL), Round(fs .MAX / MB_PER_PIXEL) +1 ) ;
	
	//set contents of label_minmax
	Set_MinMax_Text( fs .MIN, fs .MAX ) ;
	
	//set global selected_partition (see Dialog_Base_Partition::Get_New_Partition )
	this ->selected_partition = copied_partition ;
	this ->selected_partition .inside_extended = selected_partition .inside_extended ;
	this ->selected_partition .type = selected_partition .inside_extended ? GParted::LOGICAL : GParted::PRIMARY ;
}

Partition Dialog_Partition_Copy::Get_New_Partition( ) 
{
	//first call baseclass to get the correct new partition
	selected_partition = Dialog_Base_Partition::Get_New_Partition( ) ;
	
	//set proper name and status for partition
	selected_partition .status = GParted::STAT_COPY ;
	
	return selected_partition ;
}

} //GParted
