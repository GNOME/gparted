/* Copyright (C) 2004 Bart 'plors' Hakvoort
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Curtis Gedak
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

#include "Win_GParted.h"
#include "GParted_Core.h"
#include "CopyBlocks.h"
#include "BlockSpecial.h"
#include "DMRaid.h"
#include "FileSystem.h"
#include "FS_Info.h"
#include "LVM2_PV_Info.h"
#include "LUKS_Info.h"
#include "Mount_Info.h"
#include "Operation.h"
#include "OperationCopy.h"
#include "Partition.h"
#include "PartitionLUKS.h"
#include "PartitionVector.h"
#include "Proc_Partitions_Info.h"
#include "SWRaid_Info.h"
#include "Utils.h"

#include "btrfs.h"
#include "exfat.h"
#include "ext2.h"
#include "f2fs.h"
#include "fat16.h"
#include "linux_swap.h"
#include "lvm2_pv.h"
#include "luks.h"
#include "reiserfs.h"
#include "nilfs2.h"
#include "ntfs.h"
#include "xfs.h"
#include "jfs.h"
#include "hfs.h"
#include "hfsplus.h"
#include "reiser4.h"
#include "udf.h"
#include "ufs.h"

#include <parted/parted.h>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtkmm/messagedialog.h>
#include <gtkmm/main.h>

std::vector<Glib::ustring> libparted_messages ; //see ped_exception_handler()

namespace GParted
{

const std::time_t SETTLE_DEVICE_PROBE_MAX_WAIT_SECONDS = 1;
const std::time_t SETTLE_DEVICE_APPLY_MAX_WAIT_SECONDS = 10;

static bool udevadm_found = false;
static bool udevsettle_found = false;
static bool hdparm_found = false;

static const Glib::ustring GPARTED_BUG( _("GParted Bug") );

GParted_Core::GParted_Core() 
{
	thread_status_message = "" ;

	ped_exception_set_handler( ped_exception_handler ) ; 

	//get valid flags ...
	for ( PedPartitionFlag flag = ped_partition_flag_next( static_cast<PedPartitionFlag>( NULL ) ) ;
	      flag ;
	      flag = ped_partition_flag_next( flag ) )
		flags .push_back( flag ) ;
	
	//throw libpartedversion to the stdout to see which version is actually used.
	std::cout << "======================" << std::endl ;
	std::cout << "libparted : " << ped_get_version() << std::endl ;
	std::cout << "======================" << std::endl ;

	find_supported_core();

	//initialize file system list
	init_filesystems() ;

	//Determine file system support capabilities for the first time
	find_supported_filesystems() ;
}

void GParted_Core::find_supported_core()
{
	udevadm_found = ! Glib::find_program_in_path( "udevadm" ).empty();
	udevsettle_found = ! Glib::find_program_in_path( "udevsettle" ).empty();
	hdparm_found = ! Glib::find_program_in_path( "hdparm" ).empty();
}

void GParted_Core::find_supported_filesystems()
{
	std::map< FSType, FileSystem * >::iterator f;

	// Iteration of std::map is ordered according to operator< of the key.  Hence the
	// FILESYSTEMS vector is constructed in FSType enum order: FS_UNALLOCATED,
	// FS_UNKNOWN, FS_CLEARED, FS_EXTENDED, FS_BTRFS, ..., FS_LINUX_SWRAID,
	// LINUX_SWSUSPEND which ultimately controls the default order of file systems in
	// menus and dialogs.
	FILESYSTEMS .clear() ;

	for ( f = FILESYSTEM_MAP .begin() ; f != FILESYSTEM_MAP .end() ; f++ ) {
		if ( f ->second )
			FILESYSTEMS .push_back( f ->second ->get_filesystem_support() ) ;
		else {
			FS fs_notsupp( f->first );
			FILESYSTEMS .push_back( fs_notsupp ) ;
		}
	}
}

void GParted_Core::set_user_devices( const std::vector<Glib::ustring> & user_devices ) 
{
	this ->device_paths = user_devices ;
	this ->probe_devices = ! user_devices .size() ;
}
	
void GParted_Core::set_devices( std::vector<Device> & devices )
{
	Glib::Thread::create( sigc::bind(
				sigc::mem_fun( *this, &GParted_Core::set_devices_thread ),
				&devices),
			      false );
	Gtk::Main::run();
}

static bool _mainquit( void *dummy )
{
	Gtk::Main::quit();
	return false;
}

void GParted_Core::set_devices_thread( std::vector<Device> * pdevices )
{
	std::vector<Device> &devices = *pdevices;
	devices .clear() ;
	BlockSpecial::clear_cache();            // MUST BE FIRST.  Cache of name to major, minor
	                                        // numbers incrementally loaded when BlockSpecial
	                                        // objects are created in the following caches.
	Proc_Partitions_Info::load_cache();     // SHOULD BE SECOND.  Caches /proc/partitions and
	                                        // pre-populates BlockSpecial cache.
	FS_Info::load_cache();                  // SHOULD BE THRID.  Caches file system details
	                                        // from blkid output.
	DMRaid dmraid( true ) ;    //Refresh cache of dmraid device information
	LVM2_PV_Info::clear_cache();            // Cache automatically loaded if and when needed
	btrfs::clear_cache();                   // Cache incrementally loaded if and when needed
	SWRaid_Info::load_cache();
	LUKS_Info::clear_cache();               // Cache automatically loaded if and when needed
	Mount_Info::load_cache();

	//only probe if no devices were specified as arguments..
	if ( probe_devices )
	{
		device_paths .clear() ;

		//FIXME:  When libparted bug 194 is fixed, remove code to read:
		//           /proc/partitions
		//        This was a problem with no floppy drive yet BIOS indicated one existed.
		//        http://parted.alioth.debian.org/cgi-bin/trac.cgi/ticket/194
		//
		//try to find all available devices if devices exist in /proc/partitions
		std::vector<Glib::ustring> temp_devices = Proc_Partitions_Info::get_device_paths();
		if ( ! temp_devices .empty() )
		{
			//Try to find all devices in /proc/partitions
			for (unsigned int k=0; k < temp_devices .size(); k++)
			{
				/*TO TRANSLATORS: looks like Scanning /dev/sda */
				set_thread_status_message( String::ucompose ( _("Scanning %1"), temp_devices[ k ] ) ) ;
				ped_device_get( temp_devices[ k ] .c_str() ) ;
			}

			//Try to find all dmraid devices
			if (dmraid .is_dmraid_supported() ) {
				std::vector<Glib::ustring> dmraid_devices ;
				dmraid .get_devices( dmraid_devices ) ;
				for ( unsigned int k=0; k < dmraid_devices .size(); k++ ) {
					set_thread_status_message( String::ucompose ( _("Scanning %1"), dmraid_devices[k] ) ) ;
#ifndef USE_LIBPARTED_DMRAID
					dmraid .create_dev_map_entries( dmraid_devices[k] ) ;
					settle_device( SETTLE_DEVICE_PROBE_MAX_WAIT_SECONDS );
#endif
					ped_device_get( dmraid_devices[k] .c_str() ) ;
				}
			}
		}
		else
		{
			//No devices found in /proc/partitions so use libparted to probe devices
			ped_device_probe_all();
		}

		PedDevice* lp_device = ped_device_get_next( NULL ) ;
		while ( lp_device ) 
		{
			/* TO TRANSLATORS: looks like   Confirming /dev/sda */
			set_thread_status_message( String::ucompose( _("Confirming %1"), lp_device->path ) );

			//only add this device if we can read the first sector (which means it's a real device)
			if ( useable_device( lp_device ) )
				device_paths.push_back( lp_device->path );

			lp_device = ped_device_get_next( lp_device ) ;
		}

		std::sort( device_paths .begin(), device_paths .end() ) ;
	}
	else
	{
		//Device paths were passed in on the command line.

		// Sort name device paths and remove duplicates.  Avoids repeated scanning
		// the same device and showing it multiple times in the UI.
		// Reference:
		//     What's the most efficient way to erase duplicates and sort a vector?
		//     http://stackoverflow.com/questions/1041620/whats-the-most-efficient-way-to-erase-duplicates-and-sort-a-vector
		std::sort( device_paths.begin(), device_paths.end() );
		device_paths.erase( std::unique( device_paths.begin(), device_paths.end() ), device_paths.end() );

		for ( unsigned int t = 0 ; t < device_paths .size() ; t++ ) 
		{
			set_thread_status_message( String::ucompose( _("Confirming %1"), device_paths[t] ) );

#ifndef USE_LIBPARTED_DMRAID
			// Ensure that dmraid device entries are created
			if ( dmraid .is_dmraid_supported() &&
			     dmraid .is_dmraid_device( device_paths[t] ) )
			{
				dmraid .create_dev_map_entries( dmraid .get_dmraid_name( device_paths [t] ) ) ;
				settle_device( SETTLE_DEVICE_PROBE_MAX_WAIT_SECONDS );
			}
#endif

			PedDevice* lp_device = ped_device_get( device_paths[t].c_str() );
			if ( lp_device == NULL || ! useable_device( lp_device ) )
			{
				// Remove this disk device which isn't useable
				device_paths.erase( device_paths.begin() + t-- );
			}
		}
	}

	// Ensure all named paths have FS_Info blkid cache entries specifically so that
	// command line named file system image files, which blkid can't otherwise know
	// about, can be identified.
	FS_Info::load_cache_for_paths( device_paths );

	for ( unsigned int t = 0 ; t < device_paths .size() ; t++ ) 
	{
		/*TO TRANSLATORS: looks like Searching /dev/sda partitions */ 
		set_thread_status_message( String::ucompose ( _("Searching %1 partitions"), device_paths[ t ] ) ) ;
		Device temp_device;
		set_device_from_disk( temp_device, device_paths[t] );
		devices.push_back( temp_device );
	}

	set_thread_status_message("") ;
	g_idle_add( (GSourceFunc)_mainquit, NULL );
}

// runs gpart on the specified parameter
void GParted_Core::guess_partition_table(const Device & device, Glib::ustring &buff)
{
	Glib::ustring error;
	Glib::ustring cmd = "gpart -s " + Utils::num_to_str( device.sector_size ) +
	                    " " + Glib::shell_quote( device.get_path() );
	Utils::execute_command( cmd, buff, error, true );
}

void GParted_Core::set_thread_status_message( Glib::ustring msg )
{
	//Remember to clear status message when finished with thread.
	thread_status_message = msg ;
}

Glib::ustring GParted_Core::get_thread_status_message( )
{
	return thread_status_message ;
}

bool GParted_Core::snap_to_cylinder( const Device & device, Partition & partition, Glib::ustring & error ) 
{
	Sector diff = 0;

	//Determine if partition size is less than half a disk cylinder
	bool less_than_half_cylinder = false;
	if ( ( partition .sector_end - partition .sector_start ) < ( device .cylsize / 2 ) )
		less_than_half_cylinder = true;

	if ( partition.type == TYPE_LOGICAL ||
	     partition.sector_start == device .sectors
	   )
	{
		//Must account the relative offset between:
		// (A) the Extended Boot Record sector and the next track of the
		//     logical partition (usually 63 sectors), and
		// (B) the Master Boot Record sector and the next track of the first
		//     primary partition
		diff = (partition .sector_start - device .sectors) % device .cylsize ;
	}
	else if ( partition.sector_start == 34 )
	{
		// (C) the GUID Partition Table (GPT) and the start of the data
		//     partition at sector 34
		diff = (partition .sector_start - 34 ) % device .cylsize ;
	}
	else
	{
		diff = partition .sector_start % device .cylsize ;
	}
	if ( diff && ! partition .strict_start  )
	{
		if ( diff < ( device .cylsize / 2 ) || less_than_half_cylinder )
			partition .sector_start -= diff ;
		else
			partition .sector_start += (device .cylsize - diff ) ;
	}

	diff = (partition .sector_end +1) % device .cylsize ;
	if ( diff )
	{
		if ( diff < ( device .cylsize / 2 ) && ! less_than_half_cylinder )
			partition .sector_end -= diff ;
		else
			partition .sector_end += (device .cylsize - diff ) ;
	}

	return true ;
}

bool GParted_Core::snap_to_mebibyte( const Device & device, Partition & partition, Glib::ustring & error ) 
{
	Sector diff = 0;
	if ( partition .sector_start < 2 || partition .type == TYPE_LOGICAL )
	{
		//Must account the relative offset between:
		// (A) the Master Boot Record sector and the first primary/extended partition, and
		// (B) the Extended Boot Record sector and the logical partition

		//If strict_start is set then do not adjust sector start.
		//If this partition is not simply queued for a reformat then
		//  add space minimum to force alignment to next mebibyte.
		if (   (! partition .strict_start)
		    && (partition .free_space_before == 0)
		    && ( partition .status != STAT_FORMATTED)
		   )
		{
			//Unless specifically told otherwise, the Linux kernel considers extended
			//  boot records to be two sectors long, in order to "leave room for LILO".
			partition .sector_start += 2 ;
		}
	}

	//Calculate difference offset from Mebibyte boundary
	diff = Sector(partition .sector_start % ( MEBIBYTE / partition .sector_size ));

	//Align start sector only if permitted to change start sector
	if ( diff && (   (! partition .strict_start)
	              || (   partition .strict_start
	                  && (   partition .status == STAT_NEW
	                      || partition .status == STAT_COPY
	                     )
	                 )
	             )
	   )
	{
		partition .sector_start += ( (MEBIBYTE / partition .sector_size) - diff) ;

		//If this is an extended partition then check to see if sufficient space is
		//  available for any following logical partition Extended Boot Record
		if ( partition .type == TYPE_EXTENDED )
		{
			//If there is logical partition that starts less than 2 sectors
			//  from the start of this partition, then reserve a mebibyte for the EBR.
			int index_extended = find_extended_partition( device.partitions );
			if ( index_extended >= 0 )
			{
				for ( unsigned int t = 0; t < device .partitions[ index_extended ] .logicals .size(); t++ )
				{
					if (   ( device .partitions[ index_extended ] .logicals[ t ] .type == TYPE_LOGICAL )
					    && ( (  (  device .partitions[ index_extended ] .logicals[ t ] .sector_start )
					          - ( partition .sector_start )
					         )
					         //Unless specifically told otherwise, the Linux kernel considers extended
					         //  boot records to be two sectors long, in order to "leave room for LILO".
					         < 2
					       )
					   )
					{
						partition .sector_start -= (MEBIBYTE / partition .sector_size) ;
					}
				}
			}
		}
	}

	//Align end sector
	diff = (partition .sector_end + 1) % ( MEBIBYTE / partition .sector_size);
	if ( diff )
		partition .sector_end -= diff ;

	//If this is a logical partition not at end of drive then check to see if space is
	//  required for a following logical partition Extended Boot Record
	if ( partition .type == TYPE_LOGICAL )
	{
		//If there is a following logical partition that starts less than 2 sectors from
		//  the end of this partition, then reserve at least a mebibyte for the EBR.
		int index_extended = find_extended_partition( device.partitions );
		if ( index_extended >= 0 )
		{
			for ( unsigned int t = 0; t < device .partitions[ index_extended ] .logicals .size(); t++ )
			{
				if (   ( device .partitions[ index_extended ] .logicals[ t ] .type == TYPE_LOGICAL )
				    && ( device .partitions[ index_extended ] .logicals[ t ] .sector_start > partition .sector_end )
				    && ( ( device .partitions[ index_extended ] .logicals[ t ] .sector_start - partition .sector_end )
				           //Unless specifically told otherwise, the Linux kernel considers extended
				           //  boot records to be two sectors long, in order to "leave room for LILO".
				         < 2
				       )
				   )
					partition .sector_end -= ( MEBIBYTE / partition .sector_size ) ;
			}
		}

		//If the logical partition end is beyond the end of the extended partition
		//  then reduce logical partition end by a mebibyte to address the overlap.
		if (   ( index_extended != -1 )
		    && ( partition .sector_end > device .partitions[ index_extended ] .sector_end )
		   )
			partition .sector_end -= ( MEBIBYTE / partition .sector_size ) ;
	}

	//If this is a primary or an extended partition and the partition overlaps
	//  the start of the next primary or extended partition then subtract a
	//  mebibyte from the end of the partition to address the overlap.
	if ( partition .type == TYPE_PRIMARY || partition .type == TYPE_EXTENDED )
	{
		for ( unsigned int t = 0 ; t < device .partitions .size() ; t++ )
		{
			if (   (   device .partitions[ t ] .type == TYPE_PRIMARY
			        || device .partitions[ t ] .type == TYPE_EXTENDED
			       )
			    && (   //For a change to an existing partition, (e.g., move or resize)
			           //  skip comparing to original partition and
			           //  only compare to other existing partitions
			           partition .status == STAT_REAL
			        && partition .partition_number != device. partitions[ t ] .partition_number
			       )
			    && ( device .partitions[ t ] .sector_start > partition .sector_start )
			    && ( device .partitions[ t ] .sector_start <= partition .sector_end )
			   )
				partition .sector_end -= ( MEBIBYTE / partition .sector_size );
		}
	}

	//If this is an extended partition then check to see if the end of the
	//  extended partition encompasses the end of the last logical partition.
	if ( partition .type == TYPE_EXTENDED )
	{
		//If there is logical partition that has an end sector beyond the
		//  end of the extended partition, then set the extended partition
		//  end sector to be the same as the end of the logical partition.
		for ( unsigned int t = 0; t < partition .logicals .size(); t++ )
		{
			if (   ( partition .logicals[ t ] .type == TYPE_LOGICAL )
			    && (   (  partition .logicals[ t ] .sector_end )
			         > ( partition .sector_end )
			       )
			   )
			{
				partition .sector_end = partition .logicals[ t ] .sector_end ;
			}
		}
	}

	//If this is a GPT partition table and the partition ends less than 34 sectors
	//  from the end of the device, then reserve at least a mebibyte for the
	//  backup partition table
	if (    device .disktype == "gpt"
	    && ( ( device .length - partition .sector_end ) < 34 )
	   )
	{
		partition .sector_end -= ( MEBIBYTE / partition .sector_size ) ;
	}

	return true ;
}

bool GParted_Core::snap_to_alignment( const Device & device, Partition & partition, Glib::ustring & error )
{
	bool rc = true ;

	if ( partition .alignment == ALIGN_CYLINDER )
		rc = snap_to_cylinder( device, partition, error ) ;
	else if ( partition .alignment == ALIGN_MEBIBYTE )
		rc = snap_to_mebibyte( device, partition, error ) ;

	//Ensure that partition start and end are not beyond the ends of the disk device
	if ( partition .sector_start < 0 )
		partition .sector_start = 0 ;
	if ( partition .sector_end > device .length )
		partition .sector_end = device .length - 1 ;

	//do some basic checks on the partition
	if ( partition .get_sector_length() <= 0 )
	{
		error = String::ucompose(
				/* TO TRANSLATORS:  looks like   A partition cannot have a length of -1 sectors */
				_("A partition cannot have a length of %1 sectors"),
				partition .get_sector_length() ) ;
		return false ;
	}

	//FIXME: I think that this if condition should be impossible because Partition::set_sector_usage(),
	//  and ::set_used() and ::Set_Unused() before that, don't allow setting file usage figures to be
	//  larger than the partition size.  A btrfs file system spanning muiltiple partitions will have
	//  usage figures larger than any single partition but the figures will won't be set because of
	//  the above reasoning.  Confirm condition is impossible and consider removing this code.
	if ( partition .get_sector_length() < partition .sectors_used )
	{
		error = String::ucompose(
				/* TO TRANSLATORS: looks like   A partition with used sectors (2048) greater than its length (1536) is not valid */
				_("A partition with used sectors (%1) greater than its length (%2) is not valid"),
				partition .sectors_used,
				partition .get_sector_length() ) ;
		return false ;
	}

	//FIXME: it would be perfect if we could check for overlapping with adjacent partitions as well,
	//however, finding the adjacent partitions is not as easy as it seems and at this moment all the dialogs
	//already perform these checks. A perfect 'fixme-later' ;)

	return rc ;
}

