AC_INIT([libconcord], [1.0], [bug-automake@gnu.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_PROG_LIBTOOL
AC_PROG_CXX
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
