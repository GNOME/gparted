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

#include "../include/OperationFormat.h"

namespace GParted
{

OperationFormat::OperationFormat( const Device & device,
				  const Partition & partition_orig,
				  const Partition & partition_new )
{
	type = GParted::FORMAT ;

	this ->device = device ;
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
}
	
void OperationFormat::apply_to_visual( std::vector<Partition> & partitions ) 
{
	if ( partition_original .inside_extended )
	{
		index_extended = find_index_extended( partitions ) ;
		
		if ( index_extended >= 0 )
			index = find_index_original( partitions[ index_extended ] .logicals ) ;

		if ( index >= 0 )
			partitions[ index_extended ] .logicals[ index ] = partition_new ;
	}
	else
	{
		index = find_index_original( partitions ) ;

		if ( index >= 0 )
			partitions[ index ] = partition_new ;
	}
}

void OperationFormat::create_description() 
{
	/*TO TRANSLATORS: looks like  Format /dev/hda4 as linux-swap */
	description = String::ucompose( _("Format %1 as %2"),
					partition_original .get_path(),
					Utils::get_filesystem_string( partition_new .filesystem ) ) ;
}

} //GParted

