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
 
#include "../include/Win_GParted.h"

namespace GParted
{
	
Win_GParted::Win_GParted( )
{
	copied_partition .partition = "NONE" ;
	new_count = 1;
	current_device = source_device = 0 ;
	vbox_visual_disk = NULL;
	os.imbue(std::locale(""));//set locale for this stream to standard
	
	//locate all available devices and store them in devices vector
	Find_Devices() ;
		
	//==== GUI =========================
	this->set_title( _("GParted") );
	this->set_default_size( -1,500 );
		
	//Pack the main box
	this->add( vbox_main ); 
	
	//menubar....
	init_menubar() ;
	vbox_main .pack_start( menubar_main, Gtk::PACK_SHRINK );
	
	//toolbar....
	init_toolbar() ;
	vbox_main.pack_start( hbox_toolbar, Gtk::PACK_SHRINK );
	
	//hbox_visual...  ( contains the visual represenation of the disks )
	vbox_main.pack_start( hbox_visual, Gtk::PACK_SHRINK );
		
	//hpaned_main (NOTE: added to vpaned_main)
	init_hpaned_main() ;
	vpaned_main .pack1( hpaned_main, true, true ) ;
	
	//vpaned_main....
	close_operationslist() ;
	vbox_main.pack_start( vpaned_main );
	
	//device info...
	init_device_info() ;
	
	//operationslist...
	init_operationslist() ;
	vpaned_main .pack2( hbox_operations, true, true ) ;
	
	//statusbar... 
	vbox_main.pack_start( statusbar, Gtk::PACK_SHRINK );
	
	//popupmenu...
	init_popupmenu() ;
		
	//initizialize for the first time...
	optionmenu_devices_changed();

	this ->show_all_children();
}

void Win_GParted::init_menubar() 
{
	//fill menubar_main and connect callbacks 
	//gparted
	menu = manage( new Gtk::Menu() ) ;
	image = manage( new Gtk::Image( Gtk::Stock::REFRESH, Gtk::ICON_SIZE_MENU ) );
	menu ->items().push_back(Gtk::Menu_Helpers::ImageMenuElem( _("_Refresh devices"), Gtk::AccelKey("<control>r"), *image , sigc::mem_fun(*this, &Win_GParted::menu_gparted_refresh_devices) ) );
	menu ->items().push_back( Gtk::Menu_Helpers::SeparatorElem() );
	menu ->items().push_back(Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::QUIT , sigc::mem_fun(*this, &Win_GParted::menu_gparted_quit) ) );
	menubar_main.items().push_back( Gtk::Menu_Helpers::MenuElem( _("_GParted"), *menu ) );
	
	//help
	menu = manage( new Gtk::Menu() ) ;
	menu ->items().push_back(Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::HELP, sigc::mem_fun(*this, &Win_GParted::menu_help_contents) ) );
	image = manage( new Gtk::Image( "/usr/share/icons/hicolor/16x16/stock/generic/stock_about.png" ) );
	menu ->items().push_back(Gtk::Menu_Helpers::ImageMenuElem( _("About"), *image, sigc::mem_fun(*this, &Win_GParted::menu_help_about) ) );
	menubar_main.items().push_back( Gtk::Menu_Helpers::MenuElem(_("_Help"), *menu ) );
}

void Win_GParted::init_toolbar() 
{
	//initialize and pack toolbar_main 
	hbox_toolbar.pack_start( toolbar_main );
	
	//NEW and DELETE
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::NEW));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_new) );		toolbar_main.append(*toolbutton);
	toolbutton ->set_tooltip(tooltips, _("Create a new partition in the selected unallocated space") );		
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::DELETE));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_delete) );	toolbar_main.append(*toolbutton);
	toolbutton ->set_tooltip(tooltips, _("Delete the selected partition") );		
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	
	//RESIZE/MOVE
	image = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON ) );
	toolbutton = Gtk::manage(new Gtk::ToolButton( *image, _("Resize/Move") ));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_resize) ); 	toolbar_main.append(*toolbutton);
	toolbutton ->set_tooltip(tooltips, _("Resize/Move the selected partition") );		
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	
	//COPY and PASTE
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::COPY));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_copy) );		toolbar_main.append(*toolbutton);
	toolbutton ->set_tooltip(tooltips, _("Copy the selected partition to the clipboard") );		
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::PASTE));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_paste) );		toolbar_main.append(*toolbutton);
	toolbutton ->set_tooltip(tooltips, _("Paste the partition from the clipboard") );		
	toolbar_main.append( *(Gtk::manage(new Gtk::SeparatorToolItem)) );
	
	//UNDO and APPLY
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::UNDO));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_undo) );		toolbar_main.append(*toolbutton); toolbutton ->set_sensitive( false );
	toolbutton ->set_tooltip(tooltips, _("Undo last operation") );		
	toolbutton = Gtk::manage(new Gtk::ToolButton(Gtk::Stock::APPLY));
	toolbutton ->signal_clicked().connect( sigc::mem_fun(*this, &Win_GParted::activate_apply) );		toolbar_main.append(*toolbutton); toolbutton ->set_sensitive( false );
	toolbutton ->set_tooltip(tooltips, _("Apply all operations") );		
	
	//initizialize and pack optionmenu_devices
	optionmenu_devices.set_menu( menu_devices );
	optionmenu_devices.signal_changed().connect( sigc::mem_fun(*this, &Win_GParted::optionmenu_devices_changed) );
	hbox_toolbar.pack_start( optionmenu_devices , Gtk::PACK_SHRINK );
}

