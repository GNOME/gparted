/* Copyright (C) 2013 Patrick Verner <exodusrobot@yahoo.com>
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

#include "f2fs.h"
#include "FileSystem.h"
#include "Partition.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{

FS f2fs::get_filesystem_support()
{
	FS fs( FS_F2FS );

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "mkfs.f2fs" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "resize.f2fs" ).empty() )
	{
		fs.grow = FS::EXTERNAL;

	}

	if ( ! Glib::find_program_in_path( "fsck.f2fs" ).empty() )
		fs.check = FS::EXTERNAL;

	if ( ! Glib::find_program_in_path( "dump.f2fs").empty() )
	{
		fs.read = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "blkid" ).empty() )
	{
		fs.read_label = FS::EXTERNAL;
		//fs.write_label = FS::EXTERNAL;
	}

	fs .copy = FS::GPARTED ;
	fs .move = FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	return fs ;
}

void f2fs::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "blkid -s LABEL  " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                   )
	{
		partition.set_filesystem_label( Utils::regexp_label( output, "LABEL=\"(\\w*)\"" )  );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

void f2fs::set_used_sectors( Partition & partition )
{
	// Try to get an estimation on how many free "segments" there is. It seems to be proportional.
	if ( ! Utils::execute_command( "dump.f2fs -d 1 " + Glib::shell_quote( partition.get_path() ), output, error, true ))
	{

		if ( partition .busy )
		{
			Byte_Value ignored ;
			Byte_Value fs_free ;
			if ( Utils::get_mounted_filesystem_usage( partition .get_mountpoint(),
			                                          ignored, fs_free, error ) == 0 )
			{
				N = fs_free / S;
			}
			else
			{
				N = -1 ;
				partition.push_back_message( error );
			}
		} else {
			Glib::ustring temp;
			temp = Utils::regexp_label( output, "segment_count\\s+\\[0x\\s+[0-9a-f]+ : ([0-9]+)\\]" ) ;
			sscanf( temp.c_str(), "%lld", &T );
			temp = Utils::regexp_label( output, "free_segment_count\\s+\\[0x\\s+[0-9a-f]+ : ([0-9]+)\\]" ) ;
			sscanf( temp.c_str(), "%lld", &N );
			temp = Utils::regexp_label( output, "sector size = ([0-9]+)" ) ;
			sscanf( temp.c_str(), "%lld", &S );

			T = T * S * 8;
			N = N * S * 8;
			partition.sector_size = S;
		}

		if ( T > -1 && N > -1 && S > -1 )
		{
			T = Utils::round( T * ( S / double(partition.sector_size) ) );
			N = Utils::round( N * ( S / double(partition.sector_size) ) );

			partition .set_sector_usage( T, N );
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

bool f2fs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkfs.f2fs -l " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
	                          " " + Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool f2fs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	Glib::ustring str_temp = "resize.f2fs " + Glib::shell_quote( partition_new.get_path() );

	if ( ! fill_partition )
		str_temp += " -t " + Utils::num_to_str( floor( Utils::sector_to_unit(
					partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) );

	return ! execute_command( str_temp, operationdetail, EXEC_CHECK_STATUS );
}

bool f2fs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "fsck.f2fs -f -y -a " + Glib::shell_quote( partition.get_path() ),
	                               operationdetail, EXEC_CANCEL_SAFE );
	bool success = ( exit_status == 0 || exit_status == 1 || exit_status == 2 );
	set_status( operationdetail, success );
	return success;
}

} //GParted
