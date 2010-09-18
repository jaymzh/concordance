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

/* STATUS:
 *   OK, we need to re-re-re-factor, again. Pull the read-and-parse logic into a
 *   new class. We can still call it through the remote object, but there will
 *   be a "get-file-data" call which we can use to re-init a new remote object.
 *
 *   OR
 *
 *   perhaps we just add a re-init to the remote object rather than destroying
 *   it?
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <zzip/lib.h>
#include <list>
#include "libconcord.h"
#include "lc_internal.h"
#include "remote.h"
#include "binaryfile.h"
#include "hid.h"
#include "usblan.h"
#include "web.h"
#include "protocol.h"
#include "time.h"
#include "operationfile.h"

#define ZWAVE_HID_PID_MIN 0xC112
#define ZWAVE_HID_PID_MAX 0xC115

#define MAX_WAIT_FOR_BOOT 5
#define WAIT_FOR_BOOT_SLEEP 5

static class CRemoteBase *rmt;
static class OperationFile *of;
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

int is_z_remote()
{
	/* should this be in the remoteinfo struct? */
	return rmt->IsZRemote() ? 1 : 0;
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

		case LC_ERROR_IR_OVERFLOW:
			return 
			"Received IR signal is too long - release key earlier";
			break;
	}

	return "Unknown error";
}

const char *lc_cb_stage_str(int stage)
{
	switch (stage) {
		case LC_CB_STAGE_GET_IDENTITY:
			return "Requesting identity";
			break;

		case LC_CB_STAGE_INITIALIZE_UPDATE:
			return "Initializing update";
			break;

		case LC_CB_STAGE_INVALIDATE_FLASH:
			return "Invalidating Flash";
			break;

		case LC_CB_STAGE_ERASE_FLASH:
			return "Erasing flash";
			break;

		case LC_CB_STAGE_WRITE_CONFIG:
			return "Writing config";
			break;

		case LC_CB_STAGE_VERIFY_CONFIG:
			return "Verifying config";
			break;

		case LC_CB_STAGE_FINALIZE_UPDATE:
			return "Finalizing update";
			break;

		case LC_CB_STAGE_WRITE_FIRMWARE:
			return "Writing firmware";
			break;

		case LC_CB_STAGE_RESET:
			return "Rebooting remote";
			break;

		case LC_CB_STAGE_SET_TIME:
			return "Setting time";
			break;

		case LC_CB_STAGE_HTTP:
			return "Contacting website";
			break;
	}

	return "(Unknown)";
}

void delete_blob(uint8_t *ptr)
{
	delete[] ptr;
}

int identify_file(uint8_t *in, uint32_t size, int *type, int xml_only)
{
	int err;
	uint8_t *start_info_ptr, *end_info_ptr;

	/*
	 * Validate this is a remotely sane XML file
	 */
	if (xml_only) {
		start_info_ptr = in;
		end_info_ptr = start_info_ptr + size;
	} else {
		err = GetTag("INFORMATION", in, size, start_info_ptr);
		if (err == -1) {
			return LC_ERROR;
		}

		err = GetTag("/INFORMATION", in, size, end_info_ptr);
		if (err == -1) {
			return LC_ERROR;
		}
	}
	debug("start/end pointers populated");

	/*
	 * Determine size of binary data following /INFORMATION
	 */
	uint32_t data_len = size - (end_info_ptr - in);
	/*
	 * Account for CRLF after /INFORMATION>
	 * But, don't screw up if it's missing
	 */
	if (data_len >= 2 && !xml_only) {
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
			debug("not a connectivity test file");
			break;
		}
		if (!stricmp(tag_s.c_str(), "GETZAPSONLY")) {
			debug("is a connectivity test file");
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
			debug("not a firmware file");
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
		debug("returning connectivity");
		*type = LC_FILE_TYPE_CONNECTIVITY;
		return 0;
	}
	if (!found_get_zaps_only && (data_len >= 16 || xml_only) &&
		!found_firmware && !found_learn_ir) {
		debug("returning config");
		*type = LC_FILE_TYPE_CONFIGURATION;
		return 0;
	}
	if (!found_get_zaps_only && !data_len && found_firmware &&
		!found_learn_ir) {
		debug("returning firmware");
		*type = LC_FILE_TYPE_FIRMWARE;
		return 0;
	}
	if (!found_get_zaps_only && !data_len && !found_firmware &&
		found_learn_ir) {
		debug("returning ir");
		*type = LC_FILE_TYPE_LEARN_IR;
		return 0;
	}

	/*
	 * Findings didn't match a single file type; indicate a problem
	 */
	return LC_ERROR;
}

