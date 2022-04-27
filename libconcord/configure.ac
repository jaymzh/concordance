AC_INIT([libconcord], [1.5], [concordance-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_CANONICAL_HOST
#
# allow the user to disable the mime database update
#
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)
HAVE_NEW_MIME=no
AC_ARG_ENABLE(
  mime-update,
	AS_HELP_STRING([--disable-mime-update],
	               [Disable the MIME database update]),
	[disable_mime=yes],
	[disable_mime=no])
if test "x$disable_mime" = "xno"; then
	if test "x$UPDATE_MIME_DATABASE" != "xno"; then
     HAVE_NEW_MIME=yes
     AC_DEFINE(HAVE_NEW_MIME,1,[update mime database?])
	fi
fi
AM_CONDITIONAL(HAVE_NEW_MIME, test x$HAVE_NEW_MIME = xyes)
#
# allow user to specify libusb on Linux
#
AC_ARG_ENABLE(
  force-libusb-on-linux,
  AS_HELP_STRING([--enable-force-libusb-on-linux],
                 [Use libusb on Linux]),
  [force_libusb_on_linux=$enableval],
  [force_libusb_on_linux=no])
case $host_os in
  linux*)
    if test "$force_libusb_on_linux" = "yes"; then
      USBLIB="usb"
      AC_DEFINE([WANT_LIBUSB], [1], [Want libusb])
    else
      USBLIB="hidapi-libusb"
      AC_DEFINE([WANT_HIDAPI], [1], [Want hidapi])
    fi
    LIBCONCORD_LDFLAGS="-l$USBLIB"
    ;;
  darwin*)
    USBLIB="hidapi"
    AC_DEFINE([WANT_HIDAPI], [1], [Want hidapi])
    LIBCONCORD_LDFLAGS="-l$USBLIB"
    ;;
  mingw*)
    USBLIB="hidapi"
    AC_DEFINE([WANT_HIDAPI], [1], [Want hidapi])
    LIBCONCORD_LDFLAGS="-l$USBLIB -lwsock32 -no-undefined"
    ;;
  k*bsd*-gnu)
    USBLIB="hidapi"
    AC_DEFINE([WANT_HIDAPI], [1], [Want hidapi])
    LIBCONCORD_LDFLAGS="-l$USBLIB"
    ;;
  gnu*)
    USBLIB="usb"
    AC_DEFINE([WANT_LIBUSB], [1], [Want libusb])
    LIBCONCORD_LDFLAGS="-l$USBLIB"
    ;;
esac
AC_SUBST([LIBCONCORD_LDFLAGS], [$LIBCONCORD_LDFLAGS])
a=1
if test "$USBLIB" = "usb"; then
  AC_CHECK_HEADER(usb.h, [], [a=0])
  AC_CHECK_LIB(usb, usb_init, [], [a=0])
else
  AC_CHECK_HEADER(hidapi/hidapi.h, [], [a=0])
  AC_CHECK_LIB(${USBLIB}, hid_init, [], [a=0])
fi
if test $a == 0
then
  errorstr="lib$USBLIB is missing!"
  if test "x$force_libusb_on_linux" = "xno"; then
    errorstr="$errorstr
      Note that while we are deprecating the use of libusb on Linux in favor
      of libhidapi, this release still supports it via
      --enable-force-libusb-on-linux"
  fi
  AC_MSG_ERROR([$errorstr])
fi
PKG_CHECK_MODULES([ZIP], [libzip])
PKG_CHECK_MODULES([CURL], [libcurl])
AC_CONFIG_FILES([
    Makefile
])
AC_OUTPUT
if test "$force_libusb_on_linux" = "yes"; then
  AC_MSG_WARN(
    [
      libusb support on linux will be dropped in a future release please
      migrate to libhidapi!])
fi
