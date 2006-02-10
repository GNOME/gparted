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
 
#include "../include/Win_GParted.h"

#include <gtkmm/messagedialog.h>
#include <gtkmm/main.h>

int main( int argc, char *argv[] )
{
	//initialize thread system
	Glib::thread_init() ;
	
	Gtk::Main kit( argc, argv ) ;
		 
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
				_("Since GParted can be a weapon of mass destruction only root may run it.") ) ;
		
		dialog .run() ;
		exit( 0 ) ;
	}

	//deal with arguments..
	std::vector<Glib::ustring> user_devices ;
	
	for ( int t = 1 ; t < argc ; t++ )
		user_devices .push_back( argv[ t ] ) ;
	
	GParted::Win_GParted win_gparted( user_devices ) ; 
	Gtk::Main::run( win_gparted ) ;

	return 0 ;
}


