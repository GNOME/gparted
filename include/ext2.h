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
 
 
#ifndef DEFINE_EXT2
#define DEFINE_EXT2

#include "../include/FileSystem.h"

namespace GParted
{

class ext2 : public FileSystem
{
public:
	FS get_filesystem_support() ;
	void Set_Used_Sectors( Partition & partition ) ;
	bool Create( const Partition & new_partition, std::vector<OperationDetail> & operation_details ) ;
	bool Resize( const Partition & partition_new,
		     std::vector<OperationDetail> & operation_details,
		     bool fill_partition = false ) ;
	bool Copy( const Glib::ustring & src_part_path,
		   const Glib::ustring & dest_part_path,
		   std::vector<OperationDetail> & operation_details ) ;
	bool Check_Repair( const Partition & partition, std::vector<OperationDetail> & operation_details ) ;
};

} //GParted

#endif //EXT2
