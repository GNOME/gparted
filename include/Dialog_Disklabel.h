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

#include "../include/i18n.h"
#include "../include/Utils.h"

#include <gtkmm/dialog.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/expander.h>
#include <gtkmm/optionmenu.h>
#include <gtkmm/menu.h>

namespace GParted
{
	
class Dialog_Disklabel : public Gtk::Dialog
{
public:
	Dialog_Disklabel( const Glib::ustring & device_path ) ;
	
	Glib::ustring Get_Disklabel( ) ;
	
private:
	Gtk::Expander expander_advanced ;
	Gtk::HBox *hbox ;
	Gtk::VBox *vbox ;
	Gtk::Image image ;
	Gtk::OptionMenu optionmenu_labeltypes ;
	Gtk::Menu menu_labeltypes ;

	Glib::ustring str_temp ;
	std::vector <Glib::ustring> labeltypes ;
};

} //GParted


#endif //DIALOG_DISKLABEL
