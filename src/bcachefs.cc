/* Copyright (C) 2024 Mike Fleetwood
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


#include "bcachefs.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

# include <glibmm/miscutils.h>
# include <glibmm/shell.h>


namespace GParted
{


FS bcachefs::get_filesystem_support()
{
	FS fs(FS_BCACHEFS);

	fs.busy = FS::GPARTED;
	fs.move = FS::GPARTED;
	fs.copy = FS::GPARTED;
	fs.online_read = FS::GPARTED;

	if (! Glib::find_program_in_path("bcachefs").empty())
	{
		fs.create            = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	fs_limits.min_size = 16 * MEBIBYTE;

	return fs;
}


bool bcachefs::create(const Partition& new_partition, OperationDetail& operationdetail)
{
	return ! execute_command("bcachefs format -L " + Glib::shell_quote(new_partition.get_filesystem_label()) +
	                         " " + Glib::shell_quote(new_partition.get_path()),
				 operationdetail, EXEC_CHECK_STATUS);
}


} //GParted
