ACLOCAL_AMFLAGS = -I m4
bin_PROGRAMS = concordance
concordance_SOURCES = concordance.c
concordance_LDADD = ../libconcord/libconcord.la
# -Wall just makes good sense
concordance_CFLAGS = -Wall -I../libconcord
man1_MANS = concordance.1

win32-installer:
	makensis win/concordance.nsi
