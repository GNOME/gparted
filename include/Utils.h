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

namespace GParted
{

typedef long long Sector;
typedef long long Byte_Value;

//Size units defined in bytes
const Byte_Value KIBIBYTE=1024;
const Byte_Value MEBIBYTE=(KIBIBYTE * KIBIBYTE);
const Byte_Value GIBIBYTE=(MEBIBYTE * KIBIBYTE);
const Byte_Value TEBIBYTE=(GIBIBYTE * KIBIBYTE);

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

//struct to store file system information
struct FS
{
	enum Support
	{
		NONE		= 0,
		GPARTED		= 1,
#ifndef HAVE_LIBPARTED_3_0_0_PLUS
		LIBPARTED	= 2,
#endif
		EXTERNAL	= 3
	};

	FILESYSTEM filesystem ;
	Support read ; //can we get the amount of used sectors?
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

	Byte_Value MIN ; 
	Byte_Value MAX ;
	
	FS()
	{
		read = read_label = write_label = read_uuid = write_uuid = create = grow = shrink = move = check = copy = NONE;
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
	static Glib::ustring get_filesystem_string( FILESYSTEM filesystem ) ;
	static Glib::ustring get_filesystem_software( FILESYSTEM filesystem ) ;
	static bool kernel_supports_fs( const Glib::ustring & fs ) ;
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
	static Glib::ustring fat_compliant_label( const Glib::ustring & label ) ;
	static Glib::ustring create_mtoolsrc_file( char file_name[],
                    const char drive_letter, const Glib::ustring & device_path ) ;
	static Glib::ustring delete_mtoolsrc_file( const char file_name[] ) ;
	static Glib::ustring trim( const Glib::ustring & src, const Glib::ustring & c = " \t\r\n" ) ;
	static Glib::ustring cleanup_cursor( const Glib::ustring & text ) ;
	static Glib::ustring get_lang() ;
	static void tokenize( const Glib::ustring& str,
	                      std::vector<Glib::ustring>& tokens,
	                      const Glib::ustring& delimiters ) ;
	static void split( const Glib::ustring& str,
	                   std::vector<Glib::ustring>& result,
	                   const Glib::ustring& delimiters     ) ;
	static int convert_to_int(const Glib::ustring & src);
	static Glib::ustring generate_uuid(void);
};


}//GParted

#endif //UTILS
