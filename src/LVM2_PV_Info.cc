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

namespace GParted
{

enum PV_ATTRIBUTE
{
	PVATTR_PV_NAME = 0,
	PVATTR_PV_SIZE = 1,
	PVATTR_PV_FREE = 2,
	PVATTR_VG_NAME = 3
} ;

enum VG_ATTRIBUTE
{
	VGATTR_VG_NAME = 0,
	VGATTR_VG_BITS = 1,
	VGATTR_LV_NAME = 2,
	VGATTR_LV_BITS = 3
} ;

enum VG_BIT
{
	VGBIT_EXPORTED = 2,	//  "x" VG exported,                    "-" VG not exported
	VGBIT_PARTIAL  = 3	//  "p" VG has one or more PVs missing, "-" all PVs present
} ;

enum LV_BIT
{
	LVBIT_STATE = 4		//  "a" LV active, "-" LV not active
				//  (Has many more values beside just "a" and "-".  See lvs(8) for details).
} ;

//Data model:
//  lvm2_pv_info_cache_initialized
//                      - Has the cache been loaded yet?
//  lvm_found           - Is the "lvm" command available?
//  lvm2_pv_cache       - String vector storing PV attributes: pv_name, pv_size, pv_free, vg_name.
//                        One string per PV.
//                        E.g.
//                        ["/dev/sda10,1073741824,1073741824,",
//                         "/dev/sda11,1069547520,1069547520,Test-VG1",
//                         "/dev/sda12,1069547520,335544320,Test_VG2",
//                         "/dev/sda13,1069547520,0,Test_VG3",
//                         "/dev/sda14,1069547520,566231040,Test_VG3",
//                         "/dev/sda15,1069547520,545259520,Test-VG4"
//                        ]
//  lvm2_vg_cache       - String vector storing VG attributes: vg_name, vg_attr, lv_name, lv_attr.
//                        See vgs(8) and lvs(8) for details of vg_attr and lv_attr respectively.
//                        [",r-----,,",
//                         "Test-VG1,wz--n-,,",
//                         "Test_VG2,wz--n-,lvol0,-wi---",
//                         "Test_VG2,wz--n-,lvol1,-wi---",
//                         "Test_VG2,wz--n-,,",
//                         "Test_VG3,wz--n-,lvol0,-wi-a-",
//                         "Test_VG3,wz--n-,lvol0,-wi-a-",
//                         "Test_VG3,wz--n-,,",
//                         "Test-VG4,wzx-n-,lvol0,-wi---",
//                         "Test-VG4,wzx-n-,,"
//                        ]
//  error_messages      - String vector storing whole cache error messages.


//Initialize static data elements
bool LVM2_PV_Info::lvm2_pv_info_cache_initialized = false ;
bool LVM2_PV_Info::lvm_found = false ;
std::vector<Glib::ustring> LVM2_PV_Info::lvm2_pv_cache ;
std::vector<Glib::ustring> LVM2_PV_Info::lvm2_vg_cache ;
std::vector<Glib::ustring> LVM2_PV_Info::error_messages ;

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
	set_command_found() ;
	return ( lvm_found ) ;
}

Glib::ustring LVM2_PV_Info::get_vg_name( const Glib::ustring & path )
{
	initialize_if_required() ;
	return get_pv_attr_by_path( path, PVATTR_VG_NAME ) ;
}

//Return PV size in bytes, or -1 for error.
Byte_Value LVM2_PV_Info::get_size_bytes( const Glib::ustring & path )
{
	initialize_if_required() ;
	Glib::ustring str = get_pv_attr_by_path( path, PVATTR_PV_SIZE ) ;
	return lvm2_pv_attr_to_num( str ) ;
}

//Return number of free bytes in the PV, or -1 for error.
Byte_Value LVM2_PV_Info::get_free_bytes( const Glib::ustring & path )
{
	initialize_if_required() ;
	Glib::ustring str = get_pv_attr_by_path( path, PVATTR_PV_FREE ) ;
	return lvm2_pv_attr_to_num( str ) ;
}

