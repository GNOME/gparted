/* Copyright (C) 2015 Michael Zimmermann
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

#ifndef GPARTED_DIALOG_PARTITION_NAME_H
#define GPARTED_DIALOG_PARTITION_NAME_H

#include "Partition.h"
#include "i18n.h"

#include <gtkmm/dialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/table.h>
#include <gtkmm/entry.h>

namespace GParted
{

class Dialog_Partition_Name: public Gtk::Dialog
{
public:
	Dialog_Partition_Name( const Partition & partition, int max_length );
	~Dialog_Partition_Name();
	Glib::ustring get_new_name();

private:
	Gtk::Entry *entry;
};

} //GParted

#endif /* GPARTED_DIALOG_PARTITION_NAME_H */