void Win_GParted::init_popupmenu() 
{
	//fill menu_popup
	menu_popup.items().push_back( Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::NEW, sigc::mem_fun(*this, &Win_GParted::activate_new) ) );
	menu_popup.items().push_back( Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::DELETE, Gtk::AccelKey( 0xFFFF, Gdk::BUTTON1_MASK  ), sigc::mem_fun(*this, &Win_GParted::activate_delete) ) );
	menu_popup.items().push_back( Gtk::Menu_Helpers::SeparatorElem() );
	image = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_MENU ) );
	menu_popup.items().push_back( Gtk::Menu_Helpers::ImageMenuElem( _("Resize/Move"), *image, sigc::mem_fun(*this, &Win_GParted::activate_resize) ) );
	menu_popup.items().push_back( Gtk::Menu_Helpers::SeparatorElem() );
	menu_popup.items().push_back( Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::COPY, sigc::mem_fun(*this, &Win_GParted::activate_copy) ) );
	menu_popup.items().push_back( Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::PASTE, sigc::mem_fun(*this, &Win_GParted::activate_paste) ) );
	menu_popup.items().push_back( Gtk::Menu_Helpers::SeparatorElem() );
	image = manage( new Gtk::Image( Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU ) );
	menu_popup.items().push_back( Gtk::Menu_Helpers::ImageMenuElem( _("_Convert to"), *image, menu_convert ) ) ;
	menu_popup.items().push_back( Gtk::Menu_Helpers::SeparatorElem() );
	menu_popup.items().push_back( Gtk::Menu_Helpers::StockMenuElem( Gtk::Stock::DIALOG_INFO, sigc::mem_fun(*this, &Win_GParted::activate_info) ) );
	init_convert_menu() ;
	
	menu_popup .accelerate( *this ) ;  
}

void Win_GParted::init_convert_menu()
{
	filesystems.push_back( "ext2" );
	filesystems.push_back( "fat16" );
	filesystems.push_back( "fat32" );
	filesystems.push_back( "linux-swap" );
	filesystems.push_back( "reiserfs" ); 
	
	for ( unsigned int t=0;t < filesystems.size() ;t++ )
	{
		if ( filesystems[ t ] != selected_partition .filesystem )
		{
			color .set( selected_partition .Get_Color( filesystems[ t ] ) );
			hbox = manage( new Gtk::HBox() );
			
			//the colored square
			entry =  manage ( new Gtk::Entry() );
			entry->set_sensitive( false );
			entry->set_size_request( 12,12);
			entry->modify_base( entry->get_state(), color );
			hbox ->pack_start( *entry, Gtk::PACK_SHRINK   );
			
			//the label...
			label = manage( new Gtk::Label( " " + filesystems[ t ] ) );
			label ->set_alignment( Gtk::ALIGN_LEFT   );
				
			hbox ->pack_start( *label, Gtk::PACK_SHRINK   );
	
			menu_item = manage( new Gtk::MenuItem( *hbox ) ) ;
			menu_convert.items().push_back( *menu_item);
			menu_convert.items() .back() .signal_activate() .connect(  sigc::bind<Glib::ustring>(sigc::mem_fun(*this, &Win_GParted::activate_convert), filesystems[ t ] ) ) ;
	
		}			
	}
	
	menu_convert.show_all_children() ;
}

void Win_GParted::init_device_info()
{
	vbox = manage( new Gtk::VBox() ) ;
	
	//global device info
	table = manage( new Gtk::Table() ) ;
	table ->set_col_spacings(10 ) ;
	label = manage( new Gtk::Label() ) ;
	os << " <b>"  << _("Model") << ":\n " << _("Size") << ":\n " << _("Path") << ":\n " ;  
	
	//only show realpath if it's different from the short path...(hereby i assume if one device has a realpath, they all have. i guess this makes sense)
	if ( devices[ current_device ] ->Get_Path() != devices[ current_device ] ->Get_RealPath() )
		os << _("Real Path") << ":\n " ;
	
	os << "</b>" ;
	label ->set_markup( os.str() ) ; os.str("") ;
	table ->attach( *label, 0,1,0,1,Gtk::SHRINK);
	
	label_device_info1 .set_selectable( true ) ;
	table ->attach( label_device_info1, 1,2,0,1,Gtk::SHRINK);
	vbox ->pack_start( *table, Gtk::PACK_SHRINK ) ;
	
	//detailed device info
	table = manage( new Gtk::Table() ) ;
	table ->set_col_spacings(10 ) ;
	label = manage( new Gtk::Label() ) ;
	os << " <b>"  << _("DiskType") << ":\n " << _("Heads") << ":\n " << _("Sectors/Track") << ":\n " << _("Cylinders") << ":\n "<< _("Total Sectors") << ":\n ";  
	os << "</b>" ;
	label ->set_markup( os.str() ) ; os.str("") ;
	table ->attach( *label, 0,1,0,1,Gtk::SHRINK);
	
	label_device_info2 .set_selectable( true ) ;
	table ->attach( label_device_info2, 1,2,0,1,Gtk::SHRINK);
	vbox ->pack_start( *table, Gtk::PACK_SHRINK ) ;
	
	//add the vbox to the expander
	expander_device_info.add( *vbox );
	expander_device_info.set_use_markup( true );
	expander_device_info.set_label( "<b>" + (Glib::ustring) _( "Harddisk Information" ) + ":</b>" );
	
	//and add the expander to vbox_info
	vbox_info.set_spacing( 5 );
	vbox_info.pack_start( expander_device_info, Gtk::PACK_SHRINK );
}

