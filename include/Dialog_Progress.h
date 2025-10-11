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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GPARTED_DIALOG_PROGRESS_H
#define GPARTED_DIALOG_PROGRESS_H

#include "i18n.h"
#include "Utils.h"
#include "Device.h"
#include "Operation.h"
#include "Partition.h"

#include <gtkmm/dialog.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/expander.h>
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <fstream>
#include <vector>


namespace GParted
{


class Dialog_Progress : public Gtk::Dialog
{
public:
	Dialog_Progress(const std::vector<Device>& devices, const OperationVector& operations);
	~Dialog_Progress();

	sigc::signal< bool, Operation * > signal_apply_operation ;

private:
	void on_signal_update( const OperationDetail & operationdetail ) ;
	void update_gui_elements() ;
	void on_signal_show() ;
	void on_cell_data_description( Gtk::CellRenderer * renderer, const Gtk::TreeModel::iterator & iter) ;
	bool cancel_timeout();
	void on_cancel() ;
	void on_save() ;
	void write_device_details(const Device& device, std::ofstream& out);
	void write_partition_details(const Partition& partition, std::ofstream& out);
	void write_operation_details(const OperationDetail& operation_detail, std::ofstream& out);

	void on_response( int response_id ) ;
	bool on_delete_event( GdkEventAny * event ) ;
	bool pulsebar_pulse();

	// Child widgets
	Gtk::Label          m_label_current;
	Gtk::ProgressBar    m_progressbar_current;
	Gtk::Label          m_label_current_sub;
	Gtk::ProgressBar    m_progressbar_all;
	Gtk::Expander       m_expander_details;
	Gtk::ScrolledWindow m_scrolledwindow;
	Gtk::TreeView       m_treeview_operations;
	Gtk::Button*        m_cancelbutton;

	// Model for displaying the operation details
	Glib::RefPtr<Gtk::TreeStore> m_treestore_operations;
	struct TreeView_Operations_Columns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn<Glib::ustring> operation_description;
		Gtk::TreeModelColumn<Glib::ustring> elapsed_time ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > status_icon;

		TreeView_Operations_Columns()
		{ 
			add( operation_description );
			add( elapsed_time );
			add( status_icon ) ;
		} 
	};
	TreeView_Operations_Columns m_treeview_operations_columns;

	Glib::RefPtr<Gdk::Pixbuf> m_icon_execute;
	Glib::RefPtr<Gdk::Pixbuf> m_icon_success;
	Glib::RefPtr<Gdk::Pixbuf> m_icon_error;
	Glib::RefPtr<Gdk::Pixbuf> m_icon_info;
	Glib::RefPtr<Gdk::Pixbuf> m_icon_warning;

	const std::vector<Device>& m_devices;
	const OperationVector&     m_operations;
	const double               m_fraction;
	bool                       m_success;
	bool                       m_cancel;
	unsigned int               m_curr_op;
	unsigned int               m_warnings;
	Glib::ustring              m_progress_text;
	Glib::ustring              m_label_current_sub_text;
	unsigned int               m_cancel_countdown;

	sigc::connection pulsetimer;
	sigc::connection canceltimer;
};


}  // namespace GParted


#endif /* GPARTED_DIALOG_PROGRESS_H */
