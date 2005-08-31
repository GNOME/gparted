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

#include <gtkmm/label.h>
#include <glibmm/ustring.h>

#include <sstream>
#include <vector>

namespace GParted
{

typedef long long Sector;

#define MEGABYTE 2048  //try it: 2048 * 512 / 1024 /1024 == 1    :P	


//struct to store filesystems
struct FS
{
	Glib::ustring filesystem ;
	bool read ; //can we get the amount of used sectors?
	bool create ;
	bool grow ;
	bool shrink ;
	bool move ; //startpoint and endpoint
	bool check ; //some checktool available?
	bool copy ;
	int MIN ; 
	int MAX ;
	
	FS( )
	{
		read = create = grow = shrink = move = check = copy = false ;
		MIN = MAX = 0 ;
	} 
};

	
//globally used convenience functions
inline long Round( double double_value )
{
	 return static_cast<long> ( double_value + 0.5 ) ;
}

inline long Sector_To_MB( Sector sectors ) 
{
	 return Round( sectors * 0.000488281250 ) ; // that's what 512/1024/1024 gives you :)
}

inline Sector Abs( Sector sectors )
{
	return sectors < 0 ? sectors - 2*sectors : sectors ;
}

inline Gtk::Label * mk_label( const Glib::ustring & text, bool use_markup = true, bool align_left = true, bool wrap = false, const Glib::ustring & text_color = "black" ) 
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

inline Glib::ustring num_to_str( Sector number )
{
	std::stringstream ss;
	ss << number ;
	return ss .str( ) ;
}

//use http://developer.gnome.org/projects/gup/hig/2.0/design.html#Palette as a starting point..
inline Glib::ustring Get_Color( const Glib::ustring & filesystem ) 
{ 
	//blue teints
	if	( filesystem == "ext2" )	return "#9DB8D2" ;													
	else if ( filesystem == "ext3" )	return "#7590AE" ;								
	
	//redbrown
	else if ( filesystem == "linux-swap" )	return "#C1665A" ;				
		
	//greenisch stuff..
	else if ( filesystem == "fat16" ) 	return "green"	 ;			
	else if	( filesystem == "fat32" )	return "#18D918" ;							
	else if ( filesystem == "ntfs" )	return "#42E5AC" ;				
	
	//purple something..
	else if	( filesystem == "reiserfs" )	return "#ADA7C8" ;
	else if	( filesystem == "reiser4" )	return "#887FA3" ;						
	
	//darkyellow
	else if	( filesystem == "xfs" )		return "#EED680" ;
		
	else if	( filesystem == "jfs" )		return "#E0C39E" ;
		
	else if	( filesystem == "hfs" )		return "#E0B6AF" ;
		
	else if	( filesystem == "ufs" )		return "#D1940C" ;	
		
	//darkgrey and ligthblue
	else if ( filesystem == "---" )		return "darkgrey";
	else if ( filesystem == "extended" )	return "#7DFCFE" ;
	
	//unknown filesystem ( damaged, unknown or simply no filesystem )	
	else return "black";
}


}//GParted

#endif //UTILS
