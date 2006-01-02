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
#include "../include/Operation.h"

namespace GParted
{
	
Operation::Operation( const Device & device, const Partition & partition_original, const Partition & partition_new, OperationType operationtype )
{
	this ->device = device ;
	this ->partition_original = partition_original;
	this ->partition_new = partition_new;
	this ->operationtype = operationtype;
	
	str_operation = Get_String( ) ;
	
	if ( operationtype == COPY )
	{
		copied_partition_path = partition_new .partition ;
		this ->partition_new .partition = String::ucompose( _("copy of %1"), this ->partition_new .partition );	
	}
}

Glib::ustring Operation::Get_String( )
{
	Glib::ustring temp ;
	Sector diff ;
		
	switch ( operationtype )
	{
		case DELETE	:
			if (partition_original.type == GParted::TYPE_LOGICAL)
				temp = _("Logical Partition") ;
			else
				temp = partition_original .partition ;

			/*TO TRANSLATORS: looks like   Delete /dev/hda2 (ntfs, 2345 MB) from /dev/hda */
			return String::ucompose( _("Delete %1 (%2, %3 MB) from %4"), 
						 temp,
						 Utils::Get_Filesystem_String( partition_original .filesystem ), 
						 partition_original .Get_Length_MB( ), 
						 device .path ) ;

		case CREATE	:
			switch( partition_new.type )
			{
				case GParted::TYPE_PRIMARY	:
					temp = _("Primary Partition");
					break;
				case GParted::TYPE_LOGICAL	:
					temp = _("Logical Partition") ;
					break;	
				case GParted::TYPE_EXTENDED	:
					temp = _("Extended Partition");
					break;
			
				default	:
					break;
			}
			/*TO TRANSLATORS: looks like   Create Logical Partition #1 (ntfs, 2345 MB) on /dev/hda */
			return String::ucompose( _("Create %1 #%2 (%3, %4 MB) on %5"),
						 temp, 
						 partition_new.partition_number, 
						 Utils::Get_Filesystem_String( partition_new.filesystem ), 
						 partition_new .Get_Length_MB( ), 
						 device .path ) ;
			
		case RESIZE_MOVE:
			//if startsector has changed >= 1 MB we consider it a move
			diff = std::abs( partition_new .sector_start - partition_original .sector_start ) ;
			if (  diff >= MEGABYTE ) 
			{
				if ( partition_new .sector_start > partition_original .sector_start )
					temp = String::ucompose( _("Move %1 forward by %2 MB"), partition_new.partition, Utils::Sector_To_MB( diff ) ) ;
				else
					temp = String::ucompose( _("Move %1 backward by %2 MB"), partition_new.partition, Utils::Sector_To_MB( diff ) ) ;
			}
									
			//check if size has changed ( we only consider changes >= 1 MB )
			diff = std::abs( (partition_original .sector_end - partition_original .sector_start) - (partition_new .sector_end - partition_new .sector_start)  ) ;
											
			if ( diff >= MEGABYTE )
			{
				if ( temp .empty( ) ) 
					temp = String::ucompose( _("Resize %1 from %2 MB to %3 MB"), 
								 partition_new.partition,
								 partition_original .Get_Length_MB(),
								 partition_new .Get_Length_MB() ) ;
				else
					temp += " " + String::ucompose( _("and Resize %1 from %2 MB to %3 MB"),
									partition_new.partition,
									partition_original .Get_Length_MB(),
									partition_new .Get_Length_MB() ) ;
			}
			
			if ( temp .empty( ) )
				temp = _("Sorry, changes are too small to make sense");
					
			return temp;
					
		case FORMAT	:
			/*TO TRANSLATORS: looks like  Format /dev/hda4 as linux-swap */
			return String::ucompose( _("Format %1 as %2"),
						 partition_original .partition,
						 Utils::Get_Filesystem_String( partition_new .filesystem ) ) ;
			
		case COPY	:
			/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd (start at 2500 MB) */
			return String::ucompose( _("Copy %1 to %2 (start at %3 MB)"),
						 partition_new .partition,
						 device .path,
						 Utils::Sector_To_MB( partition_new .sector_start ) ) ;
			
		default		:
			return "";			
	}
		
}

void Operation::Apply_Operation_To_Visual( std::vector<Partition> & partitions )
{
	switch ( operationtype )
	{
		case DELETE	:	Apply_Delete_To_Visual( partitions ) ;		break ;
		case RESIZE_MOVE:	Apply_Resize_Move_To_Visual( partitions );	break ;
		case CREATE	:
		case FORMAT	:
		case COPY	:	Apply_Create_To_Visual( partitions ); 		break ;
	}
}

void Operation::Insert_Unallocated( std::vector<Partition> & partitions, Sector start, Sector end, bool inside_extended )
{
	Partition UNALLOCATED ;
	UNALLOCATED .Set_Unallocated( device .path, 0, 0, inside_extended ) ;
	
	//if there are no partitions at all..
	if ( partitions .empty( ) )
	{
		UNALLOCATED .sector_start = start ;
		UNALLOCATED .sector_end = end ;
		
		partitions .push_back( UNALLOCATED );
		
		return ;
	}
		
	//start <---> first partition start
	if ( (partitions .front( ) .sector_start - start) >= MEGABYTE )
	{
		UNALLOCATED .sector_start = start ;
		UNALLOCATED .sector_end = partitions .front( ) .sector_start -1 ;
		
		partitions .insert( partitions .begin( ), UNALLOCATED );
	}
	
	//look for gaps in between
	for ( unsigned int t =0 ; t < partitions .size( ) -1 ; t++ )
		if ( ( partitions[ t +1 ] .sector_start - partitions[ t ] .sector_end ) >= MEGABYTE )
		{
			UNALLOCATED .sector_start = partitions[ t ] .sector_end +1 ;
			UNALLOCATED .sector_end = partitions[ t +1 ] .sector_start -1 ;
		
			partitions .insert( partitions .begin( ) + ++t, UNALLOCATED );
		}
		
	//last partition end <---> end
	if ( (end - partitions .back( ) .sector_end ) >= MEGABYTE )
	{
		UNALLOCATED .sector_start = partitions .back( ) .sector_end +1 ;
		UNALLOCATED .sector_end = end ;
		
		partitions .push_back( UNALLOCATED );
	}
}

int Operation::Get_Index_Original( std::vector<Partition> & partitions )
{
	for ( int t = 0 ; t < static_cast<int>( partitions .size( ) ) ; t++ )
		if ( partition_original .sector_start >= partitions[ t ] .sector_start && partition_original .sector_end <= partitions[ t ] .sector_end )
		{
			//remove unallocated space preceding the original partition
			if ( t -1 >= 0 && partitions[ t -1 ] .type == GParted::TYPE_UNALLOCATED ) 
				partitions .erase( partitions .begin( ) + --t );
						
			//remove unallocated space following the original partition
			if ( t +1 < static_cast<int>( partitions .size( ) ) && partitions[ t +1 ] .type == GParted::TYPE_UNALLOCATED )
				partitions .erase( partitions .begin( ) + t +1 );
			
			return t ;
		}
	
	return -1 ; 
}

int Operation::get_index_extended( const std::vector<Partition> & partitions ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
		if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			return t ;

	return -1 ;
}
	
void Operation::Apply_Delete_To_Visual( std::vector<Partition> & partitions )
{
	if ( ! partition_original .inside_extended )
	{
		partitions .erase( partitions .begin( ) + Get_Index_Original( partitions ) );
		
		Insert_Unallocated( partitions, 0, device .length -1, false ) ;
	}
	else
	{
		unsigned int ext = get_index_extended( partitions ) ;
		partitions[ ext ] .logicals .erase( partitions[ ext ] .logicals .begin( ) + Get_Index_Original( partitions[ ext ] .logicals ) );
		
		//if deleted partition was logical we have to decrease the partitionnumbers of the logicals
		//with higher numbers by one (only if its a real partition)
		if ( partition_original .status != GParted::STAT_NEW )
			for ( unsigned int t = 0 ; t < partitions[ ext ] .logicals .size( ) ; t++ )
				if ( partitions[ ext ] .logicals[ t ] .partition_number > partition_original .partition_number )
					partitions[ ext ] .logicals[ t ] .Update_Number( partitions[ ext ] .logicals[ t ] .partition_number -1 );
				
				
		Insert_Unallocated( partitions[ ext ] .logicals, partitions[ ext ] .sector_start, partitions[ ext ] .sector_end, true ) ;
	}
}

void Operation::Apply_Create_To_Visual( std::vector<Partition> & partitions )
{ 
	if ( ! partition_original .inside_extended )
	{
		partitions[ Get_Index_Original( partitions ) ] = partition_new ;
				
		Insert_Unallocated( partitions, 0, device .length -1, false ) ;
	}
	else
	{
		unsigned int ext = get_index_extended( partitions ) ;
		partitions[ ext ] .logicals[ Get_Index_Original( partitions[ ext ] .logicals ) ] = partition_new ;
		
		Insert_Unallocated( partitions[ ext ] .logicals, partitions[ ext ] .sector_start, partitions[ ext ] .sector_end, true ) ;
	}
}

void Operation::Apply_Resize_Move_To_Visual( std::vector<Partition> & partitions)
{
	if ( partition_original .type == GParted::TYPE_EXTENDED )
		Apply_Resize_Move_Extended_To_Visual( partitions ) ;
	else 
		Apply_Create_To_Visual( partitions ) ;
}

void Operation::Apply_Resize_Move_Extended_To_Visual( std::vector<Partition> & partitions )
{ 
	//stuff OUTSIDE extended partition
	unsigned int ext = Get_Index_Original( partitions ) ;
	partitions[ ext ] .sector_start = partition_new .sector_start ;
	partitions[ ext ] .sector_end = partition_new .sector_end ;
	
	Insert_Unallocated( partitions, 0, device .length -1, false ) ;
	
	//stuff INSIDE extended partition
	ext = get_index_extended( partitions ) ;
	
	if ( partitions[ ext ] .logicals .size( ) && partitions[ ext ] .logicals .front( ) .type == GParted::TYPE_UNALLOCATED )
		partitions[ ext ] .logicals .erase( partitions[ ext ] .logicals .begin( ) ) ;
	
	if ( partitions[ ext ] .logicals .size( ) && partitions[ ext ] .logicals .back( ) .type == GParted::TYPE_UNALLOCATED )
		partitions[ ext ] .logicals .erase( partitions[ ext ] .logicals .end( ) -1 ) ;
	
	Insert_Unallocated( partitions[ ext ] .logicals, partitions[ ext ] .sector_start, partitions[ ext ] .sector_end, true ) ;
}

} //GParted
