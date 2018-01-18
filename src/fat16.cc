/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Curtis Gedak
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

#include "fat16.h"
#include "FileSystem.h"
#include "Partition.h"

/*****
//For some reason unknown, this works without these include statements.
#include <stdlib.h>    // 'C' library for mkstemp()
#include <unistd.h>    // 'C' library for write(), close()
#include <stdio.h>     // 'C' library for remove()
*****/

namespace GParted
{

const Glib::ustring fat16::Change_UUID_Warning [] =
	{ _( "Changing the UUID might invalidate the Windows Product Activation (WPA) key"
	   )
	, _( "On FAT and NTFS file systems, the"
	     " Volume Serial Number is used as the UUID."
	     " Changing the Volume Serial Number on the Windows system"
	     " partition, normally C:, might invalidate the WPA key."
	     " An invalid WPA key will prevent login until you reactivate Windows."
	   )
	, _( "Changing the UUID on external storage media and non-system partitions"
	     " is usually safe, but guarantees cannot be given."
	   )
	,    ""
	} ;

const Glib::ustring fat16::get_custom_text( CUSTOM_TEXT ttype, int index ) const
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

FS fat16::get_filesystem_support()
{
	FS fs( specific_type );

	// hack to disable silly mtools warnings
	setenv( "MTOOLS_SKIP_CHECK", "1", 0 );

	fs .busy = FS::GPARTED ;

	//find out if we can create fat file systems
	if ( ! Glib::find_program_in_path( "mkfs.fat" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
		create_cmd = "mkfs.fat" ;
	}
	else if ( ! Glib::find_program_in_path( "mkdosfs" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
		create_cmd = "mkdosfs" ;
	}

	if ( ! Glib::find_program_in_path( "fsck.fat" ) .empty() )
	{
		fs .check = GParted::FS::EXTERNAL ;
		fs .read = GParted::FS::EXTERNAL ;
		check_cmd = "fsck.fat" ;
	}
	else if ( ! Glib::find_program_in_path( "dosfsck" ) .empty() )
	{
		fs .check = GParted::FS::EXTERNAL ;
		fs .read = GParted::FS::EXTERNAL ;
		check_cmd = "dosfsck" ;
	}

	if ( ! Glib::find_program_in_path( "mdir" ) .empty() )
		fs .read_uuid = FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "mlabel" ) .empty() ) {
		fs .read_label = FS::EXTERNAL ;
		fs .write_label = FS::EXTERNAL ;
		fs .write_uuid = FS::EXTERNAL ;
	}

#ifdef HAVE_LIBPARTED_FS_RESIZE
	//resizing of start and endpoint are provided by libparted
	fs .grow = GParted::FS::LIBPARTED ;
	fs .shrink = GParted::FS::LIBPARTED ;
#endif
	fs .move = FS::GPARTED ;
	fs .copy = GParted::FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	if ( fs .filesystem == FS_FAT16 )
	{
		fs_limits.min_size = 16 * MEBIBYTE;
		fs_limits.max_size = (4096 - 1) * MEBIBYTE;  // Maximum seems to be just less than 4096 MiB.
	}
	else //FS_FAT32
	{
		fs_limits.min_size = 33 * MEBIBYTE;  // Smaller file systems will cause Windows' scandisk to fail.

		// Maximum FAT32 volume size with 512 byte sectors is 2 TiB.
		// *   Wikipedia: File Allocation Table / FAT32
		//     https://en.wikipedia.org/wiki/File_Allocation_Table#FAT32
		fs_limits.max_size = 2 * TEBIBYTE;
	}

	return fs ;
}

void fat16::set_used_sectors( Partition & partition ) 
{
	exit_status = Utils::execute_command( check_cmd + " -n -v " + Glib::shell_quote( partition.get_path() ),
	                                      output, error, true );
	if ( exit_status == 0 || exit_status == 1 )
	{
		//total file system size in logical sectors
		Glib::ustring::size_type index = output.rfind( "\n", output.find( "sectors total" ) ) + 1;
		if ( index >= output.length() || sscanf( output.substr( index ).c_str(), "%lld", &T ) != 1 )
			T = -1 ;

		//bytes per logical sector
		index = output .rfind( "\n", output .find( "bytes per logical sector" ) ) +1 ;
		if ( index >= output.length() || sscanf( output.substr( index ).c_str(), "%lld", &S ) != 1 )
			S = -1 ;

		if ( T > -1 && S > -1 )
			T = Utils::round( T * ( S / double(partition .sector_size) ) ) ;

		//free clusters
		index = output .find( ",", output .find( partition .get_path() ) + partition .get_path() .length() ) +1 ;
		if ( index < output.length() && sscanf( output.substr( index ).c_str(), "%lld/%lld", &S, &N ) == 2 )
			N -= S ;
		else
			N = -1 ;

		//bytes per cluster
		index = output .rfind( "\n", output .find( "bytes per cluster" ) ) +1 ;
		if ( index >= output.length() || sscanf( output.substr( index ).c_str(), "%lld", &S ) != 1 )
			S = -1 ;
	
		if ( N > -1 && S > -1 )
		{
			N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;
			partition.fs_block_size = S;
		}

		if ( T > -1 && N > -1 )
			partition .set_sector_usage( T, N ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

void fat16::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "mlabel -s :: -i " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                           )
	{
		partition.set_filesystem_label( Utils::trim( Utils::regexp_label( output, "Volume label is ([^(]*)" ) ) );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool fat16::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "" ;
	if ( partition.get_filesystem_label().empty() )
		cmd = "mlabel -c :: -i " + Glib::shell_quote( partition.get_path() );
	else
		cmd = "mlabel ::" + Glib::shell_quote( sanitize_label( partition.get_filesystem_label() ) ) +
		      " -i " + Glib::shell_quote( partition.get_path() );

	return ! execute_command( cmd, operationdetail, EXEC_CHECK_STATUS );
}

void fat16::read_uuid( Partition & partition )
{
	Glib::ustring cmd = "mdir -f :: -i " + Glib::shell_quote( partition.get_path() );

	if ( ! Utils::execute_command( cmd, output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "Volume Serial Number is[[:blank:]]([^[:space:]]+)" ) ;
		if ( partition .uuid == "0000-0000" )
			partition .uuid .clear() ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool fat16::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "mlabel -s -n :: -i " + Glib::shell_quote( partition.get_path() );

	return ! execute_command( cmd, operationdetail, EXEC_CHECK_STATUS );
}

bool fat16::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring fat_size = specific_type == FS_FAT16 ? "16" : "32" ;
	Glib::ustring label_args = new_partition.get_filesystem_label().empty() ? "" :
	                           "-n " + Glib::shell_quote( sanitize_label( new_partition.get_filesystem_label() ) ) + " ";
	return ! execute_command( create_cmd + " -F" + fat_size + " -v -I " + label_args +
	                          Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail,
	                          EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

bool fat16::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( check_cmd + " -a -w -v " + Glib::shell_quote( partition .get_path() ),
	                               operationdetail,
	                               EXEC_CANCEL_SAFE );
	bool success = ( exit_status == 0 || exit_status == 1 );
	set_status( operationdetail, success );
	return success;
}

//Private methods

const Glib::ustring fat16::sanitize_label( const Glib::ustring &label ) const
{
	Glib::ustring uppercase_label = label.uppercase();
	Glib::ustring new_label;

	// Copy uppercase label excluding prohibited characters.
	// [1] Microsoft TechNet: Label
	//     https://technet.microsoft.com/en-us/library/bb490925.aspx
	// [2] Replicated in Wikikedia: label (command)
	//     https://en.wikipedia.org/wiki/Label_%28command%29
	// Also exclude:
	// * Single quote (') because it is encoded by mlabel but not understood by
	//   Windows;
	// * ASCII control characters below SPACE because mlabel requests input in the
	//   same way it does for prohibited characters causing GParted to wait forever.
	Glib::ustring prohibited_chars( "*?.,;:/\\|+=<>[]\"'" );
	for ( unsigned int i = 0 ; i < uppercase_label.size() ; i ++ )
		if ( prohibited_chars.find( uppercase_label[i] ) == Glib::ustring::npos &&
		     uppercase_label[i] >= ' '                                             )
			new_label.push_back( uppercase_label[i] );

	// Pad with spaces to prevent mlabel writing corrupted labels.  See bug #700228.
	new_label .resize( Utils::get_filesystem_label_maxlength( specific_type ), ' ' ) ;

	return new_label ;
}

} //GParted
