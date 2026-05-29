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


#include "common.h"
#include "OperationDetail.h"
#include "Utils.h"
#include "gtest/gtest.h"

#include <gtkmm/main.h>
#include <stdio.h>


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


TEST(OperationDetailTest, CommandExecuteTrue)
{
	// Test command returning exit status 0 (AKA success).
	OperationDetail od("CommandExecuteTrue");
	int exit_status = od.execute_command("true");
	EXPECT_EQ(exit_status, 0);
}


TEST(OperationDetailTest, CommandExecuteFalse)
{
	// Test command returning non-zero exit status (AKA failure).
	OperationDetail od("CommandExecuteFalse");
	int exit_status = od.execute_command("false");
	EXPECT_NE(exit_status, 0);
}


TEST(OperationDetailTest, CommandExecuteExit2)
{
	// Test command returning exit status 2.
	OperationDetail od("CommandExecuteExit2");
	int exit_status = od.execute_command("sh -c 'exit 2'");
	EXPECT_EQ(exit_status, 2);
}


TEST(OperationDetailTest, CommandExecuteProvideStdin)
{
	// Test passing data on stdin for command to read.
	OperationDetail od("CommandExecuteProvideStdin");
	int exit_status = od.execute_command("cat", "This is on standard input\n");
	EXPECT_EQ(exit_status, 0);
	EXPECT_STREQ(od.get_command_output().c_str(), "This is on standard input\n");
	EXPECT_STREQ(od.get_command_error().c_str(), "");
	EXPECT_EQ(od.get_last_child().get_children().size(), 2UL);
	EXPECT_STREQ(od.get_last_child().get_children()[0]->get_description().c_str(),
	             "<tt>This is on standard input\n</tt>");
	EXPECT_STREQ(od.get_last_child().get_children()[1]->get_description().c_str(),
	             "<tt></tt>");
}


TEST(OperationDetailTest, CommandExecuteCaptureStdout)
{
	// Test capturing command writing to stdout.
	OperationDetail od("CommandExecuteCaptureStdout");
	int exit_status = od.execute_command("echo 'This is on standard output'");
	EXPECT_EQ(exit_status, 0);
	EXPECT_STREQ(od.get_command_output().c_str(), "This is on standard output\n");
	EXPECT_STREQ(od.get_command_error().c_str(), "");
	EXPECT_EQ(od.get_last_child().get_children().size(), 2UL);
	EXPECT_STREQ(od.get_last_child().get_children()[0]->get_description().c_str(),
	             "<tt>This is on standard output\n</tt>");
	EXPECT_STREQ(od.get_last_child().get_children()[1]->get_description().c_str(),
	             "<tt></tt>");
}


TEST(OperationDetailTest, CommandExecuteCaptureStderr)
{
	// Test capturing command writing to stderr.
	OperationDetail od("CommandExecuteCaptureStderr");
	int exit_status = od.execute_command("sh -c \"echo 'This is on standard error' 1>&2\"");
	EXPECT_EQ(exit_status, 0);
	EXPECT_STREQ(od.get_command_output().c_str(), "");
	EXPECT_STREQ(od.get_command_error().c_str(), "This is on standard error\n");
	EXPECT_EQ(od.get_last_child().get_children().size(), 2UL);
	EXPECT_STREQ(od.get_last_child().get_children()[0]->get_description().c_str(),
	             "<tt></tt>");
	EXPECT_STREQ(od.get_last_child().get_children()[1]->get_description().c_str(),
	             "<tt>This is on standard error\n</tt>");
}


TEST(OperationDetailTest, CommandExecuteCheckTrue)
{
	// Test checked command returning exit status 0.
	OperationDetail od("CommandExecuteCheckTrue");
	int exit_status = od.execute_command("true", EXEC_CHECK_STATUS);
	EXPECT_EQ(exit_status, 0);
	EXPECT_EQ(od.get_last_child().get_status(), STATUS_SUCCESS);
}


TEST(OperationDetailTest, CommandExecuteCheckFalse)
{
	// Test check command returning non-zero exit status.
	OperationDetail od("CommandExecuteCheckFalse");
	int exit_status = od.execute_command("false", EXEC_CHECK_STATUS);
	EXPECT_NE(exit_status, 0);
	EXPECT_EQ(od.get_last_child().get_status(), STATUS_ERROR);
}


}  // namespace GParted


// Custom Google Test main().
// Reference:
// *   GoogleTest Primer, Writing the main() function
//     https://google.github.io/googletest/primer.html#writing-the-main-function
int main(int argc, char** argv)
{
	printf("Running main() from %s\n", __FILE__);
	GParted::ensure_x11_display(argc, argv);

	// Initialise Gtk::Main to successfully use OperationDetail::execute_command().
	// Must be before InitGoogleTest().  (Don't need to initialise
	// GParted_Core::mainthread as nothing in OperationDetail uses it).
	Gtk::Main gtk_main = Gtk::Main();

	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
