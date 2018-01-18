/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Curtis Gedak
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

#include "hfs.h"
#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

FS hfs::get_filesystem_support()
{
	FS fs( FS_HFS );

	fs .busy = FS::GPARTED ;

#ifdef HAVE_LIBPARTED_FS_RESIZE
	fs .read = GParted::FS::LIBPARTED ;
	fs .shrink = GParted::FS::LIBPARTED ;
#endif

	if ( ! Glib::find_program_in_path( "hformat" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "hfsck" ) .empty() )
		fs .check = FS::EXTERNAL ;

	fs .copy = GParted::FS::GPARTED ;
	fs .move = GParted::FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	fs_limits.max_size = 2048 * MEBIBYTE;

	return fs ;
}

bool hfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "";
	if( new_partition.get_filesystem_label().empty() )
		cmd = "hformat " + Glib::shell_quote( new_partition.get_path() );
	else
		cmd = "hformat -l " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
		      " " + Glib::shell_quote( new_partition.get_path() );
	return ! execute_command( cmd , operationdetail, EXEC_CHECK_STATUS );
}

bool hfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	//FIXME: find out what the returnvalue is in case of modified.. also check what the -a flag does.. (there is no manpage)
	return ! execute_command( "hfsck -v " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

} //GParted
