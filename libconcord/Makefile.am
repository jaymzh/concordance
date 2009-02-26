# libconcord
lib_LTLIBRARIES = libconcord.la
libconcord_la_SOURCES = remote.cpp remote_z.cpp libconcord.cpp binaryfile.cpp \
	web.cpp libusb/libusbhid.cpp usblan.cpp binaryfile.h hid.h protocol_z.h \
	 remote_info.h web.h protocol.h remote.h usblan.h xml_headers.h
include_HEADERS = libconcord.h
libconcord_la_LDFLAGS = -version-info 0 -lusb
libconcord_la_LADD = usb

# udev and friends support
udev:
	./gen_udev_support -u

install_udev_generic:
	$(install_sh_DATA) libconcord.rules /etc/udev/
	(cd /etc/udev/rules.d && ln -f -s ../libconcord.rules 99_libconcord.rules)

install_udev: udev install_udev_generic

policykit:
	./gen_udev_support -p

install_policykit: policykit install_udev_generic 
	$(install_sh_DATA) libconcord.fdi $(datarootdir)/hal/fdi/policy/10osvendor/99-libconcord.fdi
	$(install_sh_DATA) org.freedesktop.hal.device-access.libconcord.policy $(datarootdir)/PolicyKit/policy/

consolekit:
	./gen_udev_support -c

install_consolekit: consolekit install_udev_generic
	$(install_sh_DATA) libconcord.perms $(datarootdir)/hal/fdi/policy/10osvendor/99-libconcord.perms
