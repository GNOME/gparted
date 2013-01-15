/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013 Curtis Gedak
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
 
#include "../include/Win_GParted.h"
#include "../include/Dialog_Progress.h"
#include "../include/DialogFeatures.h" 
#include "../include/Dialog_Disklabel.h"
#include "../include/Dialog_Rescue_Data.h"
#include "../include/Dialog_Partition_Resize_Move.h"
#include "../include/Dialog_Partition_Copy.h"
#include "../include/Dialog_Partition_New.h"
#include "../include/Dialog_Partition_Info.h"
#include "../include/Dialog_Partition_Label.h"
#include "../include/DialogManageFlags.h"
#include "../include/OperationCopy.h"
#include "../include/OperationCheck.h"
#include "../include/OperationCreate.h"
#include "../include/OperationDelete.h"
#include "../include/OperationFormat.h"
#include "../include/OperationResizeMove.h"
#include "../include/OperationChangeUUID.h"
#include "../include/OperationLabelPartition.h"
#include "../include/LVM2_PV_Info.h"
#include "../config.h"

#include <gtkmm/aboutdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/main.h>
#include <gtkmm/separator.h>

namespace GParted
{
	
Win_GParted::Win_GParted( const std::vector<Glib::ustring> & user_devices )
{
	copied_partition .Reset() ;
	selected_partition .Reset() ;
	new_count = 1;
	current_device = 0 ;
	pulse = false ; 
	OPERATIONSLIST_OPEN = true ;
	gparted_core .set_user_devices( user_devices ) ;
	
	MENU_NEW = TOOLBAR_NEW =
        MENU_DEL = TOOLBAR_DEL =
        MENU_RESIZE_MOVE = TOOLBAR_RESIZE_MOVE =
        MENU_COPY = TOOLBAR_COPY =
        MENU_PASTE = TOOLBAR_PASTE =
        MENU_FORMAT =
        MENU_TOGGLE_BUSY =
        MENU_MOUNT =
        MENU_FLAGS =
        MENU_INFO =
        MENU_LABEL_PARTITION =
        MENU_CHANGE_UUID =
        TOOLBAR_UNDO =
        TOOLBAR_APPLY = -1 ;

	//==== GUI =========================
	this ->set_title( _("GParted") );
	this ->set_default_size( 775, 500 );
	
	try
	{
		this ->set_default_icon_name( "gparted" ) ;
	}
	catch ( Glib::Exception & e )
	{ 
		std::cout << e .what() << std::endl ;
	} 
	
	//Pack the main box
	this ->add( vbox_main ); 
	
	//menubar....
	init_menubar() ;
	vbox_main .pack_start( menubar_main, Gtk::PACK_SHRINK );
	
	//toolbar....
	init_toolbar() ;
	vbox_main.pack_start( hbox_toolbar, Gtk::PACK_SHRINK );
	
	//drawingarea_visualdisk...  ( contains the visual represenation of the disks )
	drawingarea_visualdisk .signal_partition_selected .connect( 
			sigc::mem_fun( this, &Win_GParted::on_partition_selected ) ) ;
	drawingarea_visualdisk .signal_partition_activated .connect( 
			sigc::mem_fun( this, &Win_GParted::on_partition_activated ) ) ;
	drawingarea_visualdisk .signal_popup_menu .connect( 
			sigc::mem_fun( this, &Win_GParted::on_partition_popup_menu ) );
	vbox_main .pack_start( drawingarea_visualdisk, Gtk::PACK_SHRINK ) ;
	
	//hpaned_main (NOTE: added to vpaned_main)
	init_hpaned_main() ;
	vpaned_main .pack1( hpaned_main, true, true ) ;
	
	//vpaned_main....
	vbox_main .pack_start( vpaned_main );
	
	//device info...
	init_device_info() ;
	
	//operationslist...
	hbox_operations .signal_undo .connect( sigc::mem_fun( this, &Win_GParted::activate_undo ) ) ;
	hbox_operations .signal_clear .connect( sigc::mem_fun( this, &Win_GParted::clear_operationslist ) ) ;
	hbox_operations .signal_apply .connect( sigc::mem_fun( this, &Win_GParted::activate_apply ) ) ;
	hbox_operations .signal_close .connect( sigc::mem_fun( this, &Win_GParted::close_operationslist ) ) ;
	vpaned_main .pack2( hbox_operations, true, true ) ;

	//statusbar... 
	pulsebar .set_pulse_step( 0.01 );
	statusbar .add( pulsebar );
	vbox_main .pack_start( statusbar, Gtk::PACK_SHRINK );
	
	this ->show_all_children();
	
	//make sure harddisk information is closed..
	hpaned_main .get_child1() ->hide() ;
}

void Win_GParted::init_menubar() 
{
	//fill menubar_main and connect callbacks 
	//gparted
	menu = manage( new Gtk::Menu() ) ;
	image = manage( new Gtk::Image( Gtk::Stock::REFRESH, Gtk::ICON_SIZE_MENU ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem(
		_("_Refresh Devices"),
		Gtk::AccelKey("<control>r"),
		*image, 
		sigc::mem_fun(*this, &Win_GParted::menu_gparted_refresh_devices) ) );
	
	image = manage( new Gtk::Image( Gtk::Stock::HARDDISK, Gtk::ICON_SIZE_MENU ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( _("_Devices"), *image ) ) ; 
	
	menu ->items() .push_back( Gtk::Menu_Helpers::SeparatorElem( ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::StockMenuElem( 
		Gtk::Stock::QUIT, sigc::mem_fun(*this, &Win_GParted::menu_gparted_quit) ) );
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("_GParted"), *menu ) );
	
	//edit
	menu = manage( new Gtk::Menu() ) ;
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( 
		_("_Undo Last Operation"), 
		Gtk::AccelKey("<control>z"),
		* manage( new Gtk::Image( Gtk::Stock::UNDO, Gtk::ICON_SIZE_MENU ) ), 
		sigc::mem_fun(*this, &Win_GParted::activate_undo) ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( 
		_("_Clear All Operations"), 
		* manage( new Gtk::Image( Gtk::Stock::CLEAR, Gtk::ICON_SIZE_MENU ) ), 
		sigc::mem_fun(*this, &Win_GParted::clear_operationslist) ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( 
		_("_Apply All Operations"), 
		* manage( new Gtk::Image( Gtk::Stock::APPLY, Gtk::ICON_SIZE_MENU ) ), 
		sigc::mem_fun(*this, &Win_GParted::activate_apply) ) );
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("_Edit"), *menu ) );

	//view
	menu = manage( new Gtk::Menu() ) ;
	menu ->items() .push_back( Gtk::Menu_Helpers::CheckMenuElem(
		_("Device _Information"), sigc::mem_fun(*this, &Win_GParted::menu_view_harddisk_info) ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::CheckMenuElem( 
		_("Pending _Operations"), sigc::mem_fun(*this, &Win_GParted::menu_view_operations) ) );
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("_View"), *menu ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::SeparatorElem( ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::MenuElem(
		_("_File System Support"), sigc::mem_fun( *this, &Win_GParted::menu_gparted_features ) ) );

	//device
	menu = manage( new Gtk::Menu() ) ;
	menu ->items() .push_back( Gtk::Menu_Helpers::MenuElem( Glib::ustring( _("_Create Partition Table") ) + "...",
								sigc::mem_fun(*this, &Win_GParted::activate_disklabel) ) );

	menu ->items() .push_back( Gtk::Menu_Helpers::MenuElem( Glib::ustring( _("_Attempt Data Rescue") ) + "...",
								sigc::mem_fun(*this, &Win_GParted::activate_attempt_rescue_data) ) );

	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("_Device"), *menu ) );

	//partition
	init_partition_menu() ;
	menubar_main .items() .push_back( Gtk::Menu_Helpers::MenuElem( _("_Partition"), menu_partition ) );

	//help
	menu = manage( new Gtk::Menu() ) ;
	menu ->items() .push_back( Gtk::Menu_Helpers::ImageMenuElem( 
		_("_Contents"), 
		Gtk::AccelKey("F1"),
		* manage( new Gtk::Image( Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU ) ), 
		sigc::mem_fun(*this, &Win_GParted::menu_help_contents) ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::SeparatorElem( ) );
	menu ->items() .push_back( Gtk::Menu_Helpers::StockMenuElem(
		Gtk::Stock::ABOUT, sigc::mem_fun(*this, &Win_GParted::menu_help_about) ) );

	menubar_main.items() .push_back( Gtk::Menu_Helpers::MenuElem(_("_Help"), *menu ) );
}

void Win_GParted::init_toolbar() 
{
	int index = 0 ;
	//initialize and pack toolbar_main 
	hbox_toolbar.pack_start( toolbar_main );
	
	//NEW and DELETE
	image = manage( new Gtk::Image( Gtk::Stock::NEW, Gtk::ICON_SIZE_BUTTON ) );
	/*TO TRANSLATORS: "New" is a tool bar item for partition actions. */
	Glib::ustring str_temp = _("New") ;
	toolbutton = Gtk::manage(new Gtk::ToolButton( *image, str_temp ));
	toolbutton ->signal_clicked() .connect( sigc::mem_fun( *this, &Win_GParted::activate_new ) );
	toolbar_main .append( *toolbutton );
	TOOLBAR_NEW = index++ ;
	toolbutton ->set_tooltip(tooltips, _("Create a new partition in the selected unallocated space") );		
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::DELETE));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_delete) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_DEL = index++ ;
	toolbutton ->set_tooltip(tooltips, _("Delete the selected partition") );		
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	index++ ;
	
	//RESIZE/MOVE
	image = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON ) );
	str_temp = _("Resize/Move") ;
	//Condition string split and Undo button.
	//  for longer translated string, split string in two and skip the Undo button to permit full toolbar to display
	//  FIXME:  Is there a better way to do this, perhaps without the conditional?  At the moment this seems to be the best compromise.
	bool display_undo = true ;
	if( str_temp .length() > 14 ) {
		size_t index = str_temp .find( "/" ) ;
		if ( index != Glib::ustring::npos ) {
			str_temp .replace( index, 1, "\n/" ) ;
			display_undo = false ;
		}
	}
	toolbutton = Gtk::manage(new Gtk::ToolButton( *image, str_temp ));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_resize) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_RESIZE_MOVE = index++ ;
	toolbutton ->set_tooltip(tooltips, _("Resize/Move the selected partition") );		
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	index++ ;

	//COPY and PASTE
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::COPY));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_copy) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_COPY = index++ ;
	toolbutton ->set_tooltip(tooltips, _("Copy the selected partition to the clipboard") );		
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::PASTE));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_paste) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_PASTE = index++ ;
	toolbutton ->set_tooltip(tooltips, _("Paste the partition from the clipboard") );		
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	index++ ;
	
	//UNDO and APPLY
	if ( display_undo ) {
		//Undo button is displayed only if translated language "Resize/Move" is not too long.  See above setting of this condition.
		toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::UNDO));
		toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_undo) );
		toolbar_main.append(*toolbutton);
		TOOLBAR_UNDO = index++ ;
		toolbutton ->set_sensitive( false );
		toolbutton ->set_tooltip(tooltips, _("Undo Last Operation") );
	}
	
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::APPLY));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_apply) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_APPLY = index++ ;
	toolbutton ->set_sensitive( false );
	toolbutton ->set_tooltip(tooltips, _("Apply All Operations") );		
	
	//initialize and pack combo_devices
	liststore_devices = Gtk::ListStore::create( treeview_devices_columns ) ;
	combo_devices .set_model( liststore_devices ) ;

	combo_devices .pack_start( treeview_devices_columns .icon, false ) ;
	combo_devices .pack_start( treeview_devices_columns .device ) ;
	combo_devices .pack_start( treeview_devices_columns .size, false ) ;
	
	combo_devices .signal_changed() .connect( sigc::mem_fun(*this, &Win_GParted::combo_devices_changed) );

	hbox_toolbar .pack_start( combo_devices, Gtk::PACK_SHRINK ) ;
}

