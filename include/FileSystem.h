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
 
 
#ifndef FILESYSTEM
#define FILESYSTEM

#include "../include/Partition.h"

#include <gtkmm/textbuffer.h>

#include <parted/parted.h>

//Some functions used by both (sub)Filesystems and GParted_Core-------------------------------------------------
inline bool open_device( const Glib::ustring & device_path, PedDevice *& device )
{
	device = ped_device_get( device_path .c_str( ) );
	
	return device ;
}
	
inline bool open_device_and_disk( const Glib::ustring & device_path, PedDevice *& device, PedDisk *& disk ) 
{
	
	if ( open_device( device_path, device ) )
		disk = ped_disk_new( device );
		
	if ( ! disk )
	{
		ped_device_destroy( device ) ;
		device = NULL ; 
		
		return false;
	}
	
	return true ;
}

inline void close_device_and_disk( PedDevice *& device, PedDisk *& disk )
{
	if ( device )
		ped_device_destroy( device ) ;
		
	if ( disk )
		ped_disk_destroy( disk ) ;
	
	device = NULL ;
	disk = NULL ;
}	

inline bool Commit( PedDisk *& disk ) 
{
	bool return_value = ped_disk_commit_to_dev( disk ) ;
	
	ped_disk_commit_to_os( disk ) ;
		
	return return_value ;
}
//---------------------------------------------------------------------------------------------------------------

namespace GParted
{

class FileSystem
{
public:
	FileSystem( ) ;
	virtual ~FileSystem( ) { }

	virtual FS get_filesystem_support( ) = 0 ;
	virtual void Set_Used_Sectors( Partition & partition ) = 0 ;
	virtual bool Create( const Glib::ustring device_path, const Partition & new_partition ) = 0 ;
	virtual bool Resize( const Partition & partition_new, bool fill_partition = false ) = 0 ;
	virtual bool Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path ) = 0 ;
	virtual bool Check_Repair( const Partition & partition ) = 0 ;
	virtual int get_estimated_time( long MB_to_Consider ) = 0 ;
	
	Glib::RefPtr<Gtk::TextBuffer> textbuffer ;
	
	PedDisk *disk ; //see GParted_Core::Set_Used_Sectors() ...
	long cylinder_size ; //see GParted_Core::Resize()
	
protected:
	bool Execute_Command( Glib::ustring command ) ;
	
	PedDevice *device ;
	
private:
	void Update_Textview( ) ;

	Glib::ustring output ;

};

} //GParted

#endif //FILESYSTEM
