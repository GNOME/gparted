/* Copyright (C) 2004 Bart 'plors' Hakvoort
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


#include "OperationDetail.h"
#include "PipeCapture.h"
#include "ProgressBar.h"
#include "Utils.h"

#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>
#include <glibmm/stringutils.h>
#include <glibmm/spawn.h>
#include <glibmm/shell.h>
#include <gtkmm/main.h>
#include <sigc++/bind.h>
#include <sigc++/signal.h>


namespace GParted
{


namespace  // unnamed
{


// The single progress bar for the current operation
static ProgressBar single_progressbar;


// Single set of coordination data between execute_command_internal() and helpers
static struct CommandStatus {
	bool          running;
	int           pipecount;
	Glib::ustring output;
	Glib::ustring error;
	int           exit_status;

	// Default constructor to initialise POD (Plain Old Data) members
	CommandStatus() : running(false), pipecount(0), exit_status(0)  {};
} cmd_status;


static void setup_child_process()
{
	setpgrp();
}


static void execute_command_eof()
{
	if (--cmd_status.pipecount)
		return;  // Wait for second pipe to encounter EOF.
	if (! cmd_status.running)  // Already got exit status.
		Gtk::Main::quit();
}


static void store_exit_status(GPid pid, int status)
{
	cmd_status.exit_status = Utils::decode_wait_status(status);
	cmd_status.running = false;
	if (cmd_status.pipecount == 0)  // Both pipes finished first.
		Gtk::Main::quit();
	Glib::spawn_close_pid(pid);
}


static void update_command_output(OperationDetail* operationdetail, Glib::ustring* str)
{
	operationdetail->set_description(*str, FONT_MONOSPACE);
}


static void cancel_command(bool force, Glib::Pid pid, bool cancel_safe)
{
	if (force || cancel_safe)
		kill(-pid, SIGINT);
}


}  // unnamed namespace


OperationDetail::OperationDetail() : cancelflag( 0 ), status( STATUS_NONE ), time_start( -1 ), time_elapsed( -1 ),
                                     no_more_children( false )
{
}

OperationDetail::OperationDetail( const Glib::ustring & description, OperationDetailStatus status, Font font ) :
	cancelflag( 0 ), status( STATUS_NONE ), time_start( -1 ), time_elapsed( -1 ), no_more_children( false )
{
	set_description( description, font );
	set_status( status );
}

OperationDetail::~OperationDetail()
{
	cancelconnection.disconnect();
	while (sub_details.size())
	{
		delete sub_details.back();
		sub_details.pop_back();
	}
}


void OperationDetail::set_description(const Glib::ustring& description, Font font)
{
	try
	{
		switch (font)
		{
			case FONT_NORMAL:
				this->description = Glib::Markup::escape_text(description);
				break;
			case FONT_BOLD:
				this->description = "<b>" + Glib::Markup::escape_text(description) + "</b>";
				break;
			case FONT_ITALIC:
				this->description = "<i>" + Glib::Markup::escape_text(description) + "</i>";
				break;
			case FONT_BOLD_ITALIC:
				this->description = "<b><i>" + Glib::Markup::escape_text(description) + "</i></b>";
				break;
			case FONT_MONOSPACE:
				this->description = "<tt>" + Glib::Markup::escape_text(description) + "</tt>";
				break;
		}
	}
	catch (Glib::Exception& e)
	{
		this->description = e.what();
	}

	on_update(*this);
}


const Glib::ustring& OperationDetail::get_description() const
{
	return description ;
}


void OperationDetail::set_status( OperationDetailStatus status ) 
{	
	if ( this ->status != STATUS_ERROR )
	{
		switch ( status )
		{
			case STATUS_EXECUTE:
				time_elapsed = -1 ;
				time_start = std::time(nullptr);
				break ;
			case STATUS_ERROR:
			case STATUS_WARNING:
			case STATUS_SUCCESS:
				if( time_start != -1 )
					time_elapsed = std::time(nullptr) - time_start;
				break ;

			default:
				break ;
		}

		this ->status = status ;
		on_update( *this ) ;
	}
}

void OperationDetail::set_success_and_capture_errors( bool success )
{
	set_status( success ? STATUS_SUCCESS : STATUS_ERROR );
	signal_capture_errors.emit( *this, success );
	no_more_children = true;
}

OperationDetailStatus OperationDetail::get_status() const
{
	return status ;
}
	
void OperationDetail::set_treepath( const Glib::ustring & treepath ) 
{
	this ->treepath = treepath ;
}


const Glib::ustring& OperationDetail::get_treepath() const
{
	return treepath ;
}
	
Glib::ustring OperationDetail::get_elapsed_time() const 
{
	if ( time_elapsed >= 0 ) 
		return Utils::format_time( time_elapsed ) ;
	
	return "" ;
}

void OperationDetail::add_child( const OperationDetail & operationdetail )
{
	if ( no_more_children )
		// Adding a child after this OperationDetail has been set to prevent it is
		// a programming bug.  However the best way to report it is by adding yet
		// another child containing the bug report, and allowing the child to be
		// added anyway.
		add_child_implement( OperationDetail( Glib::ustring( _("GParted Bug") ) + ": " +
		                                      /* TO TRANSLATORS:
		                                       * means that GParted has encountered a programming bug.  More
		                                       * information about a step is being added after the step was
		                                       * marked as complete.  This bug description as well as the
		                                       * information being added will be visible in the details of the
		                                       * applied operations.
		                                       */
		                                      _("Adding more information to the results of this step after it "
		                                        "has been marked as completed"),
		                                      STATUS_WARNING, FONT_ITALIC ) );
	add_child_implement( operationdetail );
}


