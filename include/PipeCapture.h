#ifndef PIPECAPTURE_H
#define PIPECAPTURE_H

#include <glibmm/ustring.h>
#include <glibmm/main.h>
#include <glibmm/iochannel.h>

namespace GParted {

// captures output pipe of subprocess into a ustring and emits a signal on eof
class PipeCapture
{
	Glib::ustring &buff;
	unsigned int backcount;
	unsigned int linelength;
	Glib::RefPtr<Glib::IOChannel> channel;
	sigc::connection connection;
	bool OnReadable( Glib::IOCondition condition );
public:
	PipeCapture( int fd, Glib::ustring &buffer );
	void connect_signal( int fd );
	~PipeCapture();
	sigc::signal<void> eof;
	sigc::signal<void> update;
};

} // namepace GParted

#endif
