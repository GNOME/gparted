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
 
 
#ifndef DEFINE_FILESYSTEM
#define DEFINE_FILESYSTEM

#include "../include/Operation.h"
#include "../include/PipeCapture.h"

#include <fstream>
#include <sys/stat.h>

namespace GParted
{

class FileSystem
{
public:
	FileSystem() ;
	virtual ~FileSystem() {}

	virtual const Glib::ustring get_custom_text( CUSTOM_TEXT ttype, int index = 0 ) ;
	static const Glib::ustring get_generic_text( CUSTOM_TEXT ttype, int index = 0 ) ;

	virtual FS get_filesystem_support() = 0 ;
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
	virtual bool copy( const Glib::ustring & src_part_path,
			   const Glib::ustring & dest_part_path,
			   OperationDetail & operationdetail ) { return false; };
	virtual bool check_repair( const Partition & partition, OperationDetail & operationdetail ) { return false; };
	virtual bool remove( const Partition & partition, OperationDetail & operationdetail ) { return true; };
	bool success;
protected:
	int execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
			     bool checkstatus = false, bool cancel_safe = false );
	int execute_command_timed( const Glib::ustring & command, OperationDetail & operationdetail ) {
		return execute_command( command, operationdetail, true ); }
	void execute_command_eof();
	Glib::ustring mk_temp_dir( const Glib::ustring & infix, OperationDetail & operationdetail ) ;
	void rm_temp_dir( const Glib::ustring dir_name, OperationDetail & operationdetail ) ;

	//those are used in several places..
	Glib::ustring output, error ;
	Sector T, N, S ;  //File system [T]otal num of blocks, [N]um of free (or used) blocks, block [S]ize
	int exit_status ;
	unsigned int index ;
	
private:
	void store_exit_status( GPid pid, int status );
	bool running;
	int pipecount;
};

} //GParted

#endif //DEFINE_FILESYSTEM
