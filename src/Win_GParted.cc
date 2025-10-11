/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015 Curtis Gedak
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

#include "Win_GParted.h"
#include "Device.h"
#include "Dialog_Progress.h"
#include "DialogFeatures.h"
#include "DialogPasswordEntry.h"
#include "Dialog_Disklabel.h"
#include "Dialog_Partition_Resize_Move.h"
#include "Dialog_Partition_Copy.h"
#include "Dialog_Partition_New.h"
#include "Dialog_Partition_Info.h"
#include "Dialog_FileSystem_Label.h"
#include "Dialog_Partition_Name.h"
#include "DialogManageFlags.h"
#include "FileSystem.h"
#include "GParted_Core.h"
#include "MenuHelpers.h"
#include "Mount_Info.h"
#include "Operation.h"
#include "OperationCopy.h"
#include "OperationCheck.h"
#include "OperationCreate.h"
#include "OperationDelete.h"
#include "OperationFormat.h"
#include "OperationResizeMove.h"
#include "OperationChangeUUID.h"
#include "OperationLabelFileSystem.h"
#include "OperationNamePartition.h"
#include "Partition.h"
#include "PartitionVector.h"
#include "PasswordRAMStore.h"
#include "LVM2_PV_Info.h"
#include "LUKS_Info.h"
#include "Utils.h"
#include "../config.h"

#include <string.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/aboutdialog.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/main.h>
#include <gtkmm/separator.h>
#include <gtkmm/grid.h>
#include <gtkmm/label.h>
#include <atkmm/relation.h>
#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>
#include <glibmm/main.h>
#include <sigc++/bind.h>
#include <sigc++/signal.h>
#include <vector>
#include <memory>
#include <utility>


namespace GParted
{


Win_GParted::Win_GParted( const std::vector<Glib::ustring> & user_devices )
 : m_current_device(0), m_operationslist_open(true)
{
	copied_partition = nullptr;
	selected_partition_ptr = nullptr;
	new_count = 1;
	gparted_core .set_user_devices( user_devices ) ;

	TOOLBAR_NEW =
	TOOLBAR_DEL =
	TOOLBAR_RESIZE_MOVE =
	TOOLBAR_COPY =
	TOOLBAR_PASTE =
	TOOLBAR_UNDO =
	TOOLBAR_APPLY = -1;

	//==== GUI =========================
	this ->set_title( _("GParted") );
	this ->set_default_size( 775, 500 );
	
	try
	{
		this ->set_default_icon_name( "gparted" ) ;
	}
	catch ( Glib::Exception & e )
	{
		std::cerr << Utils::convert_ustring(e.what()) << std::endl;
	}

	// Pack the main box
	vbox_main.set_orientation(Gtk::ORIENTATION_VERTICAL);
	this ->add( vbox_main ); 
	
	//menubar....
	init_menubar() ;
	vbox_main .pack_start( menubar_main, Gtk::PACK_SHRINK );
	
	//toolbar....
	init_toolbar() ;
	vbox_main.pack_start( hbox_toolbar, Gtk::PACK_SHRINK );
	
	//drawingarea_visualdisk...  ( contains the visual representation of the disks )
	drawingarea_visualdisk .signal_partition_selected .connect( 
			sigc::mem_fun( this, &Win_GParted::on_partition_selected ) ) ;
	drawingarea_visualdisk .signal_partition_activated .connect( 
			sigc::mem_fun( this, &Win_GParted::on_partition_activated ) ) ;
	drawingarea_visualdisk .signal_popup_menu .connect( 
			sigc::mem_fun( this, &Win_GParted::on_partition_popup_menu ) );
	vbox_main .pack_start( drawingarea_visualdisk, Gtk::PACK_SHRINK ) ;
	
	//hpaned_main (NOTE: added to vpaned_main)
	init_hpaned_main() ;
	vpaned_main.set_orientation(Gtk::ORIENTATION_VERTICAL);
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
	pulsebar.set_valign(Gtk::ALIGN_CENTER);
	statusbar.pack_end(pulsebar, true, true, 10);
	statusbar.set_homogeneous();
	vbox_main .pack_start( statusbar, Gtk::PACK_SHRINK );
	
	this ->show_all_children();
	
	//make sure harddisk information is closed..
	hpaned_main .get_child1() ->hide() ;

	add_custom_css();
}

Win_GParted::~Win_GParted()
{
	delete copied_partition;
	copied_partition = nullptr;
}

void Win_GParted::init_menubar() 
{
	Gtk::MenuItem *item;

	//fill menubar_main and connect callbacks 
	//gparted
	menu = Gtk::manage(new Gtk::Menu());
	image = Utils::mk_image(Gtk::Stock::REFRESH, Gtk::ICON_SIZE_MENU);
	item = Gtk::manage(new GParted::Menu_Helpers::ImageMenuElem(
		_("_Refresh Devices"),
		Gtk::AccelKey("<control>r"),
		*image, 
		sigc::mem_fun(*this, &Win_GParted::menu_gparted_refresh_devices)));
	menu->append(*item);

	image = Utils::mk_image(Gtk::Stock::HARDDISK, Gtk::ICON_SIZE_MENU);
	item = Gtk::manage(new GParted::Menu_Helpers::ImageMenuElem(
		_("_Devices"), *image));
	menu->append(*item);
	mainmenu_items[MENU_DEVICES] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu->append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::StockMenuElem(
		Gtk::Stock::QUIT, sigc::mem_fun(*this, &Win_GParted::menu_gparted_quit)));
	menu->append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		_("_GParted"), *menu));
	menubar_main.append(*item);

	//edit
	menu = Gtk::manage(new Gtk::Menu());
	item = Gtk::manage(new GParted::Menu_Helpers::ImageMenuElem(
		_("_Undo Last Operation"), 
		Gtk::AccelKey("<control>z"),
		*Utils::mk_image(Gtk::Stock::UNDO, Gtk::ICON_SIZE_MENU),
		sigc::mem_fun(*this, &Win_GParted::activate_undo)));
	menu->append(*item);
	mainmenu_items[MENU_UNDO_OPERATION] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::ImageMenuElem(
		_("_Clear All Operations"), 
		*Utils::mk_image(Gtk::Stock::CLEAR, Gtk::ICON_SIZE_MENU),
		sigc::mem_fun(*this, &Win_GParted::clear_operationslist)));
	menu->append(*item);
	mainmenu_items[MENU_CLEAR_OPERATIONS] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::ImageMenuElem(
		_("_Apply All Operations"),
		Gtk::AccelKey(GDK_KEY_Return, Gdk::CONTROL_MASK),
		*Utils::mk_image(Gtk::Stock::APPLY, Gtk::ICON_SIZE_MENU),
		sigc::mem_fun(*this, &Win_GParted::activate_apply)));
	menu->append(*item);
	mainmenu_items[MENU_APPLY_OPERATIONS] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		_("_Edit"), *menu));
	menubar_main.append(*item);
	mainmenu_items[MENU_EDIT] = item;

	//view
	menu = Gtk::manage(new Gtk::Menu());

	item = Gtk::manage(new GParted::Menu_Helpers::CheckMenuElem(
		_("Device _Information"), sigc::mem_fun(*this, &Win_GParted::menu_view_harddisk_info)));
	menu->append(*item);
	mainmenu_items[MENU_DEVICE_INFORMATION] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::CheckMenuElem(
		_("Pending _Operations"), sigc::mem_fun(*this, &Win_GParted::menu_view_operations)));
	menu->append(*item);
	mainmenu_items[MENU_PENDING_OPERATIONS] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		_("_View"), *menu));
	menubar_main.append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu->append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		_("_File System Support"), sigc::mem_fun(*this, &Win_GParted::menu_gparted_features)));
	menu->append(*item);
	mainmenu_items[MENU_VIEW] = item;

	//device
	menu = Gtk::manage(new Gtk::Menu());

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		Glib::ustring(_("_Create Partition Table") ) + "...",
		sigc::mem_fun(*this, &Win_GParted::activate_disklabel)));
	menu->append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		_("_Device"), *menu));
	menubar_main.append(*item);
	mainmenu_items[MENU_DEVICE] = item;

	//partition
	init_partition_menu() ;

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		_("_Partition"), menu_partition));
	menubar_main.append(*item);
	mainmenu_items[MENU_PARTITION] = item;

	//help
	menu = Gtk::manage(new Gtk::Menu());

	item = Gtk::manage(new GParted::Menu_Helpers::ImageMenuElem(
		_("_Contents"), 
		Gtk::AccelKey("F1"),
		*Utils::mk_image(Gtk::Stock::HELP, Gtk::ICON_SIZE_MENU),
		sigc::mem_fun(*this, &Win_GParted::menu_help_contents)));
	menu->append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu->append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::StockMenuElem(
		Gtk::Stock::ABOUT, sigc::mem_fun(*this, &Win_GParted::menu_help_about)));
	menu->append(*item);

	item = Gtk::manage(new GParted::Menu_Helpers::MenuElem(
		_("_Help"), *menu));
	menubar_main.append(*item);
}

void Win_GParted::init_toolbar() 
{
	int index = 0 ;
	// Initialize and pack toolbar_main
	hbox_toolbar.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
	hbox_toolbar.pack_start( toolbar_main );

	//NEW and DELETE
	image = Utils::mk_image(Gtk::Stock::NEW, Gtk::ICON_SIZE_BUTTON);
	/*TO TRANSLATORS: "New" is a tool bar item for partition actions. */
	Glib::ustring str_temp = _("New") ;
	toolbutton = Gtk::manage(new Gtk::ToolButton(*image, str_temp));
	toolbutton ->signal_clicked() .connect( sigc::mem_fun( *this, &Win_GParted::activate_new ) );
	toolbar_main .append( *toolbutton );
	TOOLBAR_NEW = index++ ;
	toolbutton->set_tooltip_text(_("Create a new partition in the selected unallocated space"));
	image = Utils::mk_image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_BUTTON);
	str_temp = Utils::get_stock_label(Gtk::Stock::DELETE);
	toolbutton = Gtk::manage(new Gtk::ToolButton(*image, str_temp));
	toolbutton->set_use_underline(true);
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_delete) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_DEL = index++ ;
	toolbutton->set_tooltip_text(_("Delete the selected partition"));
	toolbar_main.append(*(Gtk::manage(new Gtk::SeparatorToolItem)));
	index++ ;

	//RESIZE/MOVE
	image = Utils::mk_image(Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON);
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
	toolbutton = Gtk::manage(new Gtk::ToolButton(*image, str_temp));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_resize) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_RESIZE_MOVE = index++ ;
	toolbutton->set_tooltip_text(_("Resize/Move the selected partition"));
	toolbar_main.append(*(Gtk::manage(new Gtk::SeparatorToolItem)));
	index++ ;

	//COPY and PASTE
	image = Utils::mk_image(Gtk::Stock::COPY, Gtk::ICON_SIZE_BUTTON);
	str_temp = Utils::get_stock_label(Gtk::Stock::COPY);
	toolbutton = Gtk::manage(new Gtk::ToolButton(*image, str_temp));
	toolbutton->set_use_underline(true);
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_copy) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_COPY = index++ ;
	toolbutton->set_tooltip_text(_("Copy the selected partition to the clipboard"));
	image = Utils::mk_image(Gtk::Stock::PASTE, Gtk::ICON_SIZE_BUTTON);
	str_temp = Utils::get_stock_label(Gtk::Stock::PASTE);
	toolbutton = Gtk::manage(new Gtk::ToolButton(*image, str_temp));
	toolbutton->set_use_underline(true);
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_paste) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_PASTE = index++ ;
	toolbutton->set_tooltip_text(_("Paste the partition from the clipboard"));
	toolbar_main.append(*(Gtk::manage(new Gtk::SeparatorToolItem)));
	index++ ;
	
	//UNDO and APPLY
	if ( display_undo ) {
		//Undo button is displayed only if translated language "Resize/Move" is not too long.  See above setting of this condition.
		image = Utils::mk_image(Gtk::Stock::UNDO, Gtk::ICON_SIZE_BUTTON);
		str_temp = Utils::get_stock_label(Gtk::Stock::UNDO);
		toolbutton = Gtk::manage(new Gtk::ToolButton(*image, str_temp));
		toolbutton->set_use_underline(true);
		toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_undo) );
		toolbar_main.append(*toolbutton);
		TOOLBAR_UNDO = index++ ;
		toolbutton ->set_sensitive( false );
		toolbutton->set_tooltip_text(_("Undo Last Operation"));
	}

	image = Utils::mk_image(Gtk::Stock::APPLY, Gtk::ICON_SIZE_BUTTON);
	str_temp = Utils::get_stock_label(Gtk::Stock::APPLY);
	toolbutton = Gtk::manage(new Gtk::ToolButton(*image, str_temp));
	toolbutton->set_use_underline(true);
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_apply) );
	toolbar_main.append(*toolbutton);
	TOOLBAR_APPLY = index++ ;
	toolbutton ->set_sensitive( false );
	toolbutton->set_tooltip_text(_("Apply All Operations"));

	//initialize and pack combo_devices
	liststore_devices = Gtk::ListStore::create(m_treeview_devices_columns);
	combo_devices .set_model( liststore_devices ) ;

	combo_devices.pack_start(m_treeview_devices_columns.icon, false);
	combo_devices.pack_start(m_treeview_devices_columns.device);
	combo_devices.pack_start(m_treeview_devices_columns.size, false);

	combo_devices_changed_connection =
		combo_devices .signal_changed() .connect( sigc::mem_fun(*this, &Win_GParted::combo_devices_changed) );

	hbox_toolbar .pack_start( combo_devices, Gtk::PACK_SHRINK ) ;
}