void Win_GParted::init_operationslist() 
{
	//create listview for pending operations
	liststore_operations = Gtk::ListStore::create( treeview_operations_columns );
	treeview_operations.set_model( liststore_operations );
	treeview_operations.set_headers_visible( false );
	treeview_operations.append_column( "", treeview_operations_columns.operation_number );
	treeview_operations.append_column( "", treeview_operations_columns.operation_description );
	treeview_operations.get_column( 0 ) ->pack_start( treeview_operations_columns.operation_icon,false );
	treeview_operations.get_selection() -> set_mode( Gtk::SELECTION_NONE );

	//init scrollwindow_operations
	scrollwindow = manage( new Gtk::ScrolledWindow() ) ;
	scrollwindow ->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	scrollwindow ->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	scrollwindow ->add ( treeview_operations ) ;
	
	//set up the close and clear buttons and pack them in a vbox
	vbox = manage( new Gtk::VBox() ) ;
	//CLOSE
	button = manage( new Gtk::Button() );
	image = manage( new Gtk::Image( Gtk::Stock::CLOSE, Gtk::ICON_SIZE_MENU ) );
	button ->add( *image ) ;
	button ->set_relief( Gtk::RELIEF_NONE );
	tooltips .set_tip( *button, _("Hide operationslist") );
	button ->signal_clicked().connect( sigc::mem_fun( *this, &Win_GParted::close_operationslist ) );
	vbox ->pack_start( *button, Gtk::PACK_SHRINK );
	
	//CLEAR
	button = manage( new Gtk::Button() );
	image = manage( new Gtk::Image( Gtk::Stock::CLEAR, Gtk::ICON_SIZE_MENU ) );
	button ->add( *image ) ;
	button ->set_relief( Gtk::RELIEF_NONE );
	tooltips .set_tip( *button, _("Clear operationslist") );
	button ->signal_clicked().connect( sigc::mem_fun( *this, &Win_GParted::clear_operationslist ) );
	vbox ->pack_start( *button, Gtk::PACK_SHRINK );
	
	//add vbox and scrollwindow_operations to hbox_operations
	hbox_operations .pack_start( *vbox, Gtk::PACK_SHRINK );
	hbox_operations .pack_start( *scrollwindow, Gtk::PACK_EXPAND_WIDGET  );
}

void Win_GParted::init_hpaned_main() 
{
	//left scrollwindow (holds device info)
	scrollwindow = manage( new Gtk::ScrolledWindow() ) ;
	scrollwindow ->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	scrollwindow ->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);

	hpaned_main.pack1( *scrollwindow, true,true );
	scrollwindow ->add( vbox_info );

	//right scrollwindow (holds treeview with partitions)
	scrollwindow = manage( new Gtk::ScrolledWindow() ) ;
	scrollwindow ->set_shadow_type(Gtk::SHADOW_ETCHED_IN);
	scrollwindow ->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
	
	//connect signal and add treeview_detail
	treeview_detail .signal_mouse_click.connect( sigc::mem_fun( this, &Win_GParted::mouse_click) );
	scrollwindow ->add ( treeview_detail );
	hpaned_main.pack2( *scrollwindow, true,true );
	
	hpaned_main.set_position( 260 );
}

void Win_GParted::Find_Devices() 
{
	for ( unsigned int t=0;t<devices.size() ; t++ )
		delete devices[ t ] ;
	
	devices.clear() ;
	
	//try to find all available devices...
	ped_device_probe_all ();
	
	//construct a list of device objects
	PedDevice *device = ped_device_get_next (NULL);
		
	while (  device )
	{  
		str_devices.push_back( device ->path  ) ;
		device = ped_device_get_next (device) ;
	}
	
	for ( unsigned int t=0;t<str_devices.size() ; t++ )
	{
		temp_device = new GParted::Device( str_devices[t] );
		temp_device ->Get_Length() > 0 ? devices.push_back( temp_device ) : delete temp_device  ;
	}
	
	str_devices.clear() ;

	//fill optionmenu_devices
	menu_devices.items() .clear() ;
	for ( unsigned int i=0;i<devices.size();i++ )
	{ 
		hbox = manage( new Gtk::HBox() );
		
		//the image...
		image = manage( new Gtk::Image( "/usr/share/icons/gnome/24x24/devices/gnome-dev-harddisk.png" ) );
		hbox ->pack_start( *image, Gtk::PACK_SHRINK   );
		
		//the label...
		os << Sector_To_MB( devices[i] ->Get_Length() ) ;
		label = manage( new Gtk::Label( " " + devices[i] ->Get_Path() + "\t (" + os.str() + " MB)" ) ); os.str("") ;
		label ->set_alignment( Gtk::ALIGN_LEFT   );
		hbox ->pack_start( *label, Gtk::PACK_SHRINK   );
	
		menu_item = manage( new Gtk::MenuItem( *hbox ) ) ;
		menu_devices .items().push_back( *menu_item );
	}
	
}

void Win_GParted::Fill_Label_Device_Info( ) 
{
	//global info...
	os << devices[ current_device ] ->Get_Model() << "\n" ;
	os << Sector_To_MB( devices[ current_device ] ->Get_Length() ) << " MB\n" ;
	os << devices[ current_device ] ->Get_Path() << "\n" ;
	
	//only show realpath if it's diffent from the short path...
	if ( devices[ current_device ] ->Get_Path() != devices[ current_device ] ->Get_RealPath() )
		os << devices[ current_device ] ->Get_RealPath() << "\n" ;
	
	label_device_info1.set_text( os.str() ) ;  os.str("") ;
	
	//detailed info
	os << devices[ current_device ] ->Get_DiskType() << "\n" ;
	os << devices[ current_device ] ->Get_Heads() << "\n" ;
	os << devices[ current_device ] ->Get_Sectors() << "\n" ;
	os << devices[ current_device ] ->Get_Cylinders() << "\n" ;
	os << devices[ current_device ] ->Get_Length() << "\n" ;
	
	label_device_info2.set_text( os.str() ) ;  os.str("") ;
}

