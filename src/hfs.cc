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
 
 
#include "../include/hfs.h"

namespace GParted
{

FS hfs::get_filesystem_support()
{
	FS fs ;
		
	fs .filesystem = GParted::FS_HFS ;

#ifdef HAVE_LIBPARTED_FS_RESIZE
	fs .read = GParted::FS::LIBPARTED ;
	fs .shrink = GParted::FS::LIBPARTED ;
#endif

	if ( ! Glib::find_program_in_path( "hformat" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "hfsck" ) .empty() )
		fs .check = FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "vol_id" ) .empty() )
		fs .read_label = FS::EXTERNAL ;

	fs .copy = GParted::FS::GPARTED ;
	fs .move = GParted::FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	fs .MAX = 2048 * MEBIBYTE ;
	
	return fs ;
}

void hfs::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "vol_id " + partition .get_path(), output, error, true ) )
	{
		Glib::ustring label = Utils::regexp_label( output, "ID_FS_LABEL=([^\n]*)" ) ;
		//FIXME: find a better way to see if label is empty.. imagine someone uses 'untitled' as label.... ;)
		if ( label != "untitled" )
			partition .set_label( label ) ;
		else
			partition .set_label( "" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool hfs::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "";
	if( new_partition .get_label() .empty() )
		cmd = "hformat " + new_partition .get_path() ;
	else
		cmd = "hformat -l \"" + new_partition .get_label() + "\" " + new_partition .get_path() ;
	return ! execute_command( cmd , operationdetail ) ;
}

bool hfs::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	//FIXME: find out what the returnvalue is in case of modified.. also check what the -a flag does.. (there is no manpage)
	return ! execute_command( "hfsck -v " + partition .get_path(), operationdetail ) ;
}

} //GParted
