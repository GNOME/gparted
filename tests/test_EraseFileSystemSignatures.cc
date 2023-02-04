/* Copyright (C) 2023 Mike Fleetwood
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

/* Test GParted_Core::erase_filesystem_signatures()
 */


#include "GParted_Core.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"
#include "gtest/gtest.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <stddef.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <gtkmm.h>
#include <parted/parted.h>
#include <glibmm/ustring.h>
#include <glibmm/thread.h>


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

	for (size_t i = 0; i < od.get_childs().size(); i++)
	{
		out << *od.get_childs()[i];
	}
	return out;
}


// Explicit test fixture class for common variables and methods used in each test.
// Reference:
//     Google Test, Primer, Test Fixtures: Using the Same Data Configuration for Multiple Tests
class EraseFileSystemSignaturesTest : public ::testing::Test
{
protected:
	virtual void create_image_file(Byte_Value size);
	virtual void write_intel_software_raid_signature();
	virtual bool image_contains_all_zeros();
	virtual void TearDown();

	bool erase_filesystem_signatures(const Partition& partition, OperationDetail& operationdetail)
		{ return m_gparted_core.erase_filesystem_signatures(partition, operationdetail); };

	static const char* s_image_name;

	GParted_Core    m_gparted_core;
	Partition       m_partition;
	OperationDetail m_operation_detail;
};


const char* EraseFileSystemSignaturesTest::s_image_name = "test_EraseFileSystemSignatures.img";


void EraseFileSystemSignaturesTest::create_image_file(Byte_Value size)
{
	// Create new image file to work with.
	unlink(s_image_name);
	int fd = open(s_image_name, O_WRONLY|O_CREAT|O_NONBLOCK, 0666);
	ASSERT_GE(fd, 0) << "Failed to create image file '" << s_image_name << "'.  errno="
	                 << errno << "," << strerror(errno);
	ASSERT_EQ(ftruncate(fd, (off_t)size), 0) << "Failed to set image file '" << s_image_name << "' to size "
	                                         << size << ".  errno=" << errno << "," << strerror(errno);
	close(fd);

	// Initialise m_partition as a Partition object spanning the whole of the image file.
	m_partition.Reset();

	PedDevice* lp_device = ped_device_get(s_image_name);
	ASSERT_TRUE(lp_device != NULL);

	m_partition.set_unpartitioned(s_image_name,
	                              lp_device->path,
	                              FS_UNALLOCATED,
	                              lp_device->length,
	                              lp_device->sector_size,
	                              false);

	ped_device_destroy(lp_device);
	lp_device = NULL;
}


void EraseFileSystemSignaturesTest::write_intel_software_raid_signature()
{
	int fd = open(s_image_name, O_WRONLY|O_NONBLOCK);
	ASSERT_GE(fd, 0) << "Failed to open image file '" << s_image_name << "'.  errno="
	                 << errno << "," << strerror(errno);
	const char* signature = "Intel Raid ISM Cfg Sig. ";
	size_t len_signature = strlen(signature);

	// Write Intel Software RAID signature at -2 sectors before the end.  Hard codes
	// sector size to 512 bytes for a file.
	// Reference:
	//     .../util-linux/libblkid/src/superblocks/isw_raid.c:probe_iswraid().
	ASSERT_GE(lseek(fd, 2 * -512, SEEK_END), 0) << "Failed to seek in image file '" << s_image_name
	                                            << "'.  errno=" << errno << "," << strerror(errno);
	ASSERT_EQ(write(fd, signature, len_signature), (ssize_t)len_signature)
	        << "Failed to write to image file '" << s_image_name << "'.  errno="
	        << errno << "," << strerror(errno);
	close(fd);
}


const char* first_non_zero_byte(const char* buf, size_t size)
{
	while (size > 0)
	{
		if (*buf != '\0')
			return buf;
		buf++;
		size--;
	}
	return NULL;
}


// Number of bytes of binary data to report.
const size_t BinaryStringChunkSize = 16;


// Format up to BinaryStringChunkSize (16) bytes of binary data ready for printing as:
//      Hex offset     ASCII text          Hex bytes
//     "0x000000000  \"ABCDEFGHabcdefgh\"  41 42 43 44 45 46 47 48 61 62 63 64 65 66 67 68"
std::string binary_string_to_print(size_t offset, const char* s, size_t len)
{
	std::ostringstream result;

	result << "0x";
	result.fill('0');
	result << std::setw(8) << std::hex << std::uppercase << offset << "  \"";

	size_t i;
	for (i = 0; i < BinaryStringChunkSize && i < len; i++)
		result.put((isprint(s[i])) ? s[i] : '.');
	result.put('\"');

	if (len > 0)
	{
		for (; i < BinaryStringChunkSize; i++)
			result.put(' ');
		result.put(' ');

		for (i = 0 ; i < BinaryStringChunkSize && i < len; i++)
			result << " "
			       << std::setw(2) << std::hex << std::uppercase
			       << (unsigned int)(unsigned char)s[i];
	}

	return result.str();
}


bool EraseFileSystemSignaturesTest::image_contains_all_zeros()
{
	int fd = open(s_image_name, O_RDONLY|O_NONBLOCK);
	if (fd < 0)
	{
		ADD_FAILURE() << __func__ << "(): Failed to open image file '" << s_image_name << "'.  errno="
		              << errno << "," << strerror(errno);
		return false;
	}

	ssize_t bytes_read = 0;
	size_t offset = 0;
	do
	{
		char buf[BUFSIZ];
		bytes_read = read(fd, buf, sizeof(buf));
		if (bytes_read < 0)
		{
			ADD_FAILURE() << __func__ << "(): Failed to read from image file '" << s_image_name
			              << "'.  errno=" << errno << "," << strerror(errno);
			close(fd);
			return false;
		}
		const char* p = first_non_zero_byte(buf, bytes_read);
		if (p != NULL)
		{
			ADD_FAILURE() << __func__ << "(): First non-zero bytes:\n"
			              << binary_string_to_print(offset + (p - buf), p, buf + bytes_read - p);
			close(fd);
			return false;
		}
	}
	while (bytes_read > 0);
	close(fd);
	return true;
}


void EraseFileSystemSignaturesTest::TearDown()
{
	unlink(s_image_name);
}


TEST_F(EraseFileSystemSignaturesTest, IntelSoftwareRAIDAligned)
{
	create_image_file(16 * MEBIBYTE);
	write_intel_software_raid_signature();

	EXPECT_TRUE(erase_filesystem_signatures(m_partition, m_operation_detail)) << m_operation_detail;
	EXPECT_TRUE(image_contains_all_zeros());
}


TEST_F(EraseFileSystemSignaturesTest, IntelSoftwareRAIDUnaligned)
{
	create_image_file(16 * MEBIBYTE - 512);
	write_intel_software_raid_signature();

	EXPECT_TRUE(erase_filesystem_signatures(m_partition, m_operation_detail)) << m_operation_detail;
	EXPECT_TRUE(image_contains_all_zeros());
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

	// Initialise threading in GParted to successfully use Utils:: and
	// FileSystem::execute_command().  Must be before InitGoogleTest().
	GParted::GParted_Core::mainthread = Glib::Thread::self();
	Gtk::Main gtk_main = Gtk::Main();

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