bool Win_GParted::on_delete_event(GdkEventAny *event)
{
	return ! Quit_Check_Operations();
}	

void Win_GParted::Add_Operation( OperationType operationtype, const Partition & new_partition)
{
	Operation operation( devices[ current_device ] ,  devices[ source_device ] , selected_partition, new_partition, operationtype );
	
	operations.push_back( operation );
	
	allow_undo( true );
	allow_apply( true );
	
	Refresh_Visual( );
	
	if ( operations.size() == 1 ) //first operation, open operationslist
	{
		int x,y;	this ->get_size( x, y );
		y -= 300;
		
		for ( int t=vpaned_main.get_position() ; t > y ; t-=3 )
		{
			vpaned_main.set_position( t );
			while (Gtk::Main::events_pending())  Gtk::Main::iteration();
		}
	}

	//make scrollwindow focus on the last operation in the list	
	Gtk::TreeIter iter = liststore_operations->children().end() ;
	iter-- ;
	treeview_operations .set_cursor( (Gtk::TreePath) (Gtk::TreeRow) *iter);
	
}

void Win_GParted::Refresh_Visual( )
{
	std::vector<Partition> partitions = devices[current_device] ->Get_Partitions()  ; 
	liststore_operations ->clear();
	
	//make all operations visible
	for (unsigned int t=0;t<operations.size();t++ )
	{
		if ( operations[t]. device ->Get_Path() == devices[ current_device ] ->Get_Path() )
			partitions = operations[t].Apply_Operation_To_Visual( partitions ) ;
	
		treerow = *(liststore_operations->append());
		treerow[ treeview_operations_columns.operation_number ] = t +1;
		treerow[ treeview_operations_columns.operation_description ] = operations[t].Get_String() ;
		switch ( operations[t].operationtype )
		{		
			case GParted::DELETE				:	treerow[ treeview_operations_columns.operation_icon ] =render_icon(Gtk::Stock::DELETE, Gtk::ICON_SIZE_MENU);	
																break;
			case	GParted::CREATE				:	treerow[ treeview_operations_columns.operation_icon ] =render_icon(Gtk::Stock::NEW, Gtk::ICON_SIZE_MENU);
																break;
			case	GParted::RESIZE_MOVE	:	treerow[ treeview_operations_columns.operation_icon ] =render_icon(Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_MENU);
																break;
			case	GParted::CONVERT			:	treerow[ treeview_operations_columns.operation_icon ] =render_icon(Gtk::Stock::CONVERT, Gtk::ICON_SIZE_MENU);
																break;
			case	GParted::COPY					:	treerow[ treeview_operations_columns.operation_icon ] =render_icon(Gtk::Stock::COPY, Gtk::ICON_SIZE_MENU);
																break;
		}
		
	}
	
	//set new statusbartext
	statusbar .pop() ;
	os << operations.size() << " " ;
	operations.size() != 1 ? os << _( "operations pending" ) : os << _( "operation pending" ) ;
	statusbar .push( os.str() );os.str("");
	
	if ( ! operations.size() ) 
	{
		allow_undo( false );
		allow_apply( false );
	}
			
	//count primary's, check for extended and logic and see if any logical is busy
	any_logic = any_extended = false;
	primary_count = highest_logic_busy = 0;
	for (unsigned int t=0;t<partitions.size();t++ )
	{
		if ( partitions[ t ] .partition == copied_partition .partition )
			copied_partition = partitions[ t ] ;
		
		switch ( partitions[t].type )
		{
			case GParted::PRIMARY		:	primary_count++;
															break;
			case GParted::EXTENDED		:	any_extended = true;
															primary_count++;
															break;
			case GParted::LOGICAL		:	any_logic = true;
															if ( partitions[t].busy && partitions[t].partition_number > highest_logic_busy )
																highest_logic_busy = partitions[t].partition_number ;
															break;
			default									:	break;
		}
	}
	
	//vbox visual
	if ( vbox_visual_disk != NULL )
	{
		hbox_visual.remove( *vbox_visual_disk );
		s2.disconnect();
		delete ( vbox_visual_disk );
	}
	
	vbox_visual_disk = new VBox_VisualDisk (  partitions, devices[ current_device ] ->Get_Length()  );
	s2 = vbox_visual_disk->signal_mouse_click.connect( sigc::mem_fun( this, &Win_GParted::mouse_click) );
	hbox_visual.pack_start( *vbox_visual_disk, Gtk::PACK_EXPAND_PADDING );

	//treeview details
	treeview_detail .Load_Partitions( partitions ) ;

	allow_new( false );	allow_delete( false );allow_resize( false );allow_copy( false );allow_paste( false );	
		
	this->show_all_children();
	
}

bool Win_GParted::Quit_Check_Operations()
{
	if ( operations.size() )
	{
		os << "<span weight=\"bold\" size=\"larger\">" + (Glib::ustring) _( "Quit GParted?" ) + "</span>\n\n" ;
		os << operations.size() << " ";
		operations.size() == 1 ? os << "operation is currently pending..." : os << "operations are currently pending..." ;
				
		Gtk::MessageDialog dialog( *this, os.str() , true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true);os.str("");
		dialog .add_button( Gtk::Stock::QUIT, Gtk::RESPONSE_CLOSE );
		dialog .add_button( Gtk::Stock::CANCEL,Gtk::RESPONSE_CANCEL );
		
		if ( dialog.run() == Gtk::RESPONSE_CANCEL ) return false;//don't close GParted
	}

	return true; //close GParted
}

