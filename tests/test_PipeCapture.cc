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
 * that it either matches the input or different expected output depending on the features
 * being tested.
 */

#include "PipeCapture.h"
#include "gtest/gtest.h"

#include <stddef.h>
#include <stdio.h>
#include <sstream>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <string>
#include <sigc++/sigc++.h>
#include <glib.h>
#include <glibmm.h>

namespace GParted
{

// Repeat a C++ string count times, where count >= 0.
static std::string repeat( const std::string & str, size_t count )
{
	std::string result = "";
	while ( count -- > 0 )
		result += str;
	return result;
}

// Number of bytes of binary data to compare and report.
const size_t BinaryStringDiffSize = 16;

// Format up to 16 bytes of binary data ready for printing as:
//      Hex offset     ASCII text          Hex bytes
//     "0x000000000  \"ABCDEFGHabcdefgh\"  41 42 43 44 45 46 47 48 61 62 63 64 65 66 67 68"
std::string BinaryStringToPrint( size_t offset, const char * s, size_t len )
{
	std::ostringstream result;

	result << "0x";
	result.fill( '0' );
	result << std::setw( 8 ) << std::hex << std::uppercase << offset << "  \"";

	size_t i;
	for ( i = 0 ; i < BinaryStringDiffSize && i < len ; i ++ )
		result.put( ( isprint( s[i] ) ) ? s[i] : '.' );
	result.put( '\"' );

	if ( len > 0 )
	{
		for ( ; i < BinaryStringDiffSize ; i ++ )
			result.put( ' ' );
		result.put( ' ' );

		for ( i = 0 ; i < BinaryStringDiffSize && i < len ; i ++ )
			result << " "
			       << std::setw( 2 ) << std::hex << std::uppercase
			       << (unsigned int)(unsigned char)s[i];
	}

	return result.str();
}

// Helper to construct and return message for equality assertion of C++ strings containing
// binary data used in:
//     EXPECT_BINARYSTRINGEQ( str1, str2 )
::testing::AssertionResult CompareHelperBinaryStringEQ( const char * lhs_expr, const char * rhs_expr,
                                                        const std::string & lhs, const std::string & rhs )
{
	// Loop comparing binary data in 16 byte amounts, stopping and reporting the first
	// difference encountered.
	bool diff = false;
	const char * p1 = lhs.data();
	const char * p2 = rhs.data();
	size_t len1 = lhs.length();
	size_t len2 = rhs.length();
	while ( len1 > 0 || len2 > 0 )
	{
		size_t cmp_span = BinaryStringDiffSize;
		cmp_span = ( len1 < cmp_span ) ? len1 : cmp_span;
		cmp_span = ( len2 < cmp_span ) ? len2 : cmp_span;
		if ( cmp_span < BinaryStringDiffSize && len1 != len2 )
		{
			diff = true;
			break;
		}
		if ( memcmp( p1, p2, cmp_span ) != 0 )
		{
			diff = true;
			break;
		}
		p1 += cmp_span;
		p2 += cmp_span;
		len1 -= cmp_span;
		len2 -= cmp_span;
	}

	if ( ! diff )
		return ::testing::AssertionSuccess();
	else
	{
		size_t offset = p1 - lhs.data();
		return ::testing::AssertionFailure()
		       << "      Expected: " << lhs_expr << "\n"
		       << "     Of length: " << lhs.length() << "\n"
		       << "To be equal to: " << rhs_expr << "\n"
		       << "     Of length: " << rhs.length() << "\n"
		       << "With first binary difference:\n"
		       << "< " << BinaryStringToPrint( offset, p1, len1 ) << "\n"
		       << "--\n"
		       << "> " << BinaryStringToPrint( offset, p2, len2 );
	}
}

// Nonfatal assertion that binary data in C++ strings are equal.
#define EXPECT_BINARYSTRINGEQ(str1, str2)  \
	EXPECT_PRED_FORMAT2(CompareHelperBinaryStringEQ, str1, str2)

// Explicit test fixture class with common variables and methods used in each test.
// Reference:
//     Google Test, Primer, Test Fixtures: Using the Same Data Configuration for Multiple Tests
class PipeCaptureTest : public ::testing::Test
{
protected:
	PipeCaptureTest() : capturedstr( "text to be replaced" ),
	                    eof_signalled( false ), update_signalled( 0U )  {};

