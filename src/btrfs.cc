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

FS btrfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_BTRFS ;

	if ( ! Glib::find_program_in_path( "btrfs-show" ) .empty() )
	{
		fs .read = GParted::FS::EXTERNAL ;
		fs .read_label = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "mkfs.btrfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "btrfsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;

	//resizing of btrfs requires btrfsctl, mount, and umount
	if (    ! Glib::find_program_in_path( "btrfsctl" ) .empty()
	     && ! Glib::find_program_in_path( "mount" ) .empty()
	     && ! Glib::find_program_in_path( "umount" ) .empty()
	     && fs .check
	   )
	{
		fs .grow = FS::EXTERNAL ;
		if ( fs .read ) //needed to determine a minimum file system size.
			fs .shrink = FS::EXTERNAL ;
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
	if ( ! Utils::execute_command( "btrfs-show " + partition .get_path(), output, error, true ) )
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
// TODO
        return true ;
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
	//Create directory
	Glib::ustring str_temp = _( "create temporary directory" ) ;
	operationdetail .add_child( OperationDetail( str_temp, STATUS_NONE, FONT_BOLD_ITALIC ) ) ;
	char dtemplate[] = "/tmp/gparted-XXXXXXXX" ;
	Glib::ustring dname =  mkdtemp( dtemplate ) ;
	if( dname .empty() )
	{
		//Failed to create temporary directory
		str_temp = _( "Failed to create temporary directory." );
		operationdetail .get_last_child() .add_child( OperationDetail( str_temp, STATUS_NONE, FONT_ITALIC ) ) ;
		return false ;
	}

	/*TO TRANSLATORS: looks like  Created temporary directory /tmp/gparted-XXCOOO8U */
	str_temp = String::ucompose ( _("Created temporary directory %1"), dname ) ;
	operationdetail .get_last_child() .add_child( OperationDetail( str_temp, STATUS_NONE, FONT_ITALIC ) ) ;

	//Mount file system.  Needed to resize btrfs.
	str_temp = "mount -v -t btrfs " + partition_new .get_path() + " " + dname ;
	exit_status = ! execute_command( str_temp, operationdetail ) ;

	if ( exit_status )
	{
		//Build resize command
		str_temp = "btrfsctl" ;
		if ( ! fill_partition )
		{
			str_temp += " -r " + Utils::num_to_str( floor( Utils::sector_to_unit(
						partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) ) + "K" ;
		}
		else
			str_temp += " -r max" ;
		str_temp += " " + dname ;

		//Execute the command
		exit_status = execute_command( str_temp, operationdetail ) ;
		//Sometimes btrfsctl returns an exit status of 256 on successful resize.
		exit_status = ( exit_status == 0 || exit_status == 256 ) ;

		//Always unmount the file system
		str_temp = "umount -v " + dname ;
		execute_command( str_temp, operationdetail ) ;
	}

	//Always remove the temporary directory name
	str_temp = "rmdir -v " + dname ;
	execute_command( str_temp, operationdetail ) ;

	return exit_status ;
}

void btrfs::read_label( Partition & partition )
{
        if ( ! Utils::execute_command( "btrfs-show " + partition .get_path(), output, error, true ) )
        {
                partition .label = Utils::regexp_label( output, "^Label ([^\n\t]*)" ) ;
        }
        else
        {
                if ( ! output .empty() )
                        partition .messages .push_back( output ) ;

                if ( ! error .empty() )
                        partition .messages .push_back( error ) ;
        }

}

} //GParted