void Win_GParted::init_partition_menu() 
{
	int index = 0 ;

	//fill menu_partition
	image = manage( new Gtk::Image( Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU ) );
	menu_partition .items() .push_back( 
			/*TO TRANSLATORS: "_New" is a sub menu item for the partition menu. */
			Gtk::Menu_Helpers::ImageMenuElem( _("_New"), 
							  *image, 
							  sigc::mem_fun(*this, &Win_GParted::activate_new) ) );
	MENU_NEW = index++ ;
	
	menu_partition .items() .push_back( 
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::DELETE, 
							  Gtk::AccelKey( GDK_Delete, Gdk::BUTTON1_MASK ),
							  sigc::mem_fun(*this, &Win_GParted::activate_delete) ) );
	MENU_DEL = index++ ;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++ ;
	
	image = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_MENU ) );
	menu_partition .items() .push_back( 
			Gtk::Menu_Helpers::ImageMenuElem( _("_Resize/Move"), 
							  *image, 
							  sigc::mem_fun(*this, &Win_GParted::activate_resize) ) );
	MENU_RESIZE_MOVE = index++ ;
	
	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++ ;
	
	menu_partition .items() .push_back( 
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::COPY,
							  sigc::mem_fun(*this, &Win_GParted::activate_copy) ) );
	MENU_COPY = index++ ;
	
	menu_partition .items() .push_back( 
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::PASTE,
							  sigc::mem_fun(*this, &Win_GParted::activate_paste) ) );
	MENU_PASTE = index++ ;
	
	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() );
	index++ ;
	
	image = manage( new Gtk::Image( Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU ) );
	/*TO TRANSLATORS: menuitem which holds a submenu with file systems.. */
	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::ImageMenuElem( _("_Format to"),
							  *image,
							  * create_format_menu() ) ) ;
	MENU_FORMAT = index++ ;
	
	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() ) ;
	index++ ;
	
	menu_partition .items() .push_back(
			//This is a placeholder text. It will be replaced with some other text before it is used
			Gtk::Menu_Helpers::MenuElem( "--placeholder--",
						     sigc::mem_fun( *this, &Win_GParted::toggle_busy_state ) ) );
	MENU_TOGGLE_BUSY = index++ ;

	/*TO TRANSLATORS: menuitem which holds a submenu with mount points.. */
	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( _("_Mount on"), * manage( new Gtk::Menu() ) ) ) ;
	MENU_MOUNT = index++ ;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() ) ;
	index++ ;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( _("M_anage Flags"),
						     sigc::mem_fun( *this, &Win_GParted::activate_manage_flags ) ) );
	MENU_FLAGS = index++ ;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( _("C_heck"),
						     sigc::mem_fun( *this, &Win_GParted::activate_check ) ) );
	MENU_CHECK = index++ ;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( _("_Label"),
						     sigc::mem_fun( *this, &Win_GParted::activate_label_partition ) ) );
	MENU_LABEL_PARTITION = index++ ;

	menu_partition .items() .push_back(
			Gtk::Menu_Helpers::MenuElem( _("New UU_ID"),
						     sigc::mem_fun( *this, &Win_GParted::activate_change_uuid ) ) );
	MENU_CHANGE_UUID = index++ ;

	menu_partition .items() .push_back( Gtk::Menu_Helpers::SeparatorElem() ) ;
	index++ ;
	
	menu_partition .items() .push_back( 
			Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::DIALOG_INFO,
							  sigc::mem_fun(*this, &Win_GParted::activate_info) ) );
	MENU_INFO = index++ ;
	
	menu_partition .accelerate( *this ) ;  
}

Gtk::Menu * Win_GParted::create_format_menu()
{
	menu = manage( new Gtk::Menu() ) ;

	for ( unsigned int t =0; t < gparted_core .get_filesystems() .size() ; t++ )
	{
		//Skip luks and unknown because these are not file systems
		if (
		     gparted_core .get_filesystems()[ t ] .filesystem == GParted::FS_LUKS    ||
		     gparted_core .get_filesystems()[ t ] .filesystem == GParted::FS_UNKNOWN
		   )
			continue ;

		hbox = manage( new Gtk::HBox() );
			
		//the colored square
		hbox ->pack_start( * manage( new Gtk::Image(
					Utils::get_color_as_pixbuf( 
						gparted_core .get_filesystems()[ t ] .filesystem, 16, 16 ) ) ),
				   Gtk::PACK_SHRINK ) ;			
		
		//the label...
		hbox ->pack_start( * Utils::mk_label(
					" " + 
					Utils::get_filesystem_string( gparted_core .get_filesystems()[ t ] .filesystem ) ),
				   Gtk::PACK_SHRINK );	
				
		menu ->items() .push_back( * manage( new Gtk::MenuItem( *hbox ) ) );
		if ( gparted_core .get_filesystems()[ t ] .create )
			menu ->items() .back() .signal_activate() .connect( 
				sigc::bind<GParted::FILESYSTEM>(sigc::mem_fun(*this, &Win_GParted::activate_format),
				gparted_core .get_filesystems()[ t ] .filesystem ) ) ;
		else
			menu ->items() .back() .set_sensitive( false ) ;
	}
	
	return menu ;
}
	
void Win_GParted::init_device_info()
{
	vbox_info.set_spacing( 5 );
	int top = 0, bottom = 1;
	
	//title
	vbox_info .pack_start( 
		* Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Device Information") ) + "</b>" ),
		Gtk::PACK_SHRINK );
	
	//GENERAL DEVICE INFO
	table = manage( new Gtk::Table() ) ;
	table ->set_col_spacings( 10 ) ;
	
	//model
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Model:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	//size
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Size:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	//path
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Path:") ) + "</b>",
					   true,
					   Gtk::ALIGN_LEFT,
					   Gtk::ALIGN_TOP ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	vbox_info .pack_start( *table, Gtk::PACK_SHRINK );
	
	//DETAILED DEVICE INFO 
	top = 0 ; bottom = 1;
	table = manage( new Gtk::Table() ) ;
	table ->set_col_spacings( 10 ) ;
	
	//one blank line
	table ->attach( * Utils::mk_label( "" ), 1, 2, top++, bottom++, Gtk::FILL );
	
	//disktype
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Partition table:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	//heads
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Heads:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	//sectors/track
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Sectors/track:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL );
	
	//cylinders
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Cylinders:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL ) ;
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;
	
	//total sectors
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Total sectors:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;

	//sector size
	table ->attach( * Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Sector size:") ) + "</b>" ),
			0, 1,
			top, bottom,
			Gtk::FILL );
	device_info .push_back( Utils::mk_label( "", true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ) ) ;
	table ->attach( * device_info .back(), 1, 2, top++, bottom++, Gtk::FILL ) ;

	vbox_info .pack_start( *table, Gtk::PACK_SHRINK );
}

void Win_GParted::init_hpaned_main() 
{
	//left scrollwindow (holds device info)
	scrollwindow = manage( new Gtk::ScrolledWindow() ) ;
	scrollwindow ->set_shadow_type( Gtk::SHADOW_ETCHED_IN );
	scrollwindow ->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );

	hpaned_main .pack1( *scrollwindow, true, true );
	scrollwindow ->add( vbox_info );

	//right scrollwindow (holds treeview with partitions)
	scrollwindow = manage( new Gtk::ScrolledWindow() ) ;
	scrollwindow ->set_shadow_type( Gtk::SHADOW_ETCHED_IN );
	scrollwindow ->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
	
	//connect signals and add treeview_detail
	treeview_detail .signal_partition_selected .connect( sigc::mem_fun( this, &Win_GParted::on_partition_selected ) );
	treeview_detail .signal_partition_activated .connect( sigc::mem_fun( this, &Win_GParted::on_partition_activated ) );
	treeview_detail .signal_popup_menu .connect( sigc::mem_fun( this, &Win_GParted::on_partition_popup_menu ) );
	scrollwindow ->add( treeview_detail );
	hpaned_main .pack2( *scrollwindow, true, true );
}

void Win_GParted::refresh_combo_devices()
{
	liststore_devices ->clear() ;
	
	menu = manage( new Gtk::Menu() ) ;
	Gtk::RadioButtonGroup radio_group ;
	
	for ( unsigned int i = 0 ; i < devices .size( ) ; i++ )
	{
		//combo...
		treerow = *( liststore_devices ->append() ) ;
		treerow[ treeview_devices_columns .icon ] =
			render_icon( Gtk::Stock::HARDDISK, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ;
		treerow[ treeview_devices_columns .device ] = devices[ i ] .get_path() ;
		treerow[ treeview_devices_columns .size ] = "(" + Utils::format_size( devices[ i ] .length, devices[ i ] .sector_size ) + ")" ; 
	
		//devices submenu....
		hbox = manage( new Gtk::HBox() ) ;
		hbox ->pack_start( * Utils::mk_label( devices[ i ] .get_path() ), Gtk::PACK_SHRINK ) ;
		hbox ->pack_start( * Utils::mk_label( "   (" + Utils::format_size( devices[ i ] .length, devices[ i ] .sector_size ) + ")",
					              true,
						      Gtk::ALIGN_RIGHT ),
				   Gtk::PACK_EXPAND_WIDGET ) ;
		
		menu ->items() .push_back( * manage( new Gtk::RadioMenuItem( radio_group ) ) ) ;
		menu ->items() .back() .add( *hbox ) ;
		menu ->items() .back() .signal_activate() .connect( 
			sigc::bind<unsigned int>( sigc::mem_fun(*this, &Win_GParted::radio_devices_changed), i ) ) ;
	}
				
	menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .remove_submenu() ;

	if ( menu ->items() .size() )
	{
		menu ->show_all() ;
		menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .set_submenu( *menu ) ;
	}
	
	combo_devices .set_active( current_device ) ;
}

void Win_GParted::show_pulsebar( const Glib::ustring & status_message ) 
{
	pulsebar .show();
	statusbar .push( status_message) ;
	
	//disable all input stuff
	toolbar_main .set_sensitive( false ) ;
	menubar_main .set_sensitive( false ) ;
	combo_devices .set_sensitive( false ) ;
	menu_partition .set_sensitive( false ) ;
	treeview_detail .set_sensitive( false ) ;
	drawingarea_visualdisk .set_sensitive( false ) ;
		
	//the actual 'pulsing'
	while ( pulse )
	{
		pulsebar .pulse();
		while ( Gtk::Main::events_pending() )
			Gtk::Main::iteration();
		usleep( 100000 );
		Glib::ustring tmp_msg = gparted_core .get_thread_status_message() ;
		if ( tmp_msg != "" )
			statusbar .push( tmp_msg ) ;
	}
	
	thread ->join() ;
	
	pulsebar .hide();
	statusbar .pop() ;
		
	//enable all disabled stuff
	toolbar_main .set_sensitive( true ) ;
	menubar_main .set_sensitive( true ) ;
	combo_devices .set_sensitive( true ) ;
	menu_partition .set_sensitive( true ) ;
	treeview_detail .set_sensitive( true ) ;
	drawingarea_visualdisk .set_sensitive( true ) ;
}

void Win_GParted::Fill_Label_Device_Info( bool clear ) 
{
	if ( clear )
		for ( unsigned int t = 0 ; t < device_info .size( ) ; t++ )
			device_info[ t ] ->set_text( "" ) ;
		
	else
	{		
		short t = 0;
		
		//global info...
		device_info[ t++ ] ->set_text( devices[ current_device ] .model ) ;
		device_info[ t++ ] ->set_text( Utils::format_size( devices[ current_device ] .length, devices[ current_device ] .sector_size ) ) ;
		device_info[ t++ ] ->set_text( Glib::build_path( "\n", devices[ current_device ] .get_paths() ) ) ;
		
		//detailed info
		device_info[ t++ ] ->set_text( devices[ current_device ] .disktype ) ;
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .heads ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .sectors ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .cylinders ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .length ) );
		device_info[ t++ ] ->set_text( Utils::num_to_str( devices[ current_device ] .sector_size ) );
	}
}

bool Win_GParted::on_delete_event( GdkEventAny *event )
{
	return ! Quit_Check_Operations();
}	

void Win_GParted::Add_Operation( Operation * operation, int index )
{
	if ( operation )
	{ 
		Glib::ustring error ;
		//Add any of the listed operations without further checking, but
		//  for the other operations (_CREATE, _RESIZE_MOVE and _COPY)
		//  ensure the partition is correctly aligned.
		//FIXME: this is becoming a mess.. maybe it's better to check if partition_new > 0
		if ( operation ->type == OPERATION_DELETE ||
		     operation ->type == OPERATION_FORMAT ||
		     operation ->type == OPERATION_CHECK ||
		     operation ->type == OPERATION_CHANGE_UUID ||
		     operation ->type == OPERATION_LABEL_PARTITION ||
		     gparted_core .snap_to_alignment( operation ->device, operation ->partition_new, error )
		   )
		{
			operation ->create_description() ;

			if ( index >= 0 && index < static_cast<int>( operations .size() ) )
				operations .insert( operations .begin() + index, operation ) ;
			else
				operations .push_back( operation );

			allow_undo_clear_apply( true ) ;
			Refresh_Visual();
			
			if ( operations .size() == 1 ) //first operation, open operationslist
				open_operationslist() ;

			//FIXME:  A slight flicker may be introduced by this extra display refresh.
			//An extra display refresh seems to prevent the disk area visual disk from
			//  disappearing when enough operations are added to require a scrollbar
			//  (about 4 operations with default window size).
			//  Note that commenting out the code to
			//  "//make scrollwindow focus on the last operation in the list"
			//  in HBoxOperations::load_operations() prevents this problem from occurring as well.
			//  See also Win_GParted::activate_undo().
			drawingarea_visualdisk .queue_draw() ;
		}
		else
		{
			Gtk::MessageDialog dialog( *this,
				   _("Could not add this operation to the list"),
				   false,
				   Gtk::MESSAGE_ERROR,
				   Gtk::BUTTONS_OK,
				   true );
			dialog .set_secondary_text( error ) ;

			dialog .run() ;
		}
	}
}

