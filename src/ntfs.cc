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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
 
 
#include "../include/ntfs.h"

namespace GParted
{

FS ntfs::get_filesystem_support( )
{
	FS fs ;
	
	fs .filesystem = GParted::FS_NTFS ;
	if ( ! system( "which ntfscluster 1>/dev/null 2>/dev/null" ) ) 
		fs .read = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which mkntfs 1>/dev/null 2>/dev/null" ) ) 
		fs .create = GParted::FS::EXTERNAL ;
	
	if ( ! system( "which ntfsfix 1>/dev/null 2>/dev/null" ) ) 
		fs .check = GParted::FS::EXTERNAL ;
	
	//resizing is a delicate process ...
	if ( ! system( "which ntfsresize 1>/dev/null 2>/dev/null" ) && fs .check ) 
	{
		fs .grow = GParted::FS::EXTERNAL ;
		
		if ( fs .read ) //needed to determine a min filesystemsize..
			fs .shrink = GParted::FS::EXTERNAL ;
	}
	
	//we need ntfsresize to set correct used/unused after cloning
	if ( ! system( "which ntfsclone 1>/dev/null 2>/dev/null" ) && fs .grow ) 
		fs .copy = GParted::FS::EXTERNAL ;
	
	return fs ;
}

void ntfs::Set_Used_Sectors( Partition & partition ) 
{
	char c_buf[ 512 ] ;
	FILE *f ;
	
	Glib::ustring output ;

        //get free sectors..
	f = popen( ( "LC_ALL=C ntfscluster --force " + partition .partition ) .c_str( ), "r" ) ;
	while ( fgets( c_buf, 512, f ) )
	{
		output = Glib::locale_to_utf8( c_buf ) ;
		
		//free sectors
		if ( output .find( "sectors of free space" ) < output .length( ) )
		{
			partition .Set_Unused( atol( (output .substr( output .find( ":" ) +1, output .length( ) ) ) .c_str( ) ) ) ;
			break ;
		}
	}
	pclose( f ) ;
}

bool ntfs::Create( const Partition & new_partition )
{
	return ! Execute_Command( "mkntfs -Q " + new_partition .partition ) ;
}

bool ntfs::Resize( const Partition & partition_new, bool fill_partition )
{
	Glib::ustring str_temp = "echo y | ntfsresize -f " + partition_new .partition ;
	
	if ( ! fill_partition )
		str_temp += " -s " + Utils::num_to_str( partition_new .Get_Length_MB( ) - cylinder_size, true ) + "M" ;
	
	return ! Execute_Command( str_temp ) ;
}

bool ntfs::Copy( const Glib::ustring & src_part_path, const Glib::ustring & dest_part_path )
{
	if ( ! Execute_Command( "ntfsclone -f --overwrite " + dest_part_path + " " + src_part_path ) )
	{
		Partition partition ;
		partition .partition = dest_part_path ;
		return Resize( partition, true ) ;
	}
	
	return false ;
}

bool ntfs::Check_Repair( const Partition & partition )
{
	//according to Szaka it's best to use ntfsresize to check the partition for errors
	//since --info is read-only i'll leave it out. just calling ntfsresize --force has also a tendency of fixing stuff :)
	return Resize( partition, true ) ;
}

int ntfs::get_estimated_time( long MB_to_Consider )
{
	return -1 ;
}
	

} //GParted


