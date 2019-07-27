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
#include <vector>
#include <stdlib.h>
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


// Hacky XML parser which strips italic and bold markup added in
// OperationDetail::set_description() and reverts just these 5 characters &<>'" encoded by
// Glib::Markup::escape_text() -> g_markup_escape_text() -> append_escaped_text().
Glib::ustring strip_markup(const Glib::ustring& str)
{
	size_t len = str.length();
	size_t i = 0;
	Glib::ustring ret;
	ret.reserve(len);
	while (i < len)
	{
		if (str.compare(i, 3, "<i>") == 0)
			i += 3;
		else if (str.compare(i, 4, "</i>") == 0)
			i += 4;
		else if (str.compare(i, 3, "<b>") == 0)
			i += 3;
		else if (str.compare(i, 4, "</b>") == 0)
			i += 4;
		else if (str.compare(i, 5, "&amp;") == 0)
		{
			ret.push_back('&');
			i += 5;
		}
		else if (str.compare(i, 4, "&lt;") == 0)
		{
			ret.push_back('<');
			i += 4;
		}
		else if (str.compare(i, 4, "&gt;") == 0)
		{
			ret.push_back('>');
			i += 4;
		}
		else if (str.compare(i, 6, "&apos;") == 0)
		{
			ret.push_back('\'');
			i += 6;
		}
		else if (str.compare(i, 6, "&quot;") == 0)
		{
			ret.push_back('"');
			i += 6;
		}
		else
		{
			ret.push_back(str[i]);
			i++;
		}
	}
	return ret;
}


// Print method for the messages in a Partition object.
std::ostream& operator<<(std::ostream& out, const Partition& partition)
{
	const std::vector<Glib::ustring> messages = partition.get_messages();
	out << "Partition messages:\n";
	for (unsigned int i = 0; i < messages.size(); i++)
		out << messages[i];
	return out;
}


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
	out << strip_markup(od.get_description());
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
#define SKIP_IF_FS_DOESNT_SUPPORT(opt)                                                    \
	if (s_ext2_support.opt != FS::EXTERNAL)                                           \
	{                                                                                 \
		std::cout << __FILE__ << ":" << __LINE__ << ": Skip test.  "              \
		          << #opt << " not supported or support not found" << std::endl;  \
		return;                                                                   \
	}


const Byte_Value IMAGESIZE_Default = 256*MEBIBYTE;
const Byte_Value IMAGESIZE_Larger  = 512*MEBIBYTE;


class ext2Test : public ::testing::Test
{
protected:
	// Initialise top-level operation detail object with description ...
	ext2Test() : m_operation_detail("Operation details:", STATUS_NONE)  {};

	virtual void extra_setup(Byte_Value size = IMAGESIZE_Default);
	virtual void TearDown();

	static void SetUpTestCase();
	static void TearDownTestCase();

	virtual void reload_partition();
	virtual void resize_image(Byte_Value new_size);
	virtual void shrink_partition(Byte_Value size);

	static FileSystem* s_ext2_obj;
	static FS          s_ext2_support;
	static const char* s_image_name;

	Partition       m_partition;
	OperationDetail m_operation_detail;
};


FileSystem* ext2Test::s_ext2_obj     = NULL;
FS          ext2Test::s_ext2_support;
const char* ext2Test::s_image_name   = "test_ext2.img";


void ext2Test::extra_setup(Byte_Value size)
{
	// Create new image file to work with.
	unlink(s_image_name);
	int fd = open(s_image_name, O_WRONLY|O_CREAT|O_NONBLOCK, 0666);
	ASSERT_GE(fd, 0) << "Failed to create image file '" << s_image_name << "'.  errno="
	                 << errno << "," << strerror(errno);
	ASSERT_EQ(ftruncate(fd, (off_t)size), 0) << "Failed to set image file '" << s_image_name << "' to size "
	                                         << size << ".  errno=" << errno << "," << strerror(errno);
	close(fd);

	reload_partition();
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


// (Re)initialise m_partition as a Partition object spanning the whole of the image file
// with file system type only.  No file system usage, label or UUID.
void ext2Test::reload_partition()
{
	m_partition.Reset();

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


void ext2Test::resize_image(Byte_Value new_size)
{
	int fd = open(s_image_name, O_WRONLY|O_NONBLOCK);
	ASSERT_GE(fd, 0) << "Failed to open image file '" << s_image_name << "'.  errno="
	                 << errno << "," << strerror(errno);
	ASSERT_EQ(ftruncate(fd, (off_t)new_size), 0) << "Failed to resize image file '" << s_image_name << "' to size "
	                                             << new_size << ".  errno=" << errno << "," << strerror(errno);
	close(fd);
}


void ext2Test::shrink_partition(Byte_Value new_size)
{
	ASSERT_LE(new_size, m_partition.get_byte_length()) << __func__ << "(): TEST_BUG: Cannot grow Partition object size";
	Sector new_sectors = (new_size + m_partition.sector_size - 1) / m_partition.sector_size;
	m_partition.sector_end = new_sectors;
}


TEST_F(ext2Test, Create)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	extra_setup();
	// Call create, check for success and print operation details on failure.
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_F(ext2Test, CreateAndReadUsage)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(read);

	extra_setup();
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	reload_partition();
	s_ext2_obj->set_used_sectors(m_partition);
	// Test file system usage is reported correctly.
	// Used is between 0 and length.
	EXPECT_LE(0, m_partition.sectors_used);
	EXPECT_LE(m_partition.sectors_used, m_partition.get_sector_length());
	// Unused is between 0 and length.
	EXPECT_LE(0, m_partition.sectors_unused);
	EXPECT_LE(m_partition.sectors_unused, m_partition.get_sector_length());
	// Unallocated is 0.
	EXPECT_EQ(m_partition.sectors_unallocated, 0);
	// Used + unused = length.
	EXPECT_EQ(m_partition.sectors_used + m_partition.sectors_unused, m_partition.get_sector_length());

	// Test messages from read operation are empty or print them.
	EXPECT_TRUE(m_partition.get_messages().empty()) << m_partition;
}


TEST_F(ext2Test, CreateAndReadLabel)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(read_label);

	const char* fs_label = "TEST_LABEL";
	extra_setup();
	m_partition.set_filesystem_label(fs_label);
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test reading the label is successful.
	reload_partition();
	s_ext2_obj->read_label(m_partition);
	EXPECT_STREQ(fs_label, m_partition.get_filesystem_label().c_str());

	// Test messages from read operation are empty or print them.
	EXPECT_TRUE(m_partition.get_messages().empty()) << m_partition;
}


