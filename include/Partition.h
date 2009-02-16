/* Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Bart Hakvoort
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
	STAT_REAL	=	0,
	STAT_NEW	=	1,
	STAT_COPY	=	2,
	STAT_FORMATTED	=	3
};

//FIXME: we should make a difference between partition- and file system size. 
//especially in the core this can make a difference when giving detailed feedback. With new cairosupport in gtkmm
//it's even possible to make stuff like this visible in the GUI in a nice and clear way
class Partition
{
public:
	Partition() ;
	Partition( const Glib::ustring & path ) ;
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
	void set_used( Sector sectors_used ) ;

	void Set_Unallocated( const Glib::ustring & device_path, 
			      Sector sector_start,
			      Sector sector_end,
			      bool inside_extended );

	//update partition number (used when a logical partition is deleted) 
	void Update_Number( int new_number );
	
	void add_path( const Glib::ustring & path, bool clear_paths = false ) ;
	void add_paths( const std::vector<Glib::ustring> & paths, bool clear_paths = false ) ;
	Sector get_length() const ; 
	Glib::ustring get_path() const ;
	std::vector<Glib::ustring> get_paths() const ;
	void add_mountpoints( const std::vector<Glib::ustring> & mountpoints, bool clear_mountpoints = false ) ;
	Glib::ustring get_mountpoint() const ; 
	void clear_mountpoints() ;
	std::vector<Glib::ustring> get_mountpoints() const ;
	Sector get_sector() const ;
	bool test_overlap( const Partition & partition ) const ;

	bool operator==( const Partition & partition ) const ;
	bool operator!=( const Partition & partition ) const ;
		
	//some public members
	Glib::ustring device_path ;
	int partition_number;
	PartitionType type;// UNALLOCATED, PRIMARY, LOGICAL, etc...
	PartitionStatus status; //STAT_REAL, STAT_NEW, etc..
	FILESYSTEM filesystem ;
	Glib::ustring label ;
	Glib::ustring uuid ;
	Sector sector_start;
	Sector sector_end;
	Sector sectors_used;
	Sector sectors_unused;
	Gdk::Color color;
	bool inside_extended;
	bool busy;
	std::vector<Glib::ustring> messages ;
	std::vector<Glib::ustring> flags ;
	
	std::vector<Partition> logicals ;

	bool strict ;		//Indicator if start and end sectors must stay unchanged
	bool strict_start ;	//Indicator if start sector must stay unchanged
	
private:
	void sort_paths_and_remove_duplicates() ;

	static bool compare_paths( const Glib::ustring & A, const Glib::ustring & B ) ;
	
	std::vector<Glib::ustring> paths ;
	std::vector<Glib::ustring> mountpoints ;
};

}//GParted
#endif //PARTITION
