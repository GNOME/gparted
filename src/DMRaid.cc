/* Copyright (C) 2009 Curtis Gedak
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "../include/DMRaid.h"

#include <stdlib.h>		//atoi function

namespace GParted
{

//Initialize static data elements
bool DMRaid::dmraid_cache_initialized = false ;
bool DMRaid::dmraid_found  = false ;
bool DMRaid::dmsetup_found = false ;
bool DMRaid::kpartx_found  = false ;
std::vector<Glib::ustring> DMRaid::dmraid_devices ;

DMRaid::DMRaid()
{
	//Ensure that cache has been loaded at least once
	if ( ! dmraid_cache_initialized ) {
		dmraid_cache_initialized = true ;
		set_commands_found() ;
		load_dmraid_cache() ;
	}
}

DMRaid::DMRaid( const bool & do_refresh )
{
	//Ensure that cache has been loaded at least once
	if ( ! dmraid_cache_initialized )
	{
		dmraid_cache_initialized = true ;
		set_commands_found() ;
		if ( do_refresh == false )
			load_dmraid_cache() ;
	}

	if ( do_refresh )
		load_dmraid_cache() ;
}

DMRaid::~DMRaid()
{
}

void DMRaid::load_dmraid_cache()
{
	//Load data into dmraid structures
	Glib::ustring output, error ;
	if ( dmraid_found ) {
		if ( ! Utils::execute_command( "dmraid -sa -c", output, error, true ) )
		{
			Glib::ustring temp = Utils::regexp_label( output, "^(no raid disks).*" ) ;
			if ( temp == "no raid disks" )
				dmraid_devices .clear() ;
			else
				Utils::tokenize( output, dmraid_devices, "\n" ) ;
		}
		else
			dmraid_devices .clear() ;
	}
}

void DMRaid::set_commands_found()
{
	//Set status of commands found 
	dmraid_found = (! Glib::find_program_in_path( "dmraid" ) .empty() ) ;
	dmsetup_found = (! Glib::find_program_in_path( "dmsetup" ) .empty() ) ;
	kpartx_found = (! Glib::find_program_in_path( "kpartx" ) .empty() ) ;
}

bool DMRaid::is_dmraid_supported()
{
	//Determine if dmraid is supported on this computer
	return ( dmraid_found && dmsetup_found && kpartx_found ) ;
}

bool DMRaid::is_dmraid_device( const Glib::ustring & dev_path )
{
	//Determine if the device is a dmraid device
	bool device_found = false ;	//assume not found

	if ( is_dmraid_supported() )
	{
		for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
			if ( dev_path == (DEV_MAP_PATH + dmraid_devices[k]) )
				device_found = true ;
	}

	return device_found ;
}

//FIXME:  Copy of code from FileSystem.cc.
//        This should be replaced when a general execute_command with asynchronous updates is built.
int DMRaid::execute_command( const Glib::ustring & command, OperationDetail & operationdetail ) 
{
	Glib::ustring output, error ;

	operationdetail .add_child( OperationDetail( command, STATUS_NONE, FONT_BOLD_ITALIC ) ) ;

	int exit_status = Utils::execute_command( "nice -n 19 " + command, output, error ) ;

	if ( ! output .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( output, STATUS_NONE, FONT_ITALIC ) ) ;
	
	if ( ! error .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( error, STATUS_NONE, FONT_ITALIC ) ) ;

	return exit_status ;
}

void DMRaid::get_devices( std::vector<Glib::ustring> & device_list )
{
	//Retrieve list of dmraid devices
	device_list .clear() ;

	for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
		device_list .push_back( DEV_MAP_PATH + dmraid_devices[k] ) ;
}

Glib::ustring DMRaid::get_dmraid_name( const Glib::ustring & dev_path )
{
	//Retrieve name of dmraid device
	Glib::ustring dmraid_name = "" ;
	Glib::ustring regexp = "" ;

	for ( unsigned int k=0; k < dmraid_devices .size(); k++ )
	{
		regexp = ".*(" + dmraid_devices[k] + ").*" ;
		if ( Utils::regexp_label( dev_path, regexp ) == dmraid_devices[k] )
			dmraid_name = dmraid_devices[k] ;
	}
	
	return dmraid_name ;
}

Glib::ustring DMRaid::get_dmraid_prefix( const Glib::ustring & dev_path )
{
	//Retrieve prefix of dmraid device representing fake RAID manufacturer

	//First try to get longer prefix including underscore to more accurately define the dmraid prefix
	Glib::ustring dmraid_name = get_dmraid_name( dev_path ) ;
	Glib::ustring regexp = "^([^_]*_[^_]*)" ;
	Glib::ustring dmraid_prefix = Utils::regexp_label( dmraid_name, regexp ) ;
	
	if ( dmraid_prefix .size() <= 0 )
	{
		//Retrieve shorter prefix because longer name did not work
		regexp = "^([^_]*)" ;
		dmraid_prefix = Utils::regexp_label( dmraid_name, regexp ) ;
	}

	return dmraid_prefix ;
}

void DMRaid::get_dmraid_dir_entries( const Glib::ustring & dev_path, std::vector<Glib::ustring> & dir_list )
{
	//Build list of all device entries matching device path

	Glib::ustring dmraid_name = get_dmraid_name( dev_path ) ;

	//Loop through the entries in the directory
	Glib::ustring filename = "" ;
	Glib::Dir dir( DEV_MAP_PATH) ;
	while ( ( filename = dir .read_name() ) != "" )
	{
		if ( filename == "control" )
			continue ;

		if ( Utils::regexp_label( filename, "^(" + dmraid_name + ")" ) == dmraid_name )
			dir_list .push_back( filename ) ;
	}
}

bool DMRaid::create_dev_map_entries( const Partition & partition, OperationDetail & operationdetail )
{
	//Create all missing dev mapper entries for a specified device.
	//  Try both dmraid -ay and kpartx -a

	Glib::ustring command ;
	bool exit_status = true ;	//assume success

	/*TO TRANSLATORS: looks like  create /dev/mapper entries */ 
	Glib::ustring tmp = String::ucompose ( _("create missing %1 entries"), DEV_MAP_PATH ) ;
	operationdetail .add_child( OperationDetail( tmp ) );

	Glib::ustring dmraid_prefix = get_dmraid_prefix( partition .device_path ) ;
	command = "dmraid -ay -v " + dmraid_prefix ;
	if ( execute_command( command, operationdetail .get_last_child() ) )
		exit_status = false ;	//command failed

	Glib::ustring dmraid_name = get_dmraid_name( partition .device_path ) ;
	command = "kpartx -a -v " + DEV_MAP_PATH + dmraid_name ;
	if ( execute_command( command, operationdetail .get_last_child() ) )
		exit_status = false ;	//command failed

	operationdetail .get_last_child() .set_status( exit_status ? STATUS_SUCCES : STATUS_ERROR ) ;

	return exit_status ;
}

