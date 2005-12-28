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

FS reiser4::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = GParted::FS_REISER4 ;
	
	if ( ! system( "which debugfs.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkfs.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which fsck.reiser4 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	/*
	 * IT SEEMS RESIZE AND COPY AREN'T IMPLEMENTED YET IN THE TOOLS...
	 * SEE http://marc.theaimsgroup.com/?t=109883161600003&r=1&w=2 for more information.. (it seems NameSys is getting commercial? :| )
	*/
	  
	return fs ;
}

void reiser4::Set_Used_Sectors( Partition & partition ) 
{
	argv .push_back( "debugfs.reiser4" ) ;
	argv .push_back( partition .partition ) ;

	envp .push_back( "LC_ALL=C" ) ;
	
	try
	{
		Glib::spawn_sync( ".", argv, envp, Glib::SPAWN_SEARCH_PATH, sigc::slot< void >(), &output ) ;
	}
	catch ( Glib::Exception & e )
	{ 
		std::cout << e .what() << std::endl ;
		return ;
	} 

	index = output .find( "free blocks" ) ;
	if ( index >= output .length() || sscanf( output.substr( index ) .c_str(), "free blocks: %Ld", &N ) != 1 )   
		N = -1 ;
	
	index = output .find( "blksize" ) ;
	if ( index >= output.length() || sscanf( output.substr( index ) .c_str(), "blksize: %Ld", &S ) != 1 )  
		S = -1 ;

	if ( N > -1 && S > -1 )
		partition .Set_Unused( N * S / 512 ) ;
}

bool reiser4::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkfs.reiser4 --yes " + new_partition .partition ) ;
}

bool reiser4::Resize( const Partition & partition_new, bool fill_partition )
{
	return true ;
}

bool reiser4::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	return true ;
}

bool reiser4::Check_Repair( const Partition & partition )
{
	return ! Execute_Command( "fsck.reiser4 --yes --fix " + partition .partition ) ;
}

int reiser4::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