std::vector<OperationDetail*>& OperationDetail::get_children()
{
	return sub_details;
}


const std::vector<OperationDetail*>& OperationDetail::get_children() const
{
	return sub_details;
}


OperationDetail & OperationDetail::get_last_child()
{
	//little bit of (healthy?) paranoia
	if ( sub_details .size() == 0 )
		add_child( OperationDetail( "---", STATUS_ERROR ) ) ;

	return *sub_details[sub_details.size() - 1];
}

void OperationDetail::run_progressbar( double progress, double target, ProgressBar_Text text_mode )
{
	if ( ! single_progressbar.running() )
		single_progressbar.start( target, text_mode );
	single_progressbar.update( progress );
	signal_update.emit( *this );
}

void OperationDetail::stop_progressbar()
{
	if ( single_progressbar.running() )
	{
		single_progressbar.stop();
		signal_update.emit( *this );
	}
}


// Execute command and capture stdout and stderr to operation details.
int OperationDetail::execute_command(const Glib::ustring& command, ExecFlags flags)
{
	StreamSlot empty_stream_slot;
	TimedSlot empty_timed_slot;
	return execute_command_internal(command, nullptr, flags, empty_stream_slot, empty_timed_slot);
}


// Execute command, pass string to stdin and capture stdout and stderr to operation
// details.
int OperationDetail::execute_command(const Glib::ustring& command, const char *input, ExecFlags flags)
{
	StreamSlot empty_stream_slot;
	TimedSlot empty_timed_slot;
	return execute_command_internal(command, input, flags, empty_stream_slot, empty_timed_slot);
}


// Execute command, capture stdout and stderr to operation details and run progress
// tracking callback when either stdout or stderr is updated (as requested by flag
// EXEC_PROGRESS_STDOUT or EXEC_PROGRESS_STDERR respectively).
int OperationDetail::execute_command(const Glib::ustring& command, ExecFlags flags, StreamSlot stream_progress_slot)
{
	TimedSlot empty_timed_slot;
	return execute_command_internal(command, nullptr, flags, stream_progress_slot, empty_timed_slot);
}


// Execute command, capture stdout and stderr to operation details and run progress
// tracking callback periodically (when requested by flag EXEC_PROGRESS_TIMED).
int OperationDetail::execute_command(const Glib::ustring& command, ExecFlags flags, TimedSlot timed_progress_slot)
{
	StreamSlot empty_stream_slot;
	return execute_command_internal(command, nullptr, flags, empty_stream_slot, timed_progress_slot);
}


const Glib::ustring& OperationDetail::get_command_output()
{
	return cmd_status.output;
}


const Glib::ustring& OperationDetail::get_command_error()
{
	return cmd_status.error;
}


// Private methods

void OperationDetail::add_child_implement( const OperationDetail & operationdetail )
{
	sub_details .push_back( new OperationDetail( operationdetail ) );

	sub_details.back()->set_treepath( treepath + ":" + Utils::num_to_str( sub_details .size() - 1 ) );
	sub_details.back()->signal_update.connect( sigc::mem_fun( this, &OperationDetail::on_update ) );
	sub_details.back()->cancelconnection = signal_cancel.connect(
				sigc::mem_fun( sub_details.back(), &OperationDetail::cancel ) );
	if ( cancelflag )
		sub_details.back()->cancel( cancelflag == 2 );
	sub_details.back()->signal_capture_errors.connect( this->signal_capture_errors );
	on_update( *sub_details.back() );
}

void OperationDetail::on_update( const OperationDetail & operationdetail ) 
{
	if ( ! treepath .empty() )
		signal_update .emit( operationdetail ) ;
}

