/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Curtis Gedak
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

#include "fat16.h"
#include "FileSystem.h"
#include "Partition.h"

#include <glibmm/miscutils.h>
#include <glibmm/shell.h>


namespace GParted
{


const Glib::ustring & fat16::get_custom_text( CUSTOM_TEXT ttype, int index ) const
{
	static const Glib::ustring change_uuid_warning[] =
		{ _("Changing the UUID might invalidate the Windows Product Activation "
		    "(WPA) key"),
		  _("On FAT and NTFS file systems, the Volume Serial Number is used as "
		    "the UUID. Changing the Volume Serial Number on the Windows system "
		    "partition, normally C:, might invalidate the WPA key. An invalid "
		    "WPA key will prevent login until you reactivate Windows."),
		  _("Changing the UUID on external storage media and non-system "
		    "partitions is usually safe, but guarantees cannot be given."),
		  ""
		};

	int i ;
	switch ( ttype ) {
		case CTEXT_CHANGE_UUID_WARNING :
			for ( i = 0 ; i < index && change_uuid_warning[i] != "" ; i++ )
				;  // Just iterate...
			return change_uuid_warning[i];
		default :
			return FileSystem::get_custom_text( ttype, index ) ;
	}
}


FS fat16::get_filesystem_support()
{
	FS fs(m_specific_fstype);

	// hack to disable silly mtools warnings
	setenv( "MTOOLS_SKIP_CHECK", "1", 0 );

	fs .busy = FS::GPARTED ;

	if (! Glib::find_program_in_path("mdir").empty())
	{
		fs.read_uuid = FS::EXTERNAL;
		if (! Glib::find_program_in_path("minfo").empty())
			fs.read = FS::EXTERNAL;
	}

	//find out if we can create fat file systems
	if ( ! Glib::find_program_in_path( "mkfs.fat" ) .empty() )
	{
		fs.create = FS::EXTERNAL;
		fs.create_with_label = FS::EXTERNAL;
	}

	if ( ! Glib::find_program_in_path( "fsck.fat" ) .empty() )
	{
		fs.check = FS::EXTERNAL;
	}

	bool fatlabel_found = ! Glib::find_program_in_path("fatlabel").empty();
	bool mlabel_found   = ! Glib::find_program_in_path("mlabel").empty();
	if (fatlabel_found)                  { fs.read_label  = FS::EXTERNAL; }
	if (fatlabel_found && mlabel_found)  { fs.write_label = FS::EXTERNAL; }
	if (mlabel_found)                    { fs.write_uuid  = FS::EXTERNAL; }

	if (fatlabel_found)
	{
		Glib::ustring output;
		Glib::ustring error;
		Utils::execute_command("fatlabel --version", output, error, true);
		int fatlabel_major_ver = 0;
		int fatlabel_minor_ver = 0;
		if (sscanf(output.c_str(), "fatlabel %d.%d", &fatlabel_major_ver, &fatlabel_minor_ver) == 2)
		{
			// When a FAT file system doesn't have a label it will have no
			// volume entry in the root directory and the FAT Boot Sector will
			// contain "NO NAME".  fatlabel <= 4.1 incorrectly fell back to
			// reporting the label from the FAT Boot Sector when there was no
			// volume entry in the root directory.  Detect old versions of
			// fatlabel with this fault.
			m_ignore_label_noname =    (fatlabel_major_ver < 4)
			                        || (fatlabel_major_ver == 4 && fatlabel_minor_ver <= 1);
		}
	}

	//resizing of start and endpoint are provided by libparted
	fs.grow = FS::LIBPARTED;
	fs.shrink = FS::LIBPARTED;
	fs .move = FS::GPARTED ;
	fs.copy = FS::GPARTED;
	fs .online_read = FS::GPARTED ;

	if (fs.fstype == FS_FAT16)
	{
		m_fs_limits.min_size = 16 * MEBIBYTE;
		m_fs_limits.max_size = (4096 - 1) * MEBIBYTE;  // Maximum seems to be just less than 4096 MiB.
	}
	else //FS_FAT32
	{
		// Smaller file systems will cause Windows' scandisk to fail.
		m_fs_limits.min_size = 33 * MEBIBYTE;

		// Maximum FAT32 volume size with 512 byte sectors is 2 TiB.
		// *   Wikipedia: File Allocation Table / FAT32
		//     https://en.wikipedia.org/wiki/File_Allocation_Table#FAT32
		m_fs_limits.max_size = 2 * TEBIBYTE;
	}

	return fs ;
}

void fat16::set_used_sectors( Partition & partition ) 
{
	// Use mdir's scanning of the FAT to get the free space.
	// https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#File_Allocation_Table
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("mdir -i " + Glib::shell_quote(partition.get_path()) + " ::/",
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// Bytes free.  Parse the value from the bottom of the directory listing by mdir.
	// Example line "                        277 221 376 bytes free".
	Glib::ustring spaced_number_str = Utils::regexp_label(output, "^ *([0-9 ]*) bytes free$");
	Glib::ustring number_str = remove_spaces(spaced_number_str);
	long long bytes_free = -1;
	if (number_str.size() > 0)
		bytes_free = atoll(number_str.c_str());

	// Use minfo's reporting of the BPB (BIOS Parameter Block) to get the file system
	// size and FS block (cluster) size.
	// https://en.wikipedia.org/wiki/Design_of_the_FAT_file_system#BIOS_Parameter_Block
	exit_status = Utils::execute_command("minfo -i " + Glib::shell_quote(partition.get_path()) + " ::",
	                                     output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// FS logical sector size in bytes
	long long logical_sector_size = -1;
	Glib::ustring::size_type index = output.find("sector size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "sector size: %lld bytes", &logical_sector_size);

	// Cluster size in FS logical sectors
	long long cluster_size = -1;
	index = output.find("cluster size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "cluster size: %lld sectors", &cluster_size);

	// FS size in logical sectors if <= 65535, or 0 otherwise
	long long small_size = -1;
	index = output.find("small size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "small size: %lld sectors", &small_size);

	// FS size in logical sectors if > 65535, or 0 otherwise
	long long big_size = -1;
	index = output.find("big size:");
	if (index < output.length())
		sscanf(output.substr(index).c_str(), "big size: %lld sectors", &big_size);

	// FS size in logical sectors
	long long logical_sectors = -1;
	if (small_size > 0)
		logical_sectors = small_size;
	else if (big_size > 0)
		logical_sectors = big_size;

	if (bytes_free > -1 && logical_sector_size > -1 && cluster_size > -1 && logical_sectors > -1)
	{
		Sector fs_free = bytes_free / partition.sector_size;
		Sector fs_size = logical_sectors * logical_sector_size / partition.sector_size;
		partition.set_sector_usage(fs_size, fs_free);
		partition.fs_block_size = logical_sector_size * cluster_size;
	}
}


void fat16::read_label(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("fatlabel " + Glib::shell_quote(partition.get_path()),
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	// (#286) Separate fatlabel output using heuristic that the label is on the last
	// or only line and optional FSCK type messages are before that.
	Glib::ustring label = Utils::trim_trailing_new_line(output);
	Glib::ustring::size_type prev_new_line = label.find_last_of('\n');
	if (prev_new_line != label.npos)
	{
		Glib::ustring fsck_messages = label.substr(0, prev_new_line);
		partition.push_back_message(fsck_messages);
		label = label.substr(prev_new_line, label.npos);
	}
	label = Utils::trim(label);

	if (m_ignore_label_noname && label == "NO NAME")
		// fatlabel <= 4.1 reported the label as "NO NAME".  Ignore it as it was
		// very likely read from the the FAT Boot Sector when the FAT file system
		// has no label.
		label = "";
	partition.set_filesystem_label(label);
}


bool fat16::write_label( const Partition & partition, OperationDetail & operationdetail )
{
	Glib::ustring cmd;
	if ( partition.get_filesystem_label().empty() )
		// Use mlabel to clear the label because fatlabel --reset option didn't
		// exist before dosfstools 4.2 and that isn't available everywhere yet.
		cmd = "mlabel -c -i " + Glib::shell_quote(partition.get_path()) + " ::";
	else
		cmd = "fatlabel " + Glib::shell_quote(partition.get_path()) + " " +
		      Glib::shell_quote(sanitize_label(partition.get_filesystem_label()));

	return ! operationdetail.execute_command(cmd, EXEC_CHECK_STATUS);
}


void fat16::read_uuid(Partition& partition)
{
	Glib::ustring output;
	Glib::ustring error;
	int exit_status = Utils::execute_command("mdir -f -i " + Glib::shell_quote(partition.get_path()) + " ::/",
	                        output, error, true);
	if (exit_status != 0)
	{
		if (! output.empty())
			partition.push_back_message(output);
		if (! error.empty())
			partition.push_back_message(error);
		return;
	}

	partition.uuid = Utils::regexp_label(output, "Volume Serial Number is[[:blank:]]([^[:space:]]+)");
	if (partition.uuid == "0000-0000")
		partition.uuid.clear();
}


bool fat16::write_uuid( const Partition & partition, OperationDetail & operationdetail )
{
	return ! operationdetail.execute_command("mlabel -s -n -i " + Glib::shell_quote(partition.get_path()) + " ::",
	                        EXEC_CHECK_STATUS);
}


bool fat16::create( const Partition & new_partition, OperationDetail & operationdetail )
{
	Glib::ustring fat_size = m_specific_fstype == FS_FAT16 ? "16" : "32";
	Glib::ustring label_args = new_partition.get_filesystem_label().empty() ? "" :
	                           "-n " + Glib::shell_quote( sanitize_label( new_partition.get_filesystem_label() ) ) + " ";
	return ! operationdetail.execute_command("mkfs.fat -F" + fat_size + " -v -I " + label_args +
	                        Glib::shell_quote(new_partition.get_path()),
	                        EXEC_CHECK_STATUS|EXEC_CANCEL_SAFE);
}


bool fat16::check_repair( const Partition & partition, OperationDetail & operationdetail )
{
	int exit_status = operationdetail.execute_command("fsck.fat -a -w -v " +
	                        Glib::shell_quote(partition.get_path()),
	                        EXEC_CANCEL_SAFE);
	// From fsck.fat(8) manual page:
	//     EXIT STATUS
	//         0   No recoverable errors have been detected.
	//         1   Recoverable errors have been detected or fsck.fat has discovered an
	//             internal inconsistency.
	//         2   Usage error.
	bool success = (exit_status == 0 || exit_status == 1);
	operationdetail.get_last_child().set_success_and_capture_errors(success);
	return success;
}


//Private methods

const Glib::ustring fat16::sanitize_label( const Glib::ustring &label ) const
{
	Glib::ustring uppercase_label = label.uppercase();
	Glib::ustring new_label;

	// Copy uppercase label excluding prohibited characters.
	// [1] Microsoft TechNet: Label
	//     https://technet.microsoft.com/en-us/library/bb490925.aspx
	// [2] Replicated in Wikikedia: label (command)
	//     https://en.wikipedia.org/wiki/Label_%28command%29
	// Also exclude:
	// * Single quote (') because it is encoded by mlabel but not understood by
	//   Windows;
	// * ASCII control characters below SPACE because mlabel requests input in the
	//   same way it does for prohibited characters causing GParted to wait forever.
	Glib::ustring prohibited_chars( "*?.,;:/\\|+=<>[]\"'" );
	for ( unsigned int i = 0 ; i < uppercase_label.size() ; i ++ )
		if ( prohibited_chars.find( uppercase_label[i] ) == Glib::ustring::npos &&
		     uppercase_label[i] >= ' '                                             )
			new_label.push_back( uppercase_label[i] );

	return new_label ;
}


Glib::ustring fat16::remove_spaces(const Glib::ustring& str)
{
	Glib::ustring result;

	for (unsigned int i = 0; i < str.size(); i++)
	{
		if (str[i] != ' ')
			result += str[i];
	}

	return result;
}


}  // namespace GParted
