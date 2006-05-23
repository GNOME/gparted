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

#ifndef OPERATION
#define OPERATION

#include "../include/Device.h"

namespace GParted
{
		
enum OperationType {
        DELETE		= 0,
	CREATE		= 1,
	RESIZE_MOVE	= 2,
	FORMAT		= 3,
	COPY		= 4
};

struct OperationDetails
{
	enum Status {
		NONE	= -1,
		EXECUTE	= 0,
		SUCCES	= 1,
		ERROR	= 2
	};
	
	OperationDetails()
	{
		status = NONE ;
		fraction = -1 ;
	}

	OperationDetails( const Glib::ustring & description, Status status = EXECUTE )
	{
		this ->description = description ;
		this ->status = status ;

		fraction = -1 ;
	}

	Glib::ustring description ;
	Status status ; 
	double fraction ;
	Glib::ustring progress_text ;

	std::vector<OperationDetails> sub_details ; 	
};

class Operation
{
	
public:
	Operation() ;
	virtual ~Operation() {}
	
	virtual void apply_to_visual( std::vector<Partition> & partitions ) = 0 ;

	//public variables
	Device device ;
	OperationType type ;
	Partition partition_original ; 
	Partition partition_new ;

	Glib::RefPtr<Gdk::Pixbuf> icon ;
	Glib::ustring description ;

	OperationDetails operation_details ;

protected:
	int find_index_original( const std::vector<Partition> & partitions ) ;
	int find_index_extended( const std::vector<Partition> & partitions ) ;
	void insert_unallocated( std::vector<Partition> & partitions, Sector start, Sector end, bool inside_extended );
	
	virtual void create_description() = 0 ;

	int index ;
	int index_extended ;
};

} //GParted

#endif //OPERATION
