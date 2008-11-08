/* Copyright (C) 2004, 2005, 2006, 2007, 2008 Bart Hakvoort
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

FS reiserfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_REISERFS ;
	
	if ( ! Glib::find_program_in_path( "debugreiserfs" ) .empty() )
	{
		fs .read = GParted::FS::EXTERNAL ;
		fs .read_label = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "reiserfstune" ) .empty() )
		fs .write_label = FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "mkreiserfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "reiserfsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
		
	//resizing is a delicate process ...
	if ( ! Glib::find_program_in_path( "resize_reiserfs" ) .empty() && fs .check )
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}

	if ( fs .check )
	{
		fs .copy = GParted::FS::GPARTED ;
		fs .move = GParted::FS::GPARTED ;
	}

	fs .MIN = 32 * MEBIBYTE ;
	
	return fs ;
}

void reiserfs::set_used_sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "debugreiserfs " + partition .get_path(), output, error, true ) )
	{
		index = output .find( "Blocksize" ) ;
		if ( index >= output .length() || 
		     sscanf( output .substr( index ) .c_str(), "Blocksize: %Ld", &S ) != 1 )
			S = -1 ;

		index = output .find( ":", output .find( "Free blocks" ) ) +1 ;
		if ( index >= output .length() ||
		     sscanf( output .substr( index ) .c_str(), "%Ld", &N ) != 1 )
			N = -1 ;

		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::round( N * ( S / 512.0 ) ) ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void reiserfs::read_label( Partition & partition )
{
	//FIXME: i think running debugreiserfs takes a long time on filled filesystems, test for this...
	if ( ! Utils::execute_command( "debugreiserfs " + partition .get_path(), output, error, true ) )
	{
		partition .label = Utils::regexp_label( output, "^label:[\t ]*(.*)$" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}
	
bool reiserfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "reiserfstune --label \"" + partition .label + "\" " + partition .get_path(), operationdetail ) ;
}

bool reiserfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkreiserfs -f --label \"" + new_partition .label + "\" " + new_partition .get_path(), operationdetail ) ;
}

bool reiserfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{ 
	Glib::ustring str_temp = "echo y | resize_reiserfs " + partition_new .get_path() ;
	
	if ( ! fill_partition )
	{
		str_temp += " -s " ;
		str_temp += Utils::num_to_str( Utils::round( Utils::sector_to_unit(
				partition_new .get_length(), GParted::UNIT_BYTE ) ) -1, true ) ;
	}

	exit_status = execute_command( str_temp, operationdetail ) ;

	return ( exit_status == 0 || exit_status == 256 ) ;
}

bool reiserfs::copy( const Glib::ustring & src_part_path,
		     const Glib::ustring & dest_part_path,
		     OperationDetail & operationdetail )
{	
	return true ;
}

bool reiserfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "reiserfsck --yes --fix-fixable --quiet " + partition .get_path(), operationdetail ) ;
	
	return ( exit_status == 0 || exit_status == 1 || exit_status == 256 ) ;
}

} //GParted
