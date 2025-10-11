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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#ifndef GPARTED_FILESYSTEM_H
#define GPARTED_FILESYSTEM_H

#include "OperationDetail.h"
#include "Partition.h"
#include "PipeCapture.h"
#include "Utils.h"

#include <glibmm/ustring.h>


namespace GParted
{


enum CUSTOM_TEXT
{
	CTEXT_NONE,
	CTEXT_ACTIVATE_FILESYSTEM,       // Activate text ('Mount', 'Swapon', VG 'Activate', ...)
	CTEXT_DEACTIVATE_FILESYSTEM,     // Deactivate text ('Unmount', 'Swapoff', VG 'Deactivate', ...)
	CTEXT_CHANGE_UUID_WARNING,       // Warning to print when changing UUIDs
	CTEXT_RESIZE_DISALLOWED_WARNING  // File system resizing currently disallowed reason
};

// Minimum and maximum file system size limits
struct FS_Limits
{
	Byte_Value min_size;  // 0 => no limit, +ve => limit defined.  (As implemented by)
	Byte_Value max_size;  // -----------------"-----------------   (code using these.)

	FS_Limits()                                 : min_size( 0 )  , max_size( 0 )    {};
	FS_Limits( Byte_Value min, Byte_Value max ) : min_size( min ), max_size( max )  {};
};

// Struct to store file system support information
struct FS
{
	enum Support
	{
		NONE      = 0,
		GPARTED   = 1,
		LIBPARTED = 2,
		EXTERNAL  = 3
	};

	FSType fstype;
	Support busy;               // How to determine if partition/file system is busy
	Support read;               // Can and how to read sector usage while inactive
	Support read_label;
	Support write_label;
	Support read_uuid;
	Support write_uuid;
	Support create;
	Support create_with_label;  // Can and how to format file system with label
	Support grow;
	Support shrink;
	Support move;               // start point and endpoint
	Support check;              // Some check tool available?
	Support copy;
	Support remove;
	Support online_read;        // Can and how to read sector usage while active
	Support online_grow;
	Support online_shrink;
	Support online_write_label;

	FS(FSType fstype_ = FS_UNSUPPORTED) : fstype(fstype_)
	{
		busy = read = read_label = write_label = read_uuid = write_uuid = create =
		create_with_label = grow = shrink = move = check = copy = remove = online_read =
		online_grow = online_shrink = online_write_label = NONE;
	}
};


class FileSystem
{
public:
	FileSystem() ;
	virtual ~FileSystem() {}

	virtual const Glib::ustring & get_custom_text( CUSTOM_TEXT ttype, int index = 0 ) const;
	static const Glib::ustring & get_generic_text( CUSTOM_TEXT ttype, int index = 0 );

	virtual FS get_filesystem_support() = 0 ;
	virtual FS_Limits get_filesystem_limits(const Partition& partition) const  { return m_fs_limits; };
	virtual bool is_busy( const Glib::ustring & path ) { return false ; } ;
	virtual void set_used_sectors( Partition & partition ) {};
	virtual void read_label( Partition & partition ) {};
	virtual bool write_label( const Partition & partition, OperationDetail & operationdetail ) { return false; };
	virtual void read_uuid( Partition & partition ) {};
	virtual bool write_uuid( const Partition & partition, OperationDetail & operationdetail ) { return false; };
	virtual bool create( const Partition & new_partition, OperationDetail & operationdetail ) { return false; };
	virtual bool resize( const Partition & partition_new,
	                     OperationDetail & operationdetail,
	                     bool fill_partition ) { return false; };
	virtual bool move( const Partition & partition_new
	                 , const Partition & partition_old
	                 , OperationDetail & operationdetail
			   ) { return false; };
	virtual bool copy( const Partition & src_part,
			   Partition & dest_part,
			   OperationDetail & operationdetail ) { return false; };
	virtual bool check_repair( const Partition & partition, OperationDetail & operationdetail ) { return false; };
	virtual bool remove( const Partition & partition, OperationDetail & operationdetail ) { return true; };

protected:
	static Glib::ustring mk_temp_dir(const Glib::ustring& infix, OperationDetail& operationdetail);
	static void rm_temp_dir(const Glib::ustring& dir_name, OperationDetail& operationdetail);

	FS_Limits m_fs_limits;  // File system minimum and maximum size limits.  In derived
	                        // classes either assign fixed values in get_filesystem_support()
	                        // or implement get_filesystem_limits() for dynamic values.

};


}  // namespace GParted


#endif /* GPARTED_FILESYSTEM_H */
