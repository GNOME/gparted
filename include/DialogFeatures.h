/* Copyright (C) 2004-2006 Bart 'plors' Hakvoort
 * Copyright (C) 2008 Curtis Gedak
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

#ifndef GPARTED_DIALOGFEATURES_H
#define GPARTED_DIALOGFEATURES_H

#include "FileSystem.h"

#include <gtkmm/dialog.h>
#include <gtkmm/frame.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/scrolledwindow.h>
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

	Gtk::Frame legend_frame ;
	Gtk::TreeView treeview_filesystems;
	Gtk::TreeRow treerow;
	Gtk::ScrolledWindow filesystems_scrolled ;
	Glib::RefPtr<Gtk::ListStore> liststore_filesystems;
	
	struct treeview_filesystems_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> filesystem;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > create ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > grow ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > online_grow ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > shrink ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > online_shrink ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > move ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > copy ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > check ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > label ;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > uuid ;
		Gtk::TreeModelColumn<Glib::ustring> software ;
				
		treeview_filesystems_Columns() 
		{ 
			add( filesystem );
			add( create ) ;
			add( grow ) ;
			add( online_grow ) ;
			add( shrink ) ;
			add( online_shrink ) ;
			add( move ) ;
			add( copy ) ;
			add( check ) ;
			add( label ) ;
			add( uuid ) ;
			add( software ) ;
		}
	};
	
	treeview_filesystems_Columns treeview_filesystems_columns ;

	Glib::RefPtr<Gdk::Pixbuf> icon_yes, icon_no, icon_blank ;
};

} //GParted

#endif /* GPARTED_DIALOGFEATURES_H */