void Win_GParted::Set_Valid_Operations()
{
	allow_new( false );	allow_delete( false ); allow_resize( false );allow_copy( false ); allow_paste( false ); allow_convert( false );
		
	//we can't perform any operation on a busy (mounted) filesystem
	if ( selected_partition.busy )
		return;
	
	//UNALLOCATED
	if ( selected_partition.type == GParted::UNALLOCATED )
	{
		allow_new( true );
		
		//find out if there is a copied partition and if it fits inside this unallocated space
		if ( copied_partition .partition != "NONE" && copied_partition .Get_Length_MB() < selected_partition .Get_Length_MB() )
			allow_paste( true ) ;
		
		return ;
	}
	
	//if there was an error reading the filesystem we allow delete and convert ( see also Device::Get_Used_Sectors() )
	if ( selected_partition .error != "" )
	{
		allow_delete( true ) ;
		allow_convert( true ) ;
		return;
	}
		
	//PRIMARY and LOGICAL
	if (  selected_partition.type != GParted::EXTENDED )
	{
		allow_delete( true ) ;
		allow_convert( true ) ;
		
		//find out if resizing/moving and copying is possible
		if  (	selected_partition.filesystem == "ext2"			 ||
				selected_partition.filesystem == "ext3"			 ||
				selected_partition.filesystem == "fat16"		 ||
				selected_partition.filesystem == "fat32"		 ||
				selected_partition.filesystem == "linux-swap" 
			)	
		{
			allow_resize( true ) ;
			
			//only allow copying of real partitions
			if ( selected_partition .partition.substr( 0, 3 ) != "New" && selected_partition .partition.substr( 0, 4 ) != "copy" )
				allow_copy( true ) ;
		}
				
		return ;
	}
	
	
	//EXTENDED
	else if ( selected_partition.type == GParted::EXTENDED )
	{
		if (  ! any_logic ) //deletion is only allowed when there are nog logical partitions inside.
			allow_delete( true ) ;
		
			allow_resize( true ); 
	}		
	
}

void Win_GParted::Set_Valid_Convert_Filesystems() 
{
	//disable conversion to the same filesystem
	for ( unsigned int t=0;t<filesystems .size() ; t++ )
		filesystems[ t ] == selected_partition .filesystem ? menu_convert.items()[ t ] .set_sensitive( false ) : menu_convert.items()[ t ] .set_sensitive( true ) ;
	
	menu_convert.items() .back() .set_sensitive( false ) ; //disable reiserfs for the time being...
}

void Win_GParted::close_operationslist() 
{
	treeview_detail .columns_autosize() ; //seemed a nice place for it..
	
	int x,y;	this ->get_size( x, y );
	y -= 210 ; //height of whole app - menubar - visualdisk - statusbar ....
	for ( int t=vpaned_main.get_position() ; t < y ; t+=3 )
	{
		vpaned_main.set_position( t );
		while (Gtk::Main::events_pending())  Gtk::Main::iteration();
	}

}

void Win_GParted::clear_operationslist() 
{
	if ( operations.size() )
	{
		operations .clear() ;
		Refresh_Visual() ;
		close_operationslist() ;
	}
}

void Win_GParted::optionmenu_devices_changed( )
{	
	//set new current device
	current_device = optionmenu_devices.get_history() ;
	
	//refresh label_device_info
	Fill_Label_Device_Info( );
	
	//rebuild visualdisk and treeview
	Refresh_Visual( );
}

void Win_GParted::menu_gparted_refresh_devices()
{
	//find out if there was any change in available devices (think about flexible media like zipdisks/usbsticks/whatever ;-) )
	Find_Devices() ;
	
	//refresh de pointer to the device in every operation
	for ( unsigned int t=0; t< operations.size() ; t++ )
		for ( unsigned int i=0; i< devices.size() ; i++ )
		{
			if ( operations[t] .device_path == devices[ i ] ->Get_Path() )
				operations[t] .device = devices[ i ] ; 
			if ( operations[t] .source_device_path == devices[ i ] ->Get_Path() )
				operations[t] .source_device = devices[ i ] ; 
		}
		
	//check if current_device is still available (think about hotpluggable shit like usbdevices)
	if ( current_device >= devices .size() )
			current_device = 0 ;	
				
	//rebuild visualdisk and treeview
	Refresh_Visual( );
}

void Win_GParted::menu_gparted_quit()
{
	if ( Quit_Check_Operations() )
		this->hide();
}

void Win_GParted::menu_help_contents()
{
	os << _("Sorry, not yet implemented.") << "\n" << _( "Please visit http://gparted.sf.net for more information and support.") ;
	Gtk::MessageDialog dialog( *this,os.str() ,false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true); os.str("") ;
	dialog.run();
}


void Win_GParted::menu_help_about()
{
	Dialog_About dialog ;
	dialog .set_transient_for( *this ) ;
	
	dialog.run();
}

void Win_GParted::mouse_click( GdkEventButton *event, const Partition & partition )
{
	selected_partition = partition;
	
	Set_Valid_Operations() ;
	
	
	treeview_detail .Set_Selected( partition );
	vbox_visual_disk ->Set_Selected( partition );
	
	if ( event->type == GDK_2BUTTON_PRESS )
		activate_info() ;
	else if ( event->button == 3 )  //right-click
	{
		//prepare convert menu
		if ( selected_partition.type != GParted::UNALLOCATED )
			Set_Valid_Convert_Filesystems() ;
		
		menu_popup.popup( event->button, event->time );
	}
}


