#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

PREFIX?=/usr
BIN=bin
BINDIR=$(PREFIX)/$(BIN)
INSTALL?=/usr/bin/install

CC?= cc
CFLAGS?= -g -Wall -O2
CFILES?= ssir_dump.c

all: ssir_dump

ssir_dump: $(CFILES)
	$(CC) $(CFLAGS) $(CFILES) -o ssir_dump

install: ssir_dump
	$(INSTALL) -m 0755 ssir_dump $(BINDIR)

clean:
	/bin/rm -f ssir_dump
