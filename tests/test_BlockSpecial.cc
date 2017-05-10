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

/* Test BlockSpecial
 *
 * In order to make the tests runable by non-root users in a wide
 * variety of environments as possible the tests try to assume as small
 * a set of available files as possible and discovers used block device
 * names and symbolic link name.  The following files are explicitly
 * needed for the tests:
 *
 *     Name                        Access    Note
 *     -------------------------   -------   -----
 *     /                           Stat
 *     /proc/partitions            Stat
 *                                 Read      To find any two block
 *                                           devices
 *     /dev/BLOCK0                 Stat      First entry from
 *                                           /proc/partitions
 *     /dev/BLOCK1                 stat      Second entry from
 *                                           /proc/partitions
 *     /dev/disk/by-path/          Readdir   To find any symlink to a
 *                                           block device
 *     /dev/disk/by-path/SYMLINK   Stat      First directory entry
 *     /dev/RBLOCK                 Stat      Device to which SYMLINK
 *                                           refers
 *
 * Other dummy names are pre-loaded into the internal BlockSpecial cache
 * and never queried from the file system.
 */

#include "BlockSpecial.h"
#include "gtest/gtest.h"

#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <glibmm/ustring.h>

namespace GParted
{

// Print method for a BlockSpecial object
std::ostream& operator<<( std::ostream & out, const BlockSpecial & bs )
{
	out << "BlockSpecial{\"" << bs.m_name << "\"," << bs.m_major << "," << bs.m_minor << "}";
	return out;
}

// Helper to construct and return message for equality assertion used in:
//     EXPECT_BSEQTUP( bs, name, major, minor )
// Reference:
//     Google Test, AdvancedGuide, Using a Predicate-Formatter
::testing::AssertionResult CompareHelperBS2TUP( const char * bs_expr, const char * name_expr,
                                                const char * major_expr, const char * minor_expr,
                                                const BlockSpecial & bs, const Glib::ustring & name,
                                                unsigned long major, unsigned long minor )
{
	if ( bs.m_name == name && bs.m_major == major && bs.m_minor == minor )
		return ::testing::AssertionSuccess();
	else
		return ::testing::AssertionFailure()
		       << "      Expected: " << bs_expr << "\n"
		       << "      Which is: " << bs << "\n"
		       << "To be equal to: (" << name_expr << "," << major_expr << "," << major_expr << ")";
}

// Nonfatal assertion that BlockSpecial is equal to tuple (name, major, minor)
#define EXPECT_BSEQTUP(bs, name, major, minor)  \
	EXPECT_PRED_FORMAT4(CompareHelperBS2TUP, bs, name, major, minor)

// Helper to print two compared BlockSpecial objects on failure.
// Usage:
//     EXPECT_TRUE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
#define ON_FAILURE_WHERE(b1, b2)  \
	"   Where: " << #b1 << " = " << (b1) << "\n"  \
	"     And: " << #b2 << " = " << (b2);

// Return block device names numbered 0 upwards like "/dev/sda" by reading entries from
// /proc/partitions.
static std::string get_block_name( unsigned want )
{
	std::ifstream proc_partitions( "/proc/partitions" );
	if ( ! proc_partitions )
	{
		ADD_FAILURE() << __func__ << "(): Failed to open '/proc/partitions'";
		return "";
	}

	std::string line;
	char name[100] = "";
	unsigned entry = 0;
	while ( getline( proc_partitions, line ) )
	{
		if ( sscanf( line.c_str(), "%*u %*u %*u %99s", name ) == 1 )
		{
			if ( entry == want )
				break;
			entry ++;
		}
	}
	proc_partitions.close();

	if ( entry == want )
		return std::string( "/dev/" ) + name;

	ADD_FAILURE() << __func__ << "(): Entry " << want << " not found in '/proc/partitions'";
	return "";
}

// Return symbolic link to a block device by reading the first entry in the directory
// /dev/disk/by-path/.
static std::string get_link_name()
{
	DIR * dir = opendir( "/dev/disk/by-path" );
	if ( dir == NULL )
	{
		ADD_FAILURE() << __func__ << "(): Failed to open directory '/dev/disk/by-path'";
		return "";
	}

	bool found = false;
	struct dirent * dentry;
	// Silence GCC [-Wparentheses] warning with double parentheses
	while ( ( dentry = readdir( dir ) ) )
	{
		if ( strcmp( dentry->d_name, "." )  != 0 &&
		     strcmp( dentry->d_name, ".." ) != 0    )
		{
			found = true;
			break;
		}
	}
	closedir( dir );

	if ( found )
		return std::string( "/dev/disk/by-path/" ) + dentry->d_name;

	ADD_FAILURE() << __func__ << "(): No entries found in directory '/dev/disk/by-path'";
	return "";
}

// Follow symbolic link return real path.
static std::string follow_link_name( std::string link )
{
	char * rpath = realpath( link.c_str(), NULL );
	if ( rpath == NULL )
	{
		ADD_FAILURE() << __func__ << "(): Failed to resolve symbolic link '" << link << "'";
		return "";
	}

	std::string rpath_copy = rpath;
	free( rpath );
	return rpath_copy;
}

TEST( BlockSpecialTest, UnnamedBlockSpecialObject )
{
	// Test default constructor produces empty BlockSpecial object ("", 0, 0).
	BlockSpecial::clear_cache();
	BlockSpecial bs;
	EXPECT_BSEQTUP( bs, "", 0, 0 );
}

TEST( BlockSpecialTest, NamedBlockSpecialObjectPlainFile )
{
	// Test any named plain file or directory (actually any non-block special name)
	// produces BlockSpecial object (name, 0, 0).
	BlockSpecial::clear_cache();
	BlockSpecial bs( "/" );
	EXPECT_BSEQTUP( bs, "/", 0, 0 );
}

TEST( BlockSpecialTest, NamedBlockSpecialObjectPlainFileDuplicate )
{
	// Test second constructed BlockSpecial object with the same named plain file or
	// directory, where the major and minor numbers are retrieved via internal
	// BlockSpecial cache, produce an equivalent object (name, 0, 0).
	BlockSpecial::clear_cache();
	BlockSpecial bs1( "/" );
	BlockSpecial bs2( "/" );
	EXPECT_BSEQTUP( bs1, "/", 0, 0 );
	EXPECT_BSEQTUP( bs2, "/", 0, 0 );
}

TEST( BlockSpecialTest, NamedBlockSpecialObjectBlockDevice )
{
	std::string bname = get_block_name( 0 );

	// Test any block special name produces BlockSpecial object (name, major, minor).
	BlockSpecial::clear_cache();
	BlockSpecial bs( bname );
	EXPECT_STREQ( bs.m_name.c_str(), bname.c_str() );
	EXPECT_TRUE( bs.m_major > 0 || bs.m_minor > 0 );
}

TEST( BlockSpecialTest, NamedBlockSpecialObjectBlockDeviceDuplicate )
{
	std::string bname = get_block_name( 0 );

	// Test that a second object with the same name of a block device produces the
	// same (name, major, minor).  Checking internal BlockSpecial caching again.
	BlockSpecial::clear_cache();
	BlockSpecial bs1( bname );
	BlockSpecial bs2( bname );
	EXPECT_STREQ( bs1.m_name.c_str(), bs2.m_name.c_str() );
	EXPECT_EQ( bs1.m_major, bs2.m_major );
	EXPECT_EQ( bs1.m_minor, bs2.m_minor );
}

TEST( BlockSpecialTest, TwoNamedBlockSpecialObjectBlockDevices )
{
	std::string bname1 = get_block_name( 0 );
	std::string bname2 = get_block_name( 1 );

	// Test that two different named block devices produce different
	// (name, major, minor).
	BlockSpecial::clear_cache();
	BlockSpecial bs1( bname1 );
	BlockSpecial bs2( bname2 );
	EXPECT_STRNE( bs1.m_name.c_str(), bs2.m_name.c_str() );
	EXPECT_TRUE( bs1.m_major != bs2.m_major || bs1.m_minor != bs2.m_minor );
}

TEST( BlockSpecialTest, NamedBlockSpecialObjectBySymlinkMatches )
{
	std::string lname = get_link_name();
	std::string bname = follow_link_name( lname );

	// Test that a symbolic link to a block device and the named block device produce
	// BlockSpecial objects with different names but the same major, minor pair.
	BlockSpecial::clear_cache();
	BlockSpecial lnk( lname );
	BlockSpecial bs( bname );
	EXPECT_STRNE( lnk.m_name.c_str(), bs.m_name.c_str() );
	EXPECT_EQ( lnk.m_major, bs.m_major );
	EXPECT_EQ( lnk.m_minor, bs.m_minor );
}

TEST( BlockSpecialTest, PreRegisteredCacheUsedBeforeFileSystem )
{
	// Test that the cache is used before querying the file system by registering a
	// deliberately wrong major, minor pair for /dev/null and ensuring those are the
	// major, minor numbers reported for the device.
	BlockSpecial::clear_cache();
	BlockSpecial::register_block_special( "/dev/null", 4, 8 );
	BlockSpecial bs( "/dev/null" );
	EXPECT_BSEQTUP( bs, "/dev/null", 4, 8 );
}

TEST( BlockSpecialTest, OperatorEqualsTwoEmptyObjects )
{
	// Test equality of two empty BlockSpecial objects.
	BlockSpecial::clear_cache();
	BlockSpecial bs1;
	BlockSpecial bs2;
	EXPECT_TRUE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsSamePlainFiles )
{
	// Test equality of two named plain file or directory BlockSpecial objects;
	BlockSpecial::clear_cache();
	BlockSpecial bs1( "/" );
	BlockSpecial bs2( "/" );
	EXPECT_TRUE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsEmptyObjectAndPlainFile )
{
	// Test inequality of empty and plain file BlockSpecial objects.
	BlockSpecial::clear_cache();
	BlockSpecial bs1;
	BlockSpecial bs2( "/" );
	EXPECT_FALSE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsDifferentPlainFiles )
{
	// Test inequality of two different plain file or directory BlockSpecial objects;
	BlockSpecial::clear_cache();
	BlockSpecial bs1( "/" );
	BlockSpecial bs2( "/proc/partitions" );
	EXPECT_FALSE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsSameBlockDevices )
{
	std::string bname = get_block_name( 0 );

	// Test equality of two BlockSpecial objects using the same named block device.
	BlockSpecial::clear_cache();
	BlockSpecial bs1( bname );
	BlockSpecial bs2( bname );
	EXPECT_TRUE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsEmptyObjectAndBlockDevice )
{
	std::string bname = get_block_name( 0 );

	// Test inequality of empty and named block device BlockSpecial objects.
	BlockSpecial::clear_cache();
	BlockSpecial bs1;
	BlockSpecial bs2( bname );
	EXPECT_FALSE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsPlainFileAndBlockDevice )
{
	std::string bname = get_block_name( 0 );

	// Test inequality of plain file and block device BlockSpecial objects.
	BlockSpecial::clear_cache();
	BlockSpecial bs1( "/" );
	BlockSpecial bs2( bname );
	EXPECT_FALSE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsTwoDifferentBlockDevices )
{
	std::string bname1 = get_block_name( 0 );
	std::string bname2 = get_block_name( 1 );

	// Test inequality of two different named block device BlockSpecial objects.
	BlockSpecial::clear_cache();
	BlockSpecial bs1( bname1 );
	BlockSpecial bs2( bname2 );
	EXPECT_FALSE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorEqualsSameBlockDevicesWithMinorZero )
{
	// Test for fix in bug #771670.  Ensure that block devices with minor number 0 are
	// compared by major, minor pair rather than by name.
	BlockSpecial::clear_cache();
	BlockSpecial::register_block_special( "/dev/mapper/encrypted_swap", 254, 0 );
	BlockSpecial::register_block_special( "/dev/dm-0", 254, 0 );
	BlockSpecial bs1( "/dev/mapper/encrypted_swap" );
	BlockSpecial bs2( "/dev/dm-0" );
	EXPECT_TRUE( bs1 == bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanTwoEmptyObjects )
{
	// Test one empty BlockSpecial object is not ordered before another empty one.
	BlockSpecial::clear_cache();
	BlockSpecial bs1;
	BlockSpecial bs2;
	EXPECT_FALSE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanSamePlainFiles )
{
	// Test one plain file BlockSpecial object is not ordered before another of the
	// same name.
	BlockSpecial::clear_cache();
	BlockSpecial bs1( "/" );
	BlockSpecial bs2( "/" );
	EXPECT_FALSE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanDifferentPlainFiles )
{
	// Test one plain file BlockSpecial object with the lower name is ordered before
	// the other plain file with a higher name.
	BlockSpecial::clear_cache();
	BlockSpecial::register_block_special( "/dummy_file1", 0, 0 );
	BlockSpecial::register_block_special( "/dummy_file2", 0, 0 );
	BlockSpecial bs1( "/dummy_file1" );
	BlockSpecial bs2( "/dummy_file2" );
	EXPECT_TRUE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanEmptyObjectAndPlainFile )
{
	// Test empty BlockSpecial object with name "" is before a plain file of any name.
	BlockSpecial::clear_cache();
	BlockSpecial bs1;
	BlockSpecial bs2( "/" );
	EXPECT_TRUE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanSameBlockDevices )
{
	std::string bname = get_block_name( 0 );

	// Test one name block device BlockSpecial object is not ordered before another
	// from the same name.
	BlockSpecial::clear_cache();
	BlockSpecial bs1( bname );
	BlockSpecial bs2( bname );
	EXPECT_FALSE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanEmptyObjectAndBlockDevice )
{
	std::string bname = get_block_name( 0 );

	// Test empty BlockSpecial object with name "" is before any block device.
	BlockSpecial::clear_cache();
	BlockSpecial bs1;
	BlockSpecial bs2( bname );
	EXPECT_TRUE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanPlainFileAndBlockDevice )
{
	std::string bname = get_block_name( 0 );

	// Test one plain file BlockSpecial object is ordered before any block device.
	BlockSpecial::clear_cache();
	BlockSpecial bs1( "/" );
	BlockSpecial bs2( bname );
	EXPECT_TRUE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanTwoDifferentBlockDevicesMajorNumbers )
{
	// Test one block device BlockSpecial object is ordered before another based on
	// major numbers.
	BlockSpecial::clear_cache();
	BlockSpecial::register_block_special( "/dummy_block2", 1, 1 );
	BlockSpecial::register_block_special( "/dummy_block1", 2, 0 );
	BlockSpecial bs1( "/dummy_block2" );
	BlockSpecial bs2( "/dummy_block1" );
	EXPECT_TRUE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

TEST( BlockSpecialTest, OperatorLessThanTwoDifferentBlockDevicesMinorNumbers )
{
	// Test one block device BlockSpecial object is ordered before another based on
	// minor numbers.
	BlockSpecial::clear_cache();
	BlockSpecial::register_block_special( "/dummy_block2", 2, 0 );
	BlockSpecial::register_block_special( "/dummy_block1", 2, 1 );
	BlockSpecial bs1( "/dummy_block2" );
	BlockSpecial bs2( "/dummy_block1" );
	EXPECT_TRUE( bs1 < bs2 ) << ON_FAILURE_WHERE( bs1, bs2 );
}

}  // namespace GParted