TEST_F(ext2Test, CreateAndReadUUID)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(read_uuid);

	extra_setup();
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test reading the UUID is successful.
	reload_partition();
	s_ext2_obj->read_uuid(m_partition);
	EXPECT_EQ(m_partition.uuid.size(), 36U);

	// Test messages from read operation are empty or print them.
	EXPECT_TRUE(m_partition.get_messages().empty()) << m_partition;
}


TEST_F(ext2Test, CreateAndWriteLabel)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(write_label);

	extra_setup();
	m_partition.set_filesystem_label("FIRST");
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test writing a label is successful.
	m_partition.set_filesystem_label("SECOND");
	ASSERT_TRUE(s_ext2_obj->write_label(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_F(ext2Test, CreateAndWriteUUID)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(write_uuid);

	extra_setup();
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test writing a new random UUID is successful.
	ASSERT_TRUE(s_ext2_obj->write_uuid(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_F(ext2Test, CreateAndCheck)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(check);

	extra_setup();
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test checking the file system is successful.
	ASSERT_TRUE(s_ext2_obj->check_repair(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_F(ext2Test, CreateAndRemove)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(remove);

	extra_setup();
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test removing the file system is successful.  Note that most file systems don't
	// implement remove so will skip this test.
	ASSERT_TRUE(s_ext2_obj->remove(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_F(ext2Test, CreateAndGrow)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(grow);

	extra_setup(IMAGESIZE_Default);
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test growing the file system is successful.
	resize_image(IMAGESIZE_Larger);
	reload_partition();
	ASSERT_TRUE(s_ext2_obj->resize(m_partition, m_operation_detail, true)) << m_operation_detail;
}


TEST_F(ext2Test, CreateAndShrink)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(shrink);

	extra_setup(IMAGESIZE_Larger);
	ASSERT_TRUE(s_ext2_obj->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test shrinking the file system is successful.
	shrink_partition(IMAGESIZE_Default);
	ASSERT_TRUE(s_ext2_obj->resize(m_partition, m_operation_detail, false)) << m_operation_detail;
}


}  // namespace GParted


// Re-execute current executable using xvfb-run so that it provides a virtual X11 display.
void exec_using_xvfb_run(int argc, char** argv)
{
	// argc+2 = Space for "xvfb-run" command, existing argc strings plus NULL pointer.
	size_t size = sizeof(char*) * (argc+2);
	char** new_argv = (char**)malloc(size);
	if (new_argv == NULL)
	{
		fprintf(stderr, "Failed to allocate %lu bytes of memory.  errno=%d,%s\n",
			(unsigned long)size, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	new_argv[0] = strdup("xvfb-run");
	if (new_argv[0] == NULL)
	{
		fprintf(stderr, "Failed to allocate %lu bytes of memory.  errno=%d,%s\n",
		        (unsigned long)strlen(new_argv[0])+1, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Copy argv pointers including final NULL pointer.
	for (unsigned int i = 0; i <= (unsigned)argc; i++)
		new_argv[i+1] = argv[i];

	execvp(new_argv[0], new_argv);
	fprintf(stderr, "Failed to execute '%s %s ...'.  errno=%d,%s\n", new_argv[0], new_argv[1],
		errno, strerror(errno));
	exit(EXIT_FAILURE);
}


// Custom Google Test main().
// Reference:
// *   Google Test, Primer, Writing the main() function
//     https://github.com/google/googletest/blob/master/googletest/docs/primer.md#writing-the-main-function
int main(int argc, char** argv)
{
	printf("Running main() from %s\n", __FILE__);

	const char* display = getenv("DISPLAY");
	if (display == NULL)
	{
		printf("DISPLAY environment variable unset.  Executing 'xvfb-run %s ...'\n", argv[0]);
		exec_using_xvfb_run(argc, argv);
	}
	printf("DISPLAY=\"%s\"\n", display);

	testing::InitGoogleTest(&argc, argv);

	// Initialise threading in GParted to allow FileSystem interface classes to
	// successfully use Utils:: and Filesystem::execute_command().
	GParted::GParted_Core::mainthread = Glib::Thread::self();
	Gtk::Main gtk_main = Gtk::Main();

	return RUN_ALL_TESTS();
}
