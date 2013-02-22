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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "../include/OperationDetail.h"
#include "../include/Utils.h"

namespace GParted
{

OperationDetail::OperationDetail()
{
	status = STATUS_NONE; // prevent uninitialized member access
	set_status( STATUS_NONE ) ;
	fraction = -1 ;
	time_elapsed = -1 ;
}

OperationDetail::OperationDetail( const Glib::ustring & description, OperationDetailStatus status, Font font )
{
	this ->status = STATUS_NONE; // prevent uninitialized member access
	set_description( description, font ) ;
	set_status( status ) ;

	fraction = -1 ;
	time_elapsed = -1 ;
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
			case STATUS_SUCCES:
				time_elapsed = std::time( NULL ) - time_start ;
				break ;

			default:
				break ;
		}

		this ->status = status ;
		on_update( *this ) ;
	}
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
	sub_details .push_back( operationdetail ) ;

	sub_details .back() .set_treepath( treepath + ":" + Utils::num_to_str( sub_details .size() -1 ) ) ;
	sub_details .back() .signal_update .connect( sigc::mem_fun( this, &OperationDetail::on_update ) ) ;
	
	on_update( sub_details .back() ) ;
}
	
std::vector<OperationDetail> & OperationDetail::get_childs() 
{
	return sub_details ;
}

const std::vector<OperationDetail> & OperationDetail::get_childs() const
{
	return sub_details ;
}

OperationDetail & OperationDetail::get_last_child()
{
	//little bit of (healthy?) paranoia
	if ( sub_details .size() == 0 )
		add_child( OperationDetail( "---", STATUS_ERROR ) ) ;

	return sub_details[sub_details.size() - 1];
}

void OperationDetail::on_update( const OperationDetail & operationdetail ) 
{
	if ( ! treepath .empty() )
		signal_update .emit( operationdetail ) ;
}

} //GParted
