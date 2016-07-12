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


/* BlockSpecial
 *
 * Representation of a POSIX block special file, e.g. /dev/sda1.  Also
 * tracks the major, minor device numbers so that different names for
 * the same block device can be compared equal.
 * Refs: mknod(1) and mknod(2).
 */

#ifndef GPARTED_BLOCKSPECIAL_H
#define GPARTED_BLOCKSPECIAL_H

#include <glibmm/ustring.h>

namespace GParted
{

class BlockSpecial
{
public:
	BlockSpecial();
	BlockSpecial( const Glib::ustring & name );
	~BlockSpecial();

	Glib::ustring m_name;   // E.g. Block special file {"/dev/sda1", 8, 1},
	unsigned long m_major;  // plain file {"FILENAME", 0, 0} and empty object
	unsigned long m_minor;  // {"", 0, 0}.

	static void clear_cache();
	static void register_block_special( const Glib::ustring & name,
	                                    unsigned long major, unsigned long minor );
};

// Operator overloading > The Decision between Member and Non-member
// http://stackoverflow.com/questions/4421706/operator-overloading/4421729#4421729
//     "2. If a binary operator treats both operands equally (it leaves them unchanged),
//     implement this operator as a non-member function."
bool operator==( const BlockSpecial & lhs, const BlockSpecial & rhs );
bool operator<( const BlockSpecial & lhs, const BlockSpecial & rhs );

}//GParted

#endif /* GPARTED_BLOCKSPECIAL_H */