void Win_GParted::activate_resize()
{
	//show warning when one tries to resize a fat16 filesystem
	if ( selected_partition .filesystem == "fat16" )
	{
		os << "<span weight=\"bold\" size=\"larger\">" ;
		os << _( "Are you sure you want to resize/move this partition?" ) << "</span>\n\n" ;
		os << _( "Resizing a fat16 partition can be quite tricky! Especially growing such a partition is very error-prone. It is advisable to first convert the filesystem to fat32.") << "\n";
						
		Gtk::MessageDialog dialog( *this,os.str(), true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_CANCEL, true); os.str("") ;
		//pffff this whole mess only for this f*cked up filesystem :-(
		Gtk::Button button_resize_move ;
		Gtk::HBox hbox_resize_move;
		Gtk::Label label_resize_move( _("Resize/Move") );
		
		image = manage( new Gtk::Image( Gtk::Stock::GOTO_LAST, Gtk::ICON_SIZE_BUTTON ) );
		hbox_resize_move .pack_start( *image, Gtk::PACK_SHRINK ) ;
		hbox_resize_move .pack_start( label_resize_move, Gtk::PACK_SHRINK ) ;
		button_resize_move .add( hbox_resize_move ) ;
				
		dialog .add_action_widget ( button_resize_move,Gtk::RESPONSE_OK ) ;
		dialog .show_all_children() ;
		
		if ( dialog.run() == Gtk::RESPONSE_CANCEL )
			return ;
		
	}
	
	std::vector <Partition> partitions = devices[ current_device ] -> Get_Partitions() ;
	
	if ( operations.size() )
		for (unsigned int t=0;t<operations.size();t++ )
			if ( operations[t]. device ->Get_Path() == devices[ current_device ] ->Get_Path() )
				partitions = operations[t].Apply_Operation_To_Visual( partitions ) ;
	
	
	Dialog_Partition_Resize_Move dialog;
	dialog.Set_Data( selected_partition, partitions);
	dialog .set_transient_for( *this ) ;	
			
	if ( dialog.run() == Gtk::RESPONSE_OK )
	{
		dialog.hide() ;//i want to be sure the dialog is gone _before_ operationslist shows up (only matters if first operation)
		
		//if selected_partition is NEW we simply remove the NEW operation from the list and add it again with the new size and position ( unless it's an EXTENDED )
		if ( selected_partition .partition.substr( 0, 3 ) == "New" && selected_partition.type != GParted::EXTENDED )
		{
			//remove operation which creates this partition
			for ( unsigned int t=0;t<operations.size() ; t++ )
			{
				if ( operations[t] .partition_new .partition == selected_partition .partition )
				{
					operations.erase( operations .begin() + t ) ;
					
					//And add the new partition to the end of the operations list
					Add_Operation( GParted::CREATE, dialog.Get_New_Partition() );
					
					break;
				}
			}
		}
		else//normal move/resize on existing partition
			Add_Operation( GParted::RESIZE_MOVE, dialog.Get_New_Partition() );
		
	}
}

void Win_GParted::activate_copy()
{
	copied_partition = selected_partition ;
	source_device = current_device ;
}

void Win_GParted::activate_paste()
{
	//Display error if user tries to create a fifth primary partition 
	if ( ! selected_partition.inside_extended && primary_count >= 4 )
	{
		os << "<span weight=\"bold\" size=\"larger\">" ;
		os << _( "It's not possible to create more then four primary partitions") << "</span>\n\n" ;
		os << _( "If you want more then four partitions you should create an extended partition. Such an partition can contain at least 24 other partitions.") ;
				
		Gtk::MessageDialog dialog( *this, os.str() ,true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true); os.str("") ;
		dialog.run();
		return;
	}
	
	Dialog_Partition_Copy dialog ;
	dialog .Set_Data( selected_partition, copied_partition ) ;
	dialog.set_transient_for( *this );
	
	if ( dialog.run() == Gtk::RESPONSE_OK )
	{
		dialog.hide() ;//i want to be sure the dialog is gone _before_ operationslist shows up (only matters if first operation)
		Add_Operation( GParted::COPY, dialog.Get_New_Partition() );		
	}
	
}

void Win_GParted::activate_new()
{
	//Display error if user tries to create more primary partitions then the partition table can hold. 
	if ( ! selected_partition.inside_extended && primary_count >= devices[ current_device ] ->Get_Max_Amount_Of_Primary_Partitions() )
	{
		os << "<span weight=\"bold\" size=\"larger\">" ;
		sprintf( c_buf, _("It is not possible to create more then %d primary partitions"), devices[ current_device ] ->Get_Max_Amount_Of_Primary_Partitions() ) ;
		os << c_buf << "</span>\n\n" ;
		os << _( "If you want more partitions you should first create an extended partition. Such an partition can contain other partitions.") ;
				
		Gtk::MessageDialog dialog( *this, os.str() ,true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true); os.str("") ;
		dialog.run();
		return;
	}
	
	Dialog_Partition_New dialog;
	dialog.Set_Data( selected_partition, any_extended, new_count ) ;
	dialog.set_transient_for( *this );
	
	if ( dialog.run() == Gtk::RESPONSE_OK )
	{
		dialog.hide() ;//i want to be sure the dialog is gone _before_ operationslist shows up (only matters if first operation)
		new_count++;
		Add_Operation( GParted::CREATE, dialog.Get_New_Partition() );
	}
		
}

