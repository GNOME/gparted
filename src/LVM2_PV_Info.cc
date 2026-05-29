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

#include "LVM2_PV_Info.h"
#include "BlockSpecial.h"
#include "VGDevice.h"

#include <glibmm/miscutils.h>


namespace GParted
{


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
//  lvm2_pv_cache       - Vector of PV fields: pv_name, pv_size, pv_free, vg_name.
//                        E.g.
//                        //pv_name                   , pv_size  , pv_free  , vg_name
//                        [{BlockSpecial("/dev/sdb12"), 268435456, 268435456, ""        },  // Empty PV
//                         {BlockSpecial("/dev/sdb13"), 264241152, 264241152, "Test-VG1"},
//                         {BlockSpecial("/dev/sdb14"), 264241152,  62914560, "Test_VG2"},
//                         {BlockSpecial("/dev/sdb15"), 264241152,         0, "Test_VG3"},
//                         {BlockSpecial("/dev/sdb16"), 264241152, 125829120, "Test_VG3"},
//                         {BlockSpecial("/dev/sdb17"), 264241152, 130023424, "Test-VG4"},
//                         {BlockSpecial("/dev/sdb18"), 264241152,  46137344, "Test_VG5"}
//                        ]
//  lvm2_vg_cache       - Vector storing VG fields: vg_name, vg_attr, lv_name, lv_attr.
//                        See vgs(8) and lvs(8) for details of vg_attr and lv_attr respectively.
//                        E.g.
//                        //vg_name   , vg_attr , lv_name           , lv_attr
//                        [{""        , "r-----", ""                , ""          },  // (from empty PV)
//                         {"Test-VG1", "wz--n-", ""                , ""          },  // Empty VG
//                         {"Test_VG2", "wz--n-", "lvol0"           , "-wi-------"},  // Inactive VG
//                         {"Test_VG2", "wz--n-", "lvol1"           , "-wi-------"},
//                         {"Test_VG2", "wz--n-", ""                , ""          },
//                         {"Test_VG3", "wz--n-", "lvol0"           , "-wi-a-----"},  // Active VG
//                         {"Test_VG3", "wz--n-", "lvol0"           , "-wi-a-----"},
//                         {"Test_VG3", "wz--n-", ""                , ""          },
//                         {"Test-VG4", "wzx-n-", "lvol0"           , "-wi-------"},  // Exported VG
//                         {"Test-VG4", "wzx-n-", ""                , ""          },
//                         {"Test_VG5", "wz--n-", "[lvol0_pmspare]" , "ewi-------"},  // VG with thin-pool LV
//                         {"Test_VG5", "wz--n-", "[testpool_tdata]", "Twi-ao----"},
//                         {"Test_VG5", "wz--n-", "[testpool_tmeta]", "ewi-ao----"},
//                         {"Test_VG5", "wz--n-", ""                , ""          }
//                        ]
//  lvm2_vgdev_cache    - Vector storing VG fields: vg_name, vg_extent_size, vg_extent_count,
//                        vg_free_count, vg_uuid.
//                        E.g.
//                        //vg_name   , pe_size, total_pe, free_pe, uuid
//                         {"Test-VG1", 4194304,       63,      63, "iVJWzy-3qPk-I0X5-g9cR-OtFY-XUla-4z8dOl"},
//                         {"Test-VG4", 4194304,       63,      31, "x5d38B-qnin-HIfO-Xe13-l7UI-pTXK-lmAVxH"},
//                         {"Test_VG2", 4194304,       63,      15, "CUI33R-INVe-lO75-Khbq-jvLz-dhwR-TFkzDR"},
//                         {"Test_VG3", 4194304,      126,      30, "mymd59-wUgj-NHbs-FgpN-zjLs-Z6lA-dAl2QW"},
//                         {"Test_VG5", 4194304,       63,      11, "Ln1HZ2-alnX-Z1Jb-4rGr-xwm8-rpEh-vEf1YT"}
//                        ]
//  lvm2_lv_cache       - Vector storing LV fields
//                        E.g.
//                        //vg_name   , lv_path                 , lv_size   , active, segtype
//                         {"Test_VG2", "/dev/Test_VG2/lvol0"   ,  134217728, false , '-'    },
//                         {"Test_VG2", "/dev/Test_VG2/lvol1"   ,   67108864, false , '-'    },
//                         {"Test_VG3", "/dev/Test_VG3/lvol0"   ,  402653184, true  , '-'    },
//                         {"Test_VG5", "/dev/Test_VG5/thinvol0", 1073741824, true  , 'V'    }
//                        ]
//  error_messages      - String vector storing whole cache error messages.


//Initialize static data elements
bool LVM2_PV_Info::lvm2_pv_info_cache_initialized = false ;
bool LVM2_PV_Info::lvm_found = false ;
std::vector<LVM2_PV> LVM2_PV_Info::lvm2_pv_cache;
std::vector<LVM2_VG> LVM2_PV_Info::lvm2_vg_cache;
std::vector<LVM2_VGDev> LVM2_PV_Info::lvm2_vgdev_cache;
std::vector<LVM2_LV> LVM2_PV_Info::lvm2_lv_cache;
std::vector<Glib::ustring> LVM2_PV_Info::error_messages ;

bool LVM2_PV_Info::is_lvm2_pv_supported()
{
	set_command_found() ;
	return ( lvm_found ) ;
}

void LVM2_PV_Info::clear_cache()
{
	lvm2_pv_cache.clear();
	lvm2_vg_cache.clear();
	lvm2_vgdev_cache.clear();
	lvm2_lv_cache.clear();
	lvm2_pv_info_cache_initialized = false;
}


const Glib::ustring& LVM2_PV_Info::get_vg_name(const Glib::ustring& path)
{
	initialize_if_required() ;
	const LVM2_PV& pv = get_pv_cache_entry_by_name(path);
	return pv.vg_name;
}

//Return PV size in bytes, or -1 for error.
Byte_Value LVM2_PV_Info::get_size_bytes( const Glib::ustring & path )
{
	initialize_if_required() ;
	const LVM2_PV& pv = get_pv_cache_entry_by_name(path);
	return pv.pv_size;
}

//Return number of free bytes in the PV, or -1 for error.
Byte_Value LVM2_PV_Info::get_free_bytes( const Glib::ustring & path )
{
	initialize_if_required() ;
	const LVM2_PV& pv = get_pv_cache_entry_by_name(path);
	return pv.pv_free;
}

//Report if any LVs are active in the VG stored in the PV.
bool LVM2_PV_Info::has_active_lvs( const Glib::ustring & path )
{
	initialize_if_required() ;
	const LVM2_PV& pv = get_pv_cache_entry_by_name(path);
	if ( pv.vg_name == "" )
		// PV not yet included in any VG or PV not found in cache
		return false ;

	for ( unsigned int i = 0 ; i < lvm2_vg_cache .size() ; i ++ )
	{
		if ( pv.vg_name == lvm2_vg_cache[i].vg_name )
		{
			if ( bit_set( lvm2_vg_cache[i].lv_attr, LVBIT_STATE ) )
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
	const LVM2_VG& vg = get_vg_cache_entry_by_name(vgname);
	return bit_set( vg.vg_attr, VGBIT_EXPORTED );
}

//Return vector of PVs which are members of the VG.  Passing "" returns all empty PVs.
std::vector<Glib::ustring> LVM2_PV_Info::get_vg_members( const Glib::ustring & vgname )
{
	initialize_if_required() ;
	std::vector<Glib::ustring> members ;

	for ( unsigned int i = 0 ; i < lvm2_pv_cache .size() ; i ++ )
	{
		if ( vgname == lvm2_pv_cache[i].vg_name )
		{
			members.push_back( lvm2_pv_cache[i].pv_name.m_name );
		}
	}

	return members ;
}

// Return vector of LVs in the VG.
std::vector<Glib::ustring> LVM2_PV_Info::get_vg_lvs( const Glib::ustring & vgname )
{
	initialize_if_required();
	std::vector<Glib::ustring> lvs;

	if ( vgname == "" )
		return lvs;

	for ( unsigned int i = 0 ; i < lvm2_vg_cache.size() ; i ++ )
	{
		if ( vgname == lvm2_vg_cache[i].vg_name && lvm2_vg_cache[i].lv_name != "" )
		{
			// Only append lv_name if not already in lvs
			if ( std::find( lvs.begin(), lvs.end(), lvm2_vg_cache[i].lv_name ) == lvs.end() )
				lvs.push_back( lvm2_vg_cache[i].lv_name );
		}
	}

	return lvs;
}


Byte_Value LVM2_PV_Info::get_lv_size_bytes(const Glib::ustring& lv_path)
{
	initialize_if_required();
	for (unsigned int i = 0; i < lvm2_lv_cache.size(); i++)
	{
		if (lvm2_lv_cache[i].lv_path == lv_path)
			return lvm2_lv_cache[i].lv_size;
	}
	return -1;
}


bool LVM2_PV_Info::is_lv_active(const Glib::ustring& lv_path)
{
	initialize_if_required();
	for (unsigned int i = 0; i < lvm2_lv_cache.size(); i++)
	{
		if (lvm2_lv_cache[i].lv_path == lv_path)
			return lvm2_lv_cache[i].active;
	}
	return false;
}


// Returns the lvm segment type code: 'C' = cache, 'M' = mirror,
// 'r' = raid, 's' = snapshot, 't' = thin, 'T' = thin pool, 'V' = thin volume,
// '\0' if unknown or LV not in cache.  See lvs(8) "lv_attr" first letter.
char LVM2_PV_Info::get_lv_segtype(const Glib::ustring& lv_path)
{
	initialize_if_required();
	for (unsigned int i = 0; i < lvm2_lv_cache.size(); i++)
	{
		if (lvm2_lv_cache[i].lv_path == lv_path)
			return lvm2_lv_cache[i].segtype;
	}
	return '\0';
}


std::vector<VGDevice *> LVM2_PV_Info::get_vg_devices()
{
	initialize_if_required();
	std::vector<VGDevice *> vg_devices;

	for (unsigned int i = 0; i < lvm2_vgdev_cache.size(); i++)
	{
		const LVM2_VGDev& vgd = lvm2_vgdev_cache[i];
		VGDevice* vg = new VGDevice();

		vg->vg_name  = vgd.vg_name;
		vg->pe_size  = vgd.pe_size;
		vg->total_pe = vgd.total_pe;
		vg->free_pe  = vgd.free_pe;
		vg->uuid     = vgd.uuid;
		if (vgd.total_pe >= 0 && vgd.free_pe >= 0)
			vg->allocated_pe = vgd.total_pe - vgd.free_pe;

		// Issue #316: use the bare VG name as the canonical name everywhere
		// (lvm vgdisplay has no path property and LVM documentation almost
		// universally refers to a VG by its bare name, not /dev/<vgname>).
		vg->set_path(vgd.vg_name);
		if (vgd.total_pe > 0 && vgd.pe_size > 0)
		{
			vg->length      = vgd.total_pe;
			vg->sector_size = vgd.pe_size;
		}

		for (unsigned int j = 0; j < lvm2_pv_cache.size(); j++)
		{
			if (lvm2_pv_cache[j].vg_name == vgd.vg_name)
				vg->pv_paths.push_back(lvm2_pv_cache[j].pv_name.m_name);
		}

		for (unsigned int j = 0; j < lvm2_lv_cache.size(); j++)
		{
			if (lvm2_lv_cache[j].vg_name == vgd.vg_name)
				vg->lv_paths.push_back(lvm2_lv_cache[j].lv_path);
		}

		const LVM2_VG& vg_cache_entry = get_vg_cache_entry_by_name(vgd.vg_name);
		if (vg_cache_entry.vg_name == vgd.vg_name)
		{
			vg->exported = bit_set(vg_cache_entry.vg_attr, VGBIT_EXPORTED);
			vg->partial  = bit_set(vg_cache_entry.vg_attr, VGBIT_PARTIAL);
		}

		vg_devices.push_back(vg);
	}

	return vg_devices;
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
	const LVM2_PV& pv = get_pv_cache_entry_by_name(path);
	const LVM2_VG& vg = get_vg_cache_entry_by_name(pv.vg_name);
	if ( bit_set( vg.vg_attr, VGBIT_PARTIAL ) )
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
	lvm2_vgdev_cache.clear();
	lvm2_lv_cache.clear();
	error_messages .clear() ;
	if ( lvm_found )
	{
		//The OS is expected to fully enable LVM, this scan does
		//  not do the full job.  It is included incase anything
		//  is changed not using lvm commands.
		Utils::execute_command( "lvm vgscan", output, error, true ) ;

		// Load LVM2 PV cache.  Output PV values in PV_FIELD order.
		Glib::ustring cmd = "lvm pvs --config \"log{command_names=0}\" --nosuffix "
		                    "--noheadings --separator , --units b -o pv_name,pv_size,pv_free,vg_name" ;
		enum PV_FIELD
		{
			PVFIELD_PV_NAME = 0,
			PVFIELD_PV_SIZE = 1,
			PVFIELD_PV_FREE = 2,
			PVFIELD_VG_NAME = 3,
			PVFIELD_COUNT   = 4
		};
		if ( ! Utils::execute_command( cmd, output, error, true ) )
		{
			if ( output != "" )
			{
				std::vector<Glib::ustring> lines;
				Utils::tokenize( output, lines, "\n" );
				for ( i = 0 ; i < lines.size() ; i ++ )
				{
					std::vector<Glib::ustring> fields;
					Utils::split( Utils::trim( lines[i] ), fields, "," );
					if ( fields.size() < PVFIELD_COUNT )
						continue;  // Not enough fields
					if ( fields[PVFIELD_PV_NAME] == "" )
						continue;  // Empty PV name
					LVM2_PV pv;
					pv.pv_name = BlockSpecial( fields[PVFIELD_PV_NAME] );
					pv.pv_size = lvm2_pv_size_to_num( fields[PVFIELD_PV_SIZE] );
					pv.pv_free = lvm2_pv_size_to_num( fields[PVFIELD_PV_FREE] );
					pv.vg_name = fields[PVFIELD_VG_NAME];
					lvm2_pv_cache.push_back( pv );
				}
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

		// Load LVM2 VG cache.  Output VG and LV values in VG_FIELD order.
		cmd = "lvm pvs --config \"log{command_names=0}\" --nosuffix "
		      "--noheadings --separator , --units b -o vg_name,vg_attr,lv_name,lv_attr" ;
		enum VG_FIELD
		{
			VGFIELD_VG_NAME = 0,
			VGFIELD_VG_ATTR = 1,
			VGFIELD_LV_NAME = 2,
			VGFIELD_LV_ATTR = 3,
			VGFIELD_COUNT   = 4
		};
		if ( ! Utils::execute_command( cmd, output, error, true ) )
		{
			if ( output != "" )
			{
				std::vector<Glib::ustring> lines;
				Utils::tokenize( output, lines, "\n" );
				for ( i = 0 ; i < lines.size() ; i ++ )
				{
					std::vector<Glib::ustring> fields;
					Utils::split( Utils::trim( lines[i] ), fields, "," );
					if ( fields.size() < VGFIELD_COUNT )
						continue;  // Not enough fields
					LVM2_VG vg;
					// VG name may be empty.  See data model example above.
					vg.vg_name = fields[VGFIELD_VG_NAME];
					vg.vg_attr = fields[VGFIELD_VG_ATTR];
					vg.lv_name = fields[VGFIELD_LV_NAME];
					vg.lv_attr = fields[VGFIELD_LV_ATTR];
					lvm2_vg_cache.push_back( vg );
				}
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

		cmd = "lvm vgs --config \"log{command_names=0}\" --nosuffix "
		      "--noheadings --separator , --units b "
		      "-o vg_name,vg_extent_size,vg_extent_count,vg_free_count,vg_uuid";
		enum VGDEV_FIELD
		{
			VGDEVFIELD_VG_NAME         = 0,
			VGDEVFIELD_VG_EXTENT_SIZE  = 1,
			VGDEVFIELD_VG_EXTENT_COUNT = 2,
			VGDEVFIELD_VG_FREE_COUNT   = 3,
			VGDEVFIELD_VG_UUID         = 4,
			VGDEVFIELD_COUNT           = 5
		};
		if (! Utils::execute_command(cmd, output, error, true))
		{
			if (output != "")
			{
				std::vector<Glib::ustring> lines;
				Utils::tokenize(output, lines, "\n");
				for (i = 0; i < lines.size(); i++)
				{
					std::vector<Glib::ustring> fields;
					Utils::split(Utils::trim(lines[i]), fields, ",");
					if (fields.size() < VGDEVFIELD_COUNT)
						continue;  // Not enough fields
					if (fields[VGDEVFIELD_VG_NAME] == "")
						continue;
					LVM2_VGDev vgd;
					vgd.vg_name  = fields[VGDEVFIELD_VG_NAME];
					vgd.pe_size  = lvm2_pv_size_to_num(fields[VGDEVFIELD_VG_EXTENT_SIZE]);
					vgd.total_pe = lvm2_pv_size_to_num(fields[VGDEVFIELD_VG_EXTENT_COUNT]);
					vgd.free_pe  = lvm2_pv_size_to_num(fields[VGDEVFIELD_VG_FREE_COUNT]);
					vgd.uuid     = fields[VGDEVFIELD_VG_UUID];
					lvm2_vgdev_cache.push_back(vgd);
				}
			}
		}
		else
		{
			error_messages.push_back(cmd);
			if (! output.empty())
				error_messages.push_back(output);
			if (! error.empty())
				error_messages.push_back(error);
		}

		cmd = "lvm lvs --config \"log{command_names=0}\" --nosuffix "
		      "--noheadings --separator , --units b "
		      "-o vg_name,lv_path,lv_size,lv_attr";
		enum LV_FIELD
		{
			LVFIELD_VG_NAME = 0,
			LVFIELD_LV_PATH = 1,
			LVFIELD_LV_SIZE = 2,
			LVFIELD_LV_ATTR = 3,
			LVFIELD_COUNT   = 4
		};
		if (! Utils::execute_command(cmd, output, error, true))
		{
			if (output != "")
			{
				std::vector<Glib::ustring> lines;
				Utils::tokenize(output, lines, "\n");
				for (i = 0; i < lines.size(); i++)
				{
					std::vector<Glib::ustring> fields;
					Utils::split(Utils::trim(lines[i] ), fields, ",");
					if (fields.size() < LVFIELD_COUNT)
						continue;  // Not enough fields
					if (fields[LVFIELD_VG_NAME] == "" || fields[LVFIELD_LV_PATH] == "")
						continue;
					LVM2_LV lv;
					lv.vg_name = fields[LVFIELD_VG_NAME];
					lv.lv_path = fields[LVFIELD_LV_PATH];
					lv.lv_size = lvm2_pv_size_to_num(fields[LVFIELD_LV_SIZE]);
					lv.active  = (fields[LVFIELD_LV_ATTR].size() >= 5   &&
					              fields[LVFIELD_LV_ATTR][4]     == 'a'   );
					lv.segtype = (fields[LVFIELD_LV_ATTR].size() >= 1)
					             ? static_cast<char>(fields[LVFIELD_LV_ATTR][0])
					             : '\0';
					lvm2_lv_cache.push_back(lv);
				}
			}
		}
		else
		{
			error_messages.push_back(cmd);
			if (! output.empty())
				error_messages.push_back(output);
			if (! error.empty())
				error_messages.push_back(error);
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

// Performs linear search of the PV cache to find the first matching pv_name.
// Returns found cache entry or not found substitute.
const LVM2_PV & LVM2_PV_Info::get_pv_cache_entry_by_name( const Glib::ustring & pvname )
{
	BlockSpecial bs_pvname( pvname );
	for ( unsigned int i = 0 ; i < lvm2_pv_cache.size() ; i ++ )
	{
		if ( bs_pvname == lvm2_pv_cache[i].pv_name )
			return lvm2_pv_cache[i];
	}
	static LVM2_PV not_found;  // {BlockSpecial(), -1, -1, ""}
	return not_found;
}


// Performs linear search of the VG cache to find the first matching vg_name.
// Returns found cache entry or not found substitute.
const LVM2_VG & LVM2_PV_Info::get_vg_cache_entry_by_name( const Glib::ustring & vgname )
{
	for ( unsigned int i = 0 ; i < lvm2_vg_cache.size() ; i ++ )
	{
		if ( vgname == lvm2_vg_cache[i].vg_name )
			return lvm2_vg_cache[i];
	}
	static LVM2_VG not_found;  // {"", "", "", ""}
	return not_found;
}


// Return string converted to a number, or -1 for error.
// Used to convert PVs size or free bytes.
Byte_Value LVM2_PV_Info::lvm2_pv_size_to_num(const Glib::ustring& str)
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


}  // namespace GParted
