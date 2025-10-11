/* Copyright (C) 2019 Mike Fleetwood
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

#ifndef GPARTED_SUPPORTEDFILESYSTEMS_H
#define GPARTED_SUPPORTEDFILESYSTEMS_H


#include "FileSystem.h"

#include <map>
#include <vector>
#include <memory>


namespace GParted
{


class SupportedFileSystems
{
public:
	SupportedFileSystems();

	void find_supported_filesystems();
	FileSystem* get_fs_object(FSType fstype) const;
	const FS& get_fs_support(FSType fstype) const;
	const std::vector<FS>& get_all_fs_support() const;
	bool supported_filesystem(FSType fstype) const;

private:
	typedef std::map<FSType, std::unique_ptr<FileSystem>> FSObjectsMap;

	std::vector<FS> m_fs_support;
	FSObjectsMap    m_fs_objects;

};


}  // namespace GParted


#endif /* GPARTED_SUPPORTEDFILESYSTEMS_H */
