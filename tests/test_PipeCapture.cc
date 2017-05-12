/* Copyright (C) 2017 Mike Fleetwood
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

/* Test PipeCapture
 *
 * All the tests work by creating a pipe(3) and using a separate thread to write data into
 * the pipe with PipeCapture running in the initial thread.  Captured data is then checked
 * that it matches the input.
 */

#include "PipeCapture.h"
#include "gtest/gtest.h"

#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <sigc++/sigc++.h>
#include <glib.h>
#include <glibmm.h>

namespace GParted
{

// Explicit test fixture class with common variables and methods used in each test.
// Reference:
//     Google Test, Primer, Test Fixtures: Using the Same Data Configuration for Multiple Tests
class PipeCaptureTest : public ::testing::Test
{
protected:
	PipeCaptureTest() : eof_signalled( false )  {};

	virtual void SetUp();
	virtual void TearDown();

	static gboolean main_loop_quit( gpointer data );
	void writer_thread( const std::string & str );
	void run_writer_thread();

	static const size_t ReaderFD = 0;
	static const size_t WriterFD = 1;

	std::string inputstr;
	Glib::ustring capturedstr;
	bool eof_signalled;
	int pipefds[2];
	Glib::RefPtr<Glib::MainLoop> glib_main_loop;

public:
	void eof_callback()  { eof_signalled = true; };
};

// Further setup PipeCaptureTest fixture before running each test.  Create pipe and Glib
// main loop object.
void PipeCaptureTest::SetUp()
{
	ASSERT_TRUE( pipe( pipefds ) == 0 ) << "Failed to create pipe.  errno="
	                                    << errno << "," << strerror( errno );
	glib_main_loop = Glib::MainLoop::create();
}

// Tear down fixture after running each test.  Close reading end of the pipe.  Also
// re-closed the writing end of the pipe, just in case something went wrong in the test.
void PipeCaptureTest::TearDown()
{
	ASSERT_TRUE( close( pipefds[ReaderFD] ) == 0 ) << "Failed to close reading end of pipe.  errno="
	                                               << errno << "," << strerror( errno );
	close( pipefds[WriterFD] );
}

// Callback used to end the currently running Glib main loop.
gboolean PipeCaptureTest::main_loop_quit( gpointer data )
{
	static_cast<PipeCaptureTest *>( data )->glib_main_loop->quit();
	return false;  // One shot g_idle_add() callback
}

// Write the string into the pipe and close the pipe for writing.  Registers callback to
// end the currently running Glib main loop.
void PipeCaptureTest::writer_thread( const std::string & str )
{
	const size_t BlockSize = 4096;
	const char * writebuf = str.data();
	size_t remaining_size = str.length();
	while ( remaining_size > 0 )
	{
		size_t write_size = ( remaining_size > BlockSize ) ? BlockSize : remaining_size;
		ssize_t written = write( pipefds[WriterFD], writebuf, write_size );
		if ( written <= 0 )
		{
			ADD_FAILURE() << __func__ << "(): Failed to write to pipe.  errno="
			              << errno << "," << strerror( errno );
			break;
		}
		remaining_size -= written;
		writebuf += written;
	}
	ASSERT_TRUE( close( pipefds[WriterFD] ) == 0 ) << "Failed to close writing end of pipe.  errno="
	                                               << errno << "," << strerror( errno );
	g_idle_add( main_loop_quit, this );
}

// Create writer thread and run the Glib main loop.
void PipeCaptureTest::run_writer_thread()
{
	Glib::Thread::create( sigc::bind( sigc::mem_fun( *this, &PipeCaptureTest::writer_thread ),
	                                  inputstr ),
	                      false );
	glib_main_loop->run();
}

TEST_F( PipeCaptureTest, EmptyPipe )
{
	// Test capturing 0 bytes with no on EOF callback registered.
	inputstr = "";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.connect_signal();
	run_writer_thread();
	EXPECT_STREQ( inputstr.c_str(), capturedstr.c_str() );
	EXPECT_FALSE( eof_signalled );
}

TEST_F( PipeCaptureTest, EmptyPipeWithEOF )
{
	// Test capturing 0 bytes and registered on EOF callback occurs.
	inputstr = "";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.signal_eof.connect( sigc::mem_fun( *this, &PipeCaptureTest::eof_callback ) );
	pc.connect_signal();
	run_writer_thread();
	EXPECT_STREQ( inputstr.c_str(), capturedstr.c_str() );
	EXPECT_TRUE( eof_signalled );
}

TEST_F( PipeCaptureTest, ShortASCIIText )
{
	// Test capturing small amount of ASCII text.
	inputstr = "The quick brown fox jumps over the lazy dog";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.signal_eof.connect( sigc::mem_fun( *this, &PipeCaptureTest::eof_callback ) );
	pc.connect_signal();
	run_writer_thread();
	EXPECT_STREQ( inputstr.c_str(), capturedstr.c_str() );
	EXPECT_TRUE( eof_signalled );
}

}  // namespace GParted
