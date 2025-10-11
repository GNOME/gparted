/* Copyright (C) 2009,2010 Luca Bruno <lucab@debian.org>
 * Copyright (C) 2010, 2011 Curtis Gedak
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

#include "btrfs.h"
#include "BlockSpecial.h"
#include "FileSystem.h"
#include "Mount_Info.h"
#include "Partition.h"
#include "Utils.h"

#include <ctype.h>
#include <glibmm/ustring.h>
#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


// Cache of required btrfs file system device information by device
// E.g. For a single device btrfs on /dev/sda2 and a three device btrfs
//      on /dev/sd[bcd]1 the cache would be as follows.  (Note that
//      BS(str) is short hand for constructor BlockSpecial(str)).
//  btrfs_device_cache[BS("/dev/sda2")] = {devid=1, members=[BS("/dev/sda2")]}
//  btrfs_device_cache[BS("/dev/sdb1")] = {devid=1, members=[BS("/dev/sdd1"), BS("/dev/sdc1"), BS("/dev/sdb1")]}
//  btrfs_device_cache[BS("/dev/sdc1")] = {devid=2, members=[BS("/dev/sdd1"), BS("/dev/sdc1"), BS("/dev/sdb1")]}
//  btrfs_device_cache[BS("/dev/sdd1")] = {devid=3, members=[BS("/dev/sdd1"), BS("/dev/sdc1"), BS("/dev/sdb1")]}
std::map<BlockSpecial, BTRFS_Device> btrfs_device_cache;
bool btrfs_found = false;

FS btrfs::get_filesystem_support()
{
	FS fs( FS_BTRFS );

	// Always set an initial fallback so at a minimum GParted_Core::is_busy() will use
	// the built-in file system mounted detection method when the btrfs command is not
	// found.  This can only determine if the mounting device is mounted or not, not
	// any of the other members of a multi-device btrfs file system.
	fs.busy = FS::GPARTED;

	if ( ! Glib::find_program_in_path( "mkfs.btrfs" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	btrfs_found = false;
	if (! Glib::find_program_in_path("btrfs").empty())
	{
		btrfs_found = true;

		// Use these btrfs multi-tool sub-commands without further checking for
		// their availability:
		//     btrfs check
		//     btrfs filesystem label
		//     btrfs filesystem resize
		//     btrfs filesystem show
		//     btrfs inspect-internal dump-super
		// as they are all available in btrfs-progs >= 4.5.

		// Perform full busy detection which also handles all members of a
		// multi-device btrfs file file system.
		fs.busy = FS::EXTERNAL;

		fs.read               = FS::EXTERNAL;
		fs.online_read        = FS::EXTERNAL;
		fs.read_label         = FS::EXTERNAL;
		fs.read_uuid          = FS::EXTERNAL;
		fs.check              = FS::EXTERNAL;
		fs.write_label        = FS::EXTERNAL;
		fs.online_write_label = FS::EXTERNAL;

		//Resizing of btrfs requires mount, umount and kernel
		//  support as well as btrfs filesystem resize
		if (    ! Glib::find_program_in_path( "mount" ) .empty()
		     && ! Glib::find_program_in_path( "umount" ) .empty()
		     && fs .check
		     && Utils::kernel_supports_fs( "btrfs" )
		   )
		{
			fs .grow = FS::EXTERNAL ;
			if ( fs .read ) //needed to determine a minimum file system size.
				fs .shrink = FS::EXTERNAL ;
		}
	}

	if ( ! Glib::find_program_in_path( "btrfstune" ).empty() )
	{
		Glib::ustring output;
		Glib::ustring error;
		Utils::execute_command( "btrfstune --help", output, error, true );
		if ( Utils::regexp_label( output + error, "^[[:blank:]]*(-u)[[:blank:]]" ) == "-u" )
			fs.write_uuid = FS::EXTERNAL;
	}

	if ( fs .check )
	{
		fs.copy = FS::GPARTED;
		fs.move = FS::GPARTED;
	}

	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
	{
		fs .online_grow = fs .grow ;
		fs .online_shrink = fs .shrink ;
	}

	m_fs_limits.min_size = 256 * MEBIBYTE;

	return fs ;
}


bool btrfs::is_busy( const Glib::ustring & path )
{
	// A btrfs file system is busy if any of the member devices are mounted.
	return ! get_mount_device( path ) .empty() ;
}


bool btrfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mkfs.btrfs -L " +
	                        Glib::shell_quote(new_partition.get_filesystem_label()) +
	                        " " + Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


bool btrfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("btrfs check " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


void btrfs::set_used_sectors(Partition& partition)
{
	// Called when the file system is unmounted *and* when mounted.
	//
	// Btrfs has a volume manager layer within the file system which allows it to
	// provide multiple levels of data redundancy, RAID levels, and use multiple
	// devices both of which can be changed while the file system is mounted.  To
	// achieve this btrfs has to allocate space at two different levels: (1) chunks of
	// 256 MiB or more at the volume manager level; and (2) extents at the metadata
	// and file data level.
	// References:
	// *   Btrfs: Working with multiple devices
	//     https://lwn.net/Articles/577961/
	// *   Btrfs wiki: Glossary
	//     https://btrfs.wiki.kernel.org/index.php/Glossary
	//
	// This makes the question of how much disk space is being used in an individual
	// device a complicated question to answer.  Additionally, even if there is a
	// correct answer for the usage / minimum size a device can be, a multi-device
	// btrfs can and does relocate extents to other devices allowing it to be shrunk
	// smaller than it's minimum size (redundancy requirements of chunks permitting).
	//
	// Btrfs inspect-internal dump-super provides chunk allocation information for the
	// current device only and a single file system wide extent level usage figure.
	// Calculate the per device used figure as the fraction of file system wide extent
	// usage apportioned per device.
	//
	// Example:
	//     # btrfs filesystem show --raw /dev/sdb1
	//     Label: none  uuid: 68195e7e-c13f-4095-945f-675af4b1a451
	//             Total devices 2 FS bytes used 178749440
	//             devid    1 size 2147483648 used 239861760 path /dev/sdb1
	//             devid    2 size 2147483648 used 436207616 path /dev/sdc1
	//
	//     # btrfs inspect-internal dump-super /dev/sdb1 | egrep 'total_bytes|bytes_used|sectorsize|devid'
	//     total_bytes             4294967296
	//     bytes_used              178749440
	//     sectorsize              4096
	//     dev_item.total_bytes    2147483648
	//     dev_item.bytes_used     239861760
	//     dev_item.devid          1
	//
	// Calculation:
	//     ptn_fs_size = dev_item_total_bytes
	//     ptn_fs_used = bytes_used * dev_item_total_bytes / total_bytes
	//
	// This calculation also ignores that btrfs allocates chunks at the volume manager
	// level.  So when fully compacted there will be partially filled chunks for
	// metadata and data for each storage profile (RAID level) not accounted for.
	Glib::ustring output;
	Glib::ustring error;
	Utils::execute_command("btrfs inspect-internal dump-super " + Glib::shell_quote(partition.get_path()),
		               output, error, true);
	// btrfs inspect-internal dump-super returns zero exit status for both success and
	// failure.  Instead use non-empty stderr to identify failure.
	if (! error.empty())
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// Btrfs file system wide size (sum of devid sizes)
	long long total_bytes = -1;
	Glib::ustring::size_type index = output.find("\ntotal_bytes");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\ntotal_bytes %lld", &total_bytes);

	// Btrfs file system wide used bytes
	long long bytes_used = -1;
	index = output.find("\nbytes_used");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nbytes_used %lld", &bytes_used);

	// Sector size
	long long sector_size = -1;
	index = output.find("\nsectorsize");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\nsectorsize %lld", &sector_size);

	// Btrfs this device size
	long long dev_item_total_bytes = -1;
	index = output.find("\ndev_item.total_bytes");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "\ndev_item.total_bytes %lld", &dev_item_total_bytes);

	if (total_bytes > -1 && bytes_used > -1 && dev_item_total_bytes > -1 && sector_size > -1)
	{
		Sector ptn_fs_size = dev_item_total_bytes / partition.sector_size;
		double devid_size_fraction = dev_item_total_bytes / double(total_bytes);
		Sector ptn_fs_used = Utils::round(bytes_used * devid_size_fraction) / partition.sector_size;
		Sector ptn_fs_free = ptn_fs_size - ptn_fs_used;
		partition.set_sector_usage(ptn_fs_size, ptn_fs_free);
		partition.fs_block_size = sector_size;
	}
}


bool btrfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	// Use the mount point when labelling a mounted btrfs, or block device containing
	// the unmounted btrfs.
	//     btrfs filesystem label '/dev/PTN' 'NEWLABEL'
	//     btrfs filesystem label '/MNTPNT' 'NEWLABEL'
	Glib::ustring path;
	if (partition.busy)
		path = partition.get_mountpoint();
	else
		path = partition.get_path();

	return ! operationdetail.execute_command("btrfs filesystem label " + Glib::shell_quote(path) +
	                        " " + Glib::shell_quote(partition.get_filesystem_label()),
	                        EXEC_CHECK_STATUS);
}


bool btrfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;
	int exit_status = 0;
	const Glib::ustring& path = partition_new.get_path();
	const BTRFS_Device& btrfs_dev = get_cache_entry(path);

	if ( btrfs_dev .devid == -1 )
	{
		operationdetail .add_child( OperationDetail(
				Glib::ustring::compose( _("Failed to find devid for path %1"), path ), STATUS_ERROR ) ) ;
		return false ;
	}
	Glib::ustring devid_str = Utils::num_to_str( btrfs_dev .devid ) ;

	Glib::ustring mount_point ;
	if ( ! partition_new .busy )
	{
		mount_point = mk_temp_dir( "", operationdetail ) ;
		if ( mount_point .empty() )
			return false ;
		exit_status = operationdetail.execute_command("mount -v -t btrfs " + Glib::shell_quote(path) +
		                        " " + Glib::shell_quote(mount_point),
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;
	}
	else
	{
		mount_point = Utils::first_directory(partition_new.get_mountpoints());
		if (mount_point.empty())
		{
			Glib::ustring mount_list = Glib::build_path(", ", partition_new.get_mountpoints());
			operationdetail.add_child(OperationDetail(
			                Glib::ustring::compose(_("No directory mount point found in %1"), mount_list),
			                STATUS_ERROR));
			return false;
		}
	}

	if ( success )
	{
		Glib::ustring size ;
		if ( ! fill_partition )
			size = Utils::num_to_str(partition_new.get_byte_length() / KIBIBYTE) + "K";
		else
			size = "max" ;
		exit_status = operationdetail.execute_command("btrfs filesystem resize " + devid_str + ":" + size +
		                        " " + Glib::shell_quote(mount_point),
		                        EXEC_CHECK_STATUS);
		if (exit_status != 0)
			success = false;

		if (! partition_new.busy)
		{
			exit_status = operationdetail.execute_command("umount -v " + Glib::shell_quote(mount_point),
			                        EXEC_CHECK_STATUS);
			if (exit_status != 0)
				success = false;
		}
	}

	if ( ! partition_new .busy )
		rm_temp_dir( mount_point, operationdetail ) ;

	return success ;
}


void btrfs::read_label(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("btrfs filesystem label " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	partition.set_filesystem_label(Utils::trim_trailing_new_line(output));
}


void btrfs::read_uuid(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	Utils::execute_command("btrfs inspect-internal dump-super " + Glib::shell_quote(partition.get_path()),
		               output, error, true);
	// btrfs inspect-internal dump-super returns zero exit status for both success and
	// failure.  Instead use non-empty stderr to identify failure.
	if (! error.empty())
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	partition.uuid = Utils::regexp_label(output, "^fsid[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")");
}


bool btrfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("btrfstune -f -u " + Glib::shell_quote(partition.get_path()),
	                        EXEC_CHECK_STATUS);
}


void btrfs::clear_cache()
{
	btrfs_device_cache .clear() ;
}

//Return the device which is mounting the btrfs in this partition.
//  Return empty string if not found (not mounted).
Glib::ustring btrfs::get_mount_device( const Glib::ustring & path )
{
	const BTRFS_Device& btrfs_dev = get_cache_entry(path);

	if ( btrfs_dev .devid == -1 || btrfs_dev .members .empty() )
	{
		//WARNING:
		//  No btrfs device cache entry found or entry without any member devices.
		//  Use fallback busy detection method which can only determine if the
		//  mounting device is mounted or not, not any of the other members of a
		//  multi-device btrfs file system.
		if ( Mount_Info::is_dev_mounted( path ) )
			return path ;
		return "" ;
	}

	for ( unsigned int i = 0 ; i < btrfs_dev .members .size() ; i ++ )
		if ( Mount_Info::is_dev_mounted( btrfs_dev.members[i] ) )
			return btrfs_dev.members[i].m_name;
	return "" ;
}

std::vector<Glib::ustring> btrfs::get_members( const Glib::ustring & path )
{
	const BTRFS_Device& btrfs_dev = get_cache_entry(path);
	std::vector<Glib::ustring> membs;
	for ( unsigned int i = 0 ; i < btrfs_dev.members.size() ; i ++ )
		membs.push_back( btrfs_dev.members[i].m_name );
	return membs;
}

//Private methods

//Return btrfs device cache entry, incrementally loading cache as required
const BTRFS_Device & btrfs::get_cache_entry( const Glib::ustring & path )
{
	static BTRFS_Device not_found_btrfs_dev = {-1, };
	if (! btrfs_found)
		// Without the btrfs command it can't be executed to discover the devices
		// in a btrfs file system and so btrfs_device_cache is never populated.
		// Hence returning not found record even before searching the cache.
		return not_found_btrfs_dev;

	std::map<BlockSpecial, BTRFS_Device>::const_iterator bd_iter = btrfs_device_cache.find( BlockSpecial( path ) );
	if ( bd_iter != btrfs_device_cache .end() )
		return bd_iter ->second ;

	Glib::ustring output;
	Glib::ustring error;
	std::vector<int> devid_list ;
	std::vector<Glib::ustring> path_list ;
	Utils::execute_command("btrfs filesystem show " + Glib::shell_quote(path), output, error, true);
	//In many cases the exit status doesn't reflect valid output or an error condition
	//  so rely on parsing the output to determine success.

	//Extract devid and path for each device from output like this:
	//  Label: none  uuid: 36eb51a2-2927-4c92-820f-b2f0b5cdae50
	//          Total devices 2 FS bytes used 156.00KB
	//          devid    2 size 2.00GB used 512.00MB path /dev/sdb2
	//          devid    1 size 2.00GB used 240.75MB path /dev/sdb1
	Glib::ustring::size_type offset = 0 ;
	Glib::ustring::size_type index ;
	while ( ( index = output .find( "devid ", offset ) ) != Glib::ustring::npos )
	{
		int devid = -1 ;
		sscanf( output .substr( index ) .c_str(), "devid %d", &devid ) ;
		Glib::ustring devid_path = Utils::regexp_label( output .substr( index ),
		                                                "devid .* path (/dev/[[:graph:]]+)" ) ;
		if ( devid > -1 && ! devid_path .empty() )
		{
			devid_list .push_back( devid ) ;
			path_list .push_back( devid_path ) ;
		}
		offset = index + 5 ;  //Next find starts immediately after current "devid"
	}
	//Add cache entries for all found devices
	std::vector<BlockSpecial> bs_list;
	for ( unsigned int i = 0 ; i < path_list.size() ; i ++ )
		bs_list.push_back( BlockSpecial( path_list[i] ) );
	for ( unsigned int i = 0 ; i < devid_list .size() ; i ++ )
	{
		BTRFS_Device btrfs_dev ;
		btrfs_dev .devid = devid_list[ i ] ;
		btrfs_dev.members = bs_list;
		btrfs_device_cache[ BlockSpecial( path_list[i] ) ] = btrfs_dev;
	}

	bd_iter = btrfs_device_cache.find( BlockSpecial( path ) );
	if ( bd_iter != btrfs_device_cache .end() )
		return bd_iter ->second ;

	//If for any reason we fail to parse the information return an "unknown" record
	return not_found_btrfs_dev;
}


}  // namespace GParted
