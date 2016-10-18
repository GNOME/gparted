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


#ifndef GPARTED_DIALOG_DISKLABEL_H
#define GPARTED_DIALOG_DISKLABEL_H

#include "Utils.h"
#include "Device.h"

#include <gtkmm/dialog.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/expander.h>
#include <gtkmm/comboboxtext.h>

namespace GParted
{
	
class Dialog_Disklabel : public Gtk::Dialog
{
public:
	Dialog_Disklabel( const Device & device ) ;

	Glib::ustring Get_Disklabel( ) ;
	
private:
	Gtk::Image image ;
	Gtk::ComboBoxText combo_labeltypes ;
	std::vector<Glib::ustring> labeltypes ;
};

} //GParted

#endif /* GPARTED_DIALOG_DISKLABEL_H */
