/* Copyright (C) 2004 Bart
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
 
 
#include "../include/FileSystem.h"

#include <cerrno>

namespace GParted
{
	
FileSystem::FileSystem()
{
}

const Glib::ustring FileSystem::get_custom_text( CUSTOM_TEXT ttype, int index )
{
	return get_generic_text( ttype, index ) ;
}

const Glib::ustring FileSystem::get_generic_text( CUSTOM_TEXT ttype, int index )
{
	return "" ;
}

int FileSystem::execute_command( const Glib::ustring & command, OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( command, STATUS_NONE, FONT_BOLD_ITALIC ) ) ;

	int exit_status = Utils::execute_command( "nice -n 19 " + command, output, error ) ;

	if ( ! output .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( output, STATUS_NONE, FONT_ITALIC ) ) ;
	
	if ( ! error .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( error, STATUS_NONE, FONT_ITALIC ) ) ;

	return exit_status ;
}

//Time command, add results to operation detail and by default set success or failure
int FileSystem::execute_command_timed( const Glib::ustring & command
                                     , OperationDetail & operationdetail
                                     , bool check_status )
{
	operationdetail .add_child( OperationDetail( command, STATUS_EXECUTE, FONT_BOLD_ITALIC ) ) ;

	int exit_status = Utils::execute_command( "nice -n 19 " + command, output, error ) ;
	if ( check_status )
	{
		if ( ! exit_status )
			operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		else
			operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
	}

	if ( ! output .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( output, STATUS_NONE, FONT_ITALIC ) ) ;

	if ( ! error .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( error, STATUS_NONE, FONT_ITALIC ) ) ;

	return exit_status ;
}

//Create uniquely named temporary directory and add results to operation detail
Glib::ustring FileSystem::mk_temp_dir( const Glib::ustring & infix, OperationDetail & operationdetail )
{
	// Construct template like "$TMPDIR/gparted-XXXXXX" or "$TMPDIR/gparted-INFIX-XXXXXX"
	Glib::ustring dir_template = Glib::get_tmp_dir() + "/gparted-" ;
	if ( ! infix .empty() )
		dir_template += infix + "-" ;
	dir_template += "XXXXXX" ;
	//Secure Programming for Linux and Unix HOWTO, Chapter 6. Avoid Buffer Overflow
	//  http://tldp.org/HOWTO/Secure-Programs-HOWTO/library-c.html
	char dir_buf[4096+1];
	sprintf( dir_buf, "%.*s", (int) sizeof(dir_buf)-1, dir_template .c_str() ) ;

	//Looks like "mkdir -v" command was run to the user
	operationdetail .add_child( OperationDetail(
			Glib::ustring( "mkdir -v " ) + dir_buf, STATUS_EXECUTE, FONT_BOLD_ITALIC ) ) ;
	const char * dir_name = mkdtemp( dir_buf ) ;
	if ( NULL == dir_name )
	{
		int e = errno ;
		operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				String::ucompose( "mkdtemp(%1): %2", dir_buf, Glib::strerror( e ) ), STATUS_NONE ) ) ;
		dir_name = "" ;
	}
	else
	{
		//Update command with actually created temporary directory
		operationdetail .get_last_child() .set_description(
				Glib::ustring( "mkdir -v " ) + dir_name, FONT_BOLD_ITALIC ) ;
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				/*TO TRANSLATORS: looks like   Created directory /tmp/gparted-CEzvSp */
				String::ucompose( _("Created directory %1"), dir_name ), STATUS_NONE ) ) ;
	}

	return Glib::ustring( dir_name ) ;
}

//Remove directory and add results to operation detail
void FileSystem::rm_temp_dir( const Glib::ustring dir_name, OperationDetail & operationdetail )
{
	//Looks like "rmdir -v" command was run to the user
	operationdetail .add_child( OperationDetail( Glib::ustring( "rmdir -v " ) + dir_name,
	                                             STATUS_EXECUTE, FONT_BOLD_ITALIC ) ) ;
	if ( rmdir( dir_name .c_str() ) )
	{
		//Don't mark operation as errored just because rmdir
		//  failed.  Set to Warning (N/A) instead.
		int e = errno ;
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;  //Stop timer
		operationdetail .get_last_child() .set_status( STATUS_N_A ) ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				String::ucompose( "rmdir(%1): ", dir_name ) + Glib::strerror( e ), STATUS_NONE ) ) ;
	}
	else
	{
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				/*TO TRANSLATORS: looks like   Removed directory /tmp/gparted-CEzvSp */
				String::ucompose( _("Removed directory %1"), dir_name ), STATUS_NONE ) ) ;
	}
}

} //GParted
