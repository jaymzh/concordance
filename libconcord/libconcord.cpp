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
 * (C) Copyright Phil Dibowitz 2007
 * (C) Copyright Kevin Timmerman 2007
 */

/*
 * This file is entry points into libconcord.
 *   - phil    Sat Aug 18 22:49:48 PDT 2007
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "libconcord.h"
#include "lc_internal.h"
#include "remote.h"
#include "binaryfile.h"
#include "hid.h"
#include "usblan.h"
#include "web.h"
#include "protocol.h"
#include "time.h"
#include <errno.h>

#define ZWAVE_HID_PID_MIN 0xC112
#define ZWAVE_HID_PID_MAX 0xC115

#define FIRMWARE_MAX_SIZE 64*1024

static class CRemoteBase *rmt;
static struct TRemoteInfo ri;
static struct THIDINFO hid_info;
static struct THarmonyTime rtime;

/*
 * BEGIN ACCESSORS
 */
const char *get_mfg()
{
	return ri.model->mfg;
}

const char *get_model()
{
	return ri.model->model;
}

const char *get_codename()
{
	return (ri.model->code_name) ? ri.model->code_name : (char *)"";
}

int get_skin()
{
	return ri.skin;
}

int get_fw_ver_maj()
{
	return ri.fw_ver_major;
}

int get_fw_ver_min()
{
	return ri.fw_ver_minor;
}

int get_fw_type()
{
	return ri.fw_type;
}

int get_hw_ver_maj()
{
	return ri.hw_ver_major;
}

int get_hw_ver_min()
{
	return ri.hw_ver_minor;
}

int get_flash_size()
{
	return ri.flash->size;
}

int get_flash_mfg()
{
	return ri.flash_mfg;
}

int get_flash_id()
{
	return ri.flash_id;
}

const char *get_flash_part_num()
{
	return ri.flash->part;
}

int get_arch()
{
	return ri.architecture;
}

int get_proto()
{
	return ri.protocol;
}

const char *get_hid_mfg_str()
{
	return hid_info.mfg.c_str();
}

const char *get_hid_prod_str()
{
	return hid_info.prod.c_str();
}

int get_hid_irl()
{
	return hid_info.irl;
}

int get_hid_orl()
{
	return hid_info.orl;
}

int get_hid_frl()
{
	return hid_info.frl;
}

int get_usb_vid()
{
	return hid_info.vid;
}

int get_usb_pid()
{
	return hid_info.pid;
}

int get_usb_bcd()
{
	return hid_info.ver;
}

char *get_serial(int p)
{
	switch (p) {
		case 1:
			return ri.serial1;
			break;
		case 2:
			return ri.serial2;
			break;
		case 3:
			return ri.serial3;
			break;
	}

	return (char *)"";
}

int get_config_bytes_used()
{
	return ri.config_bytes_used;
}

int get_config_bytes_total()
{
	return ri.max_config_size;
}

int get_time_second()
{
	return rtime.second;
}

int get_time_minute()
{
	return rtime.minute;
}

int get_time_hour()
{
	return rtime.hour;
}

int get_time_day()
{
	return rtime.day;
}

int get_time_dow()
{
	return rtime.dow;
}

int get_time_month()
{
	return rtime.month;
}

int get_time_year()
{
	return rtime.year;
}

int get_time_utc_offset()
{
	return rtime.utc_offset;
}

const char *get_time_timezone()
{
	return rtime.timezone.c_str();
}


/*
 * PUBLIC HELPER FUNCTIONS
 */

