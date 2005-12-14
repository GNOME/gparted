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
 
 
#ifndef DEFINE_FILESYSTEM
#define DEFINE_FILESYSTEM

#include "../include/Partition.h"

#include <gtkmm/textbuffer.h>

#include <parted/parted.h>
#include <fstream>
#include <sys/stat.h>

namespace GParted
{

class FileSystem
{
public:
	FileSystem( ) ;
	virtual ~FileSystem( ) { }

	virtual FS get_filesystem_support( ) = 0 ;
	virtual void Set_Used_Sectors( Partition & partition ) = 0 ;
	virtual bool Create( const Partition & new_partition ) = 0 ;
	virtual bool Resize( const Partition & partition_new, bool fill_partition = false ) = 0 ;
	virtual bool Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path ) = 0 ;
	virtual bool Check_Repair( const Partition & partition ) = 0 ;
	virtual int get_estimated_time( long MB_to_Consider ) = 0 ;
	
	Glib::RefPtr<Gtk::TextBuffer> textbuffer ;
	
	long cylinder_size ; //see GParted_Core::Resize()
	
protected:
	int Execute_Command( Glib::ustring command ) ;
	
private:
	void Update_Textview( ) ;

	Glib::ustring output ;

};

} //GParted

#endif //DEFINE_FILESYSTEM
