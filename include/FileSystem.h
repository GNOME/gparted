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

#include "../include/Operation.h"

#include <fstream>
#include <sys/stat.h>

namespace GParted
{

class FileSystem
{
public:
	FileSystem() ;
	virtual ~FileSystem() {}

	virtual FS get_filesystem_support() = 0 ;
	virtual void Set_Used_Sectors( Partition & partition ) = 0 ;
	virtual bool Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details ) = 0 ;
	virtual bool Resize( const Partition & partition_new,
			     std::vector<OperationDetails> & operation_details,
			     bool fill_partition = false ) = 0 ;
	virtual bool Copy( const Glib::ustring & src_part_path,
			   const Glib::ustring & dest_part_path,
			   std::vector<OperationDetails> & operation_details ) = 0 ;
	virtual bool Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details ) = 0 ;
	
	Sector cylinder_size ; //see GParted_Core::resize()
	
protected:
	int execute_command( const Glib::ustring & command, std::vector<OperationDetails> & operation_details ) ;

	//those are used in several places..
	Glib::ustring output, error ;
	Sector N, S ;
	int exit_status ;
	unsigned int index ;
	
private:

};

} //GParted

#endif //DEFINE_FILESYSTEM
