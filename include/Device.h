/*	Copyright (C) 2004 Bart
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
 
#ifndef DEVICE
#define DEVICE

#include "../include/Partition.h"

namespace GParted
{
	
class Device
{
	 
public:
	Device() ;
	~Device() ;
	
	void Reset() ;
	
	std::vector<Partition> partitions ;
	Sector length;
	long heads ;
	long sectors ;
	long cylinders ;
	Sector cylsize ;
	Glib::ustring model;
 	Glib::ustring path;
 	Glib::ustring realpath;
 	Glib::ustring disktype;
	int max_prims ;
	int highest_busy ;
	bool readonly ; 
			
private:
		

};
 
} //GParted
 
#endif //DEVICE
