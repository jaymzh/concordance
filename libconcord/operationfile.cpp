/*
 * vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
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
 * (C) Copyright Phil Dibowitz 2010
 */

#include "operationfile.h"

#include <stdlib.h>
#include <string.h>
#include <zip.h>
#include <string>

#include "libconcord.h"
#include "lc_internal.h"
#include "binaryfile.h"
#include "web.h"
#include "remote.h"

int find_config_binary(uint8_t *config, uint32_t config_size,
    uint8_t **binary_ptr, uint32_t *binary_size)
{
    int err;

    err = GetTag("/INFORMATION", config, config_size, *binary_ptr);
    if (err == -1)
        return LC_ERROR;

    *binary_ptr += 2;
    *binary_size = config_size - (*binary_ptr - config);

    // Limit tag searches to XML portion
    config_size -= *binary_size;

    string binary_tag_size_s;
    uint8_t *n = 0;
    err = GetTag("BINARYDATASIZE", config, config_size, n, &binary_tag_size_s);
    if (err == -1)
        return LC_ERROR;
        uint32_t binary_tag_size = (uint32_t)atoi(binary_tag_size_s.c_str());

    debug("actual data size %i", *binary_size);
    debug("reported data size %i", binary_tag_size);

    if (*binary_size != binary_tag_size) {
        debug("Config data size mismatch");
        return LC_ERROR;
    }

    string s;
    err = GetTag("CHECKSUM", config, config_size, n, &s);
    if (err != 0)
        return err;
    const uint8_t checksum = atoi(s.c_str());

    // Calculate checksum
    uint32_t u = *binary_size;
    uint8_t calc_checksum = 0x69;
    uint8_t *pc = *binary_ptr;
    while (u--)
        calc_checksum ^= *pc++;

    debug("reported checksum %i %02x", checksum, checksum);
    debug("actual checksum %i %02x", calc_checksum, calc_checksum);

    if (calc_checksum != checksum) {
        debug("Config checksum mismatch");
        return LC_ERROR;
    }

    return 0;
}

int OperationFile::ReadZipFile(char *file_name)
{
    struct zip *zip = zip_open(file_name, 0, NULL);
    if (!zip) {
        return LC_ERROR;
    }

    struct zip_stat stat;
    zip_uint64_t num_entries = zip_get_num_entries(zip, 0);
    for (zip_uint64_t i = 0; i < num_entries; i++) {
        zip_stat_index(zip, i, 0, &stat);
        struct zip_file *file = zip_fopen(zip, stat.name, 0);
        if ((strcmp(stat.name, "Data.xml") == 0) ||
            (strcmp(stat.name, "Description.xml") == 0)) {
            debug("Internal file is %s", stat.name);
            debug("Size is %d", stat.size);
            xml_size = stat.size;
            xml = new uint8_t[xml_size];
            zip_fread(file, xml, xml_size);
            debug("xml is %p, and xmlsize is %d", xml, xml_size);
        } else {
            data_size = stat.size;
            data = new uint8_t[data_size];
            data_alloc = true;
            zip_fread(file, data, data_size);
            debug("data_size is %d", data_size);
        }
        zip_fclose(file);
    }
    zip_close(zip);
    return 0;
}

int OperationFile::ReadPlainFile(char *file_name)
{
    /* Read file */
    binaryinfile file;

    if (file.open(file_name) != 0) {
        debug("Failed to open %s", file_name);
        return LC_ERROR_OS_FILE;
    }

    debug("file opened");
    uint32_t size = file.getlength();
    uint8_t *out = new uint8_t[size];
    file.read(out, size);

    if (file.close() != 0) {
        debug("Failed to close %s\n", file_name);
        return LC_ERROR_OS_FILE;
    }

    debug("finding binary bit...");
    /* Find the config part */
    find_config_binary(out, size, &data, &data_size);

    xml = out;
    xml_size = size - data_size;

    return 0;
}

OperationFile::OperationFile()
{
    data_size = xml_size = 0;
    data = xml = NULL;
    data_alloc = false;
}

OperationFile::~OperationFile()
{
    /* 
     * In certain places, the data pointer is used to point to an area
     * inside of the xml memory.  In those cases, we don't want to delete
     * it.  Use the data_alloc variable to keep track of when we've actually
     * allocated unique memory to it, and in those cases, delete it.
     */
    if (data && data_alloc)
        delete data;
    if (xml)
        delete xml;
}

int _convert_to_binary(string hex, uint8_t *&ptr)
{
    size_t size = hex.length();
    for (size_t i = 0; i < size; i += 2) {
        char tmp[6];
        sprintf(tmp, "0x%s ", hex.substr(i, 2).c_str());
        ptr[0] = (uint8_t)strtoul(tmp, NULL, 16);
        ptr++;
    }
    return 0;
}

