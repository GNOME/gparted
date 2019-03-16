/* Copyright (C) 2004 Bart
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
 
 
 
/* UTILS
 * Some stuff i need in a lot of places so i dropped in all together in one file.
 */

#ifndef GPARTED_UTILS_H
#define GPARTED_UTILS_H

#include "i18n.h"

#include <gtkmm/label.h>
#include <glibmm/ustring.h>
#include <glibmm/spawn.h>
#include <iostream>
#include <ctime>
#include <vector>

#define UUID_STRING_LENGTH 36
//Match RFC 4122 UUID strings.  Exclude Nil UUID (all zeros) by excluding
//  zero from the version field nibble.
#define RFC4122_NONE_NIL_UUID_REGEXP "[[:xdigit:]]{8}-[[:xdigit:]]{4}-[1-9a-fA-F][[:xdigit:]]{3}-[[:xdigit:]]{4}-[[:xdigit:]]{12}"

namespace GParted
{

typedef long long Sector;
typedef long long Byte_Value;

//Size units defined in bytes
const Byte_Value KIBIBYTE=1024;
const Byte_Value MEBIBYTE=(KIBIBYTE * KIBIBYTE);
const Byte_Value GIBIBYTE=(MEBIBYTE * KIBIBYTE);
const Byte_Value TEBIBYTE=(GIBIBYTE * KIBIBYTE);
const Byte_Value PEBIBYTE=(TEBIBYTE * KIBIBYTE);
const Byte_Value EXBIBYTE=(PEBIBYTE * KIBIBYTE);

const Glib::ustring UUID_RANDOM = _("(New UUID - will be randomly generated)") ;
const Glib::ustring UUID_RANDOM_NTFS_HALF = _("(Half new UUID - will be randomly generated)") ;

extern const Glib::ustring DEV_MAPPER_PATH;

enum FSType
{
	// Special partition types and functions
	FS_UNSUPPORTED     = 0,  // Type with no supported actions
	FS_UNALLOCATED     = 1,  // Unallocated space on a partitioned drive
	FS_UNKNOWN         = 2,  // Unrecognised content in a drive or partition
	FS_UNFORMATTED     = 3,  // Create a partition without a file system
	FS_CLEARED         = 4,  // Clear existing file system signatures
	FS_OTHER           = 5,  // Just for showing in the File System Support dialog
	FS_EXTENDED        = 6,

	// Fully supported file system types
	FS_BTRFS           = 7,
	FS_EXFAT           = 8, /* Also known as fat64 */
	FS_EXT2            = 9,
	FS_EXT3            = 10,
	FS_EXT4            = 11,
	FS_F2FS            = 12,
	FS_FAT16           = 13,
	FS_FAT32           = 14,
	FS_HFS             = 15,
	FS_HFSPLUS         = 16,
	FS_JFS             = 17,
	FS_LINUX_SWAP      = 18,
	FS_LUKS            = 19,
	FS_LVM2_PV         = 20,
	FS_MINIX           = 21,
	FS_NILFS2          = 22,
	FS_NTFS            = 23,
	FS_REISER4         = 24,
	FS_REISERFS        = 25,
	FS_UDF             = 26,
	FS_XFS             = 27,

	// Other recognised file system types
	FS_APFS            = 28,
	FS_BITLOCKER       = 29,
	FS_GRUB2_CORE_IMG  = 30,
	FS_ISO9660         = 31,
	FS_LINUX_SWRAID    = 32,
	FS_LINUX_SWSUSPEND = 33,
	FS_REFS            = 34,
	FS_UFS             = 35,
	FS_ZFS             = 36,

	// Partition space usage colours
	FS_USED            = 37,
	FS_UNUSED          = 38
} ;

enum SIZE_UNIT
{
	UNIT_SECTOR	= 0,
	UNIT_BYTE	= 1,
	
	UNIT_KIB	= 2,
	UNIT_MIB	= 3,
	UNIT_GIB	= 4,
	UNIT_TIB	= 5
} ;

class Utils
{
public:
	static Sector round( double double_value ) ;
	static Gtk::Label * mk_label( const Glib::ustring & text
	                            , bool use_markup = true
	                            , bool wrap = false
	                            , bool selectable = false
	                            , float yalign = 0.5 /* ALIGN_CENTER */
	                            ) ;
	static Glib::ustring num_to_str( Sector number ) ;
	static Glib::ustring get_color( FSType filesystem );
	static Glib::RefPtr<Gdk::Pixbuf> get_color_as_pixbuf( FSType filesystem, int width, int height );
	static int get_max_partition_name_length( Glib::ustring & tabletype );
	static int get_filesystem_label_maxlength( FSType filesystem );
	static Glib::ustring get_filesystem_string( FSType filesystem );
	static const Glib::ustring get_encrypted_string();
	static const Glib::ustring get_filesystem_string( bool encrypted, FSType fstype );
	static const Glib::ustring get_filesystem_kernel_name( FSType fstype );
	static Glib::ustring get_filesystem_software( FSType filesystem );
	static bool kernel_supports_fs( const Glib::ustring & fs ) ;
	static bool kernel_version_at_least( int major_ver, int minor_ver, int patch_ver ) ;
	static Glib::ustring format_size( Sector sectors, Byte_Value sector_size ) ;
	static Glib::ustring format_time( std::time_t seconds ) ;
	static double sector_to_unit( Sector sectors, Byte_Value sector_size, SIZE_UNIT size_unit ) ;
	static int execute_command( const Glib::ustring & command ) ;
	static int execute_command( const Glib::ustring & command,
				    Glib::ustring & output,
				    Glib::ustring & error,
				    bool use_C_locale = false ) ;
	static int execute_command( const Glib::ustring & command,
	                            const char * input,
	                            Glib::ustring & output,
	                            Glib::ustring & error,
				    bool use_C_locale = false );
	static int get_failure_status( Glib::SpawnError & e );
	static int decode_wait_status( int wait_status );
	static Glib::ustring regexp_label( const Glib::ustring & text
	                                 , const Glib::ustring & pattern
	                                 ) ;
	static Glib::ustring trim( const Glib::ustring & src, const Glib::ustring & c = " \t\r\n" ) ;
	static Glib::ustring last_line( const Glib::ustring & src );
	static Glib::ustring get_lang() ;
	static void tokenize( const Glib::ustring& str,
	                      std::vector<Glib::ustring>& tokens,
	                      const Glib::ustring& delimiters ) ;
	static void split( const Glib::ustring& str,
	                   std::vector<Glib::ustring>& result,
	                   const Glib::ustring& delimiters     ) ;
	static int convert_to_int(const Glib::ustring & src);
	static Glib::ustring generate_uuid(void);
	static int get_mounted_filesystem_usage( const Glib::ustring & mountpoint,
	                                         Byte_Value & fs_size, Byte_Value & fs_free,
	                                         Glib::ustring & error_message ) ;
	static Byte_Value floor_size( Byte_Value value, Byte_Value rounding_size ) ;
	static Byte_Value ceil_size( Byte_Value value, Byte_Value rounding_size ) ;

	static void nested_loop_run();
	static void nested_loop_quit();
	static bool events_pending();
	static void events_iteration();

private:
	static bool get_kernel_version( int & major_ver, int & minor_ver, int & patch_ver ) ;
};

}//GParted

#endif /* GPARTED_UTILS_H */
