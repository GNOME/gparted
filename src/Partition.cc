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
 
#include "../include/Partition.h"

namespace GParted
{
	
Partition::Partition( )
{
	this ->error = ""; //just to be sure...
	this ->status = GParted::STAT_REAL ;
}

void Partition::Set(	const Glib::ustring & partition,
			const int partition_number,
			const PartitionType type,
			const Glib::ustring & filesystem,
			const Sector & sector_start,
			const Sector & sector_end,
			const Sector & sectors_used,
			const bool inside_extended,
			const bool busy )
{
	this->partition = partition;
	this->partition_number = partition_number;
	this->type = type;
	this->filesystem = filesystem;
	this->sector_start = sector_start;
	this->sector_end = sector_end;
	this->sectors_used = sectors_used;
	sectors_used != -1 ?   this->sectors_unused = ( sector_end - sector_start)  - sectors_used  :  this->sectors_unused = -1 ;
	this->color.set( Get_Color( filesystem ) );
	this->color_string = Get_Color( filesystem );
	this->inside_extended = inside_extended;
	this->busy = busy;
}

void Partition::Set_Unallocated( Sector sector_start, Sector sector_end, bool inside_extended )
{
	this ->Set( _("Unallocated"), -1, GParted::UNALLOCATED, "unallocated", sector_start, sector_end , -1, inside_extended, false); 
	this ->error = "" ;
	this ->flags = "" ;
	this ->status = GParted::STAT_REAL ;
}

Glib::ustring Partition::Get_Color( const Glib::ustring & filesystem ) 
{ // very ugly, but somehow i can't figure out a more efficient way. must be lack of sleep or so... :-)
	//purple teints
	if		( filesystem == "ext2" )	return "#9DB8D2" ;													
	else if 	( filesystem == "ext3" )	return "#7590AE";								
	
	//brown
	else if 	( filesystem == "linux-swap" )	return "#C1665A" ;				
		
	//greenisch stuff..
	else if 	( filesystem == "fat16" ) 	return "green"	 ;			
	else if	( filesystem == "fat32" ) 		return "#18D918";							
	else if 	( filesystem == "ntfs" )	return "#42E5AC";				
	
	//blue
	else if	( filesystem == "reiserfs" )		return "blue"		 ;						
	
	//libparted can only detect  these, i decided to "yellow them" :^)
	else if	( filesystem == "HFS" )			return "yellow"	 ;	
	else if	( filesystem == "JFS" )			return "yellow"	 ;	
	else if	( filesystem == "UFS" )			return "yellow"	 ;	
	else if	( filesystem == "XFS" )			return "yellow"	 ;	
	
	//darkgrey and ligthblue
	else if 	( filesystem == "unallocated" ) return "darkgrey" ;
	else if 	( filesystem == "extended" )	return "#7DFCFE" ;
	
	//unknown filesystem	( damaged, or simply unknown )	
	else return "black";
}

void Partition::Update_Number( int new_number )
{   //of course this fails when we have devicenames with numbers over 99
	partition_number >= 10 ? partition = partition.substr( 0, partition.length() -2 ) : partition = partition.substr( 0, partition.length() -1 ) ;
		
	partition_number = new_number;
	std::ostringstream os;
	os << new_number;
	partition += os.str();
}

long Partition::Get_Length_MB( )
{
	return Sector_To_MB( this ->sector_end - this ->sector_start) ;
}

long Partition::Get_Used_MB( )
{ 
	return Sector_To_MB( this ->sectors_used) ;
}

long Partition::Get_Unused_MB( )
{
	return Get_Length_MB( ) - Get_Used_MB( ) ;
}

Partition::~Partition( )
{
}

} //GParted
