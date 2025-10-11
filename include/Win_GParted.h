/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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

#ifndef GPARTED_WIN_GPARTED_H
#define GPARTED_WIN_GPARTED_H


#include "Device.h"
#include "DrawingAreaVisualDisk.h"
#include "GParted_Core.h"
#include "HBoxOperations.h"
#include "Operation.h"
#include "Partition.h"
#include "TreeView_Detail.h"
#include "Utils.h"

#include <gtkmm/paned.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/separatortoolitem.h>
#include <gtkmm/checkmenuitem.h>
#include <gtkmm/menubar.h>
#include <gtkmm/statusbar.h>
#include <gtkmm/combobox.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/window.h>
#include <sigc++/connection.h>
#include <vector>
#include <memory>


namespace GParted
{


class Win_GParted : public Gtk::Window
{
public:
	Win_GParted( const std::vector<Glib::ustring> & user_devices ) ;
	~Win_GParted();

	Win_GParted(const Win_GParted& src) = delete;             // Copy construction prohibited
	Win_GParted& operator=(const Win_GParted& rhs) = delete;  // Copy assignment prohibited

private:
	void init_menubar() ;
	void init_toolbar() ;
	void init_partition_menu() ;
	Gtk::Menu * create_format_menu() ;
	void create_format_menu_add_item(FSType fstype, bool activate);
	void init_device_info() ;
	void init_hpaned_main() ;
	void add_custom_css();

	void refresh_combo_devices() ;
	void show_pulsebar( const Glib::ustring & status_message ) ;
	void hide_pulsebar();
	void Fill_Label_Device_Info( bool clear = false );

	void add_operation(const Device& device, std::unique_ptr<Operation> operation);
	bool merge_operation(const Operation& candidate);
	static bool operations_affect_same_partition(const Operation& first_op, const Operation& second_op);
	void Refresh_Visual();
	bool valid_display_partition_ptr( const Partition * partition_ptr );
	bool Quit_Check_Operations();
	void set_valid_operations() ;
	void show_operationslist() ;
	
	//convenience functions
	void toggle_item( bool state, int menu_item, int toolbar_item = -1 )
        {
                if (menu_item >= 0 && partitionmenu_items.count(menu_item))
                        partitionmenu_items[menu_item]->set_sensitive(state);

                if ( toolbar_item >= 0 && toolbar_item < toolbar_main .get_n_items() )
                        toolbar_main .get_nth_item( toolbar_item ) ->set_sensitive( state ) ;
        }

	void allow_new( bool state )	{ 
		toggle_item( state, MENU_NEW, TOOLBAR_NEW ) ; }
		
	void allow_delete( bool state )	{ 
		toggle_item( state, MENU_DEL, TOOLBAR_DEL ) ; } 
		
	void allow_resize( bool state ) 	{ 
		toggle_item( state, MENU_RESIZE_MOVE, TOOLBAR_RESIZE_MOVE ) ; }
		
	void allow_copy( bool state )	{ 
		toggle_item( state, MENU_COPY, TOOLBAR_COPY ) ; }
		
	void allow_paste( bool state )	{ 
		toggle_item( state, MENU_PASTE, TOOLBAR_PASTE ) ; }
		
	void allow_format( bool state )	{ 
		toggle_item( state, MENU_FORMAT ) ; }

	void allow_toggle_crypt_busy_state( bool state ) {
		toggle_item( state, MENU_TOGGLE_CRYPT_BUSY ); }

	void allow_toggle_fs_busy_state( bool state ) {
		toggle_item( state, MENU_TOGGLE_FS_BUSY ); }

	void allow_manage_flags( bool state ) {
		toggle_item( state, MENU_FLAGS ) ; } 
	
	void allow_check( bool state ) {
		toggle_item( state, MENU_CHECK ) ; } 
	
	void allow_label_filesystem( bool state ) {
		toggle_item(state, MENU_LABEL_FILESYSTEM); }

	void allow_name_partition( bool state ) {
		toggle_item( state, MENU_NAME_PARTITION ); }

	void allow_change_uuid( bool state )	{
		toggle_item( state, MENU_CHANGE_UUID ) ; }

	void allow_info( bool state )	{
		toggle_item( state, MENU_INFO ) ; } 

	void allow_undo_clear_apply( bool state )
	{
		toggle_item( state, -1, TOOLBAR_UNDO ) ; 
		static_cast<Gtk::CheckMenuItem *>(mainmenu_items[MENU_UNDO_OPERATION])
			->set_sensitive( state ) ; 

		static_cast<Gtk::CheckMenuItem *>(mainmenu_items[MENU_CLEAR_OPERATIONS])
			->set_sensitive( state ) ; 

		toggle_item( state, -1, TOOLBAR_APPLY ) ; 
		static_cast<Gtk::CheckMenuItem *>(mainmenu_items[MENU_APPLY_OPERATIONS])
			->set_sensitive( state ) ; 
	}

	static bool unmount_partition( const Partition & partition, Glib::ustring & error );

	//signal handlers
	void open_operationslist() ;
	void close_operationslist() ;
	void clear_operationslist() ;
	void combo_devices_changed();
	void radio_devices_changed( unsigned int item ) ;
	bool on_delete_event( GdkEventAny* ) ;
	void on_show() ;

	static gboolean initial_device_refresh( gpointer data );
	void menu_gparted_refresh_devices();
	void menu_gparted_features();
	void menu_gparted_quit();
	void menu_view_harddisk_info();
	void menu_view_operations();
	void show_disklabel_unrecognized(const Glib::ustring& device_name);
	void show_resize_readonly( const Glib::ustring & path );
	void show_help(const Glib::ustring & filename, const Glib::ustring & link_id);
	void menu_help_contents();
	void menu_help_about();

