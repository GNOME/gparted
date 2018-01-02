/* Copyright (C) 2004 Bart 'plors' Hakvoort
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

#include "Utils.h"
#include "GParted_Core.h"
#include "PipeCapture.h"

#include <sstream>
#include <fstream>
#include <iomanip>
#ifdef HAVE_GLIB_REGEX
#include <glibmm/regex.h>
#else
#include <regex.h>
#endif
#include <locale.h>
#include <uuid/uuid.h>
#include <cerrno>
#include <sys/statvfs.h>
#include <glibmm/ustring.h>
#include <gtkmm/main.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace GParted
{

const Glib::ustring DEV_MAPPER_PATH = "/dev/mapper/";

Sector Utils::round( double double_value )
{
	// Reference:
	//     How can I convert a floating-point value to an integer in C?
	//     https://www.cs.tut.fi/~jkorpela/round.html
	if ( double_value >= 0.0 )
		return static_cast<Sector>( double_value + 0.5 );
	else
		return static_cast<Sector>( double_value - 0.5 );
}

Gtk::Label * Utils::mk_label( const Glib::ustring & text
                            , bool use_markup
                            , bool wrap
                            , bool selectable
                            , float yalign
                            )
{
	//xalign 0.0 == Gtk::ALIGN_LEFT  (gtkmm <= 2.22)
	//           == Gtk::ALIGN_START (gtkmm >= 2.24)
	//yalign 0.5 == Gtk::ALIGN_CENTER
	//       0.0 == Gtk::ALIGN_TOP   (gtkmm <= 2.22)
	//              Gtk::ALIGN_START (gtkmm >= 2.24)
	Gtk::Label * label = manage( new Gtk::Label( text, 0.0, yalign ) ) ;

	label ->set_use_markup( use_markup ) ;
	label ->set_line_wrap( wrap ) ;
	label ->set_selectable( selectable ) ;

	return label ;
}

Glib::ustring Utils::num_to_str( Sector number )
{
	std::stringstream ss ;
	ss << number ;
	return ss .str() ;
}

// Use palette from GNOME Human Interface Guidelines as a starting point.
//     http://developer.gnome.org/hig-book/2.32/design-color.html.en
//     http://web.archive.org/web/20130922173112/https://developer.gnome.org/hig-book/stable/design-color.html.en
Glib::ustring Utils::get_color( FSType filesystem )
{
	switch( filesystem )
	{
		case FS_UNALLOCATED	: return "#A9A9A9" ;	// ~ medium grey
		case FS_UNKNOWN		: return "#000000" ;	//black
		case FS_UNFORMATTED	: return "#000000" ;	//black
		case FS_CLEARED		: return "#000000" ;	//black
		case FS_EXTENDED	: return "#7DFCFE" ;	// ~ light blue
		case FS_BTRFS		: return "#FF9955" ;	//orange
		case FS_EXT2		: return "#9DB8D2" ;	//blue hilight
		case FS_EXT3		: return "#7590AE" ;	//blue medium
		case FS_EXT4		: return "#4B6983" ;	//blue dark
		case FS_LINUX_SWAP	: return "#C1665A" ;	//red medium
		case FS_LUKS		: return "#625B81" ;	//purple dark
		case FS_F2FS		: return "#DF421E" ;	//accent red
		case FS_FAT16		: return "#00FF00" ;	//green
		case FS_FAT32		: return "#18D918" ;	// ~ medium green
		case FS_EXFAT		: return "#267726" ;	// Accent Green Dark
		case FS_NILFS2		: return "#826647" ;	//face skin shadow
		case FS_NTFS		: return "#42E5AC" ;	// ~ light aquamarine
		case FS_REISERFS	: return "#ADA7C8" ;	//purple hilight
		case FS_REISER4		: return "#887FA3" ;	//purple medium
		case FS_XFS			: return "#EED680" ;	//accent yellow
		case FS_JFS			: return "#E0C39E" ;	//face skin medium
		case FS_HFS			: return "#E0B6AF" ;	//red hilight
		case FS_HFSPLUS		: return "#C0A39E" ;	// ~ serene red
		case FS_UDF		: return "#105210";	// Accent Green Shadow
		case FS_UFS			: return "#D1940C" ;	//accent yellow dark
		case FS_USED		: return "#F8F8BA" ;	// ~ light tan yellow
		case FS_UNUSED		: return "#FFFFFF" ;	//white
		case FS_LVM2_PV		: return "#B39169" ;	//face skin dark
		case FS_BITLOCKER	: return "#494066" ;	//purple shadow
		case FS_GRUB2_CORE_IMG	: return "#666666" ;	//~ dark gray
		case FS_ISO9660		: return "#D3D3D3" ;	// ~ light gray
		case FS_LINUX_SWRAID   	: return "#5A4733" ;	// ~ dark brown
		case FS_LINUX_SWSUSPEND	: return "#884631" ;	//red dark
		case FS_REFS		: return "#2AB98A" ;	// ~ medium aquamarine
		case FS_ZFS 		: return "#CC763D" ;	// ~ darker orange

		default				: return "#000000" ;
	}
}

Glib::RefPtr<Gdk::Pixbuf> Utils::get_color_as_pixbuf( FSType filesystem, int width, int height )
{
	Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create( Gdk::COLORSPACE_RGB, false, 8, width, height ) ;

	if ( pixbuf )
	{
		std::stringstream hex( get_color( filesystem ) .substr( 1 ) + "00" ) ;
		unsigned long dec ;
		hex >> std::hex >> dec ;

		pixbuf ->fill( dec ) ;
	}

	return pixbuf ;
}

int Utils::get_max_partition_name_length( Glib::ustring & tabletype )
{
	// Partition name size found or confirmed by looking at *_partition_set_name()
	// functions in the relevant libparted label modules:
	//     dvh.c, gpt.c, mac.c, pc98.c, rdb.c (amiga)
	// http://git.savannah.gnu.org/cgit/parted.git/tree/libparted/labels
	//
	//     Table   Max      Units
	//     type    length   Reference
	//     -----   ------   ---------
	//     amiga       32   ASCII characters
	//     dvh          8   ASCII characters
	//     gpt         36   UTF-16LE code points
	//                      http://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_entries
	//     mac         32   ASCII characters
	//                      http://en.wikipedia.org/wiki/Apple_Partition_Map#Layout
	//     pc98        16   ASCII characters
	//
	// Found issues:
	// 1) amiga - Setting partition name to 32 chars is silent ignored.  Setting to 31
	//            chars or less works.
	// 2) dvh   - Probably by design, setting names on primary partitions is
	//            successfully ignored with this libparted message:
	//                failed to set dvh partition name to NAME:
	//                Only logical partitions (boot files) have a name.
	//            Setting names on logical partitions works.
	// 3) mac   - Setting partition name to 32 chars core dumps in libparted.  Setting
	//            to 31 chars or less works.
	//
	// Suspect usage of these partition tables types other than GPT is *VERY* low.
	// Given the above issues which need a little coding around, leave support of
	// naming for partition table types other than GPT disabled.  Mostly just to
	// reduce ongoing testing effort, at least until there is any user demand for it.

	if      ( tabletype == "amiga" ) return 0;   // Disabled
	else if ( tabletype == "dvh"   ) return 0;   // Disabled
	else if ( tabletype == "gpt"   ) return 36;
	else if ( tabletype == "mac"   ) return 0;   // Disabled
	else if ( tabletype == "pc98"  ) return 0;   // Disabled

	return 0;
}

int Utils::get_filesystem_label_maxlength( FSType filesystem )
{
	switch( filesystem )
	{
		//All file systems commented out are not supported for labelling
		//  by either the new partition or label partition operations.
		case FS_BTRFS		: return 255 ;
		//case FS_EXFAT		: return  ;
		case FS_EXT2		: return 16 ;
		case FS_EXT3		: return 16 ;
		case FS_EXT4		: return 16 ;
		//mkfs.f2fs says that it can create file systems with labels up to 512
		//  characters, but it core dumps with labels of 29 characters or larger!
		//  Also blkid only correctly displays labels up to 19 characters.
		//  (Suspect it is all part of a memory corruption bug in mkfs.f2fs).
		case FS_F2FS		: return 19 ;
		case FS_FAT16		: return 11 ;
		case FS_FAT32		: return 11 ;
		//mkfs.hfsplus can create hfs and hfs+ file systems with labels up to 255
		//  characters.  However there is no specific tool to read the labels and
		//  blkid, the only tool currently available, only display the first 27
		//  and 63 character respectively.
		//  Reference:
		//  util-linux-2.20.1/libblkid/src/superblocks/hfs.c:struct hfs_mdb
		case FS_HFS		: return 27 ;
		case FS_HFSPLUS		: return 63 ;
		//mkfs.jfs and jfs_tune can create and update labels to 16 characters but
		//  only displays the first 11 characters.  This is because version 1 jfs
		//  file systems only have an 11 character field for the label but version
		//  2 jfs has extra fields containing a 16 character label.  mkfs.jfs
		//  writes the extra fields containing the 16 character label, but then
		//  sets it to version 1 jfs.  It does this to be backwardly compatible
		//  with jfs before 1.0.18, released May 2002.  Blkid does display the
		//  full 16 character label by just ignoring the file system version.
		//  As using jfs_tune to get the label stick with an 11 character limit.
		//  References:
		//  jfsutils-1.1.15/tune/tune.c:main()
		//  jfsutils-1.1.15/mkfs/mkfs.c:create_aggregate()
		//  http://jfs.cvs.sourceforge.net/viewvc/jfs/jfsutils/NEWS?revision=HEAD
		case FS_JFS		: return 11 ;
		case FS_LINUX_SWAP	: return 15 ;
		//case FS_LVM2_PV	: return  ;
		case FS_NILFS2		: return 80 ;
		case FS_NTFS		: return 128 ;
		case FS_REISER4		: return 16 ;
		case FS_REISERFS	: return 16 ;
		case FS_UDF		: return 126;  // and only 63 if label contains character above U+FF
		//case FS_UFS		: return  ;
		case FS_XFS		: return 12 ;

		default			: return 30 ;
	}
}

// Return libparted file system name / GParted display name
Glib::ustring Utils::get_filesystem_string( FSType filesystem )
{
	switch( filesystem )
	{
		case FS_UNALLOCATED	: return
				/* TO TRANSLATORS:  unallocated
				 * means that this space on the disk device does
				 * not contain a recognized file system, and is in
				 * other words unallocated.
				 */
				_("unallocated") ;
		case FS_UNKNOWN		: return
				/* TO TRANSLATORS:  unknown
				 * means that this space within this partition does
				 * not contain a file system known to GParted, and
				 * is in other words unknown.
				 */
				_("unknown") ;
		case FS_UNFORMATTED	: return
				/* TO TRANSLATORS:  unformatted
				 * means that the space within this partition will not
				 * be formatted with a known file system by GParted.
				 */
				_("unformatted") ;
		case FS_CLEARED		: return
				/* TO TRANSLATORS:  cleared
				 * means that all file system signatures in the partition
				 * will be cleared by GParted.
				 */
				_("cleared") ;
		case FS_EXTENDED	: return "extended" ;
		case FS_BTRFS		: return "btrfs" ;
		case FS_EXT2		: return "ext2" ;
		case FS_EXT3		: return "ext3" ;
		case FS_EXT4		: return "ext4" ;
		case FS_LINUX_SWAP	: return "linux-swap" ;
		case FS_LUKS		: return "luks";
		case FS_F2FS		: return "f2fs" ;
		case FS_FAT16		: return "fat16" ;
		case FS_FAT32		: return "fat32" ;
		case FS_EXFAT		: return "exfat" ;
		case FS_NILFS2		: return "nilfs2" ;
		case FS_NTFS		: return "ntfs" ;
		case FS_REISERFS	: return "reiserfs" ;
		case FS_REISER4		: return "reiser4" ;
		case FS_XFS		: return "xfs" ;
		case FS_JFS		: return "jfs" ;
		case FS_HFS		: return "hfs" ;
		case FS_HFSPLUS		: return "hfs+" ;
		case FS_UDF		: return "udf";
		case FS_UFS		: return "ufs" ;
		case FS_USED		: return _("used") ;
		case FS_UNUSED		: return _("unused") ;
		case FS_LVM2_PV		: return "lvm2 pv" ;
		case FS_BITLOCKER	: return "bitlocker" ;
		case FS_GRUB2_CORE_IMG	: return "grub2 core.img";
		case FS_ISO9660		: return "iso9660";
		case FS_LINUX_SWRAID	: return "linux-raid" ;
		case FS_LINUX_SWSUSPEND	: return "linux-suspend" ;
		case FS_REFS		: return "refs" ;
		case FS_ZFS 		: return "zfs" ;

		default			: return "" ;
	}
}

