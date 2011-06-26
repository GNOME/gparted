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
 

#ifndef DIALOG_DISKLABEL
#define DIALOG_DISKLABEL

#include "../include/Utils.h"

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
	Dialog_Disklabel( const Glib::ustring & device_path, const std::vector<Glib::ustring> & disklabeltypes ) ;
	
	Glib::ustring Get_Disklabel( ) ;
	
private:
	Gtk::Expander expander_advanced ;
	Gtk::Image image ;
	Gtk::ComboBoxText combo_labeltypes ;
	std::vector<Glib::ustring> labeltypes ;
};

} //GParted


#endif //DIALOG_DISKLABEL
