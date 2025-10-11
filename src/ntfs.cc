/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
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

#include "ntfs.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <sigc++/signal.h>


namespace GParted
{


const Glib::ustring & ntfs::get_custom_text( CUSTOM_TEXT ttype, int index ) const
{
	static const Glib::ustring change_uuid_warning[] =
		{ _("Changing the UUID might invalidate the Windows Product Activation "
		    "(WPA) key"),
		  _("On FAT and NTFS file systems, the Volume Serial Number is used as "
		    "the UUID. Changing the Volume Serial Number on the Windows system "
		    "partition, normally C:, might invalidate the WPA key. An invalid "
		    "WPA key will prevent login until you reactivate Windows."),
		  _("In an attempt to avoid invalidating the WPA key, on NTFS file "
		    "systems only half of the UUID is set to a new random value."),
		  _("Changing the UUID on external storage media and non-system "
		    "partitions is usually safe, but guarantees cannot be given."),
		  ""
		};

	int i ;
	switch ( ttype ) {
		case CTEXT_CHANGE_UUID_WARNING :
			for ( i = 0 ; i < index && change_uuid_warning[i] != "" ; i++ )
				;  // Just iterate...
			return change_uuid_warning[i];
		default :
			return FileSystem::get_custom_text( ttype, index ) ;
	}
}

FS ntfs::get_filesystem_support()
{
	FS fs( FS_NTFS );

	fs .busy = FS::GPARTED ;

	if (! Glib::find_program_in_path("ntfsinfo").empty())
		fs.read = FS::EXTERNAL;

	if (! Glib::find_program_in_path("ntfslabel").empty())
	{
		fs .read_label = FS::EXTERNAL ;
		fs .write_label = FS::EXTERNAL ;
		fs.write_uuid = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "mkntfs" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	//resizing is a delicate process ...
	if (! Glib::find_program_in_path("ntfsresize").empty())
	{
		fs.check = FS::EXTERNAL;
		fs.grow = FS::EXTERNAL;

		if ( fs .read ) //needed to determine a min file system size..
			fs.shrink = FS::EXTERNAL;

		fs.move = FS::GPARTED;
	}

	if ( ! Glib::find_program_in_path( "ntfsclone" ) .empty() )
		fs.copy = FS::EXTERNAL;

	fs .online_read = FS::GPARTED ;

	//Minimum NTFS partition size = (Minimum NTFS volume size) + (backup NTFS boot sector)
	//                            = (1 MiB) + (1 sector)
	// For GParted this means 2 MiB because smallest GUI unit is MiB.
	m_fs_limits.min_size = 2 * MEBIBYTE;

	return fs ;
}


void ntfs::set_used_sectors( Partition & partition ) 
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("ntfsinfo --mft --force " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	long long cluster_size = -1;
	Glib::ustring::size_type index = output.find("Cluster Size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Cluster Size: %lld", &cluster_size);

	long long volume_size = -1;
	index = output.find("Volume Size in Clusters:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Volume Size in Clusters: %lld", &volume_size);

	long long free_clusters = -1;
	index = output.find("Free Clusters:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "Free Clusters: %lld", &free_clusters);

	if (cluster_size > -1 && volume_size > -1 && free_clusters > -1)
	{
		Sector fs_size = volume_size * cluster_size / partition.sector_size;
		Sector fs_free = free_clusters * cluster_size / partition.sector_size;
		partition.set_sector_usage(fs_size, fs_free);
		partition.fs_block_size = cluster_size;
	}
}


void ntfs::read_label( Partition & partition )
{
	Glib::ustring output;
	Glib::ustring error;
	if ( ! Utils::execute_command( "ntfslabel --force " + Glib::shell_quote( partition.get_path() ),
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


bool ntfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("ntfslabel --force " + Glib::shell_quote(partition.get_path()) +
	                        " " + Glib::shell_quote(partition.get_filesystem_label()),
	                        EXEC_CHECK_STATUS);
}


void ntfs::read_uuid( Partition & partition )
{
}


bool ntfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition .uuid == UUID_RANDOM_NTFS_HALF )
		return ! operationdetail.execute_command("ntfslabel --new-half-serial " +
		                        Glib::shell_quote(partition.get_path()),
		                        EXEC_CHECK_STATUS);
	else
		return ! operationdetail.execute_command("ntfslabel --new-serial " +
		                        Glib::shell_quote(partition.get_path()),
		                        EXEC_CHECK_STATUS);

	return true ;
}

bool ntfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkntfs -Q -v -F -L " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


bool ntfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success;
	Glib::ustring size;
	if ( ! fill_partition )
		size = " -s " + Utils::num_to_str(partition_new.get_byte_length());
	Glib::ustring cmd = "ntfsresize --force --force" + size ;

	//simulation..
	operationdetail .add_child( OperationDetail( _("run simulation") ) ) ;
	OperationDetail& child_od = operationdetail.get_last_child();
	success = ! child_od.execute_command(cmd + " --no-action " + Glib::shell_quote(partition_new.get_path()),
	                        EXEC_CHECK_STATUS);
	child_od.set_success_and_capture_errors(success);
	if ( ! success )
		return false;

	// Real resize
	operationdetail.add_child( OperationDetail( _("real resize") ) );
	child_od = operationdetail.get_last_child();
	success = ! child_od.execute_command(cmd + " " + Glib::shell_quote(partition_new.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_PROGRESS_STDOUT,
	                        static_cast<StreamSlot>(sigc::mem_fun(*this, &ntfs::resize_progress)));
	child_od.set_success_and_capture_errors(success);
	return success;
}


bool ntfs::copy( const Partition & src_part,
		 Partition & dest_part, 
		 OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("ntfsclone -f --overwrite " + Glib::shell_quote(dest_part.get_path()) +
	                        " " + Glib::shell_quote(src_part.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE|EXEC_PROGRESS_STDOUT,
	                        static_cast<StreamSlot>(sigc::mem_fun(*this, &ntfs::clone_progress)));
}


bool ntfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("ntfsresize -i -f -v " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


//Private methods

void ntfs::resize_progress( OperationDetail *operationdetail )
{
	const Glib::ustring& output = operationdetail->get_command_output();
	Glib::ustring line = Utils::last_line( output );
	// Text progress on the LAST LINE looks like " 15.24 percent completed"
	// NOTE:
	// Specifying text to match following the last converted variable in *scanf() is
	// ineffective because it reports the number of successfully converted variables
	// and has no way to indicate following text didn't match.  Specifically:
	//     sscanf( "%f percent completed", &percent )
	// will return 1 after successfully converting percent variable as 4 given this
	// line from the important information at the end of the ntfsresize output:
	//     "  4)  set the bootable flag for the partit"
	// Definitely not progress information.
	float percent;
	if ( line.find( "percent completed" ) != line.npos && sscanf( line.c_str(), "%f", &percent ) == 1 )
	{
		operationdetail->run_progressbar( percent, 100.0 );
	}
	// Or when finished, on any line, ...
	else if ( output.find( "Successfully resized NTFS on device" ) != output.npos )
	{
		operationdetail->stop_progressbar();
	}
}


void ntfs::clone_progress( OperationDetail *operationdetail )
{
	const Glib::ustring& output = operationdetail->get_command_output();
	Glib::ustring line = Utils::last_line( output );
	// Text progress on the LAST LINE looks like " 15.24 progress completed"
	float percent;
	if ( line.find( "percent completed" ) != line.npos && sscanf( line.c_str(), "%f", &percent ) == 1 )
	{
		operationdetail->run_progressbar( percent, 100.0 );
	}
	// Deliberately don't stop the progress bar when ntfsclone outputs "Syncing ..."
	// at the end as that is considered a measured part of the copy operation.  The
	// progress bar will wait at 100% (or just below) until the sync completes.  On
	// spinning rust that is typically a few seconds and on SSDs it won't be noticed
	// at all.  Instead it is left for execute_command(), which always stops the
	// progress bar when the command finishes.
}


}  // namespace GParted
