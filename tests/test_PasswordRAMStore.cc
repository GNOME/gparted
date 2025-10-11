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

/* Test PasswordRAMStore
 *
 * NOTE:
 * As well as calling the public API of PasswordRAMStore this unit testing also accesses
 * the private members of PasswordRAMStore and uses knowledge of the implementation,
 * making this white box testing.  This is so that the hidden behaviour of zeroing
 * password storing memory before and after use can be tested.
 *
 * WARNING:
 * Each test fixture would normally initialise separate resources to make the tests
 * independent of each other.  However the password store is a single long lived shared
 * resource.  Therefore, so that each test fixture is independent of all the others, the
 * password store must be returned to it's original state of being empty before each
 * fixture completes.
 *
 * Reference:
 *     Google Test, Advanced Guide, Sharing Resources Between Tests in the Same Test Case
 *     https://github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md#sharing-resources-between-tests-in-the-same-test-case
 */

#include "PasswordRAMStore.h"
#include "gtest/gtest.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <glibmm/ustring.h>


namespace GParted
{


// Generate repeatable unique keys
static const char * gen_key( unsigned int i )
{
	static char buf[14];
	snprintf( buf, sizeof( buf ), "key%u", i );
	return buf;
}

// Generate repeatable "passwords" exactly 20 characters long
static const char * gen_passwd( unsigned int i )
{
	static char buf[21];
	snprintf(buf, sizeof(buf), "password%03u         ", i%1000);
	return buf;
}

static bool mem_is_zero( const char * mem, size_t len )
{
	while ( len-- > 0 )
	{
		if ( *mem++ != '\0' )
		{
			return false;
		}
	}
	return true;
}

// Explicit test fixture class for common setup and sharing of the underlying password
// store address.
class PasswordRAMStoreTest : public ::testing::Test
{
protected:
	PasswordRAMStoreTest() : looked_up_pw(nullptr)  {};

	static void SetUpTestCase();

	static void erase_all()  { PasswordRAMStore::erase_all(); };

	static const char *  protected_mem;

