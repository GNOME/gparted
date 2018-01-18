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

#include "Dialog_Partition_Copy.h"
#include "FileSystem.h"
#include "GParted_Core.h"
#include "Partition.h"
#include "Utils.h"

namespace GParted
{

Dialog_Partition_Copy::Dialog_Partition_Copy( const FS & fs, const FS_Limits & fs_limits,
                                              const Partition & selected_partition,
                                              const Partition & copied_partition )
{
	this ->fs = fs ;
	this->fs_limits = fs_limits;

	Set_Resizer( false ) ;	
	Set_Confirm_Button( PASTE ) ;

	set_data( selected_partition, copied_partition );
}

Dialog_Partition_Copy::~Dialog_Partition_Copy()
{
	delete new_partition;
	new_partition = NULL;
}

void Dialog_Partition_Copy::set_data( const Partition & selected_partition, const Partition & copied_partition )
{
	this ->set_title( String::ucompose( _("Paste %1"), copied_partition .get_path() ) ) ;
	
	//set partition color
	Gdk::Color partition_color( Utils::get_color( copied_partition.filesystem ) );
	frame_resizer_base->set_rgb_partition_color( partition_color );

	//set some widely used values...
	MIN_SPACE_BEFORE_MB = Dialog_Base_Partition::MB_Needed_for_Boot_Record( selected_partition ) ;
	START = selected_partition .sector_start ;
	total_length = selected_partition .get_sector_length() ;
	TOTAL_MB = Utils::round( Utils::sector_to_unit( selected_partition .get_sector_length(), selected_partition .sector_size, UNIT_MIB ) ) ;
	MB_PER_PIXEL = TOTAL_MB / 500.00 ;

	//Determine minimum number of sectors needed in destination (selected) partition and
	//  handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required.
	Sector copied_min_sectors = ( copied_partition .get_byte_length() + (selected_partition .sector_size - 1) ) / selected_partition .sector_size ;

	long COPIED_LENGTH_MB = ceil( Utils::sector_to_unit( copied_min_sectors, selected_partition .sector_size, UNIT_MIB ) ) ;

	//now calculate proportional length of partition 
	frame_resizer_base ->set_x_min_space_before( Utils::round( MIN_SPACE_BEFORE_MB / MB_PER_PIXEL ) ) ;
	frame_resizer_base ->set_x_start( Utils::round(MIN_SPACE_BEFORE_MB / MB_PER_PIXEL) ) ;
	int x_end = Utils::round( (MIN_SPACE_BEFORE_MB + COPIED_LENGTH_MB) / ( TOTAL_MB/500.00 ) ) ; //> 500 px only possible with xfs...
	frame_resizer_base ->set_x_end( x_end > 500 ? 500 : x_end ) ;
	Sector min_resize = copied_partition .estimated_min_size() ;
	frame_resizer_base ->set_used( 
		Utils::round( Utils::sector_to_unit( min_resize, copied_partition .sector_size, UNIT_MIB ) / (TOTAL_MB/500.00) ) ) ;

	//Only allow pasting into a new larger partition if growing the file
	//  system is implemented and resizing it is currently allowed.
	if ( fs .grow && ! GParted_Core::filesystem_resize_disallowed( copied_partition ) )
	{
		if ( ! fs_limits.max_size || fs_limits.max_size > ((TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE) )
			fs_limits.max_size = (TOTAL_MB - MIN_SPACE_BEFORE_MB) * MEBIBYTE;
	}
	else
	{
		fs_limits.max_size = copied_partition.get_byte_length();
	}

	if ( fs .filesystem == GParted::FS_XFS ) //bit hackisch, but most effective, since it's a unique situation
		fs_limits.min_size = std::max( fs_limits.min_size, min_resize * copied_partition.sector_size );
	else
		fs_limits.min_size = COPIED_LENGTH_MB * MEBIBYTE;

	GRIP = true ;
	//set values of spinbutton_before
	spinbutton_before.set_range( MIN_SPACE_BEFORE_MB, TOTAL_MB - ceil( fs_limits.min_size / double(MEBIBYTE) ) );
	spinbutton_before .set_value( MIN_SPACE_BEFORE_MB ) ;

	//set values of spinbutton_size
	spinbutton_size.set_range( ceil( fs_limits.min_size / double(MEBIBYTE) ),
	                           ceil( fs_limits.max_size / double(MEBIBYTE) ) );
	spinbutton_size .set_value( COPIED_LENGTH_MB ) ;
	
	//set values of spinbutton_after
	spinbutton_after.set_range( 0, TOTAL_MB - MIN_SPACE_BEFORE_MB - ceil( fs_limits.min_size / double(MEBIBYTE) ) );
	spinbutton_after .set_value( TOTAL_MB - MIN_SPACE_BEFORE_MB - COPIED_LENGTH_MB ) ; 
	GRIP = false ;
	
	frame_resizer_base->set_size_limits( Utils::round( fs_limits.min_size / (MB_PER_PIXEL * MEBIBYTE) ),
	                                     Utils::round( fs_limits.max_size / (MB_PER_PIXEL * MEBIBYTE) ) );

	//set contents of label_minmax
	Set_MinMax_Text( ceil( fs_limits.min_size / double(MEBIBYTE) ),
	                 ceil( fs_limits.max_size / double(MEBIBYTE) ) );

	// Set member variable used in Dialog_Base_Partition::prepare_new_partition()
	new_partition = copied_partition.clone();
	new_partition->device_path     = selected_partition.device_path;
	new_partition->inside_extended = selected_partition.inside_extended;
	new_partition->type            = selected_partition.inside_extended ? TYPE_LOGICAL : TYPE_PRIMARY;
	new_partition->sector_size     = selected_partition.sector_size;
	//Handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required.
	new_partition->set_sector_usage(
			(   ( ( copied_partition .sectors_used + copied_partition .sectors_unused ) * copied_partition .sector_size )
			  + ( selected_partition .sector_size - 1 )
			) / selected_partition .sector_size,
			(   ( copied_partition .sectors_unused * copied_partition .sector_size )
			  + ( selected_partition .sector_size - 1 )
			) / selected_partition .sector_size ) ;

	this ->show_all_children() ;
}

const Partition & Dialog_Partition_Copy::Get_New_Partition()
{
	g_assert( new_partition != NULL );  // Bug: Not initialised by constructor calling set_data()

	//first call baseclass to get the correct new partition
	Dialog_Base_Partition::prepare_new_partition();

	//set proper name and status for partition
	new_partition->status = STAT_COPY;

	return *new_partition;
}

} //GParted
