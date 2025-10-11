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


#include "insertion_operators.h"
#include "OperationDetail.h"

#include <iostream>
#include <stddef.h>
#include <glibmm/ustring.h>


namespace GParted
{


// Hacky XML parser which strips italic and bold markup added in
// OperationDetail::set_description() and reverts just these 5 characters &<>'" encoded by
// Glib::Markup::escape_text() -> g_markup_escape_text() -> append_escaped_text().
static Glib::ustring strip_markup(const Glib::ustring& str)
{
	size_t len = str.length();
	size_t i = 0;
	Glib::ustring ret;
	ret.reserve(len);
	while (i < len)
	{
		if (str.compare(i, 3, "<i>") == 0)
			i += 3;
		else if (str.compare(i, 4, "</i>") == 0)
			i += 4;
		else if (str.compare(i, 3, "<b>") == 0)
			i += 3;
		else if (str.compare(i, 4, "</b>") == 0)
			i += 4;
		else if (str.compare(i, 5, "&amp;") == 0)
		{
			ret.push_back('&');
			i += 5;
		}
		else if (str.compare(i, 4, "&lt;") == 0)
		{
			ret.push_back('<');
			i += 4;
		}
		else if (str.compare(i, 4, "&gt;") == 0)
		{
			ret.push_back('>');
			i += 4;
		}
		else if (str.compare(i, 6, "&apos;") == 0)
		{
			ret.push_back('\'');
			i += 6;
		}
		else if (str.compare(i, 6, "&quot;") == 0)
		{
			ret.push_back('"');
			i += 6;
		}
		else
		{
			ret.push_back(str[i]);
			i++;
		}
	}
	return ret;
}


// Print method for OperationDetailStatus.
std::ostream& operator<<(std::ostream& out, const OperationDetailStatus od_status)
{
	switch (od_status)
	{
		case STATUS_NONE:     out << "NONE";     break;
		case STATUS_EXECUTE:  out << "EXECUTE";  break;
		case STATUS_SUCCESS:  out << "SUCCESS";  break;
		case STATUS_ERROR:    out << "ERROR";    break;
		case STATUS_INFO:     out << "INFO";     break;
		case STATUS_WARNING:  out << "WARNING";  break;
		default:                                 break;
	}
	return out;
}


// Print method for an OperationDetail object.
std::ostream& operator<<(std::ostream& out, const OperationDetail& od)
{
	out << strip_markup(od.get_description());
	Glib::ustring elapsed = od.get_elapsed_time();
	if (! elapsed.empty())
		out << "    " << elapsed;
	if (od.get_status() != STATUS_NONE)
		out << "  (" << od.get_status() << ")";
	out << "\n";

	for (size_t i = 0; i < od.get_children().size(); i++)
	{
		out << *od.get_children()[i];
	}
	return out;
}


}  // namespace GParted
