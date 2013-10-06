/*
 * vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright 2007 Kevin Timmerman
 * Copyright 2008 Phil Dibowitz
 */

#ifndef LC_INTERNAL_H
#define LC_INTERNAL_H

#ifdef _WIN32 /* BEGIN WINDOWS SECTION */

#include <windows.h>

#else /* END WINDOWS SECTION, BEGIN NON-WINDOWS SECTION */

#define stricmp strcasecmp
#define strnicmp strncasecmp

#endif /* END NON-WINDOWS SECTION */

/* BEGIN GLOBAL SECTION */

#ifdef _DEBUG
#define debug(FMT,...) fprintf(stderr, "DEBUG (%s): "FMT"\n", __FUNCTION__,\
         ##__VA_ARGS__);
#else
#define debug(FMT,...)
#endif

#include <string>
#include <stdint.h>
using namespace std;

void report_net_error(const char *msg);
/* NOTE: 1 for yes, 0 for no, unlike some other functions. */
int is_z_remote();
int is_usbnet_remote();
int is_mh_remote();

#endif // ifndef LC_INTERNAL_H
