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
 
#include "../include/Dialog_Filesystems.h" 

#include <gtkmm/stock.h>

namespace GParted
{

Dialog_Filesystems::Dialog_Filesystems() 
{
	set_title( _("Filesystems") ) ;
	set_has_separator( false ) ;
	set_resizable( false ) ;
	
	liststore_filesystems = Gtk::ListStore::create( treeview_filesystems_columns );
	treeview_filesystems .set_model( liststore_filesystems );
	treeview_filesystems .append_column( _("Filesystem"), treeview_filesystems_columns .filesystem );
	treeview_filesystems .append_column( _("Detect"), treeview_filesystems_columns .detect );
	treeview_filesystems .append_column( _("Read"), treeview_filesystems_columns .read );
	treeview_filesystems .append_column( _("Create"), treeview_filesystems_columns .create );
	treeview_filesystems .append_column( _("Grow"), treeview_filesystems_columns .grow );
	treeview_filesystems .append_column( _("Shrink"), treeview_filesystems_columns .shrink );
	treeview_filesystems .append_column( _("Move"), treeview_filesystems_columns .move );
	treeview_filesystems .append_column( _("Copy"), treeview_filesystems_columns .copy );
	treeview_filesystems .append_column( _("Check"), treeview_filesystems_columns .check );
		
	treeview_filesystems .get_selection() ->set_mode( Gtk::SELECTION_NONE );
	get_vbox() ->pack_start( treeview_filesystems ) ;

	//initialize icons
	icon_yes = render_icon( Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ; 
	icon_no = render_icon( Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ; 
	
	add_button( Gtk::Stock::REFRESH, Gtk::RESPONSE_OK ) ;
	add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE ) ->grab_focus() ;
	show_all_children() ;
}

void Dialog_Filesystems::Load_Filesystems( const std::vector<FS> & FILESYSTEMS )
{
	liststore_filesystems ->clear() ;
	
	for ( unsigned short t = 0; t < FILESYSTEMS .size() -1 ; t++ )
		Show_Filesystem( FILESYSTEMS[ t ] ) ;
}
		
void Dialog_Filesystems::Show_Filesystem( const FS & fs )
{
	treerow = *( liststore_filesystems ->append() );
	treerow[ treeview_filesystems_columns .filesystem ] = Utils::get_filesystem_string( fs .filesystem ) ;
	
	treerow[ treeview_filesystems_columns .detect ] = icon_yes ; 
	treerow[ treeview_filesystems_columns .read ] = fs .read ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .create ] = fs .create ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .grow ] = fs .grow ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .shrink ] = fs .shrink ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .move ] = fs .move ? icon_yes : icon_no ;  
	treerow[ treeview_filesystems_columns .copy ] = fs .copy ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .check ] = fs .check ? icon_yes : icon_no ; 
}

Dialog_Filesystems::~Dialog_Filesystems() 
{
}

} //GParted


