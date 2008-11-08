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
 
 
#include "../include/hfsplus.h"

namespace GParted
{

FS hfsplus::get_filesystem_support()
{
	FS fs ;
	
	fs .filesystem = GParted::FS_HFSPLUS ;
	
	fs .read = GParted::FS::LIBPARTED ; 
	fs .shrink = GParted::FS::LIBPARTED ; 
	
	if ( ! Glib::find_program_in_path( "mkfs.hfsplus" ) .empty() )
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! Glib::find_program_in_path( "fsck.hfsplus" ) .empty() )
		fs .check = FS::EXTERNAL ;

	if ( ! Glib::find_program_in_path( "vol_id" ) .empty() )
		fs .read_label = FS::EXTERNAL ;

	fs .copy = GParted::FS::GPARTED ;
	fs .move = GParted::FS::GPARTED ;
	
	return fs ;
}

void hfsplus::set_used_sectors( Partition & partition ) 
{
}

void hfsplus::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "vol_id " + partition .get_path(), output, error, true ) )
	{
		Glib::ustring label = Utils::regexp_label( output, "ID_FS_LABEL=([^\n]*)" ) ;
		//FIXME: find a better way to see if label is empty.. imagine someone uses 'untitled' as label.... ;)
		if( label != "untitled" ) 
			partition .label = label ; 
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool hfsplus::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return true ;
}

bool hfsplus::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd = "";
	if( new_partition .label .empty() )
		cmd = "mkfs.hfsplus " + new_partition .get_path() ;
	else
		cmd = "mkfs.hfsplus -v \"" + new_partition .label + "\" " + new_partition .get_path() ;
	return ! execute_command( cmd , operationdetail ) ;
}

bool hfsplus::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	return true ;
}

bool hfsplus::copy( const Glib::ustring & src_part_path,
		    const Glib::ustring & dest_part_path,
		    OperationDetail & operationdetail )
{
	return true ;
}

bool hfsplus::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "fsck.hfsplus -f -y " + partition .get_path(), operationdetail ) ;
}

} //GParted
