/* Copyright (C) 2016 Mike Fleetwood
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


#include "SupportedFileSystems.h"
#include "FileSystem.h"
#include "Utils.h"
#include "btrfs.h"
#include "exfat.h"
#include "ext2.h"
#include "f2fs.h"
#include "fat16.h"
#include "hfs.h"
#include "hfsplus.h"
#include "jfs.h"
#include "linux_swap.h"
#include "lvm2_pv.h"
#include "luks.h"
#include "minix.h"
#include "nilfs2.h"
#include "ntfs.h"
#include "reiser4.h"
#include "reiserfs.h"
#include "udf.h"
#include "xfs.h"

#include <stddef.h>


namespace GParted
{


SupportedFileSystems::SupportedFileSystems()
{
	// File system support falls into 3 categories determined by their entry in
	// m_fs_objects:
	// 1)  Fully supported file systems have an entry pointing to the instance of
	//     their derived FileSystem object, which determines and implements their
	//     supported actions.
	//     supported_filesystem() -> true
	// 2)  Basic supported file systems have a NULL pointer entry, with
	//     find_supported_filesystems() creating a basic set of supported actions.
	//     supported_filesystem() -> false
	// 3)  Unsupported file systems have no entry, and no supported actions.
	//     supported_filesystem() -> false
	m_fs_objects[FS_UNKNOWN]         = NULL;
	m_fs_objects[FS_OTHER]           = NULL;
	m_fs_objects[FS_BTRFS]           = new btrfs();
	m_fs_objects[FS_EXFAT]           = new exfat();
	m_fs_objects[FS_EXT2]            = new ext2(FS_EXT2);
	m_fs_objects[FS_EXT3]            = new ext2(FS_EXT3);
	m_fs_objects[FS_EXT4]            = new ext2(FS_EXT4);
	m_fs_objects[FS_F2FS]            = new f2fs();
	m_fs_objects[FS_FAT16]           = new fat16(FS_FAT16);
	m_fs_objects[FS_FAT32]           = new fat16(FS_FAT32);
	m_fs_objects[FS_HFS]             = new hfs();
	m_fs_objects[FS_HFSPLUS]         = new hfsplus();
	m_fs_objects[FS_JFS]             = new jfs();
	m_fs_objects[FS_LINUX_SWAP]      = new linux_swap();
	m_fs_objects[FS_LVM2_PV]         = new lvm2_pv();
	m_fs_objects[FS_LUKS]            = new luks();
	m_fs_objects[FS_MINIX]           = new minix();
	m_fs_objects[FS_NILFS2]          = new nilfs2();
	m_fs_objects[FS_NTFS]            = new ntfs();
	m_fs_objects[FS_REISER4]         = new reiser4();
	m_fs_objects[FS_REISERFS]        = new reiserfs();
	m_fs_objects[FS_UDF]             = new udf();
	m_fs_objects[FS_XFS]             = new xfs();
	m_fs_objects[FS_APFS]            = NULL;
	m_fs_objects[FS_ATARAID]         = NULL;
	m_fs_objects[FS_BITLOCKER]       = NULL;
	m_fs_objects[FS_GRUB2_CORE_IMG]  = NULL;
	m_fs_objects[FS_ISO9660]         = NULL;
	m_fs_objects[FS_LINUX_SWRAID]    = NULL;
	m_fs_objects[FS_LINUX_SWSUSPEND] = NULL;
	m_fs_objects[FS_REFS]            = NULL;
	m_fs_objects[FS_UFS]             = NULL;
	m_fs_objects[FS_ZFS]             = NULL;
}


SupportedFileSystems::~SupportedFileSystems()
{
	FSObjectsMap::iterator iter;
	for (iter = m_fs_objects.begin(); iter != m_fs_objects.end(); iter++)
	{
		delete iter->second;
		iter->second = NULL;
	}
}


void SupportedFileSystems::find_supported_filesystems()
{
	FSObjectsMap::iterator iter;

	// Iteration of std::map is ordered according to operator< of the key.  Hence the
	// m_fs_support vector is constructed in FSType enum order: FS_UNKNOWN, FS_BTRFS,
	// ..., FS_XFS, ... .  This ultimately controls the default order of the file
	// systems in the menus and dialogs.
	m_fs_support.clear();

	for (iter = m_fs_objects.begin(); iter != m_fs_objects.end(); iter++)
	{
		if (iter->second)
		{
			m_fs_support.push_back(iter->second->get_filesystem_support());
		}
		else
		{
			// For basic supported file systems create the supported action
			// set.
			FS fs_basicsupp(iter->first);
			fs_basicsupp.move = FS::GPARTED;
			fs_basicsupp.copy = FS::GPARTED;
			m_fs_support.push_back(fs_basicsupp);
		}
	}
}


FileSystem* SupportedFileSystems::get_fs_object(FSType fstype) const
{
	FSObjectsMap::const_iterator iter = m_fs_objects.find(fstype);
	if (iter == m_fs_objects.end())
		return NULL;
	else
		return iter->second;
}


// Return supported action set of the file system type or, if not found, not supported
// action set.
const FS& SupportedFileSystems::get_fs_support(FSType fstype) const
{
	for (unsigned int i = 0; i < m_fs_support.size(); i++)
	{
		if (m_fs_support[i].fstype == fstype)
			return m_fs_support[i];
	}

	static FS fs_notsupp(FS_UNSUPPORTED);
	return fs_notsupp;
}


const std::vector<FS>& SupportedFileSystems::get_all_fs_support() const
{
	return m_fs_support;
}


// Return true for file systems with an implementation class, false otherwise.
bool SupportedFileSystems::supported_filesystem(FSType fstype) const
{
	return get_fs_object(fstype) != NULL;
}


} //GParted
