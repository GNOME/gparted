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

#include "../include/BlockSpecial.h"

#include <glibmm/ustring.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace GParted
{

BlockSpecial::BlockSpecial() : m_name( "" ), m_major( 0UL ), m_minor( 0UL )
{
}

BlockSpecial::BlockSpecial( const Glib::ustring & name ) : m_name( name ), m_major( 0UL ), m_minor( 0UL )
{
	struct stat sb;
	if ( stat( name.c_str(), &sb ) == 0 && S_ISBLK( sb.st_mode ) )
	{
		m_major = major( sb.st_rdev );
		m_minor = minor( sb.st_rdev );
	}
}

BlockSpecial::~BlockSpecial()
{
}

bool operator==( const BlockSpecial & lhs, const BlockSpecial & rhs )
{
	if ( lhs.m_major > 0UL && lhs.m_minor > 0UL )
		// Match block special files by major, minor device numbers.
		return lhs.m_major == rhs.m_major && lhs.m_minor == rhs.m_minor;
	else
		// For non-block special files fall back to name string compare.
		return lhs.m_name == rhs.m_name;
}

} //GParted
