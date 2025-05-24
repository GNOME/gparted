/* Copyright (C) 2018 Mike Fleetwood
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


/* PasswordRAMStore
 *
 * Memory only store of passwords and passphrases.  Works like an associative array with
 * a unique key used to identify each password.  Passwords are C strings which are stored
 * in a block of the process' virtual memory locked into RAM.  Looked up pointers to
 * passwords are valid at least until the next time the store is modified by an insert or
 * erase call.  Passwords are wiped from memory when no longer wanted.
 *
 * Recommend using LUKS UUIDs as the key when storing LUKS passphrases.
 */

#ifndef GPARTED_PASSWORDRAMSTORE_H
#define GPARTED_PASSWORDRAMSTORE_H

#include <glibmm/ustring.h>


namespace GParted
{


class PasswordRAMStore
{
friend class PasswordRAMStoreTest;  // To allow unit testing PasswordRAMStoreTest class
                                    // access to private methods.

public:
	static bool store( const Glib::ustring & key, const char * password );
	static bool erase( const Glib::ustring & key );
	static const char * lookup( const Glib::ustring & key );

private:
	static void erase_all();
	static const char * get_protected_mem();
};


}  // namespace GParted


#endif /* GPARTED_PASSWORDRAMSTORE_H */
