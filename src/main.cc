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

#include <glibmm.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/application.h>

Glib::RefPtr<Gtk::Application> app;
GParted::Win_GParted *win_gparted;

void activate()
{
	g_assert(win_gparted);
	win_gparted->present();
}

void startup()
{
	//Set WM_CLASS X Window property for correct naming under GNOME Shell /*TODO*/
	/*gdk_set_program_class( "GParted" ) ;*/

	//deal with arguments../*TODO*/
	/*std::vector<Glib::ustring> user_devices(argv + 1, argv + argc);*/
	std::vector<Glib::ustring> user_devices;
	win_gparted = new GParted::Win_GParted(user_devices);

	app->signal_activate().connect(sigc::ptr_fun(activate));

	app->add_window(*win_gparted);
}

int main( int argc, char *argv[] )
{
	GParted::GParted_Core::mainthread = Glib::Thread::self(); /*TODO*/

	//i18n /*TODO*/
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR ) ;
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" ) ;
	textdomain( GETTEXT_PACKAGE ) ;

	app = Gtk::Application::create("org.gnome.GParted");
	/*uniqueness of your application is scoped to the current session*/

	//check UID /*TODO*/
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

	app->signal_startup().connect(sigc::ptr_fun(startup));
	app->run(argc, argv);

	return 0 ;
}