bool GParted_Core::apply_operation_to_disk( Operation * operation )
{
	bool success = false;
	libparted_messages .clear() ;
	operation->operation_detail.signal_capture_errors.connect(
			sigc::mem_fun( *this, &GParted_Core::capture_libparted_messages ) );

	switch ( operation->type )
	{
		// Call calibrate_partition() first for each operation to ensure the
		// correct partition path name and boundary is known before performing the
		// actual modifications.  (See comments in calibrate_partition() for
		// reasons why this is necessary).  Calibrate the most relevant partition
		// object(s), either partition_original, partition_new or partition_copy,
		// as required.  Calibrate also displays details of the partition being
		// modified in the operation results to inform the user.
		//
		// Win_GParted::set_valid_operations() determines which operations are
		// allowed on file systems depending on whether each is busy mounted or
		// not.  For encrypted file systems allowed operations also depends on
		// whether the encryption mapping is open or not.  Therefore each
		// operation must leave the status of the file system and any encryption
		// mapping in the same state which it found it, ready for the next
		// operation.

		case OPERATION_DELETE:
			success =    calibrate_partition( operation->get_partition_original(),
			                                  operation->operation_detail )
			          && remove_filesystem( operation->get_partition_original(),
			                                operation->operation_detail )
			          && delete_partition( operation->get_partition_original(),
			                               operation->operation_detail );
			break;

		case OPERATION_CHECK:
			success =    calibrate_partition( operation->get_partition_original(),
			                                  operation->operation_detail )
			          && check_repair_filesystem( operation->get_partition_original().get_filesystem_partition(),
			                                      operation->operation_detail )
			          && check_repair_maximize( operation->get_partition_original(),
			                                    operation->operation_detail );
			break;

		case OPERATION_CREATE:
			// The partition doesn't exist yet so there's nothing to calibrate.
			success = create( operation->get_partition_new(), operation->operation_detail );
			break;

		case OPERATION_RESIZE_MOVE:
			success = calibrate_partition( operation->get_partition_original(),
			                               operation->operation_detail );
			if ( ! success )
				break;

			// Replace the new partition object's path from the calibration in
			// case the path is "Copy of ..." from the partition having been
			// newly created by a paste into unallocated space earlier in the
			// sequence of operations now being applied.
			operation->get_partition_new().set_path( operation->get_partition_original().get_path() );

			success = resize_move( operation->get_partition_original(),
			                       operation->get_partition_new(),
			                       operation->operation_detail );
			break;

		case OPERATION_FORMAT:
			success = calibrate_partition( operation->get_partition_new(), operation->operation_detail );
			if ( ! success )
				break;

			// Replace the original partition object's path from the
			// calibration in case the path is "Copy of ..." from the
			// partition having been newly created by a paste into unallocated
			// space earlier in the sequence of operations now being applied.
			operation->get_partition_original().set_path( operation->get_partition_new().get_path() );

			success =    remove_filesystem( operation->get_partition_original().get_filesystem_partition(),
			                                operation->operation_detail )
			          && format( operation->get_partition_new().get_filesystem_partition(),
			                     operation->operation_detail );
			break;

		case OPERATION_COPY:
		{
			//FIXME: in case of a new partition we should make sure the new partition is >= the source partition... 
			//i think it's best to do this in the dialog_paste

			OperationCopy * copy_op = static_cast<OperationCopy*>( operation );
			success =    calibrate_partition( copy_op->get_partition_copied(),
			                                  copy_op->operation_detail )
			             // Only calibrate the destination when pasting into an existing
			             // partition, rather than when creating a new partition.
                                  && ( copy_op->get_partition_original().type == TYPE_UNALLOCATED                     ||
			               calibrate_partition( copy_op->get_partition_new(), copy_op->operation_detail )    );
			if ( ! success )
				break;

			success =    remove_filesystem( copy_op->get_partition_original().get_filesystem_partition(),
			                                copy_op->operation_detail )
			          && copy( copy_op->get_partition_copied(),
			                   copy_op->get_partition_new(),
			                   copy_op->operation_detail );
			break;
		}

		case OPERATION_LABEL_FILESYSTEM:
			success =    calibrate_partition( operation->get_partition_new(), operation->operation_detail )
			          && label_filesystem( operation->get_partition_new().get_filesystem_partition(),
			                               operation->operation_detail );
			break;

		case OPERATION_NAME_PARTITION:
			success =    calibrate_partition( operation->get_partition_new(), operation->operation_detail )
			          && name_partition( operation->get_partition_new(), operation->operation_detail );
			break;

		case OPERATION_CHANGE_UUID:
			success =    calibrate_partition( operation->get_partition_new(), operation->operation_detail )
			          && change_filesystem_uuid( operation->get_partition_new().get_filesystem_partition(),
			                                     operation->operation_detail );
			break;
	}

	return success;
}

bool GParted_Core::set_disklabel( const Device & device, const Glib::ustring & disklabel )
{
	Glib::ustring device_path = device.get_path();

	// FIXME: Should call file system specific removal actions
	// (to remove LVM2 PVs before deleting the partitions).

#ifdef ENABLE_LOOP_DELETE_OLD_PTNS_WORKAROUND
	// When creating a "loop" table with libparted 2.0 to 3.0 inclusive, it doesn't
	// inform the kernel to delete old partitions so as a consequence blkid's cache
	// becomes stale and it won't report a file system subsequently created on the
	// whole disk device.  Create a GPT first to use that code in libparted to delete
	// any old partitions.  Fixed in parted 3.1 by commit:
	//     f5c909c0cd50ed52a48dae6d35907dc08b137e88
	//     libparted: remove has_partitions check to allow loopback partitions
	if ( disklabel == "loop" )
		new_disklabel( device_path, "gpt", false );
#endif

	// Ensure that any previous whole disk device file system can't be recognised by
	// libparted in preference to the "loop" partition table signature, or by blkid in
	// preference to any partition table about to be written.
	OperationDetail dummy_od;
	Partition temp_partition;
	temp_partition.set_unpartitioned( device_path,
	                                  "",
	                                  FS_UNALLOCATED,
	                                  device.length,
	                                  device.sector_size,
	                                  false );
	erase_filesystem_signatures( temp_partition, dummy_od );

	return new_disklabel( device_path, disklabel );
}

bool GParted_Core::new_disklabel( const Glib::ustring & device_path, const Glib::ustring & disklabel,
                                  bool recreate_dmraid_devs )
{
	bool return_value = false ;

	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device( device_path, lp_device ) )
	{
		PedDiskType *type = NULL ;
		type = ped_disk_type_get( disklabel .c_str() ) ;
		
		if ( type )
		{
			lp_disk = ped_disk_new_fresh( lp_device, type );
		
			return_value = commit( lp_disk ) ;
		}
		
		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

#ifndef USE_LIBPARTED_DMRAID
	//delete and recreate disk entries if dmraid
	DMRaid dmraid ;
	if ( recreate_dmraid_devs && return_value && dmraid.is_dmraid_device( device_path ) )
	{
		dmraid .purge_dev_map_entries( device_path ) ;
		dmraid .create_dev_map_entries( device_path ) ;
	}
#endif

	return return_value ;	
}

bool GParted_Core::toggle_flag( const Partition & partition, const Glib::ustring & flag, bool state ) 
{
	bool succes = false ;
	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device_and_disk( partition .device_path, lp_device, lp_disk ) )
	{
		PedPartition* lp_partition = get_lp_partition( lp_disk, partition );
		if ( lp_partition )
		{
			PedPartitionFlag lp_flag = ped_partition_flag_get_by_name( flag .c_str() ) ;

			if ( lp_flag > 0 && ped_partition_set_flag( lp_partition, lp_flag, state ) )
				succes = commit( lp_disk ) ;
		}
	
		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

	return succes ;
}

const std::vector<FS> & GParted_Core::get_filesystems() const 
{
	return FILESYSTEMS ;
}

// Return supported capabilities of the file system type or, if not found, not supported
// capabilities set.
const FS & GParted_Core::get_fs( FSType filesystem ) const
{
	for ( unsigned int t = 0 ; t < FILESYSTEMS .size() ; t++ )
	{
		if ( FILESYSTEMS[ t ] .filesystem == filesystem )
			return FILESYSTEMS[ t ] ;
	}

	static FS fs_notsupp( FS_UNKNOWN );
	return fs_notsupp;
}

//Return all libparted's partition table types in it's preferred ordering,
//  alphabetic except with "loop" last.
//  Ref: parted >= 1.8 ./libparted/libparted.c init_disk_types()
std::vector<Glib::ustring> GParted_Core::get_disklabeltypes()
{
	std::vector<Glib::ustring> disklabeltypes ;

	PedDiskType *disk_type ;
	for ( disk_type = ped_disk_type_get_next( NULL ) ; disk_type ; disk_type = ped_disk_type_get_next( disk_type ) )
		disklabeltypes .push_back( disk_type->name ) ;

	 return disklabeltypes ;
}

std::map<Glib::ustring, bool> GParted_Core::get_available_flags( const Partition & partition ) 
{
	std::map<Glib::ustring, bool> flag_info ;

	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device_and_disk( partition .device_path, lp_device, lp_disk ) )
	{
		PedPartition* lp_partition = get_lp_partition( lp_disk, partition );
		if ( lp_partition )
		{
			for ( unsigned int t = 0 ; t < flags .size() ; t++ )
				if ( ped_partition_is_flag_available( lp_partition, flags[ t ] ) )
					flag_info[ ped_partition_flag_get_name( flags[ t ] ) ] =
						ped_partition_get_flag( lp_partition, flags[ t ] ) ;
		}
	
		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

	return flag_info ;
}
	
Glib::ustring GParted_Core::get_libparted_version() 
{
	return ped_get_version() ;
}

//private functions...

Glib::ustring GParted_Core::get_partition_path( PedPartition * lp_partition )
{
	char * lp_path;  //we have to free the result of ped_partition_get_path()
	Glib::ustring partition_path = "Partition path not found";

	lp_path = ped_partition_get_path(lp_partition);
	if ( lp_path != NULL )
	{
		partition_path = lp_path;
		free(lp_path);
	}

#ifndef USE_LIBPARTED_DMRAID
	//Ensure partition path name is compatible with dmraid
	DMRaid dmraid;   //Use cache of dmraid device information
	if (   dmraid .is_dmraid_supported()
	    && dmraid .is_dmraid_device( partition_path )
	   )
	{
		partition_path = dmraid .make_path_dmraid_compatible(partition_path);
	}
#endif

	return partition_path ;
}

void GParted_Core::set_device_from_disk( Device & device, const Glib::ustring & device_path )
{
	PedDevice* lp_device = NULL;
	PedDisk* lp_disk = NULL;
	if ( get_device( device_path, lp_device, true ) )
	{
		device.Reset();

		// Device info ...
		device.set_path( device_path );
		device.model       = lp_device->model;
		device.length      = lp_device->length;
		device.sector_size = lp_device->sector_size;
		device.heads       = lp_device->bios_geom.heads;
		device.sectors     = lp_device->bios_geom.sectors;
		device.cylinders   = lp_device->bios_geom.cylinders;
		device.cylsize     = device.heads * device.sectors;
		set_device_serial_number( device );

		// Make sure cylsize is at least 1 MiB
		if ( device.cylsize < (MEBIBYTE / device.sector_size) )
			device.cylsize = MEBIBYTE / device.sector_size;

		std::vector<Glib::ustring> messages;
		FSType fstype = detect_filesystem( lp_device, NULL, messages );
		// FS_Info (blkid) recognised file system signature on whole disk device.
		// Need to detect before libparted reported partitioning to avoid bug in
		// libparted 1.9.0 to 2.3 inclusive which recognised FAT file systems as
		// MSDOS partition tables.  Fixed in parted 2.4 by commit:
		//     616a2a1659d89ff90f9834016a451da8722df509
		//     libparted: avoid regression when processing a whole-disk FAT partition
		if ( fstype != FS_UNKNOWN )
		{
			// Clear the possible "unrecognised disk label" message
			libparted_messages.clear();

			device.disktype = "none";
			device.max_prims = 1;
			set_device_one_partition( device, lp_device, fstype, messages );
		}
		// Partitioned drive
		else if ( get_disk( lp_device, lp_disk, false ) )
		{
			// Partitioned drive (excluding "loop"), as recognised by libparted
			if ( lp_disk && lp_disk->type && lp_disk->type->name &&
			     strcmp( lp_disk->type->name, "loop" ) != 0         )
			{
				device.disktype = lp_disk->type->name;
				device.max_prims = ped_disk_get_max_primary_partition_count( lp_disk );

				// Determine if partition naming is supported.
				if ( ped_disk_type_check_feature( lp_disk->type, PED_DISK_TYPE_PARTITION_NAME ) )
				{
					device.enable_partition_naming(
							Utils::get_max_partition_name_length( device.disktype ) );
				}

				set_device_partitions( device, lp_device, lp_disk );

				if ( device.highest_busy )
				{
					device.readonly = ! commit_to_os( lp_disk, SETTLE_DEVICE_PROBE_MAX_WAIT_SECONDS );
					// Clear libparted messages.  Typically these are:
					//     The kernel was unable to re-read the partition table...
					libparted_messages.clear();
				}
			}
			// Drive just containing libparted "loop" signature and nothing
			// else.  (Actually any drive reported by libparted as "loop" but
			// not recognised by blkid on the whole disk device).
			else if ( lp_disk && lp_disk->type && lp_disk->type->name &&
			          strcmp( lp_disk->type->name, "loop" ) == 0         )
			{
				device.disktype = lp_disk->type->name;
				device.max_prims = 1;

				// Create virtual partition covering the whole disk device
				// with unknown contents.
				Partition * partition_temp = new Partition();
				partition_temp->set_unpartitioned( device.get_path(),
				                                   lp_device->path,
				                                   FS_UNKNOWN,
				                                   device.length,
				                                   device.sector_size,
				                                   false );
				// Place unknown file system message in this partition.
				partition_temp->append_messages( messages );
				device.partitions.push_back_adopt( partition_temp );
			}
			// Unrecognised, unpartitioned drive.
			else
			{
				device.disktype =
					/* TO TRANSLATORS:  unrecognized
					 * means that the partition table for this disk
					 * device is unknown or not recognized.
					 */
					_("unrecognized");
				device.max_prims = 1;

				Partition * partition_temp = new Partition();
				partition_temp->set_unpartitioned( device.get_path(),
				                                   "",  // Overridden with "unallocated"
				                                   FS_UNALLOCATED,
				                                   device.length,
				                                   device.sector_size,
				                                   false );
				// Place libparted messages in this unallocated partition
				partition_temp->append_messages( libparted_messages );
				libparted_messages.clear();
				device.partitions.push_back_adopt( partition_temp );
			}
		}

		destroy_device_and_disk( lp_device, lp_disk);
	}
}

void GParted_Core::set_device_serial_number( Device & device )
{
	if ( ! hdparm_found )
		// Serial number left blank when the hdparm command is not installed.
		return;

	Glib::ustring output;
	Glib::ustring error;
	Utils::execute_command( "hdparm -I " + Glib::shell_quote( device.get_path() ), output, error, true );
	if ( ! error.empty() )
	{
		// hdparm reported an error message to stderr.  Assume it's a device
		// without a hard drive serial number.
		//
		// Using hdparm -I to query Linux software RAID arrays and BIOS fake RAID
		// arrays, both devices without their own hard drive serial numbers,
		// produce this error:
		//     HDIO_DRIVE_CMD(identify) failed: Inappropriate ioctl for device
		//
		// And querying USB flash drives, also a device type without their own
		// hard drive serial numbers, generates this error:
		//     SG_IO: bad/missing sense data, sb[]:  70 00 05 00 00 00 00 0a ...
		device.serial_number = "none";
	}
	else
	{
		Glib::ustring serial_number = Utils::trim( Utils::regexp_label( output,
				"^[[:blank:]]*Serial Number:[[:blank:]]*(.*)[[:blank:]]*$" ) );
		if ( ! serial_number.empty() )
			device.serial_number = serial_number;
	}
	// Otherwise serial number left blank when not found in the hdparm output.
}

/**
 * Fills the device.partitions member of device by scanning
 * all partitions
 */
void GParted_Core::set_device_partitions( Device & device, PedDevice* lp_device, PedDisk* lp_disk )
{
	int EXT_INDEX = -1 ;
#ifndef USE_LIBPARTED_DMRAID
	DMRaid dmraid ;    //Use cache of dmraid device information
#endif

	//clear partitions
	device .partitions .clear() ;

	PedPartition* lp_partition = ped_disk_next_partition( lp_disk, NULL ) ;
	while ( lp_partition )
	{
		libparted_messages .clear() ;
		Partition * partition_temp = NULL;
		bool partition_is_busy = false ;
		FSType filesystem;
		std::vector<Glib::ustring> detect_messages;

		//Retrieve partition path
		Glib::ustring partition_path = get_partition_path( lp_partition );

		// NOTE: lp_partition->type bit field
		// lp_partition->type is a bit field using enumerated names for each bit.
		// GParted is only interested in partitions with these single bits set:
		// PED_PARTITION_NORMAL, PED_PARTITION_LOGICAL and PED_PARTITION_EXTENDED.
		// Partitions, ranges of blocks, with other bits set representing free
		// space and disk label meta-data, PED_PARTITION_FREESPACE and
		// PED_PARTITION_METADATA bits respectively, are ignored and GParted
		// creates its own unallocated partitions and accounts for partition
		// tables.
		// References:
		// *   struct PedPartition and type PedPartitionType
		//     https://www.gnu.org/software/parted/api/struct__PedPartition.html
		// *   enum _PedPartitionType
		//     http://git.savannah.gnu.org/cgit/parted.git/tree/include/parted/disk.in.h?id=v3.2#n45
		switch ( lp_partition ->type )
		{
			case PED_PARTITION_NORMAL:
			case PED_PARTITION_LOGICAL:
				filesystem = detect_filesystem( lp_device, lp_partition, detect_messages );
#ifndef USE_LIBPARTED_DMRAID
				//Handle dmraid devices differently because the minor number might not
				//  match the last number of the partition filename as shown by "ls -l /dev/mapper"
				if ( dmraid .is_dmraid_device( device .get_path() ) )
				{
					//Try device_name + partition_number
					Glib::ustring dmraid_path = device .get_path() + Utils::num_to_str( lp_partition ->num ) ;
					partition_is_busy = is_busy( filesystem, dmraid_path ) ;

					//Try device_name + p + partition_number
					dmraid_path = device .get_path() + "p" + Utils::num_to_str( lp_partition ->num ) ;
					partition_is_busy |= is_busy( filesystem, dmraid_path ) ;
				}
				else
#endif
				{
					partition_is_busy = is_busy( filesystem, partition_path ) ;
				}

				if ( filesystem == FS_LUKS )
					partition_temp = new PartitionLUKS();
				else
					partition_temp = new Partition();
				partition_temp->Set( device .get_path(),
				                     partition_path,
				                     lp_partition->num,
				                     ( lp_partition->type == PED_PARTITION_NORMAL ) ? TYPE_PRIMARY
				                                                                    : TYPE_LOGICAL,
				                     filesystem,
				                     lp_partition->geom.start,
				                     lp_partition->geom.end,
				                     device.sector_size,
				                     ( lp_partition->type == PED_PARTITION_LOGICAL ),
				                     partition_is_busy );
				partition_temp->append_messages( detect_messages );

				set_flags( *partition_temp, lp_partition );

				if ( filesystem == FS_LUKS )
					set_luks_partition( *dynamic_cast<PartitionLUKS *>( partition_temp ) );

				if ( partition_temp->busy && partition_temp->partition_number > device.highest_busy )
					device.highest_busy = partition_temp->partition_number;
				break ;
			
			case PED_PARTITION_EXTENDED:
				partition_temp = new Partition();
				partition_temp->Set( device.get_path(),
				                     partition_path,
				                     lp_partition->num,
				                     TYPE_EXTENDED,
				                     FS_EXTENDED,
				                     lp_partition->geom.start,
				                     lp_partition->geom.end,
				                     device.sector_size,
				                     false,
				                     false );

				set_flags( *partition_temp, lp_partition );

				EXT_INDEX = device .partitions .size() ;
				break ;

			default:
				// Ignore libparted reported partitions with other type
				// bits set.
				break;
		}

		// Only for libparted reported partition types that we care about: NORMAL,
		// LOGICAL, EXTENDED
		if ( partition_temp != NULL )
		{
			set_partition_label_and_uuid( *partition_temp );
			set_mountpoints( *partition_temp );
			set_used_sectors( *partition_temp, lp_disk );

			// Retrieve partition name
			if ( device.partition_naming_supported() )
				partition_temp->name = Glib::ustring( ped_partition_get_name( lp_partition ) );

			partition_temp->append_messages( libparted_messages );

			if ( ! partition_temp->inside_extended )
				device.partitions.push_back_adopt( partition_temp );
			else
				device.partitions[EXT_INDEX].logicals.push_back_adopt( partition_temp );
		}

		//next partition (if any)
		lp_partition = ped_disk_next_partition( lp_disk, lp_partition ) ;
	}

	if ( EXT_INDEX > -1 )
	{
		insert_unallocated( device .get_path(),
				    device .partitions[ EXT_INDEX ] .logicals,
				    device .partitions[ EXT_INDEX ] .sector_start,
				    device .partitions[ EXT_INDEX ] .sector_end,
				    device .sector_size,
				    true ) ;

		//Set busy status of extended partition if and only if
		//  there is at least one busy logical partition.
		for ( unsigned int t = 0 ; t < device .partitions[ EXT_INDEX ] .logicals .size() ; t ++ )
		{
			if ( device .partitions[ EXT_INDEX ] .logicals[ t ] .busy )
			{
				device .partitions[ EXT_INDEX ] .busy = true ;
				break ;
			}
		}
	}

	insert_unallocated( device .get_path(), device .partitions, 0, device .length -1, device .sector_size, false ) ; 
}

