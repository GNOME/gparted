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
	
Operation::Operation( Device *device, Device *source_device, const Partition & partition_original, const Partition & partition_new, OperationType operationtype )
{
	this ->device = device;
	this ->device_path = device ->Get_Path( ) ;
	
	//this is only used when operationtype == COPY
	this ->source_device = source_device;
	this ->source_device_path = source_device ->Get_Path( ) ;
	
	this ->partition_original = partition_original;
	this ->partition_new = partition_new;
	this ->operationtype = operationtype;
	
	str_operation = Get_String( ) ;
	
	//not the nicest place to put this, but definetly the most efficient :)
	if ( operationtype == COPY )
		this ->partition_new .partition = String::ucompose( _("copy of %1"), this ->partition_new .partition );	
	
}


Glib::ustring Operation::Get_String( )
{
	Glib::ustring temp ;
	Sector diff ;
		
	switch ( operationtype )
	{
		case DELETE	:	if (partition_original.type == GParted::LOGICAL)
						temp = _("Logical Partition") ;
					else
						temp = partition_original .partition ;

					/*TO TRANSLATORS: looks like   Delete /dev/hda2 (ntfs, 2345 MB) from /dev/hda */
					return String::ucompose( _("Delete %1 (%2, %3 MB) from %4"), temp, partition_original .filesystem, partition_original .Get_Length_MB(), device ->Get_Path() ) ;

		case CREATE	:	switch( partition_new.type )
					{
						case GParted::PRIMARY	:	temp = _("Primary Partition"); break;
						case GParted::LOGICAL	:	temp = _("Logical Partition") ; break;	
						case GParted::EXTENDED	:	temp = _("Extended Partition"); break;
						default			:	break;
					}
					/*TO TRANSLATORS: looks like   Create Logical Partition #1 (ntfs, 2345 MB) on /dev/hda */
					return String::ucompose( _("Create %1 #%2 (%3, %4 MB) on %5"), temp, partition_new.partition_number, partition_new.filesystem , partition_new .Get_Length_MB(), device ->Get_Path() ) ;
		case RESIZE_MOVE:	//if startsector has changed >= 1 MB we consider it a move
					diff = Abs( partition_new .sector_start - partition_original .sector_start ) ;
					if (  diff >= MEGABYTE ) 
					{
						if ( partition_new .sector_start > partition_original .sector_start )
							temp = String::ucompose( _("Move %1 forward by %2 MB"), partition_new.partition, Sector_To_MB( diff ) ) ;
						else
							temp = String::ucompose( _("Move %1 backward by %2 MB"), partition_new.partition, Sector_To_MB( diff ) ) ;
					}
									
					//check if size has changed ( we only consider changes >= 1 MB )
					diff = Abs( (partition_original .sector_end - partition_original .sector_start) - (partition_new .sector_end - partition_new .sector_start)  ) ;
												
					if ( diff >= MEGABYTE )
					{
						if ( temp .empty( ) ) 
							temp = String::ucompose( _("Resize %1 from %2 MB to %3 MB"), partition_new.partition,  partition_original .Get_Length_MB(), partition_new .Get_Length_MB() ) ;
						else
							temp += " " + String::ucompose( _("and Resize %1 from %2 MB to %3 MB"), partition_new.partition,  partition_original .Get_Length_MB(), partition_new .Get_Length_MB() ) ;
					}
					if ( temp .empty( ) )
						temp = _("Sorry, changes are too small to make sense");
					
					return temp;
		case CONVERT	:	/*TO TRANSLATORS: looks like  Convert /dev/hda4 from ntfs to linux-swap */
					return String::ucompose( _( "Convert %1 from %2 to %3"), partition_original .partition, partition_original .filesystem, partition_new .filesystem ) ;
		case COPY	:	/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd (start at 2500 MB) */
					return String::ucompose( _("Copy %1 to %2 (start at %3 MB)"), partition_new .partition,  device ->Get_Path( ), Sector_To_MB( partition_new .sector_start ) ) ;
		default		:	return "";			
	}
		
}


void Operation::Apply_Operation_To_Visual( std::vector<Partition> & partitions )
{
	switch ( operationtype )
	{
		case DELETE	:	Apply_Delete_To_Visual( partitions ) ;		break ;
		case RESIZE_MOVE:	Apply_Resize_Move_To_Visual( partitions );	break ;
		case CREATE	:
		case CONVERT	:
		case COPY	:	Apply_Create_To_Visual( partitions ); 		break ;
	}
}

void Operation::Apply_To_Disk( PedTimer * timer )
{ 
	Glib::ustring buf;
	bool result; //for some weird reason it won't work otherwise .. ( this one really beats me :-S )

	switch ( operationtype )
	{
		case DELETE	:	result = device ->Delete_Partition( partition_original ) ;
					if ( ! result )
						Show_Error( String::ucompose( _("Error while deleting %1"), partition_original.partition ) ) ;
														
					break;
		case CREATE	:	result = device ->Create_Partition_With_Filesystem( partition_new, timer ) ;
					if ( ! result ) 
						Show_Error( String::ucompose( _("Error while creating %1"), partition_new.partition ) );
											
					break;
		case RESIZE_MOVE:	result = device ->Move_Resize_Partition( partition_original, partition_new, timer ) ;
					if ( ! result ) 
						Show_Error( String::ucompose( _("Error while resizing/moving %1"), partition_new.partition ) ) ;
											
					break;
		case CONVERT	:	result = device ->Set_Partition_Filesystem( partition_new, timer ) ;
					if ( ! result ) 
						Show_Error( String::ucompose( _("Error while converting filesystem of %1"), partition_new.partition ) ) ;
										
					break;
		case COPY	:	result = device ->Copy_Partition( source_device, partition_new, timer ) ;
					if ( ! result ) 
						Show_Error( String::ucompose( _("Error while copying %1"), partition_new .partition ) ) ;
									
					break;
	}
	
}

