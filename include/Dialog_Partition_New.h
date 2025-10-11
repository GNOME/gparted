/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
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

#ifndef GPARTED_DIALOG_PARTITION_NEW_H
#define GPARTED_DIALOG_PARTITION_NEW_H


#include "Device.h"
#include "Dialog_Base_Partition.h"
#include "FileSystem.h"
#include "OptionComboBox.h"
#include "Partition.h"
#include "Utils.h"

#include <gtkmm/entry.h>
#include <gtkmm/grid.h>
#include <vector>


namespace GParted
{


class Dialog_Partition_New : public Dialog_Base_Partition
{
public:
	Dialog_Partition_New(const Device & device,
	                     const Partition & selected_partition,
	                     bool any_extended,
	                     unsigned short new_count,
	                     const std::vector<FS> & FILESYSTEMS );
	~Dialog_Partition_New();

	Dialog_Partition_New(const Dialog_Partition_New& src) = delete;             // Copy construction prohibited
	Dialog_Partition_New& operator=(const Dialog_Partition_New& rhs) = delete;  // Copy assignment prohibited

	const Partition & Get_New_Partition();

private:
	void set_data( const Device & device,
	               const Partition & partition,
	               bool any_extended,
	               unsigned short new_count,
	               const std::vector<FS> & FILESYSTEMS );
	void build_filesystems_combo(bool only_unformatted);
	Byte_Value get_filesystem_min_limit( FSType fstype );

	Gtk::Grid grid_create;
	OptionComboBox combo_type;
	OptionComboBox combo_filesystem;
	Gtk::Entry partition_name_entry;
	Gtk::Entry filesystem_label_entry;

	std::vector<FS> FILESYSTEMS ;

	//signal handlers
	void combobox_changed(bool combo_type_changed);

	unsigned short new_count;
	int default_fs;
};


}  // namespace GParted


#endif /* GPARTED_DIALOG_PARTITION_NEW_H */