bool DMRaid::create_dev_map_entries( const Glib::ustring & dev_path )
{
	//Create all missing dev mapper entries for a specified device.
	//  Try both dmraid -ay and kpartx -a

	Glib::ustring command, output, error ;

	Glib::ustring dmraid_prefix = get_dmraid_prefix( dev_path ) ;
	Glib::ustring dmraid_name = get_dmraid_name( dev_path ) ;

	command = "dmraid -ay -v " + dmraid_prefix + " && kpartx -a -v " + DEV_MAP_PATH + dmraid_name ;
	bool exit_status = ! Utils::execute_command( command, output, error, true ) ;

	return exit_status ;
}

void DMRaid::get_affected_dev_map_entries( const Partition & partition, std::vector<Glib::ustring> & affected_entries )
{
	//Build list of affected /dev/mapper entries when a partition is to be deleted.

	Glib::ustring dmraid_name = get_dmraid_name( partition .device_path ) ;
	Glib::ustring regexp = "^" + DEV_MAP_PATH + "(.*)$" ;
	Glib::ustring partition_name = Utils::regexp_label( partition .get_path(), regexp ) ;

	affected_entries .push_back( partition_name ) ;

	if (  partition .inside_extended )
	{
		std::vector<Glib::ustring> dir_list ;

		//Retrieve list of matching directory entries
		get_dmraid_dir_entries( partition .device_path, dir_list );

		Glib::ustring dmraid_name = get_dmraid_name( partition .device_path ) ;

		//All logical partition numbers greater than the partition to be deleted will be affected
		for ( unsigned int k=0; k < dir_list .size(); k++ )
		{
			if ( Utils::regexp_label( dir_list[k], "^(" + dmraid_name + ")" ) == dmraid_name )
			{
				if (   std::atoi( Utils::regexp_label( dir_list[k],
				                                       dmraid_name + "p?([0-9]+)"
				                                     ) .c_str()
				                ) > partition .partition_number
				   )
				affected_entries .push_back( dir_list[k] ) ;
			}
		}
	}
}

