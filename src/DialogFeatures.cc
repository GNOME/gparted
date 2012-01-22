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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
#include "../include/DialogFeatures.h" 

#include <gtkmm/stock.h>

namespace GParted
{

DialogFeatures::DialogFeatures() 
{
	set_title( _("File System Support") ) ;
	set_has_separator( false ) ;
	set_resizable( false ) ;

	//treeview
	liststore_filesystems = Gtk::ListStore::create( treeview_filesystems_columns );
	treeview_filesystems .set_model( liststore_filesystems );
	treeview_filesystems .append_column( _("File System"), treeview_filesystems_columns .filesystem );
	treeview_filesystems .append_column( _("Create"), treeview_filesystems_columns .create );
	treeview_filesystems .append_column( _("Grow"), treeview_filesystems_columns .grow );
	treeview_filesystems .append_column( _("Shrink"), treeview_filesystems_columns .shrink );
	treeview_filesystems .append_column( _("Move"), treeview_filesystems_columns .move );
	treeview_filesystems .append_column( _("Copy"), treeview_filesystems_columns .copy );
	treeview_filesystems .append_column( _("Check"), treeview_filesystems_columns .check );
	treeview_filesystems .append_column( _("Label"), treeview_filesystems_columns .label );
	treeview_filesystems .append_column( _("UUID"), treeview_filesystems_columns .uuid );
	treeview_filesystems .append_column( _("Required Software"), treeview_filesystems_columns .software );
	treeview_filesystems .get_selection() ->set_mode( Gtk::SELECTION_NONE );
	treeview_filesystems .set_rules_hint( true ) ;

	{
		Gtk::HBox* hbox(manage(new Gtk::HBox()));

		hbox->set_border_width(6);
		hbox->pack_start(treeview_filesystems);
		get_vbox()->pack_start(*hbox);

		//file system support legend
		Gtk::HBox* hbox2(manage(new Gtk::HBox(false, 6)));
		hbox2->set_border_width(6);

		hbox = manage(new Gtk::HBox());
		{
			Glib::ustring str_temp(_("This chart shows the actions supported on file systems."));
			str_temp += "\n" ;
			str_temp += _("Not all actions are available on all file systems, in part due to the nature of file systems and limitations in the required software.");
			hbox->pack_start(*Utils::mk_label(str_temp, true, Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER, true),
					Gtk::PACK_SHRINK);
			hbox2->pack_start(*hbox);

			{
				//icon legend
				Gtk::VBox* vbox(manage(new Gtk::VBox()));

				hbox = manage(new Gtk::HBox());

				{
					Gtk::Image* image(manage(new Gtk::Image(Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR)));
					hbox->pack_start(*image, Gtk::PACK_SHRINK);
					hbox->pack_start(*Utils::mk_label(
							/* TO TRANSLATORS:  Available
							* means that this action is valid for this file system.
							*/
							_("Available")), Gtk::PACK_EXPAND_WIDGET );
					vbox ->pack_start(*hbox);

					hbox = manage(new Gtk::HBox());
					image = manage(new Gtk::Image(Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR));
					hbox->pack_start(*image, Gtk::PACK_SHRINK);
				}

				hbox->pack_start(*Utils::mk_label(
						/* TO TRANSLATORS:  Not Available
						* means that this action is not valid for this file system.
						*/
						_("Not Available") ), Gtk::PACK_EXPAND_WIDGET);
				vbox->pack_start(*hbox);
				hbox2->pack_start(*vbox);
			}

			str_temp = "<b>";
			str_temp += _("Legend");
			str_temp += "</b>";
			expander_legend.set_label(str_temp);
			expander_legend.set_use_markup(true);
		}

		get_vbox()->pack_start(expander_legend, Gtk::PACK_SHRINK);
		expander_legend.add(*hbox2);
	}

	//initialize icons
	icon_yes = render_icon( Gtk::Stock::APPLY, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ; 
	icon_no = render_icon( Gtk::Stock::CANCEL, Gtk::ICON_SIZE_LARGE_TOOLBAR ) ; 

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
		//Skip luks, lvm2, and unknown because these are not file systems
		if (
		     FILESYSTEMS[ t ] .filesystem == GParted::FS_LUKS ||
		     FILESYSTEMS[ t ] .filesystem == GParted::FS_LVM2 ||
		     FILESYSTEMS[ t ] .filesystem == GParted::FS_UNKNOWN
		   )
			continue ;
		show_filesystem( FILESYSTEMS[ t ] ) ;
	}
}
		
void DialogFeatures::show_filesystem( const FS & fs )
{
	treerow = *( liststore_filesystems ->append() );
	treerow[ treeview_filesystems_columns .filesystem ] = Utils::get_filesystem_string( fs .filesystem ) ;

	treerow[ treeview_filesystems_columns .create ] = fs .create ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .grow ] = fs .grow ? icon_yes : icon_no ; 
	treerow[ treeview_filesystems_columns .shrink ] = fs .shrink ? icon_yes : icon_no ; 
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


