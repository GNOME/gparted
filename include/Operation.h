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
#include <glibmm/ustring.h>

namespace GParted
{
		
enum OperationType {
        DELETE				=	0,
		CREATE				=	1,
		RESIZE_MOVE		=	2,
		CONVERT			=	3,
		COPY					=	4
};

class Operation
{
	
public:
	Operation( Device *device, Device *source_device, const Partition &, const Partition &,OperationType );
	Glib::ustring Get_String();
	
	//this one can be a little confusing, it *DOES NOT* change any visual representation. It only applies the operation to the list with partitions.
	//this new list can be used to change the visual representation. For real writing to disk, see Apply_To_Disk()
	std::vector<Partition> Apply_Operation_To_Visual( std::vector<Partition> & partitions );
	
	void Apply_To_Disk( PedTimer * );
	
	//public variables
	Device *device, *source_device;  //source_device is only used in copy operations
	Glib::ustring device_path, source_device_path ;
	OperationType operationtype;
	Partition partition_new; //the new situation ( can be an whole new partition or simply the old one with a new size or.... )

private:
	std::vector<Partition> Apply_Delete_To_Visual( std::vector<Partition> & partitions );
	std::vector<Partition> Apply_Create_To_Visual( std::vector<Partition> & partitions );
	std::vector<Partition> Apply_Resize_Move_To_Visual( std::vector<Partition> & partitions );
	std::vector<Partition> Apply_Resize_Move_Extended_To_Visual( std::vector<Partition> & partitions );
	std::vector<Partition> Apply_Convert_To_Visual( std::vector<Partition> & partitions );

	void Show_Error( const Glib::ustring & message  ) ;

	Partition partition_original; //the original situation
	char c_buf[ 1024 ] ; //used by sprintf, which is needed for i18n
};

} //GParted

#endif //OPERATION