// Create one Partition object spanning the Device after identifying the file system
// on the whole disk device.  Much simplified equivalent of set_device_partitions().
void GParted_Core::set_device_one_partition( Device & device, PedDevice * lp_device, FSType fstype,
                                             std::vector<Glib::ustring> & messages )
{
	device.partitions.clear();

	Glib::ustring path = lp_device->path;
	bool partition_is_busy = is_busy( fstype, path );

	Partition * partition_temp = NULL;
	if ( fstype == FS_LUKS )
		partition_temp = new PartitionLUKS();
	else
		partition_temp = new Partition();
	partition_temp->set_unpartitioned( device.get_path(),
	                                   path,
	                                   fstype,
	                                   device.length,
	                                   device.sector_size,
	                                   partition_is_busy );

	partition_temp->append_messages( messages );

	if ( fstype == FS_LUKS )
		set_luks_partition( *dynamic_cast<PartitionLUKS *>( partition_temp ) );

	if ( partition_temp->busy )
		device.highest_busy = 1;

	set_partition_label_and_uuid( *partition_temp );
	set_mountpoints( *partition_temp );
	set_used_sectors( *partition_temp, NULL );

	device.partitions.push_back_adopt( partition_temp );
}

void GParted_Core::set_luks_partition( PartitionLUKS & partition )
{
	LUKS_Mapping mapping = LUKS_Info::get_cache_entry( partition.get_path() );
	if ( mapping.name.empty() )
		// No LUKS mapping found so no device file with which to query the
		// encrypted file system.  Assume no open dm-crypt mapping exists.
		// Details of encrypted file system left blank.
		return;

	Glib::ustring mapping_path = DEV_MAPPER_PATH + mapping.name;
	PedDevice* lp_device = NULL;
	std::vector<Glib::ustring> detect_messages;
	FSType fstype = FS_UNKNOWN;
	if ( get_device( mapping_path, lp_device ) )
	{
		fstype = detect_filesystem( lp_device, NULL, detect_messages );
		PedDisk* lp_disk = NULL;
		destroy_device_and_disk( lp_device, lp_disk );
	}
	bool fs_busy = is_busy( fstype, mapping_path );

	partition.set_luks( mapping_path,
	                    fstype,
	                    mapping.offset / partition.sector_size,
	                    mapping.length / partition.sector_size,
	                    partition.sector_size,
	                    fs_busy );

	Partition & encrypted = partition.get_encrypted();
	encrypted.append_messages( detect_messages );

	set_partition_label_and_uuid( encrypted );
	set_mountpoints( encrypted );
	set_used_sectors( encrypted, NULL );
}

void GParted_Core::set_partition_label_and_uuid( Partition & partition )
{
	Glib::ustring partition_path = partition.get_path();

	// For SWRaid members only get the label and UUID from SWRaid_Info.  Never use
	// values from FS_Info to avoid showing incorrect information in cases where blkid
	// reports the wrong values.
	if ( partition.filesystem == FS_LINUX_SWRAID )
	{
		Glib::ustring label = SWRaid_Info::get_label( partition_path );
		if ( ! label.empty() )
			partition.set_filesystem_label( label );

		partition.uuid = SWRaid_Info::get_uuid( partition_path );
		return;
	}

	// Retrieve file system label.  Use file system specific method first because it
	// has been that way since (#662537) to display Unicode labels correctly.
	// (#786502) adds support for reading Unicode labels through the FS_Info cache.
	// Either method should produce the same labels however the FS specific command is
	// run in the users locale where as blkid is run in the C locale.  Shouldn't
	// matter but who knows for sure!
	read_label( partition );
	if ( ! partition.filesystem_label_known() )
	{
		bool label_found = false;
		Glib::ustring label = FS_Info::get_label( partition_path, label_found );
		if ( label_found )
			partition.set_filesystem_label( label );
	}

	// Retrieve file system UUID.  Use cached method first in an effort to speed up
	// device scanning.
	partition.uuid = FS_Info::get_uuid( partition_path );
	if ( partition.uuid.empty() )
	{
		read_uuid( partition );
	}
}

// GParted simple internal file system signature detection.  Use sparingly.  Only when
// (old versions of) blkid and libparted don't recognise a signature.
FSType GParted_Core::detect_filesystem_internal( PedDevice * lp_device, PedPartition * lp_partition )
{
	char magic1[16];  // Big enough for largest signatures[].sig1 or sig2
	char magic2[16];
	FSType fstype = FS_UNKNOWN;

	char * buf = static_cast<char *>( malloc( lp_device->sector_size ) );
	if ( ! buf )
		return FS_UNKNOWN;

	if ( ! ped_device_open( lp_device ) )
	{
		free( buf );
		return FS_UNKNOWN;
	}

	struct {
		Byte_Value   offset1;
		const char * sig1;
		Byte_Value   offset2;
		const char * sig2;
		FSType       fstype;
	} signatures[] = {
		//offset1, sig1              , offset2, sig2  , fstype
		{ 65536LL, "ReIsEr4"         ,     0LL, NULL  , FS_REISER4        },
		{   512LL, "LABELONE"        ,   536LL, "LVM2", FS_LVM2_PV        },
		{     0LL, "LUKS\xBA\xBE"    ,     0LL, NULL  , FS_LUKS           },
		{ 65600LL, "_BHRfS_M"        ,     0LL, NULL  , FS_BTRFS          },
		{     3LL, "-FVE-FS-"        ,     0LL, NULL  , FS_BITLOCKER      },
		{  1030LL, "\x34\x34"        ,     0LL, NULL  , FS_NILFS2         },
		{     0LL, "\x52\x56\xBE\x1B",     0LL, NULL  , FS_GRUB2_CORE_IMG },
		{     0LL, "\x52\x56\xBE\x6F",     0LL, NULL  , FS_GRUB2_CORE_IMG },
		{     0LL, "\x52\xE8\x28\x01",     0LL, NULL  , FS_GRUB2_CORE_IMG },
		{     0LL, "\x52\xBF\xF4\x81",     0LL, NULL  , FS_GRUB2_CORE_IMG }
	};
	// For simple BitLocker recognition consider validation of BIOS Parameter block
	// fields unnecessary.
	// *   Detecting BitLocker
	//     http://blogs.msdn.com/b/si_team/archive/2006/10/26/detecting-bitlocker.aspx
	//
	// Recognise GRUB2 core.img just by any of the possible first 4 bytes of x86 CPU
	// instructions it starts with.
	// *   bootinfoscript v0.74 line 1963  [GRUB2 core.img possible staring 4 bytes]
	//     https://github.com/arvidjaar/bootinfoscript/blob/b1f711e39d7aa4021f4ad3d31468a8b1e1b3fda7/bootinfoscript#L1963

	for ( unsigned int i = 0 ; i < sizeof( signatures ) / sizeof( signatures[0] ) ; i ++ )
	{
		const size_t len1 = std::min( ( signatures[i].sig1 == NULL ) ? 0U : strlen( signatures[i].sig1 ),
		                              sizeof( magic1 ) );
		const size_t len2 = std::min( ( signatures[i].sig2 == NULL ) ? 0U : strlen( signatures[i].sig2 ),
		                              sizeof( magic2 ) );
		// NOTE: From this point onwards signatures[].sig1 and .sig2 are treated
		// as character buffers of known lengths len1 and len2, not NUL terminated
		// strings.
		if ( len1 == 0UL || ( signatures[i].sig2 != NULL && len2 == 0UL ) )
			continue;  // Don't allow 0 length signatures to match

		Sector start = 0LL;
		if ( lp_partition )
			start = lp_partition->geom.start;
		start += signatures[i].offset1 / lp_device->sector_size;

		memset( buf, 0, lp_device->sector_size );
		if ( ped_device_read( lp_device, buf, start, 1 ) != 0 )
		{
			memcpy( magic1, buf + signatures[i].offset1 % lp_device->sector_size, len1 );

			// WARNING: This assumes offset2 is in the same sector as offset1
			if ( signatures[i].sig2 != NULL )
			{
				memcpy( magic2, buf + signatures[i].offset2 % lp_device->sector_size, len2 );
			}

			if ( memcmp( magic1, signatures[i].sig1, len1 ) == 0     &&
			     ( signatures[i].sig2 == NULL ||
			       memcmp( magic2, signatures[i].sig2, len2 ) == 0 )     )
			{
				fstype = signatures[i].fstype;
				break;
			}
		}
	}

	ped_device_close( lp_device );
	free( buf );

	return fstype;
}

FSType GParted_Core::detect_filesystem( PedDevice * lp_device, PedPartition * lp_partition,
                                        std::vector<Glib::ustring> & messages )
{
	Glib::ustring fsname = "";
	Glib::ustring path;

	if ( lp_partition )
		// Will query partition using methods: (Q1) SWRaid, (Q2) blkid,
		// (Q3) libparted, (Q4) internal
		path = get_partition_path( lp_partition );
	else
		// Will query whole disk device using methods: (Q1) SWRaid, (Q2) blkid,
		// (Q4) internal
		path = lp_device->path;

	// (Q1) Linux Software RAID member detection
	if ( SWRaid_Info::is_member( path ) )
		return FS_LINUX_SWRAID;

	// (Q2) FS_Info (blkid) file system detection
	// Blkid detects more signatures and generally has less limitations so use before
	// libparted detection, but it doesn't report anything for extended partitions.
	fsname = FS_Info::get_fs_type( path );

	// (Q3) Libparted file system detection
	// Only used when blkid didn't report anything and only on partitions, not whole
	// disk devices.  (Doesn't detect anything on non-512 byte sector devices before
	// libparted 3.2).
	if ( fsname.empty() && lp_partition && lp_partition->fs_type )
		fsname = lp_partition->fs_type->name;

	if ( ! fsname.empty() )
	{
		if ( fsname == "extended" )
			return GParted::FS_EXTENDED ;
		else if ( fsname == "btrfs" )
			return GParted::FS_BTRFS ;
		else if ( fsname == "exfat" )
			return GParted::FS_EXFAT ;
		else if ( fsname == "ext2" )
			return GParted::FS_EXT2 ;
		else if ( fsname == "ext3" )
			return GParted::FS_EXT3 ;
		else if ( fsname == "ext4"    ||
		          fsname == "ext4dev"    )
			return GParted::FS_EXT4 ;
		else if ( fsname == "linux-swap"      ||
		          fsname == "linux-swap(v1)"  ||
		          fsname == "linux-swap(new)" ||
		          fsname == "linux-swap(v0)"  ||
		          fsname == "linux-swap(old)" ||
		          fsname == "swap"               )
			return GParted::FS_LINUX_SWAP ;
		else if ( fsname == "crypto_LUKS" )
			return FS_LUKS;
		else if ( fsname == "LVM2_member" )
			return GParted::FS_LVM2_PV ;
		else if ( fsname == "f2fs" )
			return GParted::FS_F2FS ;
		else if ( fsname == "fat16" )
			return GParted::FS_FAT16 ;
		else if ( fsname == "fat32" )
			return GParted::FS_FAT32 ;
		else if ( fsname == "nilfs2" )
			return GParted::FS_NILFS2 ;
		else if ( fsname == "ntfs" )
			return GParted::FS_NTFS ;
		else if ( fsname == "reiserfs" )
			return GParted::FS_REISERFS ;
		else if ( fsname == "xfs" )
			return GParted::FS_XFS ;
		else if ( fsname == "jfs" )
			return GParted::FS_JFS ;
		else if ( fsname == "hfs" )
			return GParted::FS_HFS ;
		else if ( fsname == "hfs+"    ||
		          fsname == "hfsx"    ||
		          fsname == "hfsplus"    )
			return GParted::FS_HFSPLUS ;
		else if ( fsname == "udf" )
			return GParted::FS_UDF;
		else if ( fsname == "ufs" )
			return GParted::FS_UFS ;
		else if ( fsname == "iso9660" )
			return FS_ISO9660;
		else if ( fsname == "linux_raid_member" )
			return FS_LINUX_SWRAID ;
		else if ( fsname == "swsusp"    ||
		          fsname == "swsuspend"    )
			return FS_LINUX_SWSUSPEND ;
		else if ( fsname == "ReFS" )
			return FS_REFS;
		else if ( fsname == "zfs_member" )
			return FS_ZFS;
	}

	// (Q4) Fallback to GParted simple internal file system detection
	FSType fstype = detect_filesystem_internal( lp_device, lp_partition );
	if ( fstype != FS_UNKNOWN )
		return fstype;

	//no file system found....
	Glib::ustring  temp = _( "Unable to detect file system! Possible reasons are:" ) ;
	temp += "\n- "; 
	temp += _( "The file system is damaged" ) ;
	temp += "\n- " ; 
	temp += _( "The file system is unknown to GParted" ) ;
	temp += "\n- "; 
	temp += _( "There is no file system available (unformatted)" ) ; 
	temp += "\n- "; 
	/* TO TRANSLATORS: looks like  The device entry /dev/sda5 is missing */
	temp += String::ucompose( _("The device entry %1 is missing"), path );
	
	messages .push_back( temp ) ;
	
	return GParted::FS_UNKNOWN ;
}
	
void GParted_Core::read_label( Partition & partition )
{
	FileSystem* p_filesystem = NULL;
	switch ( get_fs( partition.filesystem ).read_label )
	{
		case FS::EXTERNAL:
			p_filesystem = get_filesystem_object( partition.filesystem );
			if ( p_filesystem )
				p_filesystem->read_label( partition );
			break;

		default:
			break;
	}
}

void GParted_Core::read_uuid( Partition & partition )
{
	FileSystem* p_filesystem = NULL;
	switch ( get_fs( partition.filesystem ).read_uuid )
	{
		case FS::EXTERNAL:
			p_filesystem = get_filesystem_object( partition.filesystem );
			if ( p_filesystem )
				p_filesystem->read_uuid( partition );
			break;

		default:
			break;
	}
}

void GParted_Core::insert_unallocated( const Glib::ustring & device_path,
                                       PartitionVector & partitions,
                                       Sector start,
                                       Sector end,
                                       Byte_Value sector_size,
                                       bool inside_extended )
{
	//if there are no partitions at all..
	if ( partitions .empty() )
	{
		Partition * partition_temp = new Partition();
		partition_temp->Set_Unallocated( device_path, start, end, sector_size, inside_extended );
		partitions.push_back_adopt( partition_temp );
		return ;
	}
		
	//start <---> first partition start
	if ( (partitions .front() .sector_start - start) > (MEBIBYTE / sector_size) )
	{
		Sector temp_end = partitions.front().sector_start - 1;
		Partition * partition_temp = new Partition();
		partition_temp->Set_Unallocated( device_path, start, temp_end, sector_size, inside_extended );
		partitions.insert_adopt( partitions.begin(), partition_temp );
	}
	
	//look for gaps in between
	for ( unsigned int t =0 ; t < partitions .size() -1 ; t++ )
	{
		if (   ( ( partitions[ t + 1 ] .sector_start - partitions[ t ] .sector_end - 1 ) > (MEBIBYTE / sector_size) )
		    || (   ( partitions[ t + 1 ] .type != TYPE_LOGICAL )  // Only show exactly 1 MiB if following partition is not logical.
		        && ( ( partitions[ t + 1 ] .sector_start - partitions[ t ] .sector_end - 1 ) == (MEBIBYTE / sector_size) )
		       )
		   )
		{
			Sector temp_start = partitions[t].sector_end + 1;
			Sector temp_end   = partitions[t+1].sector_start - 1;
			Partition * partition_temp = new Partition();
			partition_temp->Set_Unallocated( device_path, temp_start, temp_end,
			                                 sector_size, inside_extended );
			partitions.insert_adopt( partitions.begin() + ++t, partition_temp );
		}
	}

	//last partition end <---> end
	if ( (end - partitions .back() .sector_end) >= (MEBIBYTE / sector_size) )
	{
		Sector temp_start = partitions.back().sector_end + 1;
		Partition * partition_temp = new Partition();
		partition_temp->Set_Unallocated( device_path, temp_start, end, sector_size, inside_extended );
		partitions.push_back_adopt( partition_temp );
	}
}

void GParted_Core::set_mountpoints( Partition & partition )
{
#ifndef USE_LIBPARTED_DMRAID
	DMRaid dmraid ;	//Use cache of dmraid device information
#endif

	if ( partition.filesystem == FS_LVM2_PV )
	{
		Glib::ustring vgname = LVM2_PV_Info::get_vg_name( partition.get_path() );
		if ( ! vgname.empty() )
			partition.add_mountpoint( vgname );
	}
	else if ( partition.filesystem == FS_LINUX_SWRAID )
	{
		Glib::ustring array_path = SWRaid_Info::get_array( partition.get_path() );
		if ( ! array_path.empty() )
			partition.add_mountpoint( array_path );
	}
	else if ( partition.filesystem == FS_LUKS )
	{
		LUKS_Mapping mapping = LUKS_Info::get_cache_entry( partition.get_path() );
		if ( ! mapping.name.empty() )
			partition.add_mountpoint( DEV_MAPPER_PATH + mapping.name );
	}
	// Swap spaces don't have mount points so don't bother trying to add them.
	else if ( supported_filesystem( partition.filesystem ) && partition.filesystem != FS_LINUX_SWAP )
	{
		if ( partition.busy )
		{
#ifndef USE_LIBPARTED_DMRAID
			// Handle dmraid devices differently because there may be more
			// than one partition name.
			// E.g., there might be names with and/or without a 'p' between
			//       the device name and partition number.
			if ( dmraid.is_dmraid_device( partition.device_path ) )
			{
				// Try device_name + partition_number
				Glib::ustring dmraid_path = partition.device_path +
				                            Utils::num_to_str( partition.partition_number );
				if ( set_mountpoints_helper( partition, dmraid_path ) )
					return;

				// Try device_name + p + partition_number
				dmraid_path = partition.device_path + "p" +
				              Utils::num_to_str( partition.partition_number );
				if ( set_mountpoints_helper( partition, dmraid_path ) )
					return;
			}
			else
#endif
			{
				// Normal device, not DMRaid device
				if ( set_mountpoints_helper( partition, partition.get_path() ) )
					return;
			}

			if ( partition.get_mountpoints().empty() )
				partition.push_back_message( _("Unable to find mount point") );
		}
		else  // Not busy file system
		{
			partition.add_mountpoints( Mount_Info::get_fstab_mountpoints( partition.get_path() ) );
		}
	}
}

