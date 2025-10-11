/* Copyright (C) 2009, 2010, 2011 Curtis Gedak
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

#include "DMRaid.h"
#include "BlockSpecial.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <limits.h>
#include <stdlib.h>		//atoi function
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <glibmm/shell.h>
#include <glibmm/ustring.h>


namespace GParted
{


// Data model:
// dmraid_cache_initialized - Has the cache been loaded?
// dmraid_found             - Is the "dmraid" command available?
// dmsetup_found            - Is the "dmsetup" command available?
// udevadm_found            - Is the "udevadm" command available?
// dmraid_devices           - Vector of active array names.
//                            E.g.
//                            ["isw_ecccdhhiga_MyArray"]
// dmraid_member_cache      - Vector of members of active DMRaid arrays.
//                            E.g.
//                            //member                  , array
//                            [{BlockSpecial("/dev/sdc"), "/dev/mapper/isw_ecccdhhiga_MyArray"},
//                             {BlockSpecial("/dev/sdd"), "/dev/mapper/isw_ecccdhhiga_MyArray"}
//                            ]

//Initialize static data elements
bool DMRaid::dmraid_cache_initialized = false ;
bool DMRaid::dmraid_found  = false ;
bool DMRaid::dmsetup_found = false ;
bool DMRaid::udevadm_found  = false ;
std::vector<Glib::ustring> DMRaid::dmraid_devices ;
std::vector<DMRaid_Member> DMRaid::dmraid_member_cache;


void DMRaid::load_cache()
{
	set_commands_found();
	load_dmraid_cache();
	dmraid_cache_initialized = true;
}


void DMRaid::load_dmraid_cache()
{
	Glib::ustring output, error ;
	dmraid_devices .clear() ;
	dmraid_member_cache.clear();

	// Load active DMRaid array names.
	if ( dmraid_found )
	{
		if ( ! Utils::execute_command( "dmraid -sa -c", output, error, true ) )
		{
			if (output.substr(0, 13).lowercase() != "no raid disks")
				Utils::tokenize( output, dmraid_devices, "\n" ) ;
		}
	}

	// Load members of active DMRaid arrays.
	for (unsigned int i = 0; i < dmraid_devices.size(); i++)
	{
		std::vector<Glib::ustring> members = lookup_dmraid_members(DEV_MAPPER_PATH + dmraid_devices[i]);
		for (unsigned int j = 0; j < members.size(); j++)
		{
			DMRaid_Member memb;
			memb.member = BlockSpecial(members[j]);
			memb.array  = DEV_MAPPER_PATH + dmraid_devices[i];
			dmraid_member_cache.push_back(memb);
		}
	}
}


void DMRaid::set_commands_found()
{
	//Set status of commands found 
	dmraid_found = (! Glib::find_program_in_path( "dmraid" ) .empty() ) ;
	dmsetup_found = (! Glib::find_program_in_path( "dmsetup" ) .empty() ) ;
	udevadm_found = (! Glib::find_program_in_path( "udevadm" ) .empty() ) ;
}

bool DMRaid::is_dmraid_supported()
{
	//Determine if dmraid is supported on this computer
	return ( dmraid_found && dmsetup_found ) ;
}

bool DMRaid::is_dmraid_device( const Glib::ustring & dev_path )
{
	//Determine if the device is a dmraid device
	bool device_found = false ;	//assume not found

	if ( is_dmraid_supported() )
	{
		for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
			if ( dev_path .find( dmraid_devices[k] ) != Glib::ustring::npos )
				device_found = true ;
	}

	//Some distros appear to default to /dev/dm-# for device names, so
	//  also check with udev to see if they are in fact dmraid devices
	if ( ! device_found && ( dev_path .find( "/dev/dm" ) != Glib::ustring::npos ) )
	{
		//Ensure we use the base device name if dev_path in form /dev/dm-#p#
		Glib::ustring regexp = "(/dev/dm-[0-9]+)p[0-9]+" ;
		Glib::ustring dev_path_only = Utils::regexp_label( dev_path, regexp ) ;
		if ( dev_path_only .empty() )
			dev_path_only = dev_path ;

		Glib::ustring udev_name = get_udev_dm_name( dev_path_only ) ;
		if ( ! udev_name .empty() )
		{
			for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
				if ( udev_name .find( dmraid_devices[k] ) != Glib::ustring::npos )
					device_found = true ;
		}

		//Also check for a symbolic link if device not yet found
		if (! device_found && Glib::file_test(dev_path, Glib::FILE_TEST_IS_SYMLINK))
		{
			//Path is a symbolic link so find real path
			char* rpath = realpath(dev_path.c_str(), nullptr);
			if (rpath != nullptr)
			{
				Glib::ustring tmp_path = rpath;
				free( rpath );
				if ( tmp_path .length() > 0 )
					for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
						if ( tmp_path .find( dmraid_devices[k] ) != Glib::ustring::npos )
							device_found = true ;
			}
		}
	}

	return device_found ;
}


std::vector<Glib::ustring> DMRaid::get_devices()
{
	//Retrieve list of dmraid devices
	std::vector<Glib::ustring> device_list;

	for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
		device_list.push_back( DEV_MAPPER_PATH + dmraid_devices[k] );

	return device_list;
}


Glib::ustring DMRaid::get_dmraid_name( const Glib::ustring & dev_path )
{
	//Retrieve name of dmraid device
	Glib::ustring dmraid_name;

	for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
	{
		Glib::ustring regexp = ".*(" + dmraid_devices[k] + ").*";
		if ( Utils::regexp_label( dev_path, regexp ) == dmraid_devices[k] )
			dmraid_name = dmraid_devices[k] ;
	}

	//Some distros appear to default to /dev/dm-# for device names, so
	//  also check with udev for dmraid name
	if ( dmraid_name .empty() && ( dev_path .find( "/dev/dm" ) != Glib::ustring::npos ) )
	{
		Glib::ustring udev_name = get_udev_dm_name( dev_path ) ;
		for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
			if ( udev_name .find( dmraid_devices[k] ) != Glib::ustring::npos )
				dmraid_name = dmraid_devices[k] ;

		//Also check for a symbolic link if dmraid_name not yet found
		if (dmraid_name.empty() && Glib::file_test(dev_path, Glib::FILE_TEST_IS_SYMLINK))
		{
			//Path is a symbolic link so find real path
			char* rpath = realpath(dev_path.c_str(), nullptr);
			if (rpath != nullptr)
			{
				Glib::ustring tmp_path = rpath;
				free( rpath );
				if ( tmp_path .length() > 0 )
					for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
						if ( tmp_path .find( dmraid_devices[k] ) != Glib::ustring::npos )
							dmraid_name = dmraid_devices[k] ;
			}
		}
	}

	return dmraid_name ;
}


std::vector<Glib::ustring> DMRaid::get_dmraid_dir_entries(const Glib::ustring& dev_path)
{
	//Build list of all device entries matching device path

	Glib::ustring dmraid_name = get_dmraid_name( dev_path ) ;

	//Loop through the entries in the directory
	std::vector<Glib::ustring> dir_list;
	Glib::ustring filename;
	Glib::Dir dir( DEV_MAPPER_PATH );
	while ( ( filename = dir .read_name() ) != "" )
	{
		if ( filename == "control" )
			continue ;

		if ( Utils::regexp_label( filename, "^(" + dmraid_name + ")" ) == dmraid_name )
			dir_list .push_back( filename ) ;
	}

	return dir_list;
}


int DMRaid::get_partition_number( const Glib::ustring & partition_name )
{
	Glib::ustring dmraid_name = get_dmraid_name( partition_name ) ;
	return std::atoi( Utils::regexp_label( partition_name, dmraid_name + "p?([0-9]+)" ) .c_str() ) ;
}


Glib::ustring DMRaid::get_udev_dm_name( const Glib::ustring & dev_path )
{
	Glib::ustring dm_name;
	if (! udevadm_found)
		return dm_name;

	//Retrieve DM_NAME of device using udev information
	Glib::ustring output;
	Glib::ustring error;
	Utils::execute_command("udevadm info --query=property --name=" + Glib::shell_quote(dev_path),
	                       output, error, true);
	dm_name = Utils::regexp_label(output, "^DM_NAME=([^\n]*)$");

	return dm_name ;
}


Glib::ustring DMRaid::make_path_dmraid_compatible( Glib::ustring partition_path )
{
	//The purpose of this method is to ensure that the partition name matches
	//   the name that the dmraid command would create.
	//
	//From my experience, the general rule of thumb naming convention for creating
	//  partition names is as follows:
	//
	//  A)  If the device name ends in a number, then append the letter "p" and 
	//      the partition number to create the partition name.
	//      For Example:
	//         Device Name   :  /dev/mapper/isw_cjbdddajhi_Disk0
	//         Partition Name:  /dev/mapper/isw_cjbdddajhi_Disk0p1
	//
	//  B)  If the device name does not end in a number, then append the 
	//      partition number only to create the partition name.
	//      For Example:
	//         Device Name   :  /dev/mapper/isw_cjbdddajhi_Volume
	//         Partition Name:  /dev/mapper/isw_cjbdddajhi_Volume1
	//
	//The dmraid command appears to never add the "p" into the partition name.
	//  Instead in all cases dmraid simply appends the partition number to the
	//  device name to create a partition name.
	//
	Glib::ustring reg_exp ;
	Glib::ustring partition_number ;

	for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
	{
		reg_exp = DEV_MAPPER_PATH + dmraid_devices[k] + "p([0-9]+)";
		partition_number = Utils::regexp_label( partition_path, reg_exp ) ;
		if ( ! partition_number .empty() )
		{
			partition_path = DEV_MAPPER_PATH + dmraid_devices[k] + partition_number;
			return partition_path ;
		}
	}

	//If parted is configured with --disable-device-mapper then the partition path
	//  could default to something like /dev/dm-#p# on some distros, so we need to
	//  convert these names back into /dev/mapper entries
	reg_exp = "/dev/dm-[0-9]+p([0-9]+)" ;
	partition_number = Utils::regexp_label( partition_path, reg_exp ) ;
	if ( ! partition_number .empty() )
	{
		reg_exp = "(/dev/dm-[0-9]+)p[0-9]+" ;
		Glib::ustring device_path = Utils::regexp_label( partition_path, reg_exp ) ;
		if ( ! device_path .empty() )
		{
			device_path = get_udev_dm_name( device_path ) ;
			partition_path = DEV_MAPPER_PATH + device_path + partition_number;
		}
	}

	return partition_path ;
}


bool DMRaid::create_dev_map_entries( const Partition & partition, OperationDetail & operationdetail )
{
	//Create all missing dev mapper entries for a specified device.

	bool success = true;

	/*TO TRANSLATORS: looks like  create missing /dev/mapper entries */ 
	operationdetail.add_child(OperationDetail(
	                        Glib::ustring::compose(_("create missing %1 entries"), DEV_MAPPER_PATH)));
	OperationDetail& child_od = operationdetail.get_last_child();

	//Newer dmraid defaults to always inserting the letter 'p' between the device name
	//  and the partition number.
	if (child_od.execute_command("dmraid -ay -P \"\" -v"))
		success = false;  // command failed

	child_od.set_success_and_capture_errors(success);
	return success;
}


