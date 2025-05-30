/* Copyright (C) 2004 Bart 'plors' Hakvoort
 * Copyright (C) 2008 Curtis Gedak
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

#ifndef GPARTED_OPERATIONDETAIL_H
#define GPARTED_OPERATIONDETAIL_H


#include "ProgressBar.h"

#include <glibmm/ustring.h>
#include <glibmm/markup.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <sigc++/slot.h>
#include <vector>
#include <ctime>


namespace GParted
{

enum OperationDetailStatus {
	STATUS_NONE    = 0,
	STATUS_EXECUTE = 1,
	STATUS_SUCCESS = 2,
	STATUS_ERROR   = 3,
	STATUS_INFO    = 4,
	STATUS_WARNING = 5
};

enum Font {
	FONT_NORMAL	 = 0,
	FONT_BOLD	 = 1,
	FONT_ITALIC	 = 2,
	FONT_BOLD_ITALIC = 3
} ;


enum ExecFlags
{
	EXEC_NONE            = 1 << 0,
	EXEC_CHECK_STATUS    = 1 << 1,  // Set the status of the command in the operation
	                                // details based on the exit status being zero or
	                                // non-zero.  Must either use this flag when calling
	                                // ::execute_command() or manually call
					// ::set_success_and_capture_errors() afterwards.
	EXEC_CANCEL_SAFE     = 1 << 2,
	EXEC_PROGRESS_STDOUT = 1 << 3,  // Run progress tracking callback after reading new
	                                // data on stdout from command.
	EXEC_PROGRESS_STDERR = 1 << 4,  // Same but for stderr.
	EXEC_PROGRESS_TIMED  = 1 << 5   // Run progress tracking callback periodically.
};

inline ExecFlags operator|(ExecFlags lhs, ExecFlags rhs)
	{ return static_cast<ExecFlags>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)); }

inline ExecFlags operator&(ExecFlags lhs, ExecFlags rhs)
	{ return static_cast<ExecFlags>(static_cast<unsigned>(lhs) & static_cast<unsigned>(rhs)); }


class OperationDetail;
typedef sigc::slot<void, OperationDetail*> StreamSlot;
typedef sigc::slot<bool, OperationDetail*> TimedSlot;


class OperationDetail
{

friend class Dialog_Progress;  // To allow Dialog_Progress::on_signal_update() to call
                               // get_progressbar() and get direct access to the progress bar.

public:	
	OperationDetail() ;
	~OperationDetail();
	OperationDetail( const Glib::ustring & description,
			 OperationDetailStatus status = STATUS_EXECUTE,
			 Font font = FONT_NORMAL ) ;
	void set_description( const Glib::ustring & description, Font font = FONT_NORMAL ) ;
	const Glib::ustring& get_description() const;
	void set_status( OperationDetailStatus status ) ;
	void set_success_and_capture_errors( bool success );
	OperationDetailStatus get_status() const ;
	void set_treepath( const Glib::ustring & treepath ) ;
	const Glib::ustring& get_treepath() const;
	Glib::ustring get_elapsed_time() const ;
	
	void add_child( const OperationDetail & operationdetail ) ;
	std::vector<OperationDetail*>& get_children();
	const std::vector<OperationDetail*>& get_children() const;
	OperationDetail & get_last_child() ;
	void run_progressbar(double progress, double target, ProgressBar_Text text_mode = PROGRESSBAR_TEXT_TIME_REMAINING);
	void stop_progressbar();

	int execute_command(const Glib::ustring& command, ExecFlags flags = EXEC_NONE);
	int execute_command(const Glib::ustring& command, const char* input, ExecFlags flags = EXEC_NONE);
	int execute_command(const Glib::ustring& command, ExecFlags flags, StreamSlot stream_progress_slot);
	int execute_command(const Glib::ustring& command, ExecFlags flags, TimedSlot timed_progress_slot);
	static const Glib::ustring& get_command_output();
	static const Glib::ustring& get_command_error();

	sigc::signal< void, const OperationDetail & > signal_update ;
	sigc::signal< void, bool > signal_cancel;
	sigc::signal< void, OperationDetail &, bool > signal_capture_errors;
	char cancelflag;

private:
	void add_child_implement( const OperationDetail & operationdetail );
	void on_update( const OperationDetail & operationdetail ) ;
	void cancel( bool force );
	const ProgressBar& get_progressbar() const;

	int execute_command_internal(const Glib::ustring& command, const char* input, ExecFlags flags,
	                             StreamSlot stream_progress_slot,
	                             TimedSlot timed_progress_slot);

	Glib::ustring description ;
	OperationDetailStatus status ; 

	Glib::ustring treepath ;
	
	std::vector<OperationDetail*> sub_details;
	std::time_t time_start, time_elapsed ;
	bool no_more_children;  // Disallow adding more children to ensure captured errors
	                        // remain the last child of this operation detail.

	sigc::connection cancelconnection;
};

} //GParted

#endif /* GPARTED_OPERATIONDETAIL_H */
