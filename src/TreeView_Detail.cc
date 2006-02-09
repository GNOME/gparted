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
 
#include "../include/TreeView_Detail.h"

namespace GParted
{ 

TreeView_Detail::TreeView_Detail( )
{
	block = false ;
	
	treestore_detail = Gtk::TreeStore::create( treeview_detail_columns );
	set_model( treestore_detail );
	set_rules_hint( true );
	treeselection = get_selection();
	treeselection ->signal_changed() .connect( sigc::mem_fun( *this, &TreeView_Detail::on_selection_changed ) );

	//append columns
	append_column( _("Partition"), treeview_detail_columns .partition );
	append_column( _("Filesystem"), treeview_detail_columns .color );
	append_column( _("Mountpoint"), treeview_detail_columns .mountpoint );
	append_column( _("Size"), treeview_detail_columns .size );
	append_column( _("Used"), treeview_detail_columns .used );
	append_column( _("Unused"), treeview_detail_columns .unused );
	append_column( _("Flags"), treeview_detail_columns .flags );
	
	//status_icon
	get_column( 0 ) ->pack_start( treeview_detail_columns.status_icon, false );
	
	//filesystem text
	get_column( 1 ) ->pack_start( treeview_detail_columns .filesystem, true );
	
	//colored text in Partition column 
	Gtk::CellRendererText *cell_renderer_text = 
		dynamic_cast<Gtk::CellRendererText*>( get_column( 0 ) ->get_first_cell_renderer() );
	get_column( 0 ) ->add_attribute( cell_renderer_text ->property_foreground(), 
					 treeview_detail_columns .text_color );
	
	//colored text in Filesystem column 
	std::vector<Gtk::CellRenderer*> renderers = get_column( 1 ) ->get_cell_renderers() ;
	cell_renderer_text = dynamic_cast<Gtk::CellRendererText*>( renderers .back() ) ;
	get_column( 1 ) ->add_attribute( cell_renderer_text ->property_foreground(),
					 treeview_detail_columns .text_color );
	
	//pixbuf and text are both left aligned
	get_column( 1 ) ->get_first_cell_renderer() ->property_xalign() = Gtk::ALIGN_LEFT ;
	cell_renderer_text ->property_xalign() = Gtk::ALIGN_LEFT ;

	//set alignment of numeric columns to right
	for( short t = 3 ; t < 6 ; t++ )
		get_column_cell_renderer( t ) ->property_xalign() = 1 ;

	//expand columns and centeralign the headertext
	for( short t = 3 ; t < 7 ; t++ )
	{
		get_column( t ) ->set_expand( true ) ;
		get_column( t ) ->set_alignment( 0.5 ) ;
	}
}

void TreeView_Detail::load_partitions( const std::vector<Partition> & partitions ) 
{
	bool mount_info = false ;
	treestore_detail ->clear() ;
	
	for ( unsigned int i = 0 ; i < partitions .size() ; i++ ) 
	{
		row = *( treestore_detail ->append() );
		create_row( row, partitions[ i ] );
		
		if ( partitions[ i ] .type == GParted::TYPE_EXTENDED )
		{
			for ( unsigned int t = 0 ; t < partitions[ i ] .logicals .size() ; t++ ) 
			{
				childrow = *( treestore_detail ->append( row.children() ) );
				create_row( childrow, partitions[ i ] .logicals[ t ] );
		
				if ( ! partitions[ i ] .logicals[ t ] .mountpoint .empty() )
					mount_info = true ;
			}
		}

		if ( ! partitions[ i ] .mountpoint .empty() )
			mount_info = true ;
	}

	get_column( 2 ) ->set_visible( mount_info ) ;
	
	//show logical partitions ( if any )
	expand_all();
	
	columns_autosize();
}

void TreeView_Detail::set_selected( const Partition & partition )
{
	block = true ;
	set_selected( treestore_detail ->children(), partition ) ;
	block = false ;
}

void TreeView_Detail::clear()
{
	treestore_detail ->clear() ;
}

bool TreeView_Detail::set_selected( Gtk::TreeModel::Children rows, const Partition & partition, bool inside_extended ) 
{
	for ( unsigned int t = 0 ; t < rows .size() ; t++ )
	{
		if ( static_cast<Partition>( rows[ t ] [ treeview_detail_columns .partition_struct ] ) == partition )
		{
			if ( inside_extended )
				expand_all() ;
			
			set_cursor( static_cast<Gtk::TreePath>( rows[ t ] ) ) ;
			return true ;
		}
		
		if ( rows[ t ] .children() .size() > 0 && set_selected( rows[ t ] .children(), partition, true ) )
			return true ;
	}
	
	return false ;
}

void TreeView_Detail::create_row( const Gtk::TreeRow & treerow, const Partition & partition )
{
	//hereby i assume these 2 are mutual exclusive. is this wise?? Time (and bugreports) will tell :)
	if ( partition .busy )
		treerow[ treeview_detail_columns .status_icon ] = render_icon( Gtk::Stock::DIALOG_AUTHENTICATION, Gtk::ICON_SIZE_BUTTON );
	else if ( partition .error != "" )
		treerow[ treeview_detail_columns .status_icon ] = render_icon( Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON );
	
	treerow[ treeview_detail_columns .partition ] = partition .partition;
	treerow[ treeview_detail_columns .color ] = Utils::get_color_as_pixbuf( partition .filesystem, 16, 16 ) ; 

	treerow[ treeview_detail_columns .text_color ] = 
		partition .type == GParted::TYPE_UNALLOCATED ? "darkgrey" : "black" ;
	
	treerow[ treeview_detail_columns .filesystem ] = 
		Utils::Get_Filesystem_String( partition .filesystem ) ;
	
	//mountpoint
	treerow[ treeview_detail_columns .mountpoint ] = partition .mountpoint ;
	
	//size
	treerow[ treeview_detail_columns .size ] = Utils::format_size( partition .get_length() ) ;
	
	//used
	treerow[ treeview_detail_columns .used ] =
		partition .sectors_used == -1 ? "---" : Utils::format_size( partition .sectors_used ) ;

	//unused
	treerow[ treeview_detail_columns .unused ] = 
		partition .sectors_unused == -1 ? "---" : Utils::format_size( partition .sectors_unused ) ;

	//flags	
	treerow[ treeview_detail_columns .flags ] = " " + partition .flags ;
	
	//hidden column (partition object)
	treerow[ treeview_detail_columns .partition_struct ] = partition;
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
		row = static_cast<Gtk::TreeRow>( * treeselection ->get_selected() ) ;
		signal_partition_selected .emit( row[ treeview_detail_columns .partition_struct ], true ) ;
	}
}
	
} //GParted
