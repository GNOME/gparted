/* Copyright (C) 2023 Mike Fleetwood
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

/* insertion_operators
 *
 * Insertion operators for various GParted object types which print the object to an
 * output stream.
 */

#ifndef GPARTED_TEST_INSERTION_OPERATORS_H
#define GPARTED_TEST_INSERTION_OPERATORS_H


#include "OperationDetail.h"

#include <iostream>


namespace GParted
{


std::ostream& operator<<(std::ostream& out, const OperationDetailStatus od_status);
std::ostream& operator<<(std::ostream& out, const OperationDetail& od);


}  // namespace GParted


#endif /* GPARTED_TEST_INSERTION_OPERATORS_H */
