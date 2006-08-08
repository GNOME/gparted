/* Copyright (C) 2004 Bart 'plors' Hakvoort
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

#include "../include/OperationCopy.h"

namespace GParted
{

OperationCopy::OperationCopy( const Device & device,
			      const Partition & partition_orig,
			      const Partition & partition_new,
			      const Partition & partition_copied )
{
	type = OPERATION_COPY ;

	this ->device = device ;
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
	this ->partition_copied = partition_copied ;

	this ->partition_new .add_path(  
		String::ucompose( _("copy of %1"), this ->partition_copied .get_path() ), true ) ;
}
	
void OperationCopy::apply_to_visual( std::vector<Partition> & partitions ) 
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

void OperationCopy::create_description() 
{
	if ( partition_original .type == GParted::TYPE_UNALLOCATED )
	{
		/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd (start at 250 MiB) */
		description = String::ucompose( _("Copy %1 to %2 (start at %3)"),
					        partition_copied .get_path(),
					        device .get_path(),
					        Utils::format_size( partition_new .sector_start ) ) ;
	}
	else
	{
		/*TO TRANSLATORS: looks like  Copy /dev/hda4 to /dev/hdd1 */
		description = String::ucompose( _("Copy %1 to %2"),
					        partition_copied .get_path(),
						partition_original .get_path() ) ;
	}
}

} //GParted

