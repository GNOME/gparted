/* Copyright (C) 2013 Phillip Susi
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

#ifndef GPARTED_PIPECAPTURE_H
#define GPARTED_PIPECAPTURE_H

#include <glibmm/ustring.h>
#include <glibmm/main.h>
#include <glibmm/iochannel.h>

namespace GParted {

// captures output pipe of subprocess into a ustring and emits a signal on eof
class PipeCapture
{
	Glib::ustring &buff;
	Glib::ustring::size_type linestart ;
	Glib::ustring::size_type cursor ;
	Glib::ustring::size_type lineend ;
	Glib::RefPtr<Glib::IOChannel> channel;
	bool OnReadable( Glib::IOCondition condition );
	static gboolean _OnReadable( GIOChannel *source,
	                             GIOCondition condition,
	                             gpointer data );
public:
	PipeCapture( int fd, Glib::ustring &buffer );
	void connect_signal();
	~PipeCapture();
	sigc::signal<void> signal_eof;
	sigc::signal<void> signal_update;
};

} // namepace GParted

#endif /* GPARTED_PIPECAPTURE_H */
