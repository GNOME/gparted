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
	//set locale for this stream to standard. this prevent weird locals from f*ucking up my conversions :^)
	os.imbue(std::locale(""));
	
	treestore_detail = Gtk::TreeStore::create( treeview_detail_columns );
	this->set_model( treestore_detail );
	this->set_rules_hint(true);
	this->treeselection = this->get_selection();
		
	//append columns
	this->append_column( _("Partition"), treeview_detail_columns.partition );
	this->append_column( _("Type"), treeview_detail_columns.type );
	this->append_column( (Glib::ustring) _("Size") + "(MB)", treeview_detail_columns.size );
	this->append_column( (Glib::ustring) _("Used") + "(MB)", treeview_detail_columns.used );
	this->append_column( (Glib::ustring) _("Unused") + "(MB)", treeview_detail_columns.unused );
	this->append_column( _("Flags"), treeview_detail_columns.flags );
	
	//status_icon
	this->get_column( 0 ) ->pack_start( treeview_detail_columns.status_icon,false );
	
	//colored text in Partition column (used for darkgrey unallocated)
	Gtk::CellRendererText *cell_renderer_text = dynamic_cast<Gtk::CellRendererText*>(  this->get_column( 0 ) ->get_first_cell_renderer()   );
	this->get_column( 0 ) ->add_attribute(cell_renderer_text->property_foreground(), treeview_detail_columns.text_color);
	
	//colored text in Type column
	cell_renderer_text = dynamic_cast<Gtk::CellRendererText*>( this->get_column( 1 ) ->get_first_cell_renderer() );
	this->get_column( 1 ) ->add_attribute(cell_renderer_text->property_foreground(), treeview_detail_columns.color);
	
	//set alignment of numeric columns to right
	for( short t=2;t<5;t++)
	{
		cell_renderer_text = dynamic_cast<Gtk::CellRendererText*>( this->get_column( t ) ->get_first_cell_renderer() );
		cell_renderer_text->property_xalign () = 1;
	}
	
}

void TreeView_Detail::Load_Partitions( std::vector<Partition> & partitions ) 
{
	treestore_detail ->clear() ;
	
	for ( unsigned int i=0;i<partitions.size();i++ ) 
	{
		if ( ! partitions[ i ] .inside_extended )
		{
			row = *(treestore_detail->append());
			Create_Row( row, partitions[i] );
		}
		else if (  partitions[ i ] .inside_extended )
		{
			childrow = *(treestore_detail->append( row.children() ));
			Create_Row( childrow, partitions[i] );
		}
	}
	
	//show logical partitions ( if any )
	this->expand_all();

}

void TreeView_Detail::Set_Selected( const Partition & partition )
{ 
	//look for appropiate row
	for(iter = treestore_detail->children().begin();iter!=treestore_detail->children().end();iter++ )
	{
		row = *iter;
		partition_temp = row[treeview_detail_columns.partition_struct] ;
		//primary's
		if ( partition .sector_start >= partition_temp .sector_start && partition .sector_end <=partition_temp .sector_end && partition.inside_extended == partition_temp.inside_extended )
			{ this->set_cursor( (Gtk::TreePath) row); return; }
		
		//logicals
		if ( row.children().size() > 0 ) //this is the row with the extended partition, search it's childrows...
		{
			for(iter_child = row.children().begin();iter_child != row.children().end();iter_child++ )
			{
				childrow = *iter_child;
				partition_temp = childrow[treeview_detail_columns.partition_struct] ;
						
				if ( partition .sector_start >= partition_temp .sector_start && partition .sector_end <= partition_temp.sector_end  )
				{
					this->expand_all();
					this->set_cursor( (Gtk::TreePath) childrow);
					return;
				}
			}
		}

	}
	
	
}

void TreeView_Detail::Create_Row( const Gtk::TreeRow & treerow, Partition & partition )
{
	//hereby i assume these 2 are mutual exclusive. is this wise?? Time (and bugreports) will tell :)
	if ( partition.busy )
		treerow[treeview_detail_columns.status_icon] = render_icon(Gtk::Stock::DIALOG_AUTHENTICATION, Gtk::ICON_SIZE_BUTTON);
	else if ( partition.filesystem == "unknown" || partition .error != "" )
		treerow[treeview_detail_columns.status_icon] = render_icon(Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON);

	treerow[treeview_detail_columns.partition] = partition.partition;
	treerow[treeview_detail_columns.color] =	partition.color_string;

	partition .type == GParted::UNALLOCATED ? treerow[treeview_detail_columns.text_color] = "darkgrey" : treerow[treeview_detail_columns.text_color] = "black" ;
	treerow[treeview_detail_columns.type] = partition.filesystem;
	
	os << partition .Get_Length_MB() ;
	treerow[treeview_detail_columns.size] = os.str() ; os.str("");

	partition.sectors_used != -1 ?  os << partition .Get_Used_MB() : os << "---" ;
	treerow[treeview_detail_columns.used] = os.str() ; os.str("") ;

	partition.sectors_unused != -1 ? os << partition .Get_Unused_MB() : os << "---" ;
	treerow[treeview_detail_columns.unused] =  os.str() ; os.str("") ;
	
	treerow[treeview_detail_columns.flags] = " " + partition .flags ;
	
	treerow[treeview_detail_columns.partition_struct] = partition;
}

bool TreeView_Detail::on_button_press_event(GdkEventButton* event)
{ 
	//Call base class, to allow normal handling,
  	//such as allowing the row to be selected by the right-click:
  	bool return_value = TreeView::on_button_press_event(event);

	iter = treeselection->get_selected();
		
	if ( *iter != 0 )
	{
		row = *iter;
		signal_mouse_click.emit( event, row[treeview_detail_columns.partition_struct] );
	}
		
	return return_value;
}

} //GParted
