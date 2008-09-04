/* Copyright (C) 2004, 2005, 2006, 2007, 2008 Bart Hakvoort
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
#include <iomanip>
#include <regex.h>

namespace GParted
{
	
Sector Utils::round( double double_value )
{
	 return static_cast<Sector>( double_value + 0.5 ) ;
}

Gtk::Label * Utils::mk_label( const Glib::ustring & text,
			      bool use_markup,
			      Gtk::AlignmentEnum x_align,
			      Gtk::AlignmentEnum y_align,
			      bool wrap,
			      const Glib::ustring & text_color ) 
{

	Gtk::Label * label = manage( new Gtk::Label( text, x_align, y_align ) ) ;
	
	label ->set_use_markup( use_markup ) ;
	label ->set_line_wrap( wrap ) ;
	
	if ( text_color != "black" )
	{
		Gdk::Color color( text_color ) ;
		label ->modify_fg( label ->get_state(), color ) ;
	}
	
	return label ;
}

Glib::ustring Utils::num_to_str( Sector number, bool use_C_locale )
{
	std::stringstream ss ;
	//ss.imbue( std::locale( use_C_locale ? "C" : "" ) ) ; see #157871
	ss << number ;
	return ss .str() ;
}

//use http://developer.gnome.org/projects/gup/hig/2.0/design.html#Palette as a starting point..
Glib::ustring Utils::get_color( FILESYSTEM filesystem ) 
{ 
	switch( filesystem )
	{
		case FS_UNALLOCATED	: return "#A9A9A9" ;
		case FS_UNKNOWN		: return "#000000" ;
		case FS_UNFORMATTED	: return "#000000" ;
		case FS_EXTENDED	: return "#7DFCFE" ;
		case FS_EXT2		: return "#9DB8D2" ;
		case FS_EXT3		: return "#7590AE" ;
		case FS_LINUX_SWAP	: return "#C1665A" ;
		case FS_FAT16		: return "#00FF00" ;
		case FS_FAT32		: return "#18D918" ;
		case FS_NTFS		: return "#42E5AC" ;
		case FS_REISERFS	: return "#ADA7C8" ;
		case FS_REISER4		: return "#887FA3" ;
		case FS_XFS		: return "#EED680" ;
		case FS_JFS		: return "#E0C39E" ;
		case FS_HFS		: return "#E0B6AF" ;
		case FS_HFSPLUS		: return "#C0A39E" ;
		case FS_UFS		: return "#D1940C" ;
		case FS_USED		: return "#F8F8BA" ;
		case FS_UNUSED		: return "#FFFFFF" ;

		default			: return "#000000" ;
	}
}

Glib::RefPtr<Gdk::Pixbuf> Utils::get_color_as_pixbuf( FILESYSTEM filesystem, int width, int height ) 
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

Glib::ustring Utils::get_filesystem_string( FILESYSTEM filesystem )
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
		case FS_USED		: return _("used") ;
		case FS_UNUSED		: return _("unused") ;
					  
		default			: return "" ;
	}
}

Glib::ustring Utils::format_size( Sector size ) 
{
	std::stringstream ss ;	
	//ss .imbue( std::locale( "" ) ) ;  see #157871
	ss << std::setiosflags( std::ios::fixed ) << std::setprecision( 2 ) ;
	
	if ( size < KIBIBYTE )
	{
		ss << sector_to_unit( size, UNIT_BYTE ) ;
		return String::ucompose( _("%1 B"), ss .str() ) ;
	}
	else if ( size < MEBIBYTE ) 
	{
		ss << sector_to_unit( size, UNIT_KIB ) ;
		return String::ucompose( _("%1 KiB"), ss .str() ) ;
	}
	else if ( size < GIBIBYTE ) 
	{
		ss << sector_to_unit( size, UNIT_MIB ) ;
		return String::ucompose( _("%1 MiB"), ss .str() ) ;
	}
	else if ( size < TEBIBYTE ) 
	{
		ss << sector_to_unit( size, UNIT_GIB ) ;
		return String::ucompose( _("%1 GiB"), ss .str() ) ;
	}
	else
	{
		ss << sector_to_unit( size, UNIT_TIB ) ;
		return String::ucompose( _("%1 TiB"), ss .str() ) ;
	}
}
	
Glib::ustring Utils::format_time( std::time_t seconds ) 
{
	Glib::ustring time ;
	
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

double Utils::sector_to_unit( Sector sectors, SIZE_UNIT size_unit ) 
{
	switch ( size_unit )
	{
		case UNIT_BYTE	:
			return sectors * 512 ;
		
		case UNIT_KIB	:
			return sectors / static_cast<double>( KIBIBYTE ) ;
		case UNIT_MIB	:
			return sectors / static_cast<double>( MEBIBYTE ) ;
		case UNIT_GIB	:
			return sectors / static_cast<double>( GIBIBYTE ) ;
		case UNIT_TIB	:
			return sectors / static_cast<double>( TEBIBYTE ) ;
		
		default:
			return sectors ;
	}
}
	
int Utils::execute_command( const Glib::ustring & command ) 
{
	Glib::ustring dummy ;
	return execute_command( command, dummy, dummy ) ;
}

int Utils::execute_command( const Glib::ustring & command,
		     	    Glib::ustring & output,
			    Glib::ustring & error,
		     	    bool use_C_locale )
{
	int exit_status = -1 ;
	std::string std_out, std_error ;
	
	try
	{
		if ( use_C_locale )
		{
			std::vector<std::string> envp, argv;
			envp .push_back( "LC_ALL=C" ) ;
			envp .push_back( "PATH=" + Glib::getenv( "PATH" ) ) ;

			argv .push_back( "sh" ) ;
			argv .push_back( "-c" ) ;
			argv .push_back( command ) ;

			Glib::spawn_sync( ".", 
					  argv,
					  envp,
					  Glib::SPAWN_SEARCH_PATH,
					  sigc::slot<void>(),
					  &std_out,
					  &std_error, 
					  &exit_status ) ;
		}
		else
			Glib::spawn_command_line_sync( "sh -c '" + command + "'", &std_out, &std_error, &exit_status ) ;
	}
	catch ( Glib::Exception & e )
	{ 
		 error = e .what() ;

		 return -1 ;
	}

	output = Utils::cleanup_cursor( std_out ) ;
	error = std_error ;
	
	return exit_status ;
}

Glib::ustring Utils::regexp_label( const Glib::ustring & text,
				const Glib::ustring & regular_sub_expression )
{
	//Extract text from a regular sub-expression.  E.g., "text we don't want (text we want)"
	Glib::ustring label = "";
	regex_t    preg ; 
	int        nmatch = 2 ;
	regmatch_t pmatch[  2 ] ;
	int rc = regcomp( &preg, regular_sub_expression .c_str(), REG_EXTENDED | REG_ICASE | REG_NEWLINE ) ;
	if (   ( rc == 0 ) //Reg compile OK
		&& ( regexec( &preg, text .c_str(), nmatch, pmatch, 0 ) == 0 ) //Match found
	   )
	{
		label = text .substr( pmatch[1].rm_so, pmatch[1].rm_eo - pmatch[1].rm_so ) ;
	}
	return label ;
}

Glib::ustring Utils::create_mtoolsrc_file( char file_name[], const char drive_letter,
		const Glib::ustring & device_path )
{
	//Create mtools config file
	//NOTE:  file_name will be changed by the mkstemp() function call.
	Glib::ustring err_msg = "" ;
	int fd ;
	fd = mkstemp( file_name ) ;
	if( fd != -1 ) {
		Glib::ustring fcontents =
			_("# Temporary file created by gparted.  It may be deleted.\n") ;

		//The following file contents are mtools keywords (see man mtools.conf)
		fcontents = String::ucompose(
						"drive %1: file=\"%2\"\nmtools_skip_check=1\n", drive_letter, device_path
					);
		
		if( write( fd, fcontents .c_str(), fcontents .size() ) == -1 ) {
			err_msg = String::ucompose(
							_("Label operation failed:  Unable to write to temporary file %1.\n")
							, file_name
						) ;
		}
		close( fd ) ;
	}
	else
	{
		err_msg = String::ucompose(
						_("Label operation failed:  Unable to create temporary file %1.\n")
						, file_name
					) ;
	}
	return err_msg ;
}

Glib::ustring Utils::delete_mtoolsrc_file( const char file_name[] )
{
	//Delete mtools config file
	Glib::ustring err_msg = "" ;
	remove( file_name ) ;
	return err_msg ;
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

Glib::ustring Utils::cleanup_cursor( const Glib::ustring & text )
{
	//Clean up text for commands that use cursoring to display progress.
	Glib::ustring str = text;
	//remove backspace '\b' and delete previous character.  Used in mke2fs output.
	for ( unsigned int index = str .find( "\b" ) ; index < str .length() ; index = str .find( "\b" ) ) {
		if ( index > 0 )
			str .erase( index - 1, 2 ) ;
		else
			str .erase( index, 1 ) ;
	}
	
	//remove carriage return and line up to previous line feed.  Used in ntfsclone output.
	//NOTE:  Normal linux line end is line feed.  DOS uses CR + LF.
	for ( unsigned int index1 = str .find( "\r") ; index1 < str .length() ; index1 = str .find( "\r" ) ) {
		if ( str .at(index1 + 1) != '\n') { //Only process if next character is not a LF
			unsigned int index2 = str .rfind( "\n", index1 ) ;
			if ( index2 <= index1 )
				str .erase( index2 + 1, index1 - index2 ) ;
		}
	}
	return str;
}


} //GParted..
