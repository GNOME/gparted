/* Copyright (C) 2004, 2005, 2006, 2007, 2008 Bart Hakvoort
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
 
#ifndef DIALOG_FEATURES
#define DIALOG_FEATURES

#include "../include/Utils.h"

#include <gtkmm/dialog.h>
#include <gtkmm/expander.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/image.h>

namespace GParted
{

class DialogFeatures : public Gtk::Dialog
{
public:
	DialogFeatures() ;
	~DialogFeatures() ;

	void load_filesystems( const std::vector<FS> & FILESYSTEMS ) ;
	
private:
	void show_filesystem( const FS & fs ) ;

	Gtk::HBox *hbox ;
	Gtk::HBox *hbox2 ;
	Gtk::VBox *vbox ;
	Gtk::Image *image ;
	Gtk::Expander expander_legend ;
	Gtk::TreeView treeview_filesystems;
	Gtk::TreeRow treerow;
	Glib::RefPtr<Gtk::ListStore> liststore_filesystems;
	Glib::ustring str_temp ;
	
	struct treeview_filesystems_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> filesystem;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > create ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > grow ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > shrink ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > move ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > copy ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > check ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > label ;
		Gtk::TreeModelColumn<Glib::ustring> software ;
				
		treeview_filesystems_Columns() 
		{ 
			add( filesystem );
			add( create ) ;
			add( grow ) ;
			add( shrink ) ;
			add( move ) ;
			add( copy ) ;
			add( check ) ;
			add( label ) ;
			add( software ) ;
		}
	};
	
	treeview_filesystems_Columns treeview_filesystems_columns ;

	Glib::RefPtr<Gdk::Pixbuf> icon_yes, icon_no ;
};

} //GParted

#endif //DIALOG_FEATURES