bool DMRaid::create_dev_map_entries( const Glib::ustring & dev_path )
{
	//Create all missing dev mapper entries for a specified device.

	Glib::ustring output, error ;

	//Newer dmraid defaults to always inserting the letter 'p' between the device name
	//  and the partition number.
	return ! Utils::execute_command("dmraid -ay -P \"\" -v", output, error, true);
}


std::vector<Glib::ustring> DMRaid::get_affected_dev_map_entries(const Partition& partition)
{
	//Build list of affected /dev/mapper entries when a partition is to be deleted.

	//Retrieve list of matching directory entries
	std::vector<Glib::ustring> dir_list = get_dmraid_dir_entries(partition.device_path);

	//All partition numbers equal to the number of the partition to be deleted
	//  will be affected.
	//  Also, if the partition is inside an extended partition, then all logical
	//  partition numbers greater than the number of the partition to be deleted
	//  will be affected.
	Glib::ustring dmraid_name = get_dmraid_name( partition .device_path ) ;
	std::vector<Glib::ustring> affected_entries;
	for ( unsigned int k=0; k < dir_list .size(); k++ )
	{
		if ( Utils::regexp_label( dir_list[k], "^(" + dmraid_name + ")" ) == dmraid_name )
		{
			int dir_part_num = get_partition_number( dir_list[k] ) ;
			if ( dir_part_num == partition .partition_number ||
			     ( partition .inside_extended && dir_part_num > partition .partition_number )
			   )
				affected_entries .push_back( dir_list[k] ) ;
		}
	}

	return affected_entries;
}


