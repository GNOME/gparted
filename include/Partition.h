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

#include "../include/i18n.h"

#include <gtkmm/label.h>
#include <glibmm/ustring.h>
#include <gdkmm/colormap.h>

#include <sstream>
#include <iostream>

//compose library, dedicated to the translators :P
#include "../compose/ucompose.hpp"

#define MEGABYTE 2048  //try it: 2048 * 512 / 1024 /1024 == 1    :P

namespace GParted
{
typedef long long Sector;//one day this won't be sufficient,  oh how i dream of that day... :-P

//------------global used convenience functions----------------------------	
inline long Sector_To_MB( Sector sectors ) 
{
	 return (long) ( (double) sectors * 512/1024/1024 +0.5) ;
}

inline long Round( double double_value )
{
	 return (long) ( double_value + 0.5) ;
}

inline Sector Abs( Sector sectors )
{
	return sectors < 0 ? sectors - 2*sectors : sectors ;
}

inline Gtk::Label * mk_label( const Glib::ustring & text ) 
{
	Gtk::Label * label = manage( new Gtk::Label() ) ;
	label ->set_markup( text ) ;
	label ->set_alignment( Gtk::ALIGN_LEFT ) ;
	return label ;
}

inline Glib::ustring num_to_str( Sector number )
{
	std::ostringstream os;
	os .imbue(std::locale(""));
	os << number ;
	return os .str() ;
}
//----------------------------------------------------------------------------------------------
	
	
enum PartitionType {
	PRIMARY		=	0,
	LOGICAL		=	1,
	EXTENDED	=	2,
	UNALLOCATED	=	3
};

enum PartitionStatus {
	STAT_REAL	=	1,
	STAT_NEW	=	2,
	STAT_COPY	=	3
};

/*
enum FileSystem {
	ext2		=	0,
	ext3		=	1,
	linux_swap	=	2,
	reiserfs		=	3,
	hfs		=	4,
	jfs		=	5,
	hp_ufs		=	6,
	sun_ufs		=	7,
	xfs		=	8,
	fat16		=	9,
	fat32		=	10,
	ntfs		=	11
};
*/	
class Partition
{
public:
	Partition();
	~Partition() ;
	
	//simple Set-functions.  only for convenience, since most members are public
	void Set( 	const Glib::ustring & partition,
			const int partition_number,
			const PartitionType type,
			const Glib::ustring & filesystem,
			const Sector & sector_start,
			const Sector & sector_end,
			const Sector & sectors_used,
			const bool inside_extended,
			const bool busy ) ;

	
	void Set_Unallocated( Sector sector_start, Sector sector_end, bool inside_extended );
	
	//get color associated with filesystem 
	Glib::ustring Get_Color( const Glib::ustring & filesystem );	

	//update partition number (used when a logical partition is deleted) 
	void Update_Number( int new_number );
	
	long Get_Length_MB( );
	long Get_Used_MB( );
	long Get_Unused_MB( );
		
	//some public members
	Glib::ustring partition;//the symbolic path (e.g. /dev/hda1 )
	int partition_number;
	PartitionType type;// UNALLOCATED, PRIMARY, LOGICAL, etc...
	PartitionStatus status; //STAT_REAL, STAT_NEW, etc..
	Glib::ustring filesystem;// ext2, ext3, ntfs, etc....
	Sector sector_start;
	Sector sector_end;
	Sector sectors_used;
	Sector sectors_unused;
	Gdk::Color color;
	Glib::ustring color_string;
	bool inside_extended;//used to check wether partition resides inside extended partition or not.
	bool busy;
	Glib::ustring error;
	Glib::ustring flags;
	
private:
	

};

}//GParted
#endif //PARTITION