const Glib::ustring Utils::get_encrypted_string()
{
	/* TO TRANSLATORS: means that this is an encrypted file system */
	return "[" + Glib::ustring( _("Encrypted") ) + "]";
}

const Glib::ustring Utils::get_filesystem_string( bool encrypted, FSType fstype )
{
	if ( encrypted )
		return get_encrypted_string() + " " + get_filesystem_string( fstype );
	else
		return get_filesystem_string( fstype );
}

// Return Linux kernel name only for mountable file systems.
// (Identical to a subset of the libparted names except that it's hfsplus instead of hfs+).
const Glib::ustring Utils::get_filesystem_kernel_name( FSType fstype )
{
	switch ( fstype )
	{
		case FS_BTRFS    : return "btrfs";
		case FS_EXFAT    : return "exfat";
		case FS_EXT2     : return "ext2";
		case FS_EXT3     : return "ext3";
		case FS_EXT4     : return "ext4";
		case FS_F2FS     : return "f2fs";
		case FS_FAT16    : return "fat16";
		case FS_FAT32    : return "fat32";
		case FS_HFS      : return "hfs";
		case FS_HFSPLUS  : return "hfsplus";
		case FS_JFS      : return "jfs";
		case FS_NILFS2   : return "nilfs2";
		case FS_NTFS     : return "ntfs";
		case FS_REISER4  : return "reiser4";
		case FS_REISERFS : return "reiserfs";
		case FS_UDF      : return "udf";
		case FS_UFS      : return "ufs";
		case FS_XFS      : return "xfs";
		default          : return "";
	}
}

