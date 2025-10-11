/* Copyright (C) 2004 Bart
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

#include "FileSystem.h"
#include "GParted_Core.h"
#include "OperationDetail.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <memory>
#include <glibmm/miscutils.h>
#include <glibmm/stringutils.h>
#include <glibmm/ustring.h>


namespace GParted
{


namespace  // unnamed
{


// Custom deleter class for use with smart pointer type:
//     std::unique_ptr<Glib::ustring, TempWorkingDirDeleter>
// A custom deleter is only called when it contains a none nullptr when destructed.  When
// called this deleter removes the temporary working directory from the file system and
// then deletes (frees) the memory storing it's name.
struct TempWorkingDirDeleter
{
	void operator()(const Glib::ustring* dir_name)
	{
		if (rmdir(dir_name->c_str()) != 0)
		{
			int e = errno;
			std::cerr << "rmdir(\"" << *dir_name << "\"): " + Glib::strerror(e) << std::endl;
		}
		delete dir_name;
		dir_name = nullptr;
	}
};


typedef std::unique_ptr<Glib::ustring, TempWorkingDirDeleter> UniqueTempWorkingDirPtr;


// Unique smart pointer managing the temporary working directory.  Contains either the
// nullptr or a pointer to the name of the temporary working directory only when GParted
// successfully created said directory.  This global variable goes out of scope and is
// destructed after main() returns or exit() is called.  At which point the temporary
// working directory is removed, only if it was created.
static UniqueTempWorkingDirPtr temp_working_dir = nullptr;


// Create private temporary working directory named "$TMPDIR/gparted-XXXXXX" with
// permissions 0700 using mkdtemp(3).  Everything is logged to the operation detail.  On
// success returns unique pointer to the created directory name or on error returns unique
// pointer of the nullptr.
UniqueTempWorkingDirPtr mk_temp_working_dir(OperationDetail& operationdetail)
{
	char dir_buf[4096+1];
	snprintf(dir_buf, sizeof(dir_buf), "%s/gparted-XXXXXX", Glib::get_tmp_dir().c_str());

	// Looks like "mkdir -v" command was run to the user
	operationdetail.add_child(OperationDetail(
	                        Glib::ustring("mkdir -v ") + dir_buf, STATUS_EXECUTE, FONT_BOLD_ITALIC));
	OperationDetail& child_od = operationdetail.get_last_child();
	const char* dir_name = mkdtemp(dir_buf);
	if (nullptr == dir_name)
	{
		int e = errno;
		child_od.add_child(OperationDetail(
		                        Glib::ustring::compose("mkdtemp(\"%1\"): %2", dir_buf, Glib::strerror(e)),
		                        STATUS_NONE,
		                        FONT_MONOSPACE));
		child_od.set_success_and_capture_errors(false);
		return UniqueTempWorkingDirPtr(nullptr);
	}

	// Update command with actually created temporary working directory
	child_od.set_description(Glib::ustring("mkdir -v ") + dir_name, FONT_BOLD_ITALIC);
	child_od.add_child(OperationDetail(
	                        /* TO TRANSLATORS: looks like   Created directory /tmp/gparted-CEzvSp */
	                        Glib::ustring::compose(_("Created directory %1"), dir_name),
	                        STATUS_NONE,
	                        FONT_MONOSPACE));
	child_od.set_success_and_capture_errors(true);

	return UniqueTempWorkingDirPtr(new Glib::ustring(dir_name));
}


}  // unnamed namespace


FileSystem::FileSystem()
{
}

const Glib::ustring & FileSystem::get_custom_text( CUSTOM_TEXT ttype, int index ) const
{
	return get_generic_text( ttype, index ) ;
}

