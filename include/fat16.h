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


#ifndef GPARTED_FAT16_H
#define GPARTED_FAT16_H

#include "FileSystem.h"
#include "Partition.h"

namespace GParted
{

class fat16 : public FileSystem
{
public:
	fat16(enum FSType fstype) : m_specific_fstype(fstype), m_ignore_label_noname(false)  {};

	const Glib::ustring & get_custom_text( CUSTOM_TEXT ttype, int index = 0 ) const;
	FS get_filesystem_support() ;
	void set_used_sectors( Partition & partition ) ;
	void read_label( Partition & partition ) ;
	bool write_label( const Partition & partition, OperationDetail & operationdetail ) ;
	void read_uuid( Partition & partition ) ;
	bool write_uuid( const Partition & partition, OperationDetail & operationdetail ) ;
	bool create( const Partition & new_partition, OperationDetail & operationdetail ) ;
	bool check_repair( const Partition & partition, OperationDetail & operationdetail ) ;

private:
	const Glib::ustring sanitize_label( const Glib::ustring & label ) const;
	static Glib::ustring remove_spaces(const Glib::ustring& str);

	const enum FSType m_specific_fstype;
	bool              m_ignore_label_noname;
};


} //GParted

#endif /* GPARTED_FAT16_H */
