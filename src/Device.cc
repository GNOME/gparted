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
	Reset( ) ;	
}

void Device::Reset( )
{
	device_partitions .clear( ) ;
	length = 0 ;
	heads = sectors = cylinders = cylsize = 0 ;
	model = path = realpath = disktype = "" ;
	max_prims = highest_busy = 0 ;
	readonly = false ; 	
}

Device::~Device( )
{
}


} //GParted
