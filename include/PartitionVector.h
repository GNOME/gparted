/* Copyright (C) 2015 Mike Fleetwood
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

/* Minimal implementation of a class that behaves like a std::vector<Partition> which can
 * be used in it's place with minimal change to the existing GParted code which expects to
 * be working with a std::vector<Partition>.
 * Reference:
 *     C++ Reference to std::vector
 *     http://www.cplusplus.com/reference/vector/vector/
 */

#ifndef GPARTED_PARTITIONVECTOR_H
#define GPARTED_PARTITIONVECTOR_H

#include "../include/Partition.h"

#include <cstddef>
#include <vector>

namespace GParted
{

class Partition;        // Forward declarations as Partition and PartitionVector are
class PartitionVector;  // mutually recursive classes.
                        // References:
                        //     Mutually recursive classes
                        //     http://stackoverflow.com/questions/3410637/mutually-recursive-classes
                        //     recursive definition in CPP
                        //     http://stackoverflow.com/questions/4300420/recursive-definition-in-cpp

class PartitionVector {
public:
	typedef size_t size_type;
	typedef std::vector<Partition>::iterator iterator;

	PartitionVector() {};
	~PartitionVector() {};

	// Iterators
	iterator begin()                                   { return v.begin(); };

	// Capacity
	bool empty() const                                 { return v.empty(); };

	// Element access
	Partition & operator[]( size_type n )              { return v[n]; };
	const Partition & operator[]( size_type n ) const  { return v[n]; };
	size_type size() const                             { return v.size(); };
	const Partition & front() const                    { return v.front(); };
	const Partition & back() const                     { return v.back(); };

	// Modifiers
	void pop_back()                                    { v.pop_back(); };
	void erase( const iterator position )              { v.erase( position ); };
	void clear()                                       { v.clear(); };
	void push_back( const Partition & partition )      { v.push_back( partition ); };
	void insert( iterator position, const Partition & partition )
	                                                   { v.insert( position, partition ); };

private:
	std::vector<Partition> v;
};

} //GParted

#endif /* GPARTED_PARTITIONVECTOR_H */
