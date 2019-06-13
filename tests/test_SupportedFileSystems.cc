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

/* Test SupportedFileSystems
 *
 * Test the derived FileSystem interface classes which call the file system specific
 * executables via the SupportedFileSystems class.  Rather than mocking command execution
 * and returned output just run real commands, effectively making this integration testing.
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
#include "SupportedFileSystems.h"
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


// Printable file system type which meets the requirements for a Google Test name.
// Use GParted's file system names except when they contains any non-alphabetic chars.
// Reference:
// *   Specifying Names for Value-Parameterized Test Parameters
//     https://github.com/google/googletest/blob/v1.8.x/googletest/docs/advanced.md#specifying-names-for-value-parameterized-test-parameters
//         "NOTE: test names must be non-empty, unique, and may only contain ASCII
//         alphanumeric characters.  In particular, they should not contain underscores."
const std::string test_fsname(FSType fstype)
{
	switch (fstype)
	{
		case FS_HFSPLUS:     return "hfsplus";
		case FS_LINUX_SWAP:  return "linuxswap";
		case FS_LVM2_PV:     return "lvm2pv";
		default:             break;
	}
	return std::string(Utils::get_filesystem_string(fstype));
}


// Function callable by INSTANTIATE_TEST_CASE_P() to get the file system type for printing
// as part of the test name.
const std::string param_fsname(const ::testing::TestParamInfo<FSType>& info)
{
	return test_fsname(info.param);
}


// Google Test 1.8.1 (and earlier) doesn't implement run-time test skipping so implement
// our own for GParted run-time detected of unsupported file system features.
// Ref:
//     Skipping tests at runtime with GTEST_SKIP() #1544
//     https://github.com/google/googletest/pull/1544
//     (Merged after Google Test 1.8.1)
#define SKIP_IF_FS_DOESNT_SUPPORT(opt)                                                    \
	if (s_supported_filesystems->get_fs_support(m_fstype).opt != FS::EXTERNAL)        \
	{                                                                                 \
		std::cout << __FILE__ << ":" << __LINE__ << ": Skip test.  "              \
		          << #opt << " not supported or support not found" << std::endl;  \
		return;                                                                   \
	}


#define SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(fstype)                                    \
	if (m_fstype == fstype)                                                                 \
	{                                                                                       \
		if (getuid() != 0)                                                              \
		{                                                                               \
			std::cout << __FILE__ << ":" << __LINE__ << ": Skip test.  Not root "   \
			          << "to be able to create required loop device" << std::endl;  \
			return;                                                                 \
		}                                                                               \
		m_require_loopdev = true;                                                       \
	}


#define SKIP_IF_TEST_DISABLED_FOR_FS(fstype)                                  \
	if (m_fstype == fstype)                                               \
	{                                                                     \
		std::cout << __FILE__ << ":" << __LINE__ << ": Skip test.  "  \
		          << "Test disabled for file system" << std::endl;    \
		return;                                                       \
	}


const Byte_Value IMAGESIZE_Default = 256*MEBIBYTE;
const Byte_Value IMAGESIZE_Larger  = 512*MEBIBYTE;


class SupportedFileSystemsTest : public ::testing::TestWithParam<FSType>
{
protected:
	SupportedFileSystemsTest();

	virtual void SetUp();
	virtual void extra_setup(Byte_Value size = IMAGESIZE_Default);
	virtual void TearDown();

public:
	static void SetUpTestCase();
	static void TearDownTestCase();

	static std::vector<FSType> get_supported_fstypes();

protected:
	static void setup_supported_filesystems();
	static void teardown_supported_filesystems();

	virtual const std::string create_loopdev(const std::string& image_name) const;
	virtual void detach_loopdev(const std::string& loopdev_name) const;
	virtual void reload_loopdev_size(const std::string& loopdev_name) const;

	virtual void reload_partition();
	virtual void resize_image(Byte_Value new_size);
	virtual void shrink_partition(Byte_Value size);

	static SupportedFileSystems* s_supported_filesystems;  // Owning pointer
	static const char*           s_image_name;

	FSType          m_fstype;
	FileSystem*     m_fs_object;  // Alias pointer
	bool            m_require_loopdev;
	std::string     m_dev_name;
	Partition       m_partition;
	OperationDetail m_operation_detail;
};


SupportedFileSystems* SupportedFileSystemsTest::s_supported_filesystems = NULL;
const char*           SupportedFileSystemsTest::s_image_name            = "test_SupportedFileSystems.img";


SupportedFileSystemsTest::SupportedFileSystemsTest()
 : m_fstype(GetParam()), m_fs_object(NULL), m_require_loopdev(false),
   // Initialise top-level operation detail object with description ...
   m_operation_detail("Operation details:", STATUS_NONE)
{
}


void SupportedFileSystemsTest::SetUp()
{
	ASSERT_TRUE(s_supported_filesystems != NULL) << __func__ << "(): TEST_BUG: File system interfaces not loaded";

	// Lookup file system interface object.
	m_fs_object = s_supported_filesystems->get_fs_object(m_fstype);
	ASSERT_TRUE(m_fs_object != NULL) << __func__ << "(): TEST_BUG: Interface object not found for file system "
	                                 << test_fsname(m_fstype);
}


void SupportedFileSystemsTest::extra_setup(Byte_Value size)
{
	// Create new image file to work with.
	unlink(s_image_name);
	int fd = open(s_image_name, O_WRONLY|O_CREAT|O_NONBLOCK, 0666);
	ASSERT_GE(fd, 0) << "Failed to create image file '" << s_image_name << "'.  errno="
	                 << errno << "," << strerror(errno);
	ASSERT_EQ(ftruncate(fd, (off_t)size), 0) << "Failed to set image file '" << s_image_name << "' to size "
	                                         << size << ".  errno=" << errno << "," << strerror(errno);
	close(fd);

	m_dev_name = s_image_name;
	if (m_require_loopdev)
		m_dev_name = create_loopdev(s_image_name);

	reload_partition();
}


void SupportedFileSystemsTest::TearDown()
{
	if (m_require_loopdev)
		detach_loopdev(m_dev_name);

	unlink(s_image_name);

	m_fs_object = NULL;
}


// Common test case initialisation creating the supported file system interface object.
void SupportedFileSystemsTest::SetUpTestCase()
{
	setup_supported_filesystems();
}


// Common test case teardown destroying the supported file systems interface object.
void SupportedFileSystemsTest::TearDownTestCase()
{
	teardown_supported_filesystems();
}


std::vector<FSType> SupportedFileSystemsTest::get_supported_fstypes()
{
	setup_supported_filesystems();

	std::vector<FSType> v;
	const std::vector<FS>& fss = s_supported_filesystems->get_all_fs_support();
	for (unsigned int i = 0; i < fss.size(); i++)
	{
		if (s_supported_filesystems->supported_filesystem(fss[i].fstype))
			v.push_back(fss[i].fstype);
	}
	return v;
}


// Create the supported file system interface object.
void SupportedFileSystemsTest::setup_supported_filesystems()
{
	if (s_supported_filesystems == NULL)
	{
		s_supported_filesystems = new SupportedFileSystems();

		// Discover available file systems support capabilities, base on available
		// file system specific tools.
		s_supported_filesystems->find_supported_filesystems();
	}
}


// Destroy the supported file systems interface object.
void SupportedFileSystemsTest::teardown_supported_filesystems()
{
	delete s_supported_filesystems;
	s_supported_filesystems = NULL;
}


// Create loop device over named image file and return the device name.
const std::string SupportedFileSystemsTest::create_loopdev(const std::string& image_name) const
{
	Glib::ustring output;
	Glib::ustring error;
	Glib::ustring cmd = "losetup --find --show " + Glib::shell_quote(image_name);
	int exit_status = Utils::execute_command(cmd, output, error, true);
	if (exit_status != 0)
	{
		ADD_FAILURE() << __func__ << "(): Execute: " << cmd << "\n"
		              << error
		              << __func__ << "(): Losetup failed with exit status " << exit_status << "\n"
		              << __func__ << "(): Failed to create required loop device";
		return "";
	}

	// Strip trailing New Line.
	size_t len = output.length();
	if (len > 0 && output[len-1] == '\n')
		output.resize(len-1);

	return output;
}


// Detach named loop device.
void SupportedFileSystemsTest::detach_loopdev(const std::string& loopdev_name) const
{
	Glib::ustring output;
	Glib::ustring error;
	Glib::ustring cmd = "losetup --detach " + Glib::shell_quote(loopdev_name);
	int exit_status = Utils::execute_command(cmd, output, error, true);
	if (exit_status != 0)
	{
		// Report losetup detach error but don't fail the test because of it.
		std::cout << __func__ << "(): Execute: " << cmd << "\n"
		          << error
		          << __func__ << "(): Losetup failed with exit status " << exit_status << "\n"
		          << __func__ << "(): Failed to detach loop device.  Test NOT affected" << std::endl;
	}
}


// Instruct loop device to reload the size of the underlying image file.
void SupportedFileSystemsTest::reload_loopdev_size(const std::string& loopdev_name) const
{
	Glib::ustring output;
	Glib::ustring error;
	Glib::ustring cmd = "losetup --set-capacity " + Glib::shell_quote(loopdev_name);
	int exit_status = Utils::execute_command(cmd, output, error, true);
	if (exit_status != 0)
	{
		ADD_FAILURE() << __func__ << "(): Execute: " << cmd << "\n"
		              << error
		              << __func__ << "(): Losetup failed with exit status " << exit_status << "\n"
		              << __func__ << "(): Failed to reload loop device size";
	}
}


// (Re)initialise m_partition as a Partition object spanning the whole of the image file
// with file system type only.  No file system usage, label or UUID.
void SupportedFileSystemsTest::reload_partition()
{
	m_partition.Reset();

	// Use libparted to get the sector size etc. of the image file.
	PedDevice* lp_device = ped_device_get(m_dev_name.c_str());
	ASSERT_TRUE(lp_device != NULL);

	// Prepare partition object spanning whole of the image file.
	m_partition.set_unpartitioned(m_dev_name,
	                              lp_device->path,
	                              m_fstype,
	                              lp_device->length,
	                              lp_device->sector_size,
	                              false);

	ped_device_destroy(lp_device);
	lp_device = NULL;
}


void SupportedFileSystemsTest::resize_image(Byte_Value new_size)
{
	int fd = open(s_image_name, O_WRONLY|O_NONBLOCK);
	ASSERT_GE(fd, 0) << "Failed to open image file '" << s_image_name << "'.  errno="
	                 << errno << "," << strerror(errno);
	ASSERT_EQ(ftruncate(fd, (off_t)new_size), 0) << "Failed to resize image file '" << s_image_name << "' to size "
	                                             << new_size << ".  errno=" << errno << "," << strerror(errno);
	close(fd);

	if (m_require_loopdev)
		reload_loopdev_size(m_dev_name);
}


void SupportedFileSystemsTest::shrink_partition(Byte_Value new_size)
{
	ASSERT_LE(new_size, m_partition.get_byte_length()) << __func__ << "(): TEST_BUG: Cannot grow Partition object size";
	Sector new_sectors = (new_size + m_partition.sector_size - 1) / m_partition.sector_size;
	m_partition.sector_end = new_sectors;
}


TEST_P(SupportedFileSystemsTest, Create)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);

	extra_setup();
	// Call create, check for success and print operation details on failure.
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_P(SupportedFileSystemsTest, CreateAndReadUsage)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(read);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_BTRFS);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_NILFS2);

	extra_setup();
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	reload_partition();
	m_fs_object->set_used_sectors(m_partition);
	// Test file system usage is reported correctly.
	// Used is between 0 and length.
	EXPECT_LE(0, m_partition.get_sectors_used());
	EXPECT_LE(m_partition.get_sectors_used(), m_partition.get_sector_length());
	// Unused is between 0 and length.
	EXPECT_LE(0, m_partition.get_sectors_unused());
	EXPECT_LE(m_partition.get_sectors_unused(), m_partition.get_sector_length());
	// Unallocated is 0.
	EXPECT_EQ(m_partition.get_sectors_unallocated(), 0);
	// Used + unused = length.
	EXPECT_EQ(m_partition.get_sectors_used() + m_partition.get_sectors_unused(), m_partition.get_sector_length());

	// Test messages from read operation are empty or print them.
	EXPECT_TRUE(m_partition.get_messages().empty()) << m_partition;
}


TEST_P(SupportedFileSystemsTest, CreateAndReadLabel)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(read_label);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_BTRFS);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_NILFS2);

	const char* fs_label = "TEST_LABEL";
	extra_setup();
	m_partition.set_filesystem_label(fs_label);
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test reading the label is successful.
	reload_partition();
	m_fs_object->read_label(m_partition);
	EXPECT_STREQ(fs_label, m_partition.get_filesystem_label().c_str());

	// Test messages from read operation are empty or print them.
	EXPECT_TRUE(m_partition.get_messages().empty()) << m_partition;
}


TEST_P(SupportedFileSystemsTest, CreateAndReadUUID)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(read_uuid);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_BTRFS);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_NILFS2);

	extra_setup();
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	if (m_fstype == FS_JFS)
	{
		// Write a new UUID to cause the jfs version to be updated from 1 to 2 so
		// that jfs_tune can successfully report the UUID of the file system.
		SKIP_IF_FS_DOESNT_SUPPORT(write_uuid);
		ASSERT_TRUE(m_fs_object->write_uuid(m_partition, m_operation_detail)) << m_operation_detail;
	}

	// Test reading the UUID is successful.
	reload_partition();
	m_fs_object->read_uuid(m_partition);
	EXPECT_GE(m_partition.uuid.size(), 9U);

	// Test messages from read operation are empty or print them.
	EXPECT_TRUE(m_partition.get_messages().empty()) << m_partition;
}


TEST_P(SupportedFileSystemsTest, CreateAndWriteLabel)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(write_label);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_NILFS2);

	extra_setup();
	m_partition.set_filesystem_label("FIRST");
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test writing a label is successful.
	m_partition.set_filesystem_label("SECOND");
	ASSERT_TRUE(m_fs_object->write_label(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_P(SupportedFileSystemsTest, CreateAndWriteUUID)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(write_uuid);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_NILFS2);

	extra_setup();
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test writing a new random UUID is successful.
	ASSERT_TRUE(m_fs_object->write_uuid(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_P(SupportedFileSystemsTest, CreateAndCheck)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(check);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_TEST_DISABLED_FOR_FS(FS_MINIX);  // FIXME: Enable when util-linux >= 2.27 is available everywhere

	extra_setup();
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test checking the file system is successful.
	ASSERT_TRUE(m_fs_object->check_repair(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_P(SupportedFileSystemsTest, CreateAndRemove)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(remove);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);

	extra_setup();
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test removing the file system is successful.  Note that most file systems don't
	// implement remove so will skip this test.
	ASSERT_TRUE(m_fs_object->remove(m_partition, m_operation_detail)) << m_operation_detail;
}


TEST_P(SupportedFileSystemsTest, CreateAndGrow)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(grow);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_BTRFS);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_JFS);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_NILFS2);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_XFS);

	extra_setup(IMAGESIZE_Default);
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test growing the file system is successful.
	resize_image(IMAGESIZE_Larger);
	reload_partition();
	ASSERT_TRUE(m_fs_object->resize(m_partition, m_operation_detail, true)) << m_operation_detail;
}


TEST_P(SupportedFileSystemsTest, CreateAndShrink)
{
	SKIP_IF_FS_DOESNT_SUPPORT(create);
	SKIP_IF_FS_DOESNT_SUPPORT(shrink);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_BTRFS);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_LVM2_PV);
	SKIP_IF_NOT_ROOT_FOR_REQUIRED_LOOPDEV_FOR_FS(FS_NILFS2);

	extra_setup(IMAGESIZE_Larger);
	ASSERT_TRUE(m_fs_object->create(m_partition, m_operation_detail)) << m_operation_detail;

	// Test shrinking the file system is successful.
	shrink_partition(IMAGESIZE_Default);
	ASSERT_TRUE(m_fs_object->resize(m_partition, m_operation_detail, false)) << m_operation_detail;
}


// Instantiate the test case so every test is run for the specified file system types.
// Reference:
// *   Google Test, Advanced googletest Topics, How to Write Value-Parameterized Tests
//     https://github.com/google/googletest/blob/v1.8.x/googletest/docs/advanced.md#how-to-write-value-parameterized-tests
INSTANTIATE_TEST_CASE_P(My,
                        SupportedFileSystemsTest,
                        ::testing::ValuesIn(SupportedFileSystemsTest::get_supported_fstypes()),
                        param_fsname);


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

	// Initialise threading in GParted to allow FileSystem interface classes to
	// successfully use Utils:: and Filesystem::execute_command().  Must be before
	// InitGoogleTest().
	GParted::GParted_Core::mainthread = Glib::Thread::self();
	Gtk::Main gtk_main = Gtk::Main();

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
