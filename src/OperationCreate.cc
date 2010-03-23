/* Copyright (C) 2004 Bart 'plors' Hakvoort
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../include/OperationCreate.h"

namespace GParted
{

OperationCreate::OperationCreate( const Device & device,
				  const Partition & partition_orig,
				  const Partition & partition_new )
{
	type = OPERATION_CREATE ;

	this ->device = device ;
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
}
	
void OperationCreate::apply_to_visual( std::vector<Partition> & partitions ) 
{
	index = index_extended = -1 ;

	if ( partition_original .inside_extended )
	{
		index_extended = find_index_extended( partitions ) ;
		
		if ( index_extended >= 0 )
			index = find_index_original( partitions[ index_extended ] .logicals ) ;
		
		if ( index >= 0 )
		{
			partitions[ index_extended ] .logicals[ index ] = partition_new ; 	

			insert_unallocated( partitions[ index_extended ] .logicals,
					    partitions[ index_extended ] .sector_start,
					    partitions[ index_extended ] .sector_end,
				    	    true ) ;
		}
	}
	else
	{
		index = find_index_original( partitions ) ;

		if ( index >= 0 )
		{
			partitions[ index ] = partition_new ;
		
			insert_unallocated( partitions, 0, device .length -1, false ) ;
		}
	}
}

void OperationCreate::create_description() 
{
	switch( partition_new .type )
	{
		case GParted::TYPE_PRIMARY	:
			description = _("Primary Partition");
			break;
		case GParted::TYPE_LOGICAL	:
			description = _("Logical Partition") ;
			break;	
		case GParted::TYPE_EXTENDED	:
			description = _("Extended Partition");
			break;
	
		default	:
			break;
	}
	/*TO TRANSLATORS: looks like   Create Logical Partition #1 (ntfs, 345 MiB) on /dev/hda */
	description = String::ucompose( _("Create %1 #%2 (%3, %4) on %5"),
				 	description, 
					partition_new .partition_number, 
					Utils::get_filesystem_string( partition_new .filesystem ), 
					Utils::format_size( partition_new .get_length(), DEFAULT_SECTOR_SIZE ),
					device .get_path() ) ;
}

} //GParted