void Win_GParted::init_partition_menu() 
{
	Gtk::MenuItem *item;

	//fill menu_partition
	image = Utils::mk_image(Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU);
	item = Gtk::manage(new
			/*TO TRANSLATORS: "_New" is a sub menu item for the partition menu. */
			GParted::Menu_Helpers::ImageMenuElem(_("_New"),
							  Gtk::AccelKey(GDK_KEY_Insert, Gdk::BUTTON1_MASK),
							  *image,
							  sigc::mem_fun(*this, &Win_GParted::activate_new)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_NEW] = item;

	item = Gtk::manage(new
			GParted::Menu_Helpers::StockMenuElem(Gtk::Stock::DELETE,
							  Gtk::AccelKey(GDK_KEY_Delete, Gdk::BUTTON1_MASK),
							  sigc::mem_fun(*this, &Win_GParted::activate_delete)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_DEL] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu_partition.append(*item);

	image = Utils::mk_image(Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_MENU);
	item = Gtk::manage(new
			GParted::Menu_Helpers::ImageMenuElem(_("_Resize/Move"),
							  *image, 
							  sigc::mem_fun(*this, &Win_GParted::activate_resize)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_RESIZE_MOVE] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu_partition.append(*item);

	item = Gtk::manage(new
			GParted::Menu_Helpers::StockMenuElem(Gtk::Stock::COPY,
							  sigc::mem_fun(*this, &Win_GParted::activate_copy)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_COPY] = item;

	item = Gtk::manage(new
			GParted::Menu_Helpers::StockMenuElem(Gtk::Stock::PASTE,
							  sigc::mem_fun(*this, &Win_GParted::activate_paste)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_PASTE] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu_partition.append(*item);

	image = Utils::mk_image(Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU);
	item = Gtk::manage(new
			/*TO TRANSLATORS: menuitem which holds a submenu with file systems.. */
			GParted::Menu_Helpers::ImageMenuElem(_("_Format to"),
							  *image,
							  *create_format_menu()));
	menu_partition.append(*item);
	partitionmenu_items[MENU_FORMAT] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu_partition.append(*item);

	item = Gtk::manage(new
			// Placeholder text, replaced in set_valid_operations() before the menu is shown
			GParted::Menu_Helpers::MenuElem("--toggle crypt busy--",
			                             sigc::mem_fun(*this, &Win_GParted::toggle_crypt_busy_state)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_TOGGLE_CRYPT_BUSY] = item;

	item = Gtk::manage(new
			// Placeholder text, replaced in set_valid_operations() before the menu is shown
			GParted::Menu_Helpers::MenuElem("--toggle fs busy--",
						     sigc::mem_fun(*this, &Win_GParted::toggle_fs_busy_state)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_TOGGLE_FS_BUSY] = item;

	item = Gtk::manage(new
			/*TO TRANSLATORS: menuitem which holds a submenu with mount points.. */
			GParted::Menu_Helpers::MenuElem(_("_Mount on"), *Gtk::manage(new Gtk::Menu())));
	menu_partition.append(*item);
	partitionmenu_items[MENU_MOUNT] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu_partition.append(*item);

	item = Gtk::manage(new
			GParted::Menu_Helpers::MenuElem(_("_Name Partition"),
			                             sigc::mem_fun(*this, &Win_GParted::activate_name_partition)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_NAME_PARTITION] = item;

	item = Gtk::manage(new
			GParted::Menu_Helpers::MenuElem(_("M_anage Flags"),
						     sigc::mem_fun(*this, &Win_GParted::activate_manage_flags)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_FLAGS] = item;

	item = Gtk::manage(new
			GParted::Menu_Helpers::MenuElem(_("C_heck"),
						     sigc::mem_fun(*this, &Win_GParted::activate_check)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_CHECK] = item;

	item = Gtk::manage(new
			GParted::Menu_Helpers::MenuElem(_("_Label File System"),
			                             sigc::mem_fun(*this, &Win_GParted::activate_label_filesystem)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_LABEL_FILESYSTEM] = item;

	item = Gtk::manage(new
			GParted::Menu_Helpers::MenuElem(_("New UU_ID"),
						     sigc::mem_fun(*this, &Win_GParted::activate_change_uuid)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_CHANGE_UUID] = item;

	item = Gtk::manage(new GParted::Menu_Helpers::SeparatorElem());
	menu_partition.append(*item);

	item = Gtk::manage(new
			GParted::Menu_Helpers::StockMenuElem(Gtk::Stock::DIALOG_INFO,
							  sigc::mem_fun(*this, &Win_GParted::activate_info)));
	menu_partition.append(*item);
	partitionmenu_items[MENU_INFO] = item;

	menu_partition .accelerate( *this ) ;  
}


//Create the Partition --> Format to --> (file system list) menu
Gtk::Menu * Win_GParted::create_format_menu()
{
	const std::vector<FS> & fss = gparted_core .get_filesystems() ;
	menu = Gtk::manage(new Gtk::Menu());

	for ( unsigned int t = 0 ; t < fss .size() ; t++ )
	{
		if (GParted_Core::supported_filesystem(fss[t].fstype) &&
		    fss[t].fstype != FS_LUKS                            )
			create_format_menu_add_item(fss[t].fstype, fss[t].create);
	}
	//Add cleared at the end of the list
	create_format_menu_add_item( FS_CLEARED, true ) ;

	return menu ;
}


//Add one entry to the Partition --> Format to --> (file system list) menu
void Win_GParted::create_format_menu_add_item(FSType fstype, bool activate)
{
	Gtk::Box* hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
	//the colored square
	hbox->pack_start(*Gtk::manage(new Gtk::Image(Utils::get_color_as_pixbuf(fstype, 16, 16))),
	                 Gtk::PACK_SHRINK);
	//the label...
	hbox->pack_start(*Utils::mk_label(" " + Utils::get_filesystem_string(fstype)),
	                 Gtk::PACK_SHRINK);

	Gtk::MenuItem* item = Gtk::manage(new Gtk::MenuItem(*hbox));
	menu->append(*item);
	if ( activate )
		item->signal_activate().connect(
			sigc::bind<FSType>(sigc::mem_fun(*this, &Win_GParted::activate_format), fstype));
	else
		item->set_sensitive(false);
}


void Win_GParted::init_device_info()
{
	vbox_info.set_orientation(Gtk::ORIENTATION_VERTICAL);
	vbox_info.set_spacing( 5 );
	int top = 0;

	//title
	vbox_info .pack_start( 
		* Utils::mk_label( " <b>" + static_cast<Glib::ustring>( _("Device Information") ) + "</b>" ),
		Gtk::PACK_SHRINK );

	//GENERAL DEVICE INFO
	Gtk::Grid* grid = Gtk::manage(new Gtk::Grid());
	grid->set_column_spacing(10);

	// Model
	Gtk::Label *label_model = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Model:")) + "</b>");
	grid->attach(*label_model, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_model->get_accessible());

	// Serial number
	Gtk::Label *label_serial = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Serial:")) + "</b>");
	grid->attach(*label_serial, 0, top, 1, 1);
	device_info.push_back( Utils::mk_label( "", true, false, true ) );
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_serial->get_accessible());

	// Size
	Gtk::Label *label_size = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Size:")) + "</b>");
	grid->attach(*label_size, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_size->get_accessible());

	// Path
	Gtk::Label *label_path = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Path:")) + "</b>");
	grid->attach(*label_path, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_path->get_accessible());

	vbox_info.pack_start(*grid, Gtk::PACK_SHRINK);

	//DETAILED DEVICE INFO
	top = 0;
	grid = Gtk::manage(new Gtk::Grid());
	grid->set_column_spacing(10);

	// One blank line
	grid->attach(*Utils::mk_label(""), 1, top++, 1, 1);

	// Disktype
	Gtk::Label *label_disktype = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Partition table:")) + "</b>");
	grid->attach(*label_disktype, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_disktype->get_accessible());

	// Heads
	Gtk::Label *label_heads = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Heads:")) + "</b>");
	grid->attach(*label_heads, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_heads->get_accessible());

	// Sectors / track
	Gtk::Label *label_sectors_track = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Sectors/track:")) + "</b>");
	grid->attach(*label_sectors_track, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_sectors_track->get_accessible());

	// Cylinders
	Gtk::Label *label_cylinders = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Cylinders:")) + "</b>");
	grid->attach(*label_cylinders, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_cylinders->get_accessible());

	// Total sectors
	Gtk::Label *label_total_sectors = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Total sectors:")) + "</b>");
	grid->attach(*label_total_sectors, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_total_sectors->get_accessible());

	// Sector size
	Gtk::Label *label_sector_size = Utils::mk_label(" <b>" + static_cast<Glib::ustring>(_("Sector size:")) + "</b>");
	grid->attach(*label_sector_size, 0, top, 1, 1);
	device_info .push_back( Utils::mk_label( "", true, false, true ) ) ;
	grid->attach(*device_info.back(), 1, top++, 1, 1);
	device_info.back()->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                       label_sector_size->get_accessible());

	vbox_info.pack_start(*grid, Gtk::PACK_SHRINK);
}

void Win_GParted::init_hpaned_main() 
{
	hpaned_main.set_orientation(Gtk::ORIENTATION_HORIZONTAL);
	//left scrollwindow (holds device info)
	scrollwindow = Gtk::manage(new Gtk::ScrolledWindow());
	scrollwindow ->set_shadow_type( Gtk::SHADOW_ETCHED_IN );
	scrollwindow ->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
#if HAVE_SET_PROPAGATE_NATURAL_WIDTH
	scrollwindow->set_propagate_natural_width(true);
#endif

	hpaned_main .pack1( *scrollwindow, true, true );
	scrollwindow ->add( vbox_info );

	//right scrollwindow (holds treeview with partitions)
	scrollwindow = Gtk::manage(new Gtk::ScrolledWindow());
	scrollwindow ->set_shadow_type( Gtk::SHADOW_ETCHED_IN );
	scrollwindow ->set_policy( Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC );
#if HAVE_SET_PROPAGATE_NATURAL_WIDTH
	scrollwindow->set_propagate_natural_width(true);
#endif

	//connect signals and add treeview_detail
	treeview_detail .signal_partition_selected .connect( sigc::mem_fun( this, &Win_GParted::on_partition_selected ) );
	treeview_detail .signal_partition_activated .connect( sigc::mem_fun( this, &Win_GParted::on_partition_activated ) );
	treeview_detail .signal_popup_menu .connect( sigc::mem_fun( this, &Win_GParted::on_partition_popup_menu ) );
	scrollwindow ->add( treeview_detail );
	hpaned_main .pack2( *scrollwindow, true, true );
}

void Win_GParted::add_custom_css()
{
	Glib::RefPtr<Gdk::Screen> default_screen = Gdk::Screen::get_default();
	Glib::RefPtr<Gtk::CssProvider> provider = Gtk::CssProvider::create();

	Glib::ustring custom_css;
	if (gtk_get_minor_version() >= 20)
		custom_css = "progressbar progress, trough { min-height: 8px; }";
	else
		custom_css = "GtkProgressBar { -GtkProgressBar-min-horizontal-bar-height: 8px; }";

	try
	{
		provider->load_from_data(custom_css);
	}
	catch (Glib::Error& e)
	{
		std::cerr << Utils::convert_ustring(e.what()) << std::endl;
	}

	Gtk::StyleContext::add_provider_for_screen(default_screen,
	                                           provider,
	                                           GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

void Win_GParted::refresh_combo_devices()
{
	// Temporarily block the on change callback while re-creating the device list
	// behind the combobox to prevent flashing redraw by being redrawn with an empty
	// device list.
	combo_devices_changed_connection .block();
	liststore_devices ->clear() ;

	menu = Gtk::manage(new Gtk::Menu());
	Gtk::RadioButtonGroup radio_group ;

	for (unsigned int i = 0; i < m_devices.size(); i++)
	{
		//combo...
		treerow = *( liststore_devices ->append() ) ;
		treerow[m_treeview_devices_columns.icon] =
		                Utils::mk_pixbuf(*this, Gtk::Stock::HARDDISK, Gtk::ICON_SIZE_LARGE_TOOLBAR);
		treerow[m_treeview_devices_columns.device] = m_devices[i].get_path();
		treerow[m_treeview_devices_columns.size] =
		                "(" + Utils::format_size(m_devices[i].length, m_devices[i].sector_size) + ")";

		// Devices submenu...
		Gtk::Box* hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
		hbox->pack_start(*Utils::mk_label(m_devices[i].get_path()), Gtk::PACK_EXPAND_WIDGET);
		hbox->pack_start(*Utils::mk_label("   (" + Utils::format_size(m_devices[i].length,
		                                                              m_devices[i].sector_size) + ")"),
		                 Gtk::PACK_SHRINK);

		Gtk::RadioMenuItem* item = Gtk::manage(new Gtk::RadioMenuItem(radio_group));
		menu->append(*item);
		item->add(*hbox);
		item->signal_activate().connect(
			sigc::bind<unsigned int>( sigc::mem_fun(*this, &Win_GParted::radio_devices_changed), i ) ) ;
	}

	mainmenu_items[MENU_DEVICES]->unset_submenu();

	if (menu->get_children().size())
	{
		menu ->show_all() ;
		mainmenu_items[MENU_DEVICES]->set_submenu(*menu);
	}

	combo_devices_changed_connection .unblock();
	combo_devices.set_active(m_current_device);
}


bool Win_GParted::pulsebar_pulse()
{
	pulsebar.pulse();
	Glib::ustring tmp_msg = gparted_core .get_thread_status_message() ;
	if ( tmp_msg != "" ) {
		statusbar.pop();
		statusbar.push( tmp_msg );
	}

	return true;
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
		
	// connect pulse update timer
	pulsetimer = Glib::signal_timeout().connect( sigc::mem_fun(*this, &Win_GParted::pulsebar_pulse), 100 );
}

void Win_GParted::hide_pulsebar()
{
	pulsetimer.disconnect();
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
		device_info[t++]->set_text(m_devices[m_current_device].model);
		device_info[t++]->set_text(m_devices[m_current_device].serial_number);
		device_info[t++]->set_text(Utils::format_size(m_devices[m_current_device].length,
		                                              m_devices[m_current_device].sector_size));
		device_info[t++]->set_text(m_devices[m_current_device].get_path());

		//detailed info
		device_info[t++]->set_text(m_devices[m_current_device].disktype);
		device_info[t++]->set_text(Utils::num_to_str(m_devices[m_current_device].heads));
		device_info[t++]->set_text(Utils::num_to_str(m_devices[m_current_device].sectors));
		device_info[t++]->set_text(Utils::num_to_str(m_devices[m_current_device].cylinders));
		device_info[t++]->set_text(Utils::num_to_str(m_devices[m_current_device].length));
		device_info[t++]->set_text(Utils::num_to_str(m_devices[m_current_device].sector_size));
	}
}


bool Win_GParted::on_delete_event( GdkEventAny *event )
{
	return ! Quit_Check_Operations();
}	


void Win_GParted::add_operation(const Device& device, std::unique_ptr<Operation> operation)
{
	if (nullptr == operation)
		return;

	// For operations which create new or modify existing partition boundaries ensure
	// those boundaries are valid before allowing the operation to be added.
	if (operation->m_type == OPERATION_CREATE      ||
	    operation->m_type == OPERATION_COPY        ||
	    operation->m_type == OPERATION_RESIZE_MOVE   )
	{
		Glib::ustring error;
		if (! gparted_core.valid_partition(device, operation->get_partition_new(), error))
		{
			Gtk::MessageDialog dialog(*this,
			                _("Could not add this operation to the list"),
			                false,
			                Gtk::MESSAGE_ERROR,
			                Gtk::BUTTONS_OK,
			                true);
			dialog.set_secondary_text(error);
			dialog.run();
			return;
		}
	}

	operation->create_description();

	if (merge_operation(*operation))
		// The operation was merged with an existing one already in the list so
		// there's no need to append it to the list.
		return;

	m_operations.push_back(std::move(operation));
}


// Try to merge the candidate operation with a previous operation affecting the same
// partition(s) in the m_operations[] vector.
bool Win_GParted::merge_operation(const Operation& candidate)
{
	// Find previous operation affecting the same partition(s).  Stop searching and
	// attempt merge.
	// WARNING:
	// Index into m_operations[] vector counts backwards stopping at -1, hence needing
	// to use signed int.
	for (signed int i = static_cast<signed int>(m_operations.size())-1; i >= 0; i--)
	{
		if (operations_affect_same_partition(*m_operations[i], candidate))
		{
			return m_operations[i]->merge_operations(candidate);
		}
	}
	return false;
}


// Report whether the second candidate operation affects the same partition(s) as the
// first target operation.
bool Win_GParted::operations_affect_same_partition(const Operation& first_op, const Operation& second_op)
{
	if (first_op.m_type == OPERATION_DELETE)
		// First target operation is deleting the partition so there is no
		// partition to merge the second candidate operation into.
		return false;

	if (first_op.get_partition_new().get_path() == second_op.get_partition_original().get_path())
		return true;

	// A copy operation is considered to affect both the copied source and pasted
	// target partitions, to prevent merging of operations past either.  Two copy
	// operations have four partition combinations to check.  The one above covering
	// all operations and three more below.
	Glib::ustring first_copied_path;
	if (first_op.m_type == OPERATION_COPY)
	{
		const OperationCopy& first_copy_op = static_cast<const OperationCopy&>(first_op);
		first_copied_path = first_copy_op.get_partition_copied().get_path();

		if (first_copied_path == second_op.get_partition_original().get_path())
			return true;
	}

	Glib::ustring second_copied_path;
	if (second_op.m_type == OPERATION_COPY)
	{
		const OperationCopy& second_copy_op = static_cast<const OperationCopy&>(second_op);
		second_copied_path = second_copy_op.get_partition_copied().get_path();

		if (first_op.get_partition_new().get_path() == second_copied_path)
			return true;
	}

	if (first_op.m_type == OPERATION_COPY && second_op.m_type == OPERATION_COPY)
	{
		if (first_copied_path == second_copied_path)
			return true;
	}

	return false;
}


void Win_GParted::Refresh_Visual()
{
	// How GParted displays partitions in the GUI and manages the lifetime and
	// ownership of that data:
	//
	// (1) Queries the devices and partitions on disk and refreshes the model.
	//
	//     Data owner: std::vector<Devices> Win_GParted::devices
	//     Lifetime:   Valid until the next call to Refresh_Visual()
	//     Call chain:
	//
	//         Win_GParted::menu_gparted_refresh_devices()
	//             gparted_core.set_devices( devices )
	//                 GParted_Core::set_devices_thread( devices )
	//                     devices.clear()
	//                     etc.
	//
	// (2) Takes a copy of the device and partitions for the device currently being
	//     shown in the GUI and visually applies pending operations.
	//
	//     Data owner: Device Win_GParted::m_display_device
	//     Lifetime:   Valid until the next call to Refresh_Visual().
	//     Function:   Refresh_Visual()
	//
	// (3) Loads the disk graphic and partition list with partitions to be shown in
	//     the GUI.  Both classes store pointers pointing back to each partition
	//     object in the current display device's vector of partitions.
	//
	//     Aliases:   Win_GParted::m_display_device.partitions[]
	//     Call chain:
	//
	//         Win_GParted::Refresh_Visual()
	//             drawingarea_visualdisk.load_partitions(m_display_device.partitions, device_sectors)
	//                 DrawingAreaVisualDisk::set_static_data( ... )
	//             treeview_detail.load_partitions(m_display_device.partitions)
	//                 TreeView_Detail::create_row()
	//                 TreeView_Detail::load_partitions()
	//                     TreeView_Detail::create_row()
	//
	// (4) Selecting a partition in the disk graphic or in the partition list fires
	//     the callback which passes a pointer to the selected partition stored in
	//     step (3).  The callback saves the selected partition and calls the opposite
	//     class to update it's selection.
	//
	//     Data owner: const Partition * Win_GParted::selected_partition_ptr
	//     Aliases:    Win_GParted::m_display_device.partitions[]
	//     Lifetime:   Valid until the next call to Refresh_Visual().
	//     Call chain: (example clicking on a partition in the disk graphic)
	//
	//         DrawingAreaVisualDisk::on_button_press_event()
	//             DrawingAreaVisualDisk::set_selected( visual_partitions, x, y )
	//                 signal_partition_selected.emit( ..., false )
	//                     Win_GParted::on_partition_selected( partition_ptr, src_is_treeview )
	//                         treeview_detail.set_selected( treestore_detail->children(), partition_ptr )
	//                             TreeView::set_selected( rows, partition_ptr, inside_extended )
	//                                 set_cursor()
	//                                 TreeView::set_selected( rows, partition_ptr, true )
	//                                     set_cursor()
	//
	// (5) Each new operation is added to the vector of pending operations.
	//     Eventually Refresh_Visual() is call to update the GUI.  This goes to step
	//     (2) which visually reapplies all pending operations again, including the
	//     newly added operation.
	//
	//     Data owner: std::vector<Operation *> Win_GParted::operations
	//     Lifetime:   Valid until operations have been applied by
	//                 GParted_Core::apply_operation_to_disk().  Specifically longer
	//                 than the next call call to Refresh_Visual().
	//     Call chain: (example setting a file system label)
	//
	//         Win_GParted::activate_label_filesystem()
	//             Win_GParted::add_operation(m_devices[m_current_device], operation)
	//             Win_GParted::show_operationslist()
	//                 Win_GParted::Refresh_Visual()
	//
	// (6) Selecting a partition as a copy source makes a copy of that partition
	//     object.
	//
	//     Data owner: Partition Win_GParted::copied_partition
	//     Lifetime:   Valid until GParted closed or the device is removed.
	//                 Specifically longer than the next call to Refresh_Visual().
	//     Function:   Win_GParted::activate_copy()


	//make all operations visible
	m_display_device = m_devices[m_current_device];
	for (unsigned int i = 0; i < m_operations.size(); i++)
		if (m_operations[i]->m_device == m_display_device)
			m_operations[i]->apply_to_visual(m_display_device.partitions);

	hbox_operations.load_operations(m_operations);

	//set new statusbartext
	statusbar .pop() ;
	statusbar .push( Glib::ustring::compose( ngettext( "%1 operation pending"
	                                           , "%1 operations pending"
	                                           , m_operations.size()
	                                           )
	                                 , m_operations.size()
	                                 )
	               );
		
	if (! m_operations.size())
		allow_undo_clear_apply( false ) ;

	// Refresh copy partition source as necessary and select the largest unallocated
	// partition if there is one.
	selected_partition_ptr = nullptr;
	Sector largest_unalloc_size = -1 ;
	Sector current_size ;

	for (unsigned int i = 0; i < m_display_device.partitions.size(); i++)
	{
		if (copied_partition != nullptr && m_display_device.partitions[i].get_path() == copied_partition->get_path())
		{
			delete copied_partition;
			copied_partition = m_display_device.partitions[i].clone();
		}

		if (m_display_device.partitions[i].fstype == FS_UNALLOCATED)
		{
			current_size = m_display_device.partitions[i].get_sector_length();
			if (current_size > largest_unalloc_size)
			{
				largest_unalloc_size = current_size;
				selected_partition_ptr = & m_display_device.partitions[i];
			}
		}

		if (m_display_device.partitions[i].type == TYPE_EXTENDED)
		{
			for (unsigned int j = 0; j < m_display_device.partitions[i].logicals.size(); j++)
			{
				if (copied_partition != nullptr &&
				    m_display_device.partitions[i].logicals[j].get_path() == copied_partition->get_path())
				{
					delete copied_partition;
					copied_partition = m_display_device.partitions[i].logicals[j].clone();
				}

				if (m_display_device.partitions[i].logicals[j].fstype == FS_UNALLOCATED)
				{
					current_size = m_display_device.partitions[i].logicals[j].get_sector_length();
					if ( current_size > largest_unalloc_size )
					{
						largest_unalloc_size = current_size;
						selected_partition_ptr = & m_display_device.partitions[i].logicals[j];
					}
				}
			}
		}
	}

	// frame visualdisk
	drawingarea_visualdisk.load_partitions(m_display_device.partitions, m_display_device.length);

	// treeview details
	treeview_detail.load_partitions(m_display_device.partitions);

	set_valid_operations() ;

	if ( largest_unalloc_size >= 0 )
	{
		// Inform visuals of selection of the largest unallocated partition.
		drawingarea_visualdisk.set_selected( selected_partition_ptr );
		treeview_detail.set_selected( selected_partition_ptr );
	}

	// Process Gtk events to redraw visuals with reloaded partition details.
	while (Gtk::Main::events_pending())
		Gtk::Main::iteration();

}


// Confirms that the pointer points to one of the partition objects in the vector of
// displayed partitions, Win_GParted::m_display_device.partitions[].
// Usage: g_assert(valid_display_partition_ptr(my_partition_ptr));
bool Win_GParted::valid_display_partition_ptr( const Partition * partition_ptr )
{
	for (unsigned int i = 0; i < m_display_device.partitions.size();i++)
	{
		if (& m_display_device.partitions[i] == partition_ptr)
			return true;
		else if (m_display_device.partitions[i].type == TYPE_EXTENDED)
		{
			for (unsigned int j = 0; j < m_display_device.partitions[i].logicals.size(); j++)
			{
				if (& m_display_device.partitions[i].logicals[j] == partition_ptr)
					return true;
			}
		}
	}
	return false;
}


bool Win_GParted::Quit_Check_Operations()
{
	if (m_operations.size())
	{
		Gtk::MessageDialog dialog( *this,
					   _("Quit GParted?"),
					   false,
					   Gtk::MESSAGE_QUESTION,
					   Gtk::BUTTONS_NONE,
					   true );

		dialog .set_secondary_text( Glib::ustring::compose( ngettext( "%1 operation is currently pending."
		                                                      , "%1 operations are currently pending."
		                                                      , m_operations.size()
		                                                      )
		                                            , m_operations.size()
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
	allow_paste( false ); allow_format( false );
	allow_toggle_crypt_busy_state( false ); allow_toggle_fs_busy_state( false );
	allow_name_partition( false ); allow_manage_flags( false ); allow_check( false );
	allow_label_filesystem( false ); allow_change_uuid( false ); allow_info( false );

	// Set default name for the open/close crypt menu item.
	const FileSystem * luks_filesystem_object = gparted_core.get_filesystem_object( FS_LUKS );
	g_assert(luks_filesystem_object != nullptr);  // Bug: LUKS FileSystem object not found
	dynamic_cast<Gtk::Label *>(partitionmenu_items[MENU_TOGGLE_CRYPT_BUSY]->get_child())
		->set_label( luks_filesystem_object->get_custom_text( CTEXT_ACTIVATE_FILESYSTEM ) );
	// Set default name for the file system active/deactivate menu item.
	dynamic_cast<Gtk::Label *>(partitionmenu_items[MENU_TOGGLE_FS_BUSY]->get_child())
		->set_label( FileSystem::get_generic_text( CTEXT_ACTIVATE_FILESYSTEM ) );

	partitionmenu_items[MENU_TOGGLE_FS_BUSY]->show();
	partitionmenu_items[MENU_MOUNT]->hide();

	// No partition selected ...
	if ( ! selected_partition_ptr )
		return ;
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	// Reference to the Partition object directly containing the file system.
	const Partition & selected_filesystem = selected_partition_ptr->get_filesystem_partition();

	// Get file system and LUKS encryption capabilities
	const FS& fs_cap = gparted_core.get_fs(selected_filesystem.fstype);
	const FS & enc_cap = gparted_core.get_fs( FS_LUKS );

	//if there's something, there's some info ;)
	allow_info( true ) ;

	// Set appropriate name for the open/close crypt menu item.
	if (selected_partition_ptr->fstype == FS_LUKS)
	{
		dynamic_cast<Gtk::Label *>(partitionmenu_items[MENU_TOGGLE_CRYPT_BUSY]->get_child())
			->set_label( luks_filesystem_object->get_custom_text(   selected_partition_ptr->busy
			                                                      ? CTEXT_DEACTIVATE_FILESYSTEM
			                                                      : CTEXT_ACTIVATE_FILESYSTEM ) );
	}
	// Set appropriate name for the file system active/deactivate menu item.
	if (selected_partition_ptr->fstype != FS_LUKS || selected_partition_ptr->busy)
	{
		const FileSystem *filesystem_object = gparted_core.get_filesystem_object(selected_filesystem.fstype);
		if ( filesystem_object )
		{
			dynamic_cast<Gtk::Label *>(partitionmenu_items[MENU_TOGGLE_FS_BUSY]->get_child())
				->set_label( filesystem_object->get_custom_text(   selected_filesystem.busy
				                                                 ? CTEXT_DEACTIVATE_FILESYSTEM
				                                                 : CTEXT_ACTIVATE_FILESYSTEM ) );
		}
		else
		{
			dynamic_cast<Gtk::Label *>(partitionmenu_items[MENU_TOGGLE_FS_BUSY]->get_child())
				->set_label( FileSystem::get_generic_text (  selected_filesystem.busy
				                                           ? CTEXT_DEACTIVATE_FILESYSTEM
				                                           : CTEXT_ACTIVATE_FILESYSTEM ) );
		}
	}

	// Only permit encryption open/close when available
	if (selected_partition_ptr->status == STAT_REAL &&
	    selected_partition_ptr->fstype == FS_LUKS   &&
	    ! selected_filesystem.busy                    )
		allow_toggle_crypt_busy_state( true );

	// Only permit file system mount/unmount and swapon/swapoff when available
	if (    selected_partition_ptr->status == STAT_REAL
	     && selected_partition_ptr->type   != TYPE_EXTENDED
	     && selected_filesystem.fstype     != FS_LVM2_PV
	     && selected_filesystem.fstype     != FS_LINUX_SWRAID
	     && selected_filesystem.fstype     != FS_ATARAID
	     && selected_filesystem.fstype     != FS_LUKS
	     && selected_filesystem.fstype     != FS_BCACHE
	     && selected_filesystem.fstype     != FS_JBD
	     && (    selected_filesystem.busy
	          || selected_filesystem.get_mountpoints().size() /* Have mount point(s) */
	          || selected_filesystem.fstype == FS_LINUX_SWAP
	        )
	   )
		allow_toggle_fs_busy_state( true );

	// Only permit LVM VG activate/deactivate if the PV is busy or a member of a VG.
	// For now specifically allow activation of an exported VG, which LVM will fail
	// with "Volume group "VGNAME" is exported", otherwise user won't know why the
	// inactive PV can't be activated.
	if (    selected_partition_ptr->status == STAT_REAL
	     && selected_filesystem.fstype     == FS_LVM2_PV      // Active VGNAME from mount point
	     && ( selected_filesystem.busy || selected_filesystem.get_mountpoints().size() > 0 )
	   )
		allow_toggle_fs_busy_state( true );

	// Allow partition naming on devices that support it
	if (selected_partition_ptr->status == STAT_REAL              &&
	    m_devices[m_current_device].partition_naming_supported() &&
	    (selected_partition_ptr->type == TYPE_PRIMARY  ||
	     selected_partition_ptr->type == TYPE_LOGICAL  ||
	     selected_partition_ptr->type == TYPE_EXTENDED   )         )
		allow_name_partition( true );

	// Allow partition flag management
	if ( selected_partition_ptr->status == STAT_REAL          &&
	     ( selected_partition_ptr->type == TYPE_PRIMARY  ||
	       selected_partition_ptr->type == TYPE_LOGICAL  ||
	       selected_partition_ptr->type == TYPE_EXTENDED    )    )
		allow_manage_flags( true );

	// Online resizing always required the ability to update the partition table ...
	if (! m_devices[m_current_device].readonly &&
	    selected_filesystem.busy                 )
	{
		// Can the plain file system be online resized?
		if (selected_partition_ptr->fstype != FS_LUKS    &&
		    (fs_cap.online_grow || fs_cap.online_shrink)   )
			allow_resize( true );
		// Is resizing an open LUKS mapping and the online file system within
		// supported?
		if ( selected_partition_ptr->fstype == FS_LUKS                &&
		     selected_partition_ptr->busy                             &&
		     ( ( enc_cap.online_grow && fs_cap.online_grow )     ||
		       ( enc_cap.online_shrink && fs_cap.online_shrink )    )    )
			allow_resize( true );
		// Always allow an extended partition to be resized online.
		if (selected_partition_ptr->type == TYPE_EXTENDED)
			allow_resize(true);
	}

	// Allow labelling of mounted file systems that support it.
	if (selected_filesystem.busy                    &&
	    selected_partition_ptr->status == STAT_REAL &&
	    fs_cap.online_write_label                     )
	{
		allow_label_filesystem(true);
	}

	// Only unmount/swapoff/VG deactivate or online actions allowed if busy
	if ( selected_filesystem.busy )
		return ;

	// UNALLOCATED space within a partition table or UNALLOCATED whole disk device
	if (selected_partition_ptr->fstype == FS_UNALLOCATED)
		allow_new( true );

	// UNALLOCATED space within a partition table
	if ( selected_partition_ptr->type == TYPE_UNALLOCATED )
	{
		// Find out if there is a partition to be copied and if it fits inside this unallocated space
		// FIXME:
		// Temporarily disable copying of encrypted content into new partitions
		// which can't yet be encrypted, until full LUKS read-write support is
		// implemented.
		if (copied_partition             != nullptr   &&
		    ! m_devices[m_current_device].readonly    &&
		    copied_partition->fstype     != FS_LUKS     )
		{
			const Partition & copied_filesystem_ptn = copied_partition->get_filesystem_partition();
			Byte_Value required_size ;
			if (copied_filesystem_ptn.fstype == FS_XFS)
				required_size = copied_filesystem_ptn.estimated_min_size() *
				                copied_filesystem_ptn.sector_size;
			else
				required_size = copied_filesystem_ptn.get_byte_length();

			// Determine if space needs reserving for the partition table or the EBR (Extended
			// Boot Record).  Generally a track or MEBIBYTE is reserved.  For our purposes
			// reserve a MEBIBYTE at the start of the partition.
			// NOTE: This logic also contained in Dialog_Base_Partition::MB_Needed_for_Boot_Record()
			if (selected_partition_ptr->inside_extended                                               ||
			    selected_partition_ptr->sector_start < MEBIBYTE / selected_partition_ptr->sector_size   )
				required_size += MEBIBYTE;

			//Determine if space is needed for the Extended Boot Record for a logical partition
			//  after this partition.  Generally an an additional track or MEBIBYTE
			//  is required so for our purposes reserve a MEBIBYTE in front of the partition.
			if (   (   (   selected_partition_ptr->inside_extended
			            && selected_partition_ptr->type == TYPE_UNALLOCATED
			           )
			        || ( selected_partition_ptr->type == TYPE_LOGICAL )
			       )
			    && ( selected_partition_ptr->sector_end
			         < ( m_devices[m_current_device].length
			             - ( 2 * MEBIBYTE / m_devices[m_current_device].sector_size )
			           )
			       )
			   )
				required_size += MEBIBYTE;

			//Determine if space is needed for the backup partition on a GPT partition table
			if (   ( m_devices[m_current_device].disktype == "gpt" )
			    && ( ( m_devices[m_current_device].length - selected_partition_ptr->sector_end )
			         < ( MEBIBYTE / m_devices[m_current_device].sector_size )
			       )
			   )
				required_size += MEBIBYTE ;

			if ( required_size <= selected_partition_ptr->get_byte_length() )
				allow_paste( true ) ;
		}
		
		return ;
	}
	
	// EXTENDED
	if ( selected_partition_ptr->type == TYPE_EXTENDED )
	{
		// Deletion is only allowed when there are no logical partitions inside.
		if ( selected_partition_ptr->logicals.size()      == 1                &&
		     selected_partition_ptr->logicals.back().type == TYPE_UNALLOCATED    )
			allow_delete( true ) ;
		
		if (! m_devices[m_current_device].readonly)
			allow_resize( true ) ; 

		return ;
	}	
	
	// PRIMARY, LOGICAL and UNPARTITIONED; partitions with supported file system.
	if ( ( selected_partition_ptr->type == TYPE_PRIMARY       ||
	       selected_partition_ptr->type == TYPE_LOGICAL       ||
	       selected_partition_ptr->type == TYPE_UNPARTITIONED    ) &&
	     selected_partition_ptr->fstype != FS_UNALLOCATED             )
	{
		allow_format( true ) ;

		// Only allow deletion of inactive partitions within a partition table.
		if ( ( selected_partition_ptr->type == TYPE_PRIMARY ||
		       selected_partition_ptr->type == TYPE_LOGICAL    ) &&
		     ! selected_partition_ptr->busy                         )
			allow_delete( true );

		// Resizing/moving always requires the ability to update the partition
		// table ...
		if (! m_devices[m_current_device].readonly)
		{
			// Can the plain file system be resized or moved?
			if (selected_partition_ptr->fstype != FS_LUKS     &&
			    (fs_cap.grow || fs_cap.shrink || fs_cap.move)   )
				allow_resize( true );
			// Is growing or moving this closed LUKS mapping permitted?
			if (selected_partition_ptr->fstype == FS_LUKS &&
			    ! selected_partition_ptr->busy            &&
			    (enc_cap.grow || enc_cap.move)              )
				allow_resize( true );
			// Is resizing an open LUKS mapping and the file system within
			// supported?
			if ( selected_partition_ptr->fstype == FS_LUKS         &&
			     selected_partition_ptr->busy                      &&
	                     ( ( enc_cap.online_grow && fs_cap.grow )     ||
			       ( enc_cap.online_shrink && fs_cap.shrink )    )    )
				allow_resize( true );
		}

		// Only allow copying of real partitions, excluding closed encryption
		// (which are only copied while open).
		if (selected_partition_ptr->status == STAT_REAL &&
		    selected_filesystem.fstype     != FS_LUKS   &&
		    fs_cap.copy                                   )
			allow_copy( true ) ;
		
		//only allow labelling of real partitions that support labelling
		if ( selected_partition_ptr->status == STAT_REAL && fs_cap.write_label )
			allow_label_filesystem( true );

		//only allow changing UUID of real partitions that support it
		if ( selected_partition_ptr->status == STAT_REAL && fs_cap.write_uuid )
			allow_change_uuid( true ) ;

		// Generate Mount on submenu, except for LVM2 PVs borrowing mount point to
		// display the VGNAME and read-only supported LUKS.
		if (selected_filesystem.fstype != FS_LVM2_PV     &&
		    selected_filesystem.fstype != FS_LUKS        &&
		    selected_filesystem.get_mountpoints().size()   )
		{
			partitionmenu_items[MENU_MOUNT]->unset_submenu();

			Gtk::Menu* menu = Gtk::manage(new Gtk::Menu());
			const std::vector<Glib::ustring>& temp_mountpoints = selected_filesystem.get_mountpoints();
			for ( unsigned int t = 0 ; t < temp_mountpoints.size() ; t++ )
			{
				Gtk::MenuItem *item;

				item = Gtk::manage(new
					GParted::Menu_Helpers::MenuElem(
						temp_mountpoints[t],
						sigc::bind<unsigned int>(sigc::mem_fun(*this, &Win_GParted::activate_mount_partition), t)));
				menu->append(*item);

				dynamic_cast<Gtk::Label *>(item->get_child())->set_use_underline(false);
			}
			partitionmenu_items[MENU_MOUNT]->set_submenu(*menu);

			partitionmenu_items[MENU_TOGGLE_FS_BUSY]->hide();
			partitionmenu_items[MENU_MOUNT]->show();
		}

		// See if there is a partition to be copied and it fits inside this selected partition
		if ( copied_partition != nullptr                                           &&
		     ( copied_partition->get_filesystem_partition().get_byte_length() <=
		       selected_filesystem.get_byte_length()                             ) &&
		     selected_partition_ptr->status == STAT_REAL                           &&
		     *copied_partition != *selected_partition_ptr                             )
			allow_paste( true );

		//see if we can somehow check/repair this file system....
		if ( selected_partition_ptr->status == STAT_REAL && fs_cap.check )
			allow_check( true ) ;
	}
}


void Win_GParted::show_operationslist()
{
	//Enable (or disable) Undo and Apply buttons
	allow_undo_clear_apply(m_operations.size());

	//Updates view of operations list and sensitivity of Undo and Apply buttons
	Refresh_Visual();

	if (m_operations.size() == 1)  // first operation, open operationslist
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


void Win_GParted::open_operationslist() 
{
	if (! m_operationslist_open)
	{
		m_operationslist_open = true;
		hbox_operations .show() ;

		for ( int t = vpaned_main .get_height() ; t > ( vpaned_main .get_height() - 100 ) ; t -= 5 )
		{
			vpaned_main .set_position( t );
			while ( Gtk::Main::events_pending() ) 
				Gtk::Main::iteration() ;
		}

		static_cast<Gtk::CheckMenuItem *>(mainmenu_items[MENU_PENDING_OPERATIONS])
			->set_active( true ) ;
	}
}


void Win_GParted::close_operationslist() 
{
	if (m_operationslist_open)
	{
		m_operationslist_open = false;

		for ( int t = vpaned_main .get_position() ; t < vpaned_main .get_height() ; t += 5 )
		{
			vpaned_main .set_position( t ) ;
		
			while ( Gtk::Main::events_pending() )
				Gtk::Main::iteration();
		}
		
		hbox_operations .hide() ;

		static_cast<Gtk::CheckMenuItem *>(mainmenu_items[MENU_PENDING_OPERATIONS])
			->set_active( false ) ;
	}
}


void Win_GParted::clear_operationslist() 
{
	remove_operation(REMOVE_ALL);
	close_operationslist() ;

	Refresh_Visual() ;
}


void Win_GParted::combo_devices_changed()
{
	unsigned int old_current_device = m_current_device;
	//set new current device
	m_current_device = combo_devices .get_active_row_number() ;
	if (m_current_device == (unsigned int)-1)
		m_current_device = old_current_device;
	if (m_current_device >= m_devices.size())
		m_current_device = 0;
	set_title(Glib::ustring::compose(_("%1 - GParted"), m_devices[m_current_device].get_path()));

	//refresh label_device_info
	Fill_Label_Device_Info();
	
	//rebuild visualdisk and treeview
	Refresh_Visual();
	
	// Update radio buttons..
	if (mainmenu_items[MENU_DEVICES]->has_submenu())
	{
		static_cast<Gtk::RadioMenuItem *>
		(mainmenu_items[MENU_DEVICES]->get_submenu()->get_children()[m_current_device])
			->set_active(true);
	}
}


void Win_GParted::radio_devices_changed( unsigned int item ) 
{
	if (static_cast<Gtk::RadioMenuItem *>
	    (mainmenu_items[MENU_DEVICES]->get_submenu()->get_children()[item])->get_active())
	{
		combo_devices .set_active( item ) ;
	}
}

void Win_GParted::on_show()
{
	Gtk::Window::on_show() ;
	
	vpaned_main .set_position( vpaned_main .get_height() ) ;
	close_operationslist() ;

	// Register callback for as soon as the main window has been shown to perform the
	// first load of the disk device details.  Do it this way because the Gtk  main
	// loop doesn't seem to enable quit handling until on_show(), this function, has
	// drawn the main window for the first time and returned, and we want Close Window
	// [Alt-F4] to work during the initial load of the disk device details.
	g_idle_add( initial_device_refresh, this );
}

// Callback used to load the disk device details for the first time
gboolean Win_GParted::initial_device_refresh( gpointer data )
{
	Win_GParted *win_gparted = static_cast<Win_GParted *>( data );
	win_gparted->menu_gparted_refresh_devices();
	return false;  // one shot g_idle_add() callback
}


void Win_GParted::menu_gparted_refresh_devices()
{
	show_pulsebar( _("Scanning all devices...") ) ;
	gparted_core.set_devices(m_devices);
	hide_pulsebar();

	// Check if m_current_device is still available (think about hotpluggable stuff like USB devices)
	if (m_current_device >= m_devices.size())
		m_current_device = 0;

	//see if there are any pending operations on non-existent devices
	//NOTE that this isn't 100% foolproof since some stuff (e.g. sourcedevice of copy) may slip through.
	//but anyone who removes the sourcedevice before applying the operations gets what he/she deserves :-)
	//FIXME: this actually sucks ;) see if we can use STL predicates here..
	unsigned int i ;
	for (unsigned int t = 0; t < m_operations.size(); t++)
	{
		for (i = 0; i < m_devices.size() && m_devices[i] != m_operations[t]->m_device; i++)
		{}

		if (i >= m_devices.size())
			remove_operation(REMOVE_AT, t--);
	}

	//if no devices were detected we disable some stuff and show a message in the statusbar
	if (m_devices.empty())
	{
		this ->set_title( _("GParted") );
		combo_devices .hide() ;
		
		mainmenu_items[MENU_DEVICES]->set_sensitive(false);
		mainmenu_items[MENU_EDIT]->set_sensitive(false);
		mainmenu_items[MENU_VIEW]->set_sensitive(false);
		mainmenu_items[MENU_DEVICE]->set_sensitive(false);
		mainmenu_items[MENU_PARTITION]->set_sensitive(false);
		toolbar_main .set_sensitive( false ) ;
		drawingarea_visualdisk .set_sensitive( false ) ;
		treeview_detail .set_sensitive( false ) ;

		Fill_Label_Device_Info( true ) ;
		
		drawingarea_visualdisk .clear() ;
		treeview_detail .clear() ;
		
		//hmzz, this is really paranoid, but i think it's the right thing to do ;)
		hbox_operations .clear() ;
		close_operationslist() ;
		remove_operation(REMOVE_ALL);

		statusbar .pop() ;
		statusbar .push( _( "No devices detected" ) );
	}
	else //at least one device detected
	{
		combo_devices .show() ;
		
		mainmenu_items[MENU_DEVICES]->set_sensitive(true);
		mainmenu_items[MENU_EDIT]->set_sensitive(true);
		mainmenu_items[MENU_VIEW]->set_sensitive(true);
		mainmenu_items[MENU_DEVICE]->set_sensitive(true);
		mainmenu_items[MENU_PARTITION]->set_sensitive(true);

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

	dialog.load_filesystems(gparted_core.get_filesystems());
	while ( dialog .run() == Gtk::RESPONSE_OK )
	{
		// Button [Rescan For Supported Actions] pressed in the dialog.  Rescan
		// for available core and file system specific commands and update the
		// view accordingly in the dialog.
		GParted_Core::find_supported_core();
		gparted_core .find_supported_filesystems() ;
		dialog.load_filesystems(gparted_core.get_filesystems());

		//recreate format menu...
		partitionmenu_items[MENU_FORMAT]->unset_submenu();
		partitionmenu_items[MENU_FORMAT]->set_submenu(*create_format_menu());
		partitionmenu_items[MENU_FORMAT]->get_submenu()->show_all_children();

		// Update valid operations for the currently selected partition.
		set_valid_operations();
	}
}

void Win_GParted::menu_gparted_quit()
{
	if ( Quit_Check_Operations() )
		this ->hide();
}

void Win_GParted::menu_view_harddisk_info()
{ 
	if (static_cast<Gtk::CheckMenuItem *>(mainmenu_items[MENU_DEVICE_INFORMATION])->get_active())
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
	if (static_cast<Gtk::CheckMenuItem *>(mainmenu_items[MENU_PENDING_OPERATIONS])->get_active())
		open_operationslist() ;
	else 
		close_operationslist() ;
}


void Win_GParted::show_disklabel_unrecognized (const Glib::ustring& device_name)
{
	//Display dialog box indicating that no partition table was found on the device
	Gtk::MessageDialog dialog( *this,
			/*TO TRANSLATORS: looks like   No partition table found on device /dev/sda */
			Glib::ustring::compose( _( "No partition table found on device %1" ), device_name ),
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

void Win_GParted::show_resize_readonly( const Glib::ustring & path )
{
	Gtk::MessageDialog dialog( *this,
	                           /* TO TRANSLATORS: looks like   Unable to resize read-only file system /dev/sda1 */
	                           Glib::ustring::compose( _("Unable to resize read-only file system %1"), path ),
	                           false,
	                           Gtk::MESSAGE_INFO,
	                           Gtk::BUTTONS_OK,
	                           true );
	Glib::ustring msg = _("The file system can not be resized while it is mounted read-only.");
	msg += "\n";
	msg += _("Either unmount the file system or remount it read-write.");
	dialog.set_secondary_text( msg );
	dialog.run();
}


void Win_GParted::show_help(const Glib::ustring & filename /* E.g., "gparted" */,
                            const Glib::ustring & link_id  /* For context sensitive help */)
{
	// Build uri string
	Glib::ustring uri = "help:" + filename;
	if (link_id.size() > 0)
		uri = uri + "/" + link_id;

	// Check if yelp is available to provide a useful error message.
	// Missing yelp is the most common cause of failure to display help.
	//
	// This early check is performed because failure of gtk_show_uri*()
	// method only provides a generic "Operation not permitted" message.
	if (Glib::find_program_in_path("yelp").empty())
	{
		Gtk::MessageDialog errorDialog(*this,
		                               _("Unable to open GParted Manual help file"),
		                               false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
		Glib::ustring sec_text(_("Command yelp not found."));
		sec_text.append("\n");
		sec_text.append("\n");
		sec_text.append(_("Install yelp and try again."));
		errorDialog.set_secondary_text(sec_text, true);
		errorDialog.run();
		return;
	}

	GError* error = nullptr;

	// Display help window
#if HAVE_GTK_SHOW_URI_ON_WINDOW
	// nullptr is provided for the gtk_show_uri_on_window() parent window
	// so that failures to launch yelp are reported.
	// https://gitlab.gnome.org/GNOME/gparted/-/merge_requests/82#note_1106114
	gtk_show_uri_on_window(nullptr, uri.c_str(), gtk_get_current_event_time(), &error);
#else
	GdkScreen *gscreen = gdk_screen_get_default();
	gtk_show_uri(gscreen, uri.c_str(), gtk_get_current_event_time(), &error);
#endif
	if (error != nullptr)
	{
		Gtk::MessageDialog errorDialog(*this,
		                               _("Failed to open GParted Manual help file"),
		                               false,
		                               Gtk::MESSAGE_ERROR,
		                               Gtk::BUTTONS_OK,
		                               true);
		errorDialog.set_secondary_text(error->message);
		errorDialog.run();
	}

	g_clear_error(&error);
}

void Win_GParted::menu_help_contents()
{
#ifdef ENABLE_HELP_DOC
	//GParted was built with help documentation
	show_help("gparted", "");
#else
	//GParted was built *without* help documentation using --disable-doc
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
	tmp_msg += "https://gparted.org";
	dialog .set_secondary_text( tmp_msg ) ;
	dialog .set_title( _("GParted Manual") );
	dialog .run() ;
#endif
}

void Win_GParted::menu_help_about()
{
	std::vector<Glib::ustring> strings ;
	
	Gtk::AboutDialog dialog ;
	dialog .set_transient_for( *this ) ;

	dialog.set_program_name(_("GParted"));
	dialog .set_logo_icon_name( "gparted" ) ;
	dialog .set_version( VERSION ) ;
	dialog .set_comments( _( "GNOME Partition Editor" ) ) ;
	dialog.set_copyright(  "Copyright © 2004-2006 Bart Hakvoort"
	                     "\nCopyright © 2008-2025 Curtis Gedak"
	                     "\nCopyright © 2011-2025 Mike Fleetwood");

	//authors
	//Names listed in alphabetical order by LAST name.
	//See also AUTHORS file -- names listed in opposite order to try to be fair.
	strings .push_back( "Luca Bacci <luca.bacci982@gmail.com>" );
	strings .push_back( "Sinlu Bes <e80f00@gmail.com>" ) ;
	strings .push_back( "Luca Bruno <lucab@debian.org>" ) ;
	strings .push_back( "Wrolf Courtney <wrolf@wrolf.net>" ) ;
	strings .push_back( "Jérôme Dumesnil <jerome.dumesnil@gmail.com>" ) ;
	strings .push_back( "Markus Elfring <elfring@users.sourceforge.net>" ) ;
	strings .push_back( "Mike Fleetwood <mike.fleetwood@googlemail.com>" ) ;
	strings .push_back( "Curtis Gedak <gedakc@users.sf.net>" ) ;
	strings .push_back( "Matthias Gehre <m.gehre@gmx.de>" ) ;
	strings .push_back( "Rogier Goossens <goossens.rogier@gmail.com>" ) ;
	strings .push_back( "Bart Hakvoort <gparted@users.sf.net>" ) ;
	strings .push_back( "Seth Heeren <sgheeren@gmail.com>" ) ;
	strings .push_back( "Joan Lledó <joanlluislledo@gmail.com>" ) ;
	strings .push_back( "Pali Rohár <pali.rohar@gmail.com>" );
	strings .push_back( "Phillip Susi <psusi@cfl.rr.com>" ) ;
	strings .push_back( "Antoine Viallon <antoine.viallon@gmail.com>" );
	strings. push_back( "Michael Zimmermann <sigmaepsilon92@gmail.com>" ) ;
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


	//the url is not clickable - should not invoke web browser as root
	dialog.set_website_label( "https://gparted.org" );

	dialog .run() ;
}

void Win_GParted::on_partition_selected( const Partition * partition_ptr, bool src_is_treeview )
{
	selected_partition_ptr = partition_ptr;

	set_valid_operations() ;

	if ( src_is_treeview )
		drawingarea_visualdisk.set_selected( partition_ptr );
	else
		treeview_detail.set_selected( partition_ptr );
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
	int primary_count = 0;
	for (unsigned int i = 0; i < m_display_device.partitions.size(); i++)
	{
		if (m_display_device.partitions[i].type == TYPE_PRIMARY  ||
		    m_display_device.partitions[i].type == TYPE_EXTENDED   )
		{
			primary_count ++;
		}
	}

	//Display error if user tries to create more primary partitions than the partition table can hold. 
	if (! selected_partition_ptr->inside_extended && primary_count >= m_display_device.max_prims)
	{
		Gtk::MessageDialog dialog( 
			*this,
			Glib::ustring::compose(ngettext("It is not possible to create more than %1 primary partition",
			                                "It is not possible to create more than %1 primary partitions",
			                                m_display_device.max_prims),
			                       m_display_device.max_prims),
			false,
			Gtk::MESSAGE_ERROR,
			Gtk::BUTTONS_OK,
			true);

		dialog .set_secondary_text(
			_( "If you want more partitions you should first create an extended partition. Such a partition can contain other partitions. Because an extended partition is also a primary partition it might be necessary to remove a primary partition first.") ) ;
		
		dialog .run() ;
		
		return true ;
	}
	
	return false ;
}

void Win_GParted::activate_resize()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	const Partition & selected_filesystem_ptn = selected_partition_ptr->get_filesystem_partition();
	if ( selected_filesystem_ptn.busy && selected_filesystem_ptn.fs_readonly )
	{
		// Disallow online resizing of *all* file systems mounted read-only as
		// none of them allow it, except for reiserfs.
		show_resize_readonly( selected_partition_ptr->get_path() );
		return;
	}

	PartitionVector* display_partitions_ptr = & m_display_device.partitions;
	if ( selected_partition_ptr->type == TYPE_LOGICAL )
	{
		int index_extended = find_extended_partition(m_display_device.partitions);
		if ( index_extended >= 0 )
			display_partitions_ptr = & m_display_device.partitions[index_extended].logicals;
	}

	Partition * working_ptn;
	const FSType fstype = selected_filesystem_ptn.fstype;
	FS fs_cap = gparted_core.get_fs( fstype );
	FS_Limits fs_limits = gparted_core.get_filesystem_limits( fstype, selected_filesystem_ptn );

	if (selected_partition_ptr->fstype == FS_LUKS && selected_partition_ptr->busy)
	{
		const FS & enc_cap = gparted_core.get_fs( FS_LUKS );

		// For an open LUKS mapping containing a file system being resized/moved
		// create a plain Partition object with the equivalent usage for the
		// Resize/Move dialog to work with.
		working_ptn = static_cast<const PartitionLUKS *>( selected_partition_ptr )->clone_as_plain();

		// Construct common capabilities from the file system ones.
		// Open LUKS encryption mapping can't be moved.
		fs_cap.move = FS::NONE;
		// Mask out resizing not also supported by open LUKS mapping.
		if ( ! enc_cap.online_grow )
		{
			fs_cap.grow        = FS::NONE;
			fs_cap.online_grow = FS::NONE;
		}
		if ( ! enc_cap.online_shrink )
		{
			fs_cap.shrink        = FS::NONE;
			fs_cap.online_shrink = FS::NONE;
		}

		// Adjust file system size limits accounting for LUKS encryption overhead.
		Sector luks_header_size = static_cast<const PartitionLUKS *>( selected_partition_ptr )->get_header_size();
		fs_limits.min_size = luks_header_size * working_ptn->sector_size +
		                     ( fs_limits.min_size < MEBIBYTE ) ? MEBIBYTE : fs_limits.min_size;
		if ( fs_limits.max_size > 0 )
			fs_limits.max_size += luks_header_size * working_ptn->sector_size;
	}
	else
	{
		working_ptn = selected_partition_ptr->clone();
	}

	Dialog_Partition_Resize_Move dialog(m_display_device,
	                                    fs_cap,
	                                    fs_limits,
	                                    *working_ptn,
	                                    *display_partitions_ptr);
	dialog .set_transient_for( *this ) ;	

	delete working_ptn;
	working_ptn = nullptr;

	if (dialog.run() == Gtk::RESPONSE_OK                                           &&
	    ask_for_password_for_encrypted_resize_as_required(*selected_partition_ptr)   )
	{
		dialog .hide() ;

		// Apply resize/move from the dialog to a copy of the selected partition.
		Partition * resized_ptn = selected_partition_ptr->clone();
		resized_ptn->resize( dialog.Get_New_Partition() );

		std::unique_ptr<Operation> operation = std::make_unique<OperationResizeMove>(
		                        m_devices[m_current_device],
		                        *selected_partition_ptr,
		                        *resized_ptn);
		operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_MENU);

		delete resized_ptn;
		resized_ptn = nullptr;

		// Display warning if moving a non-extended partition which already exists
		// on the disk.
		if ( operation->get_partition_original().status       != STAT_NEW                                    &&
		     operation->get_partition_original().sector_start != operation->get_partition_new().sector_start &&
		     operation->get_partition_original().type         != TYPE_EXTENDED                                  )
		{
			// Warn that move operation might break boot process
			Gtk::MessageDialog dialog( *this,
			                           _("Moving a partition might cause your operating system to fail to boot"),
			                           false,
			                           Gtk::MESSAGE_WARNING,
			                           Gtk::BUTTONS_OK,
			                           true );
			Glib::ustring tmp_msg =
					/*TO TRANSLATORS: looks like   You queued an operation to move the start sector of partition /dev/sda3. */
					Glib::ustring::compose( _( "You have queued an operation to move the start sector of partition %1." )
					                , operation->get_partition_original().get_path() );
			tmp_msg += _("  Failure to boot is most likely to occur if you move the GNU/Linux partition containing /boot, or if you move the Windows system partition C:.");
			tmp_msg += "\n";
			tmp_msg += _("You can learn how to repair the boot configuration in the GParted FAQ.");
			tmp_msg += "\n";
			tmp_msg += "https://gparted.org/faq.php";
			tmp_msg += "\n\n";
			tmp_msg += _("Moving a partition might take a very long time to apply.");
			dialog.set_secondary_text( tmp_msg );
			dialog.run();
		}

		add_operation(m_devices[m_current_device], std::move(operation));
	}

	show_operationslist() ;
}


bool Win_GParted::ask_for_password_for_encrypted_resize_as_required(const Partition& partition)
{
	if (partition.fstype != FS_LUKS || ! partition.busy)
		// Not active LUKS so won't need a password.
		return true;

	LUKS_Mapping mapping = LUKS_Info::get_cache_entry(partition.get_path());
	if (mapping.name.empty() || mapping.key_loc == KEYLOC_DMCrypt)
		// LUKS volume key stored in crypt Device-Mapper target so won't require a
		// password for encryption mapping resize.
		return true;

	const char *pw = PasswordRAMStore::lookup(partition.uuid);
	if (pw != nullptr)
		// GParted already has a password for this encryption mapping which was
		// previously used successfully or tested for correctness.
		//
		// The password will still be correct, unless it was changed by someone
		// outside GParted while running and since the last time the password was
		// used.  Re-testing the password takes 2-3 seconds which would pause the
		// UI after the [Resize/Move] button was pressed in the Resize/Move dialog
		// but before the dialog closes.  With no trivial way to provide feedback
		// that the password is being re-tested, don't spend coding effort to
		// support this use case.  So just assume the known password is still
		// correct and don't re-prompt when it will be correct 99.9% of the time.
		return true;

	DialogPasswordEntry dialog(partition,
	                           /* TO TRANSLATORS: looks like   Enter LUKS passphrase to resize /dev/sda1 */
	                           Glib::ustring::compose(_("Enter LUKS passphrase to resize %1"),
	                                                  partition.get_path()));
	dialog.set_transient_for(*this);
	bool success = false;
	do
	{
		if (dialog.run() != Gtk::RESPONSE_OK)
			// Password dialog cancelled or closed without having confirmed
			// the LUKS mapping passphrase.
			return false;

		pw = dialog.get_password();
		if (strlen(pw) == 0)
		{
			// cryptsetup won't accept a zero length password.
			dialog.set_error_message("");
			continue;
		}

		// Test the password can open the encryption mapping.
		const Glib::ustring mapping_name = Utils::generate_encryption_mapping_name(
		                                           selected_partition_ptr->get_path());
		Glib::ustring cmd = "cryptsetup luksOpen --test-passphrase " +
		                    Glib::shell_quote(partition.get_path()) + " " +
		                    Glib::shell_quote(mapping_name);
		Glib::ustring output;
		Glib::ustring error;
		success = ! Utils::execute_command(cmd, pw, output, error);

		Glib::ustring message = (success) ? "" : _("LUKS encryption passphrase check failed");
		dialog.set_error_message(message);
	}
	while (! success);

	// Save the password just entered and successfully tested on the LUKS mapping.
	PasswordRAMStore::store(partition.uuid, pw);

	dialog.hide();

	return true;
}


void Win_GParted::activate_copy()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	delete copied_partition;
	copied_partition = selected_partition_ptr->clone();
}

void Win_GParted::activate_paste()
{
	g_assert(copied_partition != nullptr);  // Bug: Paste called without partition to copy
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	// Unrecognised whole disk device (See GParted_Core::set_device_from_disk(), "unrecognized")
	if (selected_partition_ptr->type   == TYPE_UNPARTITIONED &&
	    selected_partition_ptr->fstype == FS_UNALLOCATED       )
	{
		show_disklabel_unrecognized(m_devices[m_current_device].get_path());
		return ;
	}

	const Partition & copied_filesystem_ptn = copied_partition->get_filesystem_partition();

	if ( selected_partition_ptr->type == TYPE_UNALLOCATED )
	{
		// Pasting into empty space composing new partition.

		if ( ! max_amount_prim_reached() )
		{
			FS_Limits fs_limits = gparted_core.get_filesystem_limits(
			                                      copied_filesystem_ptn.fstype,
			                                      copied_filesystem_ptn );

			// We don't want the messages, mount points or name of the source
			// partition for the new partition being created.
			Partition * part_temp = copied_filesystem_ptn.clone();
			part_temp->clear_messages();
			part_temp->clear_mountpoints();
			part_temp->name.clear();

			Dialog_Partition_Copy dialog(m_display_device,
			                             gparted_core.get_fs(copied_filesystem_ptn.fstype),
			                             fs_limits,
			                             *selected_partition_ptr,
			                             *part_temp);
			delete part_temp;
			part_temp = nullptr;
			dialog .set_transient_for( *this );
		
			if ( dialog .run() == Gtk::RESPONSE_OK )
			{
				dialog .hide() ;

				std::unique_ptr<Operation> operation = std::make_unique<OperationCopy>(
				                        m_devices[m_current_device],
				                        *selected_partition_ptr,
				                        dialog.Get_New_Partition(),
				                        *copied_partition);
				operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::COPY, Gtk::ICON_SIZE_MENU);

				// When pasting into unallocated space set a temporary
				// path of "Copy of /dev/SRC" for display purposes until
				// the partition is created and the real path queried.
				OperationCopy* copy_op = static_cast<OperationCopy*>(operation.get());
				copy_op->get_partition_new().set_path(
				        Glib::ustring::compose( _("Copy of %1"),
				                          copy_op->get_partition_copied().get_path() ) );

				add_operation(m_devices[m_current_device], std::move(operation));
			}
		}
	}
	else
	{
		// Pasting into existing partition.

		const Partition & selected_filesystem_ptn = selected_partition_ptr->get_filesystem_partition();

		bool shown_dialog = false ;
		// VGNAME from mount mount
		if (selected_filesystem_ptn.fstype == FS_LVM2_PV       &&
		    ! selected_filesystem_ptn.get_mountpoint().empty()   )
		{
			if ( ! remove_non_empty_lvm2_pv_dialog( OPERATION_COPY ) )
				return ;
			shown_dialog = true ;
		}

		Partition * partition_new;
		if (selected_partition_ptr->fstype == FS_LUKS && ! selected_partition_ptr->busy)
		{
			// Pasting into a closed LUKS encrypted partition will overwrite
			// the encryption replacing it with a non-encrypted file system.
			// Start with a plain Partition object instead of cloning the
			// existing PartitionLUKS object containing a Partition object.
			// FIXME:
			// Expect to remove this case when creating and removing LUKS
			// encryption is added as a separate action when full LUKS
			// read-write support is implemented.
			// WARNING:
			// Deliberate object slicing of *selected_partition_ptr from
			// PartitionLUKS to Partition.
			partition_new = new Partition( *selected_partition_ptr );
		}
		else
		{
			// Pasting over a file system, either a plain file system or one
			// within an open LUKS encryption mapping.  Start with a clone of
			// the existing Partition object whether it be a plain Partition
			// object or a PartitionLUKS object containing a Partition object.
			partition_new = selected_partition_ptr->clone();
		}
		partition_new->alignment = ALIGN_STRICT;

		{
			// Sub-block so that filesystem_ptn_new reference goes out of
			// scope before partition_new pointer is deallocated.
			Partition & filesystem_ptn_new = partition_new->get_filesystem_partition();
			filesystem_ptn_new.fstype = copied_filesystem_ptn.fstype;
			filesystem_ptn_new.set_filesystem_label( copied_filesystem_ptn.get_filesystem_label() );
			filesystem_ptn_new.uuid = copied_filesystem_ptn.uuid;
			Sector new_size = filesystem_ptn_new.get_sector_length();
			if ( copied_filesystem_ptn.sector_usage_known() )
			{
				if ( copied_filesystem_ptn.get_sector_length() == new_size )
				{
					// Pasting into same size existing partition, therefore
					// only block copy operation will be performed maintaining
					// the file system size.
					filesystem_ptn_new.set_sector_usage(
						copied_filesystem_ptn.sectors_used + copied_filesystem_ptn.sectors_unused,
						copied_filesystem_ptn.sectors_unused );
				}
				else
				{
					// Pasting into larger existing partition, therefore block
					// copy followed by file system grow operations (if
					// supported) will be performed making the file system
					// fill the partition.
					filesystem_ptn_new.set_sector_usage(
						new_size,
						new_size - copied_filesystem_ptn.sectors_used );
				}
			}
			else
			{
				// FS usage of source is unknown so set destination usage unknown too.
				filesystem_ptn_new.set_sector_usage( -1, -1 );
			}
			filesystem_ptn_new.clear_messages();
		}

		// Pasting an EFI System Partition (ESP) containing a FAT file system into
		// an existing partition.  Copy across the ESP flag to maintain the
		// on-disk partition type.
		if ((copied_partition->fstype == FS_FAT16 || copied_partition->fstype == FS_FAT32) &&
		    copied_partition->is_flag_set("esp")                                             )
		{
			partition_new->set_flag("esp");
		}
		// Pasting a non-FAT file system into an existing partition.  Always clear
		// the ESP flag on the target partition.
		if (copied_partition->fstype != FS_FAT16 && copied_partition->fstype != FS_FAT32)
		{
			partition_new->clear_flag("esp");
		}
		// Pasting a non-LVM2 PV file system into an existing partition.  (Always
		// true as copying an LVM2 PV is not supported).  Always clear the LVM
		// flag on the target partition.
		partition_new->clear_flag("lvm");
 
		std::unique_ptr<Operation> operation = std::make_unique<OperationCopy>(
		                        m_devices[m_current_device],
		                        *selected_partition_ptr,
		                        *partition_new,
		                        *copied_partition);
		operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::COPY, Gtk::ICON_SIZE_MENU);

		delete partition_new;
		partition_new = nullptr;

		add_operation(m_devices[m_current_device], std::move(operation));

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
			dialog .set_secondary_text(
					/*TO TRANSLATORS: looks like   The data in /dev/sda3 will be lost if you apply this operation. */
					Glib::ustring::compose( _( "The data in %1 will be lost if you apply this operation." ),
					selected_partition_ptr->get_path() ) );
			dialog .run() ;
		}
	}

	show_operationslist() ;
}

void Win_GParted::activate_new()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	// Unrecognised whole disk device (See GParted_Core::set_device_from_disk(), "unrecognized")
	if (selected_partition_ptr->type   == TYPE_UNPARTITIONED &&
	    selected_partition_ptr->fstype == FS_UNALLOCATED       )
	{
		show_disklabel_unrecognized(m_devices[m_current_device].get_path());
	}
	else if ( ! max_amount_prim_reached() )
	{
		// Check if an extended partition already exist; so that the dialog can
		// decide whether to allow the creation of the only extended partition
		// type or not.
		bool any_extended = find_extended_partition(m_display_device.partitions) >= 0;
		Dialog_Partition_New dialog(m_display_device,
		                            *selected_partition_ptr,
		                            any_extended,
		                            new_count,
		                            gparted_core.get_filesystems());
		dialog .set_transient_for( *this );
		
		if ( dialog .run() == Gtk::RESPONSE_OK )
		{
			dialog .hide() ;
			
			new_count++ ;
			std::unique_ptr<Operation> operation = std::make_unique<OperationCreate>(
			                        m_devices[m_current_device],
			                        *selected_partition_ptr,
			                        dialog.Get_New_Partition());
			operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU);

			add_operation(m_devices[m_current_device], std::move(operation));

			show_operationslist() ;
		}
	}
}


void Win_GParted::activate_delete()
{ 
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	// VGNAME from mount mount
	if (selected_partition_ptr->fstype == FS_LVM2_PV && ! selected_partition_ptr->get_mountpoint().empty())
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
	 if (selected_partition_ptr->type             == TYPE_LOGICAL                             &&
	     selected_partition_ptr->status           != STAT_NEW                                 &&
	     selected_partition_ptr->partition_number <  m_devices[m_current_device].highest_busy   )
	{	
		Gtk::MessageDialog dialog( *this,
		                           Glib::ustring::compose( _("Unable to delete %1!"), selected_partition_ptr->get_path() ),
		                           false,
		                           Gtk::MESSAGE_ERROR,
		                           Gtk::BUTTONS_OK,
		                           true );

		dialog .set_secondary_text( 
			Glib::ustring::compose( _("Please unmount any logical partitions having a number higher than %1"),
					  selected_partition_ptr->partition_number ) );

		dialog .run() ;
		return;
	}
	
	//if partition is on the clipboard...(NOTE: we can't use Partition::== here..)
	if (copied_partition != nullptr && selected_partition_ptr->get_path() == copied_partition->get_path())
	{
		Gtk::MessageDialog dialog( *this,
		                           Glib::ustring::compose( _("Are you sure you want to delete %1?"),
		                                             selected_partition_ptr->get_path() ),
		                           false,
		                           Gtk::MESSAGE_QUESTION,
		                           Gtk::BUTTONS_NONE,
		                           true );

		dialog .set_secondary_text( _("After deletion this partition is no longer available for copying.") ) ;
		
		/*TO TRANSLATORS: dialogtitle, looks like   Delete /dev/hda2 (ntfs, 2345 MiB) */
		dialog.set_title( Glib::ustring::compose( _("Delete %1 (%2, %3)"),
		                                    selected_partition_ptr->get_path(),
		                                    Utils::get_filesystem_string(selected_partition_ptr->fstype),
		                                    Utils::format_size( selected_partition_ptr->get_sector_length(), selected_partition_ptr->sector_size ) ) );
		dialog .add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
		dialog .add_button( Gtk::Stock::DELETE, Gtk::RESPONSE_OK );
	
		dialog .show_all_children() ;

		if ( dialog .run() != Gtk::RESPONSE_OK )
			return ;

		// Deleting partition on the clipboard.  Clear clipboard.
		delete copied_partition;
		copied_partition = nullptr;
	}

	// If deleted one is NEW, it doesn't make sense to add it to the operationslist,
	// we erase its creation and possible modifications like resize etc.. from the operationslist.
	// Calling Refresh_Visual will wipe every memory of its existence ;-)
	if ( selected_partition_ptr->status == STAT_NEW )
	{
		//remove all operations done on this new partition (this includes creation)	
		for (int t = 0; t < static_cast<int>(m_operations.size()); t++)
			if (m_operations[t]->m_type                         != OPERATION_DELETE                   &&
			    m_operations[t]->get_partition_new().get_path() == selected_partition_ptr->get_path()   )
			{
				remove_operation(REMOVE_AT, t--);
			}

		//determine lowest possible new_count
		new_count = 0 ; 
		for (unsigned int t = 0; t < m_operations.size(); t++)
			if (m_operations[t]->m_type                               != OPERATION_DELETE &&
			    m_operations[t]->get_partition_new().status           == STAT_NEW         &&
			    m_operations[t]->get_partition_new().partition_number >  new_count          )
				new_count = m_operations[t]->get_partition_new().partition_number;

		new_count += 1 ;

		Refresh_Visual(); 

		if (! m_operations.size())
			close_operationslist() ;
	}
	else //deletion of a real partition...
	{
		std::unique_ptr<Operation> operation = std::make_unique<OperationDelete>(
		                        m_devices[m_current_device],
		                        *selected_partition_ptr);
		operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::DELETE, Gtk::ICON_SIZE_MENU);

		add_operation(m_devices[m_current_device], std::move(operation));
	}

	show_operationslist() ;
}


void Win_GParted::activate_info()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	Dialog_Partition_Info dialog( *selected_partition_ptr );
	dialog .set_transient_for( *this );
	dialog .run();
}

void Win_GParted::activate_format( FSType new_fs )
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	const Partition & filesystem_ptn = selected_partition_ptr->get_filesystem_partition();

	// For non-empty LVM2 PV confirm overwrite before continuing.  VGNAME from mount mount.
	if (filesystem_ptn.fstype == FS_LVM2_PV && ! filesystem_ptn.get_mountpoint().empty())
	{
		if ( ! remove_non_empty_lvm2_pv_dialog( OPERATION_FORMAT ) )
			return ;
	}

	// Compose Partition object to represent the format operation.
	Partition * temp_ptn;
	if (selected_partition_ptr->fstype == FS_LUKS && ! selected_partition_ptr->busy)
	{
		// Formatting a closed LUKS encrypted partition will erase the encryption
		// replacing it with a non-encrypted file system.  Start with a plain
		// Partition object instead of cloning the existing PartitionLUKS object
		// containing a Partition object.
		// FIXME:
		// Expect to remove this case when creating and removing LUKS encryption
		// is added as a separate action when full LUKS read-write support is
		// implemented.
		temp_ptn = new Partition;
		temp_ptn->status = selected_partition_ptr->status;
	}
	else
	{
		// Formatting a file system, either a plain file system or one within an
		// open LUKS encryption mapping.  Start with a clone of the existing
		// Partition object whether it be a plain Partition object or a
		// PartitionLUKS object containing a Partition object.
		temp_ptn = selected_partition_ptr->clone();
	}
	{
		// Sub-block so that temp_filesystem_ptn reference goes out of scope
		// before temp_ptn pointer is deallocated.
		Partition & temp_filesystem_ptn = temp_ptn->get_filesystem_partition();
		temp_filesystem_ptn.Reset();
		temp_filesystem_ptn.status = filesystem_ptn.status;
		temp_filesystem_ptn.Set( filesystem_ptn.device_path,
		                         filesystem_ptn.get_path(),
		                         filesystem_ptn.partition_number,
		                         filesystem_ptn.type,
		                         new_fs,
		                         filesystem_ptn.sector_start,
		                         filesystem_ptn.sector_end,
		                         filesystem_ptn.sector_size,
		                         filesystem_ptn.inside_extended,
		                         false );
		// Leave sector usage figures as new Partition object defaults of
		// -1, -1, 0 (_used, _unused, _unallocated) representing unknown.
		// Also leaves fs_block_size default of -1 so that FileSystem derived
		// get_filesystem_limits() call below can differentiate reformat to same
		// file system case apart from resize case.
	}
	temp_ptn->name = selected_partition_ptr->name;

	// Formatting an EFI System Partition (ESP) to a FAT file system.  Copy across the
	// ESP flag to maintain the on-disk partition type.  (It's not necessary to clear
	// any flags when formatting to other file system types because the above
	// temp_filesystem_ptn.Reset() clears all flags).
	if ((new_fs == FS_FAT16 || new_fs == FS_FAT32) && selected_partition_ptr->is_flag_set("esp"))
	{
		temp_ptn->set_flag("esp");
	}
	// Formatting to LVM2 PV.  Always set the libparted LVM flag.
	if (new_fs == FS_LVM2_PV)
	{
		temp_ptn->set_flag("lvm");
	}

	// Generate minimum and maximum partition size limits for the new file system.
	FS_Limits fs_limits = gparted_core.get_filesystem_limits( new_fs, temp_ptn->get_filesystem_partition() );
	bool encrypted = false;
	if (selected_partition_ptr->fstype == FS_LUKS && selected_partition_ptr->busy)
	{
		encrypted = true;
		// Calculate the actual overhead rather than just using the size of the
		// LUKS header so that correct limits are reported in cases when the
		// active LUKS mapping doesn't extend to the end of the partition for this
		// format file system only operation.
		Byte_Value encryption_overhead = selected_partition_ptr->get_byte_length() -
		                                 filesystem_ptn.get_byte_length();
		fs_limits.min_size += encryption_overhead;
		if ( fs_limits.max_size > 0 )
			fs_limits.max_size += encryption_overhead;
	}

	// Confirm partition is the right size to store the file system.
	if ( ( selected_partition_ptr->get_byte_length() < fs_limits.min_size )                       ||
	     ( fs_limits.max_size && selected_partition_ptr->get_byte_length() > fs_limits.max_size )    )
	{
		Gtk::MessageDialog dialog( *this,
		                           Glib::ustring::compose( /* TO TRANSLATORS: looks like
		                                              * Cannot format this file system to fat16.
		                                              */
		                                             _("Cannot format this file system to %1"),
		                                             Utils::get_filesystem_string( encrypted, new_fs ) ),
		                           false,
		                           Gtk::MESSAGE_ERROR,
		                           Gtk::BUTTONS_OK,
		                           true );

		if ( selected_partition_ptr->get_byte_length() < fs_limits.min_size )
			dialog.set_secondary_text( Glib::ustring::compose(
					/* TO TRANSLATORS: looks like
					 * A fat16 file system requires a partition of at least 16.00 MiB.
					 */
					 _( "A %1 file system requires a partition of at least %2."),
					 Utils::get_filesystem_string( encrypted, new_fs ),
					 Utils::format_size( fs_limits.min_size, 1 /* Byte */ ) ) );
		else
			dialog.set_secondary_text( Glib::ustring::compose(
					/* TO TRANSLATORS: looks like
					 * A partition with a hfs file system has a maximum size of 2.00 GiB.
					 */
					 _( "A partition with a %1 file system has a maximum size of %2."),
					 Utils::get_filesystem_string( encrypted, new_fs ),
					 Utils::format_size( fs_limits.max_size, 1 /* Byte */ ) ) );

		dialog.run();
	}
	else
	{
		std::unique_ptr<Operation> operation = std::make_unique<OperationFormat>(
		                        m_devices[m_current_device],
		                        *selected_partition_ptr,
		                        *temp_ptn);
		operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU);

		add_operation(m_devices[m_current_device], std::move(operation));

		show_operationslist();
	}

	delete temp_ptn;
	temp_ptn = nullptr;
}


bool Win_GParted::open_encrypted_partition( const Partition & partition,
                                            const char * entered_password,
                                            Glib::ustring & message )
{
	const char* pw = nullptr;
	if (entered_password == nullptr)
	{
		pw = PasswordRAMStore::lookup( partition.uuid );
		if (pw == nullptr)
		{
			// Internal documentation message never shown to user.
			message = "No stored password available";
			return false;
		}
	}
	else
	{
		pw = entered_password;
		if ( strlen( pw ) == 0 )
		{
			// "cryptsetup" won't accept a zero length password.
			message = "";
			return false;
		}
	}

	const Glib::ustring mapping_name = Utils::generate_encryption_mapping_name(selected_partition_ptr->get_path());
	Glib::ustring cmd = "cryptsetup luksOpen " +
	                    Glib::shell_quote( partition.get_path() ) + " " +
	                    Glib::shell_quote( mapping_name );

	show_pulsebar( Glib::ustring::compose( _("Opening encryption on %1"), partition.get_path() ) );
	Glib::ustring output;
	Glib::ustring error;
	bool success = ! Utils::execute_command( cmd, pw, output, error );
	hide_pulsebar();
	if (success && pw != nullptr)
		// Save the password just entered and successfully used to open the LUKS
		// mapping.
		PasswordRAMStore::store( partition.uuid, pw );
	else if ( ! success )
		// Erase the password used during for the failure to open the LUKS
		// mapping.
		PasswordRAMStore::erase( partition.uuid );

	message = ( success ) ? "" : _("Failed to open LUKS encryption");
	return success;
}

void Win_GParted::toggle_crypt_busy_state()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	enum Action
	{
		NONE      = 0,
		LUKSCLOSE = 1,
		LUKSOPEN  = 2
	};
	Action action = NONE;
	Glib::ustring disallowed_msg;
	Glib::ustring pulse_msg;
	Glib::ustring failure_msg;
	if ( selected_partition_ptr->busy )
	{
		action = LUKSCLOSE;
		disallowed_msg = _("The close encryption action cannot be performed when there are operations pending for the partition.");
		pulse_msg = Glib::ustring::compose( _("Closing encryption on %1"), selected_partition_ptr->get_path() );
		failure_msg = _("Could not close encryption");
	}
	else  // ( ! selected_partition_ptr->busy )
	{
		action = LUKSOPEN;
		disallowed_msg = _("The open encryption action cannot be performed when there are operations pending for the partition.");
	}

	if ( ! check_toggle_busy_allowed( disallowed_msg ) )
		// One or more operations are pending for this partition so changing the
		// busy state is not allowed.
		//
		// After set_valid_operations() has allowed the operations to be queued
		// the rest of the code assumes the busy state of the partition won't
		// change.  Therefore pending operations must prevent changing the busy
		// state of the partition.
		return;

	bool success = false;
	Glib::ustring cmd;
	Glib::ustring output;
	Glib::ustring error;
	Glib::ustring error_msg;
	switch ( action )
	{
		case LUKSCLOSE:
			cmd = "cryptsetup luksClose " +
			      Glib::shell_quote( selected_partition_ptr->get_mountpoint() );
			show_pulsebar( pulse_msg );
			success = ! Utils::execute_command( cmd, output, error );
			hide_pulsebar();
			error_msg = "<i># " + cmd + "\n" + error + "</i>";
			break;
		case LUKSOPEN:
		{
			// Attempt to unlock LUKS using stored passphrase first.
			success = open_encrypted_partition(*selected_partition_ptr, nullptr, error_msg);
			if ( success )
				break;

			// Open password dialog and attempt to unlock LUKS mapping.
			DialogPasswordEntry dialog(*selected_partition_ptr,
			                           /* TO TRANSLATORS: looks like
						    * Enter LUKS passphrase to open /dev/sda1
						    */
			                           Glib::ustring::compose(_("Enter LUKS passphrase to open %1"),
			                                                  selected_partition_ptr->get_path()));
			dialog.set_transient_for( *this );
			do
			{
				if ( dialog.run() != Gtk::RESPONSE_OK )
					// Password dialog cancelled or closed without the
					// LUKS mapping having been opened.  Skip refresh.
					return;

				success = open_encrypted_partition( *selected_partition_ptr,
				                                    dialog.get_password(),
				                                    error_msg );
				dialog.set_error_message( error_msg );
			} while ( ! success );
		}
		default:
			// Impossible
			break;
	}

	if ( ! success )
		show_toggle_failure_dialog( failure_msg, error_msg );

	menu_gparted_refresh_devices();
}

bool Win_GParted::unmount_partition( const Partition & partition, Glib::ustring & error )
{
	const std::vector<Glib::ustring>& fs_mountpoints = partition.get_mountpoints();
	const std::vector<Glib::ustring> all_mountpoints = Mount_Info::get_all_mountpoints();

	std::vector<Glib::ustring> skipped_mountpoints;
	std::vector<Glib::ustring> umount_errors;

	for ( unsigned int i = 0 ; i < fs_mountpoints.size() ; i ++ )
	{
		if ( std::count( all_mountpoints.begin(), all_mountpoints.end(), fs_mountpoints[i] ) >= 2 )
		{
			// This mount point has two or more different file systems mounted
			// on top of each other.  Refuse to unmount it as in the general
			// case all have to be unmounted and then all except the file
			// system being unmounted have to be remounted.  This is too rare
			// a case to write such complicated code for.
			skipped_mountpoints.push_back( fs_mountpoints[i] );
		}
		else
		{
			// Unmounting below a duplicating bind mount, unmounts all copies
			// in one go so check if the file system is still mounted at this
			// mount point before trying to unmount it.
			Mount_Info::load_cache();
			if (Mount_Info::is_dev_mounted_at(partition.get_path(), fs_mountpoints[i]))
			{
				Glib::ustring cmd = "umount -v " + Glib::shell_quote(fs_mountpoints[i]);
				Glib::ustring dummy;
				Glib::ustring umount_error;

				if (Utils::execute_command(cmd, dummy, umount_error))
					umount_errors.push_back("# " + cmd + "\n" + umount_error);
			}
		}
	}

	if ( umount_errors.size() > 0 )
	{
		error = "<i>" + Glib::build_path( "</i>\n<i>", umount_errors ) + "</i>";
		return false;
	}
	if ( skipped_mountpoints.size() > 0 )
	{
		error = _("The partition could not be unmounted from the following mount points:");
		error += "\n\n<i>" + Glib::build_path( "\n", skipped_mountpoints ) + "</i>\n\n";
		error += _("This is because other partitions are also mounted on these mount points.  You are advised to unmount them manually.");
		return false;
	}
	return true;
}

bool Win_GParted::check_toggle_busy_allowed( const Glib::ustring & disallowed_msg )
{
	int operation_count = partition_in_operation_queue_count( *selected_partition_ptr );
	if ( operation_count > 0 )
	{
		Glib::ustring primary_msg = Glib::ustring::compose(
			/* TO TRANSLATORS: Singular case looks like   1 operation is currently pending for partition /dev/sdb1 */
			ngettext( "%1 operation is currently pending for partition %2",
			/* TO TRANSLATORS: Plural case looks like   3 operations are currently pending for partition /dev/sdb1 */
			          "%1 operations are currently pending for partition %2",
			          operation_count ),
			operation_count,
			selected_partition_ptr->get_path() );

		Gtk::MessageDialog dialog( *this,
		                           primary_msg,
		                           false,
		                           Gtk::MESSAGE_INFO,
		                           Gtk::BUTTONS_OK,
		                           true );

		Glib::ustring secondary_msg = disallowed_msg + "\n" +
			_("Use the Edit menu to undo, clear or apply pending operations.");
		dialog.set_secondary_text( secondary_msg );
		dialog.run();
		return false;
	}
	return true;
}

void Win_GParted::show_toggle_failure_dialog( const Glib::ustring & failure_summary,
                                              const Glib::ustring & marked_up_error )
{
	Gtk::MessageDialog dialog( *this,
	                           failure_summary,
	                           false,
	                           Gtk::MESSAGE_ERROR,
	                           Gtk::BUTTONS_OK,
	                           true );
	dialog.set_secondary_text( marked_up_error, true );
	dialog.run();
}

void Win_GParted::toggle_fs_busy_state()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	enum Action
	{
		NONE          = 0,
		SWAPOFF       = 1,
		SWAPON        = 2,
		DEACTIVATE_VG = 3,
		ACTIVATE_VG   = 4,
		UNMOUNT       = 5
	};
	Action action = NONE;
	Glib::ustring disallowed_msg;
	Glib::ustring pulse_msg;
	Glib::ustring failure_msg;
	const Partition & filesystem_ptn = selected_partition_ptr->get_filesystem_partition();
	if (filesystem_ptn.fstype == FS_LINUX_SWAP && filesystem_ptn.busy)
	{
		action = SWAPOFF;
		disallowed_msg = _("The swapoff action cannot be performed when there are operations pending for the partition.");
		pulse_msg = Glib::ustring::compose( _("Deactivating swap on %1"), filesystem_ptn.get_path() );
		failure_msg = _("Could not deactivate swap");
	}
	else if (filesystem_ptn.fstype == FS_LINUX_SWAP && ! filesystem_ptn.busy)
	{
		action = SWAPON;
		disallowed_msg = _("The swapon action cannot be performed when there are operations pending for the partition.");
		pulse_msg = Glib::ustring::compose( _("Activating swap on %1"), filesystem_ptn.get_path() );
		failure_msg = _("Could not activate swap");
	}
	else if (filesystem_ptn.fstype == FS_LVM2_PV && filesystem_ptn.busy)
	{
		action = DEACTIVATE_VG;
		disallowed_msg = _("The deactivate Volume Group action cannot be performed when there are operations pending for the partition.");
		pulse_msg = Glib::ustring::compose( _("Deactivating Volume Group %1"),
		                              filesystem_ptn.get_mountpoint() );  // VGNAME from point point
		failure_msg = _("Could not deactivate Volume Group");
	}
	else if (filesystem_ptn.fstype == FS_LVM2_PV && ! filesystem_ptn.busy)
	{
		action = ACTIVATE_VG;
		disallowed_msg = _("The activate Volume Group action cannot be performed when there are operations pending for the partition.");
		pulse_msg = Glib::ustring::compose( _("Activating Volume Group %1"),
		                              filesystem_ptn.get_mountpoint() );  // VGNAME from point point
		failure_msg = _("Could not activate Volume Group");
	}
	else if ( filesystem_ptn.busy )
	{
		action = UNMOUNT;
		disallowed_msg = _("The unmount action cannot be performed when there are operations pending for the partition.");
		pulse_msg = Glib::ustring::compose( _("Unmounting %1"), filesystem_ptn.get_path() );
		failure_msg = Glib::ustring::compose( _("Could not unmount %1"), filesystem_ptn.get_path() );
	}
	else
		// Impossible.  Mounting a file system calls activate_mount_partition().
		return;

	if ( ! check_toggle_busy_allowed( disallowed_msg ) )
		// One or more operations are pending for this partition so changing the
		// busy state is not allowed.
		//
		// After set_valid_operations() has allowed the operations to be queued
		// the rest of the code assumes the busy state of the partition won't
		// change.  Therefore pending operations must prevent changing the busy
		// state of the partition.
		return;

	show_pulsebar( pulse_msg );
	bool success = false;
	Glib::ustring cmd;
	Glib::ustring output;
	Glib::ustring error;
	Glib::ustring error_msg;
	switch ( action )
	{
		case SWAPOFF:
			cmd = "swapoff -v " + Glib::shell_quote( filesystem_ptn.get_path() );
			success = ! Utils::execute_command( cmd, output, error );
			error_msg = "<i># " + cmd + "\n" + error + "</i>";
			break;
		case SWAPON:
			cmd = "swapon -v " + Glib::shell_quote( filesystem_ptn.get_path() );
			success = ! Utils::execute_command( cmd, output, error );
			error_msg = "<i># " + cmd + "\n" + error + "</i>";
			break;
		case DEACTIVATE_VG:
			cmd = "lvm vgchange -a n " + Glib::shell_quote( filesystem_ptn.get_mountpoint() );
			success = ! Utils::execute_command( cmd, output, error );
			error_msg = "<i># " + cmd + "\n" + error + "</i>";
			break;
		case ACTIVATE_VG:
			cmd = "lvm vgchange -a y " + Glib::shell_quote( filesystem_ptn.get_mountpoint() );
			success = ! Utils::execute_command( cmd, output, error );
			error_msg = "<i># " + cmd + "\n" + error + "</i>";
			break;
		case UNMOUNT:
			success = unmount_partition( filesystem_ptn, error_msg );
			break;
		default:
			// Impossible
			break;
	}
	hide_pulsebar();

	if ( ! success )
		show_toggle_failure_dialog( failure_msg, error_msg );

	menu_gparted_refresh_devices() ;
}

void Win_GParted::activate_mount_partition( unsigned int index ) 
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	Glib::ustring disallowed_msg = _("The mount action cannot be performed when an operation is pending for the partition.");
	if ( ! check_toggle_busy_allowed( disallowed_msg ) )
		// One or more operations are pending for this partition so changing the
		// busy state by mounting the file system is not allowed.
		return;

	bool success;
	Glib::ustring cmd;
	Glib::ustring output;
	Glib::ustring error;
	Glib::ustring error_msg;

	const Partition & filesystem_ptn = selected_partition_ptr->get_filesystem_partition();
	show_pulsebar( Glib::ustring::compose( _("mounting %1 on %2"),
	                                 filesystem_ptn.get_path(),
	                                 filesystem_ptn.get_mountpoints()[index] ) );

	// First try mounting letting mount (libblkid) determine the file system type.
	cmd = "mount -v " + Glib::shell_quote( filesystem_ptn.get_path() ) +
	      " " + Glib::shell_quote( filesystem_ptn.get_mountpoints()[index] );
	success = ! Utils::execute_command( cmd, output, error );
	if ( ! success )
	{
		error_msg = "<i># " + cmd + "\n" + error + "</i>";

		Glib::ustring type = Utils::get_filesystem_kernel_name(filesystem_ptn.fstype);
		if ( ! type.empty() )
		{
			// Second try mounting specifying the GParted determined file
			// system type.
			cmd = "mount -v -t " + Glib::shell_quote( type ) +
			      " " + Glib::shell_quote( filesystem_ptn.get_path() ) +
			      " " + Glib::shell_quote( filesystem_ptn.get_mountpoints()[index] );
			success = ! Utils::execute_command( cmd, output, error );
			if ( ! success )
				error_msg += "\n<i># " + cmd + "\n" + error + "</i>";
		}
	}
	hide_pulsebar();
	if ( ! success )
	{
		Glib::ustring failure_msg = Glib::ustring::compose( _("Could not mount %1 on %2"),
		                                              filesystem_ptn.get_path(),
		                                              filesystem_ptn.get_mountpoints()[index] );
		show_toggle_failure_dialog( failure_msg, error_msg );
	}

	menu_gparted_refresh_devices() ;
}

void Win_GParted::activate_disklabel()
{
	//If there are active mounted partitions on the device then warn
	//  the user that all partitions must be unactive before creating
	//  a new partition table
	int active_count = active_partitions_on_device_count(m_devices[m_current_device]);
	if ( active_count > 0 )
	{
		Glib::ustring tmp_msg =
		    Glib::ustring::compose( /*TO TRANSLATORS: Singular case looks like  1 partition is currently active on device /dev/sda */
		                      ngettext( "%1 partition is currently active on device %2"
		                      /*TO TRANSLATORS: Plural case looks like    3 partitions are currently active on device /dev/sda */
		                              , "%1 partitions are currently active on device %2"
		                              , active_count
		                              )
		                    , active_count
		                    , m_devices[m_current_device].get_path()
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
	if (m_operations.size())
	{
		Glib::ustring tmp_msg =
		    Glib::ustring::compose( ngettext( "%1 operation is currently pending"
		                              , "%1 operations are currently pending"
		                              , m_operations.size()
		                              )
		                    , m_operations.size()
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
	Dialog_Disklabel dialog(m_devices[m_current_device]);
	dialog .set_transient_for( *this );

	if ( dialog .run() == Gtk::RESPONSE_APPLY )
	{
		if (! gparted_core.set_disklabel(m_devices[m_current_device],dialog.Get_Disklabel()))
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


void Win_GParted::activate_manage_flags() 
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	get_window()->set_cursor(Gdk::Cursor::create(Gdk::WATCH));
	while ( Gtk::Main::events_pending() )
		Gtk::Main::iteration() ;

	DialogManageFlags dialog( *selected_partition_ptr, gparted_core.get_available_flags( *selected_partition_ptr ) );
	dialog .set_transient_for( *this ) ;
	dialog .signal_get_flags .connect(
		sigc::mem_fun( &gparted_core, &GParted_Core::get_available_flags ) ) ;
	dialog .signal_toggle_flag .connect(
		sigc::mem_fun( &gparted_core, &GParted_Core::toggle_flag ) ) ;

	get_window() ->set_cursor() ;
	
	dialog .run() ;
	dialog .hide() ;

	if (dialog.m_changed)
		menu_gparted_refresh_devices() ;
}


void Win_GParted::activate_check() 
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	if (! ask_for_password_for_encrypted_resize_as_required(*selected_partition_ptr))
		// Open encryption mapping needing a passphrase to resize but the password
		// dialog was cancelled or closed without providing a working passphrase.
		return;

	// FIXME: Consider constructing new partition object with zero unallocated and
	// messages cleared to represent how applying a check operation also grows the
	// file system to fill the partition.
	std::unique_ptr<Operation> operation = std::make_unique<OperationCheck>(
	                        m_devices[m_current_device],
	                        *selected_partition_ptr);
	operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU);

	add_operation(m_devices[m_current_device], std::move(operation));

	show_operationslist() ;
}


void Win_GParted::activate_label_filesystem()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	const Partition & filesystem_ptn = selected_partition_ptr->get_filesystem_partition();
	Dialog_FileSystem_Label dialog( filesystem_ptn );
	dialog .set_transient_for( *this );

	if (	dialog .run() == Gtk::RESPONSE_OK
	     && dialog.get_new_label() != filesystem_ptn.get_filesystem_label() )
	{
		dialog .hide() ;
		// Make a duplicate of the selected partition (used in UNDO)
		Partition * part_temp = selected_partition_ptr->clone();

		part_temp->get_filesystem_partition().set_filesystem_label( dialog.get_new_label() );

		std::unique_ptr<Operation> operation = std::make_unique<OperationLabelFileSystem>(
		                        m_devices[m_current_device],
		                        *selected_partition_ptr,
		                        *part_temp);
		operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU);

		delete part_temp;
		part_temp = nullptr;

		add_operation(m_devices[m_current_device], std::move(operation));

		show_operationslist() ;
	}
}


void Win_GParted::activate_name_partition()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	Dialog_Partition_Name dialog(*selected_partition_ptr,
	                             m_devices[m_current_device].get_max_partition_name_length());
	dialog.set_transient_for( *this );

	if (	dialog.run() == Gtk::RESPONSE_OK
	     && dialog.get_new_name() != selected_partition_ptr->name )
	{
		dialog.hide();
		// Make a duplicate of the selected partition (used in UNDO)
		Partition * part_temp = selected_partition_ptr->clone();

		part_temp->name = dialog.get_new_name();

		std::unique_ptr<Operation> operation = std::make_unique<OperationNamePartition>(
		                        m_devices[m_current_device],
		                        *selected_partition_ptr,
		                        *part_temp);
		operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU);

		delete part_temp;
		part_temp = nullptr;

		add_operation(m_devices[m_current_device], std::move(operation));

		show_operationslist();
	}
}


void Win_GParted::activate_change_uuid()
{
	g_assert(selected_partition_ptr != nullptr);  // Bug: Partition callback without a selected partition
	g_assert( valid_display_partition_ptr( selected_partition_ptr ) );  // Bug: Not pointing at a valid display partition object

	const Partition & filesystem_ptn = selected_partition_ptr->get_filesystem_partition();
	const FileSystem *filesystem_object = gparted_core.get_filesystem_object(filesystem_ptn.fstype);
	if ( filesystem_object->get_custom_text( CTEXT_CHANGE_UUID_WARNING ) != "" )
	{
		int i ;
		Gtk::MessageDialog dialog( *this,
		                           filesystem_object->get_custom_text( CTEXT_CHANGE_UUID_WARNING, 0 ),
		                           false,
		                           Gtk::MESSAGE_WARNING,
		                           Gtk::BUTTONS_OK,
		                           true );
		Glib::ustring tmp_msg;
		for ( i = 1 ; filesystem_object->get_custom_text( CTEXT_CHANGE_UUID_WARNING, i ) != "" ; i++ )
		{
			if ( i > 1 )
				tmp_msg += "\n\n" ;
			tmp_msg += filesystem_object->get_custom_text( CTEXT_CHANGE_UUID_WARNING, i );
		}
		dialog .set_secondary_text( tmp_msg ) ;
		dialog .run() ;
	}

	// Make a duplicate of the selected partition (used in UNDO)
	Partition * temp_ptn = selected_partition_ptr->clone();

	{
		// Sub-block so that temp_filesystem_ptn reference goes out of scope
		// before temp_ptn pointer is deallocated.
		Partition & temp_filesystem_ptn = temp_ptn->get_filesystem_partition();
		if (temp_filesystem_ptn.fstype == FS_NTFS)
			// Explicitly ask for half, so that the user will be aware of it
			// Also, keep this kind of policy out of the NTFS code.
			temp_filesystem_ptn.uuid = UUID_RANDOM_NTFS_HALF;
		else
			temp_filesystem_ptn.uuid = UUID_RANDOM;
	}

	std::unique_ptr<Operation> operation = std::make_unique<OperationChangeUUID>(
	                        m_devices[m_current_device],
	                        *selected_partition_ptr,
	                        *temp_ptn);
	operation->m_icon = Utils::mk_pixbuf(*this, Gtk::Stock::EXECUTE, Gtk::ICON_SIZE_MENU);

	delete temp_ptn;
	temp_ptn = nullptr;

	add_operation(m_devices[m_current_device], std::move(operation));

	show_operationslist() ;
}


void Win_GParted::activate_undo()
{
	//when undoing a creation it's safe to decrease the newcount by one
	if (m_operations.back()->m_type == OPERATION_CREATE)
		new_count-- ;

	remove_operation(REMOVE_LAST);

	Refresh_Visual();

	if (! m_operations.size())
		close_operationslist() ;

	//FIXME:  A slight flicker may be introduced by this extra display refresh.
	//An extra display refresh seems to prevent the disk area visual disk from
	//  disappearing when there enough operations to require a scrollbar
	//  (about 4+ operations with default window size) and a user clicks on undo.
	//  See also Win_GParted::Add_operation().
	drawingarea_visualdisk .queue_draw() ;
}


void Win_GParted::remove_operation(OperationRemoveType rmtype, int index)
{
	switch (rmtype)
	{
		case REMOVE_ALL:
			m_operations.clear();
			break;
		case REMOVE_LAST:
			if (m_operations.size() > 0)
				m_operations.pop_back();
			break;
		case REMOVE_AT:
			if (0 <= index && index < static_cast<int>(m_operations.size()))
				m_operations.erase(m_operations.begin() + index);
			break;
	}
}


int Win_GParted::partition_in_operation_queue_count( const Partition & partition )
{
	int operation_count = 0 ;

	for (unsigned int t = 0; t < m_operations.size(); t++)
	{
		if (partition.get_path() == m_operations[t]->get_partition_original().get_path())
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
		// Count the active primary and extended partitions
		if (   device .partitions[ k ] .busy
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

		Dialog_Progress dialog_progress(m_devices, m_operations);
		dialog_progress .set_transient_for( *this ) ;
		dialog_progress .signal_apply_operation .connect(
			sigc::mem_fun(gparted_core, &GParted_Core::apply_operation_to_disk) ) ;
 
		int response ;
		do
		{
			response = dialog_progress .run() ;
		}
		while ( response == Gtk::RESPONSE_CANCEL || response == Gtk::RESPONSE_OK ) ;
		
		dialog_progress .hide() ;

		//wipe operations...
		remove_operation(REMOVE_ALL);
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
			tmp_msg = Glib::ustring::compose( _( "You are deleting non-empty LVM2 Physical Volume %1" ),
			                            selected_partition_ptr->get_path() );
			break ;
		case OPERATION_FORMAT:
			tmp_msg = Glib::ustring::compose( _( "You are formatting over non-empty LVM2 Physical Volume %1" ),
			                            selected_partition_ptr->get_path() );
			break ;
		case OPERATION_COPY:
			tmp_msg = Glib::ustring::compose( _( "You are pasting over non-empty LVM2 Physical Volume %1" ),
			                            selected_partition_ptr->get_path() );
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

	const Glib::ustring& vgname = LVM2_PV_Info::get_vg_name(selected_partition_ptr->get_path());
	std::vector<Glib::ustring> members ;
	if ( ! vgname .empty() )
		members = LVM2_PV_Info::get_vg_members( vgname );

	//Single copy of each string for translation purposes
	const Glib::ustring vgname_label  = _( "Volume Group:" ) ;
	const Glib::ustring members_label = _( "Members:" ) ;

	dialog .set_secondary_text( tmp_msg, true ) ;

	// Nicely formatted display of VG members by using a grid below the secondary
	// text in the dialog.
	Gtk::Box * msg_area = dialog .get_message_area() ;

	Gtk::Separator* hsep(Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL)));
	msg_area ->pack_start( * hsep ) ;

	Gtk::Grid* grid(Gtk::manage(new Gtk::Grid()));
	grid->set_column_spacing(10);
	msg_area->pack_start(*grid);

	// Volume Group
	Gtk::Label *label_vgname = Utils::mk_label("<b>" + Glib::ustring(vgname_label) + "</b>");
	grid->attach(*label_vgname, 0, 0, 1, 1);
	Gtk::Label *value_vgname = Utils::mk_label(vgname, true, false, true);
	grid->attach(*value_vgname, 1, 0, 1, 1);
	value_vgname->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                 label_vgname->get_accessible());

	// Members
	Gtk::Label *label_members = Utils::mk_label("<b>" + Glib::ustring(members_label) + "</b>",
	                                            true, false, false, Gtk::ALIGN_START);
	grid->attach(*label_members, 0, 1, 1, 1);
	Gtk::Label *value_members = Utils::mk_label(Glib::build_path("\n", members),
	                                            true, false, true, Gtk::ALIGN_START);
	grid->attach(*value_members, 1, 1, 1, 1);
	value_members->get_accessible()->add_relationship(Atk::RELATION_LABELLED_BY,
	                                                  label_members->get_accessible());

	dialog .add_button( Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL );
	dialog .add_button( Gtk::Stock::DELETE, Gtk::RESPONSE_OK );
	dialog .show_all() ;
	if ( dialog .run() == Gtk::RESPONSE_OK )
		return true ;
	return false ;
}

}  // namespace GParted