const char *lc_strerror(int err)
{
	switch (err) {
		case LC_ERROR:
			return "Unknown error";
			break;

		case LC_ERROR_INVALID_DATA_FROM_REMOTE:
			return "Invalid data received from remote";
			break;

		case LC_ERROR_READ:
			return "Error while reading from the remote";
			break;

		case LC_ERROR_WRITE:
			return "Error while writing to the remote";
			break;

		case LC_ERROR_INVALIDATE:
			return 
			"Error while asking the remote to invalidate it's flash";
			break;

		case LC_ERROR_ERASE:
			return "Error while erasing flash";
			break;

		case LC_ERROR_VERIFY:
			return "Error while verifying flash";
			break;

		case LC_ERROR_POST:
			return "Error sending post data to Harmony website";
			break;

		case LC_ERROR_GET_TIME:
			return "Error getting time from remote";
			break;

		case LC_ERROR_SET_TIME:
			return "Error setting time on the remote";
			break;

		case LC_ERROR_CONNECT:
			return "Error connecting or finding the remote";
			break;

		case LC_ERROR_OS:
			return "OS-level error";
			break;

		case LC_ERROR_OS_FILE:
			return "OS-level error related to file operations";
			break;

		case LC_ERROR_OS_NET:
			return "OS-level error related to network operations";
			break;

		case LC_ERROR_UNSUPP:
			return 
			"Model or configuration or operation unsupported";
			break;

		case LC_ERROR_INVALID_CONFIG:
			return 
			"The configuration present on the remote is invalid";
			break;
	}

	return "Unknown error";
}

void delete_blob(uint8_t *ptr)
{
	delete[] ptr;
}