const Glib::ustring & FileSystem::get_generic_text( CUSTOM_TEXT ttype, int index )
{
	/*TO TRANSLATORS: these labels will be used in the partition menu */
	static const Glib::ustring activate_text = _( "_Mount" ) ;
	static const Glib::ustring deactivate_text = _( "_Unmount" ) ;
	static const Glib::ustring empty_text;

	switch ( ttype ) {
		case CTEXT_ACTIVATE_FILESYSTEM :
			return index == 0 ? activate_text : empty_text;
		case CTEXT_DEACTIVATE_FILESYSTEM :
			return index == 0 ? deactivate_text : empty_text;
		default :
			return empty_text;
	}
}


// Create uniquely named temporary mount point directory and add results to operation
// detail.  Will be named "$TMPDIR/gparted-XXXXXX/mnt-N" or ".../mnt-INFIX-N".  Returns
// the name of the created directory or the empty string on error.
Glib::ustring FileSystem::mk_temp_dir( const Glib::ustring & infix, OperationDetail & operationdetail )
{
	// Create private temporary working directory once when first needed.
	if (nullptr == temp_working_dir)
	{
		temp_working_dir = mk_temp_working_dir(operationdetail);
		if (nullptr == temp_working_dir)
			return "";
	}

	// Prepare unique temporary mount point directory name.  Use forever incrementing
	// number (mnt_number) to make names unique.  This is so that there is no chance
	// of mounting over the top of a previous file system in the unlikely event that
	// it could not be unmounted.
	static unsigned int mnt_number = 0;
	mnt_number++;
	char dir_buf [4096+1];
	snprintf(dir_buf, sizeof(dir_buf), "%s/mnt-%s%s%u",
	                        temp_working_dir->c_str(),      // "$TMPDIR/gparted-XXXXXX"
	                        infix.c_str(),
	                        (infix.size() > 0 ? "-" : ""),
	                        mnt_number);

	//Looks like "mkdir -v" command was run to the user
	operationdetail .add_child( OperationDetail(
			Glib::ustring( "mkdir -v " ) + dir_buf, STATUS_EXECUTE, FONT_BOLD_ITALIC ) ) ;
	OperationDetail& child_od = operationdetail.get_last_child();
	if (mkdir(dir_buf, 0700) != 0)
	{
		int e = errno ;
		child_od.add_child(OperationDetail(
		                        Glib::ustring::compose("mkdir(\"%1\", 0700): %2", dir_buf, Glib::strerror(e)),
		                        STATUS_NONE,
		                        FONT_MONOSPACE));
		child_od.set_success_and_capture_errors(false);
		return "";
	}

	child_od.add_child(OperationDetail(
	                        Glib::ustring::compose(_("Created directory %1"), dir_buf),
	                        STATUS_NONE,
	                        FONT_MONOSPACE));
	child_od.set_success_and_capture_errors(true);
	return Glib::ustring(dir_buf);
}


//Remove directory and add results to operation detail
void FileSystem::rm_temp_dir(const Glib::ustring& dir_name, OperationDetail& operationdetail)
{
	//Looks like "rmdir -v" command was run to the user
	operationdetail .add_child( OperationDetail( Glib::ustring( "rmdir -v " ) + dir_name,
	                                             STATUS_EXECUTE, FONT_BOLD_ITALIC ) ) ;
	OperationDetail& child_od = operationdetail.get_last_child();
	if ( rmdir( dir_name .c_str() ) )
	{
		// Don't mark operation as errored just because rmdir failed.  Set to
		// Warning instead.
		int e = errno ;
		child_od.add_child(OperationDetail(
		                        Glib::ustring::compose("rmdir(\"%1\"): ", dir_name) + Glib::strerror(e),
		                        STATUS_NONE,
		                        FONT_MONOSPACE));
		child_od.set_status(STATUS_WARNING);
		return;
	}

	child_od.add_child(OperationDetail(
				/* TO TRANSLATORS: looks like   Removed directory /tmp/gparted-CEzvSp */
				Glib::ustring::compose(_("Removed directory %1"), dir_name),
	                        STATUS_NONE,
	                        FONT_MONOSPACE));
	child_od.set_success_and_capture_errors(true);
}


}  // namespace GParted