bool Win_GParted::Merge_Operations( unsigned int first, unsigned int second )
{
	if( first >= operations .size() || second >= operations .size() )
		return false;

	// Two resize operations of the same partition
	if ( operations[ first ]->type == OPERATION_RESIZE_MOVE &&
	     operations[ second ]->type == OPERATION_RESIZE_MOVE &&
	     operations[ first ]->partition_new == operations[ second ]->partition_original
	   )
	{
		operations[ first ]->partition_new = operations[ second ]->partition_new;
		operations[ first ]->create_description() ;
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two label change operations on the same partition
	else if ( operations[ first ]->type == OPERATION_LABEL_PARTITION &&
	          operations[ second ]->type == OPERATION_LABEL_PARTITION &&
	          operations[ first ]->partition_new == operations[ second ]->partition_original
	        )
	{
		operations[ first ]->partition_new.set_label( operations[ second ]->partition_new .get_label() ) ;
		operations[ first ]->create_description() ;
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two change-uuid change operations on the same partition
	else if ( operations[ first ]->type == OPERATION_CHANGE_UUID &&
	          operations[ second ]->type == OPERATION_CHANGE_UUID &&
	          operations[ first ]->partition_new == operations[ second ]->partition_original
	        )
	{
		// Changing half the UUID should not override changing all of it
		if ( operations[ first ]->partition_new.uuid == UUID_RANDOM_NTFS_HALF ||
		     operations[ second ]->partition_new.uuid == UUID_RANDOM )
			operations[ first ]->partition_new.uuid = operations[ second ]->partition_new.uuid;
		operations[ first ]->create_description() ;
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two check operations of the same partition
	else if ( operations[ first ]->type == OPERATION_CHECK &&
	          operations[ second ]->type == OPERATION_CHECK &&
	          operations[ first ]->partition_original == operations[ second ]->partition_original
	        )
	{
		remove_operation( second );

		Refresh_Visual();

		return true;
	}
	// Two format operations of the same partition
	else if ( operations[ first ]->type == OPERATION_FORMAT &&
	          operations[ second ]->type == OPERATION_FORMAT &&
	          operations[ first ]->partition_new == operations[ second ]->partition_original
	        )
	{
		operations[ first ]->partition_new = operations[ second ]->partition_new;
		operations[ first ]->create_description() ;
		remove_operation( second );

		Refresh_Visual();

		return true;
	}

	return false;
}

void Win_GParted::Refresh_Visual()
{
	std::vector<Partition> partitions = devices[ current_device ] .partitions ; 
	
	//make all operations visible
	for ( unsigned int t = 0 ; t < operations .size(); t++ )
		if ( operations[ t ] ->device == devices[ current_device ] )
			operations[ t ] ->apply_to_visual( partitions ) ;
			
	hbox_operations .load_operations( operations ) ;

	//set new statusbartext
	statusbar .pop() ;
	statusbar .push( String::ucompose( ngettext( "%1 operation pending"
	                                           , "%1 operations pending"
	                                           , operations .size()
	                                           )
	                                 , operations .size()
	                                 )
	               );
		
	if ( ! operations .size() ) 
		allow_undo_clear_apply( false ) ;
			
	//count primary's and check for extended
	index_extended = -1 ;
	primary_count = 0;
	for ( unsigned int t = 0 ; t < partitions .size() ; t++ )
	{
		if ( partitions[ t ] .get_path() == copied_partition .get_path() )
			copied_partition = partitions[ t ] ;
		
		switch ( partitions[ t ] .type )
		{
			case GParted::TYPE_PRIMARY	:
				primary_count++;
				break;
				
			case GParted::TYPE_EXTENDED	:
				index_extended = t ;
				primary_count++;
				break;
				
			default				:
				break;
		}
	}
	
	//frame visualdisk
	drawingarea_visualdisk .load_partitions( partitions, devices[ current_device ] .length ) ;

	//treeview details
	treeview_detail .load_partitions( partitions ) ;
	
	//no partition can be selected after a refresh..
	selected_partition .Reset() ;
	set_valid_operations() ; 
			
	while ( Gtk::Main::events_pending() ) 
		Gtk::Main::iteration() ;
}

bool Win_GParted::Quit_Check_Operations()
{
	if ( operations .size() )
	{
		Gtk::MessageDialog dialog( *this,
					   _("Quit GParted?"),
					   false,
					   Gtk::MESSAGE_QUESTION,
					   Gtk::BUTTONS_NONE,
					   true );

		dialog .set_secondary_text( String::ucompose( ngettext( "%1 operation is currently pending."
		                                                      , "%1 operations are currently pending."
		                                                      , operations .size()
		                                                      )
		                                            , operations .size()
		                                            )
		                          ) ;
	
		dialog .add_button( Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE );
		dialog .add_button( Gtk::Stock::CANCEL,Gtk::RESPONSE_CANCEL );
		
		if ( dialog .run() == Gtk::RESPONSE_CANCEL )
			return false;//don't close GParted
	}

	return true; //close GParted
}

void Win_GParted::set_valid_operations()
{
	allow_new( false ); allow_delete( false ); allow_resize( false ); allow_copy( false );
	allow_paste( false ); allow_format( false ); allow_toggle_busy_state( false ) ;
	allow_manage_flags( false ) ; allow_check( false ) ; allow_label_partition( false ) ;
	allow_change_uuid( false ); allow_info( false ) ;

	dynamic_cast<Gtk::Label*>( menu_partition .items()[ MENU_TOGGLE_BUSY ] .get_child() )
		->set_label( FileSystem::get_generic_text ( CTEXT_DEACTIVATE_FILESYSTEM ) ) ;

	menu_partition .items()[ MENU_TOGGLE_BUSY ] .show() ;
	menu_partition .items()[ MENU_MOUNT ] .hide() ;	

	//no partition selected...	
	if ( ! selected_partition .get_paths() .size() )
		return ;
	
	//if there's something, there's some info ;)
	allow_info( true ) ;
	
	//flag managing..
	if ( selected_partition .type != GParted::TYPE_UNALLOCATED && selected_partition .status == GParted::STAT_REAL )
		allow_manage_flags( true ) ; 

	//Activate / deactivate
	if ( gparted_core .get_filesystem_object ( selected_partition .filesystem ) )
		dynamic_cast<Gtk::Label*>( menu_partition .items()[ MENU_TOGGLE_BUSY ] .get_child() )
			->set_label( gparted_core .get_filesystem_object ( selected_partition .filesystem )
			             ->get_custom_text (  selected_partition .busy
			                                ? CTEXT_DEACTIVATE_FILESYSTEM
			                                : CTEXT_ACTIVATE_FILESYSTEM
			                               )
			           ) ;
	else
		dynamic_cast<Gtk::Label*>( menu_partition .items()[ MENU_TOGGLE_BUSY ] .get_child() )
			->set_label( FileSystem::get_generic_text (  selected_partition .busy
			                                           ? CTEXT_DEACTIVATE_FILESYSTEM
			                                           : CTEXT_ACTIVATE_FILESYSTEM )
			                                          ) ;

	//Only permit mount/unmount, swapon/swapoff, ... if action is available
	if (    selected_partition .status == GParted::STAT_REAL
	     && selected_partition .type != GParted::TYPE_EXTENDED
	     && selected_partition .filesystem != GParted::FS_LVM2_PV
	     && (    selected_partition .busy
	          || selected_partition .get_mountpoints() .size() /* Have mount point(s) */
	          || selected_partition .filesystem == GParted::FS_LINUX_SWAP
	        )
	   )
		allow_toggle_busy_state( true ) ;

	//Only permit VG deactivation if busy, or activation if not busy and a member of a VG.
	//  For now specifically allow activation of an exported VG, which LVM will fail
	//  with "Volume group "VGNAME" is exported", otherwise user won't know why the
	//  inactive PV can't be activated.
	if (    selected_partition .status == GParted::STAT_REAL
	     && selected_partition .type != GParted::TYPE_EXTENDED
	     && selected_partition .filesystem == GParted::FS_LVM2_PV
	     && (    selected_partition .busy
	          || (    ! selected_partition .busy
                       && ! selected_partition .get_mountpoints() .empty()  //VGNAME from mount point
	             )
	        )
	   )
		allow_toggle_busy_state( true ) ;

	//only unmount/swapoff/VG deactivate/... is allowed if busy
	if ( selected_partition .busy )
		return ;

	//UNALLOCATED
	if ( selected_partition .type == GParted::TYPE_UNALLOCATED )
	{
		allow_new( true );
		
		//find out if there is a copied partition and if it fits inside this unallocated space
		if ( ! copied_partition .get_path() .empty() && ! devices[ current_device ] .readonly )
		{
			Byte_Value required_size ;
			if ( copied_partition .filesystem == GParted::FS_XFS )
				required_size = copied_partition .estimated_min_size() * copied_partition .sector_size;
			else
				required_size = copied_partition .get_byte_length() ;

			//Determine if space is needed for the Master Boot Record or
			//  the Extended Boot Record.  Generally an an additional track or MEBIBYTE
			//  is required so for our purposes reserve a MEBIBYTE in front of the partition.
			//  NOTE:  This logic also contained in Dialog_Base_Partition::MB_Needed_for_Boot_Record
			if (   (   selected_partition .inside_extended
			        && selected_partition .type == TYPE_UNALLOCATED
			       )
			    || ( selected_partition .type == TYPE_LOGICAL )
			                                     /* Beginning of disk device */
			    || ( selected_partition .sector_start <= (MEBIBYTE / selected_partition .sector_size) )
			   )
				required_size += MEBIBYTE;

			//Determine if space is needed for the Extended Boot Record for a logical partition
			//  after this partition.  Generally an an additional track or MEBIBYTE
			//  is required so for our purposes reserve a MEBIBYTE in front of the partition.
			if (   (   (   selected_partition .inside_extended
			            && selected_partition .type == TYPE_UNALLOCATED
			           )
			        || ( selected_partition .type == TYPE_LOGICAL )
			       )
			    && ( selected_partition .sector_end
			         < ( devices[ current_device ] .length
			             - ( 2 * MEBIBYTE / devices[ current_device ] .sector_size )
			           )
			       )
			   )
				required_size += MEBIBYTE;

			//Determine if space is needed for the backup partition on a GPT partition table
			if (   ( devices[ current_device ] .disktype == "gpt" )
			    && ( ( devices[ current_device ] .length - selected_partition .sector_end )
			         < ( MEBIBYTE / devices[ current_device ] .sector_size )
			       )
			   )
				required_size += MEBIBYTE ;

			if ( required_size <= selected_partition .get_byte_length() )
				allow_paste( true ) ;
		}
		
		return ;
	}
	
	//EXTENDED
	if ( selected_partition .type == GParted::TYPE_EXTENDED )
	{
		//deletion is only allowed when there are no logical partitions inside.
		if ( selected_partition .logicals .size() == 1 &&
		     selected_partition .logicals .back() .type == GParted::TYPE_UNALLOCATED ) 
			allow_delete( true ) ;
		
		if ( ! devices[ current_device ] .readonly )
			allow_resize( true ) ; 

		return ;
	}	
	
	//PRIMARY and LOGICAL
	if (  selected_partition .type == GParted::TYPE_PRIMARY || selected_partition .type == GParted::TYPE_LOGICAL )
	{
		fs = gparted_core .get_fs( selected_partition .filesystem ) ;
		
		allow_delete( true ) ;
		allow_format( true ) ;
		
		//find out if resizing/moving is possible
		if ( (fs .grow || fs .shrink || fs .move ) && ! devices[ current_device ] .readonly ) 
			allow_resize( true ) ;
			
		//only allow copying of real partitions
		if ( selected_partition .status == GParted::STAT_REAL && fs .copy )
			allow_copy( true ) ;
		
		//only allow labelling of real partitions that support labelling
		if ( selected_partition .status == GParted::STAT_REAL && fs .write_label )
			allow_label_partition( true ) ;

		//only allow changing UUID of real partitions that support it
		if ( selected_partition .status == GParted::STAT_REAL && fs .write_uuid )
			allow_change_uuid( true ) ;

		//Generate Mount on submenu, except for LVM2 PVs
		//  borrowing mount point to display the VGNAME
		if (   selected_partition .filesystem != GParted::FS_LVM2_PV
		    && selected_partition .get_mountpoints() .size()
		   )
		{
			menu = menu_partition .items()[ MENU_MOUNT ] .get_submenu() ;
			menu ->items() .clear() ;
			for ( unsigned int t = 0 ; t < selected_partition .get_mountpoints() .size() ; t++ )
			{
				menu ->items() .push_back( 
					Gtk::Menu_Helpers::MenuElem( 
						selected_partition .get_mountpoints()[ t ], 
						sigc::bind<unsigned int>( sigc::mem_fun(*this, &Win_GParted::activate_mount_partition), t ) ) );

				dynamic_cast<Gtk::Label*>( menu ->items() .back() .get_child() ) ->set_use_underline( false ) ;
			}

			menu_partition .items()[ MENU_TOGGLE_BUSY ] .hide() ;
			menu_partition .items()[ MENU_MOUNT ] .show() ;	
		}

		//see if there is an copied partition and if it passes all tests
		if ( ! copied_partition .get_path() .empty() &&
		     copied_partition .get_byte_length() <= selected_partition .get_byte_length() &&
		     selected_partition .status == GParted::STAT_REAL &&
		     copied_partition != selected_partition )
		     allow_paste( true ) ;

		//see if we can somehow check/repair this file system....
		if ( fs .check && selected_partition .status == GParted::STAT_REAL )
			allow_check( true ) ;
	}
}

void Win_GParted::open_operationslist() 
{
	if ( ! OPERATIONSLIST_OPEN )
	{
		OPERATIONSLIST_OPEN = true ;
		hbox_operations .show() ;
	
		for ( int t = vpaned_main .get_height() ; t > ( vpaned_main .get_height() - 100 ) ; t -= 5 )
		{
			vpaned_main .set_position( t );
			while ( Gtk::Main::events_pending() ) 
				Gtk::Main::iteration() ;
		}

		static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 2 ] .get_submenu() ->items()[ 1 ] )
			->set_active( true ) ;
	}
}

void Win_GParted::close_operationslist() 
{
	if ( OPERATIONSLIST_OPEN )
	{
		OPERATIONSLIST_OPEN = false ;
		
		for ( int t = vpaned_main .get_position() ; t < vpaned_main .get_height() ; t += 5 )
		{
			vpaned_main .set_position( t ) ;
		
			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();
		}
		
		hbox_operations .hide() ;

		static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 2 ] .get_submenu() ->items()[ 1 ] )
			->set_active( false ) ;
	}
}