Glib::ustring Utils::get_filesystem_software( FSType filesystem )
{
	switch( filesystem )
	{
		case FS_BTRFS       : return "btrfs-progs / btrfs-tools" ;
		case FS_EXT2        : return "e2fsprogs" ;
		case FS_EXT3        : return "e2fsprogs" ;
		case FS_EXT4        : return "e2fsprogs v1.41+" ;
		case FS_F2FS        : return "f2fs-tools" ;
		case FS_FAT16       : return "dosfstools, mtools" ;
		case FS_FAT32       : return "dosfstools, mtools" ;
		case FS_HFS         : return "hfsutils" ;
		case FS_HFSPLUS     : return "hfsprogs" ;
		case FS_JFS         : return "jfsutils" ;
		case FS_LINUX_SWAP  : return "util-linux" ;
		case FS_LVM2_PV     : return "lvm2" ;
		case FS_LUKS        : return "cryptsetup, dmsetup";
		case FS_NILFS2      : return "nilfs-utils" ;
		case FS_NTFS        : return "ntfs-3g / ntfsprogs" ;
		case FS_REISER4     : return "reiser4progs" ;
		case FS_REISERFS    : return "reiserfsprogs / reiserfs-utils" ;
		case FS_UDF         : return "udftools";
		case FS_UFS         : return "" ;
		case FS_XFS         : return "xfsprogs, xfsdump" ;

		default             : return "" ;
	}
}

