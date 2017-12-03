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
#include "ProgressBar.h"
#include "Utils.h"

namespace GParted
{

// The single progress bar for the current operation
static ProgressBar single_progressbar;

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

void OperationDetail::set_description( const Glib::ustring & description, Font font )
{
	try
	{
		switch ( font )
		{
			case FONT_NORMAL:
		 		this ->description = Glib::Markup::escape_text( description ) ;
			 	break ;
			case FONT_BOLD:
		 		this ->description = "<b>" + Glib::Markup::escape_text( description ) + "</b>" ;
			 	break ;
			case FONT_ITALIC:
		 		this ->description = "<i>" + Glib::Markup::escape_text( description ) + "</i>" ;
			 	break ;
			case FONT_BOLD_ITALIC:
		 		this ->description = "<b><i>" + Glib::Markup::escape_text( description ) + "</i></b>" ;
			 	break ;
		}
	}
	catch ( Glib::Exception & e )
	{
		this ->description = e .what() ;
	}

	on_update( *this ) ;
}

Glib::ustring OperationDetail::get_description() const
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
				time_start = std::time( NULL ) ;
				break ;
			case STATUS_ERROR:
			case STATUS_WARNING:
			case STATUS_SUCCESS:
				if( time_start != -1 )
					time_elapsed = std::time( NULL ) - time_start ;
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

Glib::ustring OperationDetail::get_treepath() const
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

std::vector<OperationDetail*> & OperationDetail::get_childs()
{
	return sub_details ;
}

const std::vector<OperationDetail*> & OperationDetail::get_childs() const
{
	return sub_details ;
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

ProgressBar & OperationDetail::get_progressbar() const
{
	return single_progressbar;
}

} //GParted
