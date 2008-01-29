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
 */


class binaryfile {
protected:
	FILE *m_f;
public:
	binaryfile();
	~binaryfile();
	int close(void);
};

class binaryoutfile : public binaryfile {
public:
	binaryoutfile();
	~binaryoutfile() {};
	int open(const char*path);
	size_t write(const uint8_t *b, uint32_t len);
	size_t write(const char *c);
};

class binaryinfile : public binaryfile {
public:
	binaryinfile();
	~binaryinfile() {};
	int open(const char *path);
	unsigned int getlength(void);
	size_t read(uint8_t *b, uint32_t len);
};
