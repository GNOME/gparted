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


#include "../include/btrfs.h"
#include "../include/GParted_Core.h"

#include <ctype.h>

namespace GParted
{

bool btrfs_found = false ;
bool resize_to_same_size_fails = true ;

//Cache of all devices in each btrfs file system by device
//  E.g. For a single device btrfs on /dev/sda2 and a three device btrfs
//       on /dev/sd[bcd]1 the cache would be:
//  btrfs_device_cache["/dev/sda2"] = ["/dev/sda2"]
//  btrfs_device_cache["/dev/sdb1"] = ["/dev/sdd1", "/dev/sdc1", "/dev/sdb1"]
//  btrfs_device_cache["/dev/sdc1"] = ["/dev/sdd1", "/dev/sdc1", "/dev/sdb1"]
//  btrfs_device_cache["/dev/sdd1"] = ["/dev/sdd1", "/dev/sdc1", "/dev/sdb1"]
std::map< Glib::ustring, std::vector<Glib::ustring> > btrfs_device_cache ;

FS btrfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_BTRFS ;

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

	if ( fs .check )
	{
		fs .copy = GParted::FS::GPARTED ;
		fs .move = GParted::FS::GPARTED ;
	}

	fs .online_read = FS::GPARTED ;
#ifdef ENABLE_ONLINE_RESIZE
	if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
	{
		fs .online_grow = fs .grow ;
		fs .online_shrink = fs .shrink ;
	}
#endif

	fs .MIN = 256 * MEBIBYTE ;

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
	return (! execute_command( "mkfs.btrfs -L \"" + new_partition .get_label() + "\" " + new_partition .get_path(), operationdetail ) );
}

bool btrfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return (! execute_command( "btrfsck " + partition .get_path(), operationdetail )) ;
}

void btrfs::set_used_sectors( Partition & partition )
{
	if ( btrfs_found )
		exit_status = Utils::execute_command( "btrfs filesystem show " + partition .get_path(), output, error, true ) ;
	else
		exit_status = Utils::execute_command( "btrfs-show " + partition .get_path(), output, error, true ) ;
	if ( ! exit_status )
	{
		//FIXME: Improve free space calculation for multi-device
		//  btrfs file systems.  Currently uses the size of the
		//  btrfs device in this partition (spot on) and the
		//  file system wide used bytes (wrong for multi-device
		//  file systems).

		Byte_Value ptn_bytes = partition .get_byte_length() ;
		Glib::ustring str ;
		//Btrfs file system device size
		Glib::ustring regexp = "devid .* size ([0-9\\.]+( ?[KMGTPE]?i?B)?) .* path " + partition .get_path() ;
		if ( ! ( str = Utils::regexp_label( output, regexp ) ) .empty() )
			T = btrfs_size_to_num( str, ptn_bytes, true ) ;

		//Btrfs file system wide used bytes
		if ( ! ( str = Utils::regexp_label( output, "FS bytes used ([0-9\\.]+( ?[KMGTPE]?i?B)?)" ) ) .empty() )
			N = T - btrfs_size_to_num( str, ptn_bytes, false ) ;

		if ( T > -1 && N > -1 )
		{
			T = Utils::round( T / double(partition .sector_size) ) ;
			N = Utils::round( N / double(partition .sector_size) ) ;
			partition .set_sector_usage( T, N );
		}
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool btrfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "btrfs filesystem label " + partition .get_path() + " \"" + partition .get_label() + "\"", operationdetail ) ;
}

bool btrfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;

	Glib::ustring mount_point ;
	if ( ! partition_new .busy )
	{
		mount_point = mk_temp_dir( "", operationdetail ) ;
		if ( mount_point .empty() )
			return false ;
		success &= ! execute_command( "mount -v -t btrfs " + partition_new .get_path() + " " + mount_point,
		                              operationdetail, true ) ;
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
			cmd = "btrfs filesystem resize " + size + " " + mount_point ;
		else
			cmd = "btrfsctl -r " + size + " " + mount_point ;
		exit_status = execute_command( cmd, operationdetail, false ) ;
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
			                     || (   btrfs_found && exit_status == 30<<8 )
			                     || ( ! btrfs_found && exit_status ==  1<<8 )
			                   ) ;
		}
		operationdetail .get_last_child() .set_status( resize_succeeded ? STATUS_SUCCES : STATUS_ERROR ) ;
		success &= resize_succeeded ;

		if ( ! partition_new .busy )
			success &= ! execute_command( "umount -v " + mount_point, operationdetail, true ) ;
	}

	if ( ! partition_new .busy )
		rm_temp_dir( mount_point, operationdetail ) ;

	return success ;
}

