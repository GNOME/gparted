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
 
#ifndef GPARTED_CORE
#define GPARTED_CORE

#include "../include/Operation.h"
#include "../include/ext2.h"
#include "../include/ext3.h"
#include "../include/fat16.h"
#include "../include/fat32.h"
#include "../include/linux_swap.h"
#include "../include/reiserfs.h"
#include "../include/ntfs.h"

#include <glibmm/ustring.h>

#include <vector>
#include <fstream>

namespace GParted
{

class GParted_Core
{
public:
	GParted_Core( ) ;
	void find_supported_filesystems( ) ; 
	void get_devices( std::vector<Device> & devices, bool deep_scan ) ;
	
	int get_estimated_time( const Operation & operation ) ;

	void Apply_Operation_To_Disk( Operation & operation );
	
	bool Create( const Glib::ustring & device_path, Partition & new_partition ) ;
	bool Convert_FS( const Glib::ustring & device_path, const Partition & partition ) ;
	bool Delete( const Glib::ustring & device_path, const Partition & partition ) ;
	bool Resize( const Glib::ustring & device_path, const Partition & partition_old, const Partition & partition_new ) ; 
	bool Copy( const Glib::ustring & dest_device_path, const Glib::ustring & src_part_path, Partition & partition_dest ) ; 

	std::vector<FS> get_fs( ) ;
	Glib::RefPtr<Gtk::TextBuffer> get_textbuffer( ) ;

private:
	void set_device_partitions( Device & device, bool deep_scan = true ) ;
	Glib::ustring get_sym_path( const Glib::ustring & real_path ) ;
	void Set_Used_Sectors( Partition & partition );
	Glib::ustring Get_Flags( PedPartition *c_partition ) ;
	int Create_Empty_Partition( const Glib::ustring & device_path, Partition & new_partition, bool copy = false ) ;
	bool Resize_Container_Partition( const Glib::ustring & device_path, const Partition & partition_old, const Partition & partition_new ) ;
	bool Resize_Normal_Using_Libparted( const Glib::ustring & device_path, const Partition & partition_old, const Partition & partition_new ) ;

	void Show_Error( Glib::ustring message ) ;
	void set_proper_filesystem( const Glib::ustring & filesystem ) ;
	long Get_Cylinder_Size( const Glib::ustring & device_path ) ;
	
	Glib::RefPtr<Gtk::TextBuffer> textbuffer;
	
	std::vector<FS> FILESYSTEMS ;
	FileSystem * p_filesystem ;
	std::vector <PedPartitionFlag> flags;
	PedDevice *device ;
	PedDisk *disk ;
	PedPartition *c_partition ;
	Glib::ustring temp ;
};

} //GParted


#endif //GPARTED_CORE
