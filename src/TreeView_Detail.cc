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

#include "TreeView_Detail.h"
#include "Partition.h"
#include "PartitionLUKS.h"
#include "PartitionVector.h"
#include "Utils.h"

#include <vector>
#include <gtkmm/cellrenderer.h>
#include <gtkmm/cellrenderertext.h>
#include <pangomm/layout.h>
#include <glibmm/miscutils.h>
#include <glibmm/ustring.h>


namespace GParted
{ 


TreeView_Detail::TreeView_Detail()
 : m_block(false)
{
	m_treestore_detail = Gtk::TreeStore::create(m_treeview_detail_columns);
	set_model(m_treestore_detail);
	set_rules_hint( true );
	m_treeselection = get_selection();
	m_treeselection->signal_changed().connect(sigc::mem_fun(*this, &TreeView_Detail::on_selection_changed));

	//append columns
	append_column(_("Partition"), m_treeview_detail_columns.path);
	append_column(_("Name"), m_treeview_detail_columns.name);
	append_column(_("File System"), m_treeview_detail_columns.color);
	append_column(_("Mount Point"), m_treeview_detail_columns.mountpoint);
	append_column(_("Label"), m_treeview_detail_columns.label);
	append_column(_("Size"), m_treeview_detail_columns.size);
	append_column(_("Used"), m_treeview_detail_columns.used);
	append_column(_("Unused"), m_treeview_detail_columns.unused);
	append_column(_("Flags"), m_treeview_detail_columns.flags);

	// Tree view column "Partition"; add icon cells.
	get_column(0)->pack_start(m_treeview_detail_columns.icon2, false);
	get_column(0)->pack_start(m_treeview_detail_columns.icon1, false);

	// Tree view column "File System"; add file system text cell.
	get_column(2)->pack_start(m_treeview_detail_columns.fsname, true);
	// Color pixbuf cell is left aligned.
	get_column(2)->get_first_cell()->property_xalign() = Gtk::ALIGN_START;
	// File system text cell is left aligned.
	std::vector<Gtk::CellRenderer *> renderers = get_column(2)->get_cells();
	Gtk::CellRendererText *cell_renderer_text = dynamic_cast<Gtk::CellRendererText*>( renderers.back() );
	cell_renderer_text->property_xalign() = Gtk::ALIGN_START;

	// Tree view column "Mount Point", make column resizable and show too wide text
	// with ellipsis.
	get_column( 3 )->set_resizable( true );
	cell_renderer_text = dynamic_cast<Gtk::CellRendererText *>(get_column(3)->get_first_cell());
	cell_renderer_text->property_ellipsize() = Pango::ELLIPSIZE_END;

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


void TreeView_Detail::load_partitions( const PartitionVector & partitions )
{
	bool show_names       = false;
	bool show_mountpoints = false;
	bool show_labels      = false;

	m_block = true;
	m_treestore_detail->clear();
	m_block = false;

	load_partitions( partitions, show_names, show_mountpoints, show_labels );

	get_column( 1 )->set_visible( show_names );
	get_column( 3 )->set_visible( show_mountpoints );
	get_column( 4 )->set_visible( show_labels );

	columns_autosize();
	expand_all() ;
}


void TreeView_Detail::set_selected( const Partition * partition_ptr )
{
	m_block = true;
	set_selected_by_ptn(m_treestore_detail->children(), partition_ptr);
	m_block = false;
}


void TreeView_Detail::clear()
{
	m_treestore_detail->clear();
}


void TreeView_Detail::load_partitions( const PartitionVector & partitions,
                                       bool & show_names,
                                       bool & show_mountpoints,
                                       bool & show_labels,
                                       const Gtk::TreeRow & parent_row )
{
	Gtk::TreeRow row ;
	for ( unsigned int i = 0 ; i < partitions .size() ; i++ ) 
	{	
		row = parent_row ? *(m_treestore_detail->append(parent_row.children()))
		                 : *(m_treestore_detail->append());
		create_row( row, partitions[i], show_names, show_mountpoints, show_labels );

		if (partitions[i].type == TYPE_EXTENDED)
			load_partitions( partitions[i].logicals, show_names, show_mountpoints, show_labels, row );
	}
}


bool TreeView_Detail::set_selected_by_ptn(Gtk::TreeModel::Children rows,
                                          const Partition* partition_ptr, bool inside_extended)
{
	for ( unsigned int t = 0 ; t < rows .size() ; t++ )
	{
		if (*static_cast<const Partition*>(rows[t][m_treeview_detail_columns.partition_ptr]) == *partition_ptr)
		{
			if ( inside_extended )
				expand_all() ;
			
			set_cursor( static_cast<Gtk::TreePath>( rows[ t ] ) ) ;
			return true ;
		}

		if (set_selected_by_ptn(rows[t].children(), partition_ptr, true))
			return true ;
	}

	return false ;
}


void TreeView_Detail::create_row( const Gtk::TreeRow & treerow,
                                  const Partition & partition,
                                  bool & show_names,
                                  bool & show_mountpoints,
                                  bool & show_labels )
{
	const Partition & filesystem_ptn = partition.get_filesystem_partition();
	if ( filesystem_ptn.busy )
		treerow[m_treeview_detail_columns.icon1] =
		                Utils::mk_pixbuf(*this, Gtk::Stock::DIALOG_AUTHENTICATION, Gtk::ICON_SIZE_BUTTON);

	if ( partition.have_messages() > 0 )
	{
		if (! static_cast<Glib::RefPtr<Gdk::Pixbuf>>(treerow[m_treeview_detail_columns.icon1]))
			treerow[m_treeview_detail_columns.icon1] =
			                Utils::mk_pixbuf(*this, Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON);
		else
			treerow[m_treeview_detail_columns.icon2] =
			                Utils::mk_pixbuf(*this, Gtk::Stock::DIALOG_WARNING, Gtk::ICON_SIZE_BUTTON);
	}

	treerow[m_treeview_detail_columns.path] = partition.get_path();

	//this fixes a weird issue (see #169683 for more info)
	if (partition.type == TYPE_EXTENDED && partition.busy)
		treerow[m_treeview_detail_columns.path] = treerow[m_treeview_detail_columns.path] + "   ";

	// name
	treerow[m_treeview_detail_columns.name] = partition.name;
	if ( ! partition.name.empty() )
		show_names = true;

	// file system
	treerow[m_treeview_detail_columns.color] = Utils::get_color_as_pixbuf(filesystem_ptn.fstype, 16, 16);
	treerow[m_treeview_detail_columns.fsname] = partition.get_filesystem_string();

	// mount point
	const std::vector<Glib::ustring>& temp_mountpoints = filesystem_ptn.get_mountpoints();
	treerow[m_treeview_detail_columns.mountpoint] = Glib::build_path( ", ", temp_mountpoints );
	if ( ! temp_mountpoints.empty() )
		show_mountpoints = true;

	//label
	const Glib::ustring& temp_filesystem_label = filesystem_ptn.get_filesystem_label();
	treerow[m_treeview_detail_columns.label] = temp_filesystem_label;
	if ( ! temp_filesystem_label.empty() )
		show_labels = true;

	//size
	treerow[m_treeview_detail_columns.size] = Utils::format_size(partition.get_sector_length(), partition.sector_size);

	//used
	Sector used = partition .get_sectors_used() ;
	treerow[m_treeview_detail_columns.used] =
		used == -1 ? "---" : Utils::format_size( used, partition .sector_size ) ;

	//unused
	Sector unused = partition .get_sectors_unused() ;
	treerow[m_treeview_detail_columns.unused] =
		unused == -1 ? "---" : Utils::format_size( unused, partition .sector_size ) ;

	//flags	
	treerow[m_treeview_detail_columns.flags] =
		Glib::build_path( ", ", partition .flags ) ;

	// Hidden column (pointer to partition object)
	treerow[m_treeview_detail_columns.partition_ptr] = &partition;
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
	if (! m_block && m_treeselection->get_selected() != 0)
	{
		Gtk::TreeRow row = static_cast<Gtk::TreeRow>(*m_treeselection->get_selected());
		signal_partition_selected.emit(row[m_treeview_detail_columns.partition_ptr], true);
	}
}

	
}  // namespace GParted
