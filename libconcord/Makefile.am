# libconcord
ACLOCAL_AMFLAGS= -I m4
lib_LTLIBRARIES = libconcord.la
libconcord_la_SOURCES = remote.cpp remote_z.cpp libconcord.cpp binaryfile.cpp \
	web.cpp libusb/libusbhid.cpp usblan.cpp binaryfile.h hid.h protocol_z.h \
	 remote_info.h web.h protocol.h remote.h usblan.h xml_headers.h
include_HEADERS = libconcord.h
libconcord_la_LDFLAGS = -version-info 2:0:0 -lusb
libconcord_la_LADD = usb

# udev and friends support
udev:
	./gen_udev_support -u

install_udev_generic:
	$(MKDIR_P) $(DESTDIR)$(sysconfdir)/udev/rules.d
	$(install_sh_DATA) libconcord.rules \
		$(DESTDIR)$(sysconfdir)/udev/rules.d/99-libconcord.rules

install_udev: udev install_udev_generic

policykit:
	./gen_udev_support -p

install_policykit: policykit install_udev_generic
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/PolicyKit/policy/
	$(install_sh_DATA) libconcord.fdi \
		$(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/99-libconcord.fdi
	$(install_sh_DATA) org.freedesktop.hal.device-access.libconcord.policy \
		$(DESTDIR)$(datarootdir)/PolicyKit/policy/

consolekit:
	./gen_udev_support -c

install_consolekit: consolekit install_udev_generic
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/
	$(install_sh_DATA) libconcord.perms \
		$(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/99-libconcord.perms

