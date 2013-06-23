AC_INIT([libconcord], [1.0], [bug-automake@gnu.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL
AC_PROG_CXX
#
# allow the user to disable the mime database update
#
AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database, no)
HAVE_NEW_MIME=no
AC_ARG_ENABLE(mime-update,
	AS_HELP_STRING([--disable-mime-update],
	[Disable the MIME database update])],
	[disable_mime=yes],
	[disable_mime=no])
if test "x$disable_mime" = "xno"; then
	if test "x$UPDATE_MIME_DATABASE" != "xno"; then
               HAVE_NEW_MIME=yes
               AC_DEFINE(HAVE_NEW_MIME,1,[update mime database?])
	fi
fi
AM_CONDITIONAL(HAVE_NEW_MIME, test x$HAVE_NEW_MIME = xyes)
a=1
AC_CHECK_HEADER(usb.h, [], [a=0])
AC_CHECK_LIB(usb, usb_init, [], [a=0])
if test $a == 0
then
	AC_MSG_ERROR([Error, libusb is missing!])
fi
AC_CHECK_HEADER(zzip/lib.h, [], [a=0])
AC_CHECK_LIB(zzip, zzip_dir_open, [], [a=0])
if test $a == 0
then
	AC_MSG_ERROR([Error, libzzip is missing!])
fi
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT
