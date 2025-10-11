/* Copyright (C) 2018 Mike Fleetwood
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

#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"
#include "minix.h"

#include <glibmm.h>


namespace GParted
{


FS minix::get_filesystem_support()
{
	FS fs( FS_MINIX );

	fs.busy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs.copy = FS::GPARTED;
	fs.online_read = FS::GPARTED;

	if ( ! Glib::find_program_in_path( "mkfs.minix").empty() )
		fs.create = FS::EXTERNAL;

	if ( ! Glib::find_program_in_path( "fsck.minix").empty() )
		fs.check = FS::EXTERNAL;

	return fs;
}


bool minix::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkfs.minix -3 " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool minix::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("fsck.minix " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


}  // namespace GParted
