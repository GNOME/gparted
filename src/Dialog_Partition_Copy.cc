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

Dialog_Partition_Copy::Dialog_Partition_Copy()
{
	Set_Resizer( false ) ;	
	Set_Confirm_Button( PASTE ) ;
}

void Dialog_Partition_Copy::Set_Data( Partition & selected_partition, Partition & copied_partition )
{
	GRIP = true ; //prevents on spinbutton_changed from getting activated prematurely
	
	this ->set_title( String::ucompose( _("Paste %1"), copied_partition .partition ) ) ;
	
	//set partition color
	frame_resizer_base ->set_rgb_partition_color( copied_partition .color ) ;
	
	//set some widely used values...
	START = selected_partition.sector_start ;
	total_length = selected_partition.sector_end - selected_partition.sector_start ;
	TOTAL_MB = selected_partition .Get_Length_MB() ;
	MB_PER_PIXEL = (double) TOTAL_MB / 500    ;
	
	//now calculate proportional length of partition 
	frame_resizer_base ->set_x_start( 0 ) ;
	frame_resizer_base ->set_x_end( ( Round( (double)  (copied_partition.sector_end - copied_partition.sector_start) / ( (double)total_length/500)  )) ) ;
	frame_resizer_base ->set_used( frame_resizer_base ->get_x_end() ) ;
	
	//set values of spinbutton_before
	spinbutton_before .set_range( 0,  TOTAL_MB - copied_partition .Get_Length_MB() -1  ) ;//mind the -1  !!
	spinbutton_before .set_value( 0 ) ;
		
	//set values of spinbutton_size (check for fat16 maxsize of 1023 MB)
	double UPPER;
	if ( copied_partition.filesystem == "fat16" && Sector_To_MB( total_length ) > 1023 )
		UPPER = 1023 ;
	else
		UPPER =  TOTAL_MB;
	
	spinbutton_size .set_range(  copied_partition .Get_Length_MB() +1, UPPER ) ;
	spinbutton_size .set_value(  copied_partition .Get_Length_MB()  ) ;
	
	//set values of spinbutton_after
	spinbutton_after .set_range(  0, TOTAL_MB - copied_partition .Get_Length_MB() -1  ) ;
	spinbutton_after .set_value( TOTAL_MB - copied_partition .Get_Length_MB()  ) ;
	
	//set contents of label_minmax
	os << String::ucompose( _("Minimum Size: %1 MB"), copied_partition .Get_Length_MB() +1 ) << "\t\t" ;
	os << String::ucompose( _("Maximum Size: %1 MB"), Sector_To_MB( total_length ) ) ;
	label_minmax.set_text( os.str() ) ; os.str("") ;
	
	//set global selected_partition (see Dialog_Base_Partition::Get_New_Partition )
	this ->selected_partition = copied_partition ;
	this ->selected_partition .inside_extended = selected_partition .inside_extended  ;
	selected_partition .inside_extended ? this ->selected_partition .type = GParted::LOGICAL : this ->selected_partition .type = GParted::PRIMARY ;
	
	GRIP = false ;
}

Partition Dialog_Partition_Copy::Get_New_Partition() 
{
	//first call baseclass to get the correct new partition
	selected_partition = Dialog_Base_Partition::Get_New_Partition() ;
	
	//set proper name and status for partition
//	selected_partition.partition = String::ucompose( _("copy from %1"), selected_partition .partition );
	selected_partition.status = GParted::STAT_COPY ;
	
	return selected_partition ;
}

} //GParted
