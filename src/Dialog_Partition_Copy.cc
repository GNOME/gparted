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

#include "../include/Dialog_Partition_Copy.h"

namespace GParted
{

Dialog_Partition_Copy::Dialog_Partition_Copy( const FS & fs, Sector cylinder_size )
{
	this ->fs = fs ;
	
	BUF = cylinder_size ;
	
	Set_Resizer( false ) ;	
	Set_Confirm_Button( PASTE ) ;
}

void Dialog_Partition_Copy::Set_Data( const Partition & selected_partition, const Partition & copied_partition )
{
	this ->set_title( String::ucompose( _("Paste %1"), copied_partition .get_path() ) ) ;
	
	//set partition color
	frame_resizer_base ->set_rgb_partition_color( copied_partition .color ) ;
	
	//set some widely used values...
	START = selected_partition .sector_start ;
	total_length = selected_partition .get_sector_length() ;
	TOTAL_MB = Utils::round( Utils::sector_to_unit( selected_partition .get_sector_length(), selected_partition .sector_size, UNIT_MIB ) ) ;
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;

	//Determine minimum number of sectors needed in destination (selected) partition and
	//  handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required.
	Sector copied_min_sectors = ( copied_partition .get_byte_length() + (selected_partition .sector_size - 1) ) / selected_partition .sector_size ;

	long COPIED_LENGTH_MB = Utils::round( Utils::sector_to_unit( copied_min_sectors, selected_partition .sector_size, UNIT_MIB ) ) ; 
	//  /* Copy Primary not at start of disk to within Extended partition */
	//  Adjust when a primary partition is copied and pasted
	//  into an unallocated space in an extended partition
	//  of an MSDOS partition table.
	//  Since the Extended Boot Record requires an additional track,
	//  this must be considered in the required space for the
	//  destination (selected) partition.
	//  NOTE:  This problem does not occur for a primary partition
	//  at the the start of the disk because the size of the EBR and
	//  Master Boot Record are the same.
	//
	//  /* Copy Primary not at start of disk to Primary at start of disk */
	//  Also Adjust when a primary partition that does not start at the
	//  beginning of the disk is copied and pasted
	//  into an unallocated space at the start of the disk device.
	//  Since the Master Boot Record requires an additional track,
	//  this must be considered in the required space for the
	//  destination (selected) partition.
	//
	//  Because the largest unit used in the GUI is one
	//  cylinder size (round to cylinders), the space
	//  needed in the destination partition needs to be increased
	//  by enough to round up one cylinder size.
	//  Increase by half a cylinder size (or 4 MB) because this
	//  will round up to the next cylinder size.
	//  8 MB is typical cylinder size with todays larger disks.
	//  8 MB = (255 heads) * (63 sectors) * (512 bytes)
	//
	//FIXME:  Should confirm MSDOS partition table type, track sector size, and use cylinder size from device
	if (   (/* Copy Primary not at start of disk to within Extended partition */
		       copied_partition .type == TYPE_PRIMARY
		    && copied_partition .sector_start > 63
		    && selected_partition .type == TYPE_UNALLOCATED
		    && selected_partition .inside_extended
		   )
		|| ( /* Copy Primary not at start of disk to Primary at start of disk */
		       copied_partition .type == TYPE_PRIMARY
		    && copied_partition .sector_start > 63
		    && selected_partition .type == TYPE_UNALLOCATED
		    && selected_partition .sector_start <=63 /* Beginning of disk device */
		    && ! selected_partition .inside_extended
		   )
	   )
		COPIED_LENGTH_MB += 4 ;

	//now calculate proportional length of partition 
	frame_resizer_base ->set_x_start( 0 ) ;
	int x_end = Utils::round( COPIED_LENGTH_MB / ( TOTAL_MB/500.00 ) ) ; //> 500 px only possible with xfs...
	frame_resizer_base ->set_x_end( x_end > 500 ? 500 : x_end ) ;
	frame_resizer_base ->set_used( 
		Utils::round( Utils::sector_to_unit( 
				copied_partition .sectors_used, copied_partition .sector_size, UNIT_MIB ) / (TOTAL_MB/500.00) ) ) ;

	if ( fs .grow )
		fs .MAX = ( ! fs .MAX || fs .MAX > (TOTAL_MB * MEBIBYTE) ) ? (TOTAL_MB * MEBIBYTE) : fs .MAX - (BUF * selected_partition .sector_size) ;
	else
		fs .MAX = copied_partition .get_byte_length() ;

	//TODO: Since BUF is the cylinder size of the current device, the cylinder size of the copied device could differ for small disks
	if ( fs .filesystem == GParted::FS_XFS ) //bit hackisch, but most effective, since it's a unique situation
		fs .MIN = ( copied_partition .sectors_used + (BUF * 2) ) * copied_partition .sector_size;
	else
		fs .MIN = COPIED_LENGTH_MB * MEBIBYTE ;
	
	GRIP = true ;
	//set values of spinbutton_before
	spinbutton_before .set_range( 0, TOTAL_MB - Utils::round( Utils::sector_to_unit( fs .MIN, 1 /* Byte */, UNIT_MIB ) ) ) ;
	spinbutton_before .set_value( 0 ) ;
		
	//set values of spinbutton_size
	spinbutton_size .set_range(
		Utils::round( Utils::sector_to_unit( fs .MIN, 1 /* Byte */, UNIT_MIB ) ),
		Utils::round( Utils::sector_to_unit( fs .MAX, 1 /* Byte */, UNIT_MIB ) ) ) ;
	spinbutton_size .set_value( COPIED_LENGTH_MB ) ;
	
	//set values of spinbutton_after
	spinbutton_after .set_range( 0, TOTAL_MB - Utils::round( Utils::sector_to_unit( fs .MIN, 1 /* Byte */, UNIT_MIB ) ) ) ;
	spinbutton_after .set_value( TOTAL_MB - COPIED_LENGTH_MB ) ; 
	GRIP = false ;
	
	frame_resizer_base ->set_size_limits( Utils::round( fs .MIN / (MB_PER_PIXEL * MEBIBYTE) ),
					      Utils::round( fs .MAX / (MB_PER_PIXEL * MEBIBYTE) ) ) ;
	
	//set contents of label_minmax
	Set_MinMax_Text( 
		Utils::round( Utils::sector_to_unit( fs .MIN, 1 /* Byte */, UNIT_MIB ) ),
		Utils::round( Utils::sector_to_unit( fs .MAX, 1 /* Byte */, UNIT_MIB ) ) ) ;

	//set global selected_partition (see Dialog_Base_Partition::Get_New_Partition )
	this ->selected_partition = copied_partition ;
	this ->selected_partition .device_path = selected_partition .device_path ;
	this ->selected_partition .inside_extended = selected_partition .inside_extended ;
	this ->selected_partition .type = 
		selected_partition .inside_extended ? GParted::TYPE_LOGICAL : GParted::TYPE_PRIMARY ;
	//Handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required.
	this ->selected_partition .set_used(
			( (copied_partition .sectors_used * copied_partition .sector_size)
			  + (selected_partition .sector_size - 1)
			) / selected_partition .sector_size ) ;

	this ->show_all_children() ;
}

Partition Dialog_Partition_Copy::Get_New_Partition( Byte_Value sector_size )
{
	//first call baseclass to get the correct new partition
	selected_partition = Dialog_Base_Partition::Get_New_Partition( sector_size ) ;

	//set proper name and status for partition
	selected_partition .status = GParted::STAT_COPY ;

	return selected_partition ;
}

} //GParted
