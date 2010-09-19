/*
 * vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=8 noexpandtab:
 *
 * $Id$
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * (C) Copyright Kevin Timmerman 2007
 */

#ifndef OPERATIONFILE_H
#define OPERATIONFILE_H

#include "lc_internal.h"

class OperationFile {
private:
	uint8_t *data;
	uint32_t data_size;
	uint8_t *xml;
	uint32_t xml_size;
	int ReadPlainFile(char *file_name);
	int ReadZipFile(char *file_name);
	int _ExtractFirmwareBinary();

public:
	OperationFile();
	~OperationFile();
	uint32_t GetDataSize() {return data_size;}
	uint32_t GetXmlSize() {return xml_size;}
	uint8_t* GetData() {return data;}
	uint8_t* GetXml() {return xml;}
	int ReadAndParseOpFile(char *file_name, int *type);
};

#endif /* OPERATIONFILE_H */