void Win_GParted::clear_operationslist() 
{
	remove_operation( -1, true ) ;
	close_operationslist() ;

	Refresh_Visual() ;
}

void Win_GParted::combo_devices_changed()
{
	unsigned int old_current_device = current_device;
	//set new current device
	current_device = combo_devices .get_active_row_number() ;
	if ( current_device == (unsigned int) -1 )
		current_device = old_current_device;
	if ( current_device >= devices .size() )
		current_device = 0 ;
	set_title( String::ucompose( _("%1 - GParted"), devices[ current_device ] .get_path() ) );
	
	//refresh label_device_info
	Fill_Label_Device_Info();
	
	//rebuild visualdisk and treeview
	Refresh_Visual();
	
	//uodate radiobuttons..
	if ( menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .get_submenu() )
		static_cast<Gtk::RadioMenuItem *>( 
			& menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .get_submenu() ->
			items()[ current_device ] ) ->set_active( true ) ;
}

void Win_GParted::radio_devices_changed( unsigned int item ) 
{
	if ( static_cast<Gtk::RadioMenuItem *>( 
	     	& menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .get_submenu() ->
		items()[ item ] ) ->get_active() )
	{
		combo_devices .set_active( item ) ;
	}
}

void Win_GParted::on_show()
{
	Gtk::Window::on_show() ;
	
	vpaned_main .set_position( vpaned_main .get_height() ) ;
	close_operationslist() ;

	menu_gparted_refresh_devices() ;
}
	
void Win_GParted::thread_refresh_devices() 
{
	gparted_core .set_devices( devices ) ;
	pulse = false ;
}

void Win_GParted::menu_gparted_refresh_devices()
{
	pulse = true ;	
	thread = Glib::Thread::create( sigc::mem_fun( *this, &Win_GParted::thread_refresh_devices ), true ) ;

	show_pulsebar( _("Scanning all devices...") ) ;
	
	//check if current_device is still available (think about hotpluggable stuff like usbdevices)
	if ( current_device >= devices .size() )
		current_device = 0 ;

	//see if there are any pending operations on non-existent devices
	//NOTE that this isn't 100% foolproof since some stuff (e.g. sourcedevice of copy) may slip through.
	//but anyone who removes the sourcedevice before applying the operations gets what he/she deserves :-)
	//FIXME: this actually sucks ;) see if we can use STL predicates here..
	unsigned int i ;
	for ( unsigned int t = 0 ; t < operations .size() ; t++ )
	{
		for ( i = 0 ; i < devices .size() && devices[ i ] != operations[ t ] ->device ; i++ ) {}
		
		if ( i >= devices .size() )
			remove_operation( t-- ) ;
	}
		
	//if no devices were detected we disable some stuff and show a message in the statusbar
	if ( devices .empty() )
	{
		this ->set_title( _("GParted") );
		combo_devices .hide() ;
		
		menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .set_sensitive( false ) ;
		menubar_main .items()[ 1 ] .set_sensitive( false ) ;
		menubar_main .items()[ 2 ] .set_sensitive( false ) ;
		menubar_main .items()[ 3 ] .set_sensitive( false ) ;
		menubar_main .items()[ 4 ] .set_sensitive( false ) ;
		toolbar_main .set_sensitive( false ) ;
		drawingarea_visualdisk .set_sensitive( false ) ;
		treeview_detail .set_sensitive( false ) ;

		Fill_Label_Device_Info( true ) ;
		
		drawingarea_visualdisk .clear() ;
		treeview_detail .clear() ;
		
		//hmzz, this is really paranoid, but i think it's the right thing to do ;)
		hbox_operations .clear() ;
		close_operationslist() ;
		remove_operation( -1, true ) ;
		
		statusbar .pop() ;
		statusbar .push( _( "No devices detected" ) );
	}
	else //at least one device detected
	{
		combo_devices .show() ;
		
		menubar_main .items()[ 0 ] .get_submenu() ->items()[ 1 ] .set_sensitive( true ) ;
		menubar_main .items()[ 1 ] .set_sensitive( true ) ;
		menubar_main .items()[ 2 ] .set_sensitive( true ) ;
		menubar_main .items()[ 3 ] .set_sensitive( true ) ;
		menubar_main .items()[ 4 ] .set_sensitive( true ) ;

		toolbar_main .set_sensitive( true ) ;
		drawingarea_visualdisk .set_sensitive( true ) ;
		treeview_detail .set_sensitive( true ) ;
		
		refresh_combo_devices() ;	
	}
}

void Win_GParted::menu_gparted_features()
{
	DialogFeatures dialog ;
	dialog .set_transient_for( *this ) ;
	
	dialog .load_filesystems( gparted_core .get_filesystems() ) ;
	while ( dialog .run() == Gtk::RESPONSE_OK )
	{
		gparted_core .find_supported_filesystems() ;
		dialog .load_filesystems( gparted_core .get_filesystems() ) ;

		//recreate format menu...
		menu_partition .items()[ MENU_FORMAT ] .remove_submenu() ;
		menu_partition .items()[ MENU_FORMAT ] .set_submenu( * create_format_menu() ) ;
		menu_partition .items()[ MENU_FORMAT ] .get_submenu() ->show_all_children() ;
	}
}

void Win_GParted::menu_gparted_quit()
{
	if ( Quit_Check_Operations() )
		this ->hide();
}

void Win_GParted::menu_view_harddisk_info()
{ 
	if ( static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 2 ] .get_submenu() ->items()[ 0 ] ) ->get_active() )
	{	//open harddisk information
		hpaned_main .get_child1() ->show() ;		
		for ( int t = hpaned_main .get_position() ; t < 250 ; t += 15 )
		{
			hpaned_main .set_position( t );
			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();
		}
	}
	else 
	{ 	//close harddisk information
		for ( int t = hpaned_main .get_position() ;  t > 0 ; t -= 15 )
		{
			hpaned_main .set_position( t );
			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();
		}
		hpaned_main .get_child1() ->hide() ;
	}
}

void Win_GParted::menu_view_operations()
{
	if ( static_cast<Gtk::CheckMenuItem *>( & menubar_main .items()[ 2 ] .get_submenu() ->items()[ 1 ] ) ->get_active() )
		open_operationslist() ;
	else 
		close_operationslist() ;
}

void Win_GParted::show_disklabel_unrecognized ( Glib::ustring device_name )
{
	//Display dialog box indicating that no partition table was found on the device
	Gtk::MessageDialog dialog( *this,
			/*TO TRANSLATORS: looks like   No partition table found on device /dev/sda */
			String::ucompose( _( "No partition table found on device %1" ), device_name ),
			false,
			Gtk::MESSAGE_INFO,
			Gtk::BUTTONS_OK,
			true ) ;
	Glib::ustring tmp_msg = _( "A partition table is required before partitions can be added." ) ;
	tmp_msg += "\n" ;
	tmp_msg += _( "To create a new partition table choose the menu item:" ) ;
	tmp_msg += "\n" ;
	/*TO TRANSLATORS: this message represents the menu item Create Partition Table under the Device menu. */
	tmp_msg += _( "Device --> Create Partition Table." ) ;
	dialog .set_secondary_text( tmp_msg ) ;
	dialog .run() ;
}

void Win_GParted::show_help_dialog( const Glib::ustring & filename /* E.g., gparted */
                                  , const Glib::ustring & link_id  /* For context sensitive help */
                                  )
{
	GError *error = NULL ;
	GdkScreen *gscreen = NULL ;

	Glib::ustring uri = "ghelp:" + filename ;
	if (link_id .size() > 0 ) {
		uri = uri + "?" + link_id ;
	}

	gscreen = gdk_screen_get_default() ;

#ifdef HAVE_GTK_SHOW_URI
	gtk_show_uri( gscreen, uri .c_str(), gtk_get_current_event_time(), &error ) ;
#else
	Glib::ustring command = "gnome-open " + uri ;
	gdk_spawn_command_line_on_screen( gscreen, command .c_str(), &error ) ;
#endif
	if ( error != NULL )
	{
		//Try opening yelp application directly
		g_clear_error( &error ) ;  //Clear error from trying to open gparted help manual above (gtk_show_uri or gnome-open).
		Glib::ustring command = "yelp " + uri ;
		gdk_spawn_command_line_on_screen( gscreen, command .c_str(), &error ) ;
	}

	if ( error != NULL )
	{
		Gtk::MessageDialog dialog( *this
		                         , _( "Unable to open GParted Manual help file" )
		                         , false
		                         , Gtk::MESSAGE_ERROR
		                         , Gtk::BUTTONS_OK
		                         , true
		                         ) ;
		dialog .set_secondary_text( error ->message ) ;
		dialog .run() ;
	}
}

void Win_GParted::menu_help_contents()
{
#ifdef HAVE_DISABLE_DOC
	//GParted was configured with --disable-doc
	Gtk::MessageDialog dialog( *this,
			_( "Documentation is not available" ),
			false,
			Gtk::MESSAGE_INFO,
			Gtk::BUTTONS_OK,
			true ) ;
	Glib::ustring tmp_msg = _( "This build of gparted is configured without documentation." ) ;
	tmp_msg += "\n" ;
	tmp_msg += _( "Documentation is available at the project web site." ) ;
	tmp_msg += "\n" ;
	tmp_msg += "http://gparted.org" ;
	dialog .set_secondary_text( tmp_msg ) ;
	dialog .run() ;
#else
	//GParted was configured without --disable-doc
	show_help_dialog( "gparted", "" );
#endif
}