int OperationFile::_ExtractFirmwareBinary()
{
    debug("extracting firmware binary");
    uint32_t o_size = FIRMWARE_MAX_SIZE;
    data = new uint8_t[o_size];
    data_alloc = true;
    uint8_t *o = data;

    uint8_t *x = xml;
    uint8_t *x_new;
    uint32_t x_size = xml_size;

    /*
     * Some remotes (e.g., Arch 7) contain multiple phases in their
     * firmware update files.  In that case, extract only the first phase.
     */
    if (GetTag("PHASE", x, x_size, x_new) == 0) {
        debug("multi-phase firmware found, extracting 1st phase");
        x_size = x_size - (x_new - x);
        x = x_new;
        uint8_t *phase_end;
        GetTag("/PHASE", x, x_size, phase_end);
        x_size = phase_end - x;
    }

    string hex;
    while (GetTag("DATA", x, x_size, x_new, &hex) == 0) {
        uint32_t hex_size = hex.length() / 2;
        if (hex_size > o_size) {
            return LC_ERROR;
        }

        _convert_to_binary(hex, o);

        x_size = x_size - (x_new - x);
        x = x_new;
        o_size -= hex_size;
    }

    data_size = o - data;
    debug("acquired firmware binary");

    return 0;
}

int OperationFile::ReadAndParseOpFile(char *file_name, int *type)
{
    debug("In RAPOF");
    int err;

    if (file_name == NULL) {
        debug("Empty file_name");
        return LC_ERROR_OS_FILE;
    }

    bool is_zip = false;
    if (!ReadZipFile(file_name)) {
        debug("Is zip");
        is_zip = true;
    }

    if (!is_zip) {
        if ((err = ReadPlainFile(file_name)))
            return LC_ERROR_READ;
    }

    /* Determine the file type */
    uint8_t *start_info_ptr, *end_info_ptr;
    bool has_binary = false;
    if (data && data_size) {
        debug("Has binary!");
        has_binary = true;
    }

    /*
     * Validate this is a remotely sane XML file
     */
    debug("determining type...");
    if (is_zip) {
        start_info_ptr = xml;
        end_info_ptr = xml + xml_size;
    } else {
        err = GetTag("INFORMATION", xml, xml_size, start_info_ptr);
        debug("err is %d", err);
        if (err == -1) {
            return LC_ERROR;
        }

        err = GetTag("/INFORMATION", xml, xml_size, end_info_ptr);
        if (err == -1) {
            return LC_ERROR;
        }
    }
    debug("start/end pointers populated");

    /*
     * Search for tag only in "connectivity test" files
     */
    bool found_get_zaps_only = false;
    uint8_t *tmp_data = xml;
    uint32_t tmp_size = xml_size;
    while (1) {
        uint8_t *tag_ptr;
        string tag_s;
        err = GetTag("KEY", tmp_data, tmp_size, tag_ptr, &tag_s);
        if (err == -1) {
            debug("not a connectivity test file");
            break;
        }
        if (!stricmp(tag_s.c_str(), "GETZAPSONLY")) {
            found_get_zaps_only = true;
            break;
        }
        tmp_data = tag_ptr + tag_s.length();
        tmp_size = end_info_ptr - tmp_data;
    }

    /*
     * Search for tag only in "firmware" files
     */
    bool found_firmware = false;
    tmp_data = xml;
    tmp_size = xml_size;
    while (1) {
        uint8_t *tag_ptr;
        string tag_s;
        err = GetTag("TYPE", tmp_data, tmp_size, tag_ptr, &tag_s);
        if (err == -1) {
            debug("not a firmware file");
            break;
        }
        if (!stricmp(tag_s.c_str(), "Firmware_Main")) {
            debug("IS a firmware file");
            found_firmware = true;
            break;
        }
        tmp_data = tag_ptr + tag_s.length();
        tmp_size = end_info_ptr - tmp_data;
    }

    if (found_firmware)
        _ExtractFirmwareBinary();

    /*
     * Search for tag only in "IR learning files.
     */
    uint8_t *tag_ptr;
    err = GetTag("CHECKKEYS", xml, xml_size, tag_ptr);
    bool found_learn_ir = (err != -1);

    debug("zaps: %d, binary: %d, firmware: %d, ir: %d",
        found_get_zaps_only, has_binary, found_firmware,
        found_learn_ir);

    /*
     * Check tag search results for consistency, and deduce the file type
     */
    if (found_get_zaps_only && !has_binary && !found_firmware &&
        !found_learn_ir) {
        debug("returning connect file");
        *type = LC_FILE_TYPE_CONNECTIVITY;
        return 0;
    }
    if (!found_get_zaps_only && has_binary && data_size >= 16
        && !found_firmware && !found_learn_ir) {
        debug("returning conf file");
        *type = LC_FILE_TYPE_CONFIGURATION;
        return 0;
    }
    if (!found_get_zaps_only && found_firmware && !found_learn_ir) {
        debug("returning firmware file");
        *type = LC_FILE_TYPE_FIRMWARE;
        return 0;
    }
    if (!found_get_zaps_only && !found_firmware &&
        found_learn_ir) {
        debug("returning IR file");
        *type = LC_FILE_TYPE_LEARN_IR;
        return 0;
    }
    debug("returning error");

    /*
     * Findings didn't match a single file type; indicate a problem
     */
    return LC_ERROR;
}