//Report if any LVs are active in the VG stored in the PV.
bool LVM2_PV_Info::has_active_lvs( const Glib::ustring & path )
{
	initialize_if_required() ;
	Glib::ustring vgname = get_pv_attr_by_path( path, PVATTR_VG_NAME ) ;
	if ( vgname == "" )
		//PV not yet included in any VG
		return false ;

	for ( unsigned int i = 0 ; i < lvm2_vg_cache .size() ; i ++ )
	{
		if ( vgname == get_vg_attr_by_row( i, VGATTR_VG_NAME ) )
		{
			if ( bit_set( get_vg_attr_by_row( i, VGATTR_LV_BITS ), LVBIT_STATE ) )
				//LV in VG is active
				return true ;
		}
	}
	return false ;
}

//Report if the VG is exported.
bool LVM2_PV_Info::is_vg_exported( const Glib::ustring & vgname )
{
	initialize_if_required() ;

	return bit_set( get_vg_attr_by_name( vgname, VGATTR_VG_BITS ), VGBIT_EXPORTED ) ;
}

//Return vector of PVs which are members of the VG.  Passing "" returns all empty PVs.
std::vector<Glib::ustring> LVM2_PV_Info::get_vg_members( const Glib::ustring & vgname )
{
	initialize_if_required() ;
	std::vector<Glib::ustring> members ;

	for ( unsigned int i = 0 ; i < lvm2_pv_cache .size() ; i ++ )
	{
		if ( vgname == get_pv_attr_by_row( i, PVATTR_VG_NAME ) )
		{
			members .push_back( get_pv_attr_by_row( i, PVATTR_PV_NAME ) ) ;
		}
	}

	return members ;
}

std::vector<Glib::ustring> LVM2_PV_Info::get_error_messages( const Glib::ustring & path )
{
	initialize_if_required() ;
	if ( ! error_messages .empty() )
		//Return whole cache error messages as first choice
		return error_messages ;

	std::vector<Glib::ustring> partition_specific_messages ;
	Glib::ustring temp ;

	//Check for partition specific message: partial VG
	Glib::ustring vgname = get_pv_attr_by_path( path, PVATTR_VG_NAME ) ;
	if ( bit_set( get_vg_attr_by_name( vgname, VGATTR_VG_BITS ), VGBIT_PARTIAL ) )
	{
		temp = _("One or more Physical Volumes belonging to the Volume Group is missing.") ;
		partition_specific_messages .push_back ( temp ) ;
	}

	return partition_specific_messages ;
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
	unsigned int i ;

	lvm2_pv_cache .clear() ;
	lvm2_vg_cache .clear() ;
	error_messages .clear() ;
	if ( lvm_found )
	{
		//The OS is expected to fully enable LVM, this scan does
		//  not do the full job.  It is included incase anything
		//  is changed not using lvm commands.
		Utils::execute_command( "lvm vgscan", output, error, true ) ;

		//Load LVM2 PV attribute cache.  Output PV attributes in PV_ATTRIBUTE order
		Glib::ustring cmd = "lvm pvs --config \"log{command_names=0}\" --nosuffix "
		                    "--noheadings --separator , --units b -o pv_name,pv_size,pv_free,vg_name" ;
		if ( ! Utils::execute_command( cmd, output, error, true ) )
		{
			if ( output != "" )
			{
				Utils::tokenize( output, lvm2_pv_cache, "\n" ) ;
				for ( i = 0 ; i < lvm2_pv_cache .size() ; i ++ )
					lvm2_pv_cache [i] = Utils::trim( lvm2_pv_cache [i] ) ;
			}
		}
		else
		{
			error_messages .push_back( cmd ) ;
			if ( ! output .empty() )
				error_messages .push_back ( output ) ;
			if ( ! error .empty() )
				error_messages .push_back ( error ) ;
		}

		//Load LVM2 VG attribute cache.  Output VG and LV attributes in VG_ATTRIBUTE order
		cmd = "lvm pvs --config \"log{command_names=0}\" --nosuffix "
		      "--noheadings --separator , --units b -o vg_name,vg_attr,lv_name,lv_attr" ;
		if ( ! Utils::execute_command( cmd, output, error, true ) )
		{
			if ( output != "" )
			{
				Utils::tokenize( output, lvm2_vg_cache, "\n" ) ;
				for ( i = 0 ; i < lvm2_vg_cache .size() ; i ++ )
					lvm2_vg_cache [i] = Utils::trim( lvm2_vg_cache [i] ) ;
			}
		}
		else
		{
			error_messages .push_back( cmd ) ;
			if ( ! output .empty() )
				error_messages .push_back ( output ) ;
			if ( ! error .empty() )
				error_messages .push_back ( error ) ;
		}

		if ( ! error_messages .empty() )
		{
			Glib::ustring temp ;
			temp = _("An error occurred reading LVM2 configuration!") ;
			temp += "\n" ;
			temp += _("Some or all of the details might be missing or incorrect.") ;
			temp += "\n" ;
			temp += _("You should NOT modify any LVM2 PV partitions.") ;
			error_messages .push_back( temp ) ;
		}
	}
}