std::vector<Glib::ustring> DMRaid::get_partition_dev_map_entries(const Partition& partition)
{
	//Build list of all /dev/mapper entries for a partition.

	//Retrieve list of matching directory entries
	std::vector<Glib::ustring> dir_list = get_dmraid_dir_entries(partition.device_path);

	//Retrieve all partition numbers equal to the number of the partition.
	Glib::ustring dmraid_name = get_dmraid_name( partition .device_path ) ;
	std::vector<Glib::ustring> partition_entries;
	for ( unsigned int k=0; k < dir_list .size(); k++ )
	{
		if ( Utils::regexp_label( dir_list[k], "^(" + dmraid_name + ")" ) == dmraid_name )
		{
			int dir_part_num = get_partition_number( dir_list[k] ) ;
			if ( dir_part_num == partition .partition_number )
				partition_entries .push_back( dir_list[k] ) ;
		}
	}

	return partition_entries;
}


bool DMRaid::delete_affected_dev_map_entries( const Partition & partition, OperationDetail & operationdetail )
{
	// Delete all affected dev mapper entries (logical partitions >= specified partition)

	Glib::ustring command ;
	bool success = true;

	/*TO TRANSLATORS: looks like  delete affected /dev/mapper entries */ 
	operationdetail.add_child(OperationDetail(
	                        Glib::ustring::compose(_("delete affected %1 entries"), DEV_MAPPER_PATH)));
	OperationDetail& child_od = operationdetail.get_last_child();

	std::vector<Glib::ustring> affected_entries = get_affected_dev_map_entries(partition);

	for ( unsigned int k=0; k < affected_entries .size(); k++ )
	{
		command = "dmsetup remove " + Glib::shell_quote( DEV_MAPPER_PATH + affected_entries[k] );
		if (child_od.execute_command(command))
			success = false;  // command failed
	}

	child_od.set_success_and_capture_errors(success);
	return success;
}


