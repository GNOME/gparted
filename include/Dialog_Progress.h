/* Copyright (C) 2004 Bart
 * Copyright (C) 2009 Curtis Gedak
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
#include "../include/Operation.h"

#include <gtkmm/dialog.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/expander.h>

#include <fstream>

namespace GParted
{

class Dialog_Progress : public Gtk::Dialog
{
public:
	Dialog_Progress( const std::vector<Operation *> & operations ) ;
	~Dialog_Progress();
	
	sigc::signal< bool, Operation * > signal_apply_operation ;
	sigc::signal< Glib::ustring > signal_get_libparted_version ;
		
private:
	void on_signal_update( const OperationDetail & operationdetail ) ;
	void update_gui_elements() ;
	void on_signal_show() ;
	void on_cell_data_description( Gtk::CellRenderer * renderer, const Gtk::TreeModel::iterator & iter) ;
	void on_cancel() ;
	void on_save() ;
	void echo_operation_details( const OperationDetail & operation_detail, std::ofstream & out ) ;
	
	void on_response( int response_id ) ;
	bool on_delete_event( GdkEventAny * event ) ;
	bool pulsebar_pulse();

	Gtk::Label label_current ;
	Gtk::Label label_current_sub ;
	Gtk::ProgressBar progressbar_all, progressbar_current ;
	Gtk::TreeView treeview_operations ;
	Gtk::TreeRow treerow ;
	Gtk::ScrolledWindow scrolledwindow ;
	Gtk::Expander expander_details ;
	Gtk::Button *cancelbutton;
	
	Glib::RefPtr<Gdk::Pixbuf> icon_execute ;
	Glib::RefPtr<Gdk::Pixbuf> icon_succes ;
	Glib::RefPtr<Gdk::Pixbuf> icon_error ;
	Glib::RefPtr<Gdk::Pixbuf> icon_info ;
	Glib::RefPtr<Gdk::Pixbuf> icon_n_a ;

	Glib::RefPtr<Gtk::TreeStore> treestore_operations;
	
	struct treeview_operations_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> operation_description;
		Gtk::TreeModelColumn<Glib::ustring> elapsed_time ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > status_icon;
				
		treeview_operations_Columns() 
		{ 
			add( operation_description );
			add( elapsed_time );
			add( status_icon ) ;
		} 
	};
	treeview_operations_Columns treeview_operations_columns;
	
	std::vector<Operation *> operations ;
	OperationDetail operationdetail ;
	bool succes, cancel;
	double fraction ;
	unsigned int t, warnings ;
	sigc::connection pulsetimer;
	Glib::ustring label_current_sub_text ;
	unsigned int cancel_countdown;
	sigc::connection canceltimer;
	bool cancel_timeout();
};

}//GParted

#endif //DIALOG_PARTITION_PROGRESS
