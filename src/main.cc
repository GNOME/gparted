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
#include <iostream>
#include <stdlib.h>

Glib::RefPtr<Gtk::Application> app;
GParted::Win_GParted *win_gparted;

void activate()
{
	if (!win_gparted)
	{
		win_gparted = new GParted::Win_GParted(std::vector<Glib::ustring>());
		app->add_window(*win_gparted);
	}

	win_gparted->present();
}

void file_open(const Gio::Application::type_vec_files& files,
               const Glib::ustring&)
{
	if (!win_gparted)
	{
		std::vector<Glib::ustring> user_devices;
		for (Gio::Application::type_vec_files::const_iterator
			 it = files.begin(); it != files.end(); ++it)
		{
			user_devices.push_back((*it)->get_path());
		}

		win_gparted = new GParted::Win_GParted(user_devices);
		app->add_window(*win_gparted);

		win_gparted->present();
	}
}

void startup()
{
	GParted::GParted_Core::mainthread = Glib::Thread::self();

	// Set WM_CLASS X Window property for correct naming under GNOME Shell
	gdk_set_program_class("GParted");

	// Display version and configuration info when starting for command line users.
	std::cout << GParted::GParted_Core::get_version_and_config_string() << std::endl;

	//check UID
	if ( getuid() != 0 )
	{
		const Glib::ustring error_msg(_("Root privileges are required for running GParted"));
		std::cerr << error_msg << std::endl;

		Gtk::MessageDialog dialog(error_msg, false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK);
		dialog .set_secondary_text(
				_("Since GParted is a powerful tool capable of destroying partition tables and vast amounts of data, only root may run it.") ) ;
		
		dialog .run() ;
		app->quit();
	}
}

void shutdown()
{
	delete win_gparted;
}

int main( int argc, char *argv[] )
{
	//i18n
	bindtextdomain( GETTEXT_PACKAGE, GNOMELOCALEDIR ) ;
	bind_textdomain_codeset( GETTEXT_PACKAGE, "UTF-8" ) ;
	textdomain( GETTEXT_PACKAGE ) ;

	app = Gtk::Application::create("org.gnome.GParted",
	                                 Gio::APPLICATION_NON_UNIQUE
	                               | Gio::APPLICATION_HANDLES_OPEN);

	app->signal_startup().connect(sigc::ptr_fun(startup));
	app->signal_open().connect(sigc::ptr_fun(file_open));
	app->signal_activate().connect(sigc::ptr_fun(activate));
	app->signal_shutdown().connect(sigc::ptr_fun(shutdown));

	app->run(argc, argv);

	return EXIT_SUCCESS;
}
