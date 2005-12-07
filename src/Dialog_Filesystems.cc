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

namespace GParted
{

Dialog_Filesystems::Dialog_Filesystems( ) 
{
	this ->set_title( _("Filesystems") ) ;
	this ->set_has_separator( false ) ;
	this ->set_resizable( false ) ;
	
	liststore_filesystems = Gtk::ListStore::create( treeview_filesystems_columns );
	treeview_filesystems .set_model( liststore_filesystems );
	treeview_filesystems .append_column( _("Filesystem"), treeview_filesystems_columns .filesystem );
	treeview_filesystems .append_column( _("Create"), treeview_filesystems_columns .create );
	treeview_filesystems .append_column( _("Grow"), treeview_filesystems_columns .grow );
	treeview_filesystems .append_column( _("Shrink"), treeview_filesystems_columns .shrink );
	treeview_filesystems .append_column( _("Move"), treeview_filesystems_columns .move );
	treeview_filesystems .append_column( _("Copy"), treeview_filesystems_columns .copy );
		
	treeview_filesystems .get_selection( ) ->set_mode( Gtk::SELECTION_NONE );
	this ->get_vbox( ) ->pack_start( treeview_filesystems ) ;
	
	this ->add_button( Gtk::Stock::REFRESH, Gtk::RESPONSE_OK );
	this ->add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE ) ->grab_focus() ;
	this ->show_all_children( ) ;
}

void Dialog_Filesystems::Load_Filesystems( const std::vector< FS > & FILESYSTEMS )
{
	liststore_filesystems ->clear( ) ;
	
	for ( unsigned short t = 0; t < FILESYSTEMS .size( ) -1 ; t++ )
		Show_Filesystem( FILESYSTEMS[ t ] ) ;
}
		
void Dialog_Filesystems::Show_Filesystem( const FS & fs )
{
	treerow = *( liststore_filesystems ->append( ) );
	treerow[ treeview_filesystems_columns .filesystem ] = Get_Filesystem_String( fs .filesystem ) ;
	
	treerow[ treeview_filesystems_columns .create ] = 
		render_icon( fs .create ? Gtk::Stock::APPLY : Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR); 
	
	treerow[ treeview_filesystems_columns .grow ] = 
		render_icon( fs .grow ? Gtk::Stock::APPLY : Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ); 
	
	treerow[ treeview_filesystems_columns .shrink ] = 
		render_icon( fs .shrink ? Gtk::Stock::APPLY : Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ); 
	
	treerow[ treeview_filesystems_columns .move ] = 
		render_icon( fs .move ? Gtk::Stock::APPLY : Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ); 
	
	treerow[ treeview_filesystems_columns .copy ] = 
		render_icon( fs .copy ? Gtk::Stock::APPLY : Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ); 
}

Dialog_Filesystems::~Dialog_Filesystems( ) 
{
}

} //GParted


