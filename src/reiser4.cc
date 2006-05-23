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
 
 
#include "../include/reiser4.h"

namespace GParted
{

FS reiser4::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_REISER4 ;
	
	if ( ! Glib::find_program_in_path( "debugfs.reiser4" ) .empty() )
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "mkfs.reiser4" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "fsck.reiser4" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
	
	fs .copy = GParted::FS::GPARTED ;
	
	/*
	 * IT SEEMS RESIZE AND COPY AREN'T IMPLEMENTED YET IN THE TOOLS...
	 * SEE http://marc.theaimsgroup.com/?t=109883161600003&r=1&w=2 for more information.. 
	 */
	  
	return fs ;
}

void reiser4::Set_Used_Sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "debugfs.reiser4 " + partition .get_path(), output, error, true ) )
	{
		index = output .find( "free blocks" ) ;
		if ( index >= output .length() ||
		     sscanf( output.substr( index ) .c_str(), "free blocks: %Ld", &N ) != 1 )   
			N = -1 ;
	
		index = output .find( "blksize" ) ;
		if ( index >= output.length() ||
		     sscanf( output.substr( index ) .c_str(), "blksize: %Ld", &S ) != 1 )  
			S = -1 ;

		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::round( N * ( S / 512.0 ) ) ) ;
	}
	else
		partition .error = error ;
}

bool reiser4::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::get_filesystem_string( GParted::FS_REISER4 ) ) ) ) ;
	
	if ( ! execute_command( "mkfs.reiser4 --yes " + new_partition .get_path(), operation_details .back() .sub_details ) )
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

bool reiser4::Resize( const Partition & partition_new,
		      std::vector<OperationDetails> & operation_details,
		      bool fill_partition )
{
	return true ;
}

bool reiser4::Copy( const Glib::ustring & src_part_path, 
		    const Glib::ustring & dest_part_path,
		    std::vector<OperationDetails> & operation_details )
{
	return true ;
}

bool reiser4::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("check filesystem on %1 for errors and (if possible) fix them"),
						  partition .get_path() ) ) ) ;
	
	if ( ! execute_command( "fsck.reiser4 --yes --fix " + partition .get_path(),
				operation_details .back() .sub_details ) )
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

} //GParted


