/* Copyright (C) 2004, 2005, 2006, 2007, 2008 Bart Hakvoort
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
	
	//resizing of xfs requires xfs_growfs, xfs_repair, mount, umount and xfs support in the kernel
	if ( ! Glib::find_program_in_path( "xfs_growfs" ) .empty() &&
	     ! Glib::find_program_in_path( "mount" ) .empty() &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
    	     fs .check ) 	
	{
		Glib::ustring line ;
		std::ifstream input( "/proc/filesystems" ) ;
		while ( input >> line )
			if ( line == "xfs" )
			{
				fs .grow = GParted::FS::EXTERNAL ;
				break ;
			}
		
		input .close( ) ;
	}
	
	if ( ! Glib::find_program_in_path( "xfsdump" ) .empty() &&
	     ! Glib::find_program_in_path( "xfsrestore" ) .empty() &&
	     ! Glib::find_program_in_path( "mount" ) .empty() &&
	     ! Glib::find_program_in_path( "umount" ) .empty() &&
	     fs .check && fs .create ) 	
		fs .copy = GParted::FS::EXTERNAL ;
	
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
			partition .Set_Unused( Utils::round( N * ( S / 512.0 ) ) ) ;
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
	//mkfs.xfs will not create filesystem if label is longer than 12 characters, hence truncation.
	Glib::ustring label = new_partition .label ;
	if( label .length() > 12 )
		label = label.substr( 0, 12 ) ;
	return ! execute_command( "mkfs.xfs -f -L \"" + label + "\" " + new_partition .get_path(), operationdetail ) ;
}

bool xfs::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	bool return_value = false ;
	Glib::ustring error ;
	Glib::ustring TEMP_MP = Glib::get_tmp_dir() + "/gparted_tmp_xfs_mountpoint" ;

	//create mountpoint...
	operationdetail .add_child( OperationDetail( String::ucompose( _("create temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
	if ( ! mkdir( TEMP_MP .c_str(), 0 ) )
	{
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		
		//mount partition
		operationdetail .add_child(
			OperationDetail( String::ucompose( _("mount %1 on %2"), partition_new .get_path(), TEMP_MP ) ) ) ;

		if ( ! execute_command( "mount -v -t xfs " + partition_new .get_path() + " " + TEMP_MP,
					operationdetail .get_last_child() ) )
		{
			operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
			
			//grow the mounted filesystem..
			operationdetail .add_child( OperationDetail( _("grow mounted filesystem") ) ) ;
			
			if ( ! execute_command ( "xfs_growfs " + TEMP_MP, operationdetail .get_last_child() ) )
			{
				operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
				return_value = true ;
			}
			else
			{
				operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
			}
			
			//and unmount it...
			operationdetail .add_child(
				OperationDetail( String::ucompose( _("unmount %1"), partition_new .get_path() ) ) ) ;

			if ( ! execute_command( "umount -v " + partition_new .get_path(),
						operationdetail .get_last_child() ) )
			{
				operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
			}
			else
			{
				operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
				return_value = false ;
			}
		}
		else
		{
			operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
		}
				
		//remove the mountpoint..
		operationdetail .add_child(
			OperationDetail( String::ucompose( _("remove temporary mountpoint (%1)"), TEMP_MP ) ) ) ;
		if ( ! rmdir( TEMP_MP .c_str() ) )
		{
			operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		}
		else
		{
			operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
			operationdetail .get_last_child() .add_child(
				OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;

			return_value = false ;
		}
	}
	else
	{
		operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
		operationdetail .get_last_child() .add_child( OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;
	}
	
	return return_value ;
}

bool xfs::copy( const Glib::ustring & src_part_path,
		const Glib::ustring & dest_part_path,
		OperationDetail & operationdetail )
{
	bool return_value = false ;
	Glib::ustring error ;
	Glib::ustring SRC = Glib::get_tmp_dir() + "/gparted_tmp_xfs_src_mountpoint" ;
	Glib::ustring DST = Glib::get_tmp_dir() + "/gparted_tmp_xfs_dest_mountpoint" ;
	
	//create xfs filesystem on destination..
	/*TO TRANSLATORS: looks like Create new xfs filesystem */ 
	operationdetail .add_child( OperationDetail( 
		String::ucompose( _("create new %1 filesystem"), Utils::get_filesystem_string( FS_XFS ) ) ) ) ;

	if ( create( Partition( dest_part_path ), operationdetail .get_last_child() ) )
	{
		operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
		
		//create source mountpoint...
		operationdetail .add_child( 
			OperationDetail( String::ucompose( _("create temporary mountpoint (%1)"), SRC ) ) ) ;
		if ( ! mkdir( SRC .c_str(), 0 ) )
		{
			operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
			
			//create destination mountpoint...
			operationdetail .add_child(
				OperationDetail( String::ucompose( _("create temporary mountpoint (%1)"), DST ) ) ) ;
			if ( ! mkdir( DST .c_str(), 0 ) )
			{
				operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;

				//mount source partition
				operationdetail .add_child(
					OperationDetail( String::ucompose( _("mount %1 on %2"), src_part_path, SRC ) ) ) ;
				
				if ( ! execute_command( "mount -v -t xfs -o noatime,ro " + src_part_path + " " + SRC,
							operationdetail .get_last_child() ) )
				{
					operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;

					//mount destination partition
					operationdetail .add_child(
					OperationDetail( String::ucompose( _("mount %1 on %2"), dest_part_path, DST ) ) ) ;
			
					if ( ! execute_command( "mount -v -t xfs " + dest_part_path + " " + DST,
								operationdetail .get_last_child() ) )
					{
						operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
	
						//copy filesystem..
						operationdetail .add_child( OperationDetail( _("copy filesystem") ) ) ;
						
						if ( ! execute_command( 
							 "xfsdump -J - " + SRC + " | xfsrestore -J - " + DST,
							 operationdetail .get_last_child() ) )
						{
							operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
							return_value = true ;
						}
						else
						{
							operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
						}
						
						//unmount destination partition
						operationdetail .add_child(
							OperationDetail( String::ucompose( _("unmount %1"),
											    dest_part_path ) ) ) ;
					
						if ( ! execute_command( "umount -v  " + dest_part_path,
									operationdetail .get_last_child() ) )
						{
							operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
						}
						else
						{
							operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
							return_value = false ;
						}
					}
					else
					{
						operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
					}
					
					//unmount source partition
					operationdetail .add_child(
						OperationDetail( String::ucompose( _("unmount %1"), src_part_path ) ) ) ;
				
					if ( ! execute_command( "umount -v  " + src_part_path,
								operationdetail .get_last_child() ) )
					{
						operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
					}
					else
					{
						operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
						return_value = false ;
					}
				}
				else
				{
					operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
				}
		
				//remove destination mountpoint..
				operationdetail .add_child(
					OperationDetail( String::ucompose( _("remove temporary mountpoint (%1)"), DST ) ) ) ;
				if ( ! rmdir( DST .c_str() ) )
				{
					operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
				}
				else
				{
					operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
					operationdetail .get_last_child() .add_child(
						OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;
	
					return_value = false ;
				}
			}
			else
			{
				operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
				operationdetail .get_last_child() .add_child(
					OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;
			}
			
			//remove source mountpoint..
			operationdetail .add_child(
				OperationDetail( String::ucompose( _("remove temporary mountpoint (%1)"), SRC ) ) ) ;
			if ( ! rmdir( SRC .c_str() ) )
			{
				operationdetail .get_last_child() .set_status( STATUS_SUCCES ) ;
			}
			else
			{
				operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
				operationdetail .get_last_child() .add_child(
					OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;

				return_value = false ;
			}
		}
		else
		{
			operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;
			operationdetail .get_last_child() .add_child(
				OperationDetail( Glib::strerror( errno ), STATUS_NONE ) ) ;
		}
	}
	else 
		operationdetail .get_last_child() .set_status( STATUS_ERROR ) ;

	return return_value ;
}

bool xfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "xfs_repair -v " + partition .get_path(), operationdetail ) ;
}

} //GParted


