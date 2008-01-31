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
CXXFILES?= harmony.cpp remote.cpp remote_z.cpp \
	binaryfile.cpp web.cpp \
	libusb/libusbhid.cpp usblan.cpp
HFILES?= binaryfile.h hid.h protocol_z.h remote_info.h web.h\
	harmony.h protocol.h remote.h usblan.h xml_headers.h
LIBS?= -lusb
CPPFLAGS?=

all: harmony

harmony: $(CXXFILES) $(HFILES)
	$(CXX) $(CXXFLAGS) $(CXXFILES) -o harmony $(LIBS) $(CPPFLAGS)
	@touch harmony

strip: harmony
	$(STRIP) harmony
	@touch strip

install:
	$(INSTALL) -D --mode=755 harmony $(BINDIR)/harmony
	$(INSTALL) -D --mode=644 harmony.1 $(MAN)/man1/harmony.1

clean:
	/bin/rm -f harmony
	/bin/rm -f strip
	/bin/rm -f snoop/snoop

uninstall:
	/bin/rm -f $(BIN)/harmony
	/bin/rm -f $(MAN)/man1/harmony.1

