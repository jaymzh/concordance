#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# Free Software Foundation; either version 2 of the License, or
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
# (C) Copyright Phil Dibowitz 2007
# 

PREFIX?=/usr
BIN=bin
BINDIR=$(PREFIX)/$(BIN)
INSTALL?=/usr/bin/install
STRIP?=/usr/bin/strip
MAN?=$(PREFIX)/share/man

CXX?= g++
CXXFLAGS?= -g -Wall -O2
CXXFILES?= consnoop.cpp
LIBS?=
CPPFLAGS?=

all: consnoop

consnoop: $(CXXFILES)
	$(CXX) $(CXXFLAGS) $(CXXFILES) -o consnoop $(CPPFLAGS)

install: consnoop
	$(INSTALL) -m 0755 consnoop $(BINDIR)
	

clean:
	/bin/rm -f consnoop

