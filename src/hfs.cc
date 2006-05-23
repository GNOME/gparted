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
 
 
#include "../include/hfs.h"

namespace GParted
{

FS hfs::get_filesystem_support()
{
	FS fs ;
		
	fs .filesystem = GParted::FS_HFS ;
	
	fs .read = GParted::FS::LIBPARTED ; 
	fs .shrink = GParted::FS::LIBPARTED ; 

	if ( ! Glib::find_program_in_path( "hformat" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	fs .copy = GParted::FS::GPARTED ;
	
	fs .MAX = 2048 * MEBIBYTE ;
	
	return fs ;
}

void hfs::Set_Used_Sectors( Partition & partition ) 
{
}

bool hfs::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::get_filesystem_string( GParted::FS_HFS ) ) ) ) ;
	
	if ( ! execute_command( "hformat " + new_partition .get_path(), operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

bool hfs::Resize( const Partition & partition_new,
		  std::vector<OperationDetails> & operation_details,
		  bool fill_partition )
{
	return true ;
}

bool hfs::Copy( const Glib::ustring & src_part_path,
		const Glib::ustring & dest_part_path,
		std::vector<OperationDetails> & operation_details )
{
	return true ;
}

bool hfs::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	return true ;
}

} //GParted


