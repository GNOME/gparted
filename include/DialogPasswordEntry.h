/* Copyright (C) 2017 Mike Fleetwood
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

#ifndef GPARTED_DIALOGPASSWORDENTRY_H
#define GPARTED_DIALOGPASSWORDENTRY_H

#include "Partition.h"

#include <gtkmm/dialog.h>
#include <gtkmm/entry.h>

namespace GParted
{

class DialogPasswordEntry : public Gtk::Dialog
{
public:
	DialogPasswordEntry( const Partition & partition );
	~DialogPasswordEntry();
	const char * get_password();

private:
	Gtk::Entry *entry;
};

} //GParted

#endif /* GPARTEDPASSWORDENTRY_H */
