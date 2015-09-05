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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
 
 
#include "../include/hfsplus.h"

namespace GParted
{

FS hfsplus::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_HFSPLUS ;

	fs .busy = FS::GPARTED ;

#ifdef HAVE_LIBPARTED_FS_RESIZE
	fs .read = GParted::FS::LIBPARTED ;
	fs .shrink = GParted::FS::LIBPARTED ;
#endif

	if ( ! Glib::find_program_in_path( "mkfs.hfsplus" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .create_with_label = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "fsck.hfsplus" ) .empty() )
		fs .check = FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "vol_id" ) .empty() )
		fs .read_label = FS::EXTERNAL ;

	fs .copy = GParted::FS::GPARTED ;
	fs .move = GParted::FS::GPARTED ;
	fs .online_read = FS::GPARTED ;

	return fs ;
}

void hfsplus::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "vol_id " + partition .get_path(), output, error, true ) )
	{
		Glib::ustring label = Utils::regexp_label( output, "ID_FS_LABEL=([^\n]*)" ) ;
		//FIXME: find a better way to see if label is empty.. imagine someone uses 'untitled' as label.... ;)
		if ( label != "untitled" )
			partition.set_filesystem_label( label );
		else
			partition.set_filesystem_label( "" );
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool hfsplus::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "";
	if( new_partition.get_filesystem_label().empty() )
		cmd = "mkfs.hfsplus " + new_partition .get_path() ;
	else
		cmd = "mkfs.hfsplus -v \"" + new_partition.get_filesystem_label() + "\" " + new_partition.get_path();
	return ! execute_command( cmd , operationdetail, EXEC_CHECK_STATUS );
}

bool hfsplus::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "fsck.hfsplus -f -y " + partition.get_path(), operationdetail, EXEC_CHECK_STATUS );
}

} //GParted
