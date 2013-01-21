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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "../include/ext2.h"

namespace GParted
{

FS ext2::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = specific_type;

	//Only enable any functionality if the relevant mkfs.extX command is
	//  found to ensure that the version of e2fsprogs is new enough to
	//  support ext4.  Applying to ext2/3 too is OK as relevant mkfs.ext2/3
	//  commands exist.
	if ( ! Glib::find_program_in_path( "mkfs." + Utils::get_filesystem_string( specific_type ) ).empty() )
	{
		fs .create = FS::EXTERNAL ;

		if ( ! Glib::find_program_in_path( "dumpe2fs" ) .empty() )
			fs .read = FS::EXTERNAL ;

		if ( ! Glib::find_program_in_path( "tune2fs" ) .empty() ) {
			fs .read_uuid = FS::EXTERNAL ;
			fs .write_uuid = FS::EXTERNAL ;
		}

		if ( ! Glib::find_program_in_path( "e2label" ) .empty() ) {
			fs .read_label = FS::EXTERNAL ;
			fs .write_label = FS::EXTERNAL ;
		}

		if ( ! Glib::find_program_in_path( "e2fsck" ) .empty() )
			fs .check = FS::EXTERNAL ;
	
		if ( ! Glib::find_program_in_path( "resize2fs" ) .empty() && fs .check )
		{
			fs .grow = FS::EXTERNAL ;

			if ( fs .read ) //needed to determine a min file system size..
				fs .shrink = FS::EXTERNAL ;
		}

		if ( fs .check )
		{
			fs .copy = FS::GPARTED ;
			fs .move = FS::GPARTED ;
		}

		fs .online_read = FS::EXTERNAL ;
	}

	return fs ;
}

void ext2::set_used_sectors( Partition & partition ) 
{
	//Called when file system is unmounted *and* when mounted.  Always read
	//  the file system size from the on disk superblock using dumpe2fs to
	//  avoid overhead subtraction.  Read the free space from the kernel via
	//  the statvfs() system call when mounted and from the superblock when
	//  unmounted.
	if ( ! Utils::execute_command( "dumpe2fs -h " + partition .get_path(), output, error, true ) )
	{
		index = output .find( "Block count:" ) ;
		if ( index >= output .length() ||
		     sscanf( output.substr( index ) .c_str(), "Block count: %Ld", &T ) != 1 )
			T = -1 ;

		index = output .find( "Block size:" ) ;
		if ( index >= output.length() || 
		     sscanf( output.substr( index ) .c_str(), "Block size: %Ld", &S ) != 1 )  
			S = -1 ;

		if ( T > -1 && S > -1 )
			T = Utils::round( T * ( S / double(partition .sector_size) ) ) ;

		if ( partition .busy )
		{
			Byte_Value ignored ;
			Byte_Value fs_free ;
			if ( Utils::get_mounted_filesystem_usage( partition .get_mountpoint(),
			                                          ignored, fs_free, error ) == 0 )
				N = Utils::round( fs_free / double(partition .sector_size) ) ;
			else
			{
				N = -1 ;
				partition .messages .push_back( error ) ;
			}
		}
		else
		{
			index = output .find( "Free blocks:" ) ;
			if ( index >= output .length() ||
			     sscanf( output.substr( index ) .c_str(), "Free blocks: %Ld", &N ) != 1 )
				N = -1 ;

			if ( N > -1 && S > -1 )
				N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;
		}

		if ( T > -1 && N > -1 )
			partition .set_sector_usage( T, N ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}
	
void ext2::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "e2label " + partition .get_path(), output, error, true ) )
	{
		partition .set_label( Utils::trim( output ) ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool ext2::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "e2label " + partition .get_path() + " \"" + partition .get_label() + "\"", operationdetail ) ;
}

void ext2::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( "tune2fs -l " + partition .get_path(), output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "^Filesystem UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool ext2::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "tune2fs -U random " + partition .get_path(), operationdetail ) ;
}

bool ext2::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkfs." + Utils::get_filesystem_string( specific_type ) +
				  " -L \"" + new_partition.get_label() + "\" " + new_partition.get_path(),
				  operationdetail,
				  false,
				  true );
}

bool ext2::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	Glib::ustring str_temp = "resize2fs " + partition_new .get_path() ;
	
	if ( ! fill_partition )
		str_temp += " " + Utils::num_to_str( Utils::round( Utils::sector_to_unit( 
					partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) -1 ) + "K" ; 
		
	return ! execute_command( str_temp, operationdetail ) ;
}

bool ext2::move( const Partition & partition_new
               , const Partition & partition_old
               , OperationDetail & operationdetail
               )
{
	return true ;
}

bool ext2::copy( const Glib::ustring & src_part_path, 
		 const Glib::ustring & dest_part_path,
		 OperationDetail & operationdetail )
{
	return true ;
}

bool ext2::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "e2fsck -f -y -v " + partition.get_path(), operationdetail,
				       false, true );

	//exitstatus 256 isn't documented, but it's returned when the 'FILE SYSTEM IS MODIFIED'
	//this is quite normal (especially after a copy) so we let the function return true...
	return ( exit_status == 0 || exit_status == 1 || exit_status == 2 || exit_status == 256 ) ;
}

bool ext2::remove( const Partition & partition, OperationDetail & operationdetail )
{
	return true ;
}

} //GParted


