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
#include "../include/DrawingAreaVisualDisk.h"
#include "../include/Partition.h"
#include "../include/TreeView_Detail.h"
#include "../include/Operation.h"
#include "../include/GParted_Core.h"

#include <sigc++/class_slot.h>
#include <gtkmm/paned.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/menubar.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/liststore.h>
#include <gtkmm/combobox.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/window.h>
#include <gtkmm/table.h>

#include <unistd.h> //should be included by gtkmm headers. but decided to include it anyway after getting some bugreports..

namespace GParted
{
	
class Win_GParted : public Gtk::Window
{
public:
	Win_GParted( const std::vector<Glib::ustring> & user_devices ) ;

private:
	void init_menubar() ;
	void init_toolbar() ;
	void init_partition_menu() ;
	Gtk::Menu * create_format_menu() ;
	void init_device_info() ;
	void init_operationslist() ;
	void init_hpaned_main() ;

	void refresh_combo_devices() ;
	void show_pulsebar( const Glib::ustring & status_message ) ;
	
	//Fill txtview_device_info_buffer with some information about the selected device
	void Fill_Label_Device_Info( bool clear = false );

	//overridden signalhandler
	bool on_delete_event( GdkEventAny* ) ;
	
	void Add_Operation( OperationType, const Partition & );
	void Refresh_Visual();
	bool Quit_Check_Operations();
	void set_valid_operations() ;
	
	//convenience functions
	void allow_new( bool b )	{ 
		menu_partition .items()[ 0 ] .set_sensitive( b );
		toolbar_main .get_nth_item( 0 ) ->set_sensitive( b ); }
		
	void allow_delete( bool b )	{ 
		menu_partition .items()[ 1 ] .set_sensitive( b ); 
		toolbar_main .get_nth_item( 1 ) ->set_sensitive( b ); }
		
	void allow_resize( bool b ) 	{ 
		menu_partition .items()[ 3 ] .set_sensitive( b ); 
		toolbar_main .get_nth_item( 3 ) ->set_sensitive( b ); }
		
	void allow_copy( bool b )	{ 
		menu_partition .items()[ 5 ] .set_sensitive( b ); 
		toolbar_main .get_nth_item( 5 ) ->set_sensitive( b ); }
		
	void allow_paste( bool b )	{ 
		menu_partition .items()[ 6 ] .set_sensitive( b ); 
		toolbar_main .get_nth_item( 6 ) ->set_sensitive( b ); }
		
	void allow_format( bool b )	{ 
		menu_partition .items()[ 8 ] .set_sensitive( b ); }
	
	void allow_toggle_swap_mount_state( bool b )	{ 
		menu_partition .items()[ 10 ] .set_sensitive( b ); }
	
	void allow_info( bool b )	{
		menu_partition .items()[ 12 ] .set_sensitive( b ); }
	
	void allow_undo( bool b )	{ 
		toolbar_main .get_nth_item( 8 ) ->set_sensitive( b ); 
		static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 1 ] .get_submenu() ->items()[ 0 ] )
			->set_sensitive( b ) ; }
		
	void allow_apply( bool b )	{ 
		toolbar_main .get_nth_item( 9 ) ->set_sensitive( b ); 
		static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 1 ] .get_submenu() ->items()[ 1 ] )
			->set_sensitive( b ) ; }
		
	//threads..
	void thread_refresh_devices() ;
	void thread_unmount_partition( bool * succes, Glib::ustring * error ) ;
	void thread_mount_partition( bool * succes, Glib::ustring * error ) ;
	void thread_toggle_swap( bool * succes, Glib::ustring * error ) ;
		
	//signal handlers
	void open_operationslist() ;
	void close_operationslist() ;
	void clear_operationslist() ;
	void combo_devices_changed();
	void radio_devices_changed( unsigned int item ) ;
	void on_signal_show() ;
		
	void menu_gparted_refresh_devices();
	void menu_gparted_filesystems();
	void menu_gparted_quit();
	void menu_view_harddisk_info();
	void menu_view_operations();
	void menu_help_contents();
	void menu_help_about();

	void on_partition_selected( const Partition & partition, bool src_is_treeview ) ;
	void on_partition_activated() ;
	void on_partition_popup_menu( unsigned int button, unsigned int time ) ;
	
	bool max_amount_prim_reached() ;
	
	void activate_resize(); 
	void activate_copy();
	void activate_paste();
	void activate_new();
	void activate_delete();
	void activate_info();
	void activate_format( GParted::FILESYSTEM new_fs );
	void toggle_swap_mount_state() ;
	void activate_disklabel() ;
	
	void activate_undo();
	void activate_apply();

//private variables
	unsigned int current_device ;
	Partition selected_partition, copied_partition;
	std::vector<Device> devices;
	std::vector<Operation> operations;

//gui stuff
	Gtk::HPaned hpaned_main;
	Gtk::VPaned vpaned_main;
	Gtk::VBox vbox_main,vbox_info, *vbox;
	Gtk::HBox hbox_toolbar, hbox_operations, *hbox;
	Gtk::Toolbar toolbar_main;
	Gtk::MenuBar menubar_main;
	Gtk::ComboBox combo_devices ;
	Gtk::Menu menu_partition, *menu ;
	Gtk::ToolButton *toolbutton;
	Gtk::Statusbar statusbar;
	Gtk::Image *image ;
	Gtk::ScrolledWindow *scrollwindow;
	Gtk::Label label_device_info1, label_device_info2, *label;
	Gtk::Tooltips tooltips ;
	Gtk::Button *button;
	Gtk::Table *table ;
	Gtk::MenuItem *menu_item;
	Gtk::ProgressBar pulsebar ;
	Gtk::TreeRow treerow;
	
	DrawingAreaVisualDisk drawingarea_visualdisk ;
	TreeView_Detail treeview_detail;

	//device combo
	Glib::RefPtr<Gtk::ListStore> liststore_devices ;

	struct treeview_devices_Columns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon ;
		Gtk::TreeModelColumn<Glib::ustring> device ;
		Gtk::TreeModelColumn<Glib::ustring> size ;

		treeview_devices_Columns()
		{
			add( icon ) ;
			add( device ) ;
			add( size ) ;
		}
	};
	treeview_devices_Columns treeview_devices_columns ;
	
	//operations list
	Gtk::TreeView treeview_operations;
	Glib::RefPtr<Gtk::ListStore> liststore_operations;
	
	struct treeview_operations_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> operation_description;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > operation_icon;
				
		treeview_operations_Columns() 
		{ 
			add( operation_description );
			add( operation_icon );
		} 
	};
	treeview_operations_Columns treeview_operations_columns;
	
	//usefull variables which are used by many different functions...
	bool any_extended;//used in some checks 
	unsigned short primary_count ;//primary_count checks for max. of 4 pimary partitions
	unsigned short new_count;//new_count keeps track of the new created partitions
	FS fs ;
	bool OPERATIONSLIST_OPEN ;
									
	GParted_Core gparted_core ;
	GParted::Device *temp_device ;
	std::vector<Gtk::Label *> device_info ;
					
	//stuff for progress overview and pulsebar
	Glib::Thread *thread ;
	bool pulse ;
};

} //GParted

#endif //WIN_GPARTED