int identify_file(uint8_t *in, uint32_t size, int *type)
{
	int err;

	/*
	 * Validate this is a remotely sane XML file
	 */
	uint8_t *start_info_ptr;
	err = GetTag("INFORMATION", in, size, start_info_ptr);
	if (err == -1) {
		return LC_ERROR;
	}

	uint8_t *end_info_ptr;
	err = GetTag("/INFORMATION", in, size, end_info_ptr);
	if (err == -1) {
		return LC_ERROR;
	}

	/*
	 * Determine size of binary data following /INFORMATION
	 */
	uint32_t data_len = size - (end_info_ptr - in);
	/*
	 * Account for CRLF after /INFORMATION>
	 * But, don't screw up if it's missing
	 */
	if (data_len >= 2) {
		data_len -= 2;
	}

	/*
	 * Search for tag only in "connectivity test" files
	 */
	bool found_get_zaps_only = false;
	uint8_t *tmp_data = in;
	uint32_t tmp_size = size - data_len;
	while (1) {
		uint8_t *tag_ptr;
		string tag_s;
		err = GetTag("KEY", tmp_data, tmp_size, tag_ptr, &tag_s);
		if (err == -1) {
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
	tmp_data = in;
	tmp_size = size - data_len;
	while (1) {
		uint8_t *tag_ptr;
		string tag_s;
		err = GetTag("TYPE", tmp_data, tmp_size, tag_ptr, &tag_s);
		if (err == -1) {
			break;
		}
		if (!stricmp(tag_s.c_str(), "Firmware_Main")) {
			found_firmware = true;
			break;
		}
		tmp_data = tag_ptr + tag_s.length();
		tmp_size = end_info_ptr - tmp_data;
	}

	/*
	 * Search for tag only in "IR learning files.
	 */
	uint8_t *tag_ptr;
	err = GetTag("CHECKKEYS", in, size - data_len, tag_ptr);
	bool found_learn_ir = (err != -1);

	/*
	 * Check tag search results for consistency, and deduce the file type
	 */
	if (found_get_zaps_only && !data_len && !found_firmware &&
		!found_learn_ir) {
		*type = LC_FILE_TYPE_CONNECTIVITY;
		return 0;
	}
	if (!found_get_zaps_only && (data_len >= 16) && !found_firmware &&
		!found_learn_ir) {
		*type = LC_FILE_TYPE_CONFIGURATION;
		return 0;
	}
	if (!found_get_zaps_only && !data_len && found_firmware &&
		!found_learn_ir) {
		*type = LC_FILE_TYPE_FIRMWARE;
		return 0;
	}
	if (!found_get_zaps_only && !data_len && !found_firmware &&
		found_learn_ir) {
		*type = LC_FILE_TYPE_LEARN_IR;
		return 0;
	}

	/*
	 * Findings didn't match a single file type; indicate a problem
	 */
	return LC_ERROR;
}

/*
 * Common routine to read contents of file named *file_name into
 * byte buffer **out. Get size from file and return out[size] 
 * as read from file.
 */
int read_file(char *file_name, uint8_t **out, uint32_t *size)
{
	binaryinfile file;

	if (file_name == NULL) {
		debug("Empty file_name");
		return LC_ERROR_OS_FILE;
	}

	if (file.open(file_name) != 0) {
		debug("Failed to open %s", file_name);
		return LC_ERROR_OS_FILE;
	}

	*size = file.getlength();
	*out = new uint8_t[*size];
	file.read(*out, *size);

	if (file.close() != 0) {
		debug("Failed to close %s\n", file_name);
		return LC_ERROR_OS_FILE;
	}
	return 0;
}


/*
 * PRIVATE HELPER FUNCTIONS
 */

int _is_fw_update_supported(int direct)
{
	/*
	 * If we don't have a fw_base, then we don't support fw updates
	 * in anyway (direct or live).
	 *
	 * If we're in 'live' mode, then we need to make sure we we have a
	 * fw_up_base (we know we have a fw_base from previous portion of if),
	 * to know we're capable of doing it.
	 *
	 * Also, only allow architectures where we've figured out the
	 * structure of the initial magic bytes.
	 */
	if (ri.arch->firmware_base == 0
	    || (!direct && ri.arch->firmware_update_base == 0)
	    || (ri.arch->firmware_4847_offset == 0)) {
		return 0;
	}

	return 1;
}

int _write_fw_to_remote(uint8_t *in, uint32_t size, uint32_t addr,
	lc_callback cb,	void *cb_arg)
{
	int err = 0;

	if ((err = rmt->WriteFlash(addr, size, in,
			ri.protocol, cb, cb_arg))) {
		return LC_ERROR_WRITE;
	}
	return 0;
}

int _read_fw_from_remote(uint8_t *&out, uint32_t size, uint32_t addr,
	lc_callback cb,	void *cb_arg)
{
	out = new uint8_t[size];
	int err = 0;

	if (!cb_arg) {
		cb_arg = (void *)true;
	}

	if ((err = rmt->ReadFlash(addr, size, out,
				ri.protocol, false, cb, cb_arg))) {
		return LC_ERROR_READ;
	}

	return 0;
}

/*
 * Fix the magic bytes of the firmware binaries...
 *
 * The first few bytes of the firmware file we receive from the
 * website will be blanked out (0xFF), and we need to fill them
 * by calculating appropriate content.
 *
 * So why don't we always do this? If the user has a dump from us,
 * it already has the right initial bytes... and if somehow the
 * firmware on the device is messed up, we don't want to ignore
 * that useful data in the file.
 *
 * So we only overwrite the initial bytes if they are missing.
 * For most users, that will be all the time.
 *
 *   - Phil Dibowitz    Tue Mar 11 23:17:53 PDT 2008
 */
int _fix_magic_bytes(uint8_t *in, uint32_t size)
{
	if (size < (ri.arch->firmware_4847_offset + 2)) {
		return LC_ERROR;
	}

	if (in[0] == 0xFF && in[1] == 0xFF) {
		/*
		 * There are "always" two bytes at some location that
		 * contain 0x48 and 0x47.
		 *
		 * Note: For some arch's (10 currently) we haven't
		 * investigated where these go, hence the check for
		 * a valid location in _is_fw_update_supported.
		 *
		 * Note: Arch 2 may be an exception to rule, and needs
		 * more investigation.
		 */
		in[ri.arch->firmware_4847_offset] = 0x48;
		in[ri.arch->firmware_4847_offset + 1] = 0x47;

		/*
		 * The first 2 bytes are a simple 16-bit checksum, computed
		 * beginning at the location of the hard-coded 0x48/0x47
		 * bytes through the end of the firmware.
		 */
		uint8_t suma = 0x21;
		uint8_t sumb = 0x43;
		for (
			uint32_t index = ri.arch->firmware_4847_offset;
			index < FIRMWARE_MAX_SIZE;
			index += 2
		) {
			suma ^= in[index];
			sumb ^= in[index + 1];
		}
		in[0] = suma;
		in[1] = sumb;
	}

	return 0;
}

/*
 * Chunk the hex into words, tack on an '0x', and then throw
 * it through strtoul to get binary data (int) back.
 */
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


/*
 * GENERAL REMOTE STUFF
 */
int init_concord()
{
	int err;
	rmt = NULL;

#ifdef WIN32
	// Initialize WinSock
	WSADATA wsainfo;
	int error = WSAStartup(1*256 + 1, &wsainfo);
	if (error) {
		debug("WSAStartup() Error: %i", error);
		return LC_ERROR_OS_NET;
	}
#endif

	if (InitUSB()) {
		return LC_ERROR_OS;
	}

	if ((err = FindRemote(hid_info))) {
		hid_info.pid = 0;

#ifdef WIN32
		if ((err = FindUsbLanRemote())) {
			return LC_ERROR_CONNECT;
		}

		rmt = new CRemoteZ_TCP;
#else
		return LC_ERROR_CONNECT;
#endif
	}

	/*
	 * If hid_info is defined AND pid is 0XC11F, we found something
	 * via HID that's a 1000... that REALLY shouldn't even be possible
	 * but this'll catch that.
	 */
	if (hid_info.pid == 0xC11F) {
		return LC_ERROR_INVALID_DATA_FROM_REMOTE;
	}

	if (!rmt) {
		if (hid_info.pid >= ZWAVE_HID_PID_MIN &&
		    hid_info.pid <= ZWAVE_HID_PID_MAX) {
			// 890, Monstor, etc.
			rmt = new CRemoteZ_HID;
		} else {
			rmt = new CRemote;
		}
	}	

	return 0;
}

int deinit_concord()
{
	ShutdownUSB();
	delete rmt;
	return 0;
}

int get_identity(lc_callback cb, void *cb_arg)
{
	if ((rmt->GetIdentity(ri, hid_info, cb, cb_arg))) {
		return LC_ERROR;
	}

	/* Do some sanity checking */
	if (ri.flash->size == 0) {
		return LC_ERROR_INVALID_CONFIG;
	}

	if (ri.arch == NULL || ri.arch->cookie == 0) {
		return LC_ERROR_INVALID_CONFIG;
	}

	if (!ri.valid_config) {
		return LC_ERROR_INVALID_CONFIG;
	}

	return 0;
}

int reset_remote()
{
	int err = rmt->Reset(COMMAND_RESET_DEVICE);
	return err;
}

int invalidate_flash()
{
	int err = 0;

	if ((err = rmt->InvalidateFlash()))
		return LC_ERROR_INVALIDATE;

	return 0;
}

int post_preconfig(uint8_t *data, uint32_t size)
{
	return Post(data, size, "POSTOPTIONS", ri, true);
}

int post_postfirmware(uint8_t *data, uint32_t size)
{
	return Post(data, size, "COMPLETEPOSTOPTIONS", ri, false);
}

int post_postconfig(uint8_t *data, uint32_t size)
{
	return Post(data, size, "COMPLETEPOSTOPTIONS", ri, true);
}

int post_connect_test_success(uint8_t *data, uint32_t size)
{
	/*
	 * If we arrived, we can talk to the remote - so if it's
	 * just a connectivity test, tell the site we succeeded
	 */

	/*
	 * For some reason, on arch 9, the site sends a file missing
	 * one cookie value, so we need to tell Post() to add it in.
	 * Note that it ONLY does this for the connectivity test...
	 */
	bool add_cookiekeyval = false;
	if (ri.architecture == 9) {
		add_cookiekeyval = true;
	}

	return Post(data, size, "POSTOPTIONS", ri, true, add_cookiekeyval);
}

int get_time()
{
	int err;
	if ((err = rmt->GetTime(ri, rtime)))
		return LC_ERROR_GET_TIME;

	return 0;
}

int set_time()
{
	const time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	rtime.second = lt->tm_sec;
	rtime.minute = lt->tm_min;
	rtime.hour = lt->tm_hour;
	rtime.day = lt->tm_mday;
	rtime.dow = lt->tm_wday;
	rtime.month = lt->tm_mon + 1;
	rtime.year = lt->tm_year + 1900;
	rtime.utc_offset = 0;
	rtime.timezone = "";

	int err = rmt->SetTime(ri, rtime);
	if (err != 0) {
		return err;
	}

	return 0;
}


/*
 * CONFIG-RELATED
 */

int read_config_from_remote(uint8_t **out, uint32_t *size,
	lc_callback cb, void *cb_arg)
{
	int err = 0;

	if (!ri.valid_config) {
		return LC_ERROR_INVALID_CONFIG;
	}

	if (!cb_arg) {
		cb_arg = (void *)true;
	}

	*size = ri.config_bytes_used;
	*out = new uint8_t[*size];

	if ((err = rmt->ReadFlash(ri.arch->config_base, *size,
			*out, ri.protocol, false, cb, cb_arg))) {
		return LC_ERROR_READ;
	}

	return 0;
}

int write_config_to_remote(uint8_t *in, uint32_t size,
	lc_callback cb, void *cb_arg)
{
	int err = 0;

	if (!cb_arg) {
		cb_arg = (void *)true;
	}

	if ((err = rmt->WriteFlash(ri.arch->config_base, size, in,
			ri.protocol, cb, cb_arg))) {
		return LC_ERROR_WRITE;
	}

	return 0;
}

int write_config_to_file(uint8_t *in, uint32_t size, char *file_name,
	int binary)
{
	binaryoutfile of;

	if (of.open(file_name) != 0) {
		debug("Failed to open %s", file_name);
		return LC_ERROR_OS_FILE;
	}

	if (!binary) {
		uint32_t u = size;
		uint8_t chk = 0x69;
		uint8_t *pc = in;
		while (u--)
			chk ^= *pc++;

		/*
		 * Build XML
		 *    FIXME: Abstract this.
		 */
		extern const char *config_header;
		char *ch = new char[strlen(config_header) + 200];
		const int chlen = sprintf(ch,
			config_header, ri.protocol,
			ri.skin ,ri.flash_mfg,
			ri.flash_id, ri.hw_ver_major,
			ri.hw_ver_minor, ri.fw_type,
			ri.protocol, ri.skin,
			ri.flash_mfg, ri.flash_id,
			ri.hw_ver_major, ri.hw_ver_minor,
			ri.fw_type, ri.config_bytes_used, chk);

		of.write(reinterpret_cast<uint8_t*>(ch), chlen);
	}

	of.write(in, ri.config_bytes_used);

	if (of.close() != 0) {
		debug("Failed to close %s", file_name);
		return LC_ERROR_OS_FILE;
	}

	return 0;
}

int verify_remote_config(uint8_t *in, uint32_t size, lc_callback cb,
	void *cb_arg)
{
	int err = 0;

	if ((err = rmt->ReadFlash(ri.arch->config_base, size, in,
			ri.protocol, true, cb, cb_arg))) {
		return LC_ERROR_VERIFY;
	}

	return 0;
}

int erase_config(uint32_t size, lc_callback cb, void *cb_arg)
{
	int err = 0;

	if ((err = rmt->EraseFlash(ri.arch->config_base, size, ri, cb,
			cb_arg))) {
		return LC_ERROR_ERASE;
	}

	return 0;
}

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
	err = GetTag("BINARYDATASIZE", config, config_size, n,
		&binary_tag_size_s);
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


/*
 * SAFEMODE FIRMWARE RELATED
 */

int erase_safemode(lc_callback cb, void *cb_arg)
{
	int err = 0;

	if ((err = rmt->EraseFlash(ri.arch->flash_base, FIRMWARE_MAX_SIZE, ri,
			cb, cb_arg))) {
		return LC_ERROR_ERASE;
	}

	return 0;
}

int read_safemode_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
	void *cb_arg)
{
	*size = FIRMWARE_MAX_SIZE;
	return _read_fw_from_remote(*out, *size, ri.arch->flash_base, cb,
		cb_arg);
}

int write_safemode_to_file(uint8_t *in, uint32_t size, char *file_name)
{
	binaryoutfile of;

	if (of.open(file_name) != 0) {
		return LC_ERROR_OS_FILE;
	}

	of.write(in, size);

	if (of.close() != 0) {
		return LC_ERROR_OS_FILE;
	}

	return 0;
}


/*
 * FIRMWARE RELATED
 */

int is_fw_update_supported(int direct)
{
	/*
	 * Currently firmware upgrades are only available certain remotes.
	 */
	if (_is_fw_update_supported(direct)) {
		return 0;
	} else {
		return LC_ERROR_UNSUPP;
	}
}

int is_config_safe_after_fw()
{
	/*
	 * For some remotes, firmware updates wipes out the config. The
	 * user code needs to be able to determine this so they can tell
	 * the user and/or update the config.
	 */
	if (ri.arch->firmware_update_base == ri.arch->config_base) {
		return LC_ERROR;
	} else {
		return 0;
	}
}

int prep_firmware()
{
	int err = 0;
	uint8_t data[1];

	if (ri.arch->firmware_update_base == ri.arch->firmware_base) {
		/*
		 * The preperation for where the staging area IS the config
		 * area.
		 *    restart config
		 *    write "1" to flash addr 200000
		 */
		if ((err = rmt->RestartConfig()))
			return LC_ERROR;
		data[0] = 0x00;
		if ((err = rmt->WriteFlash(0x200000, 1, data, ri.protocol, NULL,
				NULL)))
			return LC_ERROR;
	} else {
		/*
		 * The preperation for where the staging area is distinct.
		 *    write "1" to ram addr 0
		 *    read it back
		 */
		data[0] = 0x00;
		if ((err = rmt->WriteRam(0, 1, data)))
			return LC_ERROR_WRITE;
		if ((err = rmt->ReadRam(0, 1, data)))
			return LC_ERROR_WRITE;
		if (data[0] != 0)
			return LC_ERROR_VERIFY;
	}

	return 0;
}

int finish_firmware()
{
	int err = 0;

	uint8_t data[1];
	if (ri.arch->firmware_update_base == ri.arch->firmware_base) {
		data[0] = 0x00;
		if ((err = rmt->WriteFlash(0x200000, 1, data, ri.protocol, NULL,
			NULL)))
			return LC_ERROR;
	} else {
		data[0] = 0x02;
		if ((err = rmt->WriteRam(0, 1, data))) {
			debug("Failed to write 2 to RAM 0");
			return LC_ERROR_WRITE;
		}
		if ((err = rmt->ReadRam(0, 1, data))) {
			debug("Failed to from RAM 0");
			return LC_ERROR_WRITE;
		}
		if (data[0] != 2) {
			printf("byte is %d\n",data[0]);
			debug("Finalize byte didn't match");
			return LC_ERROR_VERIFY;
		}
	}

	return 0;
}

int erase_firmware(int direct, lc_callback cb, void *cb_arg)
{
	int err = 0;

	if (!_is_fw_update_supported(direct)) {
		return LC_ERROR_UNSUPP;
	}

	uint32_t addr = ri.arch->firmware_update_base;
	if (direct) {
		debug("Writing direct");
		addr = ri.arch->firmware_base;
	}

	if ((err = rmt->EraseFlash(addr, FIRMWARE_MAX_SIZE, ri, cb, cb_arg))) {
		return LC_ERROR_ERASE;
	}

	return 0;
}

int read_firmware_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
	void *cb_arg)
{
	*size = FIRMWARE_MAX_SIZE;
	return _read_fw_from_remote(*out, *size, ri.arch->firmware_base, cb,
		cb_arg);
}

