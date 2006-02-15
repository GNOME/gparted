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
 
 /* READ THIS!!
  * Partition isn't really a partition. It's more like a geometry, a continuous part of the disk. 
  * I use it to represent partitions as well as unallocated spaces
  */
 
#ifndef PARTITION
#define PARTITION

#include "../include/Utils.h"

namespace GParted
{
	

enum PartitionType {
	TYPE_PRIMARY		=	0,
	TYPE_LOGICAL		=	1,
	TYPE_EXTENDED		=	2,
	TYPE_UNALLOCATED	=	3 
};

enum PartitionStatus {
	STAT_REAL	=	1,
	STAT_NEW	=	2,
	STAT_COPY	=	3
};

	
class Partition
{
public:
	Partition() ;
	~Partition() ;

	void Reset() ;
	
	//simple Set-functions.  only for convenience, since most members are public
	void Set( 	const Glib::ustring & device_path,
			const Glib::ustring & partition,
			int partition_number,
			PartitionType type,
			FILESYSTEM filesystem,
			Sector sector_start,
			Sector sector_end,
			bool inside_extended,
			bool busy ) ;

	void Set_Unused( Sector sectors_unused ) ;

	void Set_Unallocated( const Glib::ustring & device_path, Sector sector_start, Sector sector_end, bool inside_extended );

	//update partition number (used when a logical partition is deleted) 
	void Update_Number( int new_number );
	
	//FIXME the 3 *MB function are obsolete and should be replaced by Utils::sector_to_unit()
	long Get_Length_MB() const ;
	long Get_Used_MB() const ;
	long Get_Unused_MB() const ;
	Sector get_length() const ;

	bool operator==( const Partition & partition ) const ;
		
	//some public members
	Glib::ustring partition;//the symbolic path (e.g. /dev/hda1 )
	Glib::ustring realpath ;
	Glib::ustring device_path ;
	int partition_number;
	PartitionType type;// UNALLOCATED, PRIMARY, LOGICAL, etc...
	PartitionStatus status; //STAT_REAL, STAT_NEW, etc..
	FILESYSTEM filesystem ;
	Sector sector_start;
	Sector sector_end;
	Sector sectors_used;
	Sector sectors_unused;
	Gdk::Color color;
	bool inside_extended;
	bool busy;
	Glib::ustring error;
	Glib::ustring flags;
	std::vector<Glib::ustring> mountpoints ;
	
	std::vector<Partition> logicals ;

	bool strict ;
	
private:
	

};

}//GParted
#endif //PARTITION
