/*	Copyright (C) 2004 Bart
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
 
#ifndef WIN_GPARTED
#define WIN_GPARTED

#include "../include/Device.h"
#include "../include/VBox_VisualDisk.h"
#include "../include/Partition.h"
#include "../include/TreeView_Detail.h"
#include "../include/Dialog_Partition_Info.h"
#include "../include/Dialog_Partition_New.h"
#include "../include/Operation.h"
#include "../include/Dialog_Progress.h"
#include "../include/Dialog_Partition_Resize_Move.h"
#include "../include/Dialog_About.h"
#include "../include/Dialog_Partition_Copy.h"

#include <sigc++/class_slot.h>
#include <gtkmm/main.h>
#include <gtkmm/paned.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/menubar.h>
#include <gtkmm/expander.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>

#include <fstream>

namespace GParted
{
	
class Win_GParted : public Gtk::Window
{
public:
	Win_GParted(   );

private:
	void init_menubar() ;
	void init_toolbar() ;
	void init_popupmenu() ;
	void init_convert_menu() ;
	void init_device_info() ;
	void init_operationslist() ;
	void init_hpaned_main() ;

	void Find_Devices() ;
	
	//Fill txtview_device_info_buffer with some information about the selected device
	void Fill_Label_Device_Info( );

	//overridden signalhandler
	virtual bool on_delete_event(GdkEventAny* ) ;

	void Add_Operation( OperationType, const Partition & );
	void Refresh_Visual( );
	bool Quit_Check_Operations();
	void Set_Valid_Operations() ; //determines which operations are allowed on selected_partition 
	void Set_Valid_Convert_Filesystems() ; //determines to which filesystems a partition can be converted
	

	//convenience functions
	void allow_new( bool b )			{ menu_popup.items()[0].set_sensitive( b ); toolbar_main.get_nth_item(0) ->set_sensitive( b ); }
	void allow_delete( bool b ) 		{ menu_popup.items()[1].set_sensitive( b ); toolbar_main.get_nth_item(1) ->set_sensitive( b ); }
	void allow_resize( bool b ) 	 	{ menu_popup.items()[3].set_sensitive( b ); toolbar_main.get_nth_item(3) ->set_sensitive( b ); }
	void allow_copy( bool b )			{ menu_popup.items()[5].set_sensitive( b ); toolbar_main.get_nth_item(5) ->set_sensitive( b ); }
	void allow_paste( bool b )			{ menu_popup.items()[6].set_sensitive( b ); toolbar_main.get_nth_item(6) ->set_sensitive( b ); }
	void allow_convert( bool b )		{ menu_popup.items()[8].set_sensitive( b ); }
	void allow_undo( bool b )			{ toolbar_main.get_nth_item(8) ->set_sensitive( b ); }
	void allow_apply( bool b ) 			{ toolbar_main.get_nth_item(9) ->set_sensitive( b ); }
	
	//signal handlers
	void close_operationslist() ;
	void clear_operationslist() ;
	void optionmenu_devices_changed( );
	
	void menu_gparted_refresh_devices();
	void menu_gparted_quit();
	void menu_help_contents();
	void menu_help_about();

	void mouse_click( GdkEventButton*,  const Partition & );
	
	void activate_resize(); 
	void activate_copy();
	void activate_paste();
	void activate_new();
	void activate_delete();
	void activate_info();
	void activate_convert( const Glib::ustring & new_fs  );
	
	void activate_undo();
	void activate_apply();
	void apply_operations_thread();

//private variables
	unsigned int current_device, source_device; //source_device is used to store the device the copied_partition is from....
	Partition selected_partition, copied_partition;
	std::vector <Device * > devices;
	std::vector <Operation> operations;

//gui stuff
	Gtk::HPaned hpaned_main;
	Gtk::VPaned vpaned_main;
	Gtk::VBox vbox_main,vbox_info, *vbox;
	Gtk::HBox hbox_toolbar,hbox_visual, hbox_operations, *hbox;
	Gtk::Toolbar toolbar_main;
	Gtk::MenuBar menubar_main;
	Gtk::OptionMenu optionmenu_devices;
	Gtk::Menu menu_devices,menu_popup, menu_convert, *menu ;
	Gtk::ToolButton *toolbutton;
	Gtk::Statusbar statusbar;
	Gtk::Expander expander_device_info ;
	Gtk::Image *image ;
	Gtk::ScrolledWindow *scrollwindow;
	Gtk::Label label_device_info1, label_device_info2, *label;
	Gtk::Tooltips tooltips ;
	Gtk::Button *button;
	Gtk::Table *table ;
	Gtk::MenuItem *menu_item;
	Gtk::Entry *entry;
	
	Gdk::Color color ;
			
	VBox_VisualDisk *vbox_visual_disk;
	TreeView_Detail treeview_detail;
	
	//operations list
	Gtk::TreeView treeview_operations;
	Gtk::TreeRow treerow;
	Glib::RefPtr<Gtk::ListStore> liststore_operations;
	
	struct treeview_operations_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<int> operation_number;
		Gtk::TreeModelColumn<Glib::ustring> operation_description;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >  operation_icon;
				
		treeview_operations_Columns() {	add( operation_number ); add( operation_description );add(operation_icon);}
	};
	treeview_operations_Columns treeview_operations_columns;
	
	
	//usefull variables which are used by many different functions...
	sigc::connection s2,s3;//usefull for disconnecting and destroying a connection ( see optionmenu_devices_changed() and activate_apply() )
	std::ostringstream os;//for int to string conversions
	bool any_logic,any_extended;//used in some checks (e.g. see optionmenu_devices_changed()
	unsigned short highest_logic_busy,primary_count, new_count;//primary_count checks for max. of 4 pimary partitions, 
																								//new_count keeps track of the new created partitions,
																								//highest_logic_busy contains the number of the highest logical which is busy ( 0 if none is busy)
	
	GParted::Device *temp_device;
	std::vector <Glib::ustring> str_devices, filesystems;
	char c_buf[ 1024 ] ; //used by sprintf, which is needed for i18n
			
	//stuff for progress overview
	Dialog_Progress *dialog_progress;
	Glib::Thread *thread_operations;
	Glib::Dispatcher dispatcher_next_operation;
	bool apply;
	
};

} //GParted

#endif //WIN_GPARTED