int write_firmware_to_remote(uint8_t *in, uint32_t size, int direct,
	lc_callback cb,	void *cb_arg)
{
	uint32_t addr = ri.arch->firmware_update_base;
	int err = 0;

	if (!_is_fw_update_supported(direct)) {
		return LC_ERROR_UNSUPP;
	}

	if (size > FIRMWARE_MAX_SIZE) {
		return LC_ERROR;
	}

	if (direct) {
		debug("Writing direct");
		addr = ri.arch->firmware_base;
	}

	if ((err = _fix_magic_bytes(in, size))) {
		return LC_ERROR_READ;
	}

	return _write_fw_to_remote(in, size, addr, cb, cb_arg);
}

int write_firmware_to_file(uint8_t *in, uint32_t size, char *file_name,
	int binary)
{
	binaryoutfile of;
	if (of.open(file_name) != 0) {
		return LC_ERROR_OS_FILE;
	}

	if (binary) {
		of.write(in, size);
	} else {
#ifdef _DEBUG
		/// todo: file header
		uint16_t *pw =
		   reinterpret_cast<uint16_t*>(in);

		/*
		 * Calculate checksum
		 */
		uint16_t wc = 0x4321;
		uint32_t n = 32*1024;
		while (n--)
			wc ^= *pw++;
		debug("Checksum: %04X", wc);
#endif

		uint8_t *pf = in;
		const uint8_t *fwend = in + size;
		do {
			of.write("\t\t\t<DATA>");
			char hex[16];
			int u = 32;
			if (u > size) {
				u = size;
			}
			while (u--) {
				// convert to hex
				sprintf(hex, "%02X", *pf++);
				of.write(hex);
				size--;
			}
			of.write("</DATA>\n");
		} while (pf < fwend);
	}

	if (of.close() != 0) {
		return LC_ERROR_OS_FILE;
	}

	return 0;
}