void Win_GParted::menu_help_about()
{
	std::vector<Glib::ustring> strings ;
	
	Gtk::AboutDialog dialog ;
	dialog .set_transient_for( *this ) ;
	
	dialog .set_name( _("GParted") ) ;
	dialog .set_logo_icon_name( "gparted" ) ;
	dialog .set_version( VERSION ) ;
	dialog .set_comments( _( "GNOME Partition Editor" ) ) ;
	dialog .set_copyright( "Copyright © 2004-2006 Bart Hakvoort\nCopyright © 2008-2013 Curtis Gedak" ) ;

	//authors
	//Names listed in alphabetical order by LAST name.
	//See also AUTHORS file -- names listed in opposite order to try to be fair.
	strings .push_back( "Sinlu Bes <e80f00@gmail.com>" ) ;
	strings .push_back( "Luca Bruno <lucab@debian.org>" ) ;
	strings .push_back( "Jérôme Dumesnil <jerome.dumesnil@gmail.com>" ) ;
	strings .push_back( "Markus Elfring <elfring@users.sourceforge.net>" ) ;
	strings .push_back( "Mike Fleetwood <mike.fleetwood@googlemail.com>" ) ;
	strings .push_back( "Curtis Gedak <gedakc@users.sf.net>" ) ;
	strings .push_back( "Matthias Gehre <m.gehre@gmx.de>" ) ;
	strings .push_back( "Rogier Goossens <goossens.rogier@gmail.com>" ) ;
	strings .push_back( "Bart Hakvoort <gparted@users.sf.net>" ) ;
	strings .push_back( "Seth Heeren <sgheeren@gmail.com>" ) ;
	strings .push_back( "Joan Lledó <joanlluislledo@gmail.com>" ) ;
	strings .push_back( "Phillip Susi <psusi@cfl.rr.com>" ) ;
	dialog .set_authors( strings ) ;
	strings .clear() ;

	//artists
	strings .push_back( "Sebastian Kraft <kraft.sebastian@gmail.com>" ) ;
	dialog .set_artists( strings ) ;
	strings .clear() ;

	/*TO TRANSLATORS: your name(s) here please, if there are more translators put newlines (\n) between the names.
	  It's a good idea to provide the url of your translation team as well. Thanks! */
	Glib::ustring str_credits = _("translator-credits") ;
	if ( str_credits != "translator-credits" )
		dialog .set_translator_credits( str_credits ) ;


  	//the url is not clickable because this would introduce an new dep (gnome-vfsmm) 
	dialog .set_website( "http://gparted.org" ) ;

	dialog .run() ;
}

void Win_GParted::on_partition_selected( const Partition & partition, bool src_is_treeview ) 
{
	selected_partition = partition;

	set_valid_operations() ;
	
	if ( src_is_treeview )
		drawingarea_visualdisk .set_selected( partition ) ;
	else
		treeview_detail .set_selected( partition ) ;
}

void Win_GParted::on_partition_activated() 
{
	activate_info() ;
}

void Win_GParted::on_partition_popup_menu( unsigned int button, unsigned int time ) 
{
	menu_partition .popup( button, time );
}

bool Win_GParted::max_amount_prim_reached() 
{
	//FIXME: this is the only place where primary_count is used... instead of counting the primaries on each
	//refresh, we could just count them here.
	//Display error if user tries to create more primary partitions than the partition table can hold. 
	if ( ! selected_partition .inside_extended && primary_count >= devices[ current_device ] .max_prims )
	{
		Gtk::MessageDialog dialog( 
			*this,
			String::ucompose( ngettext( "It is not possible to create more than %1 primary partition"
			                          , "It is not possible to create more than %1 primary partitions"
			                          , devices[ current_device ] .max_prims
			                          )
			                , devices[ current_device ] .max_prims
			                ),
			false,
			Gtk::MESSAGE_ERROR,
			Gtk::BUTTONS_OK,
			true ) ;
		
		dialog .set_secondary_text(
			_( "If you want more partitions you should first create an extended partition. Such a partition can contain other partitions. Because an extended partition is also a primary partition it might be necessary to remove a primary partition first.") ) ;
		
		dialog .run() ;
		
		return true ;
	}
	
	return false ;
}

void Win_GParted::activate_resize()
{
	std::vector<Partition> partitions = devices[ current_device ] .partitions ;
	
	if ( operations .size() )
		for (unsigned int t = 0 ; t < operations .size() ; t++ )
			if ( operations[ t ] ->device == devices[ current_device ] )
				operations[ t ] ->apply_to_visual( partitions ) ;
	
	Dialog_Partition_Resize_Move dialog( gparted_core .get_fs( selected_partition .filesystem ), 
					     devices[ current_device ] .cylsize ) ;
			
	if ( selected_partition .type == GParted::TYPE_LOGICAL )
	{
		unsigned int ext = 0 ;
		while ( ext < partitions .size() && partitions[ ext ] .type != GParted::TYPE_EXTENDED ) ext++ ;
		dialog .Set_Data( selected_partition, partitions[ ext ] .logicals );
	}
	else
		dialog .Set_Data( selected_partition, partitions );
		
	dialog .set_transient_for( *this ) ;	
			
	if ( dialog .run() == Gtk::RESPONSE_OK )
	{
		dialog .hide() ;
		
		//if selected_partition is NEW we simply remove the NEW operation from the list and add 
		//it again with the new size and position ( unless it's an EXTENDED )
		if ( selected_partition .status == GParted::STAT_NEW && selected_partition .type != GParted::TYPE_EXTENDED )
		{
			//remove operation which creates this partition
			for ( unsigned int t = 0 ; t < operations .size() ; t++ )
			{
				if ( operations[ t ] ->partition_new == selected_partition )
				{
					remove_operation( t ) ;
					
					//And add the new partition to the end of the operations list
					//change 'selected_partition' into a suitable 'partition_original') 
					selected_partition .Set_Unallocated( devices[ current_device ] .get_path(),
									     selected_partition .sector_start,
									     selected_partition .sector_end,
									     devices[current_device] .sector_size,
									     selected_partition .inside_extended ) ;

					Operation * operation = new OperationCreate( devices[ current_device ],
										     selected_partition,
										     dialog .Get_New_Partition( devices[ current_device ] .sector_size ) ) ;
					operation ->icon = render_icon( Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU );

					Add_Operation( operation ) ;

					break;
				}
			}
		}
		else//normal move/resize on existing partition
		{
			Operation * operation = new OperationResizeMove( devices[ current_device ],
								         selected_partition,
								         dialog .Get_New_Partition( devices[ current_device ] .sector_size) );
			operation ->icon = render_icon( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_MENU );

			Add_Operation( operation ) ;

			//Display notification if move operation has been queued
			if (   operation ->partition_original .sector_start != operation ->partition_new .sector_start
			    && operation ->partition_original .type != TYPE_EXTENDED
			   )
			{
				//Warn that move operation might break boot process
				Gtk::MessageDialog dialog( *this
				                         , _( "Moving a partition might cause your operating system to fail to boot" )
				                         , false
				                         , Gtk::MESSAGE_WARNING
				                         , Gtk::BUTTONS_OK
				                         , true
				                         ) ;
				Glib::ustring tmp_msg =
					/*TO TRANSLATORS: looks like   You queued an operation to move the start sector of partition /dev/sda3. */
					String::ucompose( _( "You have queued an operation to move the start sector of partition %1." )
					                , operation ->partition_original .get_path()
					                ) ;
				tmp_msg += _( "  Failure to boot is most likely to occur if you move the GNU/Linux partition containing /boot, or if you move the Windows system partition C:." ) ;
				tmp_msg += "\n" ;
				tmp_msg += _( "You can learn how to repair the boot configuration in the GParted FAQ." ) ;
				tmp_msg += "\n" ;
				tmp_msg += "http://gparted.org/faq.php" ;
				tmp_msg += "\n\n" ;
				tmp_msg += _( "Moving a partition might take a very long time to apply." ) ;
				dialog .set_secondary_text( tmp_msg ) ;
				dialog .run() ;
			}

			// Try to merge with previous operation
			if ( operations .size() >= 2 )
			{
				Merge_Operations(operations .size() - 2, operations .size() - 1);
			}
		}
	}
}

void Win_GParted::activate_copy()
{
	copied_partition = selected_partition ;
}

void Win_GParted::activate_paste()
{
	//if max_prims == -1 the current device has an unrecognised disklabel (see also GParted_Core::get_devices)
	if ( devices [ current_device ] .max_prims == -1 )
	{
		show_disklabel_unrecognized( devices [current_device ] .get_path() ) ;
		return ;
	}

	if ( selected_partition .type == GParted::TYPE_UNALLOCATED )
	{
		if ( ! max_amount_prim_reached() )
		{
			Dialog_Partition_Copy dialog( gparted_core .get_fs( copied_partition .filesystem ),
						      devices[ current_device ] .cylsize ) ;
			//we don't need the messages/mount points of the source partition.
			copied_partition .messages .clear() ;
			copied_partition .clear_mountpoints() ;
			dialog .Set_Data( selected_partition, copied_partition ) ;
			dialog .set_transient_for( *this );
		
			if ( dialog .run() == Gtk::RESPONSE_OK )
			{
				dialog .hide() ;

				Operation * operation = new OperationCopy( devices[ current_device ],
									   selected_partition,
									   dialog .Get_New_Partition( devices[ current_device ] .sector_size ),
									   copied_partition ) ;
				operation ->icon = render_icon( Gtk::Stock::COPY, Gtk::ICON_SIZE_MENU );

				Add_Operation( operation ) ;
			}
		}
	}
	else
	{
		bool shown_dialog = false ;
		//VGNAME from mount mount
		if ( selected_partition .filesystem == FS_LVM2_PV && ! selected_partition .get_mountpoint() .empty() )
		{
			if ( ! remove_non_empty_lvm2_pv_dialog( OPERATION_COPY ) )
				return ;
			shown_dialog = true ;
		}

		Partition partition_new = selected_partition ;
		partition_new .alignment = ALIGN_STRICT ;
		partition_new .filesystem = copied_partition .filesystem ;
		partition_new .set_label( copied_partition .get_label() ) ;
		partition_new .uuid = copied_partition .uuid ;
		partition_new .color = copied_partition .color ;
		Sector new_size = partition_new .get_sector_length() ;
		if ( copied_partition .get_sector_length() == new_size )
		{
			//Pasting into same size existing partition, therefore only block copy operation
			//  will be performed maintaining the file system size.
			partition_new .set_sector_usage(
					copied_partition .sectors_used + copied_partition. sectors_unused,
					copied_partition. sectors_unused ) ;
		}
		else
		{
			//Pasting into larger existing partition, therefore block copy followed by file system
			//  grow operations (if supported) will be performed making the file system fill the
			//  partition.
			partition_new .set_sector_usage(
					new_size,
					new_size - copied_partition .sectors_used ) ;
		}
		partition_new .messages .clear() ;
 
		Operation * operation = new OperationCopy( devices[ current_device ],
					 	       	   selected_partition,
							   partition_new,
						       	   copied_partition ) ;
		operation ->icon = render_icon( Gtk::Stock::COPY, Gtk::ICON_SIZE_MENU );

		Add_Operation( operation ) ;

		if ( ! shown_dialog )
		{
			//Only warn that this paste operation will overwrite data in the existing
			//  partition if not already shown the remove non-empty LVM2 PV dialog.
			Gtk::MessageDialog dialog( *this
			                         , _( "You have pasted into an existing partition" )
			                         , false
			                         , Gtk::MESSAGE_WARNING
			                         , Gtk::BUTTONS_OK
			                         , true
			                         ) ;
			/*TO TRANSLATORS: looks like   The data in /dev/sda3 will be lost if you apply this operation. */
			dialog .set_secondary_text(
					String::ucompose( _( "The data in %1 will be lost if you apply this operation." ),
					partition_new .get_path() ) ) ;
			dialog .run() ;
		}
	}
}

void Win_GParted::activate_new()
{
	//if max_prims == -1 the current device has an unrecognised disklabel (see also GParted_Core::get_devices)
	if ( devices [ current_device ] .max_prims == -1 )
	{
		show_disklabel_unrecognized( devices [current_device ] .get_path() ) ;
	}
	else if ( ! max_amount_prim_reached() )
	{	
		Dialog_Partition_New dialog;
		
		dialog .Set_Data( selected_partition, 
				  index_extended > -1,
				  new_count,
				  gparted_core .get_filesystems(),
				  devices[ current_device ] .readonly,
				  devices[ current_device ] .disktype ) ;
		
		dialog .set_transient_for( *this );
		
		if ( dialog .run() == Gtk::RESPONSE_OK )
		{
			dialog .hide() ;
			
			new_count++ ;
			Operation *operation = new OperationCreate( devices[ current_device ],
								    selected_partition,
								    dialog .Get_New_Partition( devices[ current_device ] .sector_size ) ) ;
			operation ->icon = render_icon( Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU );

			Add_Operation( operation );
		}
	}
}

