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
 
#ifndef DIALOG_PARTITION_NEW
#define DIALOG_PARTITION_NEW

#include "../include/Dialog_Base_Partition.h"

#include <gtkmm/messagedialog.h>
#include <gtkmm/optionmenu.h>

namespace GParted
{

class Dialog_Partition_New : public Dialog_Base_Partition
{
public:
	Dialog_Partition_New() ;
	void Set_Data( const Partition & partition, bool any_extended, unsigned short new_count, const std::vector <FS> & FILESYSTEMS, bool only_unformatted );
	Partition Get_New_Partition() ;//overridden function

private:
	void Build_Filesystems_Menu( bool only_unformatted ) ;
	
	Gtk::Table table_create;
	Gtk::OptionMenu optionmenu_type, optionmenu_filesystem;
	Gtk::Menu menu_type, menu_filesystem;

	std::vector<FS> FILESYSTEMS ;
	
	//signal handlers
	void optionmenu_changed( bool );

	Gdk::Color color_temp;
	unsigned short new_count, first_creatable_fs ;
};

} //GParted

#endif //DIALOG_PARTITION_NEW
