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
 
#include "../include/TreeView_Detail.h"

namespace GParted
{ 

TreeView_Detail::TreeView_Detail()
{
	block = false ;
	
	treestore_detail = Gtk::TreeStore::create( treeview_detail_columns );
	set_model( treestore_detail );
	set_rules_hint( true );
	treeselection = get_selection();
	treeselection ->signal_changed() .connect( sigc::mem_fun( *this, &TreeView_Detail::on_selection_changed ) );

	//append columns
	append_column( _("Partition"), treeview_detail_columns .path );
	append_column( _("Name"), treeview_detail_columns.name );
	append_column( _("File System"), treeview_detail_columns .color );
	append_column( _("Mount Point"), treeview_detail_columns .mountpoint );
	append_column( _("Label"), treeview_detail_columns .label );
	append_column( _("Size"), treeview_detail_columns .size );
	append_column( _("Used"), treeview_detail_columns .used );
	append_column( _("Unused"), treeview_detail_columns .unused );
	append_column( _("Flags"), treeview_detail_columns .flags );
	
	//icons
	get_column( 0 ) ->pack_start( treeview_detail_columns .icon2, false );
	get_column( 0 ) ->pack_start( treeview_detail_columns .icon1, false );

	//PARTITION
	//colored text in Partition column 
	Gtk::CellRendererText *cell_renderer_text = 
		dynamic_cast<Gtk::CellRendererText*>( get_column( 0 ) ->get_first_cell_renderer() );
	get_column( 0 ) ->add_attribute( cell_renderer_text ->property_foreground(), 
					 treeview_detail_columns .text_color );
	
	//FILE SYSTEM
	//file system text
	get_column( 2 )->pack_start( treeview_detail_columns.filesystem, true );

	//colored text in File System column 
	std::vector<Gtk::CellRenderer*> renderers = get_column( 2 )->get_cell_renderers();
	cell_renderer_text = dynamic_cast<Gtk::CellRendererText*>( renderers .back() ) ;
	get_column( 2 )->add_attribute( cell_renderer_text->property_foreground(),
	                                treeview_detail_columns.text_color );

	//pixbuf and text are both left aligned
	get_column( 2 )->get_first_cell_renderer()->property_xalign() = Gtk::ALIGN_LEFT;
	cell_renderer_text ->property_xalign() = Gtk::ALIGN_LEFT ;
	
	//MOUNT POINT
	//colored text in mount point column 
	cell_renderer_text = dynamic_cast<Gtk::CellRendererText*>( get_column( 3 )->get_first_cell_renderer() );
	get_column( 3 )->add_attribute( cell_renderer_text->property_foreground(),
	                                treeview_detail_columns.mount_text_color );

	//set alignment of numeric columns to right
	for( short t = 5 ; t < 8 ; t++ )
		get_column_cell_renderer( t ) ->property_xalign() = 1 ;

	//expand columns and centeralign the headertext
	for( short t = 5 ; t < 9 ; t++ )
	{
		get_column( t ) ->set_expand( true ) ;
		get_column( t ) ->set_alignment( 0.5 ) ;
	}
}

void TreeView_Detail::load_partitions( const std::vector<Partition> & partitions ) 
{
	bool mountpoints = false, labels = false, names = false;
	treestore_detail ->clear() ;

	load_partitions( partitions, mountpoints, labels, names );

	get_column( 1 )->set_visible( names );
	get_column( 3 )->set_visible( mountpoints );
	get_column( 4 )->set_visible( labels );

	columns_autosize();
	expand_all() ;
}

void TreeView_Detail::set_selected( const Partition * partition_ptr )
{
	block = true ;
	set_selected( treestore_detail->children(), partition_ptr );
	block = false ;
}

void TreeView_Detail::clear()
{
	treestore_detail ->clear() ;
}

void TreeView_Detail::load_partitions( const std::vector<Partition> & partitions,
                                       bool & mountpoints,
                                       bool & labels,
                                       bool & names,
                                       const Gtk::TreeRow & parent_row )
{
	Gtk::TreeRow row ;
	for ( unsigned int i = 0 ; i < partitions .size() ; i++ ) 
	{	
		row = parent_row ? *( treestore_detail ->append( parent_row .children() ) ) : *( treestore_detail ->append() ) ;
		create_row( row, partitions[ i ] );
		
		if ( partitions[ i ] .type == GParted::TYPE_EXTENDED )
			load_partitions( partitions[i].logicals, mountpoints, labels, names, row );

		if ( partitions[ i ] .get_mountpoints() .size() )
			mountpoints = true ;
					
		if ( ! partitions[ i ].get_filesystem_label().empty() )
			labels = true ;

		if ( ! partitions[i].name.empty() )
			names = true;
	}
}

bool TreeView_Detail::set_selected( Gtk::TreeModel::Children rows,
                                    const Partition * partition_ptr, bool inside_extended )
{
	for ( unsigned int t = 0 ; t < rows .size() ; t++ )
	{
		if ( * static_cast<const Partition *>( rows[t][treeview_detail_columns.partition_ptr] ) == *partition_ptr )
		{
			if ( inside_extended )
				expand_all() ;
			
			set_cursor( static_cast<Gtk::TreePath>( rows[ t ] ) ) ;
			return true ;
		}

		if ( set_selected( rows[t].children(), partition_ptr, true ) )
			return true ;
	}
	
	return false ;
}

void TreeView_Detail::create_row( const Gtk::TreeRow & treerow, const Partition & partition )
{
	if ( partition .busy )
		treerow[ treeview_detail_columns .icon1 ] = 
			render_icon( Gtk::Stock::DIALOG_AUTHENTICATION, Gtk::ICON_SIZE_BUTTON );
	
	if ( partition .messages .size() > 0 )
	{
		if ( ! static_cast< Glib::RefPtr<Gdk::Pixbuf> >( treerow[ treeview_detail_columns .icon1 ] )  )
			treerow[ treeview_detail_columns .icon1 ] = 
				render_icon( Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON );
		else
			treerow[ treeview_detail_columns .icon2 ] = 
				render_icon( Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON );
	}

	
	treerow[ treeview_detail_columns .path ] = partition .get_path() ;

	//this fixes a weird issue (see #169683 for more info)
	if ( partition .type == GParted::TYPE_EXTENDED && partition .busy ) 
		treerow[ treeview_detail_columns .path ] = treerow[ treeview_detail_columns .path ] + "   " ;

	// name
	treerow[treeview_detail_columns.name] = partition.name;

	treerow[ treeview_detail_columns .color ] = Utils::get_color_as_pixbuf( partition .filesystem, 16, 16 ) ; 

	treerow[ treeview_detail_columns .filesystem ] = 
		Utils::get_filesystem_string( partition .filesystem ) ;
	
	//mount point
	treerow[ treeview_detail_columns .mountpoint ] = Glib::build_path( ", ", partition .get_mountpoints() ) ;

	//label
	treerow[ treeview_detail_columns .label ] = partition.get_filesystem_label();
		
	//size
	treerow[ treeview_detail_columns .size ] = Utils::format_size( partition .get_sector_length(), partition .sector_size ) ;
	
	//used
	Sector used = partition .get_sectors_used() ;
	treerow[ treeview_detail_columns .used ] =
		used == -1 ? "---" : Utils::format_size( used, partition .sector_size ) ;

	//unused
	Sector unused = partition .get_sectors_unused() ;
	treerow[ treeview_detail_columns .unused ] = 
		unused == -1 ? "---" : Utils::format_size( unused, partition .sector_size ) ;

	//flags	
	treerow[ treeview_detail_columns .flags ] = 
		Glib::build_path( ", ", partition .flags ) ;

	// Hidden column (pointer to partition object)
	treerow[treeview_detail_columns.partition_ptr] = & partition;
}

bool TreeView_Detail::on_button_press_event( GdkEventButton * event )
{
	//Call base class, to allow normal handling,
	bool handled = Gtk::TreeView::on_button_press_event( event ) ;

	//right-click
	if ( event ->button == 3 )  
		signal_popup_menu .emit( event ->button, event ->time ) ;

	return handled ;
}

void TreeView_Detail::on_row_activated( const Gtk::TreeModel::Path & path, Gtk::TreeViewColumn * column ) 
{
	//Call base class, to allow normal handling,
	Gtk::TreeView::on_row_activated( path, column ) ;

	signal_partition_activated .emit() ;
}

void TreeView_Detail::on_selection_changed() 
{
	if ( ! block && treeselection ->get_selected() != 0 )
	{
		Gtk::TreeRow row = static_cast<Gtk::TreeRow>( * treeselection ->get_selected() ) ;
		signal_partition_selected.emit( row[treeview_detail_columns.partition_ptr], true );
	}
}
	
} //GParted