void Win_GParted::activate_delete()
{ 
	//since logicals are *always* numbered from 5 to <last logical> there can be a shift in numbers after deletion.
	//e.g. consider /dev/hda5 /dev/hda6 /dev/hda7. Now after removal of /dev/hda6, /dev/hda7 is renumbered to /dev/hda6
	//the new situation is now /dev/hda5 /dev/hda6. If /dev/hda7 was mounted the OS cannot find /dev/hda7 anymore and the results aren't that pretty
	//it seems best to check for this and prohibit deletion with some explanation to the user.
	if ( selected_partition.type == GParted::LOGICAL &&  selected_partition .partition.substr( 0, 3 ) != "New"  && selected_partition.partition_number < highest_logic_busy )
	{	
		os << " " << selected_partition.partition_number ;
		Gtk::MessageDialog dialog( *this, "<span weight=\"bold\" size=\"larger\">" + (Glib::ustring) _( "Unable to delete partition!") + "</span>\n\n" + (Glib::ustring) _( "Please unmount any logical partitions having a number higher than")  + os.str() ,true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);os.str("");
		dialog.run() ; return;
	}
	
	os << "<span weight=\"bold\" size=\"larger\">" + (Glib::ustring) _( "Are you sure you want to delete" ) + " " + selected_partition.partition + " ?</span>" ;
	if ( selected_partition .partition == copied_partition .partition )
		os << "\n\n" + (Glib::ustring) _( "After deletion this partition is no longer available for copying.") ;
	
	Gtk::MessageDialog dialog( *this, os.str() ,true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE, true); os.str("");
	dialog.set_title( _("Delete") + (Glib::ustring)" " + selected_partition.partition + " (" + selected_partition.filesystem + ")" );
	dialog.add_button( Gtk::Stock::CANCEL,Gtk::RESPONSE_CANCEL );
	dialog.add_button( Gtk::Stock::DELETE, Gtk::RESPONSE_OK );
	
	dialog.show_all_children();
	if ( dialog.run() == Gtk::RESPONSE_OK )
	{
		dialog.hide() ;//i want to be sure the dialog is gone _before_ operationslist shows up (only matters if first operation)
		
		//if deleted partition was on the clipboard we erase it...
		if ( selected_partition .partition == copied_partition .partition )
		{
			copied_partition .partition = "NONE" ;
			source_device = current_device ;
		}
		
		//if deleted one is NEW, it doesn't make sense to add it to the operationslist, we erase its creation
		//and possible modifications like resize etc.. from the operationslist.   Calling Refresh_Visual will wipe every memory of its existence ;-)
		if ( selected_partition .partition.substr( 0, 3 ) == "New" )
		{
			//remove all operations done on this new partition (this includes creation)	
			for ( int t=0;t<(int) operations.size() ; t++ ) //i removed the unsigned 'cause t will be negative at times...
			{
				if ( operations[t] .partition_new .partition == selected_partition .partition )
				{
					operations.erase( operations .begin() + t ) ;
					t-- ;
				}
			}
				
			
			//determine lowest possible new_count
			new_count = 0 ; 
			for ( unsigned int t=0;t<operations.size() ; t++ )
				if ( operations[t] .partition_new .partition .substr( 0, 3 ) == "New" && operations[t] .partition_new .partition_number > new_count )
					new_count = operations[t] .partition_new .partition_number ;
			
			new_count += 1 ;
				
			Refresh_Visual( ); 
				
			if ( ! operations .size() )
				close_operationslist() ;
		}
		else //deletion of a real partition...
			Add_Operation( GParted::DELETE, selected_partition ); //in this case selected_partition is just a "dummy" 
		
		
	}
}

void Win_GParted::activate_info()
{
	Dialog_Partition_Info dialog( selected_partition );
	dialog.set_transient_for( *this );
	dialog.run();
}

void Win_GParted::activate_convert( const Glib::ustring & new_fs )
{
	//standard warning..
	Gtk::MessageDialog dialog( *this, "<span weight=\"bold\" size=\"larger\">" + (Glib::ustring) _( "Are you sure you want to convert this filesystem to") + " " + new_fs + " ?</span>\n\n" + (Glib::ustring) _( "This operation will destroy all data on") + " " + selected_partition .partition ,true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_CANCEL, true);
	dialog. add_button( Gtk::Stock::CONVERT, Gtk::RESPONSE_OK ) ;
	dialog. show_all_children() ;
	
	if ( dialog.run() == Gtk::RESPONSE_CANCEL )
		return ;
	
	dialog.hide() ;//i want to be sure the dialog is gone _before_ operationslist shows up (only matters if first operation)
	
	//check for the FAT limits...
	if ( new_fs  == "fat16" || new_fs  == "fat32" )
	{
		Glib::ustring message ;
		
		if ( new_fs == "fat16" && selected_partition.Get_Length_MB() < 32 )
			message =  (Glib::ustring) _("Can not convert this filesystem to fat16.") + "</span>\n\n" + (Glib::ustring) _( "A fat16 filesystem requires a partition of at least 32 MB.") ;
		else	if ( new_fs == "fat16" && selected_partition.Get_Length_MB() > 1023 )	
			message = (Glib::ustring) _("Can not convert this filesystem to fat16.") + "</span>\n\n" + (Glib::ustring) _( "A partition with a fat16 filesystem has a maximum size of 1023 MB.");
		else	if ( new_fs == "fat32" && selected_partition.Get_Length_MB() < 256 )	
			message = (Glib::ustring) _("Can not convert this filesystem to fat32.") + "</span>\n\n" + (Glib::ustring) _( "A fat32 filesystem requires a partition of at least 256 MB.");
		
		if ( message != "" )
		{
			Gtk::MessageDialog dialog( *this, "<span weight=\"bold\" size=\"larger\">" + message ,true, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK, true);
			dialog.run() ;
			return ;
		}
	
	}
	
	//ok we made it :P lets create an fitting partition object
	Partition part_temp;
	part_temp .Set( selected_partition .partition, selected_partition .partition_number, selected_partition .type, new_fs, selected_partition .sector_start, selected_partition .sector_end, -1, selected_partition .inside_extended, false ) ;
	
	
	//if selected_partition is NEW we simply remove the NEW operation from the list and add it again with the new filesystem
	if ( selected_partition .partition.substr( 0, 3 ) == "New"  )
	{
		//remove operation which creates this partition
		for ( unsigned int t=0;t<operations.size() ; t++ )
		{
			if ( operations[t] .partition_new .partition == selected_partition .partition )
			{
				operations.erase( operations .begin() + t ) ;
				
				//And add the new partition to the end of the operations list
				Add_Operation( GParted::CREATE, part_temp);
					
				break;
			}
		}
	}
	else//normal converting of an existing partition
		Add_Operation( GParted::CONVERT, part_temp ) ;
}

