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

#include "Dialog_Base_Partition.h"
#include "Device.h"
#include "Partition.h"
#include "Utils.h"

#include <gtkmm/optionmenu.h>

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

	const Partition & Get_New_Partition();

private:
	Dialog_Partition_New( const Dialog_Partition_New & src );              // Not implemented copy constructor
	Dialog_Partition_New & operator=( const Dialog_Partition_New & rhs );  // Not implemented copy assignment operator


	void set_data( const Device & device,
	               const Partition & partition,
	               bool any_extended,
	               unsigned short new_count,
	               const std::vector<FS> & FILESYSTEMS );
	void Build_Filesystems_Menu( bool only_unformatted ) ;
	Byte_Value get_filesystem_min_limit( FSType fstype );

	Gtk::Table table_create;
	Gtk::OptionMenu optionmenu_type, optionmenu_filesystem;
	Gtk::Menu menu_type, menu_filesystem;
	Gtk::Entry partition_name_entry;
	Gtk::Entry filesystem_label_entry;

	std::vector<FS> FILESYSTEMS ;
	
	//signal handlers
	void optionmenu_changed( bool );

	unsigned short new_count, first_creatable_fs ;
};

} //GParted

#endif /* GPARTED_DIALOG_PARTITION_NEW_H */
