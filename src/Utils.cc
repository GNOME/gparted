/* Copyright (C) 2004 Bart 'plors' Hakvoort
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
 
#include "../include/Utils.h"

#include <sstream>
//#include <vector>

namespace GParted
{
	
long Utils::Round( double double_value )
{
	 return static_cast<long>( double_value + 0.5 ) ;
}

long Utils::Sector_To_MB( Sector sectors ) 
{
	 return Round( sectors * 0.000488281250 ) ; // that's what 512/1024/1024 gives you :)
}

Gtk::Label * Utils::mk_label( const Glib::ustring & text, bool use_markup, bool align_left, bool wrap, const Glib::ustring & text_color ) 
{
	Gtk::Label * label = manage( new Gtk::Label( text ) ) ;
	
	label ->set_use_markup( use_markup ) ;
	
	if ( align_left )
		label ->set_alignment( Gtk::ALIGN_LEFT ) ;
	
	label ->set_line_wrap( wrap ) ;
	
	if ( text_color != "black" )
	{
		Gdk::Color color( text_color ) ;
		label ->modify_fg( label ->get_state( ), color ) ;
	}
	
	return label ;
}

Glib::ustring Utils::num_to_str( Sector number, bool use_C_locale )
{
	std::stringstream ss ;
	//ss.imbue( std::locale( use_C_locale ? "C" : "" ) ) ; see #157871
	ss << number ;
	return ss .str( ) ;
}

//use http://developer.gnome.org/projects/gup/hig/2.0/design.html#Palette as a starting point..
Glib::ustring Utils::Get_Color( FILESYSTEM filesystem ) 
{ 
	switch( filesystem )
	{
		case FS_UNALLOCATED	: return "darkgrey" ; 
		case FS_UNKNOWN		: return "black" ;
		case FS_UNFORMATTED	: return "black" ;
		case FS_EXTENDED	: return "#7DFCFE" ;
		case FS_EXT2		: return "#9DB8D2" ;
		case FS_EXT3		: return "#7590AE" ;
		case FS_LINUX_SWAP	: return "#C1665A" ;
		case FS_FAT16		: return "green" ;
		case FS_FAT32		: return "#18D918" ;
		case FS_NTFS		: return "#42E5AC" ;
		case FS_REISERFS	: return "#ADA7C8" ;
		case FS_REISER4		: return "#887FA3" ;
		case FS_XFS		: return "#EED680" ;
		case FS_JFS		: return "#E0C39E" ;
		case FS_HFS		: return "#E0B6AF" ;
		case FS_HFSPLUS		: return "#C0A39E" ;
		case FS_UFS		: return "#D1940C" ;

		default			: return "black" ;
	}
}

Glib::ustring Utils::Get_Filesystem_String( FILESYSTEM filesystem )
{
	switch( filesystem )
	{
		case FS_UNALLOCATED	: return _("unallocated") ; 
		case FS_UNKNOWN		: return _("unknown") ;
		case FS_UNFORMATTED	: return _("unformatted") ;
		case FS_EXTENDED	: return "extended" ;
		case FS_EXT2		: return "ext2" ;
		case FS_EXT3		: return "ext3" ;
		case FS_LINUX_SWAP	: return "linux-swap" ;
		case FS_FAT16		: return "fat16" ;
		case FS_FAT32		: return "fat32" ;
		case FS_NTFS		: return "ntfs" ;
		case FS_REISERFS	: return "reiserfs" ;
		case FS_REISER4		: return "reiser4" ;
		case FS_XFS		: return "xfs" ;
		case FS_JFS		: return "jfs" ;
		case FS_HFS		: return "hfs" ;
		case FS_HFSPLUS		: return "hfs+" ;
		case FS_UFS		: return "ufs" ;
					  
		default			: return "" ;
	}
}

} //GParted..
