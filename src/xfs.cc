/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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
 
 
#include "../include/xfs.h"

#include <cerrno>

namespace GParted
{

FS xfs::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_XFS ;
	
	if ( ! Glib::find_program_in_path( "xfs_db" ) .empty() ) 	
	{
		fs .read = GParted::FS::EXTERNAL ;
		fs .read_label = FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "xfs_admin" ) .empty() ) 	
		fs .write_label = FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "mkfs.xfs" ) .empty() ) 	
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "xfs_repair" ) .empty() ) 	
		fs .check = GParted::FS::EXTERNAL ;
	
	//Mounted operations require mount, umount and xfs support in the kernel
	if ( ! Glib::find_program_in_path( "mount" ) .empty()  &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
	     fs .check                                         &&
	     Utils::kernel_supports_fs( "xfs" )                   )
	{
		//Grow
		if ( ! Glib::find_program_in_path( "xfs_growfs" ) .empty() )
			fs .grow = FS::EXTERNAL ;

		//Copy using xfsdump, xfsrestore
		if ( ! Glib::find_program_in_path( "xfsdump" ) .empty()    &&
		     ! Glib::find_program_in_path( "xfsrestore" ) .empty() &&
		     fs .create                                               )
			fs .copy = FS::EXTERNAL ;
	}

	if ( fs .check )
		fs .move = GParted::FS::GPARTED ;

	fs .MIN = 32 * MEBIBYTE ;//official minsize = 16MB, but the smallest xfs_repair can handle is 32MB...
	
	return fs ;
}

void xfs::set_used_sectors( Partition & partition ) 
{
	if ( ! Utils::execute_command( 
			"xfs_db -c 'sb 0' -c 'print blocksize' -c 'print fdblocks' -r " + partition .get_path(),
			output,
			error,
			true ) )
	{
		//blocksize
		if ( sscanf( output .c_str(), "blocksize = %Ld", &S ) != 1 )
			S = -1 ;

		//free blocks
		index = output .find( "fdblocks" ) ;
		if ( index > output .length() ||
		     sscanf( output .substr( index ) .c_str(), "fdblocks = %Ld", &N ) != 1 )
			N = -1 ;

		if ( N > -1 && S > -1 )
			partition .Set_Unused( Utils::round( N * ( S / double(partition .sector_size) ) ) ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

void xfs::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "xfs_db -r -c 'label' " + partition .get_path(), output, error, true ) )
	{
		partition .label = Utils::regexp_label( output, "^label = \"(.*)\"" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool xfs::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "" ;
	if( partition .label .empty() )
		cmd = String::ucompose( "xfs_admin -L -- %1", partition .get_path() ) ;
	else
		cmd = String::ucompose( "xfs_admin -L \"%1\" %2", partition .label, partition .get_path() ) ;
	return ! execute_command( cmd, operationdetail ) ;
}

bool xfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	//mkfs.xfs will not create file system if label is longer than 12 characters, hence truncation.
	Glib::ustring label = new_partition .label ;
	if( label .length() > 12 )
		label = label.substr( 0, 12 ) ;
	return ! execute_command( "mkfs.xfs -f -L \"" + label + "\" " + new_partition .get_path(), operationdetail ) ;
}

bool xfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool success = true ;

	Glib::ustring mount_point = mk_temp_dir( "", operationdetail ) ;
	if ( mount_point .empty() )
		return false ;

	success &= ! execute_command_timed( "mount -v -t xfs " + partition_new .get_path() + " " + mount_point,
	                                    operationdetail ) ;

	if ( success )
	{
		success &= ! execute_command_timed( "xfs_growfs " + mount_point, operationdetail ) ;

		success &= ! execute_command_timed( "umount -v " + mount_point, operationdetail ) ;
	}

	rm_temp_dir( mount_point, operationdetail ) ;

	return success ;
}

bool xfs::move( const Partition & partition_new
              , const Partition & partition_old
              , OperationDetail & operationdetail
              )
{
	return true ;
}

bool xfs::copy( const Glib::ustring & src_part_path,
		const Glib::ustring & dest_part_path,
		OperationDetail & operationdetail )
{
	bool success = true ;

	success &= ! execute_command_timed( "mkfs.xfs -f " + dest_part_path, operationdetail ) ;
	if ( ! success )
		return false ;

	Glib::ustring src_mount_point = mk_temp_dir( "src", operationdetail ) ;
	if ( src_mount_point .empty() )
		return false ;

	Glib::ustring dest_mount_point = mk_temp_dir( "dest", operationdetail ) ;
	if ( dest_mount_point .empty() )
	{
		rm_temp_dir( src_mount_point, operationdetail ) ;
		return false ;
	}

	success &= ! execute_command_timed( "mount -v -t xfs -o noatime,ro " + src_part_path +
	                                    " " + src_mount_point, operationdetail ) ;

	if ( success )
	{
		success &= ! execute_command_timed( "mount -v -t xfs " + dest_part_path +
		                                    " " + dest_mount_point, operationdetail ) ;

		if ( success )
		{
			success &= ! execute_command_timed( "xfsdump -J - " + src_mount_point +
			                                    " | xfsrestore -J - " + dest_mount_point,
			                                    operationdetail ) ;

			success &= ! execute_command_timed( "umount -v " + dest_part_path, operationdetail ) ;
		}

		success &= ! execute_command_timed( "umount -v " + src_part_path, operationdetail ) ;
	}

	rm_temp_dir( dest_mount_point, operationdetail ) ;

	rm_temp_dir( src_mount_point, operationdetail ) ;

	return success ;
}

bool xfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "xfs_repair -v " + partition .get_path(), operationdetail ) ;
}

} //GParted


