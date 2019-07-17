/* Copyright (C) 2019 Mike Fleetwood
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

/* Test ext2
 *
 * Test the derived FileSystem interface classes which call the file system specific
 * executables.  Rather than mocking command execution and returned output just run real
 * commands, effectively making this integration testing.
 *
 * Test case setup determines the file system supported actions using
 * get_filesystem_support() and individual tests are skipped if a feature is not
 * supported, just as GParted does for it's actions.
 *
 * Each test creates it's own sparse image file and a fresh file system, performs a test
 * on one FileSystem interface call and deletes the image file.  This makes each test
 * independent and allows them to run as a non-root user, provided the file system command
 * itself doesn't require root.  Errors reported for a failed interface call will include
 * the GParted OperationDetails, which in turn includes the file system specific command
 * used and stdout and stderr from it's execution.
 */


#include "GParted_Core.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"
#include "ext2.h"
#include "gtest/gtest.h"

#include <iostream>
#include <fstream>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <gtkmm.h>
#include <glibmm/thread.h>
#include <glibmm/ustring.h>
#include <parted/parted.h>


namespace GParted
{


// Print method for OperationDetailStatus.
std::ostream& operator<<(std::ostream& out, const OperationDetailStatus od_status)
{
	switch (od_status)
	{
		case STATUS_NONE:     out << "NONE";     break;
		case STATUS_EXECUTE:  out << "EXECUTE";  break;
		case STATUS_SUCCESS:  out << "SUCCESS";  break;
		case STATUS_ERROR:    out << "ERROR";    break;
		case STATUS_INFO:     out << "INFO";     break;
		case STATUS_WARNING:  out << "WARNING";  break;
		default:                                 break;
	}
	return out;
}


// Print method for an OperationDetail object.
std::ostream& operator<<(std::ostream& out, const OperationDetail& od)
{
	// FIXME: Strip markup from the printed description
	out << od.get_description();
	Glib::ustring elapsed = od.get_elapsed_time();
	if (! elapsed.empty())
		out << "    " << elapsed;
	if (od.get_status() != STATUS_NONE)
		out << "  (" << od.get_status() << ")";
	out << "\n";

	for (unsigned int i = 0; i < od.get_childs().size(); i++)
	{
		out << *od.get_childs()[i];
	}
	return out;
}


// Google Test 1.8.1 (and earlier) doesn't implement run-time test skipping so implement
// our own for GParted run-time detected of unsupported file system features.
// Ref:
//     Skipping tests at runtime with GTEST_SKIP() #1544
//     https://github.com/google/googletest/pull/1544
//     (Merged after Google Test 1.8.1)
#define SKIP_IF_FS_DOESNT_SUPPORT(opt)                                           \
	if (s_ext2_support.opt != FS::EXTERNAL)                                  \
	{                                                                        \
		std::cout << __FILE__ << ":" << __LINE__ << ": Skip test.  "     \
		          << "Not supported or support not found" << std::endl;  \
		return;                                                          \
	}


class ext2Test : public ::testing::Test
{
protected:
	// Initialise top-level operation detail object with description ...
	ext2Test() : m_operation_detail("Operation details:", STATUS_NONE)  {};

	virtual void extra_setup();
	virtual void TearDown();

	static void SetUpTestCase();
	static void TearDownTestCase();

	static FileSystem* s_ext2_obj;
	static FS          s_ext2_support;
	static const char* s_image_name;

	Partition       m_partition;
	OperationDetail m_operation_detail;
};


FileSystem* ext2Test::s_ext2_obj     = NULL;
FS          ext2Test::s_ext2_support;
const char* ext2Test::s_image_name   = "test_ext2.img";


void ext2Test::extra_setup()
{
	const Byte_Value ImageSize = 256*MEBIBYTE;

	// Create new 256M image file to work with.
	unlink(s_image_name);
	int fd = open(s_image_name, O_WRONLY|O_CREAT|O_NONBLOCK, 0666);
	ASSERT_GE(fd, 0) << "Failed to create image file '" << s_image_name << "'.  errno="
	                 << errno << "," << strerror(errno);
	ASSERT_EQ(ftruncate(fd, (off_t)ImageSize), 0) << "Failed to set image file '" << s_image_name << "' to size "
	                                              << ImageSize << ".  errno=" << errno << "," << strerror(errno);
	close(fd);

	// Use libparted to get the sector size etc. of the image file.
	PedDevice* lp_device = ped_device_get(s_image_name);
	ASSERT_TRUE(lp_device != NULL);

	// Prepare partition object spanning whole of the image file.
	m_partition.set_unpartitioned(s_image_name,
	                              lp_device->path,
	                              FS_EXT2,
	                              lp_device->length,
	                              lp_device->sector_size,
	                              false);

	ped_device_destroy(lp_device);
	lp_device = NULL;
}


void ext2Test::TearDown()
{
	unlink(s_image_name);
}


// Common test case initialisation creating ext2 interface object and querying supported
// operations.
void ext2Test::SetUpTestCase()
{
	s_ext2_obj = new ext2(FS_EXT2);
	s_ext2_support = s_ext2_obj->get_filesystem_support();
}


// Common test case teardown destroying the ext2 interface object.
void ext2Test::TearDownTestCase()
{
	delete s_ext2_obj;
	s_ext2_obj = NULL;
}


TEST_F(ext2Test, Create)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	extra_setup();
	// Call create, check for success and print operation details on failure.
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;
}


}  // namespace GParted


// Custom Google Test main().
// Reference:
// *   Google Test, Primer, Writing the main() function
//     https://github.com/google/googletest/blob/master/googletest/docs/primer.md#writing-the-main-function
int main(int argc, char** argv)
{
	printf("Running main() from %s\n", __FILE__);
	testing::InitGoogleTest(&argc, argv);

	// Initialise threading in GParted to allow FileSystem interface classes to
	// successfully use Utils:: and Filesystem::execute_command().
	GParted::GParted_Core::mainthread = Glib::Thread::self();
	Gtk::Main gtk_main = Gtk::Main();

	return RUN_ALL_TESTS();
}
