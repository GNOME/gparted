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
 
#ifndef DIALOG_FILESYSTEMS
#define DIALOG_FILESYSTEMS
//FIXME add more info ( at least 'detect' which will be any filesystem we _can_ detect (and should therefore have
//it's own class) )
#include "../include/Utils.h"

#include <gtkmm/dialog.h>
#include <gtkmm/treeview.h>
#include <gtkmm/liststore.h>

namespace GParted
{

class Dialog_Filesystems : public Gtk::Dialog
{
public:
	
	Dialog_Filesystems() ;
	void Load_Filesystems( const std::vector< FS > & FILESYSTEMS ) ;
	~Dialog_Filesystems() ;
	
private:
	void Show_Filesystem( const FS & fs ) ;

	Gtk::TreeView treeview_filesystems;
	Gtk::TreeRow treerow;
	Glib::RefPtr<Gtk::ListStore> liststore_filesystems;
	
	struct treeview_filesystems_Columns : public Gtk::TreeModelColumnRecord             
	{
		Gtk::TreeModelColumn<Glib::ustring> filesystem;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > create;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > grow;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > shrink;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > move;
		Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > copy;
				
		treeview_filesystems_Columns() 
		{ 
			add( filesystem ); add( create ); add( grow );
			add( shrink ); add( move ); add( copy );
		}
	};
	
	treeview_filesystems_Columns treeview_filesystems_columns ;
};

} //GParted

#endif //DIALOG_FILESYSTEMS