bool DMRaid::delete_affected_dev_map_entries( const Partition & partition, OperationDetail & operationdetail )
{
	//Delete all affected dev mapper entries (logical partitions >= specificied partition)

	std::vector<Glib::ustring> affected_entries ;
	Glib::ustring command ;
	bool exit_status = true ;	//assume successful

	/*TO TRANSLATORS: looks like  delete affected /dev/mapper entries */ 
	Glib::ustring tmp = String::ucompose ( _("delete affected %1 entries"), DEV_MAP_PATH ) ;
	operationdetail .add_child( OperationDetail( tmp ) );

	get_affected_dev_map_entries( partition, affected_entries ) ;

	for ( unsigned int k=0; k < affected_entries .size(); k++ )
	{
		command = "dmsetup remove " + DEV_MAP_PATH + affected_entries[k] ;
		if ( execute_command( command, operationdetail .get_last_child() ) )
			exit_status = false ;	//command failed
	}

	operationdetail .get_last_child() .set_status( exit_status ? STATUS_SUCCES : STATUS_ERROR ) ;

	return exit_status ;
}

bool DMRaid::delete_dev_map_entry( const Partition & partition, OperationDetail & operationdetail )
{
	//Delete a single dev mapper entry

	/*TO TRANSLATORS: looks like  delete /dev/mapper entry */ 
	Glib::ustring tmp = String::ucompose ( _("delete %1 entry"), DEV_MAP_PATH ) ;
	operationdetail .add_child( OperationDetail( tmp ) );

	Glib::ustring command = "dmsetup remove " + partition .get_path() ;
	bool exit_status = ! execute_command( command, operationdetail .get_last_child() ) ; 

	operationdetail .get_last_child() .set_status( exit_status ? STATUS_SUCCES : STATUS_ERROR ) ;

	return exit_status ;
}

bool DMRaid::purge_dev_map_entries( const Glib::ustring & dev_path )
{
	//Delete all dev mapper entries for dmraid device

	std::vector<Glib::ustring> dir_list ;
	Glib::ustring command ;
	Glib::ustring output, error ;
	bool exit_status = true ;	//assume successful

	get_dmraid_dir_entries( dev_path, dir_list ) ;

	for ( unsigned int k=0; k < dir_list .size(); k++ )
	{
		command = "dmsetup remove " + DEV_MAP_PATH + dir_list[k] ;
		if ( Utils::execute_command( command, output, error, true ) )
			exit_status = false ;	//command failed
	}

	return exit_status ;
}

bool DMRaid::update_dev_map_entry( const Partition & partition, OperationDetail & operationdetail )
{
	//Update dmraid entry by first deleting the entry then recreating the entry

	bool exit_status = true ;	//assume successful
	
	/*TO TRANSLATORS: looks like  update /dev/mapper entry */ 
	Glib::ustring tmp = String::ucompose ( _("update %1 entry"), DEV_MAP_PATH ) ;
	operationdetail .add_child( OperationDetail( tmp ) );

	if( ! delete_dev_map_entry( partition , operationdetail .get_last_child() ) )
		exit_status = false ;	//command failed

	if( ! create_dev_map_entries( partition , operationdetail .get_last_child() ) )
		exit_status = false ;	//command failed

	operationdetail .get_last_child() .set_status( exit_status ? STATUS_SUCCES : STATUS_ERROR ) ;

	return exit_status ;
}

}//GParted
