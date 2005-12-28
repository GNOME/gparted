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
 
 
#include "../include/reiserfs.h"

namespace GParted
{

FS reiserfs::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = GParted::FS_REISERFS ;
	
	if ( ! system( "which debugreiserfs 1>/dev/null 2>/dev/null" ) ) 
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkreiserfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which reiserfsck 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
		
	//resizing is a delicate process ...
	if ( ! system( "which resize_reiserfs 1>/dev/null 2>/dev/null" ) && fs .check ) 
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}
	
	//we need to call resize_reiserfs after a copy to get proper used/unused
	if ( ! system( "which dd 1>/dev/null 2>/dev/null" ) && fs .grow ) 
		fs .copy = GParted::FS::EXTERNAL ;
	
	fs .MIN = 32 ;
	
	return fs ;
}

void reiserfs::Set_Used_Sectors( Partition & partition ) 
{
	argv .push_back( "debugreiserfs" ) ;
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

	index = output .find( "Blocksize" ) ;
	if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "Blocksize: %Ld", &S ) != 1 )
		S = -1 ;

	index = output .find( ":", output .find( "Free blocks" ) ) +1 ;
	if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "%Ld", &N ) != 1 )
		N = -1 ;

	if ( N > -1 && S > -1 )
		partition .Set_Unused( N * S / 512 ) ;
}
	
bool reiserfs::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkreiserfs -q " + new_partition .partition ) ;
}

bool reiserfs::Resize( const Partition & partition_new, bool fill_partition )
{
	Glib::ustring str_temp = "echo y | resize_reiserfs " + partition_new .partition ;
	
	if ( ! fill_partition )
		str_temp += " -s " + Utils::num_to_str( partition_new .Get_Length_MB( ) - cylinder_size, true ) + "M" ;
	
	return ! Execute_Command( str_temp ) ; 
}

bool reiserfs::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{	
	if ( ! Execute_Command( "dd bs=8192 if=" + src_part_path + " of=" + dest_part_path ) )
	{
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, true ) ;
	}
	
	return false ;
}

bool reiserfs::Check_Repair( const Partition & partition )
{
	//according to the manpage it should return 0 or 1 on succes, instead it returns 256 on succes.. 
	//blah, don't have time for this. Just check for both options, we'll fix this later with our much improved errorhandling
	int t = Execute_Command( "reiserfsck -y --fix-fixable " + partition .partition ) ;
	
	return ( t <= 1 || t == 256 ) ;
}

int reiserfs::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	


} //GParted