void Operation::Insert_Unallocated( std::vector<Partition> & partitions, Sector start, Sector end )
{
	Partition UNALLOCATED ;
	UNALLOCATED .Set_Unallocated( 0, 0, partitions .front( ) .inside_extended ) ;
	
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
	for ( int t = 0 ; t < (int)partitions .size( ) ; t++ )
		if ( partition_original .sector_start >= partitions[ t ] .sector_start && partition_original .sector_end <= partitions[ t ] .sector_end )
		{
			//remove unallocated space preceding the original partition
			if ( t -1 >= 0 && partitions[ t -1 ] .type == GParted::UNALLOCATED ) 
				partitions .erase( partitions .begin( ) + --t );
						
			//remove unallocated space following the original partition
			if ( t +1 < (int)partitions .size( ) && partitions[ t +1 ] .type == GParted::UNALLOCATED )
				partitions .erase( partitions .begin( ) + t +1 );
			
			return t ;
		}
	
	return -1 ; 
}

void Operation::Apply_Delete_To_Visual( std::vector<Partition> & partitions )
{
	if ( ! partition_original .inside_extended )
	{
		partitions .erase( partitions .begin( ) + Get_Index_Original( partitions ) );
		
		Insert_Unallocated( partitions, 0, device ->Get_Length( ) -1 ) ;
	}
	else
	{
		//unsigned int ext = Get_Index_Extended( partitions ) ;
		unsigned int ext = 0 ;
		while ( ext < partitions .size( ) && partitions[ ext ] .type != GParted::EXTENDED ) ext++ ;
		partitions[ ext ] .logicals .erase( partitions[ ext ] .logicals .begin( ) + Get_Index_Original( partitions[ ext ] .logicals ) );
		
		
		//if deleted partition was logical we have to decrease the partitionnumbers of the logicals with higher numbers by one (only if its a real partition)
		if ( partition_original .status != GParted::STAT_NEW )
			for ( unsigned int t = 0 ; t < partitions[ ext ] .logicals .size( ) ; t++ )
				if ( partitions[ ext ] .logicals[ t ] .partition_number > partition_original .partition_number )
					partitions[ ext ] .logicals[ t ] .Update_Number( partitions[ ext ] .logicals[ t ] .partition_number -1 );
				
				
		Insert_Unallocated( partitions[ ext ] .logicals, partitions[ ext ] .sector_start, partitions[ ext ] .sector_end ) ;
	}
}

void Operation::Apply_Create_To_Visual( std::vector<Partition> & partitions )
{ 
	if ( ! partition_original .inside_extended )
	{
		partitions[ Get_Index_Original( partitions ) ] = partition_new ;
				
		Insert_Unallocated( partitions, 0, device ->Get_Length( ) -1 ) ;
	}
	else
	{
		unsigned int ext = 0 ;
		while ( ext < partitions .size( ) && partitions[ ext ] .type != GParted::EXTENDED ) ext++ ;
		partitions[ ext ] .logicals[ Get_Index_Original( partitions[ ext ] .logicals ) ] = partition_new ;
		
		Insert_Unallocated( partitions[ ext ] .logicals, partitions[ ext ] .sector_start, partitions[ ext ] .sector_end ) ;
	}
}

void Operation::Apply_Resize_Move_To_Visual( std::vector<Partition> & partitions)
{
	if ( partition_original .type == GParted::EXTENDED )
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
	
	Insert_Unallocated( partitions, 0, device ->Get_Length( ) -1 ) ;
	
	//stuff INSIDE extended partition
	ext = 0 ;
	while ( ext < partitions .size( ) && partitions[ ext ] .type != GParted::EXTENDED ) ext++ ;
	
	if ( partitions[ ext ] .logicals .size( ) && partitions[ ext ] .logicals .front( ) .type == GParted::UNALLOCATED )
		partitions[ ext ] .logicals .erase( partitions[ ext ] .logicals .begin( ) ) ;
	
	if ( partitions[ ext ] .logicals .size( ) && partitions[ ext ] .logicals .back( ) .type == GParted::UNALLOCATED )
		partitions[ ext ] .logicals .erase( partitions[ ext ] .logicals .end( ) -1 ) ;
	
	Insert_Unallocated( partitions[ ext ] .logicals, partitions[ ext ] .sector_start, partitions[ ext ] .sector_end ) ;
}

void Operation::Show_Error( Glib::ustring message ) 
{
	message = "<span weight=\"bold\" size=\"larger\">" + message + "</span>\n\n" ;
	message += _( "Be aware that the failure to apply this operation could affect other operations on the list." ) ;
	Gtk::MessageDialog dialog( message ,true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true );
	
	gdk_threads_enter( );
	dialog .run( );
	gdk_threads_leave( );
}


} //GParted
