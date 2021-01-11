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
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


FS exfat::get_filesystem_support()
{
	FS fs( FS_EXFAT );

	fs .busy = FS::GPARTED ;
	fs .copy = FS::GPARTED ;
	fs .move = FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	if (! Glib::find_program_in_path("mkfs.exfat").empty())
		fs.create = FS::EXTERNAL;

	if (! Glib::find_program_in_path("tune.exfat").empty())
	{
		fs.read_label = FS::EXTERNAL;
		fs.write_label = FS::EXTERNAL;
	}

	if (! Glib::find_program_in_path("fsck.exfat").empty())
		fs.check = FS::EXTERNAL;

	return fs;
}


bool exfat::create(const Partition& new_partition, OperationDetail& operationdetail)
{
	return ! execute_command("mkfs.exfat -L " + Glib::shell_quote(new_partition.get_filesystem_label()) +
	                         " " + Glib::shell_quote(new_partition.get_path()),
	                         operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


void exfat::read_label(Partition& partition)
{
	exit_status = Utils::execute_command("tune.exfat -l " + Glib::shell_quote(partition.get_path()),
	                                     output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	partition.set_filesystem_label(Utils::regexp_label(output, "^label: ([^\n]*)"));
}


bool exfat::write_label(const Partition& partition, OperationDetail& operationdetail)
{
	return ! execute_command("tune.exfat -L " + Glib::shell_quote(partition.get_filesystem_label()) +
	                         " " + Glib::shell_quote(partition.get_path()),
	                         operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


bool exfat::check_repair(const Partition& partition, OperationDetail& operationdetail)
{
	return ! execute_command("fsck.exfat -v " + Glib::shell_quote(partition.get_path()),
	                         operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


} //GParted
