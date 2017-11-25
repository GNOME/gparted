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

#include <cerrno>
#include <iostream>
#include <gtkmm/main.h>
#include <signal.h>
#include <fcntl.h>
#include <sigc++/slot.h>

namespace GParted
{

FileSystem::FileSystem()
{
}

const Glib::ustring FileSystem::get_custom_text( CUSTOM_TEXT ttype, int index ) const
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
	exit_status = Utils::decode_wait_status( status );
	running = false;
	if (pipecount == 0) // pipes finished first
		Gtk::Main::quit();
	Glib::spawn_close_pid( pid );
}

//Callback passing the latest partial output from the external command
//  to operation detail for updating in the UI
static void update_command_output( OperationDetail *operationdetail, Glib::ustring *str )
{
	operationdetail->set_description( *str, FONT_ITALIC );
}

static void cancel_command( bool force, Glib::Pid pid, bool cancel_safe )
{
	if( force || cancel_safe )
		kill( -pid, SIGINT );
}

static void setup_child()
{
	setpgrp();
}

int FileSystem::execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
                                 ExecFlags flags )
{
	StreamSlot empty_stream_slot;
	TimedSlot empty_timed_slot;
	return execute_command_internal( command, operationdetail, flags, empty_stream_slot, empty_timed_slot );
}

int FileSystem::execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
                                 ExecFlags flags,
                                 StreamSlot stream_progress_slot )
{
	TimedSlot empty_timed_slot;
	return execute_command_internal( command, operationdetail, flags, stream_progress_slot, empty_timed_slot );
}

int FileSystem::execute_command( const Glib::ustring & command, OperationDetail & operationdetail,
                                 ExecFlags flags,
                                 TimedSlot timed_progress_slot )
{
	StreamSlot empty_stream_slot;
	return execute_command_internal( command, operationdetail, flags, empty_stream_slot, timed_progress_slot );
}

int FileSystem::execute_command_internal( const Glib::ustring & command, OperationDetail & operationdetail,
                                          ExecFlags flags,
                                          StreamSlot stream_progress_slot,
                                          TimedSlot timed_progress_slot )
{
	operationdetail.add_child( OperationDetail( command, STATUS_EXECUTE, FONT_BOLD_ITALIC ) );
	OperationDetail & cmd_operationdetail = operationdetail.get_last_child();
	Glib::Pid pid;
	// set up pipes for capture
	int out, err;
	// spawn external process
	running = true;
	try {
		Glib::spawn_async_with_pipes(
			std::string( "." ),
			Glib::shell_parse_argv( command ),
			Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH,
			sigc::ptr_fun(setup_child),
			&pid,
			0,
			&out,
			&err );
	} catch (Glib::SpawnError &e) {
		std::cerr << e.what() << std::endl;
		cmd_operationdetail.add_child( OperationDetail( e.what(), STATUS_ERROR, FONT_ITALIC ) );
		return Utils::get_failure_status( e );
	}
	fcntl( out, F_SETFL, O_NONBLOCK );
	fcntl( err, F_SETFL, O_NONBLOCK );
	Glib::signal_child_watch().connect( sigc::mem_fun( *this, &FileSystem::store_exit_status ), pid );
	pipecount = 2;
	PipeCapture outputcapture( out, output );
	PipeCapture errorcapture( err, error );
	outputcapture.signal_eof.connect( sigc::mem_fun( *this, &FileSystem::execute_command_eof ) );
	errorcapture.signal_eof.connect( sigc::mem_fun( *this, &FileSystem::execute_command_eof ) );
	cmd_operationdetail.add_child( OperationDetail( output, STATUS_NONE, FONT_ITALIC ) );
	cmd_operationdetail.add_child( OperationDetail( error, STATUS_NONE, FONT_ITALIC ) );
	std::vector<OperationDetail*> &children = cmd_operationdetail.get_childs();
	outputcapture.signal_update.connect( sigc::bind( sigc::ptr_fun( update_command_output ),
	                                                 children[children.size() - 2],
	                                                 &output ) );
	errorcapture.signal_update.connect( sigc::bind( sigc::ptr_fun( update_command_output ),
	                                                children[children.size() - 1],
	                                                &error ) );
	sigc::connection timed_conn;
	if ( flags & EXEC_PROGRESS_STDOUT && ! stream_progress_slot.empty() )
		// Call progress tracking callback when stdout updates
		outputcapture.signal_update.connect( sigc::bind( stream_progress_slot, &cmd_operationdetail ) );
	else if ( flags & EXEC_PROGRESS_STDERR && ! stream_progress_slot.empty() )
		// Call progress tracking callback when stderr updates
		errorcapture.signal_update.connect( sigc::bind( stream_progress_slot, &cmd_operationdetail ) );
	else if ( flags & EXEC_PROGRESS_TIMED && ! timed_progress_slot.empty() )
		// Call progress tracking callback every 500 ms
		timed_conn = Glib::signal_timeout().connect( sigc::bind( timed_progress_slot, &cmd_operationdetail ), 500 );
	outputcapture.connect_signal();
	errorcapture.connect_signal();

	cmd_operationdetail.signal_cancel.connect(
		sigc::bind(
			sigc::ptr_fun( cancel_command ),
			pid,
			flags & EXEC_CANCEL_SAFE ) );
	Gtk::Main::run();

	if ( flags & EXEC_CHECK_STATUS )
		cmd_operationdetail.set_success_and_capture_errors( exit_status == 0 );
	close( out );
	close( err );
	if ( timed_conn.connected() )
		timed_conn.disconnect();
	cmd_operationdetail.stop_progressbar();
	return exit_status;
}

void FileSystem::set_status( OperationDetail & operationdetail, bool success )
{
	operationdetail.get_last_child().set_success_and_capture_errors( success );
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
		operationdetail .get_last_child() .add_child( OperationDetail(
				String::ucompose( "mkdtemp(%1): %2", dir_buf, Glib::strerror( e ) ), STATUS_NONE ) ) ;
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
				String::ucompose( _("Created directory %1"), dir_name ), STATUS_NONE ) ) ;
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
		//Don't mark operation as errored just because rmdir
		//  failed.  Set to Warning (N/A) instead.
		int e = errno ;
		operationdetail .get_last_child() .add_child( OperationDetail(
				String::ucompose( "rmdir(%1): ", dir_name ) + Glib::strerror( e ), STATUS_NONE ) ) ;
		operationdetail.get_last_child().set_status( STATUS_WARNING );
	}
	else
	{
		operationdetail .get_last_child() .add_child( OperationDetail(
				/*TO TRANSLATORS: looks like   Removed directory /tmp/gparted-CEzvSp */
				String::ucompose( _("Removed directory %1"), dir_name ), STATUS_NONE ) ) ;
		operationdetail.get_last_child().set_success_and_capture_errors( true );
	}
}

} //GParted
