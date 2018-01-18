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

namespace GParted
{

FS f2fs::get_filesystem_support()
{
	FS fs( FS_F2FS );

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "mkfs.f2fs" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
	}

	fs .copy = FS::GPARTED ;
	fs .move = FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	return fs ;
}

bool f2fs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkfs.f2fs -l " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
	                          " " + Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

} //GParted
