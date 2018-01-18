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

#ifndef GPARTED_DIALOG_PARTITION_RESIZE_MOVE_H
#define GPARTED_DIALOG_PARTITION_RESIZE_MOVE_H

#include "Dialog_Base_Partition.h"
#include "FileSystem.h"
#include "Partition.h"
#include "PartitionVector.h"

namespace GParted
{

class Dialog_Partition_Resize_Move : public Dialog_Base_Partition
{
public:
	Dialog_Partition_Resize_Move( const FS & fs, const FS_Limits & fs_limits,
	                              const Partition & selected_partition,
	                              const PartitionVector & partitions );
	~Dialog_Partition_Resize_Move();

private:
	Dialog_Partition_Resize_Move( const Dialog_Partition_Resize_Move & src );              // Not implemented copy constructor
	Dialog_Partition_Resize_Move & operator=( const Dialog_Partition_Resize_Move & rhs );  // Not implemented assignment operator

	void set_data( const Partition & selected_partition, const PartitionVector & partitions );
	void Resize_Move_Normal( const PartitionVector & partitions );
	void Resize_Move_Extended( const PartitionVector & partitions );
};

} //GParted

#endif /* GPARTED_DIALOG_PARTITION_RESIZE_MOVE_H */
