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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef OPERATIONDETAIL
#define OPERATIONDETAIL

#include <glibmm/ustring.h>
#include <glibmm/markup.h>

#include <vector>
#include <ctime>

namespace GParted
{

enum OperationDetailStatus {
	STATUS_NONE	= 0, 
	STATUS_EXECUTE	= 1,
	STATUS_SUCCES	= 2,
	STATUS_ERROR	= 3,
	STATUS_INFO	= 4,
	STATUS_N_A	= 5
};

enum Font {
	FONT_NORMAL	 = 0,
	FONT_BOLD	 = 1,
	FONT_ITALIC	 = 2,
	FONT_BOLD_ITALIC = 3
} ;

class OperationDetail
{
public:	
	OperationDetail() ;
	~OperationDetail();
	OperationDetail( const Glib::ustring & description,
			 OperationDetailStatus status = STATUS_EXECUTE,
			 Font font = FONT_NORMAL ) ;
	void set_description( const Glib::ustring & description, Font font = FONT_NORMAL ) ;
	Glib::ustring get_description() const ;
	void set_status( OperationDetailStatus status ) ;
	OperationDetailStatus get_status() const ;
	void set_treepath( const Glib::ustring & treepath ) ;
	Glib::ustring get_treepath() const ;
	Glib::ustring get_elapsed_time() const ;
	
	void add_child( const OperationDetail & operationdetail ) ;
	std::vector<OperationDetail> & get_childs() ;
	const std::vector<OperationDetail> & get_childs() const ;
	OperationDetail & get_last_child() ;

	double fraction ;
	Glib::ustring progress_text ;
	
	sigc::signal< void, const OperationDetail & > signal_update ;
	sigc::signal< void, bool > signal_cancel;
	char cancelflag;
private:
	void on_update( const OperationDetail & operationdetail ) ;
	void cancel( bool force );
	Glib::ustring description ;
	OperationDetailStatus status ; 

	Glib::ustring treepath ;
	
	std::vector<OperationDetail> sub_details ; 	
	std::time_t time_start, time_elapsed ;
	sigc::connection cancelconnection;
};

} //GParted


#endif //OPERATIONDETAIL