//Report whether or not the kernel supports a particular file system
bool Utils::kernel_supports_fs( const Glib::ustring & fs )
{
	bool fs_supported = false ;

	//Read /proc/filesystems and check for the file system name.
	//  Will succeed for compiled in drivers and already loaded
	//  moduler drivers.  If not found, try loading the driver
	//  as a module and re-checking /proc/filesystems.
	std::ifstream input ;
	std::string line ;
	input .open( "/proc/filesystems" ) ;
	if ( input )
	{
		while ( input >> line )
			if ( line == fs )
			{
				fs_supported = true ;
				break ;
			}
		input .close() ;
	}
	if ( fs_supported )
		return true ;

	Glib::ustring output, error ;
	execute_command( "modprobe " + Glib::shell_quote( fs ), output, error, true );

	input .open( "/proc/filesystems" ) ;
	if ( input )
	{
		while ( input >> line )
			if ( line == fs )
			{
				fs_supported = true ;
				break ;
			}
		input .close() ;
	}

	return fs_supported ;
}

//Report if kernel version is >= (major, minor, patch)
bool Utils::kernel_version_at_least( int major_ver, int minor_ver, int patch_ver )
{
	int actual_major_ver, actual_minor_ver, actual_patch_ver ;
	if ( ! get_kernel_version( actual_major_ver, actual_minor_ver, actual_patch_ver ) )
		return false ;
	bool result =    ( actual_major_ver > major_ver )
	              || ( actual_major_ver == major_ver && actual_minor_ver > minor_ver )
	              || ( actual_major_ver == major_ver && actual_minor_ver == minor_ver && actual_patch_ver >= patch_ver ) ;
	return result ;
}

