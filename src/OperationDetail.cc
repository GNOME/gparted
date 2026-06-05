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
#include <ctime>
#include <fcntl.h>
#include <glibmm/exception.h>
#include <glibmm/main.h>
#include <glibmm/markup.h>
#include <glibmm/shell.h>
#include <glibmm/spawn.h>
#include <glibmm/stringutils.h>
#include <glibmm/ustring.h>
#include <gtkmm/main.h>
#include <iostream>
#include <memory>
#include <sigc++/bind.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <signal.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <vector>


namespace GParted
{


namespace  // unnamed
{


// The single progress bar for the current operation
static ProgressBar single_progressbar;


// Single set of coordination data between execute_command_internal() and helpers
static struct CommandStatus
{
	bool          running     = false;
	int           pipecount   = 0;
	Glib::ustring output;
	Glib::ustring error;
	int           exit_status = 0;
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


OperationDetail::OperationDetail(const Glib::ustring& description, OperationDetailStatus status, Font font)
{
	set_description( description, font );
	set_status( status );
}


OperationDetail::~OperationDetail()
{
	// Disconnect parent's connection calling back to this->cancel() because this is
	// being destructed.
	m_connection_cancel.disconnect();
	m_sub_details.clear();
}


void OperationDetail::set_description(const Glib::ustring& description, Font font)
{
	try
	{
		switch (font)
		{
			case FONT_NORMAL:
				m_description = Glib::Markup::escape_text(description);
				break;
			case FONT_BOLD:
				m_description = "<b>" + Glib::Markup::escape_text(description) + "</b>";
				break;
			case FONT_ITALIC:
				m_description = "<i>" + Glib::Markup::escape_text(description) + "</i>";
				break;
			case FONT_BOLD_ITALIC:
				m_description = "<b><i>" + Glib::Markup::escape_text(description) + "</i></b>";
				break;
			case FONT_MONOSPACE:
				m_description = "<tt>" + Glib::Markup::escape_text(description) + "</tt>";
				break;
		}
	}
	catch (Glib::Exception& e)
	{
		m_description = e.what();
	}

	on_update(*this);
}


const Glib::ustring& OperationDetail::get_description() const
{
	return m_description;
}


void OperationDetail::set_status( OperationDetailStatus status ) 
{	
	if (m_status != STATUS_ERROR)
	{
		switch ( status )
		{
			case STATUS_EXECUTE:
				m_time_elapsed = -1;
				m_time_start = std::time(nullptr);
				break ;
			case STATUS_ERROR:
			case STATUS_WARNING:
			case STATUS_SUCCESS:
				if(m_time_start != -1)
					m_time_elapsed = std::time(nullptr) - m_time_start;
				break ;

			default:
				break ;
		}

		m_status = status;
		on_update( *this ) ;
	}
}


void OperationDetail::set_success_and_capture_errors( bool success )
{
	set_status( success ? STATUS_SUCCESS : STATUS_ERROR );
	signal_capture_errors.emit( *this, success );

	// No more operation detail children should be added to this parent after any
	// errors have been captured so those errors remain its last children.
	m_no_more_children = true;
}


OperationDetailStatus OperationDetail::get_status() const
{
	return m_status;
}


void OperationDetail::set_treepath( const Glib::ustring & treepath ) 
{
	m_treepath = treepath;
}


const Glib::ustring& OperationDetail::get_treepath() const
{
	return m_treepath;
}


Glib::ustring OperationDetail::get_elapsed_time() const 
{
	if (m_time_elapsed >= 0)
		return Utils::format_time(m_time_elapsed);
	
	return "" ;
}


void OperationDetail::add_child( const OperationDetail & operationdetail )
{
	// This function deals with 3 OperationDetail objects: *this (the parent), *child
	// (newly created) and operationdetail (passed argument).  Shout by using "this->"
	// when dealing with the parent.

	if (this->m_no_more_children)
		std::cerr << "GParted Bug: Adding child operation detail when m_no_more_children is true" << std::endl;
		// But carry on regardless.

	// Create new default constructed OperationDetail object and attach as child of
	// this object with unique_ptr.  Implicit move of unique_ptr by push_back().
	this->m_sub_details.push_back(std::make_unique<OperationDetail>());

	// Populate all necessary members of the new child OperationDetail object.
	OperationDetail* child = this->m_sub_details.back().get();
	child->signal_update.connect(this->signal_update);
	child->signal_capture_errors.connect(this->signal_capture_errors);
	if (this->m_cancelflag)
		child->cancel(this->m_cancelflag == 2);
	child->m_description       = operationdetail.m_description;
	child->m_status            = operationdetail.m_status;
	child->m_treepath          = this->m_treepath + ":" + Utils::num_to_str(this->m_sub_details.size() - 1);
	child->m_time_start        = operationdetail.m_time_start;
	child->m_time_elapsed      = operationdetail.m_time_elapsed;
	child->m_no_more_children  = operationdetail.m_no_more_children;
	child->m_connection_cancel = this->signal_cancel.connect(
	                                sigc::mem_fun(child, &OperationDetail::cancel));

	on_update(*child);
}
// What each OperationDetail callback signal is for and how it works
//
// Signal:  signal_update
//     Used so that changes made to each OperationDetail object in the tree hierarchy are
//     updated in the Applying pending operations view of those operations in real time.
//
//     Callback:          Dialog_Progress::on_signal_update()
//     Connection style:  direct
//         All OperationDetail objects have the same callback.  Each individual object
//         object directly calls back to Dialog_Progress::on_signal_update() when it is
//         updated.
//
// Signal:  signal_capture_errors
//     Used so that any libparted errors which occur, can be added as children of the
//     current OperationDetail object representing that libparted action which failed.
//
//     Callback:          GParted_Core::capture_libparted_errors()
//     Connection style:  direct
//         All OperationDetail objects have the same callback.  Each individual object
//         directly calls back to GParted_Core::capture_libparted_errors() when
//         set_success_and_capture_errors() is called.
//
// Signal:  signal_cancel
//     Used to cancel the currently being applied operation.
//
//     Member variable:  m_cancelflag
//     Values:
//         0  Not cancelled
//         1  Cancel safe operations only
//         2  Force cancel - Cancel all operations, including cancel unsafe ones
//
//     Callback:          OperationDetail::cancel()
//     Connection style:  tree wide depth first traversal
//         All parent OperationDetail objects have a callback registered to each of their
//         children.  When called, each object:
//         1.  Sets its own m_cancelflag according to whether the cancel is being forced
//             or not.
//         2.  Emits the callback for each of its own children, resulting in a tree wide
//             depth first traversal.
//
//     Callback:          cancel_command()
//     Connection style:  direct
//         Only for the OperationDetail object which is executing the external command; it
//         has this second callback registered.  When emitted, via the above tree wide
//         mechanism, cancel_command() is called which sends a signal to kill the command.
//
//     Callback:          CopyBlocks::set_cancel()
//     Connection style:  direct
//         Only for the OperationDetail object which is performing an internal block copy;
//         it has this second callback registered.  When emitted, via the above tree wide
//         mechanism, CopyBlocks::set_cancel() marks the block copy to be cancelled.


OperationDetailVector& OperationDetail::get_children()
{
	return m_sub_details;
}


const OperationDetailVector& OperationDetail::get_children() const
{
	return m_sub_details;
}


OperationDetail & OperationDetail::get_last_child()
{
	//little bit of (healthy?) paranoia
	if (m_sub_details.size() == 0)
		add_child( OperationDetail( "---", STATUS_ERROR ) ) ;

	return *m_sub_details[m_sub_details.size() - 1];
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

void OperationDetail::on_update( const OperationDetail & operationdetail ) 
{
	if (! m_treepath.empty())
		signal_update .emit( operationdetail ) ;
}


void OperationDetail::cancel( bool force )
{
	if ( force )
		m_cancelflag = 2;
	else
		m_cancelflag = 1;
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
	cmd_operationdetail.add_child(OperationDetail(cmd_status.output, STATUS_NONE, FONT_MONOSPACE));
	cmd_operationdetail.add_child(OperationDetail(cmd_status.error, STATUS_NONE, FONT_MONOSPACE));
	OperationDetailVector& children = cmd_operationdetail.get_children();
	outputcapture.signal_update.connect(sigc::bind(sigc::ptr_fun(update_command_output),
	                                               children[children.size() - 2].get(),
	                                               &cmd_status.output));
	errorcapture.signal_update.connect(sigc::bind(sigc::ptr_fun(update_command_output),
	                                              children[children.size() - 1].get(),
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

	sigc::connection connection_command_cancel = cmd_operationdetail.signal_cancel.connect(
				sigc::bind(sigc::ptr_fun(cancel_command),
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
	connection_command_cancel.disconnect();
	if (timed_conn.connected())
		timed_conn.disconnect();
	cmd_operationdetail.stop_progressbar();
	return cmd_status.exit_status;
}


}  // namespace GParted
