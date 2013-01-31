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
 
 
#include "../include/ntfs.h"

namespace GParted
{

const Glib::ustring ntfs::Change_UUID_Warning [] =
	{ _( "Changing the UUID might invalidate the Windows Product Activation (WPA) key"
	   )
	, _( "On FAT and NTFS file systems, the"
	     " Volume Serial Number is used as the UUID."
	     " Changing the Volume Serial Number on the Windows system"
	     " partition, normally C:, might invalidate the WPA key."
	     " An invalid WPA key will prevent login until you reactivate Windows."
	   )
	, _( "In an attempt to avoid invalidating the WPA key, on"
	     " NTFS file systems only half of the UUID is set to a"
	     " new random value."
	   )
	, _( "Changing the UUID on external storage media and non-system partitions"
	     " is usually safe, but guarantees cannot be given."
	   )
	,    ""
	} ;

const Glib::ustring ntfs::get_custom_text( CUSTOM_TEXT ttype, int index )
{
	int i ;
	switch ( ttype ) {
		case CTEXT_CHANGE_UUID_WARNING :
			for ( i = 0 ; i < index && Change_UUID_Warning[ i ] != "" ; i++ ) {
				// Just iterate...
			}
			return Change_UUID_Warning [ i ] ;
		default :
			return FileSystem::get_custom_text( ttype, index ) ;
	}
}

FS ntfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_NTFS ;

	if ( ! Glib::find_program_in_path( "ntfsresize" ) .empty() )
	{
		fs .read = GParted::FS::EXTERNAL ;
		fs .check = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "ntfslabel" ) .empty() ) {
		Glib::ustring version ;

		fs .read_label = FS::EXTERNAL ;
		fs .write_label = FS::EXTERNAL ;

		//Not all versions of ntfslabel support setting the Volume Serial Number
		//The advanced (AR) release does as of the first 2012 release
		//The stable release does not have it yet, at the time of this writing
		//So: check for the presence of the command-line option.

		//ntfslabel --help exits with non-zero error code (1)
		Utils::execute_command( "ntfslabel --help ", output, error, false ) ;

		if ( ! ( version = Utils::regexp_label( output, "--new-serial[[:blank:]]" ) ) .empty() )
			fs .write_uuid = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "mkntfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;

	//resizing is a delicate process ...
	if ( fs .read && fs .check )
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min file system size..
			fs .shrink = GParted::FS::EXTERNAL ;
	}

	//we need ntfsresize to set correct used/unused after cloning
	if ( ! Glib::find_program_in_path( "ntfsclone" ) .empty() )
		fs .copy = GParted::FS::EXTERNAL ;

	if ( fs .check )
		fs .move = GParted::FS::GPARTED ;

	fs .online_read = FS::GPARTED ;

	return fs ;
}

void ntfs::set_used_sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( 
		"ntfsresize --info --force --no-progress-bar " + partition .get_path(), output, error, true ) )
	{
		index = output .find( "Current volume size:" ) ;
		if ( index >= output .length() ||
		     sscanf( output .substr( index ) .c_str(), "Current volume size: %Ld", &T ) != 1 )
			T = -1 ;

		index = output .find( "resize at" ) ;
		if ( index >= output .length() ||
		     sscanf( output .substr( index ) .c_str(), "resize at %Ld", &N ) != 1 )
			N = -1 ;

		if ( T > -1 && N > -1 )
		{
			T = Utils::round( T / double(partition .sector_size) ) ;
			N = Utils::round( N / double(partition .sector_size) ) ;
			partition .set_sector_usage( T, T - N );
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

void ntfs::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "ntfslabel --force " + partition .get_path(), output, error, false ) )
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

bool ntfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "ntfslabel --force " + partition .get_path() + " \"" + partition .get_label() + "\"", operationdetail ) ;
}

void ntfs::read_uuid( Partition & partition )
{
}

bool ntfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition .uuid == UUID_RANDOM_NTFS_HALF )
		return ! execute_command( "ntfslabel --new-half-serial " + partition .get_path(), operationdetail ) ;
	else
		return ! execute_command( "ntfslabel --new-serial " + partition .get_path(), operationdetail ) ;

	return true ;
}

bool ntfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkntfs -Q -v -L \"" + new_partition.get_label() +
				  "\" " + new_partition.get_path(),
				  operationdetail,
				  false,
				  true );
}

bool ntfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool return_value = false ;
	Glib::ustring str_temp = "ntfsresize -P --force --force " + partition_new .get_path() ;
	
	if ( ! fill_partition )
	{
		str_temp += " -s " ;
		str_temp += Utils::num_to_str( Utils::round( Utils::sector_to_unit(
				partition_new .get_sector_length(), partition_new .sector_size, UNIT_BYTE ) ) -1 ) ;
	}
	
	//simulation..
	operationdetail .add_child( OperationDetail( _("run simulation") ) ) ;

	if ( ! execute_command( str_temp + " --no-action", operationdetail .get_last_child() ) )
	{
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;

		//real resize
		operationdetail .add_child( OperationDetail( _("real resize") ) ) ;

		if ( ! execute_command( str_temp, operationdetail .get_last_child() ) )
		{
			operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
			return_value = true ;
		}
		else
		{
			operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
		}
	}
	else
	{
		operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
	}
	
	return return_value ;
}

bool ntfs::copy( const Glib::ustring & src_part_path,
		 const Glib::ustring & dest_part_path, 
		 OperationDetail & operationdetail )
{
	return ! execute_command( "ntfsclone -f --overwrite " + dest_part_path + " " + src_part_path,
				  operationdetail,
				  false,
				  true );
}

bool ntfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "ntfsresize -P -i -f -v " + partition .get_path(), operationdetail ) ; 
}

} //GParted


