bin_PROGRAMS = concordance
concordance_SOURCES = concordance.c
concordance_LDFLAGS = -l:libconcord.so.3
ACLOCAL_AMFLAGS= -I m4
# -Wall just makes good sense
# -ansi and -pednatic errors are needed to ensure we're compatible with
# crappy Microsoft compilers - phil
concordance_CFLAGS = -Wall -ansi -pedantic-errors
man1_MANS = concordance.1

