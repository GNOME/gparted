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
#include "../include/Utils.h"

#include <gtkmm/dialog.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/stock.h>
#include <gtkmm/label.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/textview.h>
#include <gtkmm/textbuffer.h>
#include <gtkmm/scrolledwindow.h>

namespace GParted
{

class Dialog_Progress : public Gtk::Dialog
{
public:
	Dialog_Progress( int count_operations, Glib::RefPtr<Gtk::TextBuffer> textbuffer );
	~Dialog_Progress( );

	void Set_Operation( );
		
	Glib::ustring current_operation;
	int TIME_LEFT ;
		
private:
	bool Show_Progress( ) ;
	bool Pulse( ) { progressbar_current .pulse( ) ; return true ; }
	void tglbtn_details_toggled( ) ;

	Gtk::Label label_current ;
	Gtk::ProgressBar progressbar_all, progressbar_current ;
	Gtk::ToggleButton tglbtn_details ;
	Gtk::TextView textview_details ;
	Gtk::ScrolledWindow scrolledwindow ;
	
	void signal_textbuffer_insert( const Gtk::TextBuffer::iterator & iter, const Glib::ustring & text, int ) ;
	
	double fraction, fraction_current;
	int count_operations, current_operation_number;
	sigc::connection conn ;
};

}//GParted

#endif //DIALOG_PARTITION_PROGRESS
