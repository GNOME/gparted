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

#include "Operation.h"
#include "Partition.h"
#include "PipeCapture.h"
#include "Utils.h"

#include <fstream>
#include <sys/stat.h>
#include <sigc++/slot.h>

namespace GParted
{

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

	FSType filesystem;
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

	FS( FSType fstype = FS_UNKNOWN ) : filesystem( fstype )
	{
		busy = read = read_label = write_label = read_uuid = write_uuid = create =
		create_with_label = grow = shrink = move = check = copy = remove = online_read =
		online_grow = online_shrink = NONE;
	}
};

enum ExecFlags
{
	EXEC_NONE            = 1 << 0,
	EXEC_CHECK_STATUS    = 1 << 1,  // Set the status of the command in the operation
	                                // details based on the exit status being zero or
	                                // non-zero.  Must either use this flag when calling
	                                // ::execute_command() or call ::set_status()
	                                // afterwards.
	EXEC_CANCEL_SAFE     = 1 << 2,
	EXEC_PROGRESS_STDOUT = 1 << 3,  // Run progress tracking callback after reading new
	                                // data on stdout from command.
	EXEC_PROGRESS_STDERR = 1 << 4,  // Same but for stderr.
	EXEC_PROGRESS_TIMED  = 1 << 5   // Run progress tracking callback periodically.
};

inline ExecFlags operator|( ExecFlags lhs, ExecFlags rhs )
	{ return static_cast<ExecFlags>( static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs) ); }

inline ExecFlags operator&( ExecFlags lhs, ExecFlags rhs )
	{ return static_cast<ExecFlags>( static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs) ); }

class FileSystem
{
public:
	FileSystem() ;
	virtual ~FileSystem() {}

	virtual const Glib::ustring get_custom_text( CUSTOM_TEXT ttype, int index = 0 ) const;
	static const Glib::ustring get_generic_text( CUSTOM_TEXT ttype, int index = 0 ) ;

	virtual FS get_filesystem_support() = 0 ;
	virtual FS_Limits get_filesystem_limits( const Partition & partition ) const  { return fs_limits; };
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
	typedef sigc::slot<void, OperationDetail *> StreamSlot;
	typedef sigc::slot<bool, OperationDetail *> TimedSlot;

	int execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
	                     ExecFlags flags = EXEC_NONE );
	int execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
	                     ExecFlags flags,
	                     StreamSlot stream_progress_slot );
	int execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
	                     ExecFlags flags,
	                     TimedSlot timed_progress_slot );
	void set_status( OperationDetail & operationdetail, bool success );
	void execute_command_eof();
	Glib::ustring mk_temp_dir( const Glib::ustring & infix, OperationDetail & operationdetail ) ;
	void rm_temp_dir( const Glib::ustring dir_name, OperationDetail & operationdetail ) ;

	FS_Limits fs_limits;  // File system minimum and maximum size limits.  In derived
	                      // classes either assign fixed values in get_filesystem_support()
	                      // or implement get_filesystem_limits() for dynamic values.

	//those are used in several places..
	Glib::ustring output, error ;
	Sector T, N, S ;  //File system [T]otal num of blocks, [N]um of free (or used) blocks, block [S]ize
	int exit_status ;

private:
	int execute_command_internal( const Glib::ustring & command, OperationDetail & operationdetail,
	                              ExecFlags flags,
	                              StreamSlot stream_progress_slot,
	                              TimedSlot timed_progress_slot );
	void store_exit_status( GPid pid, int status );
	bool running;
	int pipecount;
};

} //GParted

#endif /* GPARTED_FILESYSTEM_H */
