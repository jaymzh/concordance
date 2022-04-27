# libconcord
ACLOCAL_AMFLAGS = -I m4
lib_LTLIBRARIES = libconcord.la
libconcord_la_SOURCES = remote.cpp remote_z.cpp libconcord.cpp binaryfile.cpp \
	web.cpp usblan.cpp binaryfile.h hid.h protocol_z.h \
	remote_info.h web.h protocol.h remote.h usblan.h xml_headers.h \
	operationfile.cpp remote_mh.cpp libusbhid.cpp libhidapi.cpp
include_HEADERS = libconcord.h
libconcord_la_CPPFLAGS = -Wall
libconcord_la_LDFLAGS = -version-info 6:0:0 $(LIBCONCORD_LDFLAGS) -lzip -lcurl
libconcord_la_CXXFLAGS = $(ZIP_CFLAGS)
UDEVROOT ?= /
UDEVLIBDIR ?= $(UDEVROOT)/lib

# udev and friends support
udev:
	./gen_udev_support -u

old_udev:
	./gen_udev_support -o

generic_udev:
	./gen_udev_support -g

install_udev_common:
	$(MKDIR_P) $(DESTDIR)$(UDEVLIBDIR)/udev/rules.d
	$(install_sh_DATA) libconcord.rules \
		$(DESTDIR)$(UDEVLIBDIR)/udev/rules.d/60-libconcord.rules
	$(install_sh_DATA) libconcord-usbnet.rules \
		$(DESTDIR)$(UDEVLIBDIR)/udev/rules.d/80-libconcord-usbnet.rules
	$(install_sh_SCRIPT) start_concordance_dhcpd.sh \
		$(DESTDIR)$(UDEVLIBDIR)/udev/
	$(install_sh_SCRIPT) start_concordance_dhcpd_wrapper.sh \
		$(DESTDIR)$(UDEVLIBDIR)/udev/

install_udev: udev install_udev_common
install_old_udev: old_udev install_udev_common
install_generic_udev: generic_udev install_udev_common

policykit:
	./gen_udev_support -p

install_policykit: policykit install_udev_common
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/PolicyKit/policy/
	$(install_sh_DATA) libconcord.fdi \
		$(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/99-libconcord.fdi
	$(install_sh_DATA) org.freedesktop.hal.device-access.libconcord.policy \
		$(DESTDIR)$(datarootdir)/PolicyKit/policy/

consolekit:
	./gen_udev_support -c

install_consolekit: consolekit install_udev_common
	$(MKDIR_P) $(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/
	$(install_sh_DATA) libconcord.perms \
		$(DESTDIR)$(datarootdir)/hal/fdi/policy/10osvendor/99-libconcord.perms

# MIME stuff

newmimedir = $(datadir)/mime/packages
newmime_DATA = libconcord.xml

EXTRA_DIST = \
	$(mime_DATA) \
	$(newmime_DATA)

install-data-hook:
if HAVE_NEW_MIME
	if test -z "$(DESTDIR)"; then \
		$(UPDATE_MIME_DATABASE) "$(datadir)/mime"; \
	fi
endif

uninstall-hook:
if HAVE_NEW_MIME
	if test -z "$(DESTDIR)"; then \
		$(UPDATE_MIME_DATABASE) "$(datadir)/mime"; \
	fi
endif
