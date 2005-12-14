/* Copyright (C) 2004 Bart
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
#include <sys/mount.h>

namespace GParted
{

typedef long long Sector;

#define MEGABYTE 2048  //try it: 2048 * 512 / 1024 /1024 == 1    :P	

enum FILESYSTEM
{
	FS_UNALLOCATED	= 0,
	FS_UNKNOWN	= 1,
	FS_UNFORMATTED	= 2, 
	FS_EXTENDED	= 3,
	FS_EXT2		= 4,
	FS_EXT3		= 5,
	FS_LINUX_SWAP	= 6,
	FS_FAT16	= 7,
	FS_FAT32	= 8,
	FS_NTFS		= 9,
	FS_REISERFS	= 10,
	FS_REISER4	= 11,
	FS_XFS		= 12,
	FS_JFS		= 13,
	FS_HFS		= 14,
	FS_HFSPLUS	= 15,
	FS_UFS		= 16 
};

//struct to store filesysteminformation
struct FS
{
	enum Support
	{
		NONE		= 0,
		LIBPARTED	= 1,
		EXTERNAL	= 2
	};

	FILESYSTEM filesystem ;
	Support read ; //can we get the amount of used sectors?
	Support create ;
	Support grow ;
	Support shrink ;
	Support move ; //startpoint and endpoint
	Support check ; //some checktool available?
	Support copy ;
	int MIN ; 
	int MAX ;
	
	FS( )
	{
		read = create = grow = shrink = move = check = copy = NONE;
		MIN = MAX = 0 ;
	} 
};


class Utils
{
public:
	static long Round( double double_value ) ;
	static long Sector_To_MB( Sector sectors ) ;
	static Gtk::Label * mk_label( const Glib::ustring & text,
				      bool use_markup = true,
				      bool align_left = true,
				      bool wrap = false,
				      const Glib::ustring & text_color = "black" ) ;
	static Glib::ustring num_to_str( Sector number, bool use_C_locale = false ) ;
	static Glib::ustring Get_Color( FILESYSTEM filesystem ) ;
	static Glib::ustring Get_Filesystem_String( FILESYSTEM filesystem ) ;
	static bool mount( const Glib::ustring & node, 
			   const Glib::ustring & mountpoint, 
			   const Glib::ustring & filesystem,
			   Glib::ustring & error,
			   unsigned long flags = 0, 
			   const Glib::ustring & data = "" ) ;
	static bool unmount( const Glib::ustring & node, const Glib::ustring & mountpoint, Glib::ustring & error ) ;
};
	

}//GParted

#endif //UTILS
