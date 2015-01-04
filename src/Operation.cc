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
#include "../include/Operation.h"

namespace GParted
{

Operation::Operation()
{
}
	
int Operation::find_index_original( const std::vector<Partition> & partitions ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
		if ( partition_original .sector_start >= partitions[ t ] .sector_start &&
		     partition_original .sector_end <= partitions[ t ] .sector_end )
			return t ;

	return -1 ;
}

int Operation::find_index_extended( const std::vector<Partition> & partitions ) 
{
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
		if ( partitions[ t ] .type == GParted::TYPE_EXTENDED )
			return t ;

	return -1 ;
}

void Operation::insert_unallocated( std::vector<Partition> & partitions, Sector start, Sector end, Byte_Value sector_size, bool inside_extended )
{
	Partition UNALLOCATED ;
	UNALLOCATED.Set_Unallocated( device.get_path(), false, 0LL, 0LL, sector_size, inside_extended );
	
	//if there are no partitions at all..
	if ( partitions .empty() )
	{
		UNALLOCATED .sector_start = start ;
		UNALLOCATED .sector_end = end ;
		
		partitions .push_back( UNALLOCATED );
		
		return ;
	}
		
	//start <---> first partition start
	if ( (partitions .front() .sector_start - start) > (MEBIBYTE / sector_size) )
	{
		UNALLOCATED .sector_start = start ;
		UNALLOCATED .sector_end = partitions .front() .sector_start -1 ;
		
		partitions .insert( partitions .begin(), UNALLOCATED );
	}
	
	//look for gaps in between
	for ( unsigned int t =0 ; t < partitions .size() -1 ; t++ )
	{
		if (   ( ( partitions[ t + 1 ] .sector_start - partitions[ t ] .sector_end - 1 ) > (MEBIBYTE / sector_size) )
		    || (   ( partitions[ t + 1 ] .type != TYPE_LOGICAL )  // Only show exactly 1 MiB if following partition is not logical.
		        && ( ( partitions[ t + 1 ] .sector_start - partitions[ t ] .sector_end - 1 ) == (MEBIBYTE / sector_size) )
		       )
		   )
		{
			UNALLOCATED .sector_start = partitions[ t ] .sector_end +1 ;
			UNALLOCATED .sector_end = partitions[ t +1 ] .sector_start -1 ;

			partitions .insert( partitions .begin() + ++t, UNALLOCATED );
		}
	}

	//last partition end <---> end
	if ( (end - partitions .back() .sector_end ) >= (MEBIBYTE / sector_size) )
	{
		UNALLOCATED .sector_start = partitions .back() .sector_end +1 ;
		UNALLOCATED .sector_end = end ;
		
		partitions .push_back( UNALLOCATED );
	}
}

} //GParted
