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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../include/PipeCapture.h"
#include <iostream>

namespace GParted {

PipeCapture::PipeCapture( int fd, Glib::ustring &string ) : buff( string ), backcount( 0 ), linelength( 0 )
{
	// tie fd to string
	// make channel
	channel = Glib::IOChannel::create_from_fd( fd );
}

void PipeCapture::connect_signal( int fd )
{
	// connect handler to signal input/output
	connection = Glib::signal_io().connect(
		sigc::mem_fun( *this, &PipeCapture::OnReadable ),
		fd,
		Glib::IO_IN | Glib::IO_HUP | Glib::IO_ERR );
}

bool PipeCapture::OnReadable( Glib::IOCondition condition )
{
	// read from pipe and store in buff
	Glib::ustring str;
	Glib::IOStatus status = channel->read( str, 512 );
	if (status == Glib::IO_STATUS_NORMAL)
	{
		for( Glib::ustring::iterator s = str.begin();
		     s != str.end(); s++ )
		{
			if( *s == '\b' )
				backcount++;
			else if( *s == '\r' )
				backcount = linelength;
			else if( *s == '\n' ) {
				linelength = 0;
				buff += '\n';
				backcount = 0;
			}
			else {
				if (backcount) {
					buff.erase( buff.length() - backcount, backcount );
					linelength -= backcount;
					backcount = 0;
				}
				buff += *s;
				++linelength;
			}
		}
		update();
		return true;
	}
	if (status != Glib::IO_STATUS_EOF)
		std::cerr << "Pipe IOChannel read failed" << std::endl;
	// signal completion
	connection.disconnect();
	eof();
	return false;
}

PipeCapture::~PipeCapture()
{
	connection.disconnect();
}

} // namespace GParted