Glib::ustring Utils::format_size( Sector sectors, Byte_Value sector_size )
{
	std::stringstream ss ;
	ss << std::setiosflags( std::ios::fixed ) << std::setprecision( 2 ) ;

	if ( (sectors * sector_size) < KIBIBYTE )
	{
		ss << sector_to_unit( sectors, sector_size, UNIT_BYTE ) ;
		return String::ucompose( _("%1 B"), ss .str() ) ;
	}
	else if ( (sectors * sector_size) < MEBIBYTE )
	{
		ss << sector_to_unit( sectors, sector_size, UNIT_KIB ) ;
		return String::ucompose( _("%1 KiB"), ss .str() ) ;
	}
	else if ( (sectors * sector_size) < GIBIBYTE )
	{
		ss << sector_to_unit( sectors, sector_size, UNIT_MIB ) ;
		return String::ucompose( _("%1 MiB"), ss .str() ) ;
	}
	else if ( (sectors * sector_size) < TEBIBYTE )
	{
		ss << sector_to_unit( sectors, sector_size, UNIT_GIB ) ;
		return String::ucompose( _("%1 GiB"), ss .str() ) ;
	}
	else
	{
		ss << sector_to_unit( sectors, sector_size, UNIT_TIB ) ;
		return String::ucompose( _("%1 TiB"), ss .str() ) ;
	}
}

Glib::ustring Utils::format_time( std::time_t seconds )
{
	Glib::ustring time ;

	if ( seconds < 0 )
	{
		time = "-";
		seconds = -seconds;
	}

	int unit = static_cast<int>( seconds / 3600 ) ;
	if ( unit < 10 )
		time += "0" ;
	time += num_to_str( unit ) + ":" ;
	seconds %= 3600 ;

	unit = static_cast<int>( seconds / 60 ) ;
	if ( unit < 10 )
		time += "0" ;
	time += num_to_str( unit ) + ":" ;
	seconds %= 60 ;

	if ( seconds < 10 )
		time += "0" ;
	time += num_to_str( seconds ) ;

	return time ;
}

double Utils::sector_to_unit( Sector sectors, Byte_Value sector_size, SIZE_UNIT size_unit )
{
	switch ( size_unit )
	{
		case UNIT_BYTE	:
			return sectors * sector_size ;

		case UNIT_KIB	:
			return sectors / ( static_cast<double>( KIBIBYTE ) / sector_size );
		case UNIT_MIB	:
			return sectors / ( static_cast<double>( MEBIBYTE ) / sector_size );
		case UNIT_GIB	:
			return sectors / ( static_cast<double>( GIBIBYTE ) / sector_size );
		case UNIT_TIB	:
			return sectors / ( static_cast<double>( TEBIBYTE ) / sector_size );

		default:
			return sectors ;
	}
}

