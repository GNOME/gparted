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

#ifndef DIALOG_PARTITION_PROGRESS
#define DIALOG_PARTITION_PROGRESS

#include "../include/i18n.h"

#include <gtkmm/dialog.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/stock.h>
#include <gtkmm/label.h>

#include <sstream>

//compose library, dedicated to the translators :P
#include "../compose/ucompose.hpp"

class Dialog_Progress : public Gtk::Dialog
{
public:
	Dialog_Progress( int, const Glib::ustring & );
	~Dialog_Progress();

	void Set_Next_Operation( );
	void Set_Progress_Current_Operation();
	
	Glib::ustring current_operation;
	float fraction_current;
	int time_left ;
	
private:
	Gtk::Label label_all_operations, label_current, *label_temp;
	Gtk::ProgressBar progressbar_all, progressbar_current ;
	
	double fraction;
	int count_operations, current_operation_number;
	char c_buf[ 1024 ] ; //used by sprintf, which is needed for i18n
};

#endif //DIALOG_PARTITION_PROGRESS
