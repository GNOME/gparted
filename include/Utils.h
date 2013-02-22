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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
 
 
/* UTILS
 * Some stuff i need in a lot of places so i dropped in all together in one file.
 */

#ifndef UTILS
#define UTILS

#include "../include/i18n.h"

#include <gtkmm/label.h>
#include <glibmm/ustring.h>

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

const Glib::ustring UUID_RANDOM = _("(New UUID - will be randomly generated)") ;
const Glib::ustring UUID_RANDOM_NTFS_HALF = _("(Half new UUID - will be randomly generated)") ;

enum FILESYSTEM
{
	FS_UNALLOCATED	= 0,
	FS_UNKNOWN	= 1,
	FS_UNFORMATTED	= 2,
	FS_EXTENDED	= 3,

	FS_BTRFS	= 4,
	FS_EXFAT	= 5, /* Also known as fat64 */
	FS_EXT2		= 6,
	FS_EXT3		= 7,
	FS_EXT4		= 8,
	FS_FAT16	= 9,
	FS_FAT32	= 10,
	FS_HFS		= 11,
	FS_HFSPLUS	= 12,
	FS_JFS		= 13,
	FS_LINUX_SWAP	= 14,
	FS_LVM2_PV	= 15,
	FS_NILFS2	= 16,
	FS_NTFS		= 17,
	FS_REISER4	= 18,
	FS_REISERFS	= 19,
	FS_UFS		= 20,
	FS_XFS		= 21,

	FS_USED		= 22,
	FS_UNUSED	= 23,

	FS_LUKS		= 24
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

enum CUSTOM_TEXT
{
	CTEXT_NONE,
	CTEXT_ACTIVATE_FILESYSTEM,		// Activate text ('Mount', 'Swapon', VG 'Activate', ...)
	CTEXT_DEACTIVATE_FILESYSTEM,		// Deactivate text ('Unmount', 'Swapoff', VG 'Deactivate', ...)
	CTEXT_CHANGE_UUID_WARNING,		// Warning to print when changing UUIDs
	CTEXT_RESIZE_DISALLOWED_WARNING		// File system resizing currently disallowed reason
} ;

//struct to store file system information
struct FS
{
	enum Support
	{
		NONE		= 0,
		GPARTED		= 1,
		LIBPARTED	= 2,
		EXTERNAL	= 3
	};

	FILESYSTEM filesystem ;
	Support read ;  //Can and how to read sector usage while inactive
	Support read_label ;
	Support write_label ;
	Support read_uuid ;
	Support write_uuid ;
	Support create ;
	Support grow ;
	Support shrink ;
	Support move ; //startpoint and endpoint
	Support check ; //some checktool available?
	Support copy ;
	Support remove ;
	Support online_read ;  //Can and how to read sector usage while active

	Byte_Value MIN ; 
	Byte_Value MAX ;
	
	FS()
	{
		read = read_label = write_label = read_uuid = write_uuid = create = grow = shrink =
		move = check = copy = remove = online_read = NONE ;
		MIN = MAX = 0 ;
	} 
} ;


class Utils
{
public:
	static Sector round( double double_value ) ;
	static Gtk::Label * mk_label( const Glib::ustring & text,
				      bool use_markup = true,
			      	      Gtk::AlignmentEnum x_align = Gtk::ALIGN_LEFT,
			      	      Gtk::AlignmentEnum y_align = Gtk::ALIGN_CENTER,
				      bool wrap = false,
				      bool selectable = false,
				      const Glib::ustring & text_color = "black" ) ;
	static Glib::ustring num_to_str( Sector number ) ;
	static Glib::ustring get_color( FILESYSTEM filesystem ) ;
	static Glib::RefPtr<Gdk::Pixbuf> get_color_as_pixbuf( FILESYSTEM filesystem, int width, int height ) ;
	static int get_filesystem_label_maxlength( FILESYSTEM filesystem ) ;
	static Glib::ustring get_filesystem_string( FILESYSTEM filesystem ) ;
	static Glib::ustring get_filesystem_software( FILESYSTEM filesystem ) ;
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
	static Glib::ustring regexp_label( const Glib::ustring & text
	                                 , const Glib::ustring & pattern
	                                 ) ;
	static Glib::ustring create_mtoolsrc_file( char file_name[],
                    const char drive_letter, const Glib::ustring & device_path ) ;
	static Glib::ustring delete_mtoolsrc_file( const char file_name[] ) ;
	static Glib::ustring trim( const Glib::ustring & src, const Glib::ustring & c = " \t\r\n" ) ;
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
	                                         Glib::ustring error_message ) ;

private:
	static bool get_kernel_version( int & major_ver, int & minor_ver, int & patch_ver ) ;
};

}//GParted

#endif //UTILS
