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
 
#include "../include/Device.h"

namespace GParted
{

Device::Device( )
{
}

bool Device::Get_any_busy( ) 
{
	for ( unsigned int t=0;t<device_partitions.size(); t++ )
		if ( device_partitions[t].busy )
			return true;
		
	return false;
}

int Device::Get_Highest_Logical_Busy( ) 
{
	int highest_logic_busy = -1 ;
	
	for ( unsigned int t = 0 ; t < device_partitions .size( ) ; t++ )
		if ( device_partitions [ t ] .type == GParted::EXTENDED )
		{
			for ( unsigned int i = 0 ; i < device_partitions[ t ] .logicals .size( ) ; i++ )
				if ( device_partitions[ t ] .logicals[ i ] .busy && device_partitions[ t ] .logicals[ i ] .partition_number > highest_logic_busy )
					highest_logic_busy = device_partitions[ t ] .logicals[ i ] .partition_number ;
				
			break ;
		}
	
	return highest_logic_busy ;
}

Device::~Device()
{
}


} //GParted
