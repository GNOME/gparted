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
#include "../include/GParted_Core.h"

#include <cerrno>
#include <iostream>
#include <gtkmm/main.h>
#include <fcntl.h>

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
	/*TO TRANSLATORS: these labels will be used in the partition menu */
	static const Glib::ustring activate_text = _( "_Mount" ) ;
	static const Glib::ustring deactivate_text = _( "_Unmount" ) ;

	switch ( ttype ) {
		case CTEXT_ACTIVATE_FILESYSTEM :
			return index == 0 ? activate_text : "" ;
		case CTEXT_DEACTIVATE_FILESYSTEM :
			return index == 0 ? deactivate_text : "" ;
		default :
			return "" ;
	}
}

void FileSystem::store_exit_status( GPid pid, int status )
{
	exit_status = status;
	running = false;
	if (pipecount == 0) // pipes finished first
		Gtk::Main::quit();
	Glib::spawn_close_pid( pid );
}

static void relay_update( OperationDetail *operationdetail, Glib::ustring *str )
{
	operationdetail->set_description( *str, FONT_ITALIC );
}

int FileSystem::execute_command( const Glib::ustring & command, OperationDetail & operationdetail, bool checkstatus )
{
	operationdetail .add_child( OperationDetail( command, checkstatus ? STATUS_EXECUTE : STATUS_NONE, FONT_BOLD_ITALIC ) ) ;
	Glib::Pid pid;
	// set up pipes for capture
	int out, err;
	// spawn external process
	running = true;
	try {
		Glib::spawn_async_with_pipes(
			std::string(),
			Glib::shell_parse_argv( command ),
			Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH,
			sigc::slot< void >(),
			&pid,
			0,
			&out,
			&err );
	} catch (Glib::SpawnError &e) {
		std::cerr << e.what() << std::endl;
		operationdetail.get_last_child().add_child(
			OperationDetail( e.what(), STATUS_ERROR, FONT_ITALIC ) );
		return 1;
	}
	fcntl( out, F_SETFL, O_NONBLOCK );
	fcntl( err, F_SETFL, O_NONBLOCK );
	Glib::signal_child_watch().connect( sigc::mem_fun( *this, &FileSystem::store_exit_status ), pid );
	output.clear();
	error.clear();
	pipecount = 2;
	PipeCapture outputcapture( out, output );
	PipeCapture errorcapture( err, error );
	outputcapture.eof.connect( sigc::mem_fun( *this, &FileSystem::execute_command_eof ) );
	errorcapture.eof.connect( sigc::mem_fun( *this, &FileSystem::execute_command_eof ) );
	operationdetail.get_last_child().add_child(
		OperationDetail( output, STATUS_NONE, FONT_ITALIC ) );
	operationdetail.get_last_child().add_child(
		OperationDetail( error, STATUS_NONE, FONT_ITALIC ) );
	std::vector<OperationDetail> &children = operationdetail.get_last_child().get_childs();
	outputcapture.update.connect( sigc::bind( sigc::ptr_fun( relay_update ),
						  &(children[children.size() - 2]),
						  &output ) );
	errorcapture.update.connect( sigc::bind( sigc::ptr_fun( relay_update ),
						 &(children[children.size() - 1]),
						 &error ) );
	outputcapture.connect_signal( out );
	errorcapture.connect_signal( err );

	Gtk::Main::run();

	if (checkstatus) {
		if ( !exit_status )
			operationdetail.get_last_child().set_status( STATUS_SUCCES );
		else
			operationdetail.get_last_child().set_status( STATUS_ERROR );
	}
	close( out );
	close( err );
	return exit_status;
}

void FileSystem::execute_command_eof()
{
	if (--pipecount)
		return; // wait for second pipe to eof
	if ( !running ) // already got exit status
		Gtk::Main::quit();
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
