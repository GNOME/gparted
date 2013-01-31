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
 
 
#include "../include/reiser4.h"

namespace GParted
{

FS reiser4::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_REISER4 ;
	
	if ( ! Glib::find_program_in_path( "debugfs.reiser4" ) .empty() )
	{
		fs .read = GParted::FS::EXTERNAL ;
		fs .read_label = FS::EXTERNAL ;
		fs .read_uuid = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "mkfs.reiser4" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "fsck.reiser4" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;

	if ( fs .check )
	{
		fs .copy = GParted::FS::GPARTED ;
		fs .move = GParted::FS::GPARTED ;
	}

	fs .online_read = FS::GPARTED ;

	/*
	 * IT SEEMS RESIZE AND COPY AREN'T IMPLEMENTED YET IN THE TOOLS...
	 * SEE http://marc.theaimsgroup.com/?t=109883161600003&r=1&w=2 for more information.. 
	 */
	  
	return fs ;
}

void reiser4::set_used_sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "debugfs.reiser4 " + partition .get_path(), output, error, true ) )
	{
		index = output .find( "\nblocks:" ) ;
		if ( index >= output .length() ||
		     sscanf( output.substr( index ) .c_str(), "\nblocks: %Ld", &T ) != 1 )
			T = -1 ;

		index = output .find( "\nfree blocks:" ) ;
		if ( index >= output .length() ||
		     sscanf( output.substr( index ) .c_str(), "\nfree blocks: %Ld", &N ) != 1 )
			N = -1 ;

		index = output .find( "\nblksize:" ) ;
		if ( index >= output.length() ||
		     sscanf( output.substr( index ) .c_str(), "\nblksize: %Ld", &S ) != 1 )
			S = -1 ;

		if ( T > -1 && N > -1 && S > -1 )
		{
			T = Utils::round( T * ( S / double(partition .sector_size) ) ) ;
			N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;
			partition .set_sector_usage( T, N ) ;
		}
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void reiser4::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "debugfs.reiser4 " + partition .get_path(), output, error, true ) )
	{
		Glib::ustring label = Utils::regexp_label( output, "^label:[[:blank:]]*(.*)$" ) ;
		//Avoid reading any trailing junk after the label
		if ( label .length() > 16 )
			label .resize( 16 ) ;
		if ( label != "<none>" )
			partition .set_label( label ) ;
		else
			partition .set_label( "" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void reiser4::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( "debugfs.reiser4 " + partition .get_path(), output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "uuid:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool reiser4::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkfs.reiser4 --yes --label \"" + new_partition.get_label() +
				  "\" " + new_partition.get_path(), operationdetail,
				  false, true );
}

bool reiser4::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "fsck.reiser4 --yes --fix --quiet " + partition.get_path(),
				  operationdetail, false, true );
}

} //GParted


