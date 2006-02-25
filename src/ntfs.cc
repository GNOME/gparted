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

FS ntfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_NTFS ;
	
	if ( ! Glib::find_program_in_path( "ntfscluster" ) .empty() )
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "mkntfs" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "ntfsfix" ) .empty() )
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing is a delicate process ...
	if ( ! Glib::find_program_in_path( "ntfsresize" ) .empty() && fs .check )
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}
	
	//we need ntfsresize to set correct used/unused after cloning
	if ( ! Glib::find_program_in_path( "ntfsclone" ) .empty() && fs .grow )
		fs .copy = GParted::FS::EXTERNAL ;
	
	return fs ;
}

void ntfs::Set_Used_Sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( "ntfscluster --force " + partition .partition, output, error, true ) )
	{
		index = output .find( "sectors of free space" ) ;
		if ( index >= output .length() || 
		     sscanf( output .substr( index ) .c_str(), "sectors of free space : %Ld", &N ) != 1 ) 
			N = -1 ;

		if ( N > -1 )
			partition .Set_Unused( N ) ;
	}
	else
		partition .error = error ;
}

bool ntfs::Create( const Partition & new_partition, std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( String::ucompose(
								_("create new %1 filesystem"),
								Utils::Get_Filesystem_String( GParted::FS_NTFS ) ) ) ) ;
	
	if ( ! execute_command( "mkntfs -Q -vv " + new_partition .partition, operation_details .back() .sub_details ) )
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
{
	if ( fill_partition )
		operation_details .push_back( OperationDetails( _("grow filesystem to fill the partition") ) ) ;
	else
		operation_details .push_back( OperationDetails( _("resize the filesystem") ) ) ;
	
	bool return_value = false ;
	Glib::ustring str_temp = "ntfsresize -P --force --force " + partition_new .partition ;
	
	if ( ! fill_partition )
	{
		str_temp += " -s " ;
		str_temp += Utils::num_to_str( Utils::Round( Utils::sector_to_unit(
				partition_new .get_length() - cylinder_size, GParted::UNIT_BYTE ) ), true ) ;
	}
	
	//simulation..
	operation_details .back() .sub_details .push_back( OperationDetails( _("run simulation") ) ) ;

	if ( ! execute_command( str_temp + " --no-action", operation_details .back() .sub_details .back() .sub_details ) )
	{
		operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;

		//real resize (use description from 'main' operation)
		operation_details .back() .sub_details .push_back( 
			OperationDetails( operation_details .back() .description ) ) ;

		if ( ! execute_command( str_temp, operation_details .back() .sub_details .back() .sub_details ) )
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::SUCCES ;
			return_value = true ;
		}
		else
		{
			operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
		}
	}
	else
	{
		operation_details .back() .sub_details .back() .status = OperationDetails::ERROR ;
	}
	
	operation_details .back() .status = return_value ? OperationDetails::SUCCES : OperationDetails::ERROR ;
	return return_value ;
}

bool ntfs::Copy( const Glib::ustring & src_part_path,
		 const Glib::ustring & dest_part_path, 
		 std::vector<OperationDetails> & operation_details )
{
	operation_details .push_back( OperationDetails( 
				String::ucompose( _("copy contents of %1 to %2"), src_part_path, dest_part_path ) ) ) ;
	
	if ( ! execute_command( "ntfsclone -f --overwrite " + dest_part_path + " " + src_part_path,
				 operation_details .back() .sub_details ) )
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
	operation_details .push_back( OperationDetails( _("check filesystem for errors and (if possible) fix them") ) ) ;

	if ( ! execute_command( "ntfsresize -i -f -v " + partition .partition, operation_details .back() .sub_details ) ) 
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


