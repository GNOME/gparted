/* Copyright (C) 2008 Curtis Gedak
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

#ifndef GPARTED_DIALOG_FILESYSTEM_LABEL_H
#define GPARTED_DIALOG_FILESYSTEM_LABEL_H

#include "Partition.h"
#include "i18n.h"

#include <gtkmm/dialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/frame.h>
#include <gtkmm/table.h>
#include <gtkmm/entry.h>

#define BORDER 8

namespace GParted
{

class Dialog_FileSystem_Label : public Gtk::Dialog
{
public:
	Dialog_FileSystem_Label( const Partition & partition );
	~Dialog_FileSystem_Label();
	Glib::ustring get_new_label();

private:
	Gtk::Entry *entry;
};

} //GParted

#endif /* GPARTED_DIALOG_FILESYSTEM_LABEL_H */
