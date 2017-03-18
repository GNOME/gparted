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

#include "PipeCapture.h"
#include "Utils.h"

#include <iostream>
#include <string>
#include <vector>
#include <stddef.h>
#include <string.h>
#include <glib.h>
#include <glibmm/ustring.h>
#include <glibmm/iochannel.h>

namespace GParted {

const size_t READBUF_SIZE = 64*KIBIBYTE;

PipeCapture::PipeCapture( int fd, Glib::ustring &buffer ) : fill_offset( 0 ),
                                                            cursor( 0 ),
                                                            line_start( 0 ),
                                                            callerbuf( buffer )
{
	readbuf = new char[READBUF_SIZE];
	callerbuf.clear();
	callerbuf_uptodate = true;
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
	// Reads UTF-8 characters from channel.  Provides minimal interpretation so
	// programs which use text progress bars are displayed correctly.  Captures the
	// output in a buffer and runs callbacks when updated or EOF reached.
	//
	// Data model:
	//
	//                 fill_offset
	//                 v
	//     readbuf    "XXXX......................"
	//                 ^   ^
	//                 |   end_ptr
	//                 read_ptr
	//
	//     linevec    "Current line.  Text progress bar: XXXXXXXX--------"
	//                                                           ^
	//                                                           cursor
	//
	//     capturebuf "First line\n
	//                 Current line.  Text progress bar: XXXX-----------"
	//                 ^
	//                 line_start
	//
	// Processing details:
	// Bytes are read into readbuf.  Valid UTF-8 character byte sequences are
	// recognised and, applying a simple line discipline, added into the vector of
	// characters storing the current line, linevec.  (Linevec uses UCS-4 encoding for
	// fixed sized values accessible in constant time via pointer arithmetic).  When
	// a new line character is encountered the complete current line, or when readbuf
	// is drained the partial current line, is pasted into capturebuf at the offset
	// where the last line starts.  (Capturebuf stores UTF-8 encoded characters in a
	// std::string for constant time access to line_start offset).  When readbuf
	// is drained and there are registered update callbacks, capturebuf is copied into
	// callerbuf and signal_update slot fired.  (Callerbuf stores UTF-8 encoded
	// characters in a Glib::ustring).  When EOF is encountered capturebuf is copied
	// into callerbuf if required and signal_eof slot fired.
	//
	// Golden rule:
	// Use Glib::ustrings as little as possible for large amounts of data!
	// 1) Glib::ustring::iterators use pointer access under the hood and are fast, but
	//    1.1) the Glib::ustring must only contain valid UTF-8 bytes otherwise
	//         operator++(), operator--() and operator*() may read past the end of the
	//         string until a segfault occurs; and
	//    1.2) become invalid leaving them pointing at the old memory after the
	//         underlying storage is reallocated to accommodate storing extra
	//         characters.
	// 2) Indexed character access into Glib::ustrings reads all the variable width
	//    UTF-8 encoded characters from the start of the string until the particular
	//    indexed character is reached.  Replacing characters gets exponentially
	//    slower as the string gets longer and all characters beyond those replaced
	//    have to be moved in memory.

	gsize bytes_read;
	Glib::IOStatus status = channel->read( readbuf + fill_offset, READBUF_SIZE - fill_offset, bytes_read );
	if ( status == Glib::IO_STATUS_NORMAL )
	{
		const char * read_ptr = readbuf;
		const char * end_ptr = readbuf + fill_offset + bytes_read;
		fill_offset = 0;
		while ( read_ptr < end_ptr )
		{
			const gunichar UTF8_PARTIAL = (gunichar)-2;
			const gunichar UTF8_INVALID = (gunichar)-1;
			gunichar uc = g_utf8_get_char_validated( read_ptr, end_ptr - read_ptr );
			if ( uc == UTF8_PARTIAL )
			{
				// Workaround bug in g_utf8_get_char_validated() in which
				// it reports an partial UTF-8 char when a NUL byte is
				// encountered in the middle of a multi-byte character,
				// yet there are more bytes available in the length
				// specified buffer.  Report as invalid character instead.
				int len = utf8_char_length( *read_ptr );
				if ( len == -1 || read_ptr + len <= end_ptr )
					uc = UTF8_INVALID;
			}

			if ( uc == UTF8_PARTIAL )
			{
				// Partial UTF-8 character at end of read buffer.  Copy to
				// start of read buffer.
				size_t bytes_remaining = end_ptr - read_ptr;
				memcpy( readbuf, read_ptr, bytes_remaining );
				fill_offset = bytes_remaining;
				break;
			}
			else if ( uc == UTF8_INVALID )
			{
				// Skip invalid byte.
				read_ptr ++;
				continue;
			}
			else
			{
				// Advance read pointer past the read UTF-8 character.
				const char * new_ptr = g_utf8_find_next_char( read_ptr, end_ptr );
				if ( new_ptr == read_ptr && *read_ptr == '\0' )
					// Workaround bug in g_utf8_find_next_char() which
					// stops it advancing past NUL char in buffer
					// delimited by an end pointer.
					new_ptr ++;
				read_ptr = new_ptr;
				if ( read_ptr == NULL )
					read_ptr = end_ptr;
			}

			if ( uc == '\b' )
			{
				if ( cursor > 0 )
					cursor --;
			}
			else if ( uc == '\r' )
			{
				cursor = 0;
			}
			else if ( uc == '\n' )
			{
				// Append char to current line; paste current line to
				// capture buffer; reset current line.
				linevec.push_back( '\n' );
				cursor ++;

				capturebuf.resize( line_start );
				append_unichar_vector_to_utf8( capturebuf, linevec );
				line_start = capturebuf.size();
				callerbuf_uptodate = false;

				linevec.clear();
				cursor = 0;
			}
			else if ( uc == '\x01' || uc == '\x02' )
			{
				// Skip Ctrl-A and Ctrl-B chars e2fsck uses to bracket the progress bar
				continue;
			}
			else
			{
				if ( cursor < linevec.size() )
				{
					// Replace char in current line.
					linevec[cursor] = uc;
					cursor ++;
				}
				else
				{
					// Append char to current line.
					linevec.push_back( uc );
					cursor ++;
				}
			}
		}

		// Paste partial line to capture buffer.
		capturebuf.resize( line_start );
		append_unichar_vector_to_utf8( capturebuf, linevec );
		callerbuf_uptodate = false;

		if ( ! signal_update.empty() )
		{
			// Performance optimisation, especially for large capture buffers:
			// only copy capture buffer to callers buffer and fire update
			// callbacks when there are any registered update callbacks.
			callerbuf = capturebuf;
			callerbuf_uptodate = true;
			signal_update.emit();
		}
		return true;
	}

	if ( status != Glib::IO_STATUS_EOF )
	{
		std::cerr << "Pipe IOChannel read failed" << std::endl;
	}

	if ( ! callerbuf_uptodate )
	{
		callerbuf = capturebuf;
		callerbuf_uptodate = true;
	}
	// signal completion
	signal_eof.emit();
	return false;
}

void PipeCapture::append_unichar_vector_to_utf8( std::string & str, const std::vector<gunichar> & ucvec )
{
	const size_t MAX_UTF8_BYTES = 6;
	char buf[MAX_UTF8_BYTES];
	for ( unsigned int i = 0 ; i < ucvec.size() ; i ++ )
	{
		int bytes_written = g_unichar_to_utf8( ucvec[i], buf );
		str.append( buf, bytes_written );
	}
}

int PipeCapture::utf8_char_length( unsigned char firstbyte )
{
	// Recognise the size of FSS-UTF (1992) / UTF-8 (1993) characters given the first
	// byte.  Characters can be up to 6 bytes.  (Later UTF-8 (2003) limited characters
	// to 4 bytes and 21-bits of Unicode code-space).
	// Reference:
	//     https://en.wikipedia.org/wiki/UTF-8
	if ( ( firstbyte & 0x80 ) == 0x00 )       // 0xxxxxxx - 1 byte UTF-8 char
		return 1;
	else if ( ( firstbyte & 0xE0 ) == 0xC0 )  // 110xxxxx - First byte of a 2 byte UTF-8 char
		return 2;
	else if ( ( firstbyte & 0xF0 ) == 0xE0 )  // 1110xxxx - First byte of a 3 byte UTF-8 char
		return 3;
	else if ( ( firstbyte & 0xF8 ) == 0xF0 )  // 11110xxx - First byte of a 4 byte UTF-8 char
		return 4;
	else if ( ( firstbyte & 0xFC ) == 0xF8 )  // 111110xx - First byte of a 5 byte UTF-8 char
		return 5;
	else if ( ( firstbyte & 0xFE ) == 0xFC )  // 1111110x - First byte of a 6 byte UTF-8 char
		return 6;
	else if ( ( firstbyte & 0xC0 ) == 0x80 )  // 10xxxxxx - Continuation byte
		return -1;
	else                                      // Invalid byte
		return -1;
}

PipeCapture::~PipeCapture()
{
	delete[] readbuf;
}

} // namespace GParted
