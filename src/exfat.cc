/* Copyright (C) 2011 Curtis Gedak
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

#include "exfat.h"
#include "FileSystem.h"
#include "Partition.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>

namespace GParted
{

FS exfat::get_filesystem_support()
{
	FS fs( FS_EXFAT );

	if ( ! Glib::find_program_in_path( "mkexfatfs" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "exfatfsck" ).empty() )
		fs.check = FS::EXTERNAL;

	if ( ! Glib::find_program_in_path( "dumpexfat").empty() )
	{
		fs.read = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "exfatlabel" ) .empty() ) {
		Glib::ustring version ;

		fs .read_label = FS::EXTERNAL ;
		fs .write_label = FS::EXTERNAL ;
	}

	fs .busy = FS::GPARTED ;
	fs .copy = FS::GPARTED ;
	fs .move = FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	return fs ;
}


void exfat::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "exfatlabel " + Glib::shell_quote( partition.get_path() ),
	                               output, error, false )                                            )
	{
		partition.set_filesystem_label( Utils::trim( output ) );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool exfat::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "exfatlabel " + Glib::shell_quote( partition.get_path() ) +
	                          " " + Glib::shell_quote( partition.get_filesystem_label() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

void exfat::set_used_sectors( Partition & partition )
{
	if ( ! Utils::execute_command( "dumpexfat " + Glib::shell_quote( partition.get_path() ), output, error, true ))
	{

		Glib::ustring temp;
		temp = Utils::regexp_label( output, "Sector size\\s+([0-9]+)" ) ;
		sscanf( temp.c_str(), "%lld", &S );

		temp = Utils::regexp_label( output, "Sectors count\\s+([0-9]+)" ) ;
		sscanf( temp.c_str(), "%lld", &T );

		temp = Utils::regexp_label( output, "Free sectors\\s+([0-9]+)" ) ;
		sscanf( temp.c_str(), "%lld", &N );


		partition.set_sector_usage( T, N );
		partition.fs_block_size = S;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


bool exfat::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkexfatfs -n " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
	                          " " + Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool exfat::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "exfatfsck -y -a " + Glib::shell_quote( partition.get_path() ),
	                               operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

} //GParted
