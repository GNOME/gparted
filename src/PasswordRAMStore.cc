/* Copyright (C) 2017 Mike Fleetwood
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

#include "PasswordRAMStore.h"

#include <stddef.h>
#include <sys/mman.h>
#include <string.h>
#include <glibmm/ustring.h>
#include <iostream>
#include <vector>


namespace GParted
{


namespace  // unnamed
{


struct PWEntry
{
	Glib::ustring key;       // Unique key identifying this password
	char *        password;  // Pointer to the password in protected_mem
	size_t        len;       // Number of bytes in the password
};

class PWStore
{
public:
	typedef std::vector<PWEntry>::iterator iterator;

	PWStore();
	~PWStore();

	bool insert( const Glib::ustring & key, const char * password );
	bool erase( const Glib::ustring & key );
	const char * lookup( const Glib::ustring & key );
	void erase_all();
	const char * get_protected_mem();

private:
	iterator find_key( const Glib::ustring & key );

	std::vector<PWEntry> pw_entries;     // Linear vector of password entries
	char *               protected_mem;  // Block of virtual memory locked into RAM
};

// Example PWStore data model.  After this sequence of calls:
//     mystore = PWStore();
//     mystore.insert( "UUID1", "password1", 9 );
//     mystore.insert( "UUID2", "password2", 9 );
//     mystore.insert( "UUID3", "password3", 9 );
//     mystore.erase( "UUID2" );
// The data would be:
//                   {key    , password, len}
//     pw_entries = [{"UUID1",     PTR1, 9  },
//                   {"UUID3",     PTR3, 9  }
//                  ]
//                     PTR1                           PTR3
//                      v                              v
//     protected_mem = "password1\0\0\0\0\0\0\0\0\0\0\0password3\0..."
//
// Description of processing:
// Pw_entries (password entries) and the bytes of the passwords themselves (protected_mem)
// are always stored in the same order.  A new password is always added at the end of
// pw_entries and after the last password in protected_mem.  Lookup of a password is a
// linear search for the key in pw_entries.  Erasing an entry just zeros the bytes of the
// password in protected_mem and erases the entry in pw_entries vector.  No compaction of
// unused bytes in protected_mem is performed.  Reuse of protected_mem only occurs when
// the password entry at the end of the pw_entries vector is erased and subsequently a new
// password inserted.

// The 4096 bytes of protected memory is enough to store 195, 20 byte passwords.
const size_t ProtectedMemSize = 4096;

PWStore::PWStore()
{
	// MAP_ANONYMOUS also ensures RAM is zero initialised.
	protected_mem = (char*) mmap(nullptr, ProtectedMemSize, PROT_READ|PROT_WRITE,
	                             MAP_PRIVATE|MAP_ANONYMOUS|MAP_LOCKED, -1, 0);
	if ( protected_mem == MAP_FAILED )
	{
		protected_mem = nullptr;
		std::cerr << "No locked virtual memory for password RAM store" << std::endl;
	}
}

PWStore::~PWStore()
{
	erase_all();
	if (protected_mem != nullptr)
		munmap( protected_mem, ProtectedMemSize );
}

bool PWStore::insert( const Glib::ustring & key, const char * password )
{
	if (protected_mem == nullptr)
		// No locked memory for passwords
		return false;

	if ( find_key( key ) != pw_entries.end() )
		// Entry already exists
		return false;

	char * next_password = protected_mem;
	size_t available_len = ProtectedMemSize - 1;
	if ( pw_entries.size() )
	{
		const PWEntry & last_pw_entry = pw_entries.back();
		next_password = last_pw_entry.password + last_pw_entry.len + 1;
		available_len = next_password - protected_mem - 1;
	}
	size_t pw_len = strlen( password );
	if ( pw_len <= available_len )
	{
		PWEntry new_pw_entry = {key, next_password, pw_len};
		pw_entries.push_back( new_pw_entry );
		memcpy( next_password, password, pw_len );
		next_password[pw_len] = '\0';
		return true;
	}

	// Not enough space available
	std::cerr << "Password RAM store exhausted" << std::endl;
	return false;
}

bool PWStore::erase( const Glib::ustring & key )
{
	iterator pw_entry_iter = find_key( key );
	if ( pw_entry_iter != pw_entries.end() )
	{
		memset( pw_entry_iter->password, '\0', pw_entry_iter->len );
		pw_entries.erase( pw_entry_iter );
		return true;
	}

	// No such key
	return false;
}

const char * PWStore::lookup( const Glib::ustring & key )
{
	iterator pw_entry_iter = find_key( key );
	if ( pw_entry_iter != pw_entries.end() )
	{
		return pw_entry_iter->password;
	}

	// No such key
	return nullptr;
}

PWStore::iterator PWStore::find_key( const Glib::ustring & key )
{
	for ( iterator pw_entry_iter = pw_entries.begin() ; pw_entry_iter != pw_entries.end(); ++pw_entry_iter )
	{
		if ( pw_entry_iter->key == key )
			return pw_entry_iter;
	}

	return pw_entries.end();
}

void PWStore::erase_all()
{
	pw_entries.clear();
	if (protected_mem != nullptr)
		// WARNING:
		// memset() can be optimised away if the compiler knows the memory is not
		// accessed again.  In this case this memset() is in a separate method
		// which usually bounds such optimisations.  Also in the parent method the
		// the pointer to the zeroed memory is passed to munmap() afterwards which
		// the compiler has to assume the memory is accessed so it can't optimise
		// the memset() away.
		// Reference:
		// * SEI CERT C Coding Standard, MSC06-C. Beware of compiler optimizations
		//   https://www.securecoding.cert.org/confluence/display/c/MSC06-C.+Beware+of+compiler+optimizations
		//
		// NOTE:
		// For secure overwriting of memory C11 has memset_s(), Linux kernel has
		// memzero_explicit(), FreeBSD/OpenBSD have explicit_bzero() and Windows
		// has SecureZeroMemory().
		memset( protected_mem, '\0', ProtectedMemSize );
}

const char * PWStore::get_protected_mem()
{
	return protected_mem;
}

// The single password RAM store
static PWStore single_pwstore;


}  // unnamed namespace


// PasswordRAMStore public methods

bool PasswordRAMStore::store( const Glib::ustring & key, const char * password )
{
	const char * looked_up_pw = single_pwstore.lookup( key );
	if (looked_up_pw == nullptr)
		return single_pwstore.insert( key, password );

	if ( strcmp( looked_up_pw, password ) == 0 )
		return true;

	return    single_pwstore.erase( key )
	       && single_pwstore.insert( key, password );
}

bool PasswordRAMStore::erase( const Glib::ustring & key )
{
	return single_pwstore.erase( key );
}

const char * PasswordRAMStore::lookup( const Glib::ustring & key )
{
	return single_pwstore.lookup( key );
}


// PasswordRAMStore private methods

void PasswordRAMStore::erase_all()
{
	single_pwstore.erase_all();
}

const char * PasswordRAMStore::get_protected_mem()
{
	return single_pwstore.get_protected_mem();
}


}  // namespace GParted