	std::string pw;
	const char * looked_up_pw;
	size_t looked_up_len;
};

// Initialise test case class static member.
const char * PasswordRAMStoreTest::protected_mem = nullptr;

const size_t ProtectedMemSize = 4096;  // [Implementation knowledge: size]

// Common test case initialisation recording the underlying password store address.
void PasswordRAMStoreTest::SetUpTestCase()
{
	protected_mem = PasswordRAMStore::get_protected_mem();
	ASSERT_TRUE(protected_mem != nullptr) << __func__ << "(): No locked virtual memory for password RAM store";
}

TEST_F( PasswordRAMStoreTest, Initialisation )
{
	// Test locked memory is initialised with all zeros.
	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, UnknownPasswordLookup )
{
	// Test lookup of non-existent password fails.
	looked_up_pw = PasswordRAMStore::lookup( "key-unknown" );
	EXPECT_TRUE(looked_up_pw == nullptr);
}

TEST_F( PasswordRAMStoreTest, UnknownPasswordErasure )
{
	// Test erase non-existent password fails.
	EXPECT_FALSE( PasswordRAMStore::erase( "key-unknown" ) );
}

TEST_F( PasswordRAMStoreTest, SinglePassword )
{
	// Test a single password can be stored, looked up and erased (and zeroed).
	pw = "password";
	EXPECT_TRUE( PasswordRAMStore::store( "key-single", pw.c_str() ) );

	looked_up_pw = PasswordRAMStore::lookup( "key-single" );
	EXPECT_STREQ( pw.c_str(), looked_up_pw );

	looked_up_len = strlen( looked_up_pw );
	EXPECT_TRUE( PasswordRAMStore::erase( "key-single" ) );
	EXPECT_TRUE( mem_is_zero( looked_up_pw, looked_up_len ) );

	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, IdenticalPassword )
{
	// Test a password can be saved twice with the same key and looked up (and the
	// single password can be erased and zeroed).
	pw = "password";
	EXPECT_TRUE( PasswordRAMStore::store( "key-single", pw.c_str() ) );

	EXPECT_TRUE( PasswordRAMStore::store( "key-single", pw.c_str() ) );

	looked_up_pw = PasswordRAMStore::lookup( "key-single" );
	EXPECT_STREQ( pw.c_str(), looked_up_pw );

	looked_up_len = strlen( looked_up_pw );
	EXPECT_TRUE( PasswordRAMStore::erase( "key-single" ) );
	EXPECT_TRUE( mem_is_zero( looked_up_pw, looked_up_len ) );

	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, ReplacePassword )
{
	// Test a password can be saved and looked up, then saved again with a different
	// password using the same key and looked up (and the single replaced password
	// is erased and zeroed).
	pw = "password";
	EXPECT_TRUE( PasswordRAMStore::store( "key-single", pw.c_str() ) );

	looked_up_pw = PasswordRAMStore::lookup( "key-single" );
	EXPECT_STREQ( pw.c_str(), looked_up_pw );

	pw = "password2";
	EXPECT_TRUE( PasswordRAMStore::store( "key-single", pw.c_str() ) );

	looked_up_pw = PasswordRAMStore::lookup( "key-single" );
	EXPECT_STREQ( pw.c_str(), looked_up_pw );

	looked_up_len = strlen( looked_up_pw );
	EXPECT_TRUE( PasswordRAMStore::erase( "key-single" ) );
	EXPECT_TRUE( mem_is_zero( looked_up_pw, looked_up_len ) );

	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, OneHundredPasswordsForwards )
{
	// Test 100, 20 character passwords can be stored, looked up and erased (and
	// zeroed).  Passwords are erased forwards (first stored to last stored).
	unsigned int i;
	for ( i = 0 ; i < 100 ; i ++ )
	{
		pw = gen_passwd( i );
		EXPECT_TRUE( PasswordRAMStore::store( gen_key(i), pw.c_str() ) );
	}

	for ( i = 0 ; i < 100 ; i ++ )
	{
		pw = gen_passwd( i );
		looked_up_pw = PasswordRAMStore::lookup( gen_key(i) );
		EXPECT_STREQ( pw.c_str(), looked_up_pw );
	}

	for ( i = 0 ; i < 100 ; i ++ )
	{
		pw = gen_passwd( i );
		looked_up_pw = PasswordRAMStore::lookup( gen_key(i) );
		looked_up_len = strlen( looked_up_pw );
		EXPECT_TRUE( PasswordRAMStore::erase( gen_key(i) ) );
		EXPECT_TRUE( mem_is_zero( looked_up_pw, looked_up_len ) );
	}

	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, OneHundredPasswordsBackwards )
{
	// Test 100, 20 character passwords can be stored, looked up and erased (and
	// zeroed).  Passwords are erased backwards (last stored to first stored).
	unsigned int i;
	for ( i = 0 ; i < 100 ; i ++ )
	{
		pw = gen_passwd( i );
		EXPECT_TRUE( PasswordRAMStore::store( gen_key(i), pw.c_str() ) );
	}

	for ( i = 0 ; i < 100 ; i ++ )
	{
		pw = gen_passwd( i );
		looked_up_pw = PasswordRAMStore::lookup( gen_key(i) );
		EXPECT_STREQ( pw.c_str(), looked_up_pw );
	}

	for ( i = 100; i-- > 0 ; )
	{
		pw = gen_passwd( i );
		looked_up_pw = PasswordRAMStore::lookup( gen_key(i) );
		looked_up_len = strlen( looked_up_pw );
		EXPECT_TRUE( PasswordRAMStore::erase( gen_key(i) ) );
		EXPECT_TRUE( mem_is_zero( looked_up_pw, looked_up_len ) );
	}

	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, LongPassword )
{
	// Test a 4095 byte password can be stored and looked up (and erased and zeroed).
	// [Implementation knowledge: size]
	pw = std::string( ProtectedMemSize-1, 'X' );

	EXPECT_TRUE( PasswordRAMStore::store( "key-long", pw.c_str() ) );

	looked_up_pw = PasswordRAMStore::lookup( "key-long" );
	EXPECT_STREQ( pw.c_str(), looked_up_pw );

	looked_up_len = strlen( looked_up_pw );
	EXPECT_TRUE( PasswordRAMStore::erase( "key-long" ) );
	EXPECT_TRUE( mem_is_zero( looked_up_pw, looked_up_len ) );

	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, TooLongPassword )
{
	// Test a 4096 byte password can't be stored nor looked up or erased.
	// [Implementation knowledge: size]
	std::string pw = std::string( ProtectedMemSize, 'X' );

	EXPECT_FALSE( PasswordRAMStore::store( "key-too-long", pw.c_str() ) );
	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );

	looked_up_pw = PasswordRAMStore::lookup( "key-too-long" );
	EXPECT_TRUE(looked_up_pw == nullptr);

	EXPECT_FALSE(PasswordRAMStore::erase("key-too-long"));
	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}

TEST_F( PasswordRAMStoreTest, TotalErasure )
{
	// Test all passwords are erased (and zeroed using the same code called during
	// password cache destruction).
	unsigned int i;
	for ( i = 0 ; i < 100 ; i ++ )
	{
		pw = gen_passwd( i );
		EXPECT_TRUE( PasswordRAMStore::store( gen_key(i), pw.c_str() ) );
	}
	EXPECT_FALSE( mem_is_zero( protected_mem, ProtectedMemSize ) );

	PasswordRAMStoreTest::erase_all();
	EXPECT_TRUE( mem_is_zero( protected_mem, ProtectedMemSize ) );
}


}  // namespace GParted
