/* Copyright (C) 2004-2006 Bart 'plors' Hakvoort
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
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

#include "DialogFeatures.h"
#include "FileSystem.h"
#include "GParted_Core.h"

#include <gtkmm/stock.h>

namespace GParted
{

DialogFeatures::DialogFeatures() 
{
	set_title( _("File System Support") ) ;
	set_has_separator( false ) ;
	//Set minimum dialog height so it fits on an 800x600 screen
	set_size_request( -1, 500 ) ;

	//initialize icons
	icon_yes = render_icon( Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR );
	icon_no = render_icon( Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR );
	icon_blank = Gdk::Pixbuf::create( Gdk::COLORSPACE_RGB, true, 8,
	                                  icon_yes ->get_width(), icon_yes ->get_height() );
	icon_blank ->fill( 0xFFFFFF00 );

	//treeview
	Gtk::TreeView::Column *col;
	liststore_filesystems = Gtk::ListStore::create( treeview_filesystems_columns );
	treeview_filesystems .set_model( liststore_filesystems );
	treeview_filesystems .append_column( _("File System"), treeview_filesystems_columns .filesystem );
	treeview_filesystems .append_column( _("Create"), treeview_filesystems_columns .create );
	col = manage( new Gtk::TreeView::Column( _("Grow") ) );
	col ->pack_start( treeview_filesystems_columns .grow, false );
	col ->pack_start( treeview_filesystems_columns .online_grow, false );
	treeview_filesystems .append_column( *col );
	col = manage( new Gtk::TreeView::Column( _("Shrink") ) );
	col ->pack_start( treeview_filesystems_columns .shrink, false );
	col ->pack_start( treeview_filesystems_columns .online_shrink, false );
	treeview_filesystems .append_column( *col );
	treeview_filesystems .append_column( _("Move"), treeview_filesystems_columns .move );
	treeview_filesystems .append_column( _("Copy"), treeview_filesystems_columns .copy );
	treeview_filesystems .append_column( _("Check"), treeview_filesystems_columns .check );
	treeview_filesystems .append_column( _("Label"), treeview_filesystems_columns .label );
	treeview_filesystems .append_column( _("UUID"), treeview_filesystems_columns .uuid );
	treeview_filesystems .append_column( _("Required Software"), treeview_filesystems_columns .software );
	treeview_filesystems .get_selection() ->set_mode( Gtk::SELECTION_NONE );
	treeview_filesystems .set_rules_hint( true ) ;

	//scrollable file system list
	filesystems_scrolled .set_policy( Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC ) ;
	filesystems_scrolled .add( treeview_filesystems ) ;

	Gtk::HBox *filesystems_hbox( manage( new Gtk::HBox() ) ) ;
	filesystems_hbox ->set_border_width( 6 ) ;
	filesystems_hbox ->pack_start( filesystems_scrolled ) ;
	this ->get_vbox() ->pack_start( *filesystems_hbox ) ;

	//file system support legend
	Gtk::HBox *legend_hbox( manage( new Gtk::HBox( false, 6 ) ) ) ;
	legend_hbox ->set_border_width( 6 ) ;

	Gtk::HBox *legend_narrative_hbox( manage( new Gtk::HBox() ) ) ;
	Glib::ustring str_temp( _("This chart shows the actions supported on file systems.") ) ;
	str_temp += "\n" ;
	str_temp += _("Not all actions are available on all file systems, in part due to the nature of file systems and limitations in the required software.") ;
	legend_narrative_hbox ->pack_start( *Utils::mk_label( str_temp, true, true ), Gtk::PACK_SHRINK ) ;
	legend_hbox ->pack_start( *legend_narrative_hbox, Gtk::PACK_EXPAND_WIDGET, 6 ) ;

	//icon legend
	Gtk::VBox *icon_legend_vbox( manage( new Gtk::VBox() ) ) ;

	Gtk::HBox *available_both_hbox( manage( new Gtk::HBox() ) ) ;
	Gtk::Image *image_yes( manage( new Gtk::Image( icon_yes ) ) ) ;
	available_both_hbox ->pack_start( *image_yes, Gtk::PACK_SHRINK ) ;
	image_yes = manage( new Gtk::Image( icon_yes ) ) ;
	available_both_hbox ->pack_start( *image_yes, Gtk::PACK_SHRINK ) ;
	available_both_hbox ->pack_start( *Utils::mk_label(
			/* TO TRANSLATORS:  Available offline and online
			* means that this action is valid for this file system when
			* it is both unmounted and mounted.
			*/
			_("Available offline and online")), Gtk::PACK_EXPAND_WIDGET ) ;
	icon_legend_vbox ->pack_start( *available_both_hbox ) ;

	Gtk::HBox *available_online_hbox = manage( new Gtk::HBox() );
	Gtk::Image *image_no( manage( new Gtk::Image( icon_no ) ) );
	available_online_hbox->pack_start( *image_no, Gtk::PACK_SHRINK );
	image_yes = manage( new Gtk::Image( icon_yes ) );
	available_online_hbox->pack_start( *image_yes, Gtk::PACK_SHRINK );
	available_online_hbox->pack_start( *Utils::mk_label(
			/* TO TRANSLATORS:  Available online only
			 * means that this action is valid for this file system only
			 * when it is mounted.
			 */
			_("Available online only")), Gtk::PACK_EXPAND_WIDGET );
	icon_legend_vbox->pack_start( *available_online_hbox );

	Gtk::HBox *available_offline_hbox = manage(new Gtk::HBox() ) ;
	image_yes = manage( new Gtk::Image( icon_yes ) ) ;
	available_offline_hbox ->pack_start( *image_yes, Gtk::PACK_SHRINK ) ;
	Gtk::Image *image_blank( manage( new Gtk::Image( icon_blank ) ) ) ;
	available_offline_hbox ->pack_start( *image_blank, Gtk::PACK_SHRINK ) ;
	available_offline_hbox ->pack_start( *Utils::mk_label(
			/* TO TRANSLATORS:  Available offline only
			* means that this action is valid for this file system only
			* when it is unmounted.
			*/
			_("Available offline only")), Gtk::PACK_EXPAND_WIDGET ) ;
	icon_legend_vbox ->pack_start( *available_offline_hbox ) ;

	Gtk::HBox *not_available_hbox = manage( new Gtk::HBox() ) ;
	image_no = manage( new Gtk::Image( icon_no ) );
	not_available_hbox ->pack_start( *image_no, Gtk::PACK_SHRINK ) ;
	image_blank = manage( new Gtk::Image( icon_blank ) ) ;
	not_available_hbox ->pack_start( *image_blank, Gtk::PACK_SHRINK ) ;
	not_available_hbox->pack_start(*Utils::mk_label(
			/* TO TRANSLATORS:  Not Available
			* means that this action is not valid for this file system.
			*/
			_("Not Available") ), Gtk::PACK_EXPAND_WIDGET ) ;
	icon_legend_vbox ->pack_start( *not_available_hbox ) ;
	legend_hbox ->pack_start( *icon_legend_vbox ) ;

	str_temp = "<b>" ;
	str_temp += _("Legend") ;
	str_temp += "</b>" ;
	legend_frame .set_label_widget( *Utils::mk_label( str_temp ) ) ;
	legend_frame .set_shadow_type( Gtk::SHADOW_NONE ) ;
	legend_frame .add( *legend_hbox ) ;
	this ->get_vbox() ->pack_start( legend_frame, Gtk::PACK_SHRINK ) ;

	/*TO TRANSLATORS: This is a button that will search for the software tools installed and then refresh the screen with the file system actions supported. */
	add_button( _("Rescan For Supported Actions"), Gtk::RESPONSE_OK );
	add_button( Gtk::Stock::CLOSE, Gtk::RESPONSE_CLOSE ) ->grab_focus() ;
	show_all_children() ;
}

