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
 
 
#include "../include/FileSystem.h"

namespace GParted
{
	
FileSystem::FileSystem()
{
	cylinder_size = 0 ;
}
	
int FileSystem::Execute_Command( Glib::ustring command ) 
{	
	
	//stderr to stdout
	//command += " 2>&1" ;
	std::cout << command << std::endl ;
	cmd_output = command + "\n\n" ;
	
	char c_buf[ 512 ] ;
	FILE *f = popen( command .c_str( ), "r" ) ;
	
	while ( fgets( c_buf, 512, f ) )
	{
		//output = Glib::locale_to_utf8( c_buf ) ;
		//std::cout << output << std::endl ;
	}
	
	cmd_output = "" ;
	
        return pclose( f ) ;
}

int FileSystem::execute_command( std::vector<std::string> argv, std::vector<OperationDetails> & operation_details ) 
{
	Glib::ustring temp ;
	for ( unsigned int t = 0 ; t < argv .size() ; t++ )
		temp += argv[ t ] + " " ;

	operation_details .push_back( OperationDetails( temp, OperationDetails::NONE ) ) ;

	envp .clear() ;
	envp .push_back( "LC_ALL=C" ) ;
	
	try
	{
		Glib::spawn_sync( ".",
				  argv,
				  envp,
				  Glib::SPAWN_SEARCH_PATH,
				  sigc::slot< void >(),
				  &output,
				  &error,
				  &exit_status ) ;
	}
	catch ( Glib::Exception & e )
	{ 
		if ( ! e .what() .empty() )
			operation_details .back() .sub_details .push_back( OperationDetails( e .what(), OperationDetails::NONE ) ) ;
		
		return -1 ;
	} 
	
	if ( ! output .empty() )
		operation_details .back() .sub_details .push_back( OperationDetails( output, OperationDetails::NONE ) ) ;
	
	if ( ! error .empty() )
		operation_details .back() .sub_details .push_back( OperationDetails( error, OperationDetails::NONE ) ) ;

	return exit_status ;
}

} //GParted