bool DMRaid::delete_dev_map_entry( const Partition & partition, OperationDetail & operationdetail )
{
	//Delete a single partition which may be represented by multiple dev mapper entries
	bool success = true;

	/*TO TRANSLATORS: looks like  delete /dev/mapper entry */ 
	operationdetail.add_child(OperationDetail(
	                        Glib::ustring::compose(_("delete %1 entry"), DEV_MAPPER_PATH)));
	OperationDetail& child_od = operationdetail.get_last_child();

	std::vector<Glib::ustring> partition_entries = get_partition_dev_map_entries(partition);
	for ( unsigned int k = 0; k < partition_entries .size(); k++ )
	{
		Glib::ustring command = "dmsetup remove " + Glib::shell_quote( partition_entries[k] );
		if (child_od.execute_command(command))
			success = false;  // command failed
	}

	child_od.set_success_and_capture_errors(success);
	return success;
}


bool DMRaid::purge_dev_map_entries( const Glib::ustring & dev_path )
{
	//Delete all dev mapper entries for dmraid device

	std::vector<Glib::ustring> dir_list = get_dmraid_dir_entries(dev_path);
	Glib::ustring command ;
	Glib::ustring output, error ;
	bool success = true;

	for ( unsigned int k=0; k < dir_list .size(); k++ )
	{
		command = "dmsetup remove " + Glib::shell_quote( DEV_MAPPER_PATH + dir_list[k] );
		if ( Utils::execute_command( command, output, error, true ) )
			success = false;  // command failed
	}

	return success;
}