int extract_firmware_binary(uint8_t *xml, uint32_t xml_size, uint8_t **out,
	uint32_t *size)
{
	uint32_t o_size = FIRMWARE_MAX_SIZE;
	*out = new uint8_t[o_size];
	uint8_t *o = *out;

	uint8_t *x = xml;
	uint32_t x_size = xml_size;

	string hex;
	while (GetTag("DATA", x, x_size, x, &hex) == 0) {
		uint32_t hex_size = hex.length() / 2;
		if (hex_size > o_size) {
			return LC_ERROR;
		}

		_convert_to_binary(hex, o);

		x_size = xml_size - (x - xml);
		o_size -= hex_size;
	}

	*size = o - *out;

	return 0;
}


/*
 * IR stuff
 */

int learn_ir_commands(uint8_t *data, uint32_t size, int post)
{
	int err;

	if (data) {
		uint8_t *t = data;
		string keyname;
		do {
			err = GetTag("KEY", t, size - (t - data), t, &keyname);
			if (err != 0) {
				return err;
			}
		} while (keyname != "KeyName");
		uint8_t *n = 0;
		err = GetTag("VALUE", t, size, n, &keyname);
		if (err != 0) {
			return err;
		}
		printf("Key Name: %s\n",keyname.c_str());

		string ls;
		rmt->LearnIR(&ls);
		debug("Learned code: %s",ls.c_str());

		if (post) {
			Post(data, size, "POSTOPTIONS", ri, true, false, &ls,
				&keyname);
		}
	} else {
		rmt->LearnIR();
	}

	return 0;
}


/*
 * PRIVATE-SHARED INTERNAL FUNCTIONS
 * These are functions used by the whole library but are NOT part of the API
 * and probably should be somewhere else...
 */

void report_net_error(const char *msg)
{
	int err;
#ifdef WIN32
	err = WSAGetLastError();
	debug("Net error: %s failed with error %i", msg, err);
#else
	debug("Net error: %s failed with error %s", msg, strerror(errno));
#endif
}
