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


#include "common.h"

#include <iostream>
#include <sstream>
#include <string>
#include <iomanip>
#include <stddef.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


namespace GParted
{


// Format up to BinaryStringChunkSize (16) bytes of binary data ready for printing as:
//      Hex offset     ASCII text          Hex bytes
//     "0x000000000  \"ABCDEFGHabcdefgh\"  41 42 43 44 45 46 47 48 61 62 63 64 65 66 67 68"
std::string binary_string_to_print(size_t offset, const char* s, size_t len)
{
	std::ostringstream result;

	result << "0x";
	result.fill('0');
	result << std::setw(8) << std::hex << std::uppercase << offset << "  \"";

	size_t i;
	for (i = 0; i < BinaryStringChunkSize && i < len; i++)
		result.put((isprint(s[i])) ? s[i] : '.');
	result.put('\"');

	if (len > 0)
	{
		for (; i < BinaryStringChunkSize; i++)
			result.put(' ');
		result.put(' ');

		for (i = 0 ; i < BinaryStringChunkSize && i < len; i++)
			result << " "
			       << std::setw(2) << std::hex << std::uppercase
			       << (unsigned int)(unsigned char)s[i];
	}

	return result.str();
}


// Re-execute current executable using xvfb-run so that it provides a virtual X11 display.
static void exec_using_xvfb_run(int argc, char** argv)
{
	// argc+2 = Space for "xvfb-run" command, existing argc strings plus nullptr.
	size_t size = sizeof(char*) * (argc+2);
	char** new_argv = (char**)malloc(size);
	if (new_argv == nullptr)
	{
		fprintf(stderr, "Failed to allocate %lu bytes of memory.  errno=%d,%s\n",
			(unsigned long)size, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	new_argv[0] = strdup("xvfb-run");
	if (new_argv[0] == nullptr)
	{
		fprintf(stderr, "Failed to allocate %lu bytes of memory.  errno=%d,%s\n",
		        (unsigned long)strlen(new_argv[0])+1, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	// Copy argv pointers including final nullptr.
	for (size_t i = 0; i <= (unsigned)argc; i++)
		new_argv[i+1] = argv[i];

	execvp(new_argv[0], new_argv);
	fprintf(stderr, "Failed to execute '%s %s ...'.  errno=%d,%s\n", new_argv[0], new_argv[1],
		errno, strerror(errno));
	exit(EXIT_FAILURE);
}


// Ensure there is an X11 display, providing a virtual one if needed.
void ensure_x11_display(int argc, char** argv)
{
	const char* display = getenv("DISPLAY");
	if (display == nullptr)
	{
		printf("DISPLAY environment variable unset.  Executing 'xvfb-run %s ...'\n", argv[0]);
		exec_using_xvfb_run(argc, argv);
	}
	printf("DISPLAY=\"%s\"\n", display);

}


}  // namespace GParted
