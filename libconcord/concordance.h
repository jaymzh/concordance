/*
    Copyright 2007 Kevin Timmerman

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CONCORDANCE_H
#define CONCORDANCE_H

#ifdef WIN32
#include <windows.h>
#else // not Windows

#define LIBUSB

#define stricmp strcasecmp
#define strnicmp strncasecmp

#endif // end 'not Windows'

#include <string>
using namespace std;

struct options_t {
	bool binary;
	bool verbose;
	bool noweb;
};

void report_net_error(const char *msg);

#endif // ifndef CONCORDANCE_H
