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
	
int FileSystem::execute_command( std::vector<std::string> argv, std::vector<OperationDetails> & operation_details ) 
{
	Glib::ustring temp ;
	for ( unsigned int t = 0 ; t < argv .size() ; t++ )
		temp += argv[ t ] + " " ;

	operation_details .push_back( OperationDetails( temp, OperationDetails::NONE ) ) ;

	try
	{
		Glib::spawn_sync( ".",
				  argv,
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

int FileSystem::execute_command( std::vector<std::string> argv, std::string & output ) 
{
	std::vector<std::string> envp ;
	envp .push_back( "LC_ALL=C" ) ;
	
	try
	{
		Glib::spawn_sync( ".", 
				  argv,
				  envp,
				  Glib::SPAWN_SEARCH_PATH,
				  sigc::slot<void>(),
				  &output,
				  &error, //dummy
				  &exit_status) ;
	}
	catch ( Glib::Exception & e )
	{ 
		std::cout << e .what() << std::endl ;
	}

	return exit_status ;
}

} //GParted