void OperationDetail::cancel( bool force )
{
	if ( force )
		cancelflag = 2;
	else
		cancelflag = 1;
	signal_cancel.emit( force );
}

const ProgressBar& OperationDetail::get_progressbar() const
{
	return single_progressbar;
}


int OperationDetail::execute_command_internal(const Glib::ustring& command, const char *input, ExecFlags flags,
                                              StreamSlot stream_progress_slot,
                                              TimedSlot timed_progress_slot)
{
	add_child(OperationDetail(command, STATUS_EXECUTE, FONT_BOLD_ITALIC));
	OperationDetail& cmd_operationdetail = get_last_child();
	Glib::Pid pid;
	int in = -1;
	// set up pipes for capture
	int out;
	int err;
	// spawn external process
	cmd_status.running = true;
	cmd_status.pipecount = 2;
	cmd_status.exit_status = 255;  // Set to actual value by store_exit_status()
	try {
		Glib::spawn_async_with_pipes(std::string("."),
		                             Glib::shell_parse_argv(command),
		                             Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH,
		                             sigc::ptr_fun(setup_child_process),
		                             &pid,
		                             (input != nullptr) ? &in : 0,
		                             &out,
		                             &err);
	}
	catch (Glib::SpawnError &e)
	{
		std::cerr << e.what() << std::endl;
		cmd_operationdetail.add_child(OperationDetail( e.what(), STATUS_ERROR, FONT_ITALIC));
		return Utils::get_failure_status(e);
	}
	fcntl(out, F_SETFL, O_NONBLOCK);
	fcntl(err, F_SETFL, O_NONBLOCK);
	Glib::signal_child_watch().connect(sigc::ptr_fun(store_exit_status), pid);
	PipeCapture outputcapture(out, cmd_status.output);
	PipeCapture errorcapture(err, cmd_status.error);
	outputcapture.signal_eof.connect(sigc::ptr_fun(execute_command_eof));
	errorcapture.signal_eof.connect(sigc::ptr_fun(execute_command_eof));
	cmd_operationdetail.add_child(OperationDetail(cmd_status.output, STATUS_NONE, FONT_ITALIC));
	cmd_operationdetail.add_child(OperationDetail(cmd_status.error, STATUS_NONE, FONT_ITALIC));
	std::vector<OperationDetail*>& children = cmd_operationdetail.get_children();
	outputcapture.signal_update.connect(sigc::bind(sigc::ptr_fun(update_command_output),
	                                               children[children.size() - 2],
	                                               &cmd_status.output));
	errorcapture.signal_update.connect(sigc::bind(sigc::ptr_fun(update_command_output),
	                                              children[children.size() - 1],
	                                              &cmd_status.error));
	sigc::connection timed_conn;
	if (flags & EXEC_PROGRESS_STDOUT && ! stream_progress_slot.empty())
		// Register progress tracking callback called when stdout updates
		outputcapture.signal_update.connect(sigc::bind(stream_progress_slot, &cmd_operationdetail));
	else if (flags & EXEC_PROGRESS_STDERR && ! stream_progress_slot.empty())
		// Register progress tracking callback called when stderr updates
		errorcapture.signal_update.connect(sigc::bind(stream_progress_slot, &cmd_operationdetail));
	else if (flags & EXEC_PROGRESS_TIMED && ! timed_progress_slot.empty())
		// Register progress tracking callback called every 500 ms
		timed_conn = Glib::signal_timeout().connect(sigc::bind(timed_progress_slot, &cmd_operationdetail), 500);
	outputcapture.connect_signal();
	errorcapture.connect_signal();

	cmd_operationdetail.signal_cancel.connect(sigc::bind(sigc::ptr_fun(cancel_command),
	                                                     pid,
	                                                     flags & EXEC_CANCEL_SAFE));

	if (input != nullptr && in != -1)
	{
		// Write small amount of input to pipe to the child process.  Linux will
		// always accept up to 4096 bytes without blocking.  See pipe(7).
		size_t len = strlen(input);
		ssize_t written = write(in, input, len);
		if (written == -1 || (size_t)written < len)
		{
			int e = errno;
			std::cerr << "Write to child failed: " << Glib::strerror(e) << std::endl;
			cmd_operationdetail.add_child(OperationDetail("Write to child failed: " + Glib::strerror(e),
			                                              STATUS_NONE, FONT_ITALIC));
		}
		close(in);
	}

	Gtk::Main::run();

	if (flags & EXEC_CHECK_STATUS)
		cmd_operationdetail.set_success_and_capture_errors(cmd_status.exit_status == 0);
	close(out);
	close(err);
	if (timed_conn.connected())
		timed_conn.disconnect();
	cmd_operationdetail.stop_progressbar();
	return cmd_status.exit_status;
}


}  // namespace GParted
