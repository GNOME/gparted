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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "FileSystem.h"
#include "GParted_Core.h"
#include "OperationDetail.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <glibmm/miscutils.h>
#include <glibmm/stringutils.h>
#include <glibmm/ustring.h>


namespace GParted
{

FileSystem::FileSystem()
{
}

const Glib::ustring & FileSystem::get_custom_text( CUSTOM_TEXT ttype, int index ) const
{
	return get_generic_text( ttype, index ) ;
}

const Glib::ustring & FileSystem::get_generic_text( CUSTOM_TEXT ttype, int index )
{
	/*TO TRANSLATORS: these labels will be used in the partition menu */
	static const Glib::ustring activate_text = _( "_Mount" ) ;
	static const Glib::ustring deactivate_text = _( "_Unmount" ) ;
	static const Glib::ustring empty_text;

	switch ( ttype ) {
		case CTEXT_ACTIVATE_FILESYSTEM :
			return index == 0 ? activate_text : empty_text;
		case CTEXT_DEACTIVATE_FILESYSTEM :
			return index == 0 ? deactivate_text : empty_text;
		default :
			return empty_text;
	}
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
	if (nullptr == dir_name)
	{
		int e = errno ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				Glib::ustring::compose( "mkdtemp(%1): %2", dir_buf, Glib::strerror( e ) ), STATUS_NONE ) ) ;
		operationdetail.get_last_child().set_success_and_capture_errors( false );
		dir_name = "" ;
	}
	else
	{
		//Update command with actually created temporary directory
		operationdetail .get_last_child() .set_description(
				Glib::ustring( "mkdir -v " ) + dir_name, FONT_BOLD_ITALIC ) ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				/*TO TRANSLATORS: looks like   Created directory /tmp/gparted-CEzvSp */
				Glib::ustring::compose( _("Created directory %1"), dir_name ), STATUS_NONE ) ) ;
		operationdetail.get_last_child().set_success_and_capture_errors( true );
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
		// Don't mark operation as errored just because rmdir failed.  Set to
		// Warning instead.
		int e = errno ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				Glib::ustring::compose( "rmdir(%1): ", dir_name ) + Glib::strerror( e ), STATUS_NONE ) ) ;
		operationdetail.get_last_child().set_status( STATUS_WARNING );
	}
	else
	{
		operationdetail .get_last_child() .add_child( OperationDetail(
				/*TO TRANSLATORS: looks like   Removed directory /tmp/gparted-CEzvSp */
				Glib::ustring::compose( _("Removed directory %1"), dir_name ), STATUS_NONE ) ) ;
		operationdetail.get_last_child().set_success_and_capture_errors( true );
	}
}

} //GParted
