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

#include "reiser4.h"
#include "FileSystem.h"
#include "Partition.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


FS reiser4::get_filesystem_support()
{
	FS fs( FS_REISER4 );

	fs .busy = FS::GPARTED ;

	if ( ! Glib::find_program_in_path( "debugfs.reiser4" ) .empty() )
	{
		fs.read = FS::EXTERNAL;
		fs .read_label = FS::EXTERNAL ;
		fs.read_uuid = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "mkfs.reiser4" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "fsck.reiser4" ) .empty() )
		fs.check = FS::EXTERNAL;

	if ( fs .check )
	{
		fs.copy = FS::GPARTED;
		fs.move = FS::GPARTED;
	}

	fs .online_read = FS::GPARTED ;

	/*
	 * IT SEEMS RESIZE AND COPY AREN'T IMPLEMENTED YET IN THE TOOLS...
	 * SEE http://marc.theaimsgroup.com/?t=109883161600003&r=1&w=2 for more information.. 
	 */
	  
	return fs ;
}


void reiser4::set_used_sectors(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("debugfs.reiser4 " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	long long blocks = -1;
	Glib::ustring::size_type index = output.find("\nblocks:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nblocks: %lld", &blocks);

	long long free_blocks = -1;
	index = output.find("\nfree blocks:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nfree blocks: %lld", &free_blocks);

	long long block_size = -1;
	index = output.find("\nblksize:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nblksize: %lld", &block_size);

	if (blocks > -1 && free_blocks > -1 && block_size > -1)
	{
		Sector fs_size = blocks * block_size / partition.sector_size;
		Sector fs_free = free_blocks * block_size / partition.sector_size;
		partition.set_sector_usage(fs_size, fs_free);
		partition.fs_block_size = block_size;
	}
}


void reiser4::read_label( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "debugfs.reiser4 " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                           )
	{
		Glib::ustring::size_type maxlen = Utils::get_filesystem_label_maxlength( FS_REISER4 ) ;
		Glib::ustring label = Utils::regexp_label( output, "^label:[[:blank:]]*(.*)$" ) ;
		//Avoid reading any trailing junk after the label
		if ( label .length() > maxlen )
			label .resize( maxlen ) ;
		if ( label != "<none>" )
			partition.set_filesystem_label( label );
		else
			partition.set_filesystem_label( "" );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}


void reiser4::read_uuid( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("debugfs.reiser4 " + Glib::shell_quote( partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	Glib::ustring pattern = "uuid:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")";
	// Reiser4progs >= 2.0.3 supports subvolumes.  Ensure the volume UUID, rather than
	// subvolume UUID, is reported.
	if (output.find("volume uuid:") != Glib::ustring::npos)
		pattern = "volume " + pattern;
	partition.uuid = Utils::regexp_label(output, pattern);
}


bool reiser4::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkfs.reiser4 --force --yes --label " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


bool reiser4::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("fsck.reiser4 --yes --fix --quiet " +
	                        Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


}  // namespace GParted
