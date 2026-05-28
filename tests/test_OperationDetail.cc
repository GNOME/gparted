/* Copyright (C) 2026 Mike Fleetwood
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


#include "OperationDetail.h"
#include "Utils.h"
#include "gtest/gtest.h"


namespace GParted
{


// Theoretically formatted time string can be negative and have more that 2 hour digits.
static const char* TIME_PATTERN = "(-?[[:digit:]]{2,}:[[:digit:]]{2}:[[:digit:]]{2})";


TEST(OperationDetailTest, CreateEmpty)
{
	// Test creating an empty OperationDetail using default constructor.
	OperationDetail od;
	EXPECT_STREQ(od.get_description().c_str(), "");
	EXPECT_EQ(od.get_status(), STATUS_NONE);
	EXPECT_STREQ(od.get_treepath().c_str(), "");
	EXPECT_STREQ(od.get_elapsed_time().c_str(), "");
	EXPECT_EQ(od.get_children().size(), 0UL);
}


TEST(OperationDetailTest, CreateWithDescription)
{
	// Test creating an OperationDetail passing description using parameterised
	// constructor.  Status defaults to EXECUTE.
	OperationDetail od("CreateWithDescription");
	EXPECT_STREQ(od.get_description().c_str(), "CreateWithDescription");
	EXPECT_EQ(od.get_status(), STATUS_EXECUTE);
}


TEST(OperationDetailTest, CreateWithDescriptionStatus)
{
	// Test creating an OperationDetail passing description and status using
	// parameterised constructor.
	OperationDetail od("CreateWithDescriptionStatus", STATUS_NONE);
	EXPECT_STREQ(od.get_description().c_str(), "CreateWithDescriptionStatus");
	EXPECT_EQ(od.get_status(), STATUS_NONE);
}


TEST(OperationDetailTest, CreateWithDescriptionStatusFont)
{
	// Test creating an OperationDetail passing description, status and font using
	// parameterised constructor.
	OperationDetail od("CreateWithDescriptionStatusFont", STATUS_ERROR, FONT_NORMAL);
	EXPECT_STREQ(od.get_description().c_str(), "CreateWithDescriptionStatusFont");
	EXPECT_EQ(od.get_status(), STATUS_ERROR);
}


TEST(OperationDetailTest, ElapseTimeStopWithSuccess)
{
	// Test status transition EXECUTE -> SUCCESS records elapsed time.
	OperationDetail od("ElapseTimeStopWithSuccess", STATUS_EXECUTE);
	od.set_status(STATUS_SUCCESS);
	EXPECT_EQ(od.get_status(), STATUS_SUCCESS);
	// Ideally would use:
	//     EXPECT_THAT(od.get_elapsed_time().c_str(), MatchesRegex(TIME_PATTERN));
	// but that requires gMock which is not currently included in GParted.  Instead
	// use Utils::regexp_label() to return the matched time string or zero length
	// string when not matched.
	EXPECT_STRNE(Utils::regexp_label(od.get_elapsed_time(), TIME_PATTERN).c_str(), "");
}


TEST(OperationDetailTest, ElapseTimeStopWithWarning)
{
	// Test status transition EXECUTE -> WARNING records elapsed time.
	OperationDetail od("ElapseTimeStopWithWarning", STATUS_EXECUTE);
	od.set_status(STATUS_WARNING);
	EXPECT_EQ(od.get_status(), STATUS_WARNING);
	EXPECT_STRNE(Utils::regexp_label(od.get_elapsed_time(), TIME_PATTERN).c_str(), "");
}


TEST(OperationDetailTest, ElapseTimeStopWithError)
{
	// Test status transition EXECUTE -> WARNING records elapsed time.
	OperationDetail od("ElapseTimeStopWithError", STATUS_EXECUTE);
	od.set_status(STATUS_ERROR);
	EXPECT_EQ(od.get_status(), STATUS_ERROR);
	EXPECT_STRNE(Utils::regexp_label(od.get_elapsed_time(), TIME_PATTERN).c_str(), "");
}


TEST(OperationDetailTest, Hierarchy)
{
	// Test creating and accessing this OperationDetail tree hierarchy:
	//     Description              Treepath
	//     ----------------------   --------
	//     Hierarchy: grandparent   0
	//     Hierarchy: parent        0:0
	//     Hierarchy: child 0       0:0:0
	//     Hierarchy: child 1       0:0:1
	OperationDetail grandparent("Hierarchy: grandparent", STATUS_NONE);
	grandparent.set_treepath("0");
	grandparent.add_child(OperationDetail("Hierarchy: parent", STATUS_NONE));
	grandparent.get_last_child().add_child(OperationDetail("Hierarchy: child 0", STATUS_NONE));
	OperationDetail& parent = grandparent.get_last_child();
	parent.add_child(OperationDetail("Hierarchy: child 1", STATUS_NONE));

	EXPECT_STREQ(grandparent.get_description().c_str(), "Hierarchy: grandparent");
	EXPECT_STREQ(grandparent.get_treepath().c_str(), "0");

	EXPECT_EQ(grandparent.get_children().size(), 1UL);
	EXPECT_STREQ(grandparent.get_children()[0]->get_description().c_str(), "Hierarchy: parent");
	EXPECT_STREQ(grandparent.get_children()[0]->get_treepath().c_str(), "0:0");
	EXPECT_STREQ(grandparent.get_last_child().get_description().c_str(), "Hierarchy: parent");
	EXPECT_STREQ(grandparent.get_last_child().get_treepath().c_str(), "0:0");
	EXPECT_STREQ(parent.get_description().c_str(), "Hierarchy: parent");
	EXPECT_STREQ(parent.get_treepath().c_str(), "0:0");

	EXPECT_EQ(grandparent.get_last_child().get_children().size(), 2UL);
	EXPECT_STREQ(grandparent.get_last_child().get_children()[0]->get_description().c_str(), "Hierarchy: child 0");
	EXPECT_STREQ(grandparent.get_last_child().get_children()[0]->get_treepath().c_str(), "0:0:0");
	EXPECT_EQ(parent.get_children().size(), 2UL);
	EXPECT_STREQ(parent.get_children()[0]->get_description().c_str(), "Hierarchy: child 0");
	EXPECT_STREQ(parent.get_children()[0]->get_treepath().c_str(), "0:0:0");
	EXPECT_STREQ(grandparent.get_grandchild_cmd_output_description().c_str(), "Hierarchy: child 0");

	EXPECT_STREQ(grandparent.get_last_child().get_children()[1]->get_description().c_str(), "Hierarchy: child 1");
	EXPECT_STREQ(grandparent.get_last_child().get_children()[1]->get_treepath().c_str(), "0:0:1");
	EXPECT_STREQ(grandparent.get_last_child().get_last_child().get_description().c_str(), "Hierarchy: child 1");
	EXPECT_STREQ(grandparent.get_last_child().get_last_child().get_treepath().c_str(), "0:0:1");
	EXPECT_STREQ(parent.get_children()[1]->get_description().c_str(), "Hierarchy: child 1");
	EXPECT_STREQ(parent.get_children()[1]->get_treepath().c_str(), "0:0:1");
	EXPECT_STREQ(parent.get_last_child().get_description().c_str(), "Hierarchy: child 1");
	EXPECT_STREQ(parent.get_last_child().get_treepath().c_str(), "0:0:1");
}


}  // namespace GParted
