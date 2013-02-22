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
