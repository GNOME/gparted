/* Copyright (C) 2009,2010 Luca Bruno <lucab@debian.org>
 * Copyright (C) 2010, 2011 Curtis Gedak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "../include/btrfs.h"

namespace GParted
{

bool btrfs_found = false ;

FS btrfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_BTRFS ;

	if ( ! Glib::find_program_in_path( "mkfs.btrfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "btrfsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;

	btrfs_found = ( ! Glib::find_program_in_path( "btrfs" ) .empty() ) ;
	if ( btrfs_found )
	{
		//Use newer btrfs multi-tool control command.  No need
		//  to test for filesystem show and filesystem resize
		//  sub-commands as they were always included.

		fs .read = GParted::FS::EXTERNAL ;
		fs .read_label = FS::EXTERNAL ;

		//Resizing of btrfs requires mount, umount and kernel
		//  support as well as btrfs filesystem resize
		if (    ! Glib::find_program_in_path( "mount" ) .empty()
		     && ! Glib::find_program_in_path( "umount" ) .empty()
		     && fs .check
		     && Utils::kernel_supports_fs( "btrfs" )
		   )
		{
			fs .grow = FS::EXTERNAL ;
			if ( fs .read ) //needed to determine a minimum file system size.
				fs .shrink = FS::EXTERNAL ;
		}

		//Test for labelling capability in btrfs command
		if ( ! Utils::execute_command( "btrfs filesystem label --help", output, error, true ) )
			fs .write_label = FS::EXTERNAL;
	}
	else
	{
		//Fall back to using btrfs-show and btrfsctl, which
		//  were depreciated October 2011

		if ( ! Glib::find_program_in_path( "btrfs-show" ) .empty() )
		{
			fs .read = GParted::FS::EXTERNAL ;
			fs .read_label = FS::EXTERNAL ;
		}

		//Resizing of btrfs requires btrfsctl, mount, umount
		//  and kernel support
		if (    ! Glib::find_program_in_path( "btrfsctl" ) .empty()
		     && ! Glib::find_program_in_path( "mount" ) .empty()
		     && ! Glib::find_program_in_path( "umount" ) .empty()
		     && fs .check
		     && Utils::kernel_supports_fs( "btrfs" )
		   )
		{
			fs .grow = FS::EXTERNAL ;
			if ( fs .read ) //needed to determine a minimum file system size.
				fs .shrink = FS::EXTERNAL ;
		}
	}

	if ( fs .check )
	{
		fs .copy = GParted::FS::GPARTED ;
		fs .move = GParted::FS::GPARTED ;
	}

	fs .MIN = 256 * MEBIBYTE ;

	return fs ;
}

bool btrfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return (! execute_command( "mkfs.btrfs -L \"" + new_partition .label + "\" " + new_partition .get_path(), operationdetail ) );
}

bool btrfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return (! execute_command( "btrfsck " + partition .get_path(), operationdetail )) ;
}

void btrfs::set_used_sectors( Partition & partition )
{

	if ( btrfs_found )
		exit_status = Utils::execute_command( "btrfs filesystem show " + partition .get_path(), output, error, true ) ;
	else
		exit_status = Utils::execute_command( "btrfs-show " + partition .get_path(), output, error, true ) ;
	if ( ! exit_status )
	{
		Glib::ustring size_label;
		if ( ((index = output .find( "FS bytes" )) < output .length()) &&
			  (size_label=Utils::regexp_label(output .substr( index ), "^FS bytes used (.*)B")) .length() > 0)
		{
			gchar *suffix;
			gdouble rawN = g_ascii_strtod (size_label.c_str(),&suffix);
			unsigned long long mult=0;
			switch(suffix[0]){
				case 'K':
					mult=KIBIBYTE;
					break;
				case 'M':
					mult=MEBIBYTE;
					break;
				case 'G':
					mult=GIBIBYTE;
					break;
				case 'T':
					mult=TEBIBYTE;
					break;
				default:
					mult=1;
					break;
					}
			partition .set_used( Utils::round( (rawN * mult)/ double(partition .sector_size) ) ) ;
		}
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool btrfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "btrfs filesystem label " + partition .get_path() + " \"" + partition .label + "\"", operationdetail ) ;
}

bool btrfs::move( const Partition & partition_new
                , const Partition & partition_old
                , OperationDetail & operationdetail
                )
{
	return true ;
}

bool btrfs::copy( const Glib::ustring & src_part_path,
                    const Glib::ustring & dest_part_path,
                    OperationDetail & operationdetail )
{
// TODO
        return true ;
}

bool btrfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;

	Glib::ustring mount_point = mk_temp_dir( "", operationdetail ) ;
	if ( mount_point .empty() )
		return false ;

	success &= ! execute_command_timed( "mount -v -t btrfs " + partition_new .get_path() + " " + mount_point, operationdetail ) ;

	if ( success )
	{
		Glib::ustring size ;
		if ( ! fill_partition )
			size = Utils::num_to_str( floor( Utils::sector_to_unit(
					partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) ) + "K" ;
		else
			size = "max" ;
		Glib::ustring cmd ;
		if ( btrfs_found )
			cmd = "btrfs filesystem resize " + size + " " + mount_point ;
		else
			cmd = "btrfsctl -r " + size + " " + mount_point ;
		exit_status = execute_command_timed( cmd, operationdetail, false ) ;
		//Resizing a btrfs file system to the same size results
		//  in ioctl() returning -1 EINVAL (Invalid argument)
		//  from the kernel btrfs code.
		//  *   Btrfs filesystem resize reports this as exit
		//      status 30:
		//          ERROR: Unable to resize '/MOUNTPOINT'
		//  *   Btrfsctl -r reports this as exit status 1:
		//          ioctl:: Invalid argument
		//  WARNING:
		//  Ignoring these errors could mask real failures, but
		//  not ignoring them will cause resizing to the same
		//  size as part of check operation to fail.
		bool resize_succeeded = (    exit_status == 0
		                          || (   btrfs_found && exit_status == 30<<8 )
		                          || ( ! btrfs_found && exit_status ==  1<<8 )
		                        ) ;
		operationdetail .get_last_child() .set_status( resize_succeeded ? STATUS_SUCCES : STATUS_ERROR ) ;
		success &= resize_succeeded ;

		success &= ! execute_command_timed( "umount -v " + mount_point, operationdetail ) ;
	}

	rm_temp_dir( mount_point, operationdetail ) ;

	return success ;
}

void btrfs::read_label( Partition & partition )
{
	if ( btrfs_found )
	{
		exit_status = Utils::execute_command( "btrfs filesystem show " + partition .get_path(), output, error, true ) ;
		if ( ! exit_status )
		{
			partition .label = Utils::regexp_label( output, "^Label: '(.*)'  uuid:" );
			//Btrfs filesystem show encloses the label in single
			//  quotes or reports "none" without single quotes, so
			//  the cases are disginguishable and this regexp won't
			//  match the no label case.
		}
	}
	else
	{
		exit_status = Utils::execute_command( "btrfs-show " + partition .get_path(), output, error, true ) ;
		if ( ! exit_status )
		{
			Glib::ustring label = Utils::regexp_label( output, "^Label: (.*)  uuid:" ) ;
			//Btrfs-show reports "none" when there is no label, but
			//  this is indistinguishable from the label actually
			//  being "none".  Assume no label case.
			if ( label != "none" )
				partition .label = label ;
		}
	}
	if ( exit_status )
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

} //GParted
