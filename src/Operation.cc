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
	
Operation::Operation( Device *device, Device *source_device, const Partition & partition_original , const Partition & partition_new , OperationType operationtype)
{
	this->device = device;
	this->device_path = device ->Get_Path() ;
	
	//this is only used when operationtype == COPY
	this->source_device = source_device;
	this->source_device_path = source_device ->Get_Path() ;
	
	this->partition_original = partition_original;
	this->partition_new = partition_new;
	this->operationtype = operationtype;
	
	str_operation = Get_String() ;
	
	//not the nicest place to put this, but definetly the most efficient :)
	if ( operationtype == COPY )
		this->partition_new .partition = String::ucompose( _("copy of %1"), this->partition_new .partition );	
	
}


Glib::ustring Operation::Get_String()
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
						if ( temp .empty() ) 
							temp = String::ucompose( _("Resize %1 from %2 MB to %3 MB"), partition_new.partition,  partition_original .Get_Length_MB(), partition_new .Get_Length_MB() ) ;
						else
							temp += " " + String::ucompose( _("and Resize %1 from %2 MB to %3 MB"), partition_new.partition,  partition_original .Get_Length_MB(), partition_new .Get_Length_MB() ) ;
					}
					if ( temp .empty() )
						temp = _("Sorry, changes are too small to make sense");
					
					return temp;
		case CONVERT	:	/*TO TRANSLATORS: looks like  Convert /dev/hda4 from ntfs to linux-swap */
					return String::ucompose( _( "Convert %1 from %2 to %3"), partition_original .partition, partition_original .filesystem, partition_new .filesystem ) ;
		case COPY	:	/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd (start at 2500 MB) */
					return String::ucompose( _("Copy %1 to %2 (start at %3 MB)"), partition_new .partition,  device ->Get_Path(), Sector_To_MB( partition_new .sector_start ) ) ;
		default		:	return "";			
	}
		
}