bool GParted_Core::set_mountpoints_helper( Partition & partition, const Glib::ustring & path )
{
	Glib::ustring search_path ;
	if ( partition .filesystem == FS_BTRFS )
		search_path = btrfs::get_mount_device( path ) ;
	else
		search_path = path ;

	const std::vector<Glib::ustring> & mountpoints = Mount_Info::get_mounted_mountpoints( search_path );
	if ( mountpoints.size() )
	{
		partition.add_mountpoints( mountpoints );
		return true ;
	}

	return false;
}

//Report whether the partition is busy (mounted/active)
bool GParted_Core::is_busy( FSType fstype, const Glib::ustring & path )
{
	FileSystem * p_filesystem = NULL ;
	bool busy = false ;

	if ( supported_filesystem( fstype ) )
	{
		switch ( get_fs( fstype ) .busy )
		{
			case FS::GPARTED:
				//Search GParted internal mounted partitions map
				busy = Mount_Info::is_dev_mounted( path );
				break ;

			case FS::EXTERNAL:
				//Call file system specific method
				p_filesystem = get_filesystem_object( fstype ) ;
				if ( p_filesystem )
					busy = p_filesystem -> is_busy( path ) ;
				break;

			default:
				break ;
		}
	}
	else
	{
		//Still search GParted internal mounted partitions map in case an
		//  unknown file system is mounted
		busy = Mount_Info::is_dev_mounted( path );

		//Custom checks for recognised but other not-supported file system types
		busy |= ( fstype == FS_LINUX_SWRAID && SWRaid_Info::is_member_active( path ) );
	}

	return busy ;
}

void GParted_Core::set_used_sectors( Partition & partition, PedDisk* lp_disk )
{
	if ( supported_filesystem( partition.filesystem ) )
	{
		FileSystem* p_filesystem = NULL;
		if ( partition.busy )
		{
			switch( get_fs( partition.filesystem ).online_read )
			{
				case FS::EXTERNAL:
					p_filesystem = get_filesystem_object( partition.filesystem );
					if ( p_filesystem )
						p_filesystem->set_used_sectors( partition );
					break;
				case FS::GPARTED:
					mounted_set_used_sectors( partition );
					break;
				default:
					break;
			}
		}
		else  // Not busy file system
		{
			switch( get_fs( partition.filesystem ).read )
			{
				case FS::EXTERNAL:
					p_filesystem = get_filesystem_object( partition.filesystem );
					if ( p_filesystem )
						p_filesystem->set_used_sectors( partition );
					break;
#ifdef HAVE_LIBPARTED_FS_RESIZE
				case FS::LIBPARTED:
					if ( lp_disk )
						LP_set_used_sectors( partition, lp_disk );
					break;
#endif
				default:
					break;
			}
		}

		Sector unallocated;
		// Only confirm that the above code succeeded in setting the sector usage
		// values for this base Partition object, hence the explicit call to the
		// base Partition class sector_usage_known() method.  For LUKS this avoids
		// calling derived PartitionLUKS class sector_usage_known() which also
		// checks for known sector usage in the encrypted file system.  But that
		// wasn't set by the above code so in the case of luks/unknown would
		// produce a false positive.
		if ( ! partition.Partition::sector_usage_known() )
		{
			Glib::ustring temp = _("Unable to read the contents of this file system!");
			temp += "\n";
			temp += _("Because of this some operations may be unavailable.");
			if ( ! Utils::get_filesystem_software( partition.filesystem ).empty() )
			{
				temp += "\n";
				temp += _("The cause might be a missing software package.");
				temp += "\n";
				/*TO TRANSLATORS: looks like   The following list of software packages is required for NTFS file system support:  ntfsprogs. */
				temp += String::ucompose( _("The following list of software packages is required for %1 file system support:  %2."),
				                          Utils::get_filesystem_string( partition.filesystem ),
				                          Utils::get_filesystem_software( partition.filesystem )
				                        );
			}
			partition.push_back_message( temp );
		}
		else if ( ( unallocated = partition.get_sectors_unallocated() ) > 0 )
		{
			/* TO TRANSLATORS: looks like   1.28GiB of unallocated space within the partition. */
			Glib::ustring temp = String::ucompose( _("%1 of unallocated space within the partition."),
			                                       Utils::format_size( unallocated, partition.sector_size ) );
			FS fs = get_fs( partition.filesystem );
			if ( fs.check != FS::NONE && fs.grow != FS::NONE )
			{
				temp += "\n";
				/* TO TRANSLATORS: To grow the file system to fill the partition, select the partition and choose the menu item:
				 * means that the user can perform a check of the partition which will
				 * also grow the file system to fill the partition.
				 */
				temp += _("To grow the file system to fill the partition, select the partition and choose the menu item:");
				temp += "\n";
				temp += _("Partition --> Check.");
			}
			partition.push_back_message( temp );
		}

		if ( filesystem_resize_disallowed( partition ) )
		{
			Glib::ustring temp = get_filesystem_object( partition.filesystem )
			                     ->get_custom_text( CTEXT_RESIZE_DISALLOWED_WARNING );
			if ( ! temp.empty() )
				partition.push_back_message( temp );
		}
	}
}

void GParted_Core::mounted_set_used_sectors( Partition & partition )
{
	if ( partition .get_mountpoints() .size() > 0 )
	{
		Byte_Value fs_size ;
		Byte_Value fs_free ;
		Glib::ustring error_message ;
		if ( Utils::get_mounted_filesystem_usage( partition .get_mountpoint(),
		                                          fs_size, fs_free, error_message ) == 0 )
			partition .set_sector_usage( fs_size / partition .sector_size,
			                             fs_free / partition .sector_size ) ;
		else
			partition.push_back_message( error_message );
	}
}

#ifdef HAVE_LIBPARTED_FS_RESIZE
void GParted_Core::LP_set_used_sectors( Partition & partition, PedDisk* lp_disk )
{
	PedFileSystem *fs = NULL;
	PedConstraint *constraint = NULL;

	if ( lp_disk )
	{
		PedPartition* lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition .get_sector() ) ;
		
		if ( lp_partition )
		{
			fs = ped_file_system_open( & lp_partition ->geom ); 	
					
			if ( fs )
			{
				constraint = ped_file_system_get_resize_constraint( fs ) ;
				if ( constraint )
				{
					partition .set_sector_usage( fs ->geom ->length,
					                             fs ->geom ->length - constraint ->min_size ) ;
					
					ped_constraint_destroy( constraint );
				}
												
				ped_file_system_close( fs ) ;
			}
		}
	}
}
#endif

void GParted_Core::set_flags( Partition & partition, PedPartition* lp_partition )
{
	for ( unsigned int t = 0 ; t < flags .size() ; t++ )
		if ( ped_partition_is_flag_available( lp_partition, flags[ t ] ) &&
		     ped_partition_get_flag( lp_partition, flags[ t ] ) )
			partition .flags .push_back( ped_partition_flag_get_name( flags[ t ] ) ) ;
}

bool GParted_Core::create( Partition & new_partition, OperationDetail & operationdetail )
{
	bool success;
	if ( new_partition.type == TYPE_EXTENDED )
	{
		success = create_partition( new_partition, operationdetail );
	}
	else
	{
		FS_Limits fs_limits = get_filesystem_limits( new_partition.filesystem, new_partition );
		success = create_partition( new_partition, operationdetail,
		                            fs_limits.min_size / new_partition.sector_size );
	}
	if ( ! success )
		return false;

	if ( ! new_partition.name.empty() )
	{
		if ( ! name_partition( new_partition, operationdetail ) )
			return false;
	}

	if ( new_partition.type == TYPE_EXTENDED        ||
	     new_partition.filesystem == FS_UNFORMATTED    )
		return true;
	else if ( new_partition.filesystem == FS_CLEARED )
		return erase_filesystem_signatures( new_partition, operationdetail );
	else
		return    erase_filesystem_signatures( new_partition, operationdetail )
		       && set_partition_type( new_partition, operationdetail )
		       && create_filesystem( new_partition, operationdetail );

	return false;
}

bool GParted_Core::create_partition( Partition & new_partition, OperationDetail & operationdetail, Sector min_size )
{
	operationdetail .add_child( OperationDetail( _("create empty partition") ) ) ;
	
	new_partition .partition_number = 0 ;
	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device_and_disk( new_partition .device_path, lp_device, lp_disk ) )
	{
		PedPartitionType type;
		PedConstraint *constraint = NULL ;
		PedFileSystemType* fs_type = NULL ;
		
		//create new partition
		switch ( new_partition .type )
		{
			case GParted::TYPE_PRIMARY:
				type = PED_PARTITION_NORMAL ;
				break ;
			case GParted::TYPE_LOGICAL:
				type = PED_PARTITION_LOGICAL ;
				break ;
			case GParted::TYPE_EXTENDED:
				type = PED_PARTITION_EXTENDED ;
				break ;
				
			default	:
				type = PED_PARTITION_FREESPACE;
		}

		if ( new_partition .type != GParted::TYPE_EXTENDED )
			fs_type = ped_file_system_type_get( "ext2" ) ;

		PedPartition* lp_partition = ped_partition_new( lp_disk,
						  type,
						  fs_type,
						  new_partition .sector_start,
						  new_partition .sector_end ) ;
	
		if ( lp_partition )
		{
			if (   new_partition .alignment == ALIGN_STRICT
			    || new_partition .alignment == ALIGN_MEBIBYTE
			   )
			{
				PedGeometry *geom = ped_geometry_new( lp_device,
								      new_partition .sector_start,
								      new_partition .get_sector_length() ) ;

				if ( geom )
				{
					constraint = ped_constraint_exact( geom ) ;
					ped_geometry_destroy( geom );
				}
			}
			else
				constraint = ped_constraint_any( lp_device );
			
			if ( constraint )
			{
				if (   min_size > 0
				    && new_partition .filesystem != FS_XFS // Permit copying to smaller xfs partition
				   )
					constraint ->min_size = min_size ;
		
				if ( ped_disk_add_partition( lp_disk, lp_partition, constraint ) && commit( lp_disk ) )
				{
					new_partition.set_path( get_partition_path( lp_partition ) );

					new_partition .partition_number = lp_partition ->num ;
					new_partition .sector_start = lp_partition ->geom .start ;
					new_partition .sector_end = lp_partition ->geom .end ;
					
					operationdetail .get_last_child() .add_child( OperationDetail( 
						/* TO TRANSLATORS: looks like   path: /dev/sda1 (partition)
						 * This is showing the name and the fact
						 * that it is a partition within a device.
						 */
						String::ucompose( _("path: %1 (%2)"),
						                  new_partition.get_path(), _("partition") ) + "\n" +
						String::ucompose( _("start: %1"), new_partition .sector_start ) + "\n" +
						String::ucompose( _("end: %1"), new_partition .sector_end ) + "\n" +
						String::ucompose( _("size: %1 (%2)"),
								new_partition .get_sector_length(),
								Utils::format_size( new_partition .get_sector_length(), new_partition .sector_size ) ),
						STATUS_NONE,
						FONT_ITALIC ) ) ;
				}
			
				ped_constraint_destroy( constraint );
			}
		}
				
		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

	bool succes = new_partition .partition_number > 0 ;

#ifndef USE_LIBPARTED_DMRAID
	//create dev map entries if dmraid
	DMRaid dmraid ;
	if ( succes && dmraid .is_dmraid_device( new_partition .device_path ) )
		succes = dmraid .create_dev_map_entries( new_partition, operationdetail .get_last_child() ) ;
#endif

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;
}
	
bool GParted_Core::create_filesystem( const Partition & partition, OperationDetail & operationdetail ) 
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a create file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	operationdetail .add_child( OperationDetail( String::ucompose(
							/*TO TRANSLATORS: looks like create new ext3 file system */ 
							_("create new %1 file system"),
							Utils::get_filesystem_string( partition .filesystem ) ) ) ) ;
	
	bool succes = false ;
	FileSystem* p_filesystem = NULL ;
	switch ( get_fs( partition .filesystem ) .create )
	{
		case GParted::FS::NONE:
			break ;
		case GParted::FS::GPARTED:
			break ;
		case GParted::FS::LIBPARTED:
			break ;
		case GParted::FS::EXTERNAL:
			succes = ( p_filesystem = get_filesystem_object( partition .filesystem ) ) &&
				 p_filesystem ->create( partition, operationdetail .get_last_child() ) ;

			break ;

		default:
			break ;
	}

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;
}

bool GParted_Core::format( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a format file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	if ( partition .filesystem == FS_CLEARED )
		return erase_filesystem_signatures( partition, operationdetail ) ;
	else
		return    erase_filesystem_signatures( partition, operationdetail )
		       && set_partition_type( partition, operationdetail )
		       && create_filesystem( partition, operationdetail ) ;
}

bool GParted_Core::delete_partition( const Partition & partition, OperationDetail & operationdetail )
{
	operationdetail .add_child( OperationDetail( _("delete partition") ) ) ;

	bool succes = false ;
	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device_and_disk( partition .device_path, lp_device, lp_disk ) )
	{
		PedPartition* lp_partition = get_lp_partition( lp_disk, partition );

		succes = ped_disk_delete_partition( lp_disk, lp_partition ) && commit( lp_disk ) ;
	
		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

#ifndef USE_LIBPARTED_DMRAID
	//delete partition dev mapper entry, and delete and recreate all other affected dev mapper entries if dmraid
	DMRaid dmraid ;
	if ( succes && dmraid .is_dmraid_device( partition .device_path ) )
	{
		PedDevice* lp_device = NULL ;
		PedDisk* lp_disk = NULL ;
		//Open disk handle before and close after to prevent application crash.
		if ( get_device_and_disk( partition .device_path, lp_device, lp_disk ) )
		{
			if ( ! dmraid .delete_affected_dev_map_entries( partition, operationdetail .get_last_child() ) )
				succes = false ;	//comand failed

			if ( ! dmraid .create_dev_map_entries( partition, operationdetail .get_last_child() ) )
				succes = false ;	//command failed

			destroy_device_and_disk( lp_device, lp_disk ) ;
		}
	}
#endif

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;
}

bool GParted_Core::remove_filesystem( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a delete file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	bool success = true ;
	FileSystem* p_filesystem = NULL ;

	switch ( get_fs( partition .filesystem ) .remove )
	{
		case FS::EXTERNAL:
			//Run file system specific remove method to delete the file system.  Most
			//  file systems should NOT implement a remove() method as it will prevent
			//  recovery from accidental partition deletion.
			operationdetail .add_child( OperationDetail( String::ucompose(
								_("delete %1 file system"),
								Utils::get_filesystem_string( partition .filesystem ) ) ) ) ;
			success = ( p_filesystem = get_filesystem_object( partition .filesystem ) ) &&
			          p_filesystem ->remove( partition, operationdetail .get_last_child() ) ;
			operationdetail.get_last_child().set_success_and_capture_errors( success );
			break ;

		default:
			break ;
	}
	return success ;
}

bool GParted_Core::label_filesystem( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a label file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	if( partition.get_filesystem_label().empty() ) {
		operationdetail.add_child( OperationDetail(
			String::ucompose( _("Clear file system label on %1"), partition.get_path() ) ) );
	} else {
		operationdetail.add_child( OperationDetail(
			String::ucompose( _("Set file system label to \"%1\" on %2"),
			                  partition.get_filesystem_label(), partition.get_path() ) ) );
	}

	bool succes = false ;
	FileSystem* p_filesystem = NULL ;
	switch ( get_fs( partition.filesystem ).write_label )
	{
		case FS::EXTERNAL:
			succes =    ( p_filesystem = get_filesystem_object( partition.filesystem ) )
			         && p_filesystem->write_label( partition, operationdetail.get_last_child() );
			break;

		default:
			break;
	}

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;	
}

bool GParted_Core::name_partition( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.name.empty() )
		operationdetail.add_child( OperationDetail(
				String::ucompose( _("Clear partition name on %1"), partition.get_path() ) ) );
	else
		operationdetail.add_child( OperationDetail(
				String::ucompose( _("Set partition name to \"%1\" on %2"),
				                  partition.name, partition.get_path() ) ) );

	bool success = false;
	PedDevice *lp_device = NULL;
	PedDisk *lp_disk = NULL;
	if ( get_device_and_disk( partition.device_path, lp_device, lp_disk ) )
	{
		PedPartition *lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition.get_sector() );
		if ( lp_partition )
		{
			success =    ped_partition_set_name( lp_partition, partition.name.c_str() )
			          && commit( lp_disk );
		}
	}

	operationdetail.get_last_child().set_success_and_capture_errors( success );
	return success;
}

bool GParted_Core::change_filesystem_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a change file system UUID only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	if ( partition .uuid == UUID_RANDOM_NTFS_HALF ) {
		operationdetail .add_child( OperationDetail( String::ucompose(
										_("Set half of the UUID on %1 to a new, random value"),
										 partition .get_path()
									 ) ) ) ;
	} else {
		operationdetail .add_child( OperationDetail( String::ucompose(
										_("Set UUID on %1 to a new, random value"),
										 partition .get_path()
									 ) ) ) ;
	}

	bool succes = false ;
	FileSystem* p_filesystem = NULL ;
	switch ( get_fs( partition.filesystem ).write_uuid )
	{
		case FS::EXTERNAL:
			succes =    ( p_filesystem = get_filesystem_object( partition.filesystem ) )
			         && p_filesystem->write_uuid( partition, operationdetail.get_last_child() );
			break;

		default:
			break;
	}

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;
}

bool GParted_Core::resize_move( const Partition & partition_old,
                                Partition & partition_new,
                                OperationDetail & operationdetail )
{
	if (   (partition_new .alignment == ALIGN_STRICT)
	    || (partition_new .alignment == ALIGN_MEBIBYTE)
	    || partition_new .strict_start
	    || calculate_exact_geom( partition_old, partition_new, operationdetail )
	   )
	{
		if ( partition_old .type == TYPE_EXTENDED )
			return resize_move_partition( partition_old, partition_new, operationdetail, false );

		if ( partition_new .sector_start == partition_old .sector_start )
			return resize( partition_old, partition_new, operationdetail ) ;

		if ( partition_new .get_sector_length() == partition_old .get_sector_length() )
		{
			return move( partition_old, partition_new, operationdetail );
		}
		Partition * temp = NULL;
		if ( partition_new .get_sector_length() > partition_old .get_sector_length() )
		{
			//first move, then grow. Since old.length < new.length and new.start is valid, temp is valid.
			temp = partition_new.clone();
			temp->sector_end = temp->sector_start + partition_old.get_sector_length() - 1;
		}
		else  // ( partition_new.get_sector_length() < partition_old.get_sector_length() )
		{
			//first shrink, then move. Since new.length < old.length and old.start is valid, temp is valid.
			temp = partition_old.clone();
			temp->sector_end = partition_old.sector_start + partition_new.get_sector_length() - 1;
		}

		PartitionAlignment previous_alignment = temp->alignment;
		temp->alignment = ALIGN_STRICT;
		bool success = resize_move( partition_old, *temp, operationdetail );
		temp->alignment = previous_alignment;
		if ( success )
			success = resize_move( *temp, partition_new, operationdetail );

		delete temp;
		temp = NULL;

		return success;
	}

	return false ;
}