void DialogFeatures::load_filesystems( const std::vector<FS> & FILESYSTEMS )
{
	liststore_filesystems ->clear() ;

	//fill the features chart with valid file systems 
	for ( unsigned short t = 0; t < FILESYSTEMS .size() ; t++ )
	{
		if ( GParted_Core::supported_filesystem( FILESYSTEMS[t].filesystem ) )
			show_filesystem( FILESYSTEMS[t] );
	}
}
		
void DialogFeatures::show_filesystem( const FS & fs )
{
	treerow = *( liststore_filesystems ->append() );
	treerow[ treeview_filesystems_columns .filesystem ] = Utils::get_filesystem_string( fs .filesystem ) ;

	treerow[ treeview_filesystems_columns .create ] = fs .create ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .grow ] = fs .grow ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .online_grow ] = fs .online_grow ? icon_yes : icon_blank ;
	treerow[ treeview_filesystems_columns .shrink ] = fs .shrink ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .online_shrink ] = fs .online_shrink ? icon_yes : icon_blank ;
	treerow[ treeview_filesystems_columns .move ] = fs .move ? icon_yes : icon_no ;  
	treerow[ treeview_filesystems_columns .copy ] = fs .copy ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .check ] = fs .check ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .label ] = fs .write_label ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .uuid ] = fs .write_uuid ? icon_yes : icon_no ;

	treerow[ treeview_filesystems_columns .software ] = Utils::get_filesystem_software( fs .filesystem ) ;
}

DialogFeatures::~DialogFeatures() 
{
}

} //GParted