int Utils::execute_command( const Glib::ustring & command )
{
	Glib::ustring dummy ;
	return execute_command( command, dummy, dummy ) ;
}

class CommandStatus
{
public:
	bool running;
	int pipecount;
	int exit_status;
	bool foreground;
	Glib::Mutex mutex;
	Glib::Cond cond;
	void store_exit_status( GPid pid, int status );
	void execute_command_eof();
};

void CommandStatus::store_exit_status( GPid pid, int status )
{
	exit_status = Utils::decode_wait_status( status );
	running = false;
	if (pipecount == 0) // pipes finished first
	{
		if (foreground)
			Gtk::Main::quit();
		else {
			mutex.lock();
			cond.signal();
			mutex.unlock();
		}
	}
	Glib::spawn_close_pid( pid );
}

void CommandStatus::execute_command_eof()
{
	if (--pipecount)
		return; // wait for second pipe to eof
	if ( !running ) // already got exit status
	{
		if (foreground)
			Gtk::Main::quit();
		else {
			mutex.lock();
			cond.signal();
			mutex.unlock();
		}
	}
}

static void set_locale()
{
	setenv( "LC_ALL", "C", 1 );
}

static void _store_exit_status( GPid pid, gint status, gpointer data )
{
	CommandStatus *sp = (CommandStatus *)data;
	sp->store_exit_status( pid, status );
}

int Utils::execute_command( const Glib::ustring & command,
			    Glib::ustring & output,
			    Glib::ustring & error,
			    bool use_C_locale )
{
	Glib::Pid pid;
	// set up pipes for capture
	int out, err;
	CommandStatus status;
	// spawn external process
	status.running = true;
	status.pipecount = 2;
	status.foreground = (Glib::Thread::self() == GParted_Core::mainthread);
	try {
		Glib::spawn_async_with_pipes(
			std::string( "." ),
			Glib::shell_parse_argv( command ),
			Glib::SPAWN_DO_NOT_REAP_CHILD | Glib::SPAWN_SEARCH_PATH,
			use_C_locale ? sigc::ptr_fun( set_locale ) : sigc::slot< void >(),
			&pid,
			0,
			&out,
			&err );
	} catch (Glib::SpawnError &e) {
		std::cerr << e.what() << std::endl;
		return Utils::get_failure_status( e );
	}
	fcntl( out, F_SETFL, O_NONBLOCK );
	fcntl( err, F_SETFL, O_NONBLOCK );
	g_child_watch_add( pid, _store_exit_status, &status );
	//Lock mutex so we have time to setup pipecapture for output and error streams
	//  before connecting the input/output signal handler
	if( !status.foreground )
		status.mutex.lock();
	PipeCapture outputcapture( out, output );
	PipeCapture errorcapture( err, error );
	outputcapture.signal_eof.connect( sigc::mem_fun( status, &CommandStatus::execute_command_eof ) );
	errorcapture.signal_eof.connect( sigc::mem_fun( status, &CommandStatus::execute_command_eof ) );
	outputcapture.connect_signal();
	errorcapture.connect_signal();

	if( status.foreground)
		Gtk::Main::run();
	else {
		status.cond.wait( status.mutex );
		status.mutex.unlock();
	}
	close( out );
	close( err );
	return status.exit_status;
}

// Return shell style exit status when failing to execute a command.  127 for command not
// found and 126 otherwise.
// NOTE:
// Together get_failure_status() and decode_wait_status() provide complete shell style
// exit status handling.  See bash(1) manual page, EXIT STATUS section for details.
int Utils::get_failure_status( Glib::SpawnError & e )
{
	if ( e.code() == Glib::SpawnError::NOENT )
		return 127;
	return 126;
}

