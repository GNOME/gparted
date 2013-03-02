/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011 Curtis Gedak
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
 
#include "../include/ext2.h"

namespace GParted
{

FS ext2::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = specific_type;

	fs .busy = FS::GPARTED ;

	// Only enable functionality if the relevant mkfs.extX command is found to ensure
	// that the version of e2fsprogs is new enough to support ext4.  Applying to
	// ext2/3 is OK as relevant mkfs.ext2/3 commands exist.  Also for ext4 only, check
	// for e4fsprogs commands to support RHEL/CentOS 5.x which uses a separate package
	// to provide ext4 support.  The existing e2fsprogs commands only support ext2/3.
	if ( ! Glib::find_program_in_path( "mkfs." + Utils::get_filesystem_string( specific_type ) ).empty() )
	{
		mkfs_cmd = "mkfs." + Utils::get_filesystem_string( specific_type );
		fs .create = FS::EXTERNAL ;
		fs .create_with_label = FS::EXTERNAL ;

		if ( specific_type == FS_EXT4 && ! Glib::find_program_in_path( "dumpe4fs" ).empty() )
			dump_cmd = "dumpe4fs";
		else if ( ! Glib::find_program_in_path( "dumpe2fs" ).empty() )
			dump_cmd = "dumpe2fs";
		if ( dump_cmd != "" )
			fs .read = FS::EXTERNAL ;

		if ( specific_type == FS_EXT4 && ! Glib::find_program_in_path( "tune4fs" ).empty() )
			tune_cmd = "tune4fs";
		else if ( ! Glib::find_program_in_path( "tune2fs" ).empty() )
			tune_cmd = "tune2fs";
		if ( tune_cmd != "" )
		{
			fs .read_uuid = FS::EXTERNAL ;
			fs .write_uuid = FS::EXTERNAL ;
		}

		if ( specific_type == FS_EXT4 && ! Glib::find_program_in_path( "e4label" ).empty() )
			label_cmd = "e4label";
		else if ( ! Glib::find_program_in_path( "e2label" ).empty() )
			label_cmd = "e2label";
		if ( label_cmd != "" )
		{
			fs .read_label = FS::EXTERNAL ;
			fs .write_label = FS::EXTERNAL ;
		}

		if ( specific_type == FS_EXT4 && ! Glib::find_program_in_path( "e4fsck" ).empty() )
			fsck_cmd = "e4fsck";
		else if ( ! Glib::find_program_in_path( "e2fsck" ).empty() )
			fsck_cmd = "e2fsck";
		if ( fsck_cmd != "" )
			fs .check = FS::EXTERNAL ;
	
		if ( specific_type == FS_EXT4 && ! Glib::find_program_in_path( "resize4fs" ).empty() )
			resize_cmd = "resize4fs";
		else if ( ! Glib::find_program_in_path( "resize2fs" ).empty() )
			resize_cmd = "resize2fs";
		if ( resize_cmd != "" && fs .check )
		{
			fs .grow = FS::EXTERNAL ;

			if ( fs .read ) //needed to determine a min file system size..
				fs .shrink = FS::EXTERNAL ;
		}

		if ( fs .check )
		{
			fs.copy = fs.move = FS::GPARTED ;

			//If supported, use e2image to copy/move the file system as it
			//  only copies used blocks, skipping unused blocks.  This is more
			//  efficient than copying all blocks used by GParted's internal
			//  method.
			if ( specific_type == FS_EXT4 && ! Glib::find_program_in_path( "e4image" ).empty() )
				image_cmd = "e4image";
			else if ( ! Glib::find_program_in_path( "e2image" ).empty() )
				image_cmd = "e2image";
			if ( image_cmd != "" )
			{
				Utils::execute_command( image_cmd, output, error, true ) ;
				if ( Utils::regexp_label( error, "(-o src_offset)" ) == "-o src_offset" )
					fs.copy = fs.move = FS::EXTERNAL ;
			}
		}

		fs .online_read = FS::EXTERNAL ;
#ifdef ENABLE_ONLINE_RESIZE
		if ( specific_type != FS_EXT2 && Utils::kernel_version_at_least( 3, 6, 0 ) )
			fs .online_grow = fs .grow ;
#endif
	}

	return fs ;
}

