/* Copyright (C) 2012 Mike Fleetwood
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


#include "../include/LVM2_PV_Info.h"
#include "../include/lvm2_pv.h"

namespace GParted
{

const Glib::ustring lvm2_pv::get_custom_text( CUSTOM_TEXT ttype, int index )
{
	/*TO TRANSLATORS: these labels will be used in the partition menu */
	static const Glib::ustring activate_text = _( "Ac_tivate" ) ;
	static const Glib::ustring deactivate_text = _( "Deac_tivate" ) ;

	static const Glib::ustring resize_warning =
		_( "The LVM2 Physical Volume can not currently be resized because it is a member of an exported "
		   "Volume Group." ) ;

	switch ( ttype )
	{
		case CTEXT_ACTIVATE_FILESYSTEM:
			return index == 0 ? activate_text : "" ;
		case CTEXT_DEACTIVATE_FILESYSTEM:
			return index == 0 ? deactivate_text : "" ;
		case CTEXT_RESIZE_DISALLOWED_WARNING:
			return index == 0 ? resize_warning : "" ;
		default:
			return "" ;
	}
}

FS lvm2_pv::get_filesystem_support()
{
	FS fs ;
	fs .filesystem = GParted::FS_LVM2_PV ;

	LVM2_PV_Info lvm2_pv_info ;
	if ( lvm2_pv_info .is_lvm2_pv_supported() )
	{
		fs .read   = FS::EXTERNAL ;
		fs .create = FS::EXTERNAL ;
		fs .grow   = FS::EXTERNAL ;
		fs .shrink = FS::EXTERNAL ;
		fs .move   = FS::GPARTED ;
		fs .check  = FS::EXTERNAL ;
		fs .remove = FS::EXTERNAL ;
		fs .online_read = FS::EXTERNAL ;
	}

	return fs ;
}

void lvm2_pv::set_used_sectors( Partition & partition )
{
	LVM2_PV_Info lvm2_pv_info ;
	T = (Sector) lvm2_pv_info .get_size_bytes( partition .get_path() ) ;
	N = (Sector) lvm2_pv_info .get_free_bytes( partition .get_path() ) ;
	if ( T > -1 && N > -1 )
	{
		T = Utils::round( T / double(partition .sector_size) ) ;
		N = Utils::round( N / double(partition .sector_size) ) ;
		partition .set_sector_usage( T, N ) ;
	}

	std::vector<Glib::ustring> error_messages = lvm2_pv_info .get_error_messages( partition .get_path() ) ;
	if ( ! error_messages .empty() )
	{
		for ( unsigned int i = 0 ; i < error_messages .size() ; i ++ )
			partition .messages .push_back( error_messages [ i ] ) ;
	}
}

bool lvm2_pv::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "lvm pvcreate -M 2 " + new_partition .get_path(), operationdetail ) ;
}

bool lvm2_pv::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	Glib::ustring size = "" ;
	if ( ! fill_partition )
		size = " --setphysicalvolumesize " +
			Utils::num_to_str( floor( Utils::sector_to_unit(
				partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) ) + "K " ;
	return ! execute_command( "lvm pvresize -v " + size + partition_new .get_path(), operationdetail ) ;
}

bool lvm2_pv::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "lvm pvck -v " + partition .get_path(), operationdetail ) ;
}

bool lvm2_pv::remove( const Partition & partition, OperationDetail & operationdetail )
{
	LVM2_PV_Info lvm2_pv_info ;
	Glib::ustring vgname = lvm2_pv_info .get_vg_name( partition .get_path() ) ;
	Glib::ustring cmd ;
	if ( vgname .empty() )
		cmd = "lvm pvremove " + partition .get_path() ;
	else
		//Must force the removal of a PV which is a member of a VG
		cmd = "lvm pvremove --force --force --yes " + partition .get_path() ;
	return ! execute_command( cmd, operationdetail ) ;
}

} //GParted
