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
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */


#include "../include/LVM2_PV_Info.h"
#include "../include/lvm2_pv.h"

namespace GParted
{

const Glib::ustring lvm2_pv::get_custom_text( CUSTOM_TEXT ttype, int index ) const
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

	if ( LVM2_PV_Info::is_lvm2_pv_supported() )
	{
		fs .busy   = FS::EXTERNAL ;
		fs .read   = FS::EXTERNAL ;
		fs .create = FS::EXTERNAL ;
		fs .grow   = FS::EXTERNAL ;
		fs .shrink = FS::EXTERNAL ;
		fs .move   = FS::GPARTED ;
		fs .check  = FS::EXTERNAL ;
		fs .remove = FS::EXTERNAL ;
		fs .online_read = FS::EXTERNAL ;
#ifdef ENABLE_ONLINE_RESIZE
		if ( Utils::kernel_version_at_least( 3, 6, 0 ) )
		{
			fs .online_grow = fs .grow ;
			fs .online_shrink = fs.shrink ;
		}
#endif
	}

	return fs ;
}

bool lvm2_pv::is_busy( const Glib::ustring & path )
{
	return LVM2_PV_Info::has_active_lvs( path );
}

void lvm2_pv::set_used_sectors( Partition & partition )
{
	T = (Sector) LVM2_PV_Info::get_size_bytes( partition.get_path() );
	N = (Sector) LVM2_PV_Info::get_free_bytes( partition.get_path() );
	if ( T > -1 && N > -1 )
	{
		T = Utils::round( T / double(partition .sector_size) ) ;
		N = Utils::round( N / double(partition .sector_size) ) ;
		partition .set_sector_usage( T, N ) ;
	}

	std::vector<Glib::ustring> error_messages = LVM2_PV_Info::get_error_messages( partition.get_path() );
	if ( ! error_messages .empty() )
	{
		for ( unsigned int i = 0 ; i < error_messages .size() ; i ++ )
			partition .messages .push_back( error_messages [ i ] ) ;
	}
}

bool lvm2_pv::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	return ! execute_command( "lvm pvcreate -M 2 " + new_partition.get_path(), operationdetail, EXEC_CHECK_STATUS );
}

bool lvm2_pv::resize( const Partition & partition_new, OperationDetail & operationdetail, bool fill_partition )
{
	Glib::ustring size = "" ;
	if ( ! fill_partition )
		size = " --setphysicalvolumesize " +
			Utils::num_to_str( floor( Utils::sector_to_unit(
				partition_new .get_sector_length(), partition_new .sector_size, UNIT_KIB ) ) ) + "K " ;
	return ! execute_command( "lvm pvresize -v " + size + partition_new.get_path(),
	                          operationdetail, EXEC_CHECK_STATUS );
}

bool lvm2_pv::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	return ! execute_command( "lvm pvck -v " + partition.get_path(), operationdetail, EXEC_CHECK_STATUS );
}

bool lvm2_pv::remove( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring vgname = LVM2_PV_Info::get_vg_name( partition.get_path() );
	Glib::ustring cmd ;
	if ( vgname .empty() )
		cmd = "lvm pvremove " + partition .get_path() ;
	else
		//Must force the removal of a PV which is a member of a VG
		cmd = "lvm pvremove --force --force --yes " + partition .get_path() ;
	return ! execute_command( cmd, operationdetail, EXEC_CHECK_STATUS );
}

} //GParted