void Win_GParted::activate_delete()
{ 
	//VGNAME from mount mount
	if ( selected_partition .filesystem == FS_LVM2_PV && ! selected_partition .get_mountpoint() .empty() )
	{
		if ( ! remove_non_empty_lvm2_pv_dialog( OPERATION_DELETE ) )
			return ;
	}

	/* since logicals are *always* numbered from 5 to <last logical> there can be a shift
	 * in numbers after deletion.
	 * e.g. consider /dev/hda5 /dev/hda6 /dev/hda7. Now after removal of /dev/hda6,
	 * /dev/hda7 is renumbered to /dev/hda6
	 * the new situation is now /dev/hda5 /dev/hda6. If /dev/hda7 was mounted 
	 * the OS cannot find /dev/hda7 anymore and the results aren't that pretty.
	 * It seems best to check for this and prohibit deletion with some explanation to the user.*/
	 if ( 	selected_partition .type == GParted::TYPE_LOGICAL &&
		selected_partition .status != GParted::STAT_NEW && 
		selected_partition .partition_number < devices[ current_device ] .highest_busy )
	{	
		Gtk::MessageDialog dialog( *this,
					   String::ucompose( _( "Unable to delete %1!"), selected_partition .get_path() ),
					   false,
					   Gtk::MESSAGE_ERROR,
					   Gtk::BUTTONS_OK,
					   true ) ;

		dialog .set_secondary_text( 
			String::ucompose( _("Please unmount any logical partitions having a number higher than %1"),
					  selected_partition .partition_number ) ) ;
		
		dialog .run() ;
		return;
	}
	
	//if partition is on the clipboard...(NOTE: we can't use Partition::== here..)
	if ( selected_partition .get_path() == copied_partition .get_path() )
	{
		Gtk::MessageDialog dialog( *this,
					   String::ucompose( _( "Are you sure you want to delete %1?"), 
					      		     selected_partition .get_path() ),
					   false,
					   Gtk::MESSAGE_QUESTION,
					   Gtk::BUTTONS_NONE,
					   true ) ;

		dialog .set_secondary_text( _("After deletion this partition is no longer available for copying.") ) ;
		
		/*TO TRANSLATORS: dialogtitle, looks like   Delete /dev/hda2 (ntfs, 2345 MiB) */
		dialog .set_title( String::ucompose( _("Delete %1 (%2, %3)"), 
						     selected_partition .get_path(), 
						     Utils::get_filesystem_string( selected_partition .filesystem ),
						     Utils::format_size( selected_partition .get_sector_length(), selected_partition .sector_size ) ) );
		dialog .add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
		dialog .add_button( Gtk::Stock::DELETE, Gtk::RESPONSE_OK );
	
		dialog .show_all_children() ;

		if ( dialog .run() != Gtk::RESPONSE_OK )
			return ;
	}
	
	//if deleted partition was on the clipboard we erase it...
	if ( selected_partition .get_path() == copied_partition .get_path() )
		copied_partition .Reset() ;
			
	/* if deleted one is NEW, it doesn't make sense to add it to the operationslist,
	 * we erase its creation and possible modifications like resize etc.. from the operationslist.
	 * Calling Refresh_Visual will wipe every memory of its existence ;-)*/
	if ( selected_partition .status == GParted::STAT_NEW )
	{
		//remove all operations done on this new partition (this includes creation)	
		for ( int t = 0 ; t < static_cast<int>( operations .size() ) ; t++ ) 
			if ( operations[ t ] ->partition_new .get_path() == selected_partition .get_path() )
				remove_operation( t-- ) ;
				
		//determine lowest possible new_count
		new_count = 0 ; 
		for ( unsigned int t = 0 ; t < operations .size() ; t++ )
			if ( operations[ t ] ->partition_new .status == GParted::STAT_NEW &&
			     operations[ t ] ->partition_new .partition_number > new_count )
				new_count = operations[ t ] ->partition_new .partition_number ;
			
		new_count += 1 ;
			
		// Verify if the two operations can be merged
		for ( int t = 0 ; t < static_cast<int>( operations .size() - 1 ) ; t++ )
		{
			Merge_Operations( t, t+1 );
		}

		Refresh_Visual(); 
				
		if ( ! operations .size() )
			close_operationslist() ;
	}
	else //deletion of a real partition...
	{
		Operation * operation = new OperationDelete( devices[ current_device ], selected_partition ) ;
		operation ->icon = render_icon( Gtk::Stock::DELETE, Gtk::ICON_SIZE_MENU ) ;

		Add_Operation( operation ) ;
	}
}

void Win_GParted::activate_info()
{
	Dialog_Partition_Info dialog( selected_partition );
	dialog .set_transient_for( *this );
	dialog .run();
}

void Win_GParted::activate_format( GParted::FILESYSTEM new_fs )
{
	//VGNAME from mount mount
	if ( selected_partition .filesystem == FS_LVM2_PV && ! selected_partition .get_mountpoint() .empty() )
	{
		if ( ! remove_non_empty_lvm2_pv_dialog( OPERATION_FORMAT ) )
			return ;
	}

	//check for some limits...
	fs = gparted_core .get_fs( new_fs ) ;

	if ( ( selected_partition .get_byte_length() < fs .MIN ) ||
	     ( fs .MAX && selected_partition .get_byte_length() > fs .MAX ) )
	{
		Gtk::MessageDialog dialog( *this,
					   String::ucompose( 
							/* TO TRANSLATORS: looks like
							* Cannot format this file system to fat16.
							*/
							_( "Cannot format this file system to %1" ),
							   Utils::get_filesystem_string( new_fs ) ) ,
					   false,
					   Gtk::MESSAGE_ERROR,
					   Gtk::BUTTONS_OK,
					   true );

		if ( selected_partition .get_byte_length() < fs .MIN )
			dialog .set_secondary_text( String::ucompose(
						/* TO TRANSLATORS: looks like
						 * A fat16 file system requires a partition of at least 16.00 MiB.
						 */
						_( "A %1 file system requires a partition of at least %2."),
						Utils::get_filesystem_string( new_fs ),
						Utils::format_size( fs .MIN, 1 /* Byte */ ) ) );
		else
			dialog .set_secondary_text( String::ucompose(
						/* TO TRANSLATORS: looks like
						 * A partition with a hfs file system has a maximum size of 2.00 GiB.
						 */
						_( "A partition with a %1 file system has a maximum size of %2."),
						Utils::get_filesystem_string( new_fs ),
						Utils::format_size( fs .MAX, 1 /* Byte */ ) ) );
		
		dialog .run() ;
		return ;
	}
	
	//ok we made it.  lets create an fitting partition object
	Partition part_temp ;
	part_temp .Set( devices[ current_device ] .get_path(),
			selected_partition .get_path(),
			selected_partition .partition_number,
			selected_partition .type,
			new_fs,
			selected_partition .sector_start,
			selected_partition .sector_end,
			devices[ current_device ] .sector_size,
			selected_partition .inside_extended,
			false ) ;
	//Leave sector usage figures to new Partition object defaults of
	//  -1, -1, 0 (_used, _unused, _unallocated) representing unknown.
	 
	part_temp .status = GParted::STAT_FORMATTED ;
	
	//if selected_partition is NEW we simply remove the NEW operation from the list and
	//add it again with the new file system
	if ( selected_partition .status == GParted::STAT_NEW )
	{
		//remove operation which creates this partition
		for ( unsigned int t = 0 ; t < operations .size() ; t++ )
		{
			if ( operations[ t ] ->partition_new == selected_partition )
			{
				remove_operation( t ) ;
				
				//And insert the new partition at the old position in the operations list
				//(NOTE: in this case we set status to STAT_NEW)
				part_temp .status = STAT_NEW ;
				
				Operation * operation = new OperationCreate( devices[ current_device ],
									     selected_partition,
								 	     part_temp ) ;
				operation ->icon = render_icon( Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU );

				Add_Operation( operation, t ) ;
					
				break;
			}
		}
	}
	else//normal formatting of an existing partition
	{
		Operation *operation = new OperationFormat( devices[ current_device ],
				 			    selected_partition,
						 	    part_temp );
		operation ->icon = render_icon( Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU );

		Add_Operation( operation ) ;

		// Try to merge with previous operation
		if ( operations .size() >= 2 )
		{
			Merge_Operations( operations .size() - 2, operations .size() - 1 );
		}
	}
}

void Win_GParted::thread_unmount_partition( bool * succes, Glib::ustring * error ) 
{
	std::vector<Glib::ustring> errors, failed_mountpoints, mountpoints = gparted_core .get_all_mountpoints() ;
	Glib::ustring dummy ;

	*succes = true ; 
	for ( unsigned int t = 0 ; t < selected_partition .get_mountpoints() .size() ; t++ )
		if ( std::count( mountpoints .begin(),
				 mountpoints .end(),
				 selected_partition .get_mountpoints()[ t ] ) <= 1 ) 
		{
			if ( Utils::execute_command( "umount -v \"" + selected_partition .get_mountpoints()[ t ] + "\"",
						     dummy,
						     *error ) )
			{
				*succes = false ;
				errors .push_back( *error ) ;	
			}
		}
		else
			failed_mountpoints .push_back( selected_partition .get_mountpoints()[ t ] ) ;

	
	if ( *succes && failed_mountpoints .size() )
	{
		*succes = false ;
		*error = _("The partition could not be unmounted from the following mount points:") ;
		*error += "\n\n<i>" + Glib::build_path( "\n", failed_mountpoints ) + "</i>\n\n" ;
		*error +=  _("Most likely other partitions are also mounted on these mount points. You are advised to unmount them manually.") ;
	}
	else
		*error = "<i>" + Glib::build_path( "\n", errors ) + "</i>" ;

	pulse = false ;
}
	
void Win_GParted::thread_mount_partition( Glib::ustring mountpoint, bool * succes, Glib::ustring * error ) 
{
	Glib::ustring dummy ;
	std::vector<Glib::ustring> errors ;
	
	*succes = ! Utils::execute_command( "mount -v " + selected_partition .get_path() + " \"" + mountpoint + "\"",
					    dummy,
					    *error ) ;

	pulse = false ;
}

void Win_GParted::thread_toggle_swap( bool * succes, Glib::ustring * error ) 
{	
	Glib::ustring dummy ;
	
	if ( selected_partition .busy )
		*succes = ! Utils::execute_command( "swapoff -v " + selected_partition .get_path() + " && sync",
						    dummy,
						    *error ) ;
	else
		*succes = ! Utils::execute_command( "swapon -v " + selected_partition .get_path() + " && sync",
						    dummy,
						    *error ) ;

	pulse = false ;
}

void Win_GParted::thread_toggle_lvm2_pv( bool * success, Glib::ustring * error )
{
	Glib::ustring dummy ;

	if ( selected_partition .busy )
		//VGNAME from mount point
		*success = ! Utils::execute_command( "lvm vgchange -a n " + selected_partition .get_mountpoint(),
		                                     dummy,
		                                     *error ) ;
	else
		*success = ! Utils::execute_command( "lvm vgchange -a y " + selected_partition .get_mountpoint(),
		                                     dummy,
		                                     *error ) ;

	pulse = false ;
}

// Runs gpart in a thread
void Win_GParted::thread_guess_partition_table()
{
	this->gpart_output="";
	this->gparted_core.guess_partition_table(devices[ current_device ], this->gpart_output);
	pulse=false;
}