bool GParted_Core::move( const Partition & partition_old,
                         const Partition & partition_new,
                         OperationDetail & operationdetail )
{
	if ( partition_old .get_sector_length() != partition_new .get_sector_length() )
	{	
		operationdetail .add_child( OperationDetail(
			/* TO TRANSLATORS:
			 * means that GParted has encountered a programming bug and tried
			 * to change the size of a partition when performing a move only
			 * step which is not permitted to change the partition size.
			 */
			GPARTED_BUG + ": " + _("size of the partition is changing for a move only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false ;
	}

	if ( ! check_repair_filesystem( partition_old, operationdetail ) )
		return false;

	// NOTE:
	// Logical partitions are preceded by meta data.  To prevent this meta data from
	// being overwritten we first expand the partition to encompass all of the space
	// involved in the move.  In this way we prevent overwriting the meta data for
	// this partition when we move this partition to the left.  We also prevent
	// overwriting the meta data of a following partition when we move this partition
	// to the right.
	Partition * partition_all_space = partition_old.clone();
	partition_all_space->alignment = ALIGN_STRICT;
	if ( partition_new.sector_start < partition_all_space->sector_start )
		partition_all_space->sector_start = partition_new.sector_start;
	if ( partition_new.sector_end > partition_all_space->sector_end )
		partition_all_space->sector_end = partition_new.sector_end;

	// Make old partition all encompassing and if move file system fails then return
	// partition table to original state
	bool success = false;
	if ( resize_move_partition( partition_old, *partition_all_space, operationdetail, true ) )
	{
		// Note move of file system is from old values to new values, not from the
		// all encompassing values.
		if ( ! move_filesystem( partition_old, partition_new, operationdetail ) )
		{
			operationdetail.add_child( OperationDetail( _("rollback last change to the partition") ) );

			Partition * partition_restore = partition_old.clone();
			partition_restore->alignment = ALIGN_STRICT;  // Ensure that old partition boundaries are not modified
			if ( resize_move_partition( *partition_all_space, *partition_restore,
			                            operationdetail.get_last_child(), false ) )
			{
				operationdetail.get_last_child().set_success_and_capture_errors( true );
				check_repair_filesystem( partition_old, operationdetail );
			}
			else
			{
				operationdetail.get_last_child().set_success_and_capture_errors( false );
			}

			delete partition_restore;
			partition_restore = NULL;
		}
		else
		{
			success = true;
		}
	}

	// Make new partition from all encompassing partition
	if ( success )
	{
		success =    resize_move_partition( *partition_all_space, partition_new, operationdetail, false )
		          && update_bootsector( partition_new, operationdetail );
	}

	delete partition_all_space;
	partition_all_space = NULL;

	if ( ! success )
		return false;

	if ( partition_new.filesystem == FS_LINUX_SWAP )
		// linux-swap is recreated, not moved
		return recreate_linux_swap_filesystem( partition_new, operationdetail );

	return true;
}

bool GParted_Core::move_filesystem( const Partition & partition_old,
		   		    const Partition & partition_new,
				    OperationDetail & operationdetail ) 
{
	if ( partition_new .sector_start < partition_old .sector_start )
		operationdetail .add_child( OperationDetail( _("move file system to the left") ) ) ;
	else if ( partition_new .sector_start > partition_old .sector_start )
		operationdetail .add_child( OperationDetail( _("move file system to the right") ) ) ;
	else
	{
		operationdetail .add_child( OperationDetail( _("move file system") ) ) ;
		operationdetail .get_last_child() .add_child( 
			OperationDetail( _("new and old file system have the same position.  Hence skipping this operation"),
					 STATUS_NONE,
					 FONT_ITALIC ) ) ;

		operationdetail.get_last_child().set_success_and_capture_errors( true );
		return true ;
	}

	bool succes = false ;
	FileSystem* p_filesystem = NULL ;
	Sector total_done = 0;
	switch ( get_fs( partition_old .filesystem ) .move )
	{
		case GParted::FS::NONE:
			break ;
		case GParted::FS::GPARTED:
			succes = false ;
			if ( partition_new .test_overlap( partition_old ) )
			{
				succes = copy_filesystem_internal( partition_old,
				                                   partition_new,
				                                   operationdetail.get_last_child(),
				                                   total_done,
				                                   true );

				operationdetail.get_last_child().get_last_child()
					.set_success_and_capture_errors( succes );
				if ( ! succes )
				{
					rollback_move_filesystem( partition_old,
					                          partition_new,
					                          operationdetail.get_last_child(),
					                          total_done );
				}
			}
			else
				succes = copy_filesystem_internal( partition_old,
				                                   partition_new,
				                                   operationdetail.get_last_child(),
				                                   total_done,
				                                   true );

			break ;
		case GParted::FS::LIBPARTED:
			break ;
		case GParted::FS::EXTERNAL:
			succes = ( p_filesystem = get_filesystem_object( partition_new .filesystem ) ) &&
			         p_filesystem ->move( partition_old
			                            , partition_new
			                            , operationdetail .get_last_child()
			                            ) ;
			break ;

		default:
			break ;
	}

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;
}

#ifdef HAVE_LIBPARTED_FS_RESIZE
bool GParted_Core::resize_move_filesystem_using_libparted( const Partition & partition_old,
		  	      		            	   const Partition & partition_new,
						    	   OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( _("using libparted"), STATUS_NONE ) ) ;

	bool return_value = false ;
	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device_and_disk( partition_old .device_path, lp_device, lp_disk ) )
	{
		PedFileSystem *	fs = NULL ;
		PedGeometry * lp_geom = NULL ;	
		
		lp_geom = ped_geometry_new( lp_device,
					    partition_old .sector_start,
					    partition_old .get_sector_length() ) ;
		if ( lp_geom )
		{
			fs = ped_file_system_open( lp_geom );

			ped_geometry_destroy( lp_geom );
			lp_geom = NULL;

			if ( fs )
			{
				lp_geom = ped_geometry_new( lp_device,
							    partition_new .sector_start,
							    partition_new .get_sector_length() ) ;
				if ( lp_geom )
				{
					// Use thread for libparted FS resize call to avoid blocking GUI
					Glib::Thread::create( sigc::bind<PedFileSystem *, PedGeometry *, bool *>(
					                          sigc::mem_fun( *this, &GParted_Core::thread_lp_ped_file_system_resize ),
					                          fs,
					                          lp_geom,
					                          &return_value ),
					                      false );
					Gtk::Main::run();

					if ( return_value )
						commit( lp_disk ) ;

					ped_geometry_destroy( lp_geom );
				}

				ped_file_system_close( fs );
			}
		}

		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

	return return_value ;
}

void GParted_Core::thread_lp_ped_file_system_resize( PedFileSystem * fs,
                                                     PedGeometry * lp_geom,
                                                     bool * return_value )
{
	*return_value = ped_file_system_resize( fs, lp_geom, NULL );
	g_idle_add( (GSourceFunc)_mainquit, NULL );
}
#endif

bool GParted_Core::resize( const Partition & partition_old, 
			   const Partition & partition_new,
			   OperationDetail & operationdetail )
{
	if ( partition_old .sector_start != partition_new .sector_start )
	{	
		operationdetail .add_child( OperationDetail( 
			/* TO TRANSLATORS:
			 * means that GParted has encountered a programming bug and tried
			 * to move the start of the partition when performing a resize
			 * only step which is not permitted to change the start of the
			 * partition.
			 */
			GPARTED_BUG + ": " + _("start of the partition is changing for a resize only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false ;
	}

	if ( partition_new.filesystem == FS_LUKS )
		return resize_encryption( partition_old, partition_new, operationdetail );
	else
		return resize_plain( partition_old, partition_new, operationdetail );
}

bool GParted_Core::resize_encryption( const Partition & partition_old,
                                      const Partition & partition_new,
                                      OperationDetail & operationdetail )
{
	if ( partition_old.filesystem != FS_LUKS )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition does not contain LUKS encryption for a resize encryption only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	Sector delta = partition_new.get_sector_length() - partition_old.get_sector_length();

	if ( ! partition_old.busy && delta < 0LL )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("impossible to shrink a closed LUKS encryption volume"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	if ( ! partition_old.busy )
	{
		// grow closed LUKS.
		// maximize_encryption() is only called to display no action needed
		// operation message generated in luks::resize() for this case.
		return    resize_move_partition( partition_old, partition_new, operationdetail, true )
		       && maximize_encryption( partition_new, operationdetail );
	}

	const Partition & filesystem_ptn_new = partition_new.get_filesystem_partition();

	if ( filesystem_ptn_new.filesystem == FS_LINUX_SWAP )
	{
		// LUKS is resized, but linux-swap is recreated, not resized
		if ( delta < 0LL )  // shrink
		{
			return    shrink_encryption( partition_old, partition_new, operationdetail )
			       && resize_move_partition( partition_old, partition_new, operationdetail, false )
			       && recreate_linux_swap_filesystem( filesystem_ptn_new, operationdetail );
		}
		else if ( delta > 0LL ) // grow
		{
			return    resize_move_partition( partition_old, partition_new, operationdetail, true )
			       && maximize_encryption( partition_new, operationdetail )
			       && recreate_linux_swap_filesystem( filesystem_ptn_new, operationdetail );
		}
	}

	const Partition & filesystem_ptn_old = partition_old.get_filesystem_partition();
	if ( delta < 0LL )  // shrink
	{
		return    check_repair_filesystem( filesystem_ptn_old, operationdetail )
		       && shrink_filesystem( filesystem_ptn_old, filesystem_ptn_new, operationdetail )
		       && shrink_encryption( partition_old, partition_new, operationdetail )
		       && resize_move_partition( partition_old, partition_new, operationdetail, false );
	}
	else if ( delta > 0LL )  // grow
	{
		return    check_repair_filesystem( filesystem_ptn_old, operationdetail )
		       && resize_move_partition( partition_old, partition_new, operationdetail, true )
		       && maximize_encryption( partition_new, operationdetail )
		       && maximize_filesystem( filesystem_ptn_new, operationdetail );
	}

	return true;
}

bool GParted_Core::resize_plain( const Partition & partition_old,
                                 const Partition & partition_new,
                                 OperationDetail & operationdetail )
{
	if ( partition_old.filesystem == FS_LUKS && partition_old.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a resize file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	if ( partition_new.filesystem == FS_LINUX_SWAP )
	{
		// linux-swap is recreated, not resized
		return    resize_move_partition( partition_old, partition_new, operationdetail, true )
		       && recreate_linux_swap_filesystem( partition_new, operationdetail );
	}

	Sector delta = partition_new.get_sector_length() - partition_old.get_sector_length();
	if ( delta < 0LL )  // shrink
	{
		return    check_repair_filesystem( partition_new, operationdetail )
		       && shrink_filesystem( partition_old, partition_new, operationdetail )
		       && resize_move_partition( partition_old, partition_new, operationdetail, false );
	}
	else if ( delta > 0LL )  // grow
	{
		return    check_repair_filesystem( partition_new, operationdetail )
		       && resize_move_partition( partition_old, partition_new, operationdetail, true )
		       && maximize_filesystem( partition_new, operationdetail );
	}

	return true;
}

bool GParted_Core::resize_move_partition( const Partition & partition_old,
                                          const Partition & partition_new,
                                          OperationDetail & operationdetail,
                                          bool rollback_on_fail )
{
	if ( partition_new.type == TYPE_UNPARTITIONED )
		// Trying to resize/move a non-partitioned whole disk device is a
		// successful non-operation.
		return true;

	//i'm not too happy with this, but i think it is the correct way from a i18n POV
	enum Action
	{
		NONE			= 0,
		MOVE_RIGHT	 	= 1,
		MOVE_LEFT		= 2,
		GROW 			= 3,
		SHRINK			= 4,
		MOVE_RIGHT_GROW		= 5,
		MOVE_RIGHT_SHRINK	= 6,
		MOVE_LEFT_GROW		= 7,
		MOVE_LEFT_SHRINK	= 8
	} ;
	Action action = NONE ;

	if ( partition_new .get_sector_length() > partition_old .get_sector_length() )
		action = GROW ;
	else if ( partition_new .get_sector_length() < partition_old .get_sector_length() )
		action = SHRINK ;

	if ( partition_new .sector_start > partition_old .sector_start &&
	     partition_new .sector_end > partition_old .sector_end )
		action = action == GROW ? MOVE_RIGHT_GROW : action == SHRINK ? MOVE_RIGHT_SHRINK : MOVE_RIGHT ;
	else if ( partition_new .sector_start < partition_old .sector_start &&
	     partition_new .sector_end < partition_old .sector_end )
		action = action == GROW ? MOVE_LEFT_GROW : action == SHRINK ? MOVE_LEFT_SHRINK : MOVE_LEFT ;

	Glib::ustring description ;	
	switch ( action )
	{
		case NONE		:
			description = _("resize/move partition") ;
			break ;
		case MOVE_RIGHT		:
			description = _("move partition to the right") ;
			break ;
		case MOVE_LEFT		:
			description = _("move partition to the left") ;
			break ;
		case GROW 		:
			description = _("grow partition from %1 to %2") ;
			break ;
		case SHRINK		:
			description = _("shrink partition from %1 to %2") ;
			break ;
		case MOVE_RIGHT_GROW	:
			description = _("move partition to the right and grow it from %1 to %2") ;
			break ;
		case MOVE_RIGHT_SHRINK	:
			description = _("move partition to the right and shrink it from %1 to %2") ;
			break ;
		case MOVE_LEFT_GROW	:
			description = _("move partition to the left and grow it from %1 to %2") ;
			break ;
		case MOVE_LEFT_SHRINK	:
			description = _("move partition to the left and shrink it from %1 to %2") ;
			break ;
	}

	if ( ! description .empty() && action != NONE && action != MOVE_LEFT && action != MOVE_RIGHT )
		description = String::ucompose( description,
						Utils::format_size( partition_old .get_sector_length(), partition_old .sector_size ),
						Utils::format_size( partition_new .get_sector_length(), partition_new .sector_size ) ) ;

	operationdetail .add_child( OperationDetail( description ) ) ;

	
	if ( action == NONE )
	{
		operationdetail .get_last_child() .add_child( 
			OperationDetail( _("new and old partition have the same size and position.  Hence skipping this operation"),
					  STATUS_NONE,
					  FONT_ITALIC ) ) ;

		operationdetail.get_last_child().set_success_and_capture_errors( true );
		return true ;
	}

	operationdetail .get_last_child() .add_child( 
		OperationDetail(
			String::ucompose( _("old start: %1"), partition_old .sector_start ) + "\n" +
			String::ucompose( _("old end: %1"), partition_old .sector_end ) + "\n" +
			String::ucompose( _("old size: %1 (%2)"),
					partition_old .get_sector_length(),
					Utils::format_size( partition_old .get_sector_length(), partition_old .sector_size ) ),
		STATUS_NONE, 
		FONT_ITALIC ) ) ;
	
	//finally the actual resize/move
	Sector new_start = -1;
	Sector new_end = -1;
	bool success = resize_move_partition_implement( partition_old, partition_new, new_start, new_end );
	if ( success )
	{
		//Change to partition succeeded
		operationdetail .get_last_child() .add_child( 
			OperationDetail(
				String::ucompose( _("new start: %1"), new_start ) + "\n" +
				String::ucompose( _("new end: %1"), new_end ) + "\n" +
				String::ucompose( _("new size: %1 (%2)"),
						new_end - new_start + 1,
						Utils::format_size( new_end - new_start + 1, partition_new .sector_size ) ),
			STATUS_NONE, 
			FONT_ITALIC ) ) ;

		//update dev mapper entry if partition is dmraid.
		success = success && update_dmraid_entry( partition_new, operationdetail );
	}
	else
	{
		//Change to partition failed
		operationdetail .get_last_child() .add_child(
			OperationDetail(
				String::ucompose( _("requested start: %1"), partition_new .sector_start ) + "\n" +
				String::ucompose( _("requested end: %1"), partition_new . sector_end ) + "\n" +
				String::ucompose( _("requested size: %1 (%2)"),
						partition_new .get_sector_length(),
						Utils::format_size( partition_new .get_sector_length(), partition_new .sector_size ) ),
								STATUS_NONE,
								FONT_ITALIC )
								) ;
	}
	operationdetail.get_last_child().set_success_and_capture_errors( success );

	if ( ! success && rollback_on_fail )
	{
		operationdetail.add_child(
				OperationDetail( _("attempt to rollback failed change to the partition") ) );

		// Create a temporary partition which is the intersection of the old and
		// new partitions so that the middle sector can be used to identify the
		// partition needing rollback, whether or not the above failed partition
		// change made it to the drive or not.
		Partition *partition_intersection = partition_new.clone();
		partition_intersection->sector_start = std::max( partition_old.sector_start,
		                                                 partition_new.sector_start );
		partition_intersection->sector_end   = std::min( partition_old.sector_end,
		                                                 partition_new.sector_end );

		Partition *partition_restore = partition_old.clone();
		// Ensure that old partition boundaries are not modified
		partition_restore->alignment = ALIGN_STRICT;

		bool rollback_success = resize_move_partition_implement( *partition_intersection, *partition_restore,
		                                                         new_start, new_end );

		operationdetail.get_last_child().add_child(
			OperationDetail(
				String::ucompose( _("original start: %1"), partition_restore->sector_start ) + "\n" +
				String::ucompose( _("original end: %1"), partition_restore->sector_end ) + "\n" +
				String::ucompose( _("original size: %1 (%2)"),
					partition_restore->get_sector_length(),
					Utils::format_size( partition_restore->get_sector_length(), partition_restore->sector_size ) ),
				STATUS_NONE, FONT_ITALIC ) );

		// Update dev mapper entry if partition is dmraid.
		rollback_success = rollback_success && update_dmraid_entry( *partition_restore, operationdetail );

		delete partition_restore;
		partition_restore = NULL;
		delete partition_intersection;
		partition_intersection = NULL;

		operationdetail.get_last_child().set_success_and_capture_errors( rollback_success );
	}

	return success;
}

bool GParted_Core::resize_move_partition_implement( const Partition & partition_old,
                                                    const Partition & partition_new,
                                                    Sector & new_start,
                                                    Sector & new_end )
{
	bool success = false;
	PedDevice *lp_device = NULL;
	PedDisk *lp_disk = NULL;
	if ( get_device_and_disk( partition_old.device_path, lp_device, lp_disk ) )
	{
		PedPartition *lp_partition = get_lp_partition( lp_disk, partition_old );
		if ( lp_partition )
		{
			PedConstraint *constraint = NULL;
			if ( partition_new.alignment == ALIGN_STRICT   ||
			     partition_new.alignment == ALIGN_MEBIBYTE ||
			     partition_new.strict_start                   )
			{
				PedGeometry *geom = ped_geometry_new( lp_device,
				                                      partition_new.sector_start,
				                                      partition_new.get_sector_length() );
				if ( geom )
				{
					constraint = ped_constraint_exact( geom );
					ped_geometry_destroy( geom );
				}
			}
			else
			{
				constraint = ped_constraint_any( lp_device );
			}

			if ( constraint )
			{
				if ( ped_disk_set_partition_geom( lp_disk,
				                                  lp_partition,
				                                  constraint,
				                                  partition_new.sector_start,
				                                  partition_new.sector_end ) )
				{
					new_start = lp_partition->geom.start;
					new_end = lp_partition->geom.end;

					success = commit( lp_disk );
				}

				ped_constraint_destroy( constraint );
			}
		}

		destroy_device_and_disk( lp_device, lp_disk );
	}

	return success;
}

bool GParted_Core::shrink_encryption( const Partition & partition_old,
                                      const Partition & partition_new,
                                      OperationDetail & operationdetail )
{
	if ( ! ( partition_old.filesystem == FS_LUKS && partition_old.busy ) )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition does not contain open LUKS encryption for a shrink encryption only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	operationdetail.add_child( OperationDetail( _("shrink encryption volume") ) );
	bool success = resize_filesystem_implement( partition_old, partition_new, operationdetail );
	operationdetail.get_last_child().set_success_and_capture_errors( success );
	return success;
}

bool GParted_Core::maximize_encryption( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.filesystem != FS_LUKS )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition does not contain LUKS encryption for a maximize encryption only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	operationdetail.add_child( OperationDetail( _("grow encryption volume to fill the partition") ) );

	// Checking if growing is allowed is only relevant for the check repair operation
	// to inform the user why the grow step is being skipped.  For a resize/move
	// operation these growing checks are merely retesting those performed to allow
	// the operation to be queued in the first place.  See
	// Win_GParted::set_valid_operations().
	if ( get_fs( partition.filesystem ).grow == FS::NONE )
	{
		operationdetail.get_last_child().add_child( OperationDetail(
				_("growing is not available for this encryption volume"),
				STATUS_NONE, FONT_ITALIC ) );
		operationdetail.get_last_child().set_status( STATUS_WARNING );
		return true;
	}

	bool success = resize_filesystem_implement( partition, partition, operationdetail );
	operationdetail.get_last_child().set_success_and_capture_errors( success );
	return success;
}

bool GParted_Core::shrink_filesystem( const Partition & partition_old,
                                      const Partition & partition_new,
                                      OperationDetail & operationdetail )
{
	if ( partition_old.filesystem == FS_LUKS && partition_old.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a shrink file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}
	if ( partition_new.get_sector_length() >= partition_old.get_sector_length() )
	{
		operationdetail.add_child( OperationDetail(
			/* TO TRANSLATORS:
			 * means that GParted has encountered a programming bug and tried
			 * to grow the partition size or keep it the same when performing
			 * a shrink partition only step.
			 */
			GPARTED_BUG + ": " + _("the new partition size is larger or the same for a shrink only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	operationdetail.add_child( OperationDetail( _("shrink file system") ) );
	bool success = resize_filesystem_implement( partition_old, partition_new, operationdetail );
	operationdetail.get_last_child().set_success_and_capture_errors( success );
	return success;
}

bool GParted_Core::maximize_filesystem( const Partition & partition, OperationDetail & operationdetail ) 
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a maximize file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	operationdetail .add_child( OperationDetail( _("grow file system to fill the partition") ) ) ;

	// Checking if growing is available or allowed is only relevant for the check
	// repair operation to inform the user why the grow step is being skipped.  For a
	// resize/move operation these growing checks are merely retesting those performed
	// to allow the operation to be queued in the first place.  See
	// Win_GParted::set_valid_operations() and
	// Dialog_Partition_Resize_Move::Resize_Move_Normal().
	if ( get_fs( partition .filesystem ) .grow == GParted::FS::NONE )
	{
		operationdetail .get_last_child() .add_child( 
			OperationDetail( _("growing is not available for this file system"),
					  STATUS_NONE,
					  FONT_ITALIC ) ) ;
		operationdetail.get_last_child().set_status( STATUS_WARNING );
		return true ;
	}
	else if ( filesystem_resize_disallowed( partition ) )
	{
		Glib::ustring msg        = _("growing the file system is currently disallowed") ;
		Glib::ustring custom_msg = get_filesystem_object( partition .filesystem )
		                           ->get_custom_text( CTEXT_RESIZE_DISALLOWED_WARNING ) ;
		if ( ! custom_msg .empty() )
		{
			msg += "\n" ;
			msg += custom_msg ;
		}
		operationdetail .get_last_child() .add_child( OperationDetail( msg, STATUS_NONE, FONT_ITALIC ) ) ;
		operationdetail.get_last_child().set_status( STATUS_WARNING );
		return true ;
	}

	bool success = resize_filesystem_implement( partition, partition, operationdetail );
	operationdetail.get_last_child().set_success_and_capture_errors( success );
	return success;
}

bool GParted_Core::recreate_linux_swap_filesystem( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.filesystem != FS_LINUX_SWAP )
	{
		operationdetail.add_child( OperationDetail(
			/* TO TRANSLATORS: looks like   not a linux-swap file system for a recreate linux-swap only step */
			GPARTED_BUG + ": " + String::ucompose( _("not a %1 file system for a recreate %1 only step"),
			                                       Utils::get_filesystem_string( FS_LINUX_SWAP ),
			                                       Utils::get_filesystem_string( FS_LINUX_SWAP ) ),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	if ( ! erase_filesystem_signatures( partition, operationdetail ) )
		return false;

	operationdetail.add_child( OperationDetail(
		/* TO TRANSLATORS: looks like   recreate linux-swap file system */
		String::ucompose( _("recreate %1 file system"),
		                  Utils::get_filesystem_string( FS_LINUX_SWAP ) ) ) );

	// Linux-swap is recreated by using the linux_swap::resize() method
	bool success = resize_filesystem_implement( partition, partition, operationdetail );
	operationdetail.get_last_child().set_success_and_capture_errors( success );
	return success;
}

bool GParted_Core::resize_filesystem_implement( const Partition & partition_old,
                                                const Partition & partition_new,
                                                OperationDetail & operationdetail )
{

	bool fill_partition = false;
	const FS & fs_cap = get_fs( partition_new.filesystem );
	FS::Support action = FS::NONE;
	if ( partition_new.get_sector_length() >= partition_old.get_sector_length() )
	{
		// grow (always maximises the file system to fill the partition)
		fill_partition = true;
		action = ( partition_old.busy ) ? fs_cap.online_grow : fs_cap.grow;
	}
	else
	{
		// shrink
		fill_partition = false;
		action = ( partition_old.busy ) ? fs_cap.online_shrink : fs_cap.shrink;
	}
	bool success = false;
	FileSystem* p_filesystem = NULL;
	switch ( action )
	{
		case FS::NONE:
			break;
		case FS::GPARTED:
			break;
#ifdef HAVE_LIBPARTED_FS_RESIZE
		case FS::LIBPARTED:
			success = resize_move_filesystem_using_libparted( partition_old,
			                                                  partition_new,
			                                                  operationdetail.get_last_child() );
			break;
#endif
		case FS::EXTERNAL:
			success = ( p_filesystem = get_filesystem_object( partition_new.filesystem ) ) &&
			          p_filesystem->resize( partition_new,
			                                operationdetail.get_last_child(),
			                                fill_partition );
			break;
		default:
			break;
	}

	return success;
}

bool GParted_Core::copy( const Partition & partition_src,
			 Partition & partition_dst,
			 OperationDetail & operationdetail ) 
{
	const Partition & filesystem_ptn_src = partition_src.get_filesystem_partition();
	Partition & filesystem_ptn_dst = partition_dst.get_filesystem_partition();

	if (   filesystem_ptn_dst.get_byte_length() < filesystem_ptn_src.get_byte_length()
	    && filesystem_ptn_src.filesystem != FS_XFS  // Permit copying to smaller xfs partition
	   )
	{
		operationdetail .add_child( OperationDetail( 
			_("the destination is smaller than the source partition"), STATUS_ERROR, FONT_ITALIC ) ) ;

		return false ;
	}

	if ( ! check_repair_filesystem( filesystem_ptn_src, operationdetail ) )
		return false;

	if ( partition_dst.status == STAT_COPY )
	{
		// Handle situation where src sector size is smaller than dst sector size
		// and an additional partial dst sector is required.
		Sector min_size = ( partition_src.get_byte_length() + partition_dst.sector_size - 1 ) /
		                  partition_dst.sector_size;
		if ( ! create_partition( partition_dst, operationdetail, min_size ) )
			return false;
	}

	// NOTE:
	// Deliberately call set_partition_type() on the Partition object directly
	// containing the file system, rather than the Partition object containing the
	// partition.  When copying into an existing open LUKS encryption mapping this
	// will avoid changing partition type, where as when copying into a plain
	// partition the type will be set.
	bool success =    set_partition_type( filesystem_ptn_dst, operationdetail )
	               && copy_filesystem( filesystem_ptn_src, filesystem_ptn_dst, operationdetail )
	               && update_bootsector( partition_dst, operationdetail );
	if ( ! success )
		return false;

	if ( filesystem_ptn_dst.filesystem == FS_LINUX_SWAP )
	{
		// linux-swap is recreated, not copied
		return recreate_linux_swap_filesystem( filesystem_ptn_dst, operationdetail );
	}
	else if ( filesystem_ptn_dst.get_byte_length() > filesystem_ptn_src.get_byte_length() )
	{
		// Copied into a bigger partition so maximise file system
		return    check_repair_filesystem( filesystem_ptn_dst, operationdetail )
		       && maximize_filesystem( filesystem_ptn_dst, operationdetail );
	}
	return true;
}

bool GParted_Core::copy_filesystem( const Partition & partition_src,
                                    Partition & partition_dst,
                                    OperationDetail & operationdetail )
{
	if ( partition_src.filesystem == FS_LUKS && partition_src.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("source partition contains open LUKS encryption for a file system copy only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}
	if ( partition_dst.filesystem == FS_LUKS && partition_dst.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("destination partition contains open LUKS encryption for a file system copy only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	operationdetail.add_child( OperationDetail(
			String::ucompose( _("copy file system from %1 to %2"),
			                  partition_src.get_path(),
			                  partition_dst.get_path() ) ) );

	bool success = false;
	FileSystem* p_filesystem = NULL;
	switch ( get_fs( partition_dst.filesystem ).copy )
	{
		case FS::GPARTED:
			success = copy_filesystem_internal( partition_src,
			                                    partition_dst,
			                                    operationdetail.get_last_child(),
			                                    true );
			break;

		case FS::LIBPARTED:
			// FIXME: see if copying through libparted has any advantages
			break;

		case FS::EXTERNAL:
			success = ( p_filesystem = get_filesystem_object( partition_dst.filesystem ) ) &&
			          p_filesystem->copy( partition_src,
			                              partition_dst,
			                              operationdetail.get_last_child() );
			break;

		default:
			break;
	}

	operationdetail.get_last_child().set_success_and_capture_errors( success );
	return success;
}

bool GParted_Core::copy_filesystem_internal( const Partition & partition_src,
                                             const Partition & partition_dst,
                                             OperationDetail & operationdetail,
                                             bool cancel_safe )
{
	Sector dummy ;
	return copy_blocks( partition_src.device_path,
	                    partition_dst.device_path,
	                    partition_src.sector_start,
	                    partition_dst.sector_start,
	                    partition_src.sector_size,
	                    partition_dst.sector_size,
	                    partition_src.get_byte_length(),
	                    operationdetail,
	                    dummy,
	                    cancel_safe );
}

bool GParted_Core::copy_filesystem_internal( const Partition & partition_src,
                                             const Partition & partition_dst,
                                             OperationDetail & operationdetail,
                                             Byte_Value & total_done,
                                             bool cancel_safe )
{
	return copy_blocks( partition_src.device_path,
	                    partition_dst.device_path,
	                    partition_src.sector_start,
	                    partition_dst.sector_start,
	                    partition_src.sector_size,
	                    partition_dst.sector_size,
	                    partition_src.get_byte_length(),
	                    operationdetail,
	                    total_done,
	                    cancel_safe );
}
	
bool GParted_Core::copy_blocks( const Glib::ustring & src_device,
                                const Glib::ustring & dst_device,
                                Sector src_start,
                                Sector dst_start,
                                Byte_Value src_sector_size,
                                Byte_Value dst_sector_size,
                                Byte_Value src_length,
                                OperationDetail & operationdetail,
                                Byte_Value & total_done,
                                bool cancel_safe )
{
	operationdetail .add_child( OperationDetail( _("using internal algorithm"), STATUS_NONE ) ) ;
	operationdetail .add_child( OperationDetail(
		String::ucompose( /*TO TRANSLATORS: looks like  copy 1.00 MiB */
		                  _("copy %1"), Utils::format_size( src_length, 1 ) ),
		STATUS_NONE ) ) ;

	operationdetail .add_child( OperationDetail( _("finding optimal block size"), STATUS_NONE ) ) ;

	Byte_Value benchmark_blocksize = (1 * MEBIBYTE) ;
	Byte_Value N = (16 * MEBIBYTE) ;
	Byte_Value optimal_blocksize = benchmark_blocksize ;
	Sector offset_read = src_start ;
	Sector offset_write = dst_start ;

	//Handle situation where we need to perform the copy beginning
	//  with the end of the partition and finishing with the start.
	if ( dst_start > src_start )
	{
		offset_read  += (src_length/src_sector_size) - (N/src_sector_size) ;
		/* Handle situation where src sector size is smaller than dst sector size and an additional partial dst sector is required. */
		offset_write += ((src_length + (dst_sector_size - 1))/dst_sector_size) - (N/dst_sector_size) ;
	}

	total_done = 0 ;
	Byte_Value done = 0 ;
	Glib::Timer timer ;
	double smallest_time = 1000000 ;
	bool succes = true ;
	OperationDetail & benchmark_od = operationdetail.get_last_child();

	//Benchmark copy times using different block sizes to determine optimal size
	while ( succes &&
		llabs( done ) + N <= src_length && 
		benchmark_blocksize <= N )
	{
		benchmark_od.add_child( OperationDetail(
				/*TO TRANSLATORS: looks like   copy 16.00 MiB using a block size of 1.00 MiB */
				String::ucompose( _("copy %1 using a block size of %2"),
				                  Utils::format_size( N, 1 ),
				                  Utils::format_size( benchmark_blocksize, 1 ) ) ) );

		timer .reset() ;
		succes = CopyBlocks( src_device,
		                     dst_device,
		                     offset_read  + (done / src_sector_size),
		                     offset_write + (done / dst_sector_size),
		                     N,
		                     benchmark_blocksize,
		                     benchmark_od,
		                     total_done,
		                     src_length,
		                     cancel_safe ).copy();
		timer.stop() ;

		benchmark_od.get_last_child().add_child( OperationDetail(
			String::ucompose( _("%1 seconds"), timer .elapsed() ), STATUS_NONE, FONT_ITALIC ) ) ;
		benchmark_od.get_last_child().set_success_and_capture_errors( succes );

		if ( timer .elapsed() <= smallest_time )
		{
			smallest_time = timer .elapsed() ;
			optimal_blocksize = benchmark_blocksize ; 
		}
		benchmark_blocksize *= 2 ;

		if ( ( dst_start > src_start ) )
			done -= N ;
		else
			done += N ;
	}
	
	if ( succes )
		operationdetail .get_last_child() .add_child( OperationDetail( String::ucompose( 
				/*TO TRANSLATORS: looks like  optimal block size is 1.00 MiB */
				_("optimal block size is %1"),
				Utils::format_size( optimal_blocksize, 1 ) ),
				STATUS_NONE ) ) ;

	if ( succes && llabs( done ) < src_length )
	{
		Byte_Value remaining_length = src_length - llabs( done );
		operationdetail.add_child( OperationDetail(
				/*TO TRANSLATORS: looks like   copy 16.00 MiB using a block size of 1.00 MiB */
				String::ucompose( _("copy %1 using a block size of %2"),
				                  Utils::format_size( remaining_length, 1 ),
				                  Utils::format_size( optimal_blocksize, 1 ) ) ) );
		succes = CopyBlocks( src_device,
		                     dst_device,
		                     src_start + ((done > 0 ? done : 0) / src_sector_size),
		                     dst_start + ((done > 0 ? done : 0) / dst_sector_size),
		                     remaining_length,
		                     optimal_blocksize,
		                     operationdetail,
		                     total_done,
		                     src_length,
		                     cancel_safe ).copy();
		operationdetail.get_last_child().set_success_and_capture_errors( succes );
	}

	operationdetail .add_child( OperationDetail( 
		String::ucompose( /*TO TRANSLATORS: looks like  1.00 MiB (1048576 B) copied */
		                  _("%1 (%2 B) copied"), Utils::format_size( total_done, 1 ), total_done ),
		STATUS_NONE ) ) ;
	return succes ;
}

void GParted_Core::rollback_move_filesystem( const Partition & partition_src,
                                             const Partition & partition_dst,
                                             OperationDetail & operationdetail,
                                             Byte_Value total_done )
{
	if ( total_done > 0 )
	{
		//find out exactly which part of the file system was copied (and to where it was copied)..
		Partition * temp_src = partition_src.clone();
		Partition * temp_dst = partition_dst.clone();
		bool rollback_needed = true;

		if ( partition_dst .sector_start > partition_src .sector_start )
		{
			Sector distance = partition_dst.sector_start - partition_src.sector_start;
			temp_src->sector_start = temp_src->sector_end - ( (total_done / temp_src->sector_size) - 1 ) + distance;
			temp_dst->sector_start = temp_dst->sector_end - ( (total_done / temp_dst->sector_size) - 1 ) + distance;
			if ( temp_src->sector_start > temp_src->sector_end )
				// Nothing has been overwritten yet, so nothing to roll back
				rollback_needed = false;
		}
		else
		{
			Sector distance = partition_src.sector_start - partition_dst.sector_start;
			temp_src->sector_end = temp_src->sector_start + ( (total_done / temp_src->sector_size) - 1 ) - distance;
			temp_dst->sector_end = temp_dst->sector_start + ( (total_done / temp_dst->sector_size) - 1 ) - distance;
			if ( temp_src->sector_start > temp_src->sector_end )
				// Nothing has been overwritten yet, so nothing to roll back
				rollback_needed = false;
		}

		if ( rollback_needed )
		{
			operationdetail.add_child( OperationDetail( _("rollback failed file system move") ) );

			//and copy it back (NOTE the reversed dst and src)
			bool success = copy_filesystem_internal( *temp_dst,
			                                         *temp_src,
			                                         operationdetail.get_last_child(),
			                                         false );

			operationdetail.get_last_child().set_success_and_capture_errors( success );
		}

		delete temp_src;
		delete temp_dst;
		temp_src = NULL;
		temp_dst = NULL;
	}
}

bool GParted_Core::check_repair_filesystem( const Partition & partition, OperationDetail & operationdetail ) 
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for a check file system only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	if ( partition.busy )
		// Trying to check an online file system is a successful non-operation.
		return true;

	operationdetail .add_child( OperationDetail( 
				String::ucompose(
						/* TO TRANSLATORS: looks like   check file system on /dev/sda5 for errors and (if possible) fix them */
						_("check file system on %1 for errors and (if possible) fix them"),
						  partition .get_path() ) ) ) ;
	
	bool succes = false ;
	FileSystem* p_filesystem = NULL ;
	switch ( get_fs( partition .filesystem ) .check )
	{
		case GParted::FS::NONE:
			operationdetail .get_last_child() .add_child(
				OperationDetail( _("checking is not available for this file system"),
						 STATUS_NONE,
						 FONT_ITALIC ) ) ;
			operationdetail.get_last_child().set_status( STATUS_WARNING );
			return true ;	

			break ;
		case GParted::FS::GPARTED:
			break ;
		case GParted::FS::LIBPARTED:
			break ;
		case GParted::FS::EXTERNAL:
			succes = ( p_filesystem = get_filesystem_object( partition .filesystem ) ) &&
				 p_filesystem ->check_repair( partition, operationdetail .get_last_child() ) ;

			break ;

		default:
			break ;
	}

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;
}

bool GParted_Core::check_repair_maximize( const Partition & partition,
                                          OperationDetail & operationdetail )
{
	if ( partition.filesystem == FS_LUKS )
	{
		// Pretend that the LUKS partition is closed so that
		// resize_filesystem_implement() checks the offline .grow capability
		// rather than .online_grow so that maximising LUKS in the context of a
		// check encrypted file system operation, which doesn't want to change the
		// partition boundaries, can be performed even if libparted and/or the
		// kernel aren't new enough to support online partition resizing.
		// luks::resize() determines the open status of the LUKS mapping by
		// querying the LUKS_Info module cache so doesn't care if the partition
		// busy status is wrong.
		Partition * temp_offline = partition.clone();
		temp_offline->busy = false;
		bool success = maximize_encryption( *temp_offline, operationdetail );
		delete temp_offline;
		temp_offline = NULL;
		if ( ! success )
			return false;

		if ( partition.busy )
			success = maximize_filesystem( partition.get_filesystem_partition(), operationdetail );
		return success;
	}
	else
	{
		return maximize_filesystem( partition, operationdetail );
	}
}

bool GParted_Core::set_partition_type( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.type == TYPE_UNPARTITIONED )
		// Trying to set the type of a partition on a non-partitioned whole disk
		// device is a successful non-operation.
		return true;

	operationdetail .add_child( OperationDetail(
				String::ucompose( _("set partition type on %1"), partition .get_path() ) ) ) ;
	//Set partition type appropriately for the type of file system stored in the partition.
	//  Libparted treats every type as a file system, except LVM which it treats as a flag.

	bool return_value = false ;
	
	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device_and_disk( partition .device_path, lp_device, lp_disk ) )
	{
		PedPartition* lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition.get_sector() );
		if ( lp_partition )
		{
			Glib::ustring fs_type = Utils::get_filesystem_string( partition.filesystem );

			// Lookup libparted file system type using GParted's name, as most match
			PedFileSystemType * lp_fs_type = ped_file_system_type_get( fs_type.c_str() );

			// If not found, and FS is linux-swap, then try linux-swap(v1)
			if ( ! lp_fs_type && fs_type == "linux-swap" )
				lp_fs_type = ped_file_system_type_get( "linux-swap(v1)" );

			// If not found, and FS is linux-swap, then try linux-swap(new)
			if ( ! lp_fs_type && fs_type == "linux-swap" )
				lp_fs_type = ped_file_system_type_get( "linux-swap(new)" );

			// If not found, and FS is udf, then try ntfs.
			// Actually MBR 07 IFS (Microsoft Installable File System) or
			// GPT BDP (Windows Basic Data Partition).
			// Ref: https://serverfault.com/a/829172
			if ( ! lp_fs_type && fs_type == "udf" )
				lp_fs_type = ped_file_system_type_get( "ntfs" );

			// default is Linux (83)
			if ( ! lp_fs_type )
				lp_fs_type = ped_file_system_type_get( "ext2" );

			bool supports_lvm_flag = ped_partition_is_flag_available( lp_partition, PED_PARTITION_LVM );

			if ( lp_fs_type && partition.filesystem != FS_LVM2_PV )
			{
				// Also clear any libparted LVM flag so that it doesn't
				// override the file system type
				if ( ( ! supports_lvm_flag                                          ||
				       ped_partition_set_flag( lp_partition, PED_PARTITION_LVM, 0 )    ) &&
				     ped_partition_set_system( lp_partition, lp_fs_type )                &&
				     commit( lp_disk )                                                      )
				{
					operationdetail.get_last_child().add_child(
						/* TO TRANSLATORS: looks like   new partition type: ext4 */
						OperationDetail( String::ucompose( _("new partition type: %1"),
						                                   lp_partition->fs_type->name ),
						                 STATUS_NONE,
						                 FONT_ITALIC ) );
					return_value = true;
				}
			}
			else if ( partition.filesystem == FS_LVM2_PV )
			{
				if ( supports_lvm_flag                                            &&
				     ped_partition_set_flag( lp_partition, PED_PARTITION_LVM, 1 ) &&
				     commit( lp_disk )                                               )
				{
					operationdetail.get_last_child().add_child(
						/* TO TRANSLATORS: looks like   new partition flag: lvm */
						OperationDetail( String::ucompose( _("new partition flag: %1"),
						                                   ped_partition_flag_get_name( PED_PARTITION_LVM ) ),
						                 STATUS_NONE,
						                 FONT_ITALIC ) );
					return_value = true;
				}
				else if ( ! supports_lvm_flag )
				{
					// Skip setting the lvm flag because the partition
					// table type doesn't support it.  Applies to dvh
					// and pc98 disk labels.
					return_value = true;
				}
			}
		}

		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

	operationdetail.get_last_child().set_success_and_capture_errors( return_value );
	return return_value ;
}

bool GParted_Core::calibrate_partition( Partition & partition, OperationDetail & operationdetail ) 
{
	if ( partition.type == TYPE_PRIMARY  || partition.type == TYPE_LOGICAL       ||
	     partition.type == TYPE_EXTENDED || partition.type == TYPE_UNPARTITIONED    )
	{
		Glib::ustring curr_path = partition.get_path();
		operationdetail.add_child( OperationDetail( String::ucompose( _("calibrate %1"), curr_path ) ) );
	
		bool success = false;
		PedDevice* lp_device = NULL ;
		PedDisk* lp_disk = NULL ;
		if ( get_device( partition.device_path, lp_device ) )
		{	
			if ( partition.type == TYPE_UNPARTITIONED )
			{
				// Virtual partition spanning whole disk device
				success = true;
			}
			else if ( get_disk( lp_device, lp_disk ) )
			{
				// Partitioned device
				PedPartition *lp_partition = get_lp_partition( lp_disk, partition );
				if ( lp_partition )  // FIXME: add check to see if lp_partition->type matches partition.type..
				{
					if ( ! file_test( curr_path, Glib::FILE_TEST_EXISTS ) )
					{
						// Re-set the real partition path from libparted.
						//
						// When creating a copy operation by pasting into
						// unallocated space the path for the partition
						// object was set to "Copy of /dev/SRC" because
						// the partition didn't yet exist before the
						// operations were applied.  Additional operations
						// on that new partition also got the path set to
						// "Copy of /dev/SRC".  This re-sets the real path
						// to "/dev/NEW".  This provides the real path for
						// file system specific tools used during those
						// additional operations such mkfs for the format
						// operation or fsck and others for the
						// resize/move operation.
						partition.set_path( get_partition_path( lp_partition ) );
					}

					// Reload the partition boundaries from libparted
					// to ensure that GParted knows what the actual
					// partition boundaries are before applying the
					// next operation.  Necessary when the previous
					// operation in the sequence was a resize/move
					// operation where GParted may have only estimated
					// where libparted would move the boundaries to.
					partition.sector_start = lp_partition->geom.start;
					partition.sector_end   = lp_partition->geom.end;

					success = true;
				}
			}
			// else error from get_disk() reading partition table

			if ( success )
			{
				operationdetail .get_last_child() .add_child( 
					OperationDetail(
						/* TO TRANSLATORS: looks like   path: /dev/sda (device)
						 *              or looks like   path: /dev/sda1 (partition)
						 * This is showing the name and whether it
						 * is a whole disk device or a partition
						 * within a device.
						 */
						String::ucompose( _("path: %1 (%2)"),
						                  partition.get_path(),
						                  ( partition.type == TYPE_UNPARTITIONED )
						                                             ? _("device")
						                                             : _("partition") ) + "\n" +
						String::ucompose( _("start: %1"), partition .sector_start ) + "\n" +
						String::ucompose( _("end: %1"), partition .sector_end ) + "\n" +
						String::ucompose( _("size: %1 (%2)"),
								partition .get_sector_length(),
								Utils::format_size( partition .get_sector_length(), partition .sector_size ) ),
					STATUS_NONE, 
					FONT_ITALIC ) ) ;

				if ( partition.filesystem == FS_LUKS && partition.busy )
				{
					const Partition & encrypted = dynamic_cast<const PartitionLUKS *>( &partition )->get_encrypted();
					operationdetail.get_last_child().add_child( OperationDetail(
						String::ucompose( _("encryption path: %1"), encrypted.get_path() ),
						STATUS_NONE, FONT_ITALIC ) );
				}
			}

			destroy_device_and_disk( lp_device, lp_disk ) ;
		}

		// (#762941) Above libparted partition querying triggers udev >= 219 to
		// remove and re-add all the partition specific /dev/ entries.  Wait for
		// this to complete to avoid FS specific commands failing because they
		// happen to run just when the needed /dev/PTN entry doesn't exist.
		settle_device( SETTLE_DEVICE_APPLY_MAX_WAIT_SECONDS );

		operationdetail.get_last_child().set_success_and_capture_errors( success );
		return success;
	}
	else //nothing to calibrate...
		return true ;
}

bool GParted_Core::calculate_exact_geom( const Partition & partition_old,
			       	         Partition & partition_new,
				         OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( 
		String::ucompose( _("calculate new size and position of %1"), partition_new .get_path() ) ) ) ;

	operationdetail .get_last_child() .add_child( 
		OperationDetail(
			String::ucompose( _("requested start: %1"), partition_new .sector_start ) + "\n" +
			String::ucompose( _("requested end: %1"), partition_new .sector_end ) + "\n" +
			String::ucompose( _("requested size: %1 (%2)"),
					partition_new .get_sector_length(),
					Utils::format_size( partition_new .get_sector_length(), partition_new .sector_size ) ),
		STATUS_NONE,
		FONT_ITALIC ) ) ;
	
	bool succes = false ;
	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	if ( get_device_and_disk( partition_old .device_path, lp_device, lp_disk ) )
	{
		PedPartition* lp_partition = get_lp_partition( lp_disk, partition_old );
		if ( lp_partition )
		{
			PedConstraint *constraint = NULL ;
			constraint = ped_constraint_any( lp_device ) ;
			
			if ( constraint )
			{
				//FIXME: if we insert a weird partitionnew geom here (e.g. start > end) 
				//ped_disk_set_partition_geom() will still return true (althoug an lp exception is written
				//to stdout.. see if this also affect create_partition and resize_move_partition
				//sended a patch to fix this to libparted list. will probably be in 1.7.2
				if ( ped_disk_set_partition_geom( lp_disk,
								  lp_partition,
								  constraint,
								  partition_new .sector_start,
								  partition_new .sector_end ) )
				{
					partition_new .sector_start = lp_partition ->geom .start ;
					partition_new .sector_end = lp_partition ->geom .end ;
					succes = true ;
				}
					
				ped_constraint_destroy( constraint );
			}
		}

		destroy_device_and_disk( lp_device, lp_disk ) ;
	}

	if ( succes ) 
	{
		operationdetail .get_last_child() .add_child( 
			OperationDetail(
				String::ucompose( _("new start: %1"), partition_new .sector_start ) + "\n" +
				String::ucompose( _("new end: %1"), partition_new .sector_end ) + "\n" +
				String::ucompose( _("new size: %1 (%2)"),
						partition_new .get_sector_length(),
						Utils::format_size( partition_new .get_sector_length(), partition_new .sector_size ) ),
			STATUS_NONE,
			FONT_ITALIC ) ) ;

		//Update dev mapper entry if partition is dmraid.
		succes = succes && update_dmraid_entry( partition_new, operationdetail );
	}

	operationdetail.get_last_child().set_success_and_capture_errors( succes );
	return succes ;
}

bool GParted_Core::update_dmraid_entry( const Partition & partition, OperationDetail & operationdetail )
{
	bool success = true;
#ifndef USE_LIBPARTED_DMRAID
	DMRaid dmraid;
	if ( dmraid.is_dmraid_device( partition.device_path ) )
	{
		PedDevice *lp_device = NULL;
		PedDisk *lp_disk;
		// Open disk handle before and close after to prevent application crash.
		if ( get_device_and_disk( partition.device_path, lp_device, lp_disk ) )
		{
			success = dmraid.update_dev_map_entry( partition, operationdetail.get_last_child() );
			destroy_device_and_disk( lp_device, lp_disk );
		}
	}
#endif
	return success;
}

FileSystem * GParted_Core::get_filesystem_object( FSType filesystem )
{
	if ( FILESYSTEM_MAP .count( filesystem ) )
	    return FILESYSTEM_MAP[ filesystem ] ;
	else
	    return NULL ;
}

// Return true for file systems with an implementation class, false otherwise
bool GParted_Core::supported_filesystem( FSType fstype )
{
	return get_filesystem_object( fstype ) != NULL;
}

FS_Limits GParted_Core::get_filesystem_limits( FSType fstype, const Partition & partition )
{
	FileSystem *p_filesystem = get_filesystem_object( fstype );
	FS_Limits fs_limits;
	if ( p_filesystem != NULL )
		fs_limits = p_filesystem->get_filesystem_limits( partition );
	return fs_limits;
}

bool GParted_Core::filesystem_resize_disallowed( const Partition & partition )
{
	if ( partition .filesystem == FS_LVM2_PV )
	{
		//The LVM2 PV can't be resized when it's a member of an export VG
		Glib::ustring vgname = LVM2_PV_Info::get_vg_name( partition.get_path() );
		if ( vgname .empty() )
			return false ;
		return LVM2_PV_Info::is_vg_exported( vgname );
	}
	return false ;
}

bool GParted_Core::erase_filesystem_signatures( const Partition & partition, OperationDetail & operationdetail )
{
	if ( partition.filesystem == FS_LUKS && partition.busy )
	{
		operationdetail.add_child( OperationDetail(
			GPARTED_BUG + ": " + _("partition contains open LUKS encryption for an erase file system signatures only step"),
			STATUS_ERROR, FONT_ITALIC ) );
		return false;
	}

	bool overall_success = false ;
	operationdetail .add_child( OperationDetail(
			String::ucompose( _("clear old file system signatures in %1"),
			                  partition .get_path() ) ) ) ;

	//Get device, disk & partition and open the device.  Allocate buffer and fill with
	//  zeros.  Buffer size is the greater of 4 KiB and the sector size.
	PedDevice* lp_device = NULL ;
	PedDisk* lp_disk = NULL ;
	PedPartition* lp_partition = NULL ;
	bool device_is_open = false ;
	Byte_Value bufsize = 4LL * KIBIBYTE ;
	char * buf = NULL ;
	if ( get_device( partition.device_path, lp_device ) )
	{
		if ( partition.type == TYPE_UNPARTITIONED )
		{
			// Virtual partition spanning whole disk device
			overall_success = true;
		}
		else if ( get_disk( lp_device, lp_disk ) )
		{
			// Partitioned device
			lp_partition = ped_disk_get_partition_by_sector( lp_disk, partition.get_sector() );
			overall_success = ( lp_partition != NULL );
		}

		if ( overall_success && ped_device_open( lp_device ) )
		{
			device_is_open = true ;

			bufsize = std::max( bufsize, lp_device ->sector_size ) ;
			buf = static_cast<char *>( malloc( bufsize ) ) ;
			if ( buf )
				memset( buf, 0, bufsize ) ;
		}
		overall_success &= device_is_open;
	}

	//Erase all file system super blocks, including their signatures.  The specified
	//  byte ranges are converted to whole sectors (as disks fundamentally only read
	//  or write whole sectors) and written using ped_geometry_write().  Therefore
	//  don't try to surgically overwrite just the few bytes of each signature as this
	//  code overwrites whole sectors and it embeds more knowledge that is necessary.
	//
	//  First byte range from offset 0 of length 512 KiB covers the primary super
	//  block of all currently supported file systems and is also likely to include
	//  future file system super blocks too.  Only a few file systems have super
	//  blocks and signatures located elsewhere.
	//
	//  Btrfs super blocks are located at: 64 KiB, 64 MiB, 256 GiB and 1 PiB.
	//  (The super block at 64 KiB will be erased by the zeroing from offset 0.  Other
	//  super block mirror copies need to be explicitly cleared).
	//  https://btrfs.wiki.kernel.org/index.php/On-disk_Format#Superblock
	//
	//  ZFS labels are 256 KiB in size and 4 copies are stored on every member device,
	//  2 at the beginning and 2 at the end of the device, aligned to 256 KiB.  (The
	//  front two labels, L0 and L1, will be erased by the zeroing from offset 0.  The
	//  back two labels, L2 and L3, need to be explicitly cleared).
	//  Reference:
	//      ZFS On-Disk Specification
	//      http://maczfs.googlecode.com/files/ZFSOnDiskFormat.pdf
	//
	//  Linux Software RAID metadata 0.90 stores it's super block at 64 KiB before the
	//  end of the device, aligned to 64 KiB boundary.  Length 4 KiB.
	//  Ref: mdadm/super0.c load_super0()
	//       mdadm/md_p.h   #define MD_NEW_SIZE_SECTORS(x) ...
	//
	//  Linux Software RAID metadata 1.0 stores it's super block at 8 KiB before the
	//  end of the device, aligned to 4 KiB boundary.  Length 4 KiB.  (Metadata 1.1
	//  and 1.2 store their super blocks at 0 KiB and 4 KiB respectively so will be
	//  erased by the zeroing from offset 0).
	//  Ref: mdadm/super1.c load_super1()
	//                      #define MAX_SB_SIZE 4096
	//
	//  Nilfs2 secondary super block is located at the last whole 4 KiB block.
	//  Ref: nilfs-utils-2.1.4/include/nilfs2_fs.h
	//  #define NILFS_SB2_OFFSET_BYTES(devsize) ((((devsize) >> 12) - 1) << 12)
	//
	// NOTE:
	// Most of the time partitions are aligned to whole MiBs so the writing of zeros
	// at offsets -64 KiB and -8 KiB will be overwriting zeros already just written
	// at offset -512 KiB, length 512 KiB.  This will double write 12 KiB of zeros.
	// However partitions don't have to be MiB aligned and real disk drives generally
	// aren't an exact multiple of 1024^2 bytes in size either.  So zeroing at offsets
	// -64 KiB and -8 KiB with their smaller rounding / alignment requirements will
	// write outside the -512 KiB zeroed region and needs to be kept.  Because of the
	// small amount of double writing it is not worth the effort to suppress it when
	// not needed.
	struct {
		Byte_Value offset;    //Negative offsets work backwards from the end of the partition
		Byte_Value rounding;  //Minimum desired rounding for offset
		Byte_Value length;
	} ranges[] = {
		//offset           , rounding       , length
		{    0LL           ,   1LL           , 512LL * KIBIBYTE },  // All primary super blocks
		{   64LL * MEBIBYTE,   1LL           ,   4LL * KIBIBYTE },  // Btrfs super block mirror copy
		{  256LL * GIBIBYTE,   1LL           ,   4LL * KIBIBYTE },  // Btrfs super block mirror copy
		{    1LL * PEBIBYTE,   1LL           ,   4LL * KIBIBYTE },  // Btrfs super block mirror copy
		{ -512LL * KIBIBYTE, 256LL * KIBIBYTE, 512LL * KIBIBYTE },  // ZFS labels L2 and L3
		{  -64LL * KIBIBYTE,  64LL * KIBIBYTE,   4LL * KIBIBYTE },  // SWRaid metadata 0.90 super block
		{   -8LL * KIBIBYTE,   4LL * KIBIBYTE,   8LL * KIBIBYTE }   // @-8K SWRaid metadata 1.0 super block
		                                                            // and @-4K Nilfs2 secondary super block
	} ;
	for ( unsigned int i = 0 ; overall_success && i < sizeof( ranges ) / sizeof( ranges[0] ) ; i ++ )
	{
		//Rounding is performed in multiples of the sector size because writes are in whole sectors.
		Byte_Value rounding_size = Utils::ceil_size( ranges[i].rounding, lp_device ->sector_size ) ;
		Byte_Value byte_offset ;
		Byte_Value byte_len ;

		// Compute range to be erased.  Starting position takes into account
		// negative offsets from the end of the partition and minimum desired
		// rounding requirements.  Length is only rounded to device sector size.
		// Ranges may become larger, but not smaller than requested.
		if ( ranges[i] .offset >= 0LL )
		{
			byte_offset = Utils::floor_size( ranges[i] .offset, rounding_size ) ;
			byte_len    = Utils::ceil_size( ranges[i].offset + ranges[i].length, lp_device->sector_size )
			              - byte_offset ;
		}
		else //Negative offsets
		{
			Byte_Value notional_offset = Utils::floor_size( partition .get_byte_length() + ranges[i] .offset, ranges[i]. rounding ) ;
			byte_offset = Utils::floor_size( notional_offset, rounding_size ) ;
			byte_len    = Utils::ceil_size( notional_offset + ranges[i].length, lp_device->sector_size )
			              - byte_offset ;
		}

		//Limit range to partition size.
		if ( byte_offset + byte_len <= 0LL )
		{
			//Byte range entirely before partition start.  Programmer error!
			continue;
		}
		else if ( byte_offset < 0 )
		{
			//Byte range spans partition start.  Trim to fit.
			byte_len += byte_offset ;
			byte_offset = 0LL ;
		}
		if ( byte_offset >= partition .get_byte_length() )
		{
			//Byte range entirely after partition end.  Ignore.
			continue ;
		}
		else if ( byte_offset + byte_len > partition .get_byte_length() )
		{
			//Byte range spans partition end.  Trim to fit.
			byte_len = partition .get_byte_length() - byte_offset ;
		}

		OperationDetail & od = operationdetail .get_last_child() ;
		Byte_Value written = 0LL ;
		bool zero_success = false ;
		if ( device_is_open && buf )
		{
			od .add_child( OperationDetail(
					/*TO TRANSLATORS: looks like  write 68.00 KiB of zeros at byte offset 0 */
					String::ucompose( "write %1 of zeros at byte offset %2",
					                  Utils::format_size( byte_len, 1 ),
					                  byte_offset ) ) ) ;

			// Start sector of the whole disk device or the partition
			Sector ptn_start = 0LL;
			if ( lp_partition )
				ptn_start = lp_partition->geom.start;

			while ( written < byte_len )
			{
				//Write in bufsize amounts.  Last write may be smaller but
				//  will still be a whole number of sectors.
				Byte_Value amount = std::min( bufsize, byte_len - written ) ;
				zero_success = ped_device_write( lp_device, buf,
				                                 ptn_start + ( byte_offset + written ) / lp_device->sector_size,
				                                 amount / lp_device->sector_size );
				if ( ! zero_success )
					break ;
				written += amount ;
			}

			od.get_last_child().set_success_and_capture_errors( zero_success );
		}
		overall_success &= zero_success ;
	}
	if ( buf )
		free( buf ) ;

	//Linux kernel doesn't maintain buffer cache coherency between the whole disk
	//  device and partition devices.  So even though the file system signatures
	//  have been overwritten on the disk via a partition device, libparted reading
	//  via the whole disk device will read the old data and report the file system
	//  as still existing.
	//
	//  Calling ped_device_sync() to flush the cache is required when the partition is
	//  just being cleared.  However the sync can be skipped when the wipe is being
	//  performed as part of writing a new file system as the partition type is also
	//  changed, which modified the partition table so GParted calls
	//  ped_disk_commit_to_os().  This instructs the kernel to remove all non-busy
	//  partitions on the disk and re-adds them, thus effectively flushing the cache
	//  of the disk.
	//
	//  Opening the device and calling ped_device_sync() took 0.15 seconds or less on
	//  a 5400 RPM laptop hard drive.  For now just always call ped_device_sync() as
	//  it doesn't add much time to the overall operation.
	if ( overall_success )
	{
		OperationDetail & od = operationdetail .get_last_child() ;
		od .add_child( OperationDetail( String::ucompose( _("flush operating system cache of %1"),
		                                                  lp_device ->path ) ) ) ;

		bool flush_success = false ;
		if ( device_is_open )
		{
			flush_success = ped_device_sync( lp_device ) ;
			ped_device_close( lp_device ) ;
		}
		od.get_last_child().set_success_and_capture_errors( flush_success );
		overall_success &= flush_success ;
	}
	destroy_device_and_disk( lp_device, lp_disk ) ;

	operationdetail.get_last_child().set_success_and_capture_errors( overall_success );
	return overall_success ;
}

bool GParted_Core::update_bootsector( const Partition & partition, OperationDetail & operationdetail ) 
{
	//only for ntfs atm...
	//FIXME: this should probably be done in the fs classes...
	if ( partition .filesystem == FS_NTFS )
	{
		//The NTFS file system stores a value in the boot record called the
		//  Number of Hidden Sectors.  This value must match the partition start
		//  sector number in order for Windows to boot from the file system.
		//  For more details, refer to the NTFS Volume Boot Record at:
		//  http://www.geocities.com/thestarman3/asm/mbr/NTFSBR.htm

		operationdetail .add_child( OperationDetail( 
				/*TO TRANSLATORS: update boot sector of ntfs file system on /dev/sdd1 */
				String::ucompose( _("update boot sector of %1 file system on %2"),
					  Utils::get_filesystem_string( partition .filesystem ),
					  partition .get_path() ) ) ) ;

		//convert start sector to hex string
		std::stringstream ss ;
		ss << std::hex << partition .sector_start ;
		Glib::ustring hex = ss .str() ;

		//fill with zeros and reverse...
		hex .insert( 0, 8 - hex .length(), '0' ) ;
		Glib::ustring reversed_hex ;
		for ( int t = 6 ; t >= 0 ; t -=2 )
			reversed_hex .append( hex .substr( t, 2 ) ) ;

		//convert reversed hex codes into ascii characters
		char buf[4] ;
		for ( unsigned int k = 0; (k < 4 && k < (reversed_hex .length() / 2)); k++ )
		{
			Glib::ustring tmp_hex = "0x" + reversed_hex .substr( k * 2, 2 ) ;
			buf[k] = (char)( std::strtol( tmp_hex .c_str(), NULL, 16 ) ) ;
		}

		//write new Number of Hidden Sectors value into NTFS boot sector at offset 0x1C
		Glib::ustring error_message = "" ;
		std::ofstream dev_file ;
		dev_file .open( partition .get_path() .c_str(), std::ios::out | std::ios::binary ) ;
		if ( dev_file .is_open() )
		{
			dev_file .seekp( 0x1C ) ;
			if ( dev_file .good() )
			{
				dev_file .write( buf, 4 ) ;
				if ( dev_file .bad() )
				{
					/*TO TRANSLATORS: looks like Error trying to write to boot sector in /dev/sdd1 */
					error_message = String::ucompose( _("Error trying to write to boot sector in %1"), partition .get_path() ) ;
				}
			}
			else
			{
				/*TO TRANSLATORS: looks like Error trying to seek to position 0x1C in /dev/sdd1 */
				error_message = String::ucompose( _("Error trying to seek to position 0x1c in %1"), partition .get_path() ) ;
			}
			dev_file .close( ) ;
		}
		else
		{
			/*TO TRANSLATORS: looks like Error trying to open /dev/sdd1 */
			error_message = String::ucompose( _("Error trying to open %1"), partition .get_path() ) ;
		}

		//append error messages if any found
		bool succes = true ;
		if ( ! error_message .empty() )
		{
			succes = false ;
			error_message += "\n" ;
			/*TO TRANSLATORS: looks like Failed to set the number of hidden sectors to 05ab4f00 in the ntfs boot record. */
			error_message += String::ucompose( _("Failed to set the number of hidden sectors to %1 in the NTFS boot record."), reversed_hex ) ;
			error_message += "\n" ;
			error_message += String::ucompose( _("You might try the following command to correct the problem:"), reversed_hex ) ;
			error_message += "\n" ;
			error_message += String::ucompose( "echo %1 | xxd -r -p | dd conv=notrunc of=%2 bs=1 seek=28", reversed_hex, partition .get_path() ) ;
			operationdetail .get_last_child() .add_child( OperationDetail( error_message, STATUS_NONE, FONT_ITALIC ) ) ;
		}

		operationdetail.get_last_child().set_success_and_capture_errors( succes );
		return succes ;
	}

	return true ;
}

void GParted_Core::init_filesystems()
{
	FILESYSTEM_MAP[FS_UNALLOCATED]     = NULL;
	FILESYSTEM_MAP[FS_UNKNOWN]         = NULL;
	FILESYSTEM_MAP[FS_CLEARED]         = NULL;
	FILESYSTEM_MAP[FS_EXTENDED]        = NULL;
	FILESYSTEM_MAP[FS_BTRFS]           = new btrfs();
	FILESYSTEM_MAP[FS_EXFAT]           = new exfat();
	FILESYSTEM_MAP[FS_EXT2]            = new ext2( FS_EXT2 );
	FILESYSTEM_MAP[FS_EXT3]            = new ext2( FS_EXT3 );
	FILESYSTEM_MAP[FS_EXT4]            = new ext2( FS_EXT4 );
	FILESYSTEM_MAP[FS_F2FS]            = new f2fs();
	FILESYSTEM_MAP[FS_FAT16]           = new fat16( FS_FAT16 );
	FILESYSTEM_MAP[FS_FAT32]           = new fat16( FS_FAT32 );
	FILESYSTEM_MAP[FS_HFS]             = new hfs();
	FILESYSTEM_MAP[FS_HFSPLUS]         = new hfsplus();
	FILESYSTEM_MAP[FS_JFS]             = new jfs();
	FILESYSTEM_MAP[FS_LINUX_SWAP]      = new linux_swap();
	FILESYSTEM_MAP[FS_LVM2_PV]         = new lvm2_pv();
	FILESYSTEM_MAP[FS_LUKS]            = new luks();
	FILESYSTEM_MAP[FS_NILFS2]          = new nilfs2();
	FILESYSTEM_MAP[FS_NTFS]            = new ntfs();
	FILESYSTEM_MAP[FS_REISER4]         = new reiser4();
	FILESYSTEM_MAP[FS_REISERFS]        = new reiserfs();
	FILESYSTEM_MAP[FS_UDF]             = new udf();
	FILESYSTEM_MAP[FS_UFS]             = new ufs();
	FILESYSTEM_MAP[FS_XFS]             = new xfs();
	FILESYSTEM_MAP[FS_BITLOCKER]       = NULL;
	FILESYSTEM_MAP[FS_LINUX_SWRAID]    = NULL;
	FILESYSTEM_MAP[FS_LINUX_SWSUSPEND] = NULL;
}

void GParted_Core::fini_filesystems()
{
	std::map<FSType, FileSystem *>::iterator fs_iter;
	for ( fs_iter = FILESYSTEM_MAP.begin() ; fs_iter != FILESYSTEM_MAP.end() ; fs_iter ++ )
	{
		delete fs_iter->second;
		fs_iter->second = NULL;
	}
}

void GParted_Core::capture_libparted_messages( OperationDetail & operationdetail, bool success )
{
	if ( libparted_messages.size() > 0 )
	{
		operationdetail.add_child( OperationDetail( _("libparted messages"),
		                                            success ? STATUS_INFO : STATUS_ERROR ) );
		for ( unsigned int i = 0 ; i < libparted_messages.size() ; i++ )
			operationdetail.get_last_child().add_child(
					OperationDetail( libparted_messages[i], STATUS_NONE, FONT_ITALIC ) );
		libparted_messages.clear();
	}
}

bool GParted_Core::useable_device( PedDevice * lp_device )
{
	g_assert( lp_device != NULL );  // Bug: Not initialised by call to ped_device_get() or ped_device_get_next()

#ifndef USE_LIBPARTED_LARGE_SECTOR_SUPPORT
	if ( lp_device->sector_size != 512 )
	{
		/* TO TRANSLATORS: looks like   Ignoring device /dev/sde with logical sector size of 2048 bytes. */
		Glib::ustring msg = String::ucompose ( _("Ignoring device %1 with logical sector size of %2 bytes."),
		                                       lp_device ->path, lp_device ->sector_size );
		msg += "\n";
		msg += _("GParted requires libparted version 2.2 or higher to support devices with sector sizes larger than 512 bytes.");
		std::cout << msg << std::endl << std::endl;
		return false;
	}
#endif

	char * buf = static_cast<char *>( malloc( lp_device->sector_size ) );
	if ( ! buf )
		return false;

	// Must be able to read from the first sector before the disk device is considered
	// useable in GParted.
	bool success = false;
	if ( ped_device_open( lp_device )            &&
	     ped_device_read( lp_device, buf, 0, 1 )    )
	{
		success = true;

		ped_device_close( lp_device );
	}

	free( buf );

	return success;
}

//Flush the Linux kernel caches, and therefore ensure coherency between the caches of the
//  whole disk device and the partition devices.
//
//  Libparted >= 2.0 works around this by calling ioctl(fd, BLKFLSBUF) to flush the cache
//  when opening the whole disk device, but only for kernels before 2.6.0.
//  Ref: parted v3.1-52-g1c659d5 ./libparted/arch/linux.c linux_open()
//  1657         /* With kernels < 2.6 flush cache for cache coherence issues */
//  1658         if (!_have_kern26())
//  1659                 _flush_cache (dev);
//
//  Libparted >= v3.1-61-gfb99ba5 works around this for all kernel versions.
//  Ref: parted v3.1-61-gfb99ba5 ./libparted/arch/linux.c linux_open()
//  1640         _flush_cache (dev);
bool GParted_Core::flush_device( PedDevice * lp_device )
{
	bool success = false ;
	if ( ped_device_open( lp_device ) )
	{
		success = ped_device_sync( lp_device ) ;
		ped_device_close( lp_device ) ;
	}
	return success ;
}

bool GParted_Core::get_device( const Glib::ustring & device_path, PedDevice *& lp_device, bool flush )
{
	lp_device = ped_device_get( device_path.c_str() );
	if ( lp_device )
	{
		if ( flush )
			// Force cache coherency before going on to read the partition
			// table so that libparted reading the whole disk device and the
			// file system tools reading the partition devices read the same
			// data.
			flush_device( lp_device );

		return true;
	}
	return false;
}

bool GParted_Core::get_disk( PedDevice *& lp_device, PedDisk *& lp_disk, bool strict )
{
	if ( lp_device )
	{
		lp_disk = ped_disk_new( lp_device );

		// if ! disk and writable it's probably a HD without disklabel.
		// We return true here and deal with them in
		// GParted_Core::set_device_from_disk().
		if ( lp_disk || ( ! strict && ! lp_device ->read_only ) )
			return true;

		destroy_device_and_disk( lp_device, lp_disk );
	}

	return false;
}

bool GParted_Core::get_device_and_disk( const Glib::ustring & device_path,
                                        PedDevice*& lp_device, PedDisk*& lp_disk, bool strict, bool flush )
{
	if ( get_device( device_path, lp_device, flush ) )
	{
		return get_disk( lp_device, lp_disk, strict );
	}

	return false;
}

void GParted_Core::destroy_device_and_disk( PedDevice*& lp_device, PedDisk*& lp_disk )
{
	if ( lp_disk )
		ped_disk_destroy( lp_disk ) ;
	lp_disk = NULL ;

	if ( lp_device )
		ped_device_destroy( lp_device ) ;
	lp_device = NULL ;
}

bool GParted_Core::commit( PedDisk* lp_disk )
{
	// (#790418) Hold a file handle open across the ped_disk_commit_to_dev() and
	// commit_to_os()->ped_disk_commit_to_os() calls to avoid libparted having to open
	// and close the device twice itself.  This avoids the kernel and udev events
	// removing and re-adding partitions exactly when ped_disk_commit_to_os() is
	// trying to doing the same thing.
	bool opened = ped_device_open( lp_disk->dev );

	bool succes = ped_disk_commit_to_dev( lp_disk ) ;
	
	succes = commit_to_os( lp_disk, SETTLE_DEVICE_APPLY_MAX_WAIT_SECONDS ) && succes;

	if ( opened )
	{
		ped_device_close( lp_disk->dev );
		// Wait for udev rules to complete and partition device nodes to settle
		// from this ped_device_close().
		settle_device( SETTLE_DEVICE_APPLY_MAX_WAIT_SECONDS );
	}

	return succes ;
}

bool GParted_Core::commit_to_os( PedDisk* lp_disk, std::time_t timeout )
{
	bool succes ;
#ifndef USE_LIBPARTED_DMRAID
	DMRaid dmraid ;
	if ( dmraid .is_dmraid_device( lp_disk ->dev ->path ) )
		succes = true ;
	else
	{
#endif
		succes = ped_disk_commit_to_os( lp_disk ) ;
#ifdef ENABLE_PT_REREAD_WORKAROUND
		//Work around to try to alleviate problems caused by
		//  bug #604298 - Failure to inform kernel of partition changes
		//  If not successful the first time, try one more time.
		if ( ! succes )
		{
			sleep( 1 ) ;
			succes = ped_disk_commit_to_os( lp_disk ) ;
		}
#endif
#ifndef USE_LIBPARTED_DMRAID
	}
#endif

	// Wait for udev rules to complete and partition device nodes to settle from above
	// ped_disk_commit_to_os() initiated kernel update of the partitions.
	settle_device( timeout ) ;

	return succes ;
}

void GParted_Core::settle_device( std::time_t timeout )
{
	if ( udevsettle_found )
		Utils::execute_command( "udevsettle --timeout=" + Utils::num_to_str( timeout ) ) ;
	else if ( udevadm_found )
		Utils::execute_command( "udevadm settle --timeout=" + Utils::num_to_str( timeout ) ) ;
	else
		sleep( timeout ) ;
}

PedPartition* GParted_Core::get_lp_partition( const PedDisk* lp_disk, const Partition & partition )
{
	if ( partition.type == TYPE_EXTENDED )
		return ped_disk_extended_partition( lp_disk );
	return ped_disk_get_partition_by_sector( lp_disk, partition.get_sector() );
}

class PedExceptionMsg : public Gtk::MessageDialog
{
public:
	PedExceptionMsg( PedException &e ) : MessageDialog( Glib::ustring(e.message), false, Gtk::MESSAGE_ERROR, Gtk::BUTTONS_NONE, true )
		{
			switch( e.type )
			{
			case PED_EXCEPTION_INFORMATION:
				set_title( _("Libparted Information") );
				property_message_type() = Gtk::MESSAGE_INFO;
				break;
			case PED_EXCEPTION_WARNING:
				set_title( _("Libparted Warning") );
				property_message_type() = Gtk::MESSAGE_WARNING;
				break;
			case PED_EXCEPTION_ERROR:
				set_title( _("Libparted Error") );
				break;
			case PED_EXCEPTION_FATAL:
				set_title( _("Libparted Fatal") );
				break;
			case PED_EXCEPTION_BUG:
				set_title( _("Libparted Bug") );
				break;
			case PED_EXCEPTION_NO_FEATURE:
				set_title( _("Libparted Unsupported Feature") );
				break;
			default:
				set_title( _("Libparted unknown exception") );
				break;
			}
			if (e.options & PED_EXCEPTION_FIX)
				add_button( _("Fix"), PED_EXCEPTION_FIX );
			if (e.options & PED_EXCEPTION_YES)
				add_button( _("Yes"), PED_EXCEPTION_YES );
			if (e.options & PED_EXCEPTION_OK)
				add_button( _("Ok"), PED_EXCEPTION_OK );
			if (e.options & PED_EXCEPTION_RETRY)
				add_button( _("Retry"), PED_EXCEPTION_RETRY );
			if (e.options & PED_EXCEPTION_NO)
				add_button( _("No"), PED_EXCEPTION_NO );
			if (e.options & PED_EXCEPTION_CANCEL)
				add_button( _("Cancel"), PED_EXCEPTION_CANCEL );
			if (e.options & PED_EXCEPTION_IGNORE)
				add_button( _("Ignore"), PED_EXCEPTION_IGNORE );
		}
};

struct ped_exception_ctx {
	PedExceptionOption ret;
	PedException *e;
	Glib::Mutex mutex;
	Glib::Cond cond;
};

static bool _ped_exception_handler( struct ped_exception_ctx *ctx )
{
	std::cerr << ctx->e->message << std::endl;

	libparted_messages.push_back( ctx->e->message );
	char optcount = 0;
	int opt = 0;
	for( char c = 0; c < 10; c++ )
		if( ctx->e->options & (1 << c) ) {
			opt = (1 << c);
			optcount++;
		}
	// if only one option was given, choose it without popup
	if( optcount == 1 && ctx->e->type != PED_EXCEPTION_BUG && ctx->e->type != PED_EXCEPTION_FATAL )
	{
		ctx->ret = (PedExceptionOption)opt;
		ctx->mutex.lock();
		ctx->cond.signal();
		ctx->mutex.unlock();
		return false;
	}
	PedExceptionMsg msg( *ctx->e );
	msg.show_all();
	ctx->ret = (PedExceptionOption)msg.run();
	if (ctx->ret < 0)
		ctx->ret = PED_EXCEPTION_UNHANDLED;
	ctx->mutex.lock();
	ctx->cond.signal();
	ctx->mutex.unlock();
	return false;
}

PedExceptionOption GParted_Core::ped_exception_handler( PedException * e )
{
	struct ped_exception_ctx ctx;
	ctx.ret = PED_EXCEPTION_UNHANDLED;
	ctx.e = e;
	if (Glib::Thread::self() != GParted_Core::mainthread) {
		ctx.mutex.lock();
		g_idle_add( (GSourceFunc)_ped_exception_handler, &ctx );
		ctx.cond.wait( ctx.mutex );
		ctx.mutex.unlock();
	} else _ped_exception_handler( &ctx );
	return ctx.ret;
}

GParted_Core::~GParted_Core() 
{
	// Delete file system map entries
	fini_filesystems();
}

Glib::Thread *GParted_Core::mainthread;

std::map< FSType, FileSystem * > GParted_Core::FILESYSTEM_MAP;

} //GParted