std::vector<Partition> Operation::Apply_Operation_To_Visual( std::vector<Partition> & partitions )
{
	switch ( operationtype )
	{
		case DELETE	:	return Apply_Delete_To_Visual( partitions );
		case CREATE	:	return Apply_Create_To_Visual( partitions );
		case RESIZE_MOVE:	return Apply_Resize_Move_To_Visual( partitions );	
		case CONVERT	:   	return Apply_Convert_To_Visual( partitions ) ;
		case COPY	:	return Apply_Create_To_Visual( partitions );
	}
	
	return partitions;
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

std::vector<Partition> Operation::Apply_Delete_To_Visual( std::vector<Partition> & partitions )
{
	unsigned int t ;
	
	for ( t=0;t<partitions.size();t++)
	{
		//since extended can only be deleted if there are no logicals left, we know for sure the next element in partitions is unallocated space
		//we simply remove the extended one and set the inside_extended flag from the unallocated space to false
		if (this ->partition_original.type == GParted::EXTENDED && partitions[t].type == GParted::EXTENDED ) 
		{
			partitions[t+1].inside_extended = false;
			partitions.erase( partitions.begin() + t );
			break;//we're done here ;-)
		}
		
		else if ( partitions[t].sector_start == this-> partition_original.sector_start )
		{	
			partitions[t].Set_Unallocated( partition_original.sector_start , partition_original.sector_end, partition_original.inside_extended );
			break;//we're done here ;-)
		}
	}
	 
	
	//if deleted partition was logical we have to decrease the partitionnumbers of the logicals with higher numbers by one (only if its a real partition)
	if ( partition_original.type == GParted::LOGICAL && partition_original .status != GParted::STAT_NEW )
		for ( t=0;t<partitions.size() ;t++)
			if ( partitions[t].type == GParted::LOGICAL && partitions[t].partition_number > partition_original.partition_number  )
				partitions[t].Update_Number( partitions[t].partition_number -1 );
	
	
	//now we merge separate unallocated spaces next to each other together FIXME: more performance if we only check t -1 and t +1
	for ( t=0;t<partitions.size() -1;t++)
	{
		if ( partitions[t].type == GParted::UNALLOCATED && partitions[t +1].type == GParted::UNALLOCATED && partitions[t].inside_extended == partitions[t +1].inside_extended )
		{
			partitions[t].sector_end = partitions[t+1].sector_end;
			partitions.erase( partitions.begin() + t + 1 );
			t--;
		}
	}
	
	return partitions;
}

std::vector<Partition> Operation::Apply_Create_To_Visual( std::vector<Partition> & partitions )
{ 
	Partition partition_temp;
		
	//find the original partition and replace it with the new one (original "partition" is unallocated space of course)
	unsigned int t;
	for ( t=0;t<partitions.size() ;t++)
	{ 
	//since the stored partition_original might have been changed (e.g. deletion of virtual partition) we look for the partition which contains the 
	//startsector of te new one and set this partition as the original one. 
		if ( partition_new.sector_start >= partitions[t].sector_start && partition_new.sector_start <= partitions[t].sector_end && partitions[t].type != GParted::EXTENDED )
		{ 
			partition_original = partitions[t] ;
			partitions.erase( partitions.begin() + t );
			partitions.insert( partitions.begin() + t, partition_new  );
			break;
		}
		
	}
	
		
	//check borders and create new elements if necessary
	//BEFORE new partition
	if ( (partition_new.sector_start - partition_original.sector_start) >= MEGABYTE  ) //at least 1 MB between begin new partition and begin original one
	{
		partition_temp.Set_Unallocated(  partition_original.sector_start, partition_new.sector_start -1 , partition_new.inside_extended );
		partitions.insert( partitions.begin() + t, partition_temp );
		t++;
	}
	
	//AFTER new partition
	if ( ( partition_original.sector_end - partition_new.sector_end) >= MEGABYTE  ) //at least 1 MB between end new partition and end original one
	{
		partition_temp.Set_Unallocated(  partition_new.sector_end +1, partition_original.sector_end ,partition_new.inside_extended  );
		partitions.insert( partitions.begin() + t +1, partition_temp );
	}
	
	//if new created partition is an extended partition, we need to add unallocated space of the same size to the list
	if ( partition_new.type == GParted::EXTENDED )
	{
		partition_temp.Set_Unallocated(  partition_new.sector_start, partition_new.sector_end , true );	
		partitions.insert( partitions.begin() + t +1, partition_temp );
	}
	
	return partitions;
}

std::vector<Partition> Operation::Apply_Resize_Move_To_Visual( std::vector<Partition> & partitions)
{
	//extended handling is so different i decided to take it apart from normal handling
	if ( partition_original.type == GParted::EXTENDED )
		return Apply_Resize_Move_Extended_To_Visual( partitions ) ;
	
	//normal partitions ( Primary and Logical )
	Partition partition_temp;
	
	unsigned int t;
	for ( t=0;t<partitions.size() ;t++)
	{ 
	//since the stored partition_original might have been changed (e.g. deletion of virtual partition) we look for the partition which contains the 
	//startsector of the original one 
		if ( partition_original.sector_start >= partitions[t].sector_start && partition_original.sector_start <= partitions[t].sector_end && partitions[t].type != GParted::EXTENDED )
		{
			partition_original = partitions[t] ;
			partitions[t].sector_start = partition_new.sector_start ;
			partitions[t].sector_end = partition_new.sector_end ;
			partitions[t].sectors_unused = partition_new.sectors_unused ;
			break;
		}
	}
	
	//determine correct perimeters of moving/resizing space
	Sector START	= partition_original.sector_start ;
	Sector END	= partition_original.sector_end ;
	
	
	//now we have the index of the original partition we can remove the surrounding partitions ( if UNALLOCATED and inside_extended is the same )
	if ( t > 0 && partitions[t -1].type == GParted::UNALLOCATED && partitions[t -1].inside_extended == partition_new.inside_extended  )
	{
		START = partitions[t -1] .sector_start ;
		partitions.erase( partitions.begin() + t -1 );
		t-- ;
	}
			
	if ( t +1 < partitions.size() && partitions[t +1].type == GParted::UNALLOCATED && partitions[t +1].inside_extended == partition_new.inside_extended)
	{
		END =  partitions[t +1] .sector_end ;
		partitions.erase( partitions.begin() + t +1 );
	}
		
	//now look for unallocated space > 1 MB and add to the list
	if ( (partition_new.sector_start - START) >= MEGABYTE ) 
	{ 
		partition_temp.Set_Unallocated(  START, partition_new.sector_start -1 , partition_new.inside_extended );	
		partitions.insert( partitions.begin() + t , partition_temp );
		t++;
	}
	
	if ( (END - partition_new.sector_end) >= MEGABYTE ) 
	{ 
		partition_temp.Set_Unallocated(  partition_new.sector_end +1, END , partition_new.inside_extended );	
		partitions.insert( partitions.begin() + t +1, partition_temp );
	}
	
	
	return partitions;
}

std::vector<Partition> Operation::Apply_Resize_Move_Extended_To_Visual( std::vector<Partition> & partitions )
{
	unsigned int t, INDEX =0;
		
	//look for index of partition and set the extended partition's new size.
	for ( t=0;t<partitions.size() ;t++)
	{ 
		if ( partitions[t].type == GParted::EXTENDED )
		{
			partition_original = partitions[t] ;
			partitions[t].sector_start = partition_new.sector_start ;
			partitions[t].sector_end = partition_new.sector_end ;
			INDEX = t ;
				
			break;
		}
	}
	
	
	/* remove all relevant unallocated spaces...
	 * E.G. partitions looks like: P-P-U-E-U-L-L-U-U <--- last U is OUTSIDE extended!!
	 *                                                   1-2 - 3- 4- 5 -6-7- 8 - 9
	 * now we check 3, 5, 8 and 9 . If any of them is unallocated we remove it. In this case all of them.. :)
	 */
	
	//check 3
	if ( partitions[ INDEX -1 ] .type == GParted::UNALLOCATED )
		partitions .erase( partitions .begin() + --INDEX ) ;
	
	//check 5
	if ( partitions[ INDEX +1 ] .type == GParted::UNALLOCATED )
		partitions .erase( partitions .begin() + INDEX +1 ) ;
	
	//find index of last logic
	for ( t = partitions.size() -1 ; ! partitions[ t ] .inside_extended && t > INDEX ; t-- ) {}
	
	//check 8
	if ( t > INDEX && partitions[ t ] .type == GParted::UNALLOCATED )
		partitions .erase( partitions .begin() + t ) ;
	else
		t++ ;
		
	//check 9
	if ( t < partitions .size( ) && partitions[ t ] .type == GParted::UNALLOCATED )
		partitions .erase( partitions .begin() + t ) ;
		
		
	
	//and insert new unallocated spaces at fitting places
	Sector TEMP ;
	Partition partition_temp ;
	
	//check 3
	TEMP = INDEX > 0 ? partitions[ INDEX -1 ] .sector_end +1 : 0 ;
	if ( ( partitions[ INDEX ] .sector_start - TEMP ) > MEGABYTE )
	{
		partition_temp.Set_Unallocated( TEMP, partitions[ INDEX ] .sector_start -1, false ) ;
		partitions.insert( partitions.begin() + INDEX, partition_temp );
		INDEX++;
	}
	
	//check 5
	if ( (INDEX +1) < partitions .size() && partitions[ INDEX +1 ] .type == GParted::LOGICAL )
		TEMP = partitions[ INDEX +1 ] .sector_start -1 ;
	else
		TEMP = partition_new .sector_end ;
	
	if ( ( TEMP - partitions[ INDEX ] .sector_start ) > MEGABYTE )
	{
		partition_temp.Set_Unallocated( partitions[ INDEX ] .sector_start, TEMP, true ) ;
		partitions.insert( partitions.begin() +INDEX +1, partition_temp );
	}
	
	
	//find index of last logic
	for ( t = partitions .size( ) -1 ; ! partitions[ t ] .inside_extended && t > INDEX ; t-- ) { }
	
	//check 8
	if ( partitions[ t ] .type != GParted::UNALLOCATED && ( partition_new .sector_end - partitions[ t ] .sector_end ) > MEGABYTE )
	{
		partition_temp.Set_Unallocated( partitions[ t ] .sector_end +1, partition_new .sector_end, true ) ;	
		partitions.insert( partitions.begin() +t +1, partition_temp );
		t++ ;
	}
	
	//check 9
	t++ ;
	TEMP = t < partitions .size() ? partitions[ t ] .sector_start -1 : device ->Get_Length() -1;
	
	if ( ( TEMP - partition_new .sector_end ) > MEGABYTE )
	{
		partition_temp.Set_Unallocated( partition_new .sector_end +1, TEMP -1, false ) ;
		partitions.insert( partitions.begin() +t, partition_temp );
	}
	
	
	return partitions ;
}

std::vector<Partition> Operation::Apply_Convert_To_Visual( std::vector<Partition> & partitions )
{
	for ( unsigned int t=0;t<partitions.size() ;t++)
	{ 
	//since the stored partition_original might have been changed (e.g. deletion of virtual partition) we look for the partition which contains the 
	//startsector of the original one 
		if ( partition_original.sector_start >= partitions[t].sector_start && partition_original.sector_start <= partitions[t].sector_end && partitions[t].type != GParted::EXTENDED )
		{	
			partitions[ t ] = partition_new ;
			break;
		}
	}
	
	return partitions;
	
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
