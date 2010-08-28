AC_INIT([libconcord], [0.23], [bug-automake@gnu.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AC_PROG_CXX
a=1
AC_CHECK_HEADER(usb.h, [], [a=0])
AC_CHECK_LIB(usb, usb_init, [], [a=0])
if test $a == 0
then
	AC_MSG_ERROR([Error, libusb is missing!])
fi
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT
