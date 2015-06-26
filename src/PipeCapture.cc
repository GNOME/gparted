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

#include "../include/PipeCapture.h"
#include <iostream>

namespace GParted {

PipeCapture::PipeCapture( int fd, Glib::ustring &string ) : buff( string ),
                                                            linestart( 0 ), cursor( 0 ), lineend( 0 )
{
	// tie fd to string
	// make channel
	channel = Glib::IOChannel::create_from_fd( fd );
	channel->set_encoding("");
}

void PipeCapture::connect_signal()
{
	// connect handler to signal input/output
	g_io_add_watch( channel->gobj(),
	                GIOCondition(G_IO_IN | G_IO_ERR | G_IO_HUP),
	                _OnReadable,
	                this );
}

gboolean PipeCapture::_OnReadable( GIOChannel *source,
				   GIOCondition condition,
				   gpointer data )
{
	PipeCapture *pc = static_cast<PipeCapture *>(data);
	gboolean rc = pc->OnReadable( Glib::IOCondition(condition) );
	return rc;
}


bool PipeCapture::OnReadable( Glib::IOCondition condition )
{
	//Read from pipe and store in buff.  Provide minimal interpretation so
	//  programs which use text progress bars are displayed correctly.
	//  Linestart, cursor and lineend are offsets into buff like this:
	//      "Previous line\n
	//       Current line.  Text progress bar: XXXXXXXXXX----------"
	//       ^                                           ^         ^
	//       linestart                                   cursor    lineend
	Glib::ustring str;
	Glib::IOStatus status = channel->read( str, 512 );
	if (status == Glib::IO_STATUS_NORMAL)
	{
		for( Glib::ustring::iterator s = str.begin(); s != str.end(); s++ )
		{
			if( *s == '\b' ) {
				if ( cursor > linestart )
					cursor -- ;
			}
			else if( *s == '\r' )
				cursor = linestart ;
			else if( *s == '\n' ) {
				cursor = lineend ;
				buff .append( 1, '\n' ) ;
				cursor ++ ;
				linestart = cursor ;
				lineend = cursor ;
			}
			else if (*s == '\x01' || *s == '\x02' )
				//Skip Ctrl-A and Ctrl-B chars e2fsck uses to bracket the progress bar
				continue;
			else {
				if ( cursor < lineend ) {
					buff .replace( cursor, 1, 1, *s ) ;
					cursor ++ ;
				}
				else {
					buff .append( 1, *s ) ;
					cursor ++ ;
					lineend = cursor ;
				}
			}
		}
		signal_update.emit();
		return true;
	}
	if (status != Glib::IO_STATUS_EOF)
		std::cerr << "Pipe IOChannel read failed" << std::endl;
	// signal completion
	signal_eof.emit();
	return false;
}

PipeCapture::~PipeCapture()
{
}

} // namespace GParted