	void on_partition_selected( const Partition * partition_ptr, bool src_is_treeview );
	void on_partition_activated() ;
	void on_partition_popup_menu( unsigned int button, unsigned int time ) ;
	
	bool max_amount_prim_reached() ;
	
	void activate_resize(); 
	bool ask_for_password_for_encrypted_resize_as_required(const Partition& partition);
	void activate_copy();
	void activate_paste();
	void activate_new();
	void activate_delete();
	void activate_info();
	void activate_format( FSType new_fs );
	bool open_encrypted_partition( const Partition & partition, const char * password, Glib::ustring & message );
	void toggle_crypt_busy_state();
	bool check_toggle_busy_allowed( const Glib::ustring & disallowed_msg );
	void show_toggle_failure_dialog( const Glib::ustring & failure_summary,
	                                 const Glib::ustring & marked_up_error );
	void toggle_fs_busy_state();
	void activate_mount_partition( unsigned int index ) ;
	void activate_disklabel() ;
	void activate_manage_flags() ;
	void activate_check() ;
	void activate_change_uuid() ;
	void activate_label_filesystem();
	void activate_name_partition();

	void activate_undo();

	enum OperationRemoveType
	{
		REMOVE_ALL,
		REMOVE_LAST,
		REMOVE_AT
	};
	void remove_operation(OperationRemoveType rmtype, int index = -1);
	int  partition_in_operation_queue_count( const Partition & partition ) ;
	int  active_partitions_on_device_count( const Device & device ) ;
	void activate_apply();
	bool remove_non_empty_lvm2_pv_dialog( const OperationType optype ) ;

	// private variables
	unsigned int        m_current_device;
	std::vector<Device> m_devices;
	Device              m_display_device;       // Copy of m_devices[m_current_device] with pending operations
	                                            // applied to partitions for displaying in the UI.
	const Partition * selected_partition_ptr;   // Pointer to the selected partition.  (Alias to element
	                                            // in Win_GParted::m_display_device.partitions[] vector).
	const Partition* copied_partition;          // nullptr or copy of source partition object.
	OperationVector     m_operations;

//gui stuff
	Gtk::Paned hpaned_main;
	Gtk::Paned vpaned_main;
	Gtk::Box vbox_main;
	Gtk::Box vbox_info;
	Gtk::Box hbox_toolbar;
	Gtk::Toolbar toolbar_main;
	Gtk::MenuBar menubar_main;
	Gtk::ComboBox combo_devices ;
	Gtk::Menu menu_partition, *menu ;
	Gtk::ToolButton *toolbutton;
	Gtk::Statusbar statusbar;
	Gtk::Image *image ;
	Gtk::ScrolledWindow *scrollwindow;
	Gtk::ProgressBar pulsebar ;
	Gtk::TreeRow treerow;
	
	DrawingAreaVisualDisk drawingarea_visualdisk ;
	TreeView_Detail treeview_detail;
	HBoxOperations hbox_operations ;

	//device combo
	Glib::RefPtr<Gtk::ListStore> liststore_devices ;
	sigc::connection combo_devices_changed_connection;

	struct TreeView_Devices_Columns : public Gtk::TreeModelColumnRecord
	{
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon ;
		Gtk::TreeModelColumn<Glib::ustring> device ;
		Gtk::TreeModelColumn<Glib::ustring> size ;

		TreeView_Devices_Columns()
		{
			add( icon ) ;
			add( device ) ;
			add( size ) ;
		}
	};
	TreeView_Devices_Columns m_treeview_devices_columns;

	// Indices for toolbar
	int
	TOOLBAR_NEW,
	TOOLBAR_DEL,
	TOOLBAR_RESIZE_MOVE,
	TOOLBAR_COPY,
	TOOLBAR_PASTE,
	TOOLBAR_UNDO,
	TOOLBAR_APPLY;

	enum MainMenu_Items
	{
		MENU_DEVICES = 0,
		MENU_EDIT,
		MENU_UNDO_OPERATION,
		MENU_CLEAR_OPERATIONS,
		MENU_APPLY_OPERATIONS,
		MENU_VIEW,
		MENU_DEVICE_INFORMATION,
		MENU_PENDING_OPERATIONS,
		MENU_DEVICE,
		MENU_PARTITION
	};

	enum PartitionMenu_Items
	{
		MENU_NEW = 0,
		MENU_DEL,
		MENU_RESIZE_MOVE,
		MENU_COPY,
		MENU_PASTE,
		MENU_FORMAT,
		MENU_TOGGLE_CRYPT_BUSY,
		MENU_TOGGLE_FS_BUSY,
		MENU_MOUNT,
		MENU_NAME_PARTITION,
		MENU_FLAGS,
		MENU_CHECK,
		MENU_LABEL_FILESYSTEM,
		MENU_CHANGE_UUID,
		MENU_INFO
	};
	std::map<int, Gtk::MenuItem*> mainmenu_items;
	std::map<int, Gtk::MenuItem*> partitionmenu_items;

	//usefull variables which are used by many different functions...
	unsigned short new_count;//new_count keeps track of the new created partitions
	bool m_operationslist_open;

	GParted_Core gparted_core ;
	std::vector<Gtk::Label *> device_info ;
					
	//stuff for progress overview and pulsebar
	bool pulsebar_pulse();
	sigc::connection pulsetimer;
};


}  // namespace GParted


#endif /* GPARTED_WIN_GPARTED_H */
