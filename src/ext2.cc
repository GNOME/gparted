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

#include "ext2.h"
#include "FileSystem.h"
#include "OperationDetail.h"
#include "Partition.h"
#include "Utils.h"

#include <glibmm/ustring.h>

namespace GParted
{

FS ext2::get_filesystem_support()
{
	FS fs( specific_type );

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

		// Determine mkfs.ext4 version specific capabilities.
		bool have_64bit_feature = false;
		force_auto_64bit = false;
		if ( specific_type == FS_EXT4 )
		{
			Utils::execute_command( mkfs_cmd + " -V", output, error, true );
			int mke4fs_major_ver = 0;
			int mke4fs_minor_ver = 0;
			int mke4fs_patch_ver = 0;
			if ( sscanf( error.c_str(), "mke%*[24]fs %d.%d.%d",
			             &mke4fs_major_ver, &mke4fs_minor_ver, &mke4fs_patch_ver ) >= 2 )
			{
				// Ext4 64bit feature was added in e2fsprogs 1.42, but
				// only enable large volumes from 1.42.9 when a large
				// number of 64bit bugs were fixed.
				// *   Release notes, E2fsprogs 1.42 (November 29, 2011)
				//     http://e2fsprogs.sourceforge.net/e2fsprogs-release.html#1.42
				// *   Release notes, E2fsprogs 1.42.9 (December 28, 2013)
				//     http://e2fsprogs.sourceforge.net/e2fsprogs-release.html#1.42.9
				have_64bit_feature =    ( mke4fs_major_ver > 1 )
				                     || ( mke4fs_major_ver == 1 && mke4fs_minor_ver > 42 )
				                     || ( mke4fs_major_ver == 1 && mke4fs_minor_ver == 42 && mke4fs_patch_ver >= 9 );

				// (#766910) E2fsprogs 1.43 creates 64bit ext4 file
				// systems by default.  RHEL/CentOS 7 configured e2fsprogs
				// 1.42.9 to create 64bit ext4 file systems by default.
				// Theoretically this can be done when 64bit feature was
				// added in e2fsprogs 1.42.  GParted will re-implement the
				// removed mke2fs.conf(5) auto_64-bit_support option to
				// avoid the issues with multiple boot loaders not working
				// with 64bit ext4 file systems.
				force_auto_64bit =    ( mke4fs_major_ver > 1 )
				                   || ( mke4fs_major_ver == 1 && mke4fs_minor_ver >= 42 );
			}
		}

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

		// Maximum size of an ext2/3/4 volume is 2^32 - 1 blocks, except for ext4
		// with 64bit feature.  That is just under 16 TiB with a 4K block size.
		// *   Ext4 Disk Layout, Blocks
		//     https://ext4.wiki.kernel.org/index.php/Ext4_Disk_Layout#Blocks
		// FIXME: Rounding down to whole MiB here should not be necessary.  The
		// Copy, New and Resize/Move dialogs should limit FS correctly without
		// this.  See bug #766910 comment #12 onwards for further discussion.
		//     https://bugzilla.gnome.org/show_bug.cgi?id=766910#c12
		if ( specific_type == FS_EXT2                             ||
		     specific_type == FS_EXT3                             ||
		     ( specific_type == FS_EXT4 && ! have_64bit_feature )    )
			fs_limits.max_size = Utils::floor_size( 16 * TEBIBYTE - 4 * KIBIBYTE, MEBIBYTE );
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
	if ( ! Utils::execute_command( dump_cmd + " -h " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                          )
	{
		Glib::ustring::size_type index = output.find( "Block count:" );
		if ( index >= output .length() ||
		     sscanf( output.substr( index ).c_str(), "Block count: %lld", &T ) != 1 )
			T = -1 ;

		index = output .find( "Block size:" ) ;
		if ( index >= output.length() || 
		     sscanf( output.substr( index ).c_str(), "Block size: %lld", &S ) != 1 )
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
				partition.push_back_message( error );
			}
		}
		else
		{
			index = output .find( "Free blocks:" ) ;
			if ( index >= output .length() ||
			     sscanf( output.substr( index ).c_str(), "Free blocks: %lld", &N ) != 1 )
				N = -1 ;

			if ( N > -1 && S > -1 )
				N = Utils::round( N * ( S / double(partition .sector_size) ) ) ;
		}

		if ( T > -1 && N > -1 && S > -1 )
		{
			partition .set_sector_usage( T, N ) ;
			partition.fs_block_size = S;
		}
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}
	
void ext2::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( label_cmd + " " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                        )
	{
		partition.set_filesystem_label( Utils::trim( output ) );
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );
		
		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool ext2::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( label_cmd + " " + Glib::shell_quote( partition.get_path() ) +
	                          " " + Glib::shell_quote( partition.get_filesystem_label() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

void ext2::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( tune_cmd + " -l " + Glib::shell_quote( partition.get_path() ),
	                               output, error, true )                                          )
	{
		partition .uuid = Utils::regexp_label( output, "^Filesystem UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition.push_back_message( output );

		if ( ! error .empty() )
			partition.push_back_message( error );
	}
}

bool ext2::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( tune_cmd + " -U random " + Glib::shell_quote( partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool ext2::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring features;
	if ( force_auto_64bit )
	{
		// (#766910) Manually implement mke2fs.conf(5) auto_64-bit_support option
		// by setting or clearing the 64bit feature on the command line depending
		// of the partition size.
		if ( new_partition.get_byte_length() >= 16 * TEBIBYTE )
			features = " -O 64bit";
		else
			features = " -O ^64bit";
	}
	return ! execute_command( mkfs_cmd + " -F" + features +
	                          " -L " + Glib::shell_quote( new_partition.get_filesystem_label() ) +
	                          " " + Glib::shell_quote( new_partition.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE|EXEC_PROGRESS_STDOUT,
	                          static_cast<StreamSlot>( sigc::mem_fun( *this, &ext2::create_progress ) ) );
}

bool ext2::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	Glib::ustring str_temp = resize_cmd + " -p " + Glib::shell_quote( partition_new.get_path() );
	
	if ( ! fill_partition )
		str_temp += " " + Utils::num_to_str( floor( Utils::sector_to_unit(
					partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) ) + "K";

	return ! execute_command( str_temp, operationdetail, EXEC_CHECK_STATUS|EXEC_PROGRESS_STDOUT,
	                          static_cast<StreamSlot>( sigc::mem_fun( *this, &ext2::resize_progress ) ) );
}

bool ext2::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	exit_status = execute_command( fsck_cmd + " -f -y -v -C 0 " + Glib::shell_quote( partition.get_path() ),
	                               operationdetail, EXEC_CANCEL_SAFE|EXEC_PROGRESS_STDOUT,
	                               static_cast<StreamSlot>( sigc::mem_fun( *this, &ext2::check_repair_progress ) ) );
	bool success = ( exit_status == 0 || exit_status == 1 || exit_status == 2 );
	set_status( operationdetail, success );
	return success;
}

bool ext2::move( const Partition & partition_new,
                 const Partition & partition_old,
                 OperationDetail & operationdetail )
{
	Sector distance = partition_old.sector_start - partition_new.sector_start;
	Glib::ustring offset = Utils::num_to_str( llabs(distance) * partition_new.sector_size );
	Glib::ustring cmd;
	if ( distance < 0 )
		cmd = image_cmd + " -ra -p -o " + offset + " " + Glib::shell_quote( partition_new.get_path() );
	else
		cmd = image_cmd + " -ra -p -O " + offset + " " + Glib::shell_quote( partition_new.get_path() );

	fs_block_size = partition_old.fs_block_size;
	return ! execute_command( cmd, operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE|EXEC_PROGRESS_STDERR,
	                          static_cast<StreamSlot>( sigc::mem_fun( *this, &ext2::copy_progress ) ) );
}

bool ext2::copy( const Partition & src_part,
                 Partition & dest_part,
                 OperationDetail & operationdetail )
{
	fs_block_size = src_part.fs_block_size;
	return ! execute_command( image_cmd + " -ra -p " + Glib::shell_quote( src_part.get_path() ) +
	                          " " + Glib::shell_quote( dest_part.get_path() ),
	                          operationdetail, EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE|EXEC_PROGRESS_STDERR,
	                          static_cast<StreamSlot>( sigc::mem_fun( *this, &ext2::copy_progress ) ) );
}

//Private methods

void ext2::resize_progress( OperationDetail *operationdetail )
{
	Glib::ustring line = Utils::last_line( output );
	size_t llen = line.length();
	// There may be multiple text progress bars on subsequent last lines which look
	// like: "Scanning inode table          XXXXXXXXXXXXXXXXXXXXXXXXXXXX------------"
	if ( llen > 0 && ( line[llen-1] == 'X' || line[llen-1] == '-' ) )
	{
		// p = Start of progress bar
		size_t p = line.find_last_not_of( "X-" );
		if ( p == line.npos )
			p = 0;
		else
			p++;
		size_t barlen = llen - p;

		// q = First dash in progress bar or end of string
		size_t q = line.find( '-', p );
		if ( q == line.npos )
			q = llen;
		size_t xlen = q - p;

		operationdetail->run_progressbar( (double)xlen, (double)barlen );
	}
	// Ending summary line looks like:
	// "The filesystem on /dev/sdb3 is now 256000 block long."
	else if ( output.find( " is now " ) != output.npos )
	{
		operationdetail->stop_progressbar();
	}
}

void ext2::create_progress( OperationDetail *operationdetail )
{
	Glib::ustring line = Utils::last_line( output );
	// Text progress on the LAST LINE looks like "Writing inode tables:  105/1600"
	long long progress, target;
	if ( sscanf( line.c_str(), "Writing inode tables: %lld/%lld", &progress, &target ) == 2 )
	{
		operationdetail->run_progressbar( (double)progress, (double)target );
	}
	// Or when finished, on any line, ...
	else if ( output.find( "Writing inode tables: done" ) != output.npos )
	{
		operationdetail->stop_progressbar();
	}
}

void ext2::check_repair_progress( OperationDetail *operationdetail )
{
	Glib::ustring line = Utils::last_line( output );
	// Text progress on the LAST LINE looks like
	// "/dev/sdd3: |=====================================================   \ 95.1%   "
	size_t p = line.rfind( "%" );
	Glib::ustring pct;
	if ( p != line.npos && p >= 5 )
		pct = line.substr( p-5, 6 );
	float progress;
	if ( line.find( ": |" ) != line.npos && sscanf( pct.c_str(), "%f", &progress ) == 1 )
	{
		operationdetail->run_progressbar( progress, 100.0 );
	}
	// Only allow stopping the progress bar after seeing "non-contiguous" in the
	// summary at the end to prevent the GUI progress bar flashing back to pulsing
	// mode when the text progress bar is temporarily missing/incomplete before fsck
	// output is fully updated when switching from one pass to the next.
	else if ( output.find( "non-contiguous" ) != output.npos )
	{
		operationdetail->stop_progressbar();
	}
}

void ext2::copy_progress( OperationDetail *operationdetail )
{
	Glib::ustring line = Utils::last_line( error );
	// Text progress on the LAST LINE of STDERR looks like "Copying 146483 / 258033 blocks ..."
	long long progress, target;
	if ( sscanf( line.c_str(), "Copying %lld / %lld blocks", &progress, &target ) == 2 )
	{
		operationdetail->run_progressbar( (double)(progress * fs_block_size),
		                                  (double)(target * fs_block_size),
		                                  PROGRESSBAR_TEXT_COPY_BYTES );
	}
	// Or when finished, on any line of STDERR, looks like "Copied 258033 / 258033 blocks ..."
	else if ( error.find( "\nCopied " ) != error.npos )
	{
		operationdetail->stop_progressbar();
	}
}

} //GParted
