/* Copyright (C) 2004 Bart
 * Copyright (C) 2008, 2009, 2010 Curtis Gedak
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

#include "Partition.h"

namespace GParted
{

Partition::Partition()
{
	Reset() ;
}

Partition * Partition::clone() const
{
	// Virtual copy constructor method
	// Reference:
	//     Wikibooks: More C++ Idioms / Virtual Constructor
	//     https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Virtual_Constructor
	return new Partition( *this );
}

void Partition::Reset()
{
	path.clear();
	messages .clear() ;
	status = GParted::STAT_REAL ;
	type = GParted::TYPE_UNALLOCATED ;
	alignment = ALIGN_STRICT ;
	filesystem = GParted::FS_UNALLOCATED ;
	have_filesystem_label = false;
	uuid .clear() ;
	name.clear();
	partition_number = sector_start = sector_end = sectors_used = sectors_unused = -1;
	sectors_unallocated = 0 ;
	significant_threshold = 1 ;
	free_space_before = -1 ;
	sector_size = 0 ;
	fs_block_size = -1;
	inside_extended = busy = strict_start = false ;
	logicals .clear() ;
	flags .clear() ;
	mountpoints .clear() ;
}

void Partition::Set( const Glib::ustring & device_path,
                     const Glib::ustring & partition,
                     int partition_number,
                     PartitionType type,
                     FSType filesystem,
                     Sector sector_start,
                     Sector sector_end,
                     Byte_Value sector_size,
                     bool inside_extended,
                     bool busy )
{
	this ->device_path = device_path ;
	this->path = partition;
	this ->partition_number = partition_number;
	this ->type = type;
	this ->filesystem = filesystem;
	this ->sector_start = sector_start;
	this ->sector_end = sector_end;
	this ->sector_size = sector_size;
	this ->inside_extended = inside_extended;
	this ->busy = busy;
}

//Set file system size and free space, which also calculates unallocated
//  space.  Set sectors_fs_size = -1 for unknown.
void Partition::set_sector_usage( Sector sectors_fs_size, Sector sectors_fs_unused )
{
	Sector length = get_sector_length() ;
	if (    0 <= sectors_fs_size   && sectors_fs_size   <= length
	     && 0 <= sectors_fs_unused && sectors_fs_unused <= sectors_fs_size
	   )
	{
		sectors_used          = sectors_fs_size - sectors_fs_unused ;
		sectors_unused        = sectors_fs_unused ;
		sectors_unallocated   = length - sectors_fs_size ;
		significant_threshold = calc_significant_unallocated_sectors() ;
	}
	else if ( sectors_fs_size == -1 )
	{
		if ( 0 <= sectors_fs_unused && sectors_fs_unused <= length )
		{
			sectors_used   = length - sectors_fs_unused ;
			sectors_unused = sectors_fs_unused ;
		}
		else
		{
			 sectors_used = -1 ;
			 sectors_unused = -1 ;
		}
		sectors_unallocated   = 0 ;
		significant_threshold = 1 ;
	}
}

bool Partition::sector_usage_known() const
{
	return sectors_used >= 0 && sectors_unused >= 0 ;
}

Sector Partition::estimated_min_size() const
{
	//Add unallocated sectors up to the significant threshold, to
	//  account for any intrinsic unallocated sectors in the
	//  file systems minimum partition size.
	if ( sectors_used >= 0 )
		return sectors_used + std::min( sectors_unallocated, significant_threshold ) ;
	return -1 ;
}

//Return user displayable used sectors.
//  Only add unallocated sectors up to the significant threshold to
//  account for any intrinsic unallocated sectors in the file system.
//  Above the threshold just return the used sectors figure.
Sector Partition::get_sectors_used() const
{
	if ( sectors_used >= 0 )
	{
		if ( sectors_unallocated < significant_threshold )
			return sectors_used + sectors_unallocated ;
		else
			return sectors_used ;
	}
	return -1 ;
}

//Return user displayable unused sectors.
Sector Partition::get_sectors_unused() const
{
	return sectors_unused ;
}

//Return user displayable unallocated sectors.
//  Return zero below the significant unallocated sectors threshold, as
//  the figure has been added to the displayable used sectors.  Above the
//  threshold just return the unallocated sectors figure.
Sector Partition::get_sectors_unallocated() const
{
	if ( sectors_unallocated < significant_threshold )
		return 0 ;
	else
		return sectors_unallocated ;
}

// Update size, position and FS usage from new_size.
void Partition::resize( const Partition & new_size )
{
	sector_start      = new_size.sector_start;
	sector_end        = new_size.sector_end;
	alignment         = new_size.alignment;
	free_space_before = new_size.free_space_before;
	strict_start      = new_size.strict_start;

	Sector fs_size   = new_size.sectors_used + new_size.sectors_unused;
	Sector fs_unused = fs_size - new_size.sectors_used;
	set_sector_usage( fs_size, fs_unused );
}

void Partition::Set_Unallocated( const Glib::ustring & device_path,
                                 Sector sector_start,
                                 Sector sector_end,
                                 Byte_Value sector_size,
                                 bool inside_extended )
{
	Reset() ;
	
	Set( device_path,
	     Utils::get_filesystem_string( FS_UNALLOCATED ),
	     -1,
	     TYPE_UNALLOCATED,
	     FS_UNALLOCATED,
	     sector_start,
	     sector_end,
	     sector_size,
	     inside_extended,
	     false ); 
	
	status = GParted::STAT_REAL ;
}

void Partition::set_unpartitioned( const Glib::ustring & device_path,
                                   const Glib::ustring & partition_path,
                                   FSType fstype,
                                   Sector length,
                                   Byte_Value sector_size,
                                   bool busy )
{
	Reset();
	Set( device_path,
	     // The path from the parent Device object and this child Partition object
	     // spanning the whole device would appear to be the same.  However the former
	     // may have come from the command line and the later is the canonicalised
	     // name returned from libparted.  (See GParted_Core::set_device_from_disk()
	     // "loop" table and GParted_Core::set_device_one_partition() calls to
	     // set_unpartitioned()).  Therefore they can be different.
	     //
	     // For unrecognised whole disk device partitions use "unallocated" as the
	     // partition path to match what Set_Unallocated() does when it created such
	     // whole disk device partitions.
	     ( fstype == FS_UNALLOCATED ) ? Utils::get_filesystem_string( FS_UNALLOCATED )
	                                  : partition_path,
	     1,
	     TYPE_UNPARTITIONED,
	     fstype,
	     0LL,
	     length - 1LL,
	     sector_size,
	     false,
	     busy );
}

void Partition::Update_Number( int new_number )
{  
	unsigned int index = path.rfind( Utils::num_to_str( partition_number ) );
	if ( index < path.length() )
		path.replace( index,
		              Utils::num_to_str( partition_number ).length(),
		              Utils::num_to_str( new_number ) );

	partition_number = new_number;
}
	
void Partition::set_path( const Glib::ustring & path )
{
	this->path = path;
}
	
Byte_Value Partition::get_byte_length() const 
{
	if ( get_sector_length() >= 0 )
		return get_sector_length() * sector_size ;
	else
		return -1 ;
}

Sector Partition::get_sector_length() const 
{
	if ( sector_start >= 0 && sector_end >= 0 )
		return sector_end - sector_start + 1 ;
	else
		return -1 ;
}

Glib::ustring Partition::get_path() const
{
	return path;
}

bool Partition::filesystem_label_known() const
{
	return have_filesystem_label;
}

//Return the file system label or "" if unknown.
Glib::ustring Partition::get_filesystem_label() const
{
	if ( have_filesystem_label )
		return filesystem_label;
	return "";
}

void Partition::set_filesystem_label( const Glib::ustring & filesystem_label )
{
	this->filesystem_label = filesystem_label;
	have_filesystem_label = true;
}

bool Partition::operator==( const Partition & partition ) const
{
	return device_path == partition .device_path &&
	       partition_number == partition .partition_number && 
	       sector_start == partition .sector_start && 
	       type == partition .type ;
}

bool Partition::operator!=( const Partition & partition ) const
{
	return ! ( *this == partition ) ;
}

//Get the "best" display integers (pixels or percentages) from partition
//  usage figures.  Rounds the smaller two figures and then subtracts
//  them from the desired total for the largest figure.
void Partition::get_usage_triple( int imax, int & i1, int & i2, int & i3 ) const
{
	Sector s1 = get_sectors_used() ;
	Sector s2 = get_sectors_unused() ;
	Sector s3 = get_sectors_unallocated() ;
	if ( s1 < 0 ) s1 = 0 ;
	if ( s2 < 0 ) s2 = 0 ;
	if ( s3 < 0 ) s3 = 0 ;
	Sector stot = s1 + s2 + s3 ;
	if ( s1 <= s2 && s1 <= s3 )
		get_usage_triple_helper( stot, s1, s2, s3, imax, i1, i2, i3 ) ;
	else if ( s2 <  s1 && s2 <= s3 )
		get_usage_triple_helper( stot, s2, s1, s3, imax, i2, i1, i3 ) ;
	else if ( s3 <  s1 && s3 <  s2 )
		get_usage_triple_helper( stot, s3, s1, s2, imax, i3, i1, i2 ) ;
}

//Calculate the "best" display integers when s1 <= s2 and s1 <= s3.
//  Ensure i1 <= i2 and i1 <= i3.
void Partition::get_usage_triple_helper( Sector stot, Sector s1, Sector s2, Sector s3, int imax, int & i1, int & i2, int & i3 )
{
	int t ;
	i1 = Utils::round( static_cast<double>( s1 ) / stot * imax ) ;
	if ( s2 <= s3 )
	{
		i2 = Utils::round( static_cast<double>( s2 ) / stot * imax ) ;
		i3 = imax - i1 - i2 ;
		if ( i1 > i3 )
		{
			// i1 rounded up making it larger than i3.  Swap i1 with i3.
			t  = i1 ;
			i1 = i3 ;
			i3 = t ;
		}
	}
	else
	{
		i3 = Utils::round( static_cast<double>( s3 ) / stot * imax ) ;
		i2 = imax - i1 - i3 ;
		if ( i1 > i2 )
		{
			// i1 rounded up making it larger than i2.  Swap i1 with i2.
			t  = i1 ;
			i1 = i2 ;
			i2 = t ;
		}
	}
}

void Partition::add_mountpoint( const Glib::ustring & mountpoint )
{
	this ->mountpoints .push_back( mountpoint ) ;
}

void Partition::add_mountpoints( const std::vector<Glib::ustring> & mountpoints )
{
	this ->mountpoints .insert( this ->mountpoints .end(), mountpoints .begin(), mountpoints .end() ) ;
}

Glib::ustring Partition::get_mountpoint() const 
{
	if ( mountpoints .size() > 0 )
		return mountpoints .front() ;

	return "" ;
}

std::vector<Glib::ustring> Partition::get_mountpoints() const 
{
	return mountpoints ;
}

Sector Partition::get_sector() const 
{
	return (sector_start + sector_end) / 2 ; 
}
	
bool Partition::test_overlap( const Partition & partition ) const
{
	return ( (partition .sector_start >= sector_start && partition .sector_start <= sector_end) 
		 ||
		 (partition .sector_end >= sector_start && partition .sector_end <= sector_end)
		 ||
		 (partition .sector_start < sector_start && partition .sector_end > sector_end) ) ;
}

void Partition::clear_mountpoints()
{
	mountpoints .clear() ;
}

//Return threshold of sectors which is considered above the intrinsic
//  level for a file system which "fills" the partition.  Calculation
//  is:
//      %age of partition size           , when
//      5%                               ,           ptn size <= 100 MiB
//      linear scaling from 5% down to 2%, 100 MiB < ptn size <= 1 GiB
//      2%                               , 1 GiB   < ptn size
Sector Partition::calc_significant_unallocated_sectors() const
{
	const double HIGHER_UNALLOCATED_FRACTION = 0.05 ;
	const double LOWER_UNALLOCATED_FRACTION  = 0.02 ;
	Sector     length   = get_sector_length() ;
	Byte_Value byte_len = length * sector_size ;
	Sector     significant ;

	if ( byte_len <= 0 )
	{
		significant = 1;
	}
	else if ( byte_len <= 100 * MEBIBYTE )
	{
		significant = Utils::round( length * HIGHER_UNALLOCATED_FRACTION ) ;
	}
	else if ( byte_len <= 1 * GIBIBYTE )
	{
		double fraction = ( HIGHER_UNALLOCATED_FRACTION - LOWER_UNALLOCATED_FRACTION ) -
		                  ( byte_len - 100 * MEBIBYTE ) * ( HIGHER_UNALLOCATED_FRACTION - LOWER_UNALLOCATED_FRACTION ) /
		                  ( 1 * GIBIBYTE - 100 * MEBIBYTE ) +
		                  LOWER_UNALLOCATED_FRACTION ;
		significant = Utils::round( length * fraction ) ;
	}
	else
	{
		significant = Utils::round( length * LOWER_UNALLOCATED_FRACTION ) ;
	}
	if ( significant <= 1 )
		significant = 1;
	return significant ;
}

Partition::~Partition()
{
}

} //GParted