void ext2::set_used_sectors( Partition & partition ) 
{
	//Called when file system is unmounted *and* when mounted.  Always read
	//  the file system size from the on disk superblock using dumpe2fs to
	//  avoid overhead subtraction.  Read the free space from the kernel via
	//  the statvfs() system call when mounted and from the superblock when
	//  unmounted.
	if ( ! Utils::execute_command( dump_cmd + " -h " + partition .get_path(), output, error, true ) )
	{
		index = output .find( "Block count:" ) ;
		if ( index >= output .length() ||
		     sscanf( output.substr( index ) .c_str(), "Block count: %Ld", &T ) != 1 )
			T = -1 ;

		index = output .find( "Block size:" ) ;
		if ( index >= output.length() || 
		     sscanf( output.substr( index ) .c_str(), "Block size: %Ld", &S ) != 1 )  
			S = -1 ;

		if ( T > -1 && S > -1 )
			T = Utils::round( T * ( S / double(partition .sector_size) ) ) ;

		if ( partition .busy )
		{
			Byte_Value ignored ;
			Byte_Value fs_free ;
			if ( Utils::get_mounted_filesystem_usage( partition .get_mountpoint(),
			                                          ignored, fs_free, error ) == 0 )
				N = Utils::round( fs_free / double(partition .sector_size) ) ;
			else
			{
				N = -1 ;
				partition .messages .push_back( error ) ;
			}
		}
		else
		{
			index = output .find( "Free blocks:" ) ;
			if ( index >= output .length() ||
			     sscanf( output.substr( index ) .c_str(), "Free blocks: %Ld", &N ) != 1 )
				N = -1 ;

			if ( N > -1 && S > -1 )
				N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;
		}

		if ( T > -1 && N > -1 )
			partition .set_sector_usage( T, N ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}
	
void ext2::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( label_cmd + " " + partition .get_path(), output, error, true ) )
	{
		partition.set_filesystem_label( Utils::trim( output ) );
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool ext2::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( label_cmd + " " + partition.get_path() +
	                          " \"" + partition.get_filesystem_label() + "\"",
	                          operationdetail, EXEC_CHECK_STATUS );
}

void ext2::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( tune_cmd + " -l " + partition .get_path(), output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "^Filesystem UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool ext2::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( tune_cmd + " -U random " + partition .get_path(),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool ext2::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	sigc::connection c = signal_progress.connect( sigc::mem_fun( *this, &ext2::create_progress ) );
	bool ret = ! execute_command( mkfs_cmd + " -F -L \"" + new_partition.get_filesystem_label() + "\" " +
	                              new_partition.get_path(),
	                              operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
	c.disconnect();
	return ret;
}

bool ext2::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	Glib::ustring str_temp = resize_cmd + " -p " + partition_new .get_path() ;
	
	if ( ! fill_partition )
		str_temp += " " + Utils::num_to_str( floor( Utils::sector_to_unit(
					partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) ) + "K";

	sigc::connection c = signal_progress.connect( sigc::mem_fun( *this, &ext2::resize_progress ) );
	bool ret = ! execute_command( str_temp, operationdetail, EXEC_CHECK_STATUS );
	c.disconnect();
	return ret;
}

bool ext2::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	sigc::connection c = signal_progress.connect( sigc::mem_fun( *this, &ext2::check_repair_progress ) );
	exit_status = execute_command( fsck_cmd + " -f -y -v -C 0 " + partition.get_path(), operationdetail,
	                               EXEC_CANCEL_SAFE );
	c.disconnect();
	bool success = ( exit_status == 0 || exit_status == 1 || exit_status == 2 );
	set_status( operationdetail, success );
	return success;
}

bool ext2::move( const Partition & partition_new,
                 const Partition & partition_old,
                 OperationDetail & operationdetail )
{
	Sector distance;
	Glib::ustring offset;
	distance = partition_old.sector_start - partition_new.sector_start;
	offset = Utils::num_to_str( llabs(distance) * partition_new.sector_size );
	if ( distance < 0 )
		return ! execute_command( image_cmd + " -ra -p -o " + offset + " " + partition_new.get_path(),
		                          operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
	else
		return ! execute_command( image_cmd + " -ra -p -O " + offset + " " + partition_new.get_path(),
		                          operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );

}

bool ext2::copy( const Partition & src_part,
                 Partition & dest_part,
                 OperationDetail & operationdetail )
{
	return ! execute_command( image_cmd + " -ra -p " + src_part.get_path() + " " + dest_part.get_path(),
	                          operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE );
}

//Private methods

void ext2::resize_progress( OperationDetail *operationdetail )
{
	Glib::ustring ss;
	size_t p = output.find_last_of('\n');
	// looks like "Scanning inode table          XXXXXXXXXXXXXXXXXXXXXXXXXXXX------------"
	if ( p == output.npos )
		return;
	ss = output.substr( p );
	if ( ss.empty() )
		return;
	size_t sslen = ss.length();
	if ( ss[sslen-1] != 'X' && ss[sslen-1] != '-' )
		return;
	// p = Start of progress bar
	p = ss.find_last_not_of( "X-" );
	if ( p == ss.npos )
		p = 0;
	else
		p++;
	size_t barlen = sslen - p;
	// q = First dash in progress bar or end of string
	size_t q = ss.find( '-', p );
	if ( q == ss.npos )
		q = sslen;
	size_t xlen = q - p;
	operationdetail->fraction = (double)xlen / barlen;
	operationdetail->signal_update( *operationdetail );
}

void ext2::create_progress( OperationDetail *operationdetail )
{
	Glib::ustring ss;
	size_t p = output.find_last_of('\n');
	// looks like "Writing inode tables: xx/yy"
	if ( p == output.npos )
		return;
	ss = output.substr( p );
	int x, y;
	if ( sscanf( ss.c_str(), "\nWriting inode tables: %d/%d", &x, &y ) == 2 )
	{
		operationdetail->fraction = (double)x / y;
		operationdetail->signal_update( *operationdetail );
	}
}

void ext2::check_repair_progress( OperationDetail *operationdetail )
{
	Glib::ustring ss;
	size_t p = output.find_last_of('\n');
	// looks like "/dev/loop0p1: |==============================================  \ 95.1%"
	if ( p == output.npos )
		return;
	ss = output.substr( p );
	p = ss.find_last_of('%');
	if ( p == ss.npos )
		return;
	ss = ss.substr( p-5, p );
	float frac;
	if ( sscanf( ss.c_str(), "%f%%", &frac ) == 1 )
	{
		operationdetail->fraction = frac / 100;
		operationdetail->signal_update( *operationdetail );
	}
}

} //GParted
