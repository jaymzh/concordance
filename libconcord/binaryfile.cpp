/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  (C) Copyright Kevin Timmerman 2007
 *  (C) Copyright Phil Dibowitz 2007
 */

// C++ iostreams will eat EOF chars, even when ios::binary is used with read() and write()


#include "harmony.h"

#include <stdio.h>

#ifdef WIN32
#include <io.h>
#else
#include <sys/stat.h>
#endif

#include "binaryfile.h"


binaryfile::binaryfile()
{
	m_f = NULL;
}

binaryfile::~binaryfile()
{
	close();
}

int binaryfile::close(void)
{
	if (m_f) {
		const int i = fclose(m_f);
		m_f = NULL;
		return i;
	} else {
		return 0;
	}
}

binaryoutfile::binaryoutfile()
{
	::binaryfile();
}

int binaryoutfile::open(const char *path)
{
	m_f = fopen(path, "wb");
	return m_f ? 0 : 1;
}

size_t binaryoutfile::write(const uint8_t *b, uint32_t len)
{
	return fwrite(b, len, 1, m_f);
}

size_t binaryoutfile::write(const char *c)
{
	return fwrite(c, strlen(c), 1, m_f);
}


binaryinfile::binaryinfile()
{
	::binaryfile();
}

int binaryinfile::open(const char *path)
{
	m_f=fopen(path, "rb");
	return m_f ? 0 : 1;
}

unsigned int binaryinfile::getlength(void)
{
#ifdef WIN32
	return filelength(fileno(m_f));
#else
	struct stat fs;
	fstat(fileno(m_f), &fs);
	return fs.st_size;
#endif
}

size_t binaryinfile::read(uint8_t *b, uint32_t len)
{
	return fread(b, len, 1, m_f);
}

