/* Copyright (C) 2004 Bart
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
 
 
#include "../include/ntfs.h"

namespace GParted
{

FS ntfs::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = GParted::FS_NTFS ;
	if ( ! system( "which ntfscluster 1>/dev/null 2>/dev/null" ) ) 
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkntfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which ntfsfix 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing is a delicate process ...
	if ( ! system( "which ntfsresize 1>/dev/null 2>/dev/null" ) && fs .check ) 
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}
	
	//we need ntfsresize to set correct used/unused after cloning
	if ( ! system( "which ntfsclone 1>/dev/null 2>/dev/null" ) && fs .grow ) 
		fs .copy = GParted::FS::EXTERNAL ;
	
	return fs ;
}

void ntfs::Set_Used_Sectors( Partition & partition ) 
{
	argv .push_back( "ntfscluster" ) ;
	argv .push_back( "--force" ) ;
	argv .push_back( partition .partition ) ;

	envp .push_back( "LC_ALL=C" ) ;
	
	try
	{
		Glib::spawn_sync( ".", argv, envp, Glib::SPAWN_SEARCH_PATH, sigc::slot< void >(), &output ) ;
	}
	catch ( Glib::Exception & e )
	{ 
		std::cout << e .what() << std::endl ;
		return ;
	} 

	index = output .find( "sectors of free space" ) ;
	if ( index >= output .length() || sscanf( output .substr( index ) .c_str(), "sectors of free space : %Ld", &N ) != 1 ) 
		N = -1 ;

	if ( N > -1 )
		partition .Set_Unused( N ) ;
}

bool ntfs::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::Get_Filesystem_String( GParted::FS_NTFS ) ) ) ) ;
	argv .clear() ;
	argv .push_back( "mkntfs" ) ;
	argv .push_back( "-Q" ) ;
	argv .push_back( "-vv" ) ;
	argv .push_back( new_partition .partition ) ;
	if ( ! execute_command( argv, operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

bool ntfs::Resize( const Partition & partition_new, 
		   std::vector<OperationDetails> & operation_details,
		   bool fill_partition )
{//FIXME probeer dmv piping y the echoen in ntfsresize
	//-contact szaka en probeer een --yes oid in de API te krijgen
	//-perform eerst testruns e.d. in ntfsresize te bouwen..
	if ( fill_partition )
		operation_details .push_back( OperationDetails( _("grow filesystem to fill the partition") ) ) ;
	else
		operation_details .push_back( OperationDetails( _("resize the filesystem") ) ) ;
	
	Glib::ustring str_temp = "echo y | ntfsresize -f " + partition_new .partition ;
	
	if ( ! fill_partition )
		str_temp += " -s " + Utils::num_to_str( partition_new .Get_Length_MB( ) - cylinder_size, true ) + "M" ;
	
	if ( ! Execute_Command( str_temp ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

bool ntfs::Copy( const Glib::ustring & src_part_path,
		 const Glib::ustring & dest_part_path, 
		 std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("copy contents of %1 to %2"), src_part_path, dest_part_path ) ) ) ;
	
	argv .clear() ;
	argv .push_back( "ntfsclone" ) ;
	argv .push_back( "-f" ) ;
	argv .push_back( "--overwrite" ) ;
	argv .push_back( dest_part_path ) ;
	argv .push_back( src_part_path ) ;

	if ( ! execute_command( argv, operation_details .back() .sub_details ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
	
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, operation_details, true ) ;
	}
		
	operation_details .back() .status = OperationDetails::ERROR ;
	return false ;
}

bool ntfs::Check_Repair( const Partition & partition, std::vector<OperationDetails> & operation_details )
{
	//according to Szaka it's best to use ntfsresize to check the partition for errors
	//since --info is read-only i'll leave it out. just calling ntfsresize --force has also a tendency of fixing stuff :)
	operation_details .push_back( OperationDetails( _("check filesystem for errors and (if possible) fix them") ) ) ;
//FIXME.. ook hier kan eea verbeterd/verduidelijkt..	
	if ( Resize( partition, operation_details .back() .sub_details, true ) )
	{
		operation_details .back() .status = OperationDetails::SUCCES ;
		return true ;
	}
	else
	{
		operation_details .back() .status = OperationDetails::ERROR ;
		return false ;
	}
}

} //GParted


