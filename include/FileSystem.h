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

#include "../include/Operation.h"
#include "../include/PipeCapture.h"

#include <fstream>
#include <sys/stat.h>

namespace GParted
{

enum ExecFlags
{
	EXEC_NONE         = 1 << 0,
	EXEC_CHECK_STATUS = 1 << 1,  // Set the status of the command in the operation
	                             // details based on the exit status being zero or
	                             // non-zero.  Must either use this flag when calling
	                             // ::execute_command() or call ::set_status()
	                             // afterwards.
	EXEC_CANCEL_SAFE  = 1 << 2
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
	virtual bool is_busy( const Glib::ustring & path ) { return false ; } ;
	virtual void set_used_sectors( Partition & partition ) {};
	virtual void read_label( Partition & partition ) {};
	virtual bool write_label( const Partition & partition, OperationDetail & operationdetail ) { return false; };
	virtual void read_uuid( Partition & partition ) {};
	virtual bool write_uuid( const Partition & partition, OperationDetail & operationdetail ) { return false; };
	virtual bool create( const Partition & new_partition, OperationDetail & operationdetail ) { return false; };
	virtual bool resize( const Partition & partition_new,
			     OperationDetail & operationdetail,
			     bool fill_partition = false ) { return false; };
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
	int execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
	                     ExecFlags flags = EXEC_NONE );
	void set_status( OperationDetail & operationdetail, bool success );
	void execute_command_eof();
	Glib::ustring mk_temp_dir( const Glib::ustring & infix, OperationDetail & operationdetail ) ;
	void rm_temp_dir( const Glib::ustring dir_name, OperationDetail & operationdetail ) ;

	//those are used in several places..
	Glib::ustring output, error ;
	Sector T, N, S ;  //File system [T]otal num of blocks, [N]um of free (or used) blocks, block [S]ize
	int exit_status ;
	unsigned int index ;
	sigc::signal<void, OperationDetail *> signal_progress;

private:
	void update_command_progress( OperationDetail *operationdetail );
	void store_exit_status( GPid pid, int status );
	bool running;
	int pipecount;
};

} //GParted

#endif /* GPARTED_FILESYSTEM_H */
