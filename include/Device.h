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
 
 /******************************************************
 This class.... is a mess ;-)
 It's does some kind of wrapping around libparted, but it's not very clear and it looks like shit.
 The one thing i have to say in favor of it is: IT JUST WORKS :)
 
 When i've time i will look into it and try to sort things out a bit. 
 Maybe building a small decent wrapper or so...
 ********************************************************/
 
#ifndef DEVICE
#define DEVICE

#include <parted/parted.h>

#include "../include/Partition.h"

#include <gtkmm/messagedialog.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>

#include <fstream>


namespace GParted
{
	
class Device
{
	 
public:
	Device() ;
	Device( const Glib::ustring & device_path, std::vector<FS> *filesystems );
	~Device() ;
	//this function creates a fresh list with al the partitions and free spaces
	void Read_Disk_Layout() ;
	bool Delete_Partition( const Partition & partition ) ;
	bool Create_Partition_With_Filesystem( Partition & new_part, PedTimer *timer) ;
	bool Move_Resize_Partition( const Partition & partition_original, const Partition & partition_new , PedTimer *timer) ;
	bool Set_Partition_Filesystem( const Partition & new_partition, PedTimer * timer);
	bool Copy_Partition( Device *source_device, const Partition & source_partition, PedTimer * timer ) ;
	int Create_Empty_Partition( const Partition & new_partition, PedConstraint * constraint = NULL ) ;
	bool Commit() ;

	PedPartition * Get_c_partition( int part_num );
			
	std::vector<Partition> Get_Partitions() ;
	Sector Get_Length() ;
	long Get_Heads() ;
	long Get_Sectors() ;
	long Get_Cylinders() ;
	Glib::ustring Get_Model() ;
	Glib::ustring Get_Path() ;
	Glib::ustring Get_RealPath() ;
	Glib::ustring Get_DiskType() ;
	bool Get_any_busy() ;
	int Get_Max_Amount_Of_Primary_Partitions() ;

private:
	//make a try to get the amount of used sectors on a filesystem ( see comments in implementation )
	Sector Get_Used_Sectors( PedPartition *c_partition, const Glib::ustring & sym_path );
	//bool Supported( const Glib::ustring & filesystem ) ;

	Glib::ustring Get_Flags( PedPartition *c_partition ) ;	

	bool open_device_and_disk() ;
	void close_device_and_disk() ;
	bool Resize_Extended( const Partition & partition, PedTimer *timer) ;
		
	std::vector<Partition> device_partitions ;
	std::vector<FS> * FILESYSTEMS ;
	Sector length;
	long heads ;
	long sectors ;
	long cylinders ;
	Glib::ustring model;
 	Glib::ustring path;
 	Glib::ustring realpath;
 	Glib::ustring disktype;
	 
	//private variables
	PedDevice *device ;
	PedDisk *disk ;
	PedPartition *c_partition ;
		
	std::vector <PedPartitionFlag> flags;
	Glib::ustring temp, error ; //error may contain an errormessage for an specific partition ( see Get_Used_Sectors() ) 		
	Partition partition_temp ;
 };
 
 } //GParted
 
#endif //DEVICE
