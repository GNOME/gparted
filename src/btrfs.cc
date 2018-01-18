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

#include <ctype.h>

namespace GParted
{

bool btrfs_found = false ;
bool resize_to_same_size_fails = true ;

// Cache of required btrfs file system device information by device
// E.g. For a single device btrfs on /dev/sda2 and a three device btrfs
//      on /dev/sd[bcd]1 the cache would be as follows.  (Note that
//      BS(str) is short hand for constructor BlockSpecial(str)).
//  btrfs_device_cache[BS("/dev/sda2")] = {devid=1, members=[BS("/dev/sda2")]}
//  btrfs_device_cache[BS("/dev/sdb1")] = {devid=1, members=[BS("/dev/sdd1"), BS("/dev/sdc1"), BS("/dev/sdb1")]}
//  btrfs_device_cache[BS("/dev/sdc1")] = {devid=2, members=[BS("/dev/sdd1"), BS("/dev/sdc1"), BS("/dev/sdb1")]}
//  btrfs_device_cache[BS("/dev/sdd1")] = {devid=3, members=[BS("/dev/sdd1"), BS("/dev/sdc1"), BS("/dev/sdb1")]}
std::map<BlockSpecial, BTRFS_Device> btrfs_device_cache;

FS btrfs::get_filesystem_support()
{
	FS fs( FS_BTRFS );

	fs .busy = FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "mkfs.btrfs" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "btrfsck" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;

	btrfs_found = ( ! Glib::find_program_in_path( "btrfs" ) .empty() ) ;
	if ( btrfs_found )
	{
		//Use newer btrfs multi-tool control command.  No need
		//  to test for filesystem show and filesystem resize
		//  sub-commands as they were always included.

		fs .read = GParted::FS::EXTERNAL ;
		fs .read_label = FS::EXTERNAL ;
		fs .read_uuid = FS::EXTERNAL ;

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

		//Test for labelling capability in btrfs command
		if ( ! Utils::execute_command( "btrfs filesystem label --help", output, error, true ) )
			fs .write_label = FS::EXTERNAL;
	}
	else
	{
		//Fall back to using btrfs-show and btrfsctl, which
		//  were depreciated October 2011

		if ( ! Glib::find_program_in_path( "btrfs-show" ) .empty() )
		{
			fs .read = GParted::FS::EXTERNAL ;
			fs .read_label = FS::EXTERNAL ;
			fs .read_uuid = FS::EXTERNAL ;
		}

		//Resizing of btrfs requires btrfsctl, mount, umount
		//  and kernel support
		if (    ! Glib::find_program_in_path( "btrfsctl" ) .empty()
		     && ! Glib::find_program_in_path( "mount" ) .empty()
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
		Utils::execute_command( "btrfstune --help", output, error, true );
		if ( Utils::regexp_label( output + error, "^[[:blank:]]*(-u)[[:blank:]]" ) == "-u" )
			fs.write_uuid = FS::EXTERNAL;
	}

	if ( fs .check )
	{
		fs .copy = GParted::FS::GPARTED ;
		fs .move = GParted::FS::GPARTED ;
	}

	fs .online_read = FS::EXTERNAL ;
#ifdef ENABLE_ONLINE_RESIZE
	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
	{
		fs .online_grow = fs .grow ;
		fs .online_shrink = fs .shrink ;
	}
#endif

	fs_limits.min_size = 256 * MEBIBYTE;

	//Linux before version 3.2 fails when resizing btrfs file system
	//  to the same size.
	resize_to_same_size_fails = ! Utils::kernel_version_at_least( 3, 2, 0 ) ;

	return fs ;
}

bool btrfs::is_busy( const Glib::ustring & path )
{
	//A btrfs file system is busy if any of the member devices are mounted.
	//  WARNING:
	//  Removal of the mounting device from a btrfs file system makes it impossible to
	//  determine whether the file system is mounted or not for linux <= 3.4.  This is
	//  because /proc/mounts continues to show the old device which is no longer a
	//  member of the file system.  Fixed in linux 3.5 by commit:
	//      Btrfs: implement ->show_devname
	//      https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=9c5085c147989d48dfe74194b48affc23f376650
	return ! get_mount_device( path ) .empty() ;
}

bool btrfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkfs.btrfs -L " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
	                          " " + Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool btrfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "btrfsck " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

void btrfs::set_used_sectors( Partition & partition )
{
	//Called when the file system is unmounted *and* when mounted.
	//
	//  Btrfs has a volume manager layer within the file system which allows it to
	//  provide multiple levels of data redundancy, RAID levels, and use multiple
	//  devices both of which can be changed while the file system is mounted.  To
	//  achieve this btrfs has to allocate space at two different levels: (1) chunks
	//  of 256 MiB or more at the volume manager level; and (2) extents at the file
	//  data level.
	//  References:
	//  *   Btrfs: Working with multiple devices
	//      https://lwn.net/Articles/577961/
	//  *   Btrfs wiki: Glossary
	//      https://btrfs.wiki.kernel.org/index.php/Glossary
	//
	//  This makes the question of how much disk space is being used in an individual
	//  device a complicated question to answer.  Further, the current btrfs tools
	//  don't provide the required information.
	//
	//  Btrfs filesystem show only provides space usage information at the chunk level
	//  per device.  At the file extent level only a single figure for the whole file
	//  system is provided.  It also reports size of the data and metadata being
	//  stored, not the larger figure of the amount of space taken after redundancy is
	//  applied.  So it is impossible to answer the question of how much disk space is
	//  being used in an individual device.  Example output:
	//
	//      Label: none  uuid: 36eb51a2-2927-4c92-820f-b2f0b5cdae50
	//              Total devices 2 FS bytes used 156.00KB
	//              devid    2 size 2.00GB used 512.00MB path /dev/sdb2
	//              devid    1 size 2.00GB used 240.75MB path /dev/sdb1
	//
	//  Guesstimate the per device used figure as the fraction of the file system wide
	//  extent usage based on chunk usage per device.
	//
	//  Positives:
	//  1) Per device used figure will correctly be between zero and allocated chunk
	//     size.
	//
	//  Known inaccuracies:
	//  [for single and multi-device btrfs file systems]
	//  1) Btrfs filesystem show reports file system wide file extent usage without
	//     considering redundancy applied to that data.  (By default btrfs stores two
	//     copies of metadata and one copy of data).
	//  2) At minimum size when all data has been consolidated there will be a few
	//     partly filled chunks of 256 MiB or more for data and metadata of each
	//     storage profile (RAID level).
	//  [for multi-device btrfs file systems only]
	//  3) Data may be far from evenly distributed between the chunks on multiple
	//     devices.
	//  4) Extents can be and are relocated to other devices within the file system
	//     when shrinking a device.
	if ( btrfs_found )
		Utils::execute_command( "btrfs filesystem show " + Glib::shell_quote( partition.get_path() ),
		                        output, error, true );
	else
		Utils::execute_command( "btrfs-show " + Glib::shell_quote( partition.get_path() ),
		                        output, error, true );
	//In many cases the exit status doesn't reflect valid output or an error condition
	//  so rely on parsing the output to determine success.

	//Extract the per device size figure.  Guesstimate the per device used
	// figure as discussed above.  Example output:
	//
	//      Label: none  uuid: 36eb51a2-2927-4c92-820f-b2f0b5cdae50
	//              Total devices 2 FS bytes used 156.00KB
	//              devid    2 size 2.00GB used 512.00MB path /dev/sdb2
	//              devid    1 size 2.00GB used 240.75MB path /dev/sdb1
	//
	// Calculations:
	//      ptn fs size = devid size
	//      ptn fs used = total fs used * devid used / sum devid used

	Byte_Value ptn_size = partition .get_byte_length() ;
	Byte_Value total_fs_used = -1 ;  //total fs used
	Byte_Value sum_devid_used = 0 ;  //sum devid used
	Byte_Value devid_used = -1 ;     //devid used
	Byte_Value devid_size = -1 ;     //devid size

	//Btrfs file system wide used bytes (extents and items)
	Glib::ustring str ;
	if ( ! ( str = Utils::regexp_label( output, "FS bytes used ([0-9\\.]+( ?[KMGTPE]?i?B)?)" ) ) .empty() )
		total_fs_used = Utils::round( btrfs_size_to_gdouble( str ) ) ;

	Glib::ustring::size_type offset = 0 ;
	Glib::ustring::size_type index ;
	while ( ( index = output .find( "devid ", offset ) ) != Glib::ustring::npos )
	{
		Glib::ustring devid_path = Utils::regexp_label( output .substr( index ),
		                                                "devid .* path (/dev/[[:graph:]]+)" ) ;
		if ( ! devid_path .empty() )
		{
			//Btrfs per devid used bytes (chunks)
			Byte_Value used = -1 ;
			if ( ! ( str = Utils::regexp_label( output .substr( index ),
			                                    "devid .* used ([0-9\\.]+( ?[KMGTPE]?i?B)?) path" ) ) .empty() )
			{
				used = btrfs_size_to_num( str, ptn_size, false ) ;
				sum_devid_used += used ;
				if ( devid_path == partition .get_path() )
					devid_used = used ;
			}

			if ( devid_path == partition .get_path() )
			{
				//Btrfs per device size bytes (chunks)
				if ( ! ( str = Utils::regexp_label( output .substr( index ),
				                                    "devid .* size ([0-9\\.]+( ?[KMGTPE]?i?B)?) used " ) ) .empty() )
					devid_size = btrfs_size_to_num( str, ptn_size, true ) ;
			}
		}
		offset = index + 5 ;  //Next find starts immediately after current "devid"
	}

	if ( total_fs_used > -1 && devid_size > -1 && devid_used > -1 && sum_devid_used > 0 )
	{
		T = Utils::round( devid_size / double(partition .sector_size) ) ;               //ptn fs size
		double ptn_fs_used = total_fs_used * ( devid_used / double(sum_devid_used) ) ;  //ptn fs used
		N = T - Utils::round( ptn_fs_used / double(partition .sector_size) ) ;
		partition .set_sector_usage( T, N ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool btrfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "btrfs filesystem label " + Glib::shell_quote( partition.get_path() ) +
	                          " " + Glib::shell_quote( partition.get_filesystem_label() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool btrfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;
	Glib::ustring path = partition_new .get_path() ;

	BTRFS_Device btrfs_dev = get_cache_entry( path ) ;
	if ( btrfs_dev .devid == -1 )
	{
		operationdetail .add_child( OperationDetail(
				String::ucompose( _("Failed to find devid for path %1"), path ), STATUS_ERROR ) ) ;
		return false ;
	}
	Glib::ustring devid_str = Utils::num_to_str( btrfs_dev .devid ) ;

	Glib::ustring mount_point ;
	if ( ! partition_new .busy )
	{
		mount_point = mk_temp_dir( "", operationdetail ) ;
		if ( mount_point .empty() )
			return false ;
		success &= ! execute_command( "mount -v -t btrfs " + Glib::shell_quote( path ) +
		                              " " + Glib::shell_quote( mount_point ),
		                              operationdetail, EXEC_CHECK_STATUS );
	}
	else
		mount_point = partition_new .get_mountpoint() ;

	if ( success )
	{
		Glib::ustring size ;
		if ( ! fill_partition )
			size = Utils::num_to_str( floor( Utils::sector_to_unit(
					partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) ) + "K" ;
		else
			size = "max" ;
		Glib::ustring cmd ;
		if ( btrfs_found )
			cmd = "btrfs filesystem resize " + devid_str + ":" + size + " " + Glib::shell_quote( mount_point );
		else
			cmd = "btrfsctl -r " + devid_str + ":" + size + " " + Glib::shell_quote( mount_point );
		exit_status = execute_command( cmd, operationdetail );
		bool resize_succeeded = ( exit_status == 0 ) ;
		if ( resize_to_same_size_fails )
		{
			//Linux before version 3.2 fails when resizing a
			//  btrfs file system to the same size with ioctl()
			//  returning -1 EINVAL (Invalid argument) from the
			//  kernel btrfs code.
			//  *   Btrfs filesystem resize reports this as exit
			//      status 30:
			//          ERROR: Unable to resize '/MOUNTPOINT'
			//  *   Btrfsctl -r reports this as exit status 1:
			//          ioctl:: Invalid argument
			//  WARNING:
			//  Ignoring these errors could mask real failures,
			//  but not ignoring them will cause resizing to the
			//  same size as part of check operation to fail.
			resize_succeeded = (    exit_status == 0
			                     || (   btrfs_found && exit_status == 30 )
			                     || ( ! btrfs_found && exit_status ==  1 )
			                   ) ;
		}
		set_status( operationdetail, resize_succeeded );
		success &= resize_succeeded ;

		if ( ! partition_new .busy )
			success &= ! execute_command( "umount -v " + Glib::shell_quote( mount_point ),
			                              operationdetail, EXEC_CHECK_STATUS );
	}

	if ( ! partition_new .busy )
		rm_temp_dir( mount_point, operationdetail ) ;

	return success ;
}

void btrfs::read_label( Partition & partition )
{
	if ( btrfs_found )
		Utils::execute_command( "btrfs filesystem show " + Glib::shell_quote( partition.get_path() ),
		                        output, error, true );
	else
		Utils::execute_command( "btrfs-show " + Glib::shell_quote( partition.get_path() ),
		                        output, error, true );
	//In many cases the exit status doesn't reflect valid output or an error condition
	//  so rely on parsing the output to determine success.

	if ( output .compare( 0, 18, "Label: none  uuid:" ) == 0 )
	{
		//Indistinguishable cases of either no label or the label is actually set
		//  to "none".  Assume no label case.
		partition.set_filesystem_label( "" );
	}
	else
	{
		//Try matching a label enclosed in single quotes, as used by
		//  btrfs filesystem show, then without quotes, as used by btrfs-show.
		Glib::ustring label = Utils::regexp_label( output, "^Label: '(.*)'  uuid:" ) ;
		if ( label .empty() )
			label = Utils::regexp_label( output, "^Label: (.*)  uuid:" ) ;

		if ( ! label .empty() )
			partition.set_filesystem_label( label );
		else
		{
			if ( ! output .empty() )
				partition.push_back_message( output );

			if ( ! error .empty() )
				partition.push_back_message( error );
		}
	}
}

void btrfs::read_uuid( Partition & partition )
{
	if ( btrfs_found )
		Utils::execute_command( "btrfs filesystem show " + Glib::shell_quote( partition.get_path() ),
		                        output, error, true );
	else
		Utils::execute_command( "btrfs-show " + Glib::shell_quote( partition.get_path() ),
		                        output, error, true );
	//In many cases the exit status doesn't reflect valid output or an error condition
	//  so rely on parsing the output to determine success.

	Glib::ustring uuid_str = Utils::regexp_label( output, "uuid:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	if ( ! uuid_str .empty() )
		partition .uuid = uuid_str ;
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool btrfs::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "btrfstune -f -u " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

void btrfs::clear_cache()
{
	btrfs_device_cache .clear() ;
}

//Return the device which is mounting the btrfs in this partition.
//  Return empty string if not found (not mounted).
Glib::ustring btrfs::get_mount_device( const Glib::ustring & path )
{
	BTRFS_Device btrfs_dev = get_cache_entry( path ) ;
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
	BTRFS_Device btrfs_dev = get_cache_entry( path ) ;
	std::vector<Glib::ustring> membs;
	for ( unsigned int i = 0 ; i < btrfs_dev.members.size() ; i ++ )
		membs.push_back( btrfs_dev.members[i].m_name );
	return membs;
}

//Private methods

//Return btrfs device cache entry, incrementally loading cache as required
const BTRFS_Device & btrfs::get_cache_entry( const Glib::ustring & path )
{
	std::map<BlockSpecial, BTRFS_Device>::const_iterator bd_iter = btrfs_device_cache.find( BlockSpecial( path ) );
	if ( bd_iter != btrfs_device_cache .end() )
		return bd_iter ->second ;

	Glib::ustring output, error ;
	std::vector<int> devid_list ;
	std::vector<Glib::ustring> path_list ;
	if ( btrfs_found )
		Utils::execute_command( "btrfs filesystem show " + Glib::shell_quote( path ), output, error, true );
	else
		Utils::execute_command( "btrfs-show " + Glib::shell_quote( path ), output, error, true );
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
	static BTRFS_Device btrfs_dev = { -1, } ;
	return btrfs_dev ;
}

//Return the value of a btrfs tool formatted size, including reversing
//  changes in certain cases caused by using binary prefix multipliers
//  and rounding to two decimal places of precision.  E.g. "2.00GB".
Byte_Value btrfs::btrfs_size_to_num( Glib::ustring str, Byte_Value ptn_bytes, bool scale_up )
{
	Byte_Value size_bytes = Utils::round( btrfs_size_to_gdouble( str ) ) ;
	gdouble delta         = btrfs_size_max_delta( str ) ;
	Byte_Value upper_size = size_bytes + ceil( delta ) ;
	Byte_Value lower_size = size_bytes - floor( delta ) ;

	if ( size_bytes > ptn_bytes && lower_size <= ptn_bytes )
	{
		//Scale value down to partition size:
		//  The btrfs tool reported size appears larger than the partition
		//  size, but the minimum possible size which could have been rounded
		//  to the reported figure is within the partition size so use the
		//  smaller partition size instead.  Applied to FS device size and FS
		//  wide used bytes.
		//      ............|         ptn_bytes
		//               [    x    )  size_bytes with upper & lower size
		//                  x         scaled down size_bytes
		//  Do this to avoid the FS size or used bytes being larger than the
		//  partition size and GParted failing to read the file system usage and
		//  report a warning.
		size_bytes = ptn_bytes ;
	}
	else if ( scale_up && size_bytes < ptn_bytes && upper_size > ptn_bytes )
	{
		//Scale value up to partition size:
		//  The btrfs tool reported size appears smaller than the partition
		//  size, but the maximum possible size which could have been rounded
		//  to the reported figure is within the partition size so use the
		//  larger partition size instead.  Applied to FS device size only.
		//      ............|     ptn_bytes
		//           [    x    )  size_bytes with upper & lower size
		//                  x     scaled up size_bytes
		//  Make an assumption that the file system actually fills the
		//  partition, rather than is slightly smaller to avoid false reporting
		//  of unallocated space.
		size_bytes = ptn_bytes ;
	}

	return size_bytes ;
}

//Return maximum delta for which num +/- delta would be rounded by btrfs
//  tools to str.  E.g. btrfs_size_max_delta("2.00GB") -> 5368709.12
gdouble btrfs::btrfs_size_max_delta( Glib::ustring str )
{
	Glib::ustring limit_str ;
	//Create limit_str.  E.g. str = "2.00GB" -> limit_str = "0.005GB"
	for ( Glib::ustring::iterator p = str .begin() ; p != str .end() ; p ++ )
	{
		if ( isdigit( *p ) )
			limit_str .append( "0" ) ;
		else if ( *p == '.' )
			limit_str .append( "." ) ;
		else
		{
			limit_str .append( "5" ) ;
			limit_str .append( p, str .end() ) ;
			break ;
		}
	}
	gdouble max_delta = btrfs_size_to_gdouble( limit_str ) ;
	return max_delta ;
}

//Return the value of a btrfs tool formatted size.
//  E.g. btrfs_size_to_gdouble("2.00GB") -> 2147483648.0
gdouble btrfs::btrfs_size_to_gdouble( Glib::ustring str )
{
	gchar * suffix ;
	gdouble rawN = g_ascii_strtod( str .c_str(), & suffix ) ;
	while ( isspace( suffix[0] ) )  //Skip white space before suffix
		suffix ++ ;
	unsigned long long mult ;
	switch ( suffix[0] )
	{
		case 'K':	mult = KIBIBYTE ;	break ;
		case 'M':	mult = MEBIBYTE ;	break ;
		case 'G':	mult = GIBIBYTE ;	break ;
		case 'T':	mult = TEBIBYTE ;	break ;
		case 'P':	mult = PEBIBYTE ;	break ;
		case 'E':	mult = EXBIBYTE ;	break ;
		default:	mult = 1 ;		break ;
	}
	return rawN * mult ;
}

} //GParted