// Return shell style decoding of waitpid(2) encoded exit statuses.  Return command exit
// status or 128 + signal number when terminated by a signal.
int Utils::decode_wait_status( int wait_status )
{
	if ( WIFEXITED( wait_status ) )
		return WEXITSTATUS( wait_status );
	else if ( WIFSIGNALED( wait_status ) )
		return 128 + WTERMSIG( wait_status );

	// Other cases of WIFSTOPPED() and WIFCONTINUED() occur when the process is
	// stopped or resumed by signals.  Should be impossible as this function is only
	// called after the process has exited.
	std::cerr << "Unexpected wait status " << wait_status << std::endl;
	return 255;
}

Glib::ustring Utils::regexp_label( const Glib::ustring & text
                                 , const Glib::ustring & pattern
                                 )
{
	//Extract text from a regular sub-expression or pattern.
	//  E.g., "text we don't want (text we want)"
#ifdef HAVE_GLIB_REGEX
	std::vector<Glib::ustring> results;
	Glib::RefPtr<Glib::Regex> myregexp =
		Glib::Regex::create( pattern
		                   , Glib::REGEX_CASELESS | Glib::REGEX_MULTILINE
		                   );

	results = myregexp ->split( text );

	if ( results .size() >= 2 )
		return results[ 1 ] ;
	else
		return "" ;
#else /* ! HAVE_GLIB_REGEX */
	Glib::ustring label = "" ;
	regex_t preg ;
	int nmatch = 2 ;
	regmatch_t pmatch[ 2 ] ;
	int rc = regcomp( &preg, pattern .c_str(), REG_EXTENDED | REG_ICASE | REG_NEWLINE ) ;
	if ( rc == 0 )
	{
		if ( regexec( &preg, text .c_str(), nmatch, pmatch, 0 ) == 0 )
			label = text .substr( pmatch[ 1 ] .rm_so, pmatch[ 1 ] .rm_eo - pmatch[ 1 ] .rm_so ) ;
		regfree( &preg ) ;
	}
	return label ;
#endif
}

Glib::ustring Utils::trim( const Glib::ustring & src, const Glib::ustring & c /* = " \t\r\n" */ )
{
	//Trim leading and trailing whitespace from string
	Glib::ustring::size_type p2 = src.find_last_not_of(c);
	if (p2 == Glib::ustring::npos) return Glib::ustring();
	Glib::ustring::size_type p1 = src.find_first_not_of(c);
	if (p1 == Glib::ustring::npos) p1 = 0;
	return src.substr(p1, (p2-p1)+1);
}

// Return portion of string after the last carriage return character or
// the whole string when there is no carriage return character.
Glib::ustring Utils::last_line( const Glib::ustring & src )
{
	Glib::ustring::size_type p = src.find_last_of( '\n' );
	if ( p == Glib::ustring::npos )
		return src;
	return src.substr( p+1 );
}

Glib::ustring Utils::get_lang()
{
	//Extract base language from string that may look like "en_CA.UTF-8"
	//  and return in the form "en-CA"
	Glib::ustring lang = setlocale( LC_CTYPE, NULL ) ;

	//Strip off anything after the period "." or at sign "@"
	lang = Utils::regexp_label( lang .c_str(), "^([^.@]*)") ;

	//Convert the underscore "_" to a hyphen "-"
	Glib::ustring sought = "_" ;
	Glib::ustring replacement = "-" ;
	//NOTE:  Application crashes if string replace is called and sought is not found,
	//       so we need to only perform replace if the sought is found.
	if ( lang .find(sought) != Glib::ustring::npos )
		lang .replace( lang .find(sought), sought .size(), replacement ) ;

	return lang ;
}

