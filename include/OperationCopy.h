/* Copyright (C) 2004 Bart 'plors' Hakvoort
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

#ifndef OPERATION_COPY
#define OPERATION_COPY

#include "../include/Operation.h"

namespace GParted
{

class OperationCopy : public Operation
{
public:
	OperationCopy( const Device & device,
		       const Partition & partition_orig,
		       const Partition & partition_new,
		       const Partition & partition_copied) ;
	
	void apply_to_visual( std::vector<Partition> & partitions ) ;

	Partition partition_copied ;

private:
	void create_description() ;
} ;

} //GParted

#endif //OPERATION_COPY
