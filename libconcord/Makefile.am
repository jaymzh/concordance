# libharmony
lib_LTLIBRARIES = libharmony.la
libharmony_la_SOURCES = remote.cpp remote_z.cpp libharmony.cpp binaryfile.cpp \
	web.cpp libusb/libusbhid.cpp usblan.cpp binaryfile.h hid.h protocol_z.h \
	 remote_info.h web.h protocol.h remote.h usblan.h xml_headers.h
include_HEADERS = libharmony.h
libharmony_la_LDFLAGS = -version-info 0 -lusb
libharmony_la_LADD = usb

bin_PROGRAMS = harmony
harmony_SOURCES = harmony.cpp
harmony_LDADD = libharmony.la
