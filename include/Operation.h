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

#include "../include/Partition.h"
#include "../include/Device.h"
#include "../include/i18n.h"

#include <gtkmm/messagedialog.h>

namespace GParted
{
		
enum OperationType {
        DELETE		= 0,
	CREATE		= 1,
	RESIZE_MOVE	= 2,
	CONVERT		= 3,
	COPY		= 4
};

class Operation
{
	
public:
	Operation( const Glib::ustring device_path, Sector device_length, const Partition &, const Partition &, OperationType );
		
	//this one can be a little confusing, it *DOES NOT* change any visual representation. It only applies the operation to the list with partitions.
	//this new list can be used to change the visual representation. For real writing to disk, see Apply_To_Disk()
	void Apply_Operation_To_Visual( std::vector<Partition> & partitions );
	
	//public variables
	Glib::ustring device_path ;
	Sector device_length ;
	OperationType operationtype;
	Partition partition_original; //the original situation
	Partition partition_new; //the new situation ( can be an whole new partition or simply the old one with a new size or.... )
	Glib::ustring str_operation ;
	Glib::ustring copied_partition_path ; //for copy operation..

private:
	void Insert_Unallocated( std::vector<Partition> & partitions, Sector start, Sector end );
	int Get_Index_Original( std::vector<Partition> & partitions ) ;
	
	void Apply_Delete_To_Visual( std::vector<Partition> & partitions );
	void Apply_Create_To_Visual( std::vector<Partition> & partitions );
	void Apply_Resize_Move_To_Visual( std::vector<Partition> & partitions );
	void Apply_Resize_Move_Extended_To_Visual( std::vector<Partition> & partitions );
	
	Glib::ustring Get_String( ); //only used in c'tor
};

} //GParted

#endif //OPERATION