	virtual void SetUp();
	virtual void TearDown();

	static gboolean main_loop_quit( gpointer data );
	void writer_thread( const std::string & str );
	void run_writer_thread();

	static const size_t ReaderFD = 0;
	static const size_t WriterFD = 1;

	std::string inputstr;
	std::string expectedstr;
	Glib::ustring capturedstr;
	bool eof_signalled;
	unsigned update_signalled;
	int pipefds[2];
	Glib::RefPtr<Glib::MainLoop> glib_main_loop;

public:
	void eof_callback()  { eof_signalled = true; };
	void update_callback_leading_match();
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

// Callback fired from CapturePipe counting calls and ensuring captured string matches
// leading portion of input string.
void PipeCaptureTest::update_callback_leading_match()
{
	update_signalled ++;
	EXPECT_BINARYSTRINGEQ( inputstr.substr( 0, capturedstr.raw().length() ),
	                       capturedstr.raw() );
	if ( HasFailure() )
		// No point trying to PipeCapture the rest of the input and report
		// hundreds of further failures in the same test, so end the currently
		// running Glib main loop immediately.
		// References:
		// *   Google Test, AdvancedGuide, Propagating Fatal Failures
		// *   Google Test, AdvancedGuide, Checking for Failures in the Current Test
		glib_main_loop->quit();
}

TEST_F( PipeCaptureTest, EmptyPipe )
{
	// Test capturing 0 bytes with no on EOF callback registered.
	inputstr = "";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.connect_signal();
	run_writer_thread();
	EXPECT_BINARYSTRINGEQ( inputstr, capturedstr.raw() );
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
	EXPECT_BINARYSTRINGEQ( inputstr, capturedstr.raw() );
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
	EXPECT_BINARYSTRINGEQ( inputstr, capturedstr.raw() );
	EXPECT_TRUE( eof_signalled );
}

TEST_F( PipeCaptureTest, LongASCIIText )
{
	// Test capturing 1 MiB of ASCII text (requiring multiple reads in PipeCapture).
	inputstr = repeat( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_\n", 16384 );
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.signal_eof.connect( sigc::mem_fun( *this, &PipeCaptureTest::eof_callback ) );
	pc.connect_signal();
	run_writer_thread();
	EXPECT_BINARYSTRINGEQ( inputstr, capturedstr.raw() );
	EXPECT_TRUE( eof_signalled );
}

TEST_F( PipeCaptureTest, LongASCIITextWithUpdate )
{
	// Test capturing 1 MiB of ASCII text, that registered update callback occurs and
	// intermediate captured string is a leading match for the input string.
	inputstr = repeat( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_\n", 16384 );
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.signal_eof.connect( sigc::mem_fun( *this, &PipeCaptureTest::eof_callback ) );
	pc.signal_update.connect( sigc::mem_fun( *this, &PipeCaptureTest::update_callback_leading_match ) );
	pc.connect_signal();
	run_writer_thread();
	EXPECT_BINARYSTRINGEQ( inputstr, capturedstr.raw() );
	EXPECT_GT( update_signalled, 0U );
	EXPECT_TRUE( eof_signalled );
}

TEST_F( PipeCaptureTest, MinimalBinaryCrash777973 )
{
	// Test for bug #777973.  Minimal test case of binary data returned by fsck.fat
	// as file names from a very corrupt FAT, leading to GParted crashing from a
	// segmentation fault.
	inputstr = "/LOST.DIR/!\xE2\x95\x9F\xE2\x88\xA9\xC2\xA0!\xE2\x95\x9F\xE2\x88\xA9\xC2";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.signal_eof.connect( sigc::mem_fun( *this, &PipeCaptureTest::eof_callback ) );
	pc.connect_signal();
	run_writer_thread();
	// Final \xC2 byte is part of an incomplete UTF-8 character so will be skipped by
	// PipeCapture.
	expectedstr = "/LOST.DIR/!\xE2\x95\x9F\xE2\x88\xA9\xC2\xA0!\xE2\x95\x9F\xE2\x88\xA9";
	EXPECT_BINARYSTRINGEQ( expectedstr, capturedstr.raw() );
	EXPECT_TRUE( eof_signalled );
}

TEST_F( PipeCaptureTest, ReadEmbeddedNULCharacter )
{
	// Test embedded NUL character in the middle of the input is read correctly.
	const char * buf = "ABC\0EF";
	inputstr = std::string( buf, 6 );
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.signal_eof.connect( sigc::mem_fun( *this, &PipeCaptureTest::eof_callback ) );
	pc.connect_signal();
	run_writer_thread();
	EXPECT_BINARYSTRINGEQ( inputstr, capturedstr.raw() );
	EXPECT_TRUE( eof_signalled );
}

TEST_F( PipeCaptureTest, ReadNULByteInMiddleOfMultiByteUTF8Character )
{
	// Test NUL byte in the middle of reading a multi-byte UTF-8 character.
	const char * buf = "\xC0\x00_45678";
	inputstr = std::string( buf, 8 );
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.signal_eof.connect( sigc::mem_fun( *this, &PipeCaptureTest::eof_callback ) );
	pc.connect_signal();
	run_writer_thread();
	// Initial \xC0 byte is part of an incomplete UTF-8 characters so will be skipped
	// by PipeCapture.
	buf = "\x00_45678";
	expectedstr = std::string( buf, 7 );
	EXPECT_BINARYSTRINGEQ( expectedstr, capturedstr.raw() );
	EXPECT_TRUE( eof_signalled );
}

TEST_F( PipeCaptureTest, LineDisciplineCarriageReturn )
{
	// Test PipeCapture line discipline processes carriage return character.
	inputstr = "1111\n2222\r33";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.connect_signal();
	run_writer_thread();
	expectedstr = "1111\n3322";
	EXPECT_BINARYSTRINGEQ( expectedstr, capturedstr.raw() );
}

TEST_F( PipeCaptureTest, LineDisciplineCarriageReturn2 )
{
	// Test PipeCapture line discipline processes multiple carriage return characters.
	inputstr = "1111\n2222\r33\r\r4";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.connect_signal();
	run_writer_thread();
	expectedstr = "1111\n4322";
	EXPECT_BINARYSTRINGEQ( expectedstr, capturedstr.raw() );
}

TEST_F( PipeCaptureTest, LineDisciplineBackspace )
{
	// Test PipeCapture line discipline processes backspace character.
	inputstr = "1111\n2222\b33";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.connect_signal();
	run_writer_thread();
	expectedstr = "1111\n22233";
	EXPECT_BINARYSTRINGEQ( expectedstr, capturedstr.raw() );
}

TEST_F( PipeCaptureTest, LineDisciplineBackspace2 )
{
	// Test PipeCapture line discipline processes too many backspace characters moving
	// the cursor back only to the beginning of the current line.
	inputstr = "1111\n2222\b\b\b\b\b\b33\b4";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.connect_signal();
	run_writer_thread();
	expectedstr = "1111\n3422";
	EXPECT_BINARYSTRINGEQ( expectedstr, capturedstr.raw() );
}

TEST_F( PipeCaptureTest, LineDisciplineSkipCtrlAB )
{
	// Test PipeCapture line discipline skips Ctrl-A and Ctrl-B.
	inputstr = "ij\x01kl\x02mn";
	PipeCapture pc( pipefds[ReaderFD], capturedstr );
	pc.connect_signal();
	run_writer_thread();
	expectedstr = "ijklmn";
	EXPECT_BINARYSTRINGEQ( expectedstr, capturedstr.raw() );
}

}  // namespace GParted

// Custom Google Test main() which also initialises the Glib threading system for
// distributions with glib/glibmm before version 2.32.
// References:
// *   Google Test, Primer, Writing the main() Function
// *   Deprecated thread API, g_thread_init()
//     https://developer.gnome.org/glib/stable/glib-Deprecated-Thread-APIs.html#g-thread-init
int main( int argc, char **argv )
{
	printf("Running main() from %s\n", __FILE__ );
	testing::InitGoogleTest( &argc, argv );

	Glib::thread_init();

	return RUN_ALL_TESTS();
}
