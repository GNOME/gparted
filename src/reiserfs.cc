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

#include "reiserfs.h"
#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

FS reiserfs::get_filesystem_support()
{
	FS fs( FS_REISERFS );

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "debugreiserfs" ) .empty() )
	{
		fs .read = GParted::FS::EXTERNAL ;
		fs .read_label = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "reiserfstune" ) .empty() )
	{
		fs .write_label = FS::EXTERNAL ;
		fs .read_uuid = FS::EXTERNAL ;
		fs .write_uuid = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "mkreiserfs" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
	}
	
	if ( ! Glib::find_program_in_path( "reiserfsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
		
	//resizing is a delicate process ...
	if ( ! Glib::find_program_in_path( "resize_reiserfs" ) .empty() && fs .check )
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min file system size..
			fs .shrink = GParted::FS::EXTERNAL ;
	}

	if ( fs .check )
	{
		fs .copy = GParted::FS::GPARTED ;
		fs .move = GParted::FS::GPARTED ;
	}

	fs .online_read = FS::GPARTED ;
#ifdef ENABLE_ONLINE_RESIZE
	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
		fs. online_grow = fs. grow ;
#endif

	//Actual minimum is at least 18 blocks larger than 32 MiB for the journal offset
	fs_limits.min_size = 34 * MEBIBYTE;

	return fs ;
}

void reiserfs::set_used_sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "debugreiserfs " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                         )
	{
		Glib::ustring::size_type index = output.find( "Count of blocks on the device:" );
		if ( index >= output .length() ||
		     sscanf( output.substr( index ).c_str(), "Count of blocks on the device: %lld", &T ) != 1 )
			T = -1 ;

		index = output .find( "Blocksize:" ) ;
		if ( index >= output .length() || 
		     sscanf( output.substr( index ).c_str(), "Blocksize: %lld", &S ) != 1 )
			S = -1 ;

		index = output .find( ":", output .find( "Free blocks" ) ) +1 ;
		if ( index >= output .length() ||
		     sscanf( output.substr( index ).c_str(), "%lld", &N ) != 1 )
			N = -1 ;

		if ( T > -1 && N > -1 && S > -1 )
		{
			T = Utils::round( T * ( S / double(partition .sector_size) ) ) ;
			N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;
			partition .set_sector_usage( T, N ) ;
			partition.fs_block_size = S;
		}
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

void reiserfs::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "debugreiserfs " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                         )
	{
		partition.set_filesystem_label( Utils::regexp_label( output, "^label:[\t ]*(.*)$" ) );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}
	
bool reiserfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "reiserfstune --label " + Glib::shell_quote( partition.get_filesystem_label() ) +
	                          " " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

void reiserfs::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( "debugreiserfs " + Glib::shell_quote( partition .get_path() ), output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "^UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool reiserfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "reiserfstune -u random " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool reiserfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkreiserfs -f -f --label " +
	                          Glib::shell_quote( new_partition.get_filesystem_label() ) +
	                          " " + Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

bool reiserfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{ 
	Glib::ustring size = "" ;
	if ( ! fill_partition )
	{
		size = " -s " + Utils::num_to_str( floor( Utils::sector_to_unit(
		                   partition_new .get_sector_length(), partition_new .sector_size, UNIT_BYTE ) ) ) ;
	}
	const Glib::ustring resize_cmd = "echo y | resize_reiserfs" + size +
	                                 " " + Glib::shell_quote( partition_new.get_path() );
	exit_status = execute_command( "sh -c " + Glib::shell_quote( resize_cmd ), operationdetail );
	// NOTE: Neither resize_reiserfs manual page nor the following commit, which first
	// added this check, indicate why exit status 1 also indicates success.  Commit
	// from 2006-05-23:
	//     7bb7e8a84f164cd913384509a6adc3739a9d8b78
	//     Use ped_device_read and ped_device_write instead of 'dd' to copy
	bool success = ( exit_status == 0 || exit_status == 1 );
	set_status( operationdetail, success );
	return success;
}

bool reiserfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "reiserfsck --yes --fix-fixable --quiet " +
	                               Glib::shell_quote( partition.get_path() ),
	                               operationdetail, EXEC_CANCEL_SAFE );
	bool success = ( exit_status == 0 || exit_status == 1 );
	set_status( operationdetail, success );
	return success;
}

} //GParted
