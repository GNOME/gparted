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

#include <string>
#include <vector>
#include <stddef.h>            // typedef size_t
#include <glib.h>              // typedef gunichar
#include <glibmm/ustring.h>
#include <glibmm/refptr.h>
#include <glibmm/iochannel.h>

namespace GParted {

// captures output pipe of subprocess into a ustring and emits a signal on eof
class PipeCapture
{
public:
	PipeCapture( int fd, Glib::ustring &buffer );
	~PipeCapture();

	void connect_signal();
	sigc::signal<void> signal_eof;
	sigc::signal<void> signal_update;

private:
	bool OnReadable( Glib::IOCondition condition );
	static gboolean _OnReadable( GIOChannel *source,
	                             GIOCondition condition,
	                             gpointer data );
	static void append_unichar_vector_to_utf8( std::string & str,
	                                           const std::vector<gunichar> & ucvec );
	static int utf8_char_length( unsigned char firstbyte );

	Glib::RefPtr<Glib::IOChannel> channel;  // Wrapper around fd
	char * readbuf;                 // Bytes read from IOChannel (fd)
	size_t fill_offset;             // Filling offset into readbuf
	std::vector<gunichar> linevec;  // Current line stored as UCS-4 characters
	size_t cursor;                  // Cursor position index into linevec
	std::string capturebuf;         // Captured output as UTF-8 characters
	size_t line_start;              // Index into bytebuf where current line starts
	Glib::ustring & callerbuf;      // Reference to caller supplied buffer
	bool callerbuf_uptodate;        // Has capturebuf changed since last copied to callerbuf?
};

} // namepace GParted

#endif /* GPARTED_PIPECAPTURE_H */