void Win_GParted::toggle_busy_state()
{
	int operation_count = partition_in_operation_queue_count( selected_partition ) ;
	if ( operation_count > 0 )
	{
		//Note that this situation will only occur when trying to swapon a partition
		//  or activate the Volume Group of a Physical Volume.  This is because
		//  GParted does not permit queueing operations on partitions that are
		//  currently active (i.e., swap enabled, mounted or active VG).  Hence
		//  this situation will not occur for the swapoff, unmount or deactivate VG
		//  actions that this method handles.

		/*TO TRANSLATORS: Singular case looks like   1 operation is currently pending for partition /dev/sdd8. */
		Glib::ustring tmp_msg =
		    String::ucompose( ngettext( "%1 operation is currently pending for partition %2"
		                              , "%1 operations are currently pending for partition %2"
		                              , operation_count
		                              )
		                    , operation_count
		                    , selected_partition .get_path()
		                    ) ;
		Gtk::MessageDialog dialog( *this
		                         , tmp_msg
		                         , false
		                         , Gtk::MESSAGE_INFO
		                         , Gtk::BUTTONS_OK
		                         , true
		                         ) ;
		if ( selected_partition .filesystem == GParted::FS_LINUX_SWAP )
		{
			tmp_msg = _( "The swapon action cannot be performed if an operation is pending for the partition." ) ;
			tmp_msg += "\n" ;
			tmp_msg += _( "Use the Edit menu to undo, clear, or apply operations before using swapon with this partition." ) ;
		}
		else if ( selected_partition .filesystem == GParted::FS_LVM2_PV )
		{
			tmp_msg = _( "The activate Volume Group action cannot be performed if an operation is pending for the partition." ) ;
			tmp_msg += "\n" ;
			tmp_msg += _( "Use the Edit menu to undo, clear, or apply operations before using activate Volume Group with this partition." ) ;
		}
		dialog .set_secondary_text( tmp_msg ) ;
		dialog .run() ;
		return ;
	}

	bool succes = false ;
	Glib::ustring error ;

	pulse = true ;

	if ( selected_partition .filesystem == GParted::FS_LINUX_SWAP )
	{
		thread = Glib::Thread::create( sigc::bind<bool *, Glib::ustring *>( 
			sigc::mem_fun( *this, &Win_GParted::thread_toggle_swap ), &succes, &error ), true ) ;

		show_pulsebar( 
			String::ucompose( 
				selected_partition .busy ? _("Deactivating swap on %1") : _("Activating swap on %1"),
				selected_partition .get_path() ) ) ;

		if ( ! succes )
		{
			Gtk::MessageDialog dialog( 
				*this,
				selected_partition .busy ? _("Could not deactivate swap") : _("Could not activate swap"),
				false,
				Gtk::MESSAGE_ERROR,
				Gtk::BUTTONS_OK,
				true ) ;

			dialog .set_secondary_text( error ) ;
			
			dialog.run() ;
		}
	}
	else if ( selected_partition .filesystem == GParted::FS_LVM2_PV )
	{
		thread = Glib::Thread::create( sigc::bind<bool *, Glib::ustring *>(
			sigc::mem_fun( *this, &Win_GParted::thread_toggle_lvm2_pv ), &succes, &error ), true ) ;

		show_pulsebar(
			String::ucompose(
				selected_partition .busy ? _("Deactivating Volume Group %1")
				                         : _("Activating Volume Group %1"),
				//VGNAME from mount point
				selected_partition .get_mountpoint() ) ) ;

		if ( ! succes )
		{
			Gtk::MessageDialog dialog(
				*this,
				selected_partition .busy ? _("Could not deactivate Volume Group")
				                         : _("Could not activate Volume Group"),
				false,
				Gtk::MESSAGE_ERROR,
				Gtk::BUTTONS_OK,
				true ) ;

			dialog .set_secondary_text( error ) ;

			dialog.run() ;
		}
	}
	else if ( selected_partition .busy )
	{
		thread = Glib::Thread::create( sigc::bind<bool *, Glib::ustring *>( 
			sigc::mem_fun( *this, &Win_GParted::thread_unmount_partition ), &succes, &error ), true ) ;

		show_pulsebar( String::ucompose( _("Unmounting %1"), selected_partition .get_path() ) ) ;
	
		if ( ! succes )
		{
			Gtk::MessageDialog dialog( *this, 
						   String::ucompose( _("Could not unmount %1"), selected_partition .get_path() ),
						   false,
						   Gtk::MESSAGE_ERROR,
						   Gtk::BUTTONS_OK,
						   true );

			dialog .set_secondary_text( error, true ) ;
		
			dialog.run() ;
		}
	}

	menu_gparted_refresh_devices() ;
}

void Win_GParted::activate_mount_partition( unsigned int index ) 
{
	int operation_count = partition_in_operation_queue_count( selected_partition ) ;
	if ( operation_count > 0 )
	{
		/*TO TRANSLATORS: Plural case looks like   4 operations are currently pending for partition /dev/sdd8. */
		Glib::ustring tmp_msg =
		    String::ucompose( ngettext( "%1 operation is currently pending for partition %2"
		                              , "%1 operations are currently pending for partition %2"
		                              , operation_count
		                              )
		                    , operation_count
		                    , selected_partition .get_path()
		                    ) ;
		Gtk::MessageDialog dialog( *this
		                         , tmp_msg
		                         , false
		                         , Gtk::MESSAGE_INFO
		                         , Gtk::BUTTONS_OK
		                         , true
		                         ) ;
		tmp_msg  = _( "The mount action cannot be performed if an operation is pending for the partition." ) ;
		tmp_msg += "\n" ;
		tmp_msg += _( "Use the Edit menu to undo, clear, or apply operations before using mount with this partition." ) ;
		dialog .set_secondary_text( tmp_msg ) ;
		dialog .run() ;
		return ;
	}

	bool succes = false ;
	Glib::ustring error ;

	pulse = true ;

	thread = Glib::Thread::create( sigc::bind<Glib::ustring, bool *, Glib::ustring *>( 
						sigc::mem_fun( *this, &Win_GParted::thread_mount_partition ),
						selected_partition .get_mountpoints()[ index ],
						&succes,
						&error ),
				       true ) ;

	show_pulsebar( String::ucompose( _("mounting %1 on %2"),
					 selected_partition .get_path(),
					 selected_partition .get_mountpoints()[ index ] ) ) ;

	if ( ! succes )
	{
		Gtk::MessageDialog dialog( *this, 
					   String::ucompose( _("Could not mount %1 on %2"),
						   	     selected_partition .get_path(),
							     selected_partition .get_mountpoints()[ index ] ),
					   false,
					   Gtk::MESSAGE_ERROR,
					   Gtk::BUTTONS_OK,
					   true );

		dialog .set_secondary_text( error, true ) ;
	
		dialog.run() ;
	}

	menu_gparted_refresh_devices() ;
}

void Win_GParted::activate_disklabel()
{
	//If there are active mounted partitions on the device then warn
	//  the user that all partitions must be unactive before creating
	//  a new partition table
	int active_count = active_partitions_on_device_count( devices[ current_device ] ) ;
	if ( active_count > 0 )
	{
		Glib::ustring tmp_msg =
		    String::ucompose( /*TO TRANSLATORS: Singular case looks like  1 partition is currently active on device /dev/sda */
		                      ngettext( "%1 partition is currently active on device %2"
		                      /*TO TRANSLATORS: Plural case looks like    3 partitions are currently active on device /dev/sda */
		                              , "%1 partitions are currently active on device %2"
		                              , active_count
		                              )
		                    , active_count
		                    , devices[ current_device ] .get_path()
		                    ) ;
		Gtk::MessageDialog dialog( *this
		                         , tmp_msg
		                         , false
		                         , Gtk::MESSAGE_INFO
		                         , Gtk::BUTTONS_OK
		                         , true
		                         ) ;
		tmp_msg  = _( "A new partition table cannot be created when there are active partitions." ) ;
		tmp_msg += "  " ;
		tmp_msg += _( "Active partitions are those that are in use, such as a mounted file system, or enabled swap space." ) ;
		tmp_msg += "\n" ;
		tmp_msg += _( "Use Partition menu options, such as unmount or swapoff, to deactivate all partitions on this device before creating a new partition table." ) ;
		dialog .set_secondary_text( tmp_msg ) ;
		dialog .run() ;
		return ;
	}

	//If there are pending operations then warn the user that these
	//  operations must either be applied or cleared before creating
	//  a new partition table.
	if ( operations .size() )
	{
		Glib::ustring tmp_msg =
		    String::ucompose( ngettext( "%1 operation is currently pending"
		                              , "%1 operations are currently pending"
		                              , operations .size()
		                              )
		                    , operations .size()
		                    ) ;
		Gtk::MessageDialog dialog( *this
		                         , tmp_msg
		                         , false
		                         , Gtk::MESSAGE_INFO
		                         , Gtk::BUTTONS_OK
		                         , true
		                         ) ;
		tmp_msg  = _( "A new partition table cannot be created when there are pending operations." ) ;
		tmp_msg += "\n" ;
		tmp_msg += _( "Use the Edit menu to either clear or apply all operations before creating a new partition table." ) ;
		dialog .set_secondary_text( tmp_msg ) ;
		dialog .run() ;
		return ;
	}

	//Display dialog for creating a new partition table.
	Dialog_Disklabel dialog( devices[ current_device ] .get_path(), gparted_core .get_disklabeltypes() ) ;
	dialog .set_transient_for( *this );

	if ( dialog .run() == Gtk::RESPONSE_APPLY )
	{
		if ( ! gparted_core .set_disklabel( devices[ current_device ] .get_path(), dialog .Get_Disklabel() ) )
		{
			Gtk::MessageDialog dialog( *this,
						   _("Error while creating partition table"),
						   true,
						   Gtk::MESSAGE_ERROR,
						   Gtk::BUTTONS_OK,
						   true ) ;
			dialog .run() ;
		}

		dialog .hide() ;
			
		menu_gparted_refresh_devices() ;
	}
}

