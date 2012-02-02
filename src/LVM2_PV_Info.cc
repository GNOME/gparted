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

namespace GParted
{

enum PV_ATTRIBUTE
{
	PVATTR_PV_NAME = 0,
	PVATTR_VG_NAME = 1,
	PVATTR_PV_FREE = 2
} ;

enum LV_ATTRIBUTE
{
	LVATTR_LV_NAME = 0,
	LVATTR_VG_NAME = 1,
	LVATTR_LV_BITS = 2
} ;

//Data model:
//  lvm2_pv_info_cache_initialized
//                      - Has the cache been loaded let?
//  lvm_found           - Is the "lvm" command available?
//  lvm2_pv_cache       - String vector storing attributes of a PV.
//                        Attributes are: pv_name,vg_name,pv_free.
//                        Pv_free is the number of free bytes.
//                        E.g.
//                        ["/dev/sda6,vg_test,4022337536",
//                         "/dev/sda8,vg_test2,5087690752",
//                         "/dev/sda10,,2147483648"]
//  lvm2_lv_cache       - String vector storing attributes of an LV.
//                        Attributes are: lv_name,vg_name,lv_attrs.
//                        See lvs(8) for details of lv_attrs.
//                        E.g.
//                        ["lvol0,vg_test,-wi---",
//                         "lvol0,vg_test2,-wi-a-"]

//Initialize static data elements
bool LVM2_PV_Info::lvm2_pv_info_cache_initialized = false ;
bool LVM2_PV_Info::lvm_found = false ;
std::vector<Glib::ustring> LVM2_PV_Info::lvm2_pv_cache ;
std::vector<Glib::ustring> LVM2_PV_Info::lvm2_lv_cache ;

LVM2_PV_Info::LVM2_PV_Info()
{
}

LVM2_PV_Info::LVM2_PV_Info( bool do_refresh )
{
	if ( do_refresh )
	{
		set_command_found() ;
		load_lvm2_pv_info_cache() ;
		lvm2_pv_info_cache_initialized = true ;
	}
}

LVM2_PV_Info::~LVM2_PV_Info()
{
}

bool LVM2_PV_Info::is_lvm2_pv_supported()
{
	if ( ! lvm2_pv_info_cache_initialized )
		set_command_found() ;
	return ( lvm_found ) ;
}

Glib::ustring LVM2_PV_Info::get_vg_name( const Glib::ustring & path )
{
	initialize_if_required() ;
	return get_pv_attr( path, PVATTR_VG_NAME ) ;
}

//Return number of free bytes in the PV, or -1 for error.
Byte_Value LVM2_PV_Info::get_free_bytes( const Glib::ustring & path )
{
	initialize_if_required() ;
	Byte_Value free_bytes = -1 ;
	Glib::ustring fb_str = get_pv_attr( path, PVATTR_PV_FREE ) ;
	if ( fb_str != "" )
	{
		gchar * suffix ;
		free_bytes = (Byte_Value) g_ascii_strtoll( fb_str .c_str(), & suffix, 10 ) ;
		if ( free_bytes < 0 || ( free_bytes == 0 && suffix == fb_str ) )
			//Negative number or conversion failed
			free_bytes = -1 ;
	}
	return free_bytes ;
}

//Report if any LVs are active in the VG stored in the PV.
bool LVM2_PV_Info::has_active_lvs( const Glib::ustring & path )
{
	initialize_if_required() ;
	Glib::ustring vgname = get_pv_attr( path, PVATTR_VG_NAME ) ;
	if ( vgname == "" )
		//PV not yet included in any VG
		return false ;

	for ( unsigned int i = 0 ; i < lvm2_lv_cache .size() ; i ++ )
	{
		std::vector<Glib::ustring> lv_attrs ;
		Utils::split( lvm2_lv_cache [i], lv_attrs, "," ) ;
		if ( vgname == lv_attrs [LVATTR_VG_NAME] )
		{
			//5th "bit" is active status.  E.g.
			//  "-wi---" inactive, "-wi-a-" active, ...
			//  Treat any non-hyphen character as active.
			if ( lv_attrs [LVATTR_LV_BITS] [4] != '-' )
				//LV in VG is active
				return true ;
		}
	}
	return false ;
}

//Private methods

void LVM2_PV_Info::initialize_if_required()
{
	if ( ! lvm2_pv_info_cache_initialized )
	{
		set_command_found() ;
		load_lvm2_pv_info_cache() ;
		lvm2_pv_info_cache_initialized = true ;
	}
}

void LVM2_PV_Info::set_command_found()
{
	//Set status of command found
	lvm_found = ( ! Glib::find_program_in_path( "lvm" ) .empty() ) ;
}

void LVM2_PV_Info::load_lvm2_pv_info_cache()
{
	Glib::ustring output, error ;

	lvm2_pv_cache .clear() ;
	lvm2_lv_cache .clear() ;
	if ( lvm_found )
	{
		//The OS is expected to fully enable LVM, this scan does
		//  not do the full job.  It is included incase anything
		//  is changed not using lvm commands.
		Utils::execute_command( "lvm vgscan", output, error, true ) ;

		//Load LVM2 PV attribute cache.  Output PV attributes in
		//  PV_ATTRIBUTE order
		if ( ! Utils::execute_command( "lvm pvs --config \"log{command_names=0}\" --nosuffix --noheadings --separator , --units b -o pv_name,vg_name,pv_free", output, error, true ) )
		{
			if ( output != "" )
				Utils::tokenize( output, lvm2_pv_cache, " \n" ) ;
		}

		//Load LVM2 LV attribute cache.  Output LV attributes in
		//  LV_ATTRIBUTE order
		if ( ! Utils::execute_command( "lvm lvs --config \"log{command_names=0}\" --noheadings --separator , -o lv_name,vg_name,lv_attr", output, error, true ) )
		{
			if ( output != "" )
				Utils::tokenize( output, lvm2_lv_cache, " \n" ) ;
		}
	}
}

//Return PV's nth attribute.  Attributes are numbered 0 upwards for:
//  pv_name,vg_name,pv_free
Glib::ustring LVM2_PV_Info::get_pv_attr( const Glib::ustring & path, unsigned int entry )
{
	for ( unsigned int i = 0 ; i < lvm2_pv_cache .size() ; i ++ )
	{
		std::vector<Glib::ustring> pv_attrs ;
		Utils::split( lvm2_pv_cache [i], pv_attrs, "," ) ;
		if ( path == pv_attrs [PVATTR_PV_NAME] )
		{
			if ( entry < pv_attrs .size() )
				return pv_attrs [entry] ;
			else
				break ;
		}
	}

	return "" ;
}

}//GParted
