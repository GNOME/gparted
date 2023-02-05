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


#include "common.h"
#include "insertion_operators.h"
#include "GParted_Core.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"
#include "gtest/gtest.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <gtkmm.h>
#include <parted/parted.h>
#include <glibmm/thread.h>


namespace GParted
{


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


// Custom Google Test main().
// Reference:
// *   Google Test, Primer, Writing the main() function
//     https://github.com/google/googletest/blob/master/googletest/docs/primer.md#writing-the-main-function
int main(int argc, char** argv)
{
	printf("Running main() from %s\n", __FILE__);
	GParted::ensure_x11_display(argc, argv);

	// Initialise threading in GParted to successfully use Utils:: and
	// FileSystem::execute_command().  Must be before InitGoogleTest().
	GParted::GParted_Core::mainthread = Glib::Thread::self();
	Gtk::Main gtk_main = Gtk::Main();

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
