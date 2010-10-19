/* Copyright (C) 2009,2010 Luca Bruno <lucab@debian.org>
 * Copyright (C) 2010 Curtis Gedak
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
// TODO
        return true ;
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
