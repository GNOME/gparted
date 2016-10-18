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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "GParted_Core.h"
#include "Win_GParted.h"

#include <gtkmm/messagedialog.h>
#include <gtkmm/main.h>

int main( int argc, char *argv[] )
{
	//initialize thread system
	Glib::thread_init() ;
	GParted::GParted_Core::mainthread = Glib::Thread::self();

	Gtk::Main kit( argc, argv ) ;

	//Set WM_CLASS X Window property for correct naming under GNOME Shell
	gdk_set_program_class( "GParted" ) ;

	//i18n
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR ) ;
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" ) ;
	textdomain( GETTEXT_PACKAGE ) ;

	//check UID
	if ( getuid() != 0 )
	{
		Gtk::MessageDialog dialog( _("Root privileges are required for running GParted"), 
					   false,
					   Gtk::MESSAGE_ERROR,
					   Gtk::BUTTONS_OK ) ;
		dialog .set_secondary_text(
				_("Since GParted is a powerful tool capable of destroying partition tables and vast amounts of data, only root may run it.") ) ;
		
		dialog .run() ;
		exit( 0 ) ;
	}

	//deal with arguments..
	std::vector<Glib::ustring> user_devices(argv + 1, argv + argc);
	
	GParted::Win_GParted win_gparted( user_devices ) ; 
	Gtk::Main::run( win_gparted ) ;

	return 0 ;
}