Glib::ustring LVM2_PV_Info::get_pv_attr_by_path( const Glib::ustring & path, unsigned int entry ) const
{
	return get_attr_by_name( lvm2_pv_cache, path, entry ) ;
}

Glib::ustring LVM2_PV_Info::get_pv_attr_by_row( unsigned int row, unsigned int entry ) const
{
	return get_attr_by_row( lvm2_pv_cache, row, entry ) ;
}

Glib::ustring LVM2_PV_Info::get_vg_attr_by_name( const Glib::ustring & vgname, unsigned int entry ) const
{
	return get_attr_by_name( lvm2_vg_cache, vgname, entry ) ;
}

Glib::ustring LVM2_PV_Info::get_vg_attr_by_row( unsigned int row, unsigned int entry ) const
{
	return get_attr_by_row( lvm2_vg_cache, row, entry ) ;
}

//Performs linear search of the cache and uses the first matching name.
//  Return nth attribute.  Attributes are numbered 0 upward using
//  PV_ATTRIBUTE and VG_ATTRIBUTE enumerations for lvm2_pv_cache and
//  lvm2_vg_cache respectively.
Glib::ustring LVM2_PV_Info::get_attr_by_name( const std::vector<Glib::ustring> cache,
                                              const Glib::ustring name, unsigned int entry )
{
	for ( unsigned int i = 0 ; i < cache .size() ; i ++ )
	{
		std::vector<Glib::ustring> attrs ;
		Utils::split( cache [i], attrs, "," ) ;
		if ( attrs .size() >= 1 && attrs [0] == name )
		{
			if ( entry < attrs .size() )
				return attrs [entry] ;
			else
				break ;
		}
	}

	return "" ;
}

//Lookup row from the cache and return nth attribute.  Row is numbered
//  0 upwards and attributes are numbered 0 upwards using PV_ATTRIBUTE
//  and VG_ATTRIBUTE enumerations for lvm2_pv_cache and lvm2_vg_cache
//  respectively.
Glib::ustring LVM2_PV_Info::get_attr_by_row( const std::vector<Glib::ustring> cache,
                                             unsigned int row, unsigned int entry )
{
	if ( row >= cache .size() )
		return "" ;
	std::vector<Glib::ustring> attrs ;
	Utils::split( cache [row], attrs, "," ) ;
	if ( entry < attrs .size() )
		return attrs [entry] ;
	return "" ;
}

//Return string converted to a number, or -1 for error.
//Used to convert PVs size or free bytes.
Byte_Value LVM2_PV_Info::lvm2_pv_attr_to_num( const Glib::ustring str )
{
	Byte_Value num = -1 ;
	if ( str != "" )
	{
		gchar * suffix ;
		num = (Byte_Value) g_ascii_strtoll( str .c_str(), & suffix, 10 ) ;
		if ( num < 0 || ( num == 0 && suffix == str ) )
			//Negative number or conversion failed
			num = -1 ;
	}
	return num ;
}

//Test if the bit is set in either VG or LV "bits" attribute string
//  E.g.  bit_set( "wz--n-", VGBIT_EXPORTED ) -> false
//        bit_set( "wzx-n-", VGBIT_EXPORTED ) -> true
bool LVM2_PV_Info::bit_set( const Glib::ustring & attr, unsigned int bit )
{
	//NOTE:
	//  This code treats hyphen '-' as unset and everything else as set.
	//  Some of the "bits", including LV state (LVBIT_STATE) encode many
	//  settings as different letters, but this assumption is still valid.
	//  See vgs(8) and lvs(8) for details.
	if ( ! attr .empty() && attr .length() > bit )
		return ( attr[ bit ] != '-' ) ;
	return false ;
}

}//GParted
