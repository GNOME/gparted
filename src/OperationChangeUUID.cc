/* Copyright (C) 2012 Rogier Goossens
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

#include "../include/OperationChangeUUID.h"

namespace GParted
{

OperationChangeUUID::OperationChangeUUID( const Device & device
                                        , const Partition & partition_orig
                                        , const Partition & partition_new
                                        )
{
	type = OPERATION_CHANGE_UUID ;

	this ->device = device ;
	this ->partition_original = partition_orig ;
	this ->partition_new = partition_new ;
}

void OperationChangeUUID::apply_to_visual( std::vector<Partition> & partitions )
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

void OperationChangeUUID::create_description()
{
	/*TO TRANSLATORS: looks like   Set a new random UUID on ext4 file system on /dev/sda1 */
	description = String::ucompose( _("Set a new random UUID on %1 file system on %2")
	                              , Utils::get_filesystem_string( partition_new .filesystem )
	                              , partition_new .get_path()
	                              ) ;
}

} //GParted
