AC_INIT([concordance], [1.5], [concordance-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_PROG_CC_C_O
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL
AC_PROG_CXX
AC_CANONICAL_HOST
LIBCONCORD_VERSION="6"
case $host_os in
  darwin*)
    LIBCONCORD_LDFLAGS="-lconcord.$LIBCONCORD_VERSION"
    ;;
  mingw*)
    LIBCONCORD_LDFLAGS="-l:libconcord-$LIBCONCORD_VERSION.dll -lwsock32"
    ;;
  *)
    LIBCONCORD_LDFLAGS="-l:libconcord.so.$LIBCONCORD_VERSION"
    ;;
esac
AC_SUBST([LIBCONCORD_LDFLAGS], [$LIBCONCORD_LDFLAGS])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
])
AC_OUTPUT
