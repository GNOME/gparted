/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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
 
 
#include "../include/linux_swap.h"

namespace GParted
{

const Glib::ustring linux_swap::get_custom_text( CUSTOM_TEXT ttype, int index )
{
	/*TO TRANSLATORS: these labels will be used in the partition menu */
	static const Glib::ustring activate_text = _( "_Swapon" ) ;
	static const Glib::ustring deactivate_text = _( "_Swapoff" ) ;

	switch ( ttype ) {
		case CTEXT_ACTIVATE_FILESYSTEM :
			return index == 0 ? activate_text : "" ;
		case CTEXT_DEACTIVATE_FILESYSTEM :
			return index == 0 ? deactivate_text : "" ;
		default :
			return "" ;
	}
}

FS linux_swap::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_LINUX_SWAP ;
	
	if ( ! Glib::find_program_in_path( "mkswap" ) .empty() )
	{
		fs .create = GParted::FS::EXTERNAL ;
		fs .grow = GParted::FS::EXTERNAL ;
		fs .shrink = GParted::FS::EXTERNAL ;
	}

	if ( ! Glib::find_program_in_path( "swaplabel" ) .empty() )
	{
		fs .read_label = FS::EXTERNAL ;
		fs .write_label = FS::EXTERNAL ;
		fs .read_uuid = FS::EXTERNAL ;
		fs .write_uuid = FS::EXTERNAL ;
	}

	fs .copy = GParted::FS::EXTERNAL ;
	fs .move = GParted::FS::EXTERNAL ;
	
	return fs ;
}

void linux_swap::read_label( Partition & partition )
{
	if ( ! Utils::execute_command( "swaplabel " + partition .get_path(), output, error, true ) )
	{
		partition .set_label( Utils::regexp_label( output, "^LABEL:[[:blank:]]*(.*)$" ) ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;
		
		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}

bool linux_swap::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "swaplabel -L \"" + partition .get_label() + "\" " + partition .get_path(), operationdetail ) ;
}

void linux_swap::read_uuid( Partition & partition )
{
	if ( ! Utils::execute_command( "swaplabel " + partition .get_path(), output, error, true ) )
	{
		partition .uuid = Utils::regexp_label( output, "^UUID:[[:blank:]]*(" RFC4122_NONE_NIL_UUID_REGEXP ")" ) ;
	}
	else
	{
		if ( ! output .empty() )
			partition .messages .push_back( output ) ;

		if ( ! error .empty() )
			partition .messages .push_back( error ) ;
	}
}


bool linux_swap::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "swaplabel -U \"" + Utils::generate_uuid() + "\" " + partition .get_path(), operationdetail ) ;
}

bool linux_swap::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "mkswap -L \"" + new_partition .get_label() + "\" " + new_partition .get_path(), operationdetail ) ;
}

bool linux_swap::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	/*TO TRANSLATORS: looks like create new linux-swap file system */ 
	operationdetail .add_child( OperationDetail( 
		String::ucompose( _("create new %1 file system"), Utils::get_filesystem_string( FS_LINUX_SWAP ) ) ) ) ;

	//Maintain label and uuid when recreating swap
	Glib::ustring command = "mkswap -L \"" + partition_new .get_label() + "\" " ;
	if ( ! partition_new .uuid .empty() )
		command +=  " -U \"" + partition_new .uuid + "\" " ;
	command += partition_new .get_path() ;
	bool exit_status = ! execute_command( command , operationdetail .get_last_child() ) ;

	operationdetail .get_last_child() .set_status( exit_status ? STATUS_SUCCES : STATUS_ERROR ) ;
	return exit_status ;
}

bool linux_swap::move( const Partition & partition_new
                     , const Partition & partition_old
                     , OperationDetail & operationdetail
                     )
{
	//Since linux-swap does not contain data, do not actually move the partition
	operationdetail .add_child(
	    OperationDetail(
	                     /* TO TRANSLATORS: looks like   Partition move action skipped because linux-swap file system does not contain data */
	                     String::ucompose( _("Partition move action skipped because %1 file system does not contain data")
	                                     , Utils::get_filesystem_string( FS_LINUX_SWAP )
	                                     )
	                   , STATUS_NONE
	                   , FONT_ITALIC
	                   )
	                          ) ;

	return true ;
}

bool linux_swap::copy( const Glib::ustring & src_part_path,
		       const Glib::ustring & dest_part_path,
		       OperationDetail & operationdetail )
{
	//Since linux-swap does not contain data, do not actually copy the partition
	operationdetail .add_child(
	    OperationDetail(
	                     /* TO TRANSLATORS: looks like   Partition copy action skipped because linux-swap file system does not contain data */
	                     String::ucompose( _("Partition copy action skipped because %1 file system does not contain data")
	                                     , Utils::get_filesystem_string( FS_LINUX_SWAP )
	                                     )
	                   , STATUS_NONE
	                   , FONT_ITALIC
	                   )
	                          ) ;

	return true ;
}

} //GParted
