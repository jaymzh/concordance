# libconcord
lib_LTLIBRARIES = libconcord.la
libconcord_la_SOURCES = remote.cpp remote_z.cpp libconcord.cpp binaryfile.cpp \
	web.cpp libusb/libusbhid.cpp usblan.cpp binaryfile.h hid.h protocol_z.h \
	 remote_info.h web.h protocol.h remote.h usblan.h xml_headers.h
include_HEADERS = libconcord.h
libconcord_la_LDFLAGS = -version-info 0 -lusb
libconcord_la_LADD = usb

