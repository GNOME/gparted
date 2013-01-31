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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
 
#include "../include/fat16.h"
#include "../include/fat32.h"

/*****
//For some reason unknown, this works without these include statements.
#include <stdlib.h>    // 'C' library for mkstemp()
#include <unistd.h>    // 'C' library for write(), close()
#include <stdio.h>     // 'C' library for remove()
*****/

namespace GParted
{

const Glib::ustring ( & fat32::Change_UUID_Warning ) [] = fat16::Change_UUID_Warning ;

const Glib::ustring fat32::get_custom_text( CUSTOM_TEXT ttype, int index )
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

FS fat32::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_FAT32 ;

	// hack to disable silly mtools warnings
	setenv( "MTOOLS_SKIP_CHECK", "1", 0 );

	//find out if we can create fat32 file systems
	if ( ! Glib::find_program_in_path( "mkdosfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "dosfsck" ) .empty() )
	{
		fs .check = GParted::FS::EXTERNAL ;
		fs .read = GParted::FS::EXTERNAL ;
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
#ifdef HAVE_LIBPARTED_3_0_0_PLUS
	fs .move = FS::GPARTED ;
#else
	fs .move = GParted::FS::LIBPARTED ;
#endif

	fs .copy = GParted::FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	fs .MIN = 33 * MEBIBYTE ; //Smaller file systems will cause windows scandisk to fail.
	
	return fs ;
}

void fat32::set_used_sectors( Partition & partition ) 
{
	//FIXME: i've encoutered a readonly fat32 file system.. this won't work with the -a ... best check also without the -a
	exit_status = Utils::execute_command( "dosfsck -n -v " + partition .get_path(), output, error, true ) ;
	if ( exit_status == 0 || exit_status == 1 || exit_status == 256 )
	{
		//total file system size in logical sectors
		index = output .rfind( "\n", output .find( "sectors total" ) ) +1 ;
		if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "%Ld", &T ) != 1 )
			T = -1 ;

		//bytes per logical sector
		index = output .rfind( "\n", output .find( "bytes per logical sector" ) ) +1 ;
		if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "%Ld", &S ) != 1 )
			S = -1 ;

		if ( T > -1 && S > -1 )
			T = Utils::round( T * ( S / double(partition .sector_size) ) ) ;

		//free clusters
		index = output .find( ",", output .find( partition .get_path() ) + partition .get_path() .length() ) +1 ;
		if ( index < output .length() && sscanf( output .substr( index ) .c_str(), "%Ld/%Ld", &S, &N ) == 2 ) 
			N -= S ;
		else
			N = -1 ;

		//bytes per cluster
		index = output .rfind( "\n", output .find( "bytes per cluster" ) ) +1 ;
		if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "%Ld", &S ) != 1 )
			S = -1 ;
	
		if ( N > -1 && S > -1 )
			N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;

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

void fat32::read_label( Partition & partition )
{
	Glib::ustring cmd = "mlabel -s :: -i " + partition.get_path();

	if ( ! Utils::execute_command( cmd, output, error, true ) )
	{
		partition .set_label( Utils::trim( Utils::regexp_label( output, "Volume label is ([^(]*)" ) ) ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool fat32::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "" ;
	if ( partition .get_label() .empty() )
		cmd = "mlabel -c :: -i " + partition.get_path();
	else
		cmd = "mlabel ::\"" + partition.get_label() + "\" -i " + partition.get_path();

	operationdetail .add_child( OperationDetail( cmd, STATUS_NONE, FONT_BOLD_ITALIC ) ) ;

	int exit_status = Utils::execute_command( cmd, output, error ) ;

	if ( ! output .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( output, STATUS_NONE, FONT_ITALIC ) ) ;

	if ( ! error .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( error, STATUS_NONE, FONT_ITALIC ) ) ;

	return ( exit_status == 0 );
}

void fat32::read_uuid( Partition & partition )
{
	Glib::ustring cmd = "mdir -f :: -i " + partition.get_path();

	if ( ! Utils::execute_command( cmd, output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "Volume Serial Number is[[:blank:]]([^[:space:]]+)" ) ;
		if ( partition .uuid == "0000-0000" )
			partition .uuid .clear() ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}


bool fat32::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "mlabel -s -n :: -i " + partition.get_path();

	operationdetail .add_child( OperationDetail( cmd, STATUS_NONE, FONT_BOLD_ITALIC ) ) ;

	int exit_status = Utils::execute_command( cmd, output, error ) ;

	if ( ! output .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( output, STATUS_NONE, FONT_ITALIC ) ) ;

	if ( ! error .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( error, STATUS_NONE, FONT_ITALIC ) ) ;

	return ( exit_status == 0 );
}

bool fat32::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkdosfs -F32 -v -I -n \"" + new_partition.get_label() +
				  "\" " + new_partition.get_path(), operationdetail,
				  false, true );
}

bool fat32::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( "dosfsck -a -w -v " + partition.get_path(), operationdetail,
				       false, true );

	return ( exit_status == 0 || exit_status == 1 || exit_status == 256 ) ;
}

} //GParted
