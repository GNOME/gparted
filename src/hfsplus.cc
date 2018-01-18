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

#include "hfsplus.h"
#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

FS hfsplus::get_filesystem_support()
{
	FS fs( FS_HFSPLUS );

	fs .busy = FS::GPARTED ;

#ifdef HAVE_LIBPARTED_FS_RESIZE
	fs .read = GParted::FS::LIBPARTED ;
	fs .shrink = GParted::FS::LIBPARTED ;
#endif

	if ( ! Glib::find_program_in_path( "mkfs.hfsplus" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "fsck.hfsplus" ) .empty() )
		fs .check = FS::EXTERNAL ;

	fs .copy = GParted::FS::GPARTED ;
	fs .move = GParted::FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	return fs ;
}

bool hfsplus::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "";
	if( new_partition.get_filesystem_label().empty() )
		cmd = "mkfs.hfsplus " + Glib::shell_quote( new_partition.get_path() );
	else
		cmd = "mkfs.hfsplus -v " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
		      " " + Glib::shell_quote( new_partition.get_path() );
	return ! execute_command( cmd , operationdetail, EXEC_CHECK_STATUS );
}

bool hfsplus::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "fsck.hfsplus -f -y " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

} //GParted
