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

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


FS hfs::get_filesystem_support()
{
	FS fs( FS_HFS );

	fs .busy = FS::GPARTED ;
	fs.read = FS::LIBPARTED;
	fs.shrink = FS::LIBPARTED;

	if ( ! Glib::find_program_in_path( "hformat" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "hfsck" ) .empty() )
		fs .check = FS::EXTERNAL ;

	fs.copy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs .online_read = FS::GPARTED ;

	m_fs_limits.max_size = 2048 * MEBIBYTE;

	return fs ;
}

bool hfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd;
	if( new_partition.get_filesystem_label().empty() )
		cmd = "hformat " + Glib::shell_quote( new_partition.get_path() );
	else
		cmd = "hformat -l " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
		      " " + Glib::shell_quote( new_partition.get_path() );
	return ! operationdetail.execute_command(cmd, EXEC_CHECK_STATUS);
}


bool hfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	//FIXME: find out what the returnvalue is in case of modified.. also check what the -a flag does.. (there is no manpage)
	return ! operationdetail.execute_command("hfsck -v " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


}  // namespace GParted
