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
#include <glibmm/ustring.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>


namespace GParted
{


class DialogPasswordEntry : public Gtk::Dialog
{
public:
	DialogPasswordEntry(const Partition& partition, const Glib::ustring& reason);
	const char * get_password();
	void set_error_message( const Glib::ustring & message );

private:
	void on_button_unlock();

	Gtk::Entry *m_entry;
	Gtk::Label *m_error_message;
};


}  // namespace GParted


#endif /* GPARTEDPASSWORDENTRY_H */
