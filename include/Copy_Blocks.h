/*  Copyright (C) 2013 Phillip Susi
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

#ifndef COPY_BLOCKS_H
#define COPY_BLOCKS_H

#include "../include/Operation.h"
#include <parted/parted.h>

namespace GParted {

class copy_blocks {
	const Glib::ustring & src_device;
	const Glib::ustring & dst_device;
	Byte_Value length;
	Byte_Value blocksize;
	OperationDetail &operationdetail;
	bool readonly;
	Byte_Value & total_done;
	char *buf;
	Byte_Value done;
	PedDevice *lp_device_src;
	PedDevice *lp_device_dst;
	Sector offset_src;
	Sector offset_dst;
	Glib::Timer timer_total;
	bool success;
	Glib::ustring error_message;
	bool set_progress_info();
	void copy_thread();
public:
copy_blocks( const Glib::ustring & in_src_device,
	     const Glib::ustring & in_dst_device,
	     Sector src_start,
	     Sector dst_start,
	     Byte_Value in_length,
	     Byte_Value in_blocksize,
	     OperationDetail & in_operationdetail,
	     bool in_readonly,
	     Byte_Value & in_total_done ) :
	src_device( in_src_device ),
		dst_device ( in_dst_device ),
		length ( in_length ),
		blocksize ( in_blocksize ),
		operationdetail ( in_operationdetail ),
		readonly ( in_readonly ),
		total_done ( in_total_done ),
		offset_src ( src_start ),
		offset_dst ( dst_start ) {};
	bool copy();
	void copy_block();
};

} // namespace GParted

#endif // COPY_BLOCKS_H
