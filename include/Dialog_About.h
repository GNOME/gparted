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
 
 /*
 of course i could've used Gnome::UI::About but that would add another dependacy to GParted
 Besides, i always wanted to build my own creditsdialog :-P
 */
 
#ifndef DIALOG_ABOUT
#define DIALOG_ABOUT

#include "../include/i18n.h"

#include <gtkmm/dialog.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/stock.h>
#include <gtkmm/notebook.h>

class Dialog_About : public Gtk::Dialog
{
public:
	Dialog_About() ;
	
	
private:
	void	Show_Credits() ;

	Gtk::Label *label_temp ;
	Gtk::Button button_credits;
};


#endif