int read_zip_file(char *file_name, uint8_t **data, uint32_t *data_size,
	uint8_t **xml, uint32_t *xml_size)
{
	/* TODO: error checking */
	zzip_error_t zip_err;
	ZZIP_DIR *dir = zzip_dir_open(file_name, &zip_err);
	if (dir) {
		ZZIP_DIRENT dirent;
		while (zzip_dir_read(dir, &dirent)) {
			ZZIP_FILE *fh = zzip_file_open(dir, dirent.d_name, 0);
			if (strcmp(dirent.d_name, "Data.xml") == 0) {
				*xml_size = dirent.st_size;
				*xml = new uint8_t[*xml_size];
				zzip_size_t len = zzip_file_read(fh, *xml,
					*xml_size);
			} else {
				*data_size = dirent.st_size;
				*data = new uint8_t[*data_size];
				zzip_size_t len = zzip_file_read(fh, *data,
					*data_size);
			}
			zzip_file_close(fh);
		}
	} else {
		return LC_ERROR;
	}
	zzip_dir_close(dir);
	return 0;
}

int read_and_parse_file(char *filename)
{
	debug("Calling RAPOF");
	of = new OperationFile;
	return of->ReadAndParseOpFile(filename);
}
#if 0
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
#endif


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
	lc_callback cb,	void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	if ((err = rmt->WriteFlash(addr, size, in,
			ri.protocol, cb, cb_arg, cb_stage))) {
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

int _get_identity(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	if ((rmt->GetIdentity(ri, hid_info, cb, cb_arg, cb_stage))) {
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

int get_identity(lc_callback cb, void *cb_arg)
{
	_get_identity(cb, cb_arg, LC_CB_STAGE_GET_IDENTITY);
}

int reset_remote(lc_callback cb, void *cb_arg)
{
	int err;
	if (cb)
		cb(LC_CB_STAGE_RESET, 0, 0, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	if ((err = rmt->Reset(COMMAND_RESET_DEVICE)))
		return err;

	if (cb)
		cb(LC_CB_STAGE_RESET, 1, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	deinit_concord();
	for (int i = 0; i < MAX_WAIT_FOR_BOOT; i++) {
		sleep(WAIT_FOR_BOOT_SLEEP);
		err = init_concord();
		if (err == 0) {
			err = get_identity(NULL, NULL);
			if (err == 0) {
				break;
			}
			deinit_concord();
		}
	}

	if (err != 0)
		return err;

	if (cb)
		cb(LC_CB_STAGE_RESET, 2, 2, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	return 0;
}

/* FIXME: This should almost certainly be rolled into prep_config() */
int _invalidate_flash(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	if ((err = rmt->InvalidateFlash(cb, cb_arg, cb_stage)))
		return LC_ERROR_INVALIDATE;

	return 0;
}

int invalidate_flash(lc_callback cb, void *cb_arg)
{
	_invalidate_flash(cb, cb_arg, LC_CB_STAGE_INVALIDATE_FLASH);
}

int post_preconfig(lc_callback cb, void *cb_arg)
{
	int err;
	if (cb)
		cb(LC_CB_STAGE_HTTP, 0, 0, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);
	if ((err = Post(of->GetXml(), of->GetXmlSize(), "POSTOPTIONS", ri,
			true))) {
		return err;
	}
	if (cb)
		cb(LC_CB_STAGE_HTTP, 1, 1, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);
	return 0;
}

int post_postfirmware(lc_callback cb, void *cb_arg)
{
	int err;
	if (cb)
		cb(LC_CB_STAGE_HTTP, 0, 0, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	if ((err = Post(of->GetXml(), of->GetXmlSize(), "COMPLETEPOSTOPTIONS", ri,
			false)))
		return err;

	if (cb)
		cb(LC_CB_STAGE_HTTP, 1, 1, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);
	return 0;
}

int post_postconfig(lc_callback cb, void *cb_arg)
{
	int err;
	if (cb)
		cb(LC_CB_STAGE_HTTP, 0, 0, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	if ((err = Post(of->GetXml(), of->GetXmlSize(), "COMPLETEPOSTOPTIONS", ri,
		        true, false, is_z_remote() ? true : false, NULL, NULL)))
		return err;

	if (cb)
		cb(LC_CB_STAGE_HTTP, 1, 1, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	return 0;
}

int post_connect_test_success(lc_callback cb, void *cb_arg)
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
	int err;
	bool add_cookiekeyval = false;
	if (ri.architecture == 9) {
		add_cookiekeyval = true;
	}

	if (cb)
		cb(LC_CB_STAGE_HTTP, 0, 0, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	if ((err = Post(of->GetXml(), of->GetXmlSize(), "POSTOPTIONS", ri, true,
			add_cookiekeyval)))
		return err;

	if (cb)
		cb(LC_CB_STAGE_HTTP, 1, 1, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	return 0;
}

int get_time()
{
	int err;
	if ((err = rmt->GetTime(ri, rtime)))
		return LC_ERROR_GET_TIME;

	return 0;
}

int _set_time(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	const time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	if (cb)
		cb(cb_stage, 0, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

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
	if (cb)
		cb(cb_stage, 1, 2, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg);

	return 0;
}

int set_time(lc_callback cb, void *cb_arg)
{
	return _set_time(cb, cb_arg, LC_CB_STAGE_SET_TIME);
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

int _write_config_to_remote(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	if (!cb_arg) {
		cb_arg = (void *)true;
	}

	if (is_z_remote()) {
		if ((err = rmt->UpdateConfig(of->GetDataSize(), of->GetData(),
				cb, cb_arg, cb_stage)))
			return LC_ERROR_WRITE;
	} else {
		if ((err = rmt->WriteFlash(ri.arch->config_base,
				of->GetDataSize(), of->GetData(), ri.protocol,
				cb, cb_arg, cb_stage)))
			return LC_ERROR_WRITE;
	}

	return 0;
}

int write_config_to_remote(lc_callback cb, void *cb_arg)
{
	return _write_config_to_remote(cb, cb_arg, LC_CB_STAGE_WRITE_CONFIG);
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
		free(ch);
	}

	of.write(in, ri.config_bytes_used);

	if (of.close() != 0) {
		debug("Failed to close %s", file_name);
		return LC_ERROR_OS_FILE;
	}

	return 0;
}

int _verify_remote_config(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	if ((err = rmt->ReadFlash(ri.arch->config_base, of->GetDataSize(),
			of->GetData(), ri.protocol, true, cb, cb_arg, cb_stage))) {
		return LC_ERROR_VERIFY;
	}

	return 0;
}

int verify_remote_config(lc_callback cb, void *cb_arg)
{
	_verify_remote_config(cb, cb_arg, LC_CB_STAGE_VERIFY_CONFIG);
}

int _prep_config(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	if ((err = rmt->PrepConfig(ri, cb, cb_arg, cb_stage))) {
		return LC_ERROR;
	}

	return 0;
}

int prep_config(lc_callback cb, void *cb_arg)
{
	return _prep_config(cb, cb_arg, LC_CB_STAGE_INITIALIZE_UPDATE);
}

int _finish_config(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	if ((err = rmt->FinishConfig(ri))) {
		return LC_ERROR;
	}

	return 0;
}

int finish_config(lc_callback cb, void *cb_arg)
{
	return _finish_config(cb, cb_arg, LC_CB_STAGE_FINALIZE_UPDATE);
}

int _erase_config(lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	if ((err = rmt->EraseFlash(ri.arch->config_base, of->GetDataSize(),
			ri, cb, cb_arg, cb_stage))) {
		return LC_ERROR_ERASE;
	}

	return 0;
}

int erase_config(lc_callback cb, void *cb_arg)
{
	return _erase_config(cb, cb_arg, LC_CB_STAGE_ERASE_FLASH);
}

int _update_configuration_zwave(lc_callback cb, void *cb_arg)
{
	int err;

	if ((err = _write_config_to_remote(cb, cb_arg, NULL))) {
		return err;
	}

	return 0;
}

int _update_configuration_hid(lc_callback cb, void *cb_arg) {
	int err;
	int cb_count = 0;

	if ((err = prep_config(cb, cb_arg))) {
		return err;
	}

	/*
	 * We must invalidate flash before we erase and write so that
	 * nothing will attempt to reference it while we're working.
	 */
	if ((err = invalidate_flash(cb, cb_arg))) {
		return err;
	}

	/*
	 * Flash can be changed to 0, but not back to 1, so you must
	 * erase the flash (to 1) in order to write the flash.
	 */
	if ((err = erase_config(cb, cb_arg))) {
		return err;
	}

	if ((err = write_config_to_remote(cb, cb_arg))) {
		return err;
	}

	if ((err = verify_remote_config(cb, cb_arg))) {
		return err;
	}

	if ((err = finish_config(cb, cb_arg))) {
		return err;
	}

	return 0;
}

int update_configuration(lc_callback cb, void *cb_arg, int noreset)
{
	int err;
	if (is_z_remote()) {
		err = _update_configuration_zwave(cb, cb_arg);
	} else {
		err = _update_configuration_hid(cb, cb_arg);
	}

	if (err != 0)
		return err;

	if (noreset)
		return 0;

	if ((err = reset_remote(cb, cb_arg)))
		return err;

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

int prep_firmware(lc_callback cb, void *cb_arg)
{
	int err = 0;

	if ((err = rmt->PrepFirmware(ri, cb, cb_arg,
			LC_CB_STAGE_INITIALIZE_UPDATE))) {
		return LC_ERROR;
	}

	return 0;
}

int finish_firmware(lc_callback cb, void *cb_arg)
{
	int err = 0;

	if ((err = rmt->FinishFirmware(ri, cb, cb_arg,
			LC_CB_STAGE_FINALIZE_UPDATE))) {
		return LC_ERROR;
	}

	return 0;
}

int _erase_firmware(int direct, lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;

	uint32_t addr = ri.arch->firmware_update_base;
	if (direct) {
		debug("Writing direct");
		addr = ri.arch->firmware_base;
	}

	if ((err = rmt->EraseFlash(addr, FIRMWARE_MAX_SIZE, ri, cb, cb_arg,
			cb_stage))) {
		return LC_ERROR_ERASE;
	}

	return 0;
}

int erase_firmware(int direct, lc_callback cb, void *cb_arg)
{
	_erase_firmware(direct, cb, cb_arg, LC_CB_STAGE_ERASE_FLASH);
}

int read_firmware_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
	void *cb_arg)
{
	*size = FIRMWARE_MAX_SIZE;
	return _read_fw_from_remote(*out, *size, ri.arch->firmware_base, cb,
		cb_arg);
}

int _write_firmware_to_remote(int direct, lc_callback cb, void *cb_arg,
	uint32_t cb_stage)
{
	uint32_t addr = ri.arch->firmware_update_base;
	int err = 0;

	if (of->GetDataSize() > FIRMWARE_MAX_SIZE) {
		return LC_ERROR;
	}

	if (direct) {
		debug("Writing direct");
		addr = ri.arch->firmware_base;
	}

	if ((err = _fix_magic_bytes(of->GetData(), of->GetDataSize()))) {
		return LC_ERROR_READ;
	}

	return _write_fw_to_remote(of->GetData(), of->GetDataSize(), addr, cb,
		cb_arg, cb_stage);
}

int write_firmware_to_remote(int direct, lc_callback cb, void *cb_arg)
{
	return _write_firmware_to_remote(direct, cb, cb_arg,
		LC_CB_STAGE_WRITE_FIRMWARE);
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
			uint32_t u = 32;
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

int update_firmware(lc_callback cb, void *cb_arg, int noreset, int direct)
{
	int err;

	if (!_is_fw_update_supported(direct)) {
		return LC_ERROR_UNSUPP;
	}

	if (!direct) {
		if ((err = prep_firmware(cb, cb_arg)))
			return err;
	}

	if ((err = invalidate_flash(cb, cb_arg)))
		return err;

	if ((err = erase_firmware(direct, cb, cb_arg)))
		return err;

	if ((err = write_firmware_to_remote(direct, cb, cb_arg)))
		return err;

	if (!direct) {
		if ((err = finish_firmware(cb, cb_arg)))
			return err;
	}

	if (noreset)
		return 0;

	reset_remote(cb, cb_arg);

	if ((err = set_time(cb, cb_arg)))
		return err;

	return 0;
}


/*
 * IR stuff
 */

/*
 * List of key names to be learned is passed in section INPUTPARMS
 * as e.g.:
 * <PARAMETER><KEY>KeyName</KEY><VALUE>PowerToggle</VALUE></PARAMETER>
 * First of these is repeated in section PARAMETERS, so we must
 * concentrate on INPUTPARMS to avoid duplication.
 */

/*
 * locate the INPUTPARMS section in *data:
 */
int _init_key_scan(uint8_t *data, uint32_t size, 
	uint8_t **inputparams_start, uint8_t **inputparams_end)
{
	int err;
		
	/* locating start tag "<INPUTPARMS>" */
	err = GetTag("INPUTPARMS", data, size, *inputparams_start);
	if (err == 0) {
		/* locating end tag "</INPUTPARMS>" */
		err = GetTag("/INPUTPARMS", *inputparams_start, 
			size - (*inputparams_start - data), *inputparams_end);
	}
	return err;
}
	
int _next_key_name(uint8_t **start, uint8_t *inputparams_end, string *keyname)
{
	int err;
	/*
	 * to be really paranoid, we would have to narrow the search range
	 * further down to next <PARAMETER>...</PARAMETER>, but IMHO it
	 * should be safe to assume that Logitech always sends sane files:
	 */
	do {
		err = GetTag("KEY", *start, (inputparams_end - *start),
				*start, keyname);
		if (err != 0) {
			return err;
		}
	} while (*keyname != "KeyName");

	err = GetTag("VALUE", *start, (inputparams_end - *start),
			*start, keyname);

	if (err == 0) {
		/* found next key name : */
		debug("Key Name: %s\n", (*keyname).c_str());
	}
	return err;
}


int get_key_names(char ***key_names, uint32_t *key_names_length)
{
	using namespace std;
	uint8_t *cursor = of->GetXml();
	uint8_t *inputparams_end;
	uint32_t key_index = 0;
	list<string> key_list;
	string key_name;
	
	if ((key_names == NULL) || (key_names_length == NULL)) {
		return LC_ERROR;
	}
	/* setup data scanning, locating start and end of keynames section: */
	if (_init_key_scan(of->GetXml(), of->GetXmlSize(), &cursor,
		&inputparams_end) != 0) {
		return LC_ERROR;
	}
	
	/* scan for key names and append found names to list: */
	while (_next_key_name(&cursor, inputparams_end, &key_name) == 0) {
		key_list.push_back(key_name);
	}
	
	if (key_list.size() == 0) {
		return LC_ERROR;
	}
	
	*key_names_length = key_list.size();
	*key_names = new char*[*key_names_length];
			
	/* copy list of found names to allocated buffer: */
	for (
		list<string>::const_iterator cursor = key_list.begin();
		cursor != key_list.end();
		++cursor
	) {
		(*key_names)[key_index++] = strdup((*cursor).c_str());
	}
	/* C++ should take care of key_name and key_list deallocation */
	return 0;
}

/*
 * Free memory allocated by get_key_names:
 */
void delete_key_names(char **key_names, uint32_t key_names_length)
{
	uint32_t key_count = 0;
	if (key_names != NULL) {
		for (key_count = 0; key_count < key_names_length; key_count++) {
			free(key_names[key_count]);
			/* allocated by strdup -> free() */
		}
		delete[](key_names); /* allocated by new[] -> delete[] */
	}
}

/*
 * Fill ir_data with IR code learned from other remote
 * via Harmony IR receiver.
 * Returns 0 for success, error code for failure.
 */
int learn_from_remote(uint32_t *carrier_clock,
	uint32_t **ir_signal, uint32_t *ir_signal_length,
	lc_callback cb, void *cb_arg)
{
	if (rmt == NULL){
		return LC_ERROR_CONNECT;
	}
	if ((carrier_clock == NULL) || (ir_signal == NULL)
			|| (ir_signal_length == NULL)){
		/* nothing to write to: */
		return LC_ERROR;
	}
	
	/* try to learn code via Harmony from original remote: */
	return rmt->LearnIR(carrier_clock, ir_signal, ir_signal_length, cb,
		cb_arg);
}

/*
 * Free memory allocated by learn_from_remote:
 */
void delete_ir_signal(uint32_t *ir_signal)
{
	delete[] ir_signal;  /* allocated by new[] -> delete[] */
}

/*
 * Fill encoded_signal with IR code encoded to Logitech
 * posting string format.
 * Returns 0 for success, error code in case of failure.
 */
int encode_for_posting(uint32_t carrier_clock,
	uint32_t *ir_signal, uint32_t ir_signal_length,
	char **encoded_signal)
{
	int err = 0;
	string encoded;
	if (ir_signal == NULL || ir_signal_length == 0
			|| encoded_signal == NULL) {
		return LC_ERROR;	/* cannot do anything without */
	}
	err = encode_ir_signal(carrier_clock, 
			ir_signal, ir_signal_length, &encoded);
	if (err == 0) {
		debug("Learned code: %s",encoded.c_str());
		*encoded_signal = strdup(encoded.c_str());
	}
	return err;
}

/*
 * Free memory allocated by encode_for_posting:
 */
void delete_encoded_signal(char *encoded_signal)
{
	free(encoded_signal); /* allocated by strdup -> free() */
}

/*
 * Post encoded IR-code with key_name and additional 
 * information from XML data[size] to Logitech.
 * Returns 0 for success, error code for failure.
 */
int post_new_code(char *key_name, char *encoded_signal)
{
	string learn_key;
	string learn_seq;

	if (key_name == NULL || encoded_signal == NULL) {
		return LC_ERROR_POST;	/* cannot do anything without */
	}

	learn_key = key_name;
	learn_seq = encoded_signal;

	return Post(of->GetXml(), of->GetXmlSize(), "POSTOPTIONS", ri, true,
			false, false, &learn_seq, &learn_key);
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