void Win_GParted::activate_undo()
{
	//when undoing an creation it's safe to decrease the newcount by one
	if ( operations.back() .operationtype == GParted::CREATE )
		new_count -- ;
	
	operations.erase( operations.end() );
	
	Refresh_Visual();
	
	if ( ! operations .size() )
		close_operationslist() ;
	
}

void Win_GParted::activate_apply()
{
	Gtk::MessageDialog dialog( *this, "<span weight=\"bold\" size=\"larger\">" + (Glib::ustring) _( "Are you sure you want to apply the pending operations?" ) + "</span>\n\n" + (Glib::ustring) _( "It is recommended to backup valueable data before proceeding.") ,true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_NONE, true);
	dialog.set_title( _( "Apply operations to harddisk" ) );
	
	dialog.add_button( Gtk::Stock::CANCEL,Gtk::RESPONSE_CANCEL );
	dialog.add_button( Gtk::Stock::APPLY, Gtk::RESPONSE_OK );
	
	dialog.show_all_children();
	if ( dialog.run() == Gtk::RESPONSE_OK )
	{
		dialog.hide() ; //hide confirmationdialog
		
		apply = true;
		dialog_progress = new Dialog_Progress ( operations.size(), operations.front().Get_String() ) ;
		s3 = dispatcher_next_operation.connect( sigc::mem_fun(*dialog_progress, &Dialog_Progress::Set_Next_Operation) );
		thread_operations = Glib::Thread::create(SigC::slot_class(*this, &Win_GParted::apply_operations_thread), true);
		
		dialog_progress->set_transient_for( *this );
		while ( dialog_progress->run() != Gtk::RESPONSE_OK  ) 
			apply = false;//finish current operation . then stop applying operations
			
		//after hiding the progressdialog
		s3.disconnect();
		delete ( dialog_progress ) ;
		thread_operations->join() ;
		
		//make list of involved devices which have at least one busy partition..
		std::vector <Glib::ustring> devicenames ;
		for (unsigned int t=0; t<operations .size(); t++ )
			if ( 	std::find( devicenames .begin(), devicenames .end() , operations[ t ] .device ->Get_Path() ) == devicenames .end() &&
					operations[ t ] .device ->Get_any_busy()
				)
				devicenames .push_back( operations[ t ] .device ->Get_Path() ) ;
		
		//show warning if necessary
		if ( devicenames .size() )
		{
			os << "<span weight=\"bold\" size=\"larger\">" ;
			os << _("The kernel was unable to re-read the partition table on :") << "\n";
			for (unsigned int t=0; t<devicenames .size(); t++ )
				os << "- " << devicenames[ t ] << "\n";
			os << "</span>\n\n" << _( "This means Linux won't know anything about the modifications you made until you reboot.") << "\n\n" ;
			if ( devicenames .size() > 1 )
				os << _( "You should reboot your computer before doing anything with these devices.") ; 
			else 
				os << _( "You should reboot your computer before doing anything with this device.") ; 
				
			Gtk::MessageDialog dialog( *this,  os.str()  ,true, Gtk::MESSAGE_WARNING, Gtk::BUTTONS_OK, true); os.str("") ;
			dialog.run() ;
		}					
		
				
		//wipe operations...
		operations.clear();
		
		//reread devices and their layouts...
		menu_gparted_refresh_devices() ;
					
		//then clean up the rest
		new_count = 1;
		close_operationslist() ;
	
	}
	
}
//-------AFAIK it's not possible to use a C++ memberfunction as a callback for a C libary function (if you know otherwise, PLEASE contact me------------
Dialog_Progress *dp;
Glib::Dispatcher dispatcher_current_operation;

void progress_callback(PedTimer * timer, void *context  )
{
	if (  time(NULL) - timer ->start  > 0 )
	{
		dp ->time_left = timer ->predicted_end - time(NULL) ;
		dp ->fraction_current =  timer ->frac ;
		dispatcher_current_operation() ;
	}
}
//---------------------------------------------------------------------------------------

void Win_GParted::apply_operations_thread()
{ 
	dp = dialog_progress ;
	sigc::connection signal = dispatcher_current_operation.connect( sigc::mem_fun(*dialog_progress, &Dialog_Progress::Set_Progress_Current_Operation) );
	PedTimer *timer = ped_timer_new( progress_callback, NULL ) ;
	
	for ( unsigned int t=0;t<operations.size() && apply ;t++ )
	{ 	
		operations[t] .Apply_To_Disk( timer );
		
		if ( t < operations .size() -1 )
		{
			dialog_progress ->current_operation = operations[ t +1 ] .Get_String() ;
			dispatcher_next_operation() ;
		}
	}
	
	dialog_progress->response( Gtk::RESPONSE_OK );
	signal.disconnect() ;
}


} // GParted
