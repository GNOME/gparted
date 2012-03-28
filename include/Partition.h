/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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

enum PartitionAlignment {
	ALIGN_CYLINDER = 0,    //Align to nearest cylinder
	ALIGN_MEBIBYTE = 1,    //Align to nearest mebibyte
	ALIGN_STRICT   = 2     //Strict alignment - no rounding
	                       //  Indicator if start and end sectors must remain unchanged
};

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
			Byte_Value sector_size,
			bool inside_extended,
			bool busy ) ;

	void set_sector_usage( Sector sectors_fs_size, Sector sectors_fs_unused ) ;
	bool significant_unallocated_space() const ;

	void Set_Unallocated( const Glib::ustring & device_path, 
			      Sector sector_start,
			      Sector sector_end,
			      Byte_Value sector_size,
			      bool inside_extended );

	//update partition number (used when a logical partition is deleted) 
	void Update_Number( int new_number );
	
	void add_path( const Glib::ustring & path, bool clear_paths = false ) ;
	void add_paths( const std::vector<Glib::ustring> & paths, bool clear_paths = false ) ;
	Byte_Value get_byte_length() const ;
	Sector get_sector_length() const ; 
	Glib::ustring get_path() const ;
	std::vector<Glib::ustring> get_paths() const ;
	void add_mountpoint( const Glib::ustring & mountpoint, bool clear_mountpoints = false ) ;
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
	PartitionAlignment alignment;   //ALIGN_CYLINDER, ALIGN_STRICT, etc
	FILESYSTEM filesystem ;
	Glib::ustring label ;
	Glib::ustring uuid ;
	Sector sector_start;
	Sector sector_end;
	Sector sectors_used;
	Sector sectors_unused;
	Sector sectors_unallocated;  //Difference between the size of the partition and the file system
	Gdk::Color color;
	bool inside_extended;
	bool busy;
	std::vector<Glib::ustring> messages ;
	std::vector<Glib::ustring> flags ;
	
	std::vector<Partition> logicals ;

	bool strict_start ;	//Indicator if start sector must stay unchanged
	Sector free_space_before ;  //Free space preceding partition value

	Byte_Value sector_size ;  //Sector size of the disk device needed for converting to/from sectors and bytes.

private:
	void sort_paths_and_remove_duplicates() ;

	static bool compare_paths( const Glib::ustring & A, const Glib::ustring & B ) ;
	
	std::vector<Glib::ustring> paths ;
	std::vector<Glib::ustring> mountpoints ;
};

}//GParted
#endif //PARTITION