//Extract a list of tokens from any number of background separator characters
//  E.g., tokenize(str="  word1   word2   ", tokens, delimiters=" ")
//        -> tokens=["word1","word2]
//The tokenize method copied and adapted from:
//  http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
void Utils::tokenize( const Glib::ustring& str,
                      std::vector<Glib::ustring>& tokens,
                      const Glib::ustring& delimiters = " " )
{
	// Skip delimiters at beginning.
	Glib::ustring::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	Glib::ustring::size_type pos     = str.find_first_of(delimiters, lastPos);

	while (Glib::ustring::npos != pos || Glib::ustring::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

//Split string on every delimiter, appending to the vector.  Inspired by:
//  http://stackoverflow.com/questions/236129/how-to-split-a-string-in-c/3616605#3616605
//  E.g. using Utils::split(str, result, ":") for str -> result
//  "" -> []   "a" -> ["a"]   "::" -> ["","",""]   ":a::bb" -> ["","a","","bb"]
void Utils::split( const Glib::ustring& str,
                   std::vector<Glib::ustring>& result,
                   const Glib::ustring& delimiters     )
{
	//Special case zero length string to empty vector
	if ( str == "" )
		return ;
	Glib::ustring::size_type fromPos  = 0 ;
	Glib::ustring::size_type delimPos = str.find_first_of( delimiters );
	while ( Glib::ustring::npos != delimPos )
	{
		Glib::ustring word( str, fromPos, delimPos - fromPos ) ;
		result .push_back( word ) ;
		fromPos = delimPos + 1 ;
		delimPos = str.find_first_of( delimiters, fromPos ) ;
	}
	Glib::ustring word( str, fromPos ) ;
	result. push_back( word ) ;
}

// Converts a Glib::ustring into a int
int Utils::convert_to_int(const Glib::ustring & src)
{
	int ret_val;
	std::istringstream stream(src);
	stream >> ret_val;

	return ret_val;
}

// Create a new UUID
Glib::ustring Utils::generate_uuid(void)
{
	uuid_t uuid;
	char uuid_str[UUID_STRING_LENGTH+1];

	uuid_generate(uuid);
	uuid_unparse(uuid,uuid_str);

	return uuid_str;
}

//Wrapper around statvfs() system call to get mounted file system size
//  and free space, both in bytes.
int Utils::get_mounted_filesystem_usage( const Glib::ustring & mountpoint,
                                         Byte_Value & fs_size, Byte_Value & fs_free,
                                         Glib::ustring & error_message )
{
	struct statvfs sfs ;
	int ret ;
	ret = statvfs( mountpoint .c_str(), &sfs ) ;
	if ( ret == 0 )
	{
		fs_size = static_cast<Byte_Value>( sfs .f_blocks ) * sfs .f_frsize ;
		fs_free = static_cast<Byte_Value>( sfs .f_bfree ) * sfs .f_bsize ;
	}
	else
		error_message = "statvfs(\"" + mountpoint + "\"): " + Glib::strerror( errno ) ;

	return ret ;
}

//Round down to multiple of rounding_size
Byte_Value Utils::floor_size( Byte_Value value, Byte_Value rounding_size )
{
	return value / rounding_size * rounding_size ;
}

//Round up to multiple of rounding_size
Byte_Value Utils::ceil_size( Byte_Value value, Byte_Value rounding_size )
{
	return ( value + rounding_size - 1LL ) / rounding_size * rounding_size ;
}

//private functions ...

//Read kernel version, reporting success or failure
bool Utils::get_kernel_version( int & major_ver, int & minor_ver, int & patch_ver )
{
	static bool read_file = false ;
	static int read_major_ver = 0 ;
	static int read_minor_ver = 0 ;
	static int read_patch_ver = 0 ;
	static bool success = false ;

	if ( ! read_file )
	{
		std::ifstream input( "/proc/version" ) ;
		std::string line ;
		if ( input )
		{
			getline( input, line ) ;
			int assigned = sscanf( line .c_str(), "Linux version %d.%d.%d",
			                       &read_major_ver, &read_minor_ver, &read_patch_ver ) ;
			success = ( assigned >= 2 ) ;  //At least 2 kernel version components read
			input .close() ;
		}
		read_file = true ;
	}
	if ( success )
	{
		major_ver = read_major_ver ;
		minor_ver = read_minor_ver ;
		patch_ver = read_patch_ver ;
	}
	return success ;
}

} //GParted..