void btrfs::read_label( Partition & partition )
{
	if ( btrfs_found )
	{
		exit_status = Utils::execute_command( "btrfs filesystem show " + partition .get_path(), output, error, true ) ;
		if ( ! exit_status )
		{
			partition .set_label( Utils::regexp_label( output, "^Label: '(.*)'  uuid:" ) ) ;
			//Btrfs filesystem show encloses the label in single
			//  quotes or reports "none" without single quotes, so
			//  the cases are distinguishable and this regexp won't
			//  match the no label case.  In the no match case
			//  regexp_label() returns "" and this is used to set
			//  the set the blank label.
		}
	}
	else
	{
		exit_status = Utils::execute_command( "btrfs-show " + partition .get_path(), output, error, true ) ;
		if ( ! exit_status )
		{
			Glib::ustring label = Utils::regexp_label( output, "^Label: (.*)  uuid:" ) ;
			//Btrfs-show reports "none" when there is no label, but
			//  this is indistinguishable from the label actually
			//  being "none".  Assume no label case.
			if ( label != "none" )
				partition .set_label( label ) ;
			else
				partition .set_label( "" ) ;
		}
	}
	if ( exit_status )
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void btrfs::read_uuid( Partition & partition )
{
	if ( btrfs_found )
	{
		exit_status = Utils::execute_command( "btrfs filesystem show " + partition .get_path(), output, error, true ) ;
		if ( ! exit_status )
		{
			partition .uuid = Utils::regexp_label( output, "uuid:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
		}
	}
	else
	{
		exit_status = Utils::execute_command( "btrfs-show " + partition .get_path(), output, error, true ) ;
		if ( ! exit_status )
		{
			partition .uuid = Utils::regexp_label( output, "uuid:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
		}
	}
	if ( exit_status )
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void btrfs::clear_cache()
{
	btrfs_device_cache .clear() ;
}

//Return the device which is mounting the btrfs in this partition.
//  Return empty string if not found (not mounted).
Glib::ustring btrfs::get_mount_device( const Glib::ustring & path )
{
	std::vector<Glib::ustring> entry = get_cache_entry( path ) ;
	for ( unsigned int i = 0 ; i < entry .size() ; i ++ )
		if ( GParted_Core::is_dev_mounted( entry[ i ] ) )
			return entry[ i ] ;
	return "" ;
}

//Private methods

//Return btrfs device cache entry, incrementally loading cache as required
const std::vector<Glib::ustring> btrfs::get_cache_entry( const Glib::ustring & path )
{
	std::vector<Glib::ustring> entry = btrfs_device_cache[ path ] ;

	if ( ! entry .empty() )
		return entry ;

	int exit_status ;
	Glib::ustring output, error ;
	if ( btrfs_found )
		exit_status = Utils::execute_command( "btrfs filesystem show " + path, output, error, true ) ;
	else
		exit_status = Utils::execute_command( "btrfs-show " + path, output, error, true ) ;
	if ( ! exit_status )
	{
		//Extract path for each devid from output like this:
		//  Label: none  uuid: 36eb51a2-2927-4c92-820f-b2f0b5cdae50
		//          Total devices 2 FS bytes used 156.00KB
		//          devid    2 size 2.00GB used 512.00MB path /dev/sdb2
		//          devid    1 size 2.00GB used 240.75MB path /dev/sdb1
		Glib::ustring::size_type offset = 0 ;
		Glib::ustring::size_type index ;
		while ( ( index = output .find( "devid ", offset ) ) != Glib::ustring::npos )
		{
			Glib::ustring devid_path = Utils::regexp_label( output .substr( index ),
			                                                "devid .* path (/dev/[[:graph:]]+)" ) ;
			if ( ! devid_path .empty() )
			{
				entry .push_back( devid_path ) ;
			}
			offset = index + 5 ;  //Next find starts immediately after current "devid"
		}
	}
	//Add cache entries for all found devices
	for ( unsigned int i = 0 ; i < entry .size() ; i ++ )
		btrfs_device_cache[ entry[ i ] ] = entry ;
	return entry ;
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