bool DMRaid::update_dev_map_entry( const Partition & partition, OperationDetail & operationdetail )
{
	//Update dmraid entry by first deleting the entry then recreating the entry

	//Skip if extended partition because the only change is to the partition table.
	if (partition.type == TYPE_EXTENDED)
		return true ;

	bool success = true;

	/*TO TRANSLATORS: looks like  update /dev/mapper entry */ 
	operationdetail.add_child(OperationDetail(
	                        Glib::ustring::compose(_("update %1 entry"), DEV_MAPPER_PATH)));
	OperationDetail& child_od = operationdetail.get_last_child();

	if (! delete_dev_map_entry(partition, child_od))
		success = false;  // command failed

	if (! create_dev_map_entries(partition, child_od))
		success = false;  // command failed

	child_od.set_success_and_capture_errors(success);
	return success;
}


// Return whether the named device (e.g. "/dev/sdc") is a member of a DMRaid array or not.
bool DMRaid::is_member(const Glib::ustring& member_path)
{
	const DMRaid_Member& memb = get_cache_entry_by_member(member_path);
	if (memb.member.m_name.length() > 0)
		return true;

	return false;
}

// Return whether the named device (e.g. "/dev/sdc") is a member of an *active* DMRaid
// array or not.
// As the cache only records members of active DMRaid arrays:
//     is_member_active(x) == is_member(x)
bool DMRaid::is_member_active(const Glib::ustring& member_path)
{
	return is_member(member_path);
}


// Return array device containing the specified member, or "" if the array is not running
// or there is no such member.
const Glib::ustring& DMRaid::get_array(const Glib::ustring& member_path)
{
	const DMRaid_Member& memb = get_cache_entry_by_member(member_path);
	return memb.array;
}


// Return vector of member devices of an active DMRaid array.
// E.g. lookup_dmraid_members("/dev/mapper/isw_ecccdhhiga_MyArray") -> ["/dev/sdc", "/dev/sdd"]
std::vector<Glib::ustring> DMRaid::lookup_dmraid_members(const Glib::ustring& array)
{
	// Method:
	// (1) Query udev for the kernel device name;
	// (2) List member names from the directory /sys/block/${name}/slaves.

	std::vector<Glib::ustring> members;
	if (! udevadm_found)
		return members;  // Empty vector

	Glib::ustring output;
	Glib::ustring error;
	Utils::execute_command("udevadm info --query=name " + Glib::shell_quote(array),
	                       output, error, true);
	output = Utils::trim_trailing_new_line(output);

	if (output.empty())
		return members;  // Empty vector

	Glib::ustring filename;
	Glib::Dir dir("/sys/block/" + output + "/slaves");
	while ((filename = dir.read_name()) != "")
	{
		members.push_back("/dev/" + filename);
	}

	return members;
}


// Perform linear search of the cache to find the matching member.
// Return found cache entry or not found substitute.
const DMRaid_Member& DMRaid::get_cache_entry_by_member(const Glib::ustring& member_path)
{
	BlockSpecial bs = BlockSpecial(member_path);
	for (unsigned int i = 0; i < dmraid_member_cache.size(); i++)
	{
		if (bs == dmraid_member_cache[i].member)
			return dmraid_member_cache[i];
	}

	static const DMRaid_Member notfound_memb = {BlockSpecial(), ""};
	return notfound_memb;
}


}  // namespace GParted
