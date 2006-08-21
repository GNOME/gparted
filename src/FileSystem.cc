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
}

int FileSystem::execute_command( const Glib::ustring & command, OperationDetail & operationdetail ) 
{
	operationdetail .add_child( OperationDetail( command, STATUS_NONE, FONT_BOLD_ITALIC ) ) ;

	int exit_status = Utils::execute_command( "nice -n 19 " + command, output, error ) ;

	if ( ! output .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( output, STATUS_NONE, FONT_ITALIC ) ) ;
	
	if ( ! error .empty() )
		operationdetail .get_last_child() .add_child( OperationDetail( error, STATUS_NONE, FONT_ITALIC ) ) ;

	return exit_status ;
}

} //GParted