//Runs when the Device->Attempt Rescue Data is clicked
void Win_GParted::activate_attempt_rescue_data()
{
	if(Glib::find_program_in_path( "gpart" ) .empty()) //Gpart must be installed to continue
	{
		Gtk::MessageDialog errorDialog(*this, "", true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		errorDialog.set_message(_("Command gpart was not found"));
		errorDialog.set_secondary_text(_("This feature uses gpart. Please install gpart and try again."));

		errorDialog.run();

		return;
	}

	//Dialog information
	Glib::ustring sec_text = _( "A full disk scan is needed to find file systems." ) ;
	sec_text += "\n" ;
	sec_text +=_("The scan might take a very long time.");
	sec_text += "\n" ;
	sec_text += _("After the scan you can mount any discovered file systems and copy the data to other media.") ;
	sec_text += "\n" ;
	sec_text += _("Do you want to continue?");

	Gtk::MessageDialog messageDialog(*this, "", true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK_CANCEL, true);
	/*TO TRANSLATORS: looks like	Search for file systems on /deb/sdb */
	messageDialog.set_message(String::ucompose(_("Search for file systems on %1"), devices[ current_device ] .get_path()));
	messageDialog.set_secondary_text(sec_text);

	if(messageDialog.run()!=Gtk::RESPONSE_OK)
	{
		return;
	}

	messageDialog.hide();

	pulse=true;
	this->thread = Glib::Thread::create( sigc::mem_fun( *this, &Win_GParted::thread_guess_partition_table ), true ) ;

	/*TO TRANSLATORS: looks like	Searching for file systems on /deb/sdb */
	show_pulsebar(String::ucompose( _("Searching for file systems on %1"), devices[ current_device ] .get_path()));

	Dialog_Rescue_Data dialog;
	dialog .set_transient_for( *this );

	//Reads the output of gpart
	dialog.init_partitions(&devices[ current_device ], this->gpart_output);

	if(dialog.get_partitions().size()==0) //No partitions found
	{
		//Dialog information
		Gtk::MessageDialog errorDialog(*this, "", true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		
		/*TO TRANSLATORS: looks like	No file systems found on /deb/sdb */
		errorDialog.set_message(String::ucompose(_("No file systems found on %1"), devices[ current_device ] .get_path()));
		errorDialog.set_secondary_text(_("The disk scan by gpart did not find any recognizable file systems on this disk."));

		errorDialog.run();
		return;
	}

	dialog.run();
	dialog.hide();

	Glib::ustring commandUmount= "umount /tmp/gparted-roview*";

	Utils::execute_command(commandUmount);

	menu_gparted_refresh_devices() ;
}

void Win_GParted::activate_manage_flags() 
{
	get_window() ->set_cursor( Gdk::Cursor( Gdk::WATCH ) ) ;
	while ( Gtk::Main::events_pending() )
		Gtk::Main::iteration() ;

	DialogManageFlags dialog( selected_partition, gparted_core .get_available_flags( selected_partition ) ) ;
	dialog .set_transient_for( *this ) ;
	dialog .signal_get_flags .connect(
		sigc::mem_fun( &gparted_core, &GParted_Core::get_available_flags ) ) ;
	dialog .signal_toggle_flag .connect(
		sigc::mem_fun( &gparted_core, &GParted_Core::toggle_flag ) ) ;

	get_window() ->set_cursor() ;
	
	dialog .run() ;
	dialog .hide() ;
	
	if ( dialog .any_change )
		menu_gparted_refresh_devices() ;
}
	
void Win_GParted::activate_check() 
{
	Operation *operation = new OperationCheck( devices[ current_device ], selected_partition ) ;

	operation ->icon = render_icon( Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU );

	Add_Operation( operation ) ;

	// Verify if the two operations can be merged
	for ( unsigned int t = 0 ; t < operations .size() - 1 ; t++ )
	{
		if ( operations[ t ] ->type == OPERATION_CHECK )
		{
			if( Merge_Operations( t, operations .size() -1 ) )
				break;
		}
	}
}

void Win_GParted::activate_label_partition() 
{
	Dialog_Partition_Label dialog( selected_partition );
	dialog .set_transient_for( *this );

	if (	( dialog .run() == Gtk::RESPONSE_OK )
	     && ( dialog .get_new_label() != selected_partition .get_label() ) )
	{
		dialog .hide() ;
		//Make a duplicate of the selected partition (used in UNDO)
		Partition part_temp = selected_partition ;

		part_temp .set_label( dialog .get_new_label() ) ;

		Operation * operation = new OperationLabelPartition( devices[ current_device ],
									selected_partition, part_temp ) ;
		operation ->icon = render_icon( Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU );

		Add_Operation( operation ) ;

		// Verify if the two operations can be merged
		for ( unsigned int t = 0 ; t < operations .size() - 1 ; t++ )
		{
			if ( operations[ t ] ->type == OPERATION_LABEL_PARTITION )
			{
				if( Merge_Operations( t, operations .size() -1 ) )
					break;
			}
		}
	}
}
	
void Win_GParted::activate_change_uuid()
{
	if ( gparted_core .get_filesystem_object( selected_partition .filesystem ) ->get_custom_text ( CTEXT_CHANGE_UUID_WARNING ) != "" ) {
		int i ;
		Gtk::MessageDialog dialog( *this
					 , gparted_core .get_filesystem_object( selected_partition .filesystem ) ->get_custom_text ( CTEXT_CHANGE_UUID_WARNING, 0 )
					 , false
					 , Gtk::MESSAGE_WARNING
					 , Gtk::BUTTONS_OK
					 , true
					 ) ;
		Glib::ustring tmp_msg = "" ;
		for ( i = 1 ; gparted_core .get_filesystem_object( selected_partition .filesystem ) ->get_custom_text ( CTEXT_CHANGE_UUID_WARNING, i ) != "" ; i++ ) {
			if ( i > 1 )
				tmp_msg += "\n\n" ;
			tmp_msg += gparted_core .get_filesystem_object( selected_partition .filesystem ) ->get_custom_text ( CTEXT_CHANGE_UUID_WARNING, i ) ;
		}
		dialog .set_secondary_text( tmp_msg ) ;
		dialog .run() ;
	}

	//Make a duplicate of the selected partition (used in UNDO)
	Partition part_temp = selected_partition ;

	if ( part_temp .filesystem == GParted::FS_NTFS )
		//Explicitly ask for half, so that the user will be aware of it
		//Also, keep this kind of policy out of the NTFS code.
		part_temp .uuid = UUID_RANDOM_NTFS_HALF ;
	else
		part_temp .uuid = UUID_RANDOM ;


	Operation * operation = new OperationChangeUUID( devices[ current_device ],
								selected_partition, part_temp ) ;
	operation ->icon = render_icon( Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU );

	Add_Operation( operation ) ;

	// Verify if the two operations can be merged
	for ( unsigned int t = 0 ; t < operations .size() - 1 ; t++ )
	{
		if ( operations[ t ] ->type == OPERATION_CHANGE_UUID )
		{
			if( Merge_Operations( t, operations .size() -1 ) )
				break;
		}
	}
}

void Win_GParted::activate_undo()
{
	//when undoing a creation it's safe to decrease the newcount by one
	if ( operations .back() ->type == OPERATION_CREATE )
		new_count-- ;

	remove_operation() ;		
	
	Refresh_Visual();
	
	if ( ! operations .size() )
		close_operationslist() ;

	//FIXME:  A slight flicker may be introduced by this extra display refresh.
	//An extra display refresh seems to prevent the disk area visual disk from
	//  disappearing when there enough operations to require a scrollbar
	//  (about 4+ operations with default window size) and a user clicks on undo.
	//  See also Win_GParted::Add_operation().
	drawingarea_visualdisk .queue_draw() ;
}

void Win_GParted::remove_operation( int index, bool remove_all ) 
{
	if ( remove_all )
	{
		for ( unsigned int t = 0 ; t < operations .size() ; t++ )
			delete operations[ t ] ;

		operations .clear() ;
	}
	else if ( index == -1  && operations .size() > 0 )
	{
		delete operations .back() ;
		operations .pop_back() ;
	}
	else if ( index > -1 && index < static_cast<int>( operations .size() ) )
	{
		delete operations[ index ] ;
		operations .erase( operations .begin() + index ) ;
	}
}

int Win_GParted::partition_in_operation_queue_count( const Partition & partition )
{
	int operation_count = 0 ;

	for ( unsigned int t = 0 ; t < operations .size() ; t++ )
	{
		if ( partition .get_path() == operations[ t ] ->partition_original .get_path() )
			operation_count++ ;
	}

	return operation_count ;
}

int  Win_GParted::active_partitions_on_device_count( const Device & device )
{
	int active_count = 0 ;

	//Count the active partitions on the device
	for ( unsigned int k=0; k < device .partitions .size(); k++ )
	{
		//Count the active primary partitions
		if (   device .partitions[ k ] .busy
		    && device .partitions[ k ] .type != TYPE_EXTENDED
		    && device .partitions[ k ] .type != TYPE_UNALLOCATED
		   )
			active_count++ ;

		//Count the active logical partitions
		if (   device .partitions[ k ] .busy
		    && device .partitions[ k ] .type == TYPE_EXTENDED
		   )
		{
			for ( unsigned int j=0; j < device .partitions[ k ] .logicals .size(); j++ )
			{
				if (   device .partitions[ k ] .logicals [ j ] .busy
				    && device .partitions[ k ] .logicals [ j ] .type != TYPE_UNALLOCATED
				   )
					active_count++ ;
			}
		}
	}

	return active_count ;
}

void Win_GParted::activate_apply()
{
	Gtk::MessageDialog dialog( *this,
				   _("Are you sure you want to apply the pending operations?"),
				   false,
				   Gtk::MESSAGE_WARNING,
				   Gtk::BUTTONS_NONE,
				   true );
	Glib::ustring temp;
	temp =  _( "Editing partitions has the potential to cause LOSS of DATA.") ;
	temp += "\n" ;
	temp += _( "You are advised to backup your data before proceeding." ) ;
	dialog .set_secondary_text( temp ) ;
	dialog .set_title( _( "Apply operations to device" ) );
	
	dialog .add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	dialog .add_button( Gtk::Stock::APPLY, Gtk::RESPONSE_OK );
	
	dialog .show_all_children() ;
	if ( dialog.run() == Gtk::RESPONSE_OK )
	{
		dialog .hide() ; //hide confirmationdialog
		
		Dialog_Progress dialog_progress( operations ) ;
		dialog_progress .set_transient_for( *this ) ;
		dialog_progress .signal_apply_operation .connect(
			sigc::mem_fun(gparted_core, &GParted_Core::apply_operation_to_disk) ) ;
		dialog_progress .signal_get_libparted_version .connect(
			sigc::mem_fun(gparted_core, &GParted_Core::get_libparted_version) ) ;
 
		int response ;
		do
		{
			response = dialog_progress .run() ;
		}
		while ( response == Gtk::RESPONSE_CANCEL || response == Gtk::RESPONSE_OK ) ;
		
		dialog_progress .hide() ;
		
		//wipe operations...
		remove_operation( -1, true ) ;
		hbox_operations .clear() ;
		close_operationslist() ;
							
		//reset new_count to 1
		new_count = 1 ;
		
		//reread devices and their layouts...
		menu_gparted_refresh_devices() ;
	}
}

bool Win_GParted::remove_non_empty_lvm2_pv_dialog( const OperationType optype )
{
	Glib::ustring tmp_msg ;
	switch ( optype )
	{
		case OPERATION_DELETE:
			tmp_msg = String::ucompose( _( "You are deleting non-empty LVM2 Physical Volume %1" ),
			                            selected_partition .get_path() ) ;
			break ;
		case OPERATION_FORMAT:
			tmp_msg = String::ucompose( _( "You are formatting over non-empty LVM2 Physical Volume %1" ),
			                            selected_partition .get_path() ) ;
			break ;
		case OPERATION_COPY:
			tmp_msg = String::ucompose( _( "You are pasting over non-empty LVM2 Physical Volume %1" ),
			                            selected_partition .get_path() ) ;
			break ;
		default:
			break ;
	}
	Gtk::MessageDialog dialog( *this, tmp_msg,
	                           false, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true ) ;

	tmp_msg =  _( "Deleting or overwriting the Physical Volume is irrecoverable and will destroy or damage the "
	              " Volume Group." ) ;
	tmp_msg += "\n\n" ;
	tmp_msg += _( "To avoid destroying or damaging the Volume Group, you are advised to cancel and use external "
	              "LVM commands to free the Physical Volume before attempting this operation." ) ;
	tmp_msg += "\n\n" ;
	tmp_msg += _( "Do you want to continue to forcibly delete the Physical Volume?" ) ;

	LVM2_PV_Info lvm2_pv_info ;
	Glib::ustring vgname = lvm2_pv_info .get_vg_name( selected_partition .get_path() ) ;
	std::vector<Glib::ustring> members ;
	if ( ! vgname .empty() )
		members = lvm2_pv_info .get_vg_members( vgname ) ;

	//Single copy of each string for translation purposes
	const Glib::ustring vgname_label  = _( "Volume Group:" ) ;
	const Glib::ustring members_label = _( "Members:" ) ;

#ifndef HAVE_GET_MESSAGE_AREA
	//Basic method of displaying VG members by appending it to the secondary text in the dialog.
	tmp_msg += "\n____________________\n\n" ;
	tmp_msg += "<b>" ;
	tmp_msg +=       vgname_label ;
	tmp_msg +=                    "</b>    " ;
	tmp_msg +=                               vgname ;
	tmp_msg += "\n" ;
	tmp_msg += "<b>" ;
	tmp_msg +=       members_label ;
	tmp_msg +=                     "</b>" ;
	if ( ! members .empty() )
	{
		tmp_msg += "    " ;
		tmp_msg +=        members [0] ;
		for ( unsigned int i = 1 ; i < members .size() ; i ++ )
		{
			tmp_msg += "    " ;
			tmp_msg +=        members [i] ;
		}
	}
#endif /* ! HAVE_GET_MESSAGE_AREA */

	dialog .set_secondary_text( tmp_msg, true ) ;

#ifdef HAVE_GET_MESSAGE_AREA
	//Nicely formatted method of displaying VG members by using a table below the secondary text
	//  in the dialog.  Uses Gtk::MessageDialog::get_message_area() which was new in gtkmm-2.22
	//  released September 2010.
	Gtk::Box * msg_area = dialog .get_message_area() ;

	Gtk::HSeparator * hsep( manage( new Gtk::HSeparator() ) ) ;
	msg_area ->pack_start( * hsep ) ;

	Gtk::Table * table( manage( new Gtk::Table() ) ) ;
        table ->set_border_width( 0 ) ;
        table ->set_col_spacings( 10 ) ;
        msg_area ->pack_start( * table ) ;

	int top = 0, bottom = 1 ;

	//Volume Group
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( vgname_label ) + "</b>" ),
	                0, 1, top, bottom, Gtk::FILL ) ;
	table ->attach( * Utils::mk_label( vgname, true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
	                1, 2, top++, bottom++, Gtk::FILL ) ;

	//Members
	table ->attach( * Utils::mk_label( "<b>" + Glib::ustring( members_label ) + "</b>"),
			0, 1, top, bottom, Gtk::FILL ) ;

	if ( members .empty() )
		table ->attach( * Utils::mk_label( "" ), 1, 2, top++, bottom++, Gtk::FILL ) ;
	else
	{
		table ->attach( * Utils::mk_label( members [0], true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
				1, 2, top++, bottom++, Gtk::FILL ) ;
		for ( unsigned int i = 1 ; i < members .size() ; i ++ )
		{
			table ->attach( * Utils::mk_label( "" ), 0, 1, top, bottom, Gtk::FILL ) ;
			table ->attach( * Utils::mk_label( members [i], true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, false, true ),
					1, 2, top++, bottom++, Gtk::FILL ) ;
		}

	}
#endif /* HAVE_GET_MESSAGE_AREA */

	dialog .add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	dialog .add_button( Gtk::Stock::DELETE, Gtk::RESPONSE_OK );
	dialog .show_all() ;
	if ( dialog .run() == Gtk::RESPONSE_OK )
		return true ;
	return false ;
}

} // GParted
