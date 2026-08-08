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
#include <gtkmm/image.h>
#include <gdkmm/pixbuf.h>
#include <glibmm/ustring.h>
#include <glibmm/spawn.h>
#include <iostream>
#include <ctime>
#include <vector>
#include <string>

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
	FS_INACTIVE        = 2,  // Inactive Logical Volume whose content cannot be read
	FS_UNKNOWN         = 3,  // Unrecognised content in a drive or partition
	FS_UNFORMATTED     = 4,  // Create a partition without a file system
	FS_CLEARED         = 5,  // Clear existing file system signatures
	FS_OTHER           = 6,  // Just for showing in the File System Support dialog
	FS_EXTENDED        = 7,

	// Fully supported file system types
	FS_BCACHEFS        = 8,
	FS_BTRFS           = 9,
	FS_EXFAT           = 10, /* Also known as fat64 */
	FS_EXT2            = 11,
	FS_EXT3            = 12,
	FS_EXT4            = 13,
	FS_F2FS            = 14,
	FS_FAT16           = 15,
	FS_FAT32           = 16,
	FS_HFS             = 17,
	FS_HFSPLUS         = 18,
	FS_JFS             = 19,
	FS_LINUX_SWAP      = 20,
	FS_LUKS            = 21,
	FS_LVM2_PV         = 22,
	FS_MINIX           = 23,
	FS_NILFS2          = 24,
	FS_NTFS            = 25,
	FS_REISER4         = 26,
	FS_REISERFS        = 27,
	FS_UDF             = 28,
	FS_XFS             = 29,

	// Other recognised file system types
	FS_APFS            = 30,
	FS_ATARAID         = 31,
	FS_BCACHE          = 32,
	FS_BITLOCKER       = 33,
	FS_GRUB2_CORE_IMG  = 34,
	FS_ISO9660         = 35,
	FS_JBD             = 36,
	FS_LINUX_SWRAID    = 37,
	FS_LINUX_SWSUSPEND = 38,
	FS_LVM2_THINPOOL   = 39,  // LVM2 thin pool LV (read-only: displayed as a flat LV)
	FS_REFS            = 40,
	FS_UFS             = 41,
	FS_ZFS             = 42,

	// Partition space usage colours
	FS_USED            = 43,
	FS_UNUSED          = 44
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
	                            , Gtk::Align valign = Gtk::ALIGN_CENTER
	                            ) ;
	static Gtk::Image* mk_image(const Gtk::StockID& stock_id, Gtk::IconSize icon_size);
	static Glib::RefPtr<Gdk::Pixbuf> mk_pixbuf(Gtk::Widget& widget,
	                                           const Gtk::StockID& stock_id,
	                                           Gtk::IconSize icon_size);
	static Glib::ustring get_stock_label(const Gtk::StockID& stock_id);
	static Glib::ustring num_to_str( Sector number ) ;
	static Glib::ustring get_color(FSType fstype);
	static Glib::RefPtr<Gdk::Pixbuf> get_color_as_pixbuf(FSType fstype, int width, int height);
	static int get_max_partition_name_length( Glib::ustring & tabletype );
	static int get_filesystem_label_maxlength(FSType fstype);
	static const Glib::ustring get_filesystem_string(FSType fstype);
	static const Glib::ustring get_encrypted_string();
	static const Glib::ustring get_filesystem_string( bool encrypted, FSType fstype );
	static const Glib::ustring get_filesystem_kernel_name( FSType fstype );
	static const Glib::ustring get_filesystem_software(FSType fstype);
	static const Glib::ustring generate_encryption_mapping_name(const Glib::ustring& path);
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
	static int get_failure_status(const Glib::SpawnError& e);
	static int decode_wait_status( int wait_status );
	static std::string convert_ustring(const Glib::ustring& ustr);
	static Glib::ustring regexp_label( const Glib::ustring & text
	                                 , const Glib::ustring & pattern
	                                 ) ;
	static Glib::ustring trim( const Glib::ustring & src, const Glib::ustring & c = " \t\r\n" ) ;
	static Glib::ustring trim_trailing_new_line(const Glib::ustring& src);
	static Glib::ustring last_line( const Glib::ustring & src );
	static Glib::ustring get_lang() ;
	static void tokenize( const Glib::ustring& str,
	                      std::vector<Glib::ustring>& tokens,
	                      const Glib::ustring& delimiters ) ;
	static void split( const Glib::ustring& str,
	                   std::vector<Glib::ustring>& result,
	                   const Glib::ustring& delimiters     ) ;
	static Glib::ustring generate_uuid(void);
	static int get_mounted_filesystem_usage( const Glib::ustring & mountpoint,
	                                         Byte_Value & fs_size, Byte_Value & fs_free,
	                                         Glib::ustring & error_message ) ;
	static bool is_dev_busy(const Glib::ustring& path);
	static const Glib::ustring& first_directory(const std::vector<Glib::ustring>& paths);
	static Byte_Value floor_size( Byte_Value value, Byte_Value rounding_size ) ;
	static Byte_Value ceil_size( Byte_Value value, Byte_Value rounding_size ) ;

private:
	static bool get_kernel_version( int & major_ver, int & minor_ver, int & patch_ver ) ;
};


}  // namespace GParted


#endif /* GPARTED_UTILS_H */
