/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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
 *  (C) Copyright Phil Dibowitz 2007
 *  (C) Copyright Kevin Timmerman 2007
 */

/*
 * This file is entry points into what will hopefully become
 * libharmony.
 *   - phil    Sat Aug 18 22:49:48 PDT 2007
 */

#include <stdio.h>
#include "libharmony.h"
#include "harmony.h"
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

#define FIRMWARE_SIZE 64*1024

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
	return rtime.day;
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
 * HELPER FUNCTIONS
 */

const char *lh_strerror(int err)
{
	switch (err) {
		case LH_ERROR:
			return "Unknown error";
			break;

		case LH_ERROR_INVALID_DATA_FROM_REMOTE:
			return "Invalid data received from remote";
			break;

		case LH_ERROR_READ:
			return "Error while reading from the remote";
			break;

		case LH_ERROR_WRITE:
			return "Error while writing to the remote";
			break;

		case LH_ERROR_INVALIDATE:
			return 
			"Error while asking the remote to invalidate it's flash";
			break;

		case LH_ERROR_ERASE:
			return "Error while erasing flash";
			break;

		case LH_ERROR_VERIFY:
			return "Error while verifying flash";
			break;

		case LH_ERROR_POST:
			return "Error sending post data to Harmony website";
			break;

		case LH_ERROR_GET_TIME:
			return "Error getting time from remote";
			break;

		case LH_ERROR_SET_TIME:
			return "Error setting time on the remote";
			break;

		case LH_ERROR_CONNECT:
			return "Error connecting or finding the remote";
			break;

		case LH_ERROR_OS:
			return "OS-level error";
			break;

		case LH_ERROR_OS_FILE:
			return "OS-level error related to file operations";
			break;

		case LH_ERROR_OS_NET:
			return "OS-level error related to network operations";
			break;

		case LH_ERROR_UNSUPP:
			return 
			"Model or configuration or operation unsupported";
			break;

		case LH_ERROR_INVALID_CONFIG:
			return 
			"The configuration present on the remote is invalid";
			break;
	}

	return "Unknown error";
}

int find_binary_size(uint8_t *ptr, uint32_t *size)
{
	string size_s;
	int err = GetTag("BINARYDATASIZE", ptr, &size_s);
	if (err == -1)
		return LH_ERROR;

	*size = (uint32_t)atoi(size_s.c_str());
	return 0;
}

int find_binary_start(uint8_t **ptr, uint32_t *size)
{
	uint8_t *optr = *ptr;
	int err = GetTag("/INFORMATION", *ptr);
	if (err == -1)
		return LH_ERROR;

	*ptr += 2;
	*size -= ((*ptr) - optr);

	return 0;
}

int delete_blob(uint8_t *ptr)
{
	delete[] ptr;
	return 1;
}


/*
 * GENERAL REMOTE STUFF
 */
int init_harmony()
{
	int err;
	rmt = NULL;

#ifdef WIN32
	// Initialize WinSock
	WSADATA wsainfo;
	int error = WSAStartup(1*256 + 1, &wsainfo);
	if (error) {
#ifdef _DEBUG
		printf("WSAStartup() Error: %i\n", error);
#endif
		return LH_ERROR_OS_NET;
	}
#endif

	if (InitUSB()) {
		return LH_ERROR_OS;
	}

	if ((err = FindRemote(hid_info))) {
		hid_info.pid = 0;

#ifdef WIN32
		if ((err = FindUsbLanRemote())) {
			return LH_ERROR_CONNECT;
		}

		rmt = new CRemoteZ_TCP;
#else
		return LH_ERROR_CONNECT;
#endif
	}

	/*
	 * If hid_info is defined AND pid is 0XC11F, we found something
	 * via HID that's a 1000... that REALLY shouldn't even be possible
	 * but this'll catch that.
	 */
	if (hid_info.pid == 0xC11F) {
		return LH_ERROR_INVALID_DATA_FROM_REMOTE;
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

int deinit_harmony()
{
	ShutdownUSB();
	delete rmt;
	return 0;
}

int get_identity(lh_callback cb, void *cb_arg)
{
	if ((rmt->GetIdentity(ri, hid_info, cb, cb_arg))) {
		return LH_ERROR;
	}

	/* Do some sanity checking */
	if (ri.flash->size == 0) {
		return LH_ERROR_INVALID_CONFIG;
	}

	if (ri.arch == NULL || ri.arch->cookie==0) {
		return LH_ERROR_INVALID_CONFIG;
	}

	if (!ri.valid_config) {
		return LH_ERROR_INVALID_CONFIG;
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
		return LH_ERROR_INVALIDATE;

	return 0;
}

int post_preconfig(uint8_t *data)
{
	Post(data, "POSTOPTIONS", ri);
	return 0;
}

int post_postconfig(uint8_t *data)
{
	Post(data, "POSTOPTIONS", ri);
	return 0;
}

int post_connect_test_success(char *file_name)
{
	/*
	 * If we arrived, we can talk to the remote - so if it's
	 * just a connectivity test, tell the site we succeeded
	 */
	binaryinfile file;
	if (file.open(file_name) != 0) {
		return LH_ERROR_OS_FILE;
	}

	const uint32_t size = file.getlength();
	uint8_t * const buf = new uint8_t[size+1];
	file.read(buf,size);
	// Prevent GetTag() from going off the deep end
	buf[size]=0;

	Post(buf,"POSTOPTIONS", ri);

	if (file.close() != 0) {
		return LH_ERROR_OS_FILE;
	}

	return 0;
}

int get_time()
{
	int err;
	if ((err = rmt->GetTime(ri, rtime)))
		return LH_ERROR_GET_TIME;

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
	lh_callback cb, void *cb_arg)
{
	int err = 0;

	if (!ri.valid_config) {
		return LH_ERROR_INVALID_CONFIG;
	}

	if (!cb_arg) {
		cb_arg = (void *)true;
	}

	*size = ri.config_bytes_used;
	*out = new uint8_t[*size];

	if ((err = rmt->ReadFlash(ri.arch->config_base, *size,
			*out, ri.protocol, false, cb, cb_arg))) {
		return LH_ERROR_READ;
	}

	return 0;
}

int write_config_to_remote(uint8_t *in, uint32_t size,
	lh_callback cb, void *cb_arg)
{
	int err = 0;

	if (!cb_arg) {
		cb_arg = (void *)true;
	}

	if ((err = rmt->WriteFlash(ri.arch->config_base, size, in,
			ri.protocol, cb, cb_arg))) {
		return LH_ERROR_WRITE;
	}

	return 0;
}

int read_config_from_file(char *file_name, uint8_t **out, uint32_t *size)
{
	binaryinfile file;

	if (file.open(file_name) != 0) {
#ifdef _DEBUG
		printf("Failed to open %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
	}

	*size = file.getlength();
	*out = new uint8_t[(*size) + 1];
	file.read(*out, *size);

	if (file.close() != 0) {
#ifdef _DEBUG
		printf("Failed to close %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
	}

	/*
	 * Prevent GetTag() from going off the deep end
	 *   FIXME: Should probably be a \0, not a 0
	 */
	(*out)[*size] = 0;

	return 0;
}

int write_config_to_file(uint8_t *in, char *file_name, uint32_t size,
	int binary)
{
	binaryoutfile of;

	if (of.open(file_name) != 0) {
#ifdef _DEBUG
		printf("Failed to open %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
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
#ifdef _DEBUG
		printf("Failed to close %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
	}

	return 0;
}

int verify_xml_config(uint8_t *data, uint32_t size)
{

	uint8_t *ptr = data;
	string s;

	GetTag("BINARYDATASIZE", ptr, &s);
	uint32_t data_size = atoi(s.c_str());

	GetTag("CHECKSUM", ptr, &s);
	const uint8_t checksum = atoi(s.c_str());

	// Calculate size by moving pointer to end of XML
	GetTag("/INFORMATION", ptr);
	ptr += 2;
	size -= (ptr - data);

#ifdef _DEBUG
	printf("reported data size %i\n", data_size);
	printf("checksum %i\n", checksum);
	printf("actual data size %i\n", size);
#endif

	if (size != data_size) {
#ifdef _DEBUG
		printf("Data size mismatch %i %i\n", size, data_size);
#endif
		return LH_ERROR;
	}

	// Calculate checksum
	uint32_t u = size;
	uint8_t chk = 0x69;
	uint8_t *pc = ptr;
	while (u--)
		chk ^= *pc++;

	if (chk != checksum) {
#ifdef _DEBUG
		printf("Bad checksum %02X %02X\n",chk, checksum);
#endif
		return LH_ERROR;
	}

	return 0;
}

int verify_remote_config(uint8_t *in, uint32_t size, lh_callback cb,
	void *cb_arg)
{
	int err = 0;

	if ((err = rmt->ReadFlash(ri.arch->config_base, size, in,
			ri.protocol, true, cb, cb_arg))) {
		return LH_ERROR_VERIFY;
	}

	return 0;
}

int erase_config(uint32_t size, lh_callback cb, void *cb_arg)
{
	int err = 0;

	if ((err = rmt->EraseFlash(ri.arch->config_base, size, ri, cb,
			cb_arg))) {
		return LH_ERROR_ERASE;
	}

	return 0;
}

/*
 * Private fuctions that the safemode and firmware functions use.
 */
int _is_fw_update_supported()
{
	/*
	 * Currently firmware upgrades are only available on architecture 8
	 */
	if (ri.architecture != 8) {
		return 0;
	}
	return 1;
}

int _write_fw_to_remote(uint8_t *in, uint32_t addr, lh_callback cb,
	void *cb_arg)
{
	int err = 0;

	if (!_is_fw_update_supported()) {
		return LH_ERROR_UNSUPP;
	}

	if ((err = rmt->WriteFlash(addr, FIRMWARE_SIZE, in,
			ri.protocol, cb, cb_arg))) {
		return LH_ERROR_WRITE;
	}
	return 0;
}

int _read_fw_from_remote(uint8_t *&out, uint32_t addr, lh_callback cb,
	void *cb_arg)
{
	out = new uint8_t[FIRMWARE_SIZE];
	int err = 0;

	if (!cb_arg) {
		cb_arg = (void *)true;
	}

	if ((err = rmt->ReadFlash(addr, FIRMWARE_SIZE, out,
				ri.protocol, false, cb, cb_arg))) {
		return LH_ERROR_READ;
	}

	return 0;
}

/*
 * SAFEMODE FIRMWARE RELATED
 */
int erase_safemode(lh_callback cb, void *cb_arg)
{
	int err = 0;

	if ((err = rmt->EraseFlash(ri.arch->flash_base, FIRMWARE_SIZE, ri,
			cb, cb_arg))) {
		return LH_ERROR_ERASE;
	}

	return 0;
}

int read_safemode_from_remote(uint8_t **out, lh_callback cb,
	void *cb_arg)
{
	return _read_fw_from_remote(*out, ri.arch->flash_base, cb, cb_arg);
}

int write_safemode_to_file(uint8_t *in, char *file_name)
{
	binaryoutfile of;

	if (of.open(file_name) != 0) {
		return LH_ERROR_OS_FILE;
	}

	of.write(in, FIRMWARE_SIZE);

	if (of.close() != 0) {
		return LH_ERROR_OS_FILE;
	}

	return 0;
}

int read_safemode_from_file(char *file_name, uint8_t **out)
{
	binaryinfile file;

	if (file.open(file_name) != 0) {
#ifdef _DEBUG
		printf("Failed to open %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
	}

	*out = new uint8_t[FIRMWARE_SIZE];
	file.read(*out, FIRMWARE_SIZE);

	if (file.close() != 0) {
#ifdef _DEBUG
		printf("Failed to close %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
	}

	return 0;
}

/*
 * FIRMWARE RELATED
 */

int is_fw_update_supported()
{
	/*
	 * Currently firmware upgrades are only available on architecture 8
	 */
	if (_is_fw_update_supported()) {
		return 0;
	} else {
		return LH_ERROR_UNSUPP;
	}
}

int prep_firmware()
{
	int err = 0;

	uint8_t data[1] = { 0 };
	if ((err = rmt->WriteRam(0, 1, data)))
		return LH_ERROR_WRITE;
	if ((err = rmt->ReadRam(0, 1, data)))
		return LH_ERROR_WRITE;
	if (data[0] != 0)
		return LH_ERROR_VERIFY;

	return 0;
}

int finish_firmware()
{
	int err = 0;

	uint8_t data[1] = { 2 };
	if ((err = rmt->WriteRam(0, 1, data)))
		return LH_ERROR_WRITE;
	if ((err = rmt->ReadRam(0, 1, data)))
		return LH_ERROR_WRITE;
	if (data[0] != 2)
		return LH_ERROR_VERIFY;

	return 0;
}

int erase_firmware(int direct, lh_callback cb, void *cb_arg)
{
	int err = 0;

	if (!_is_fw_update_supported()) {
		return LH_ERROR_UNSUPP;
	}

	uint32_t addr = ri.arch->firmware_update_base;
	if (direct) {
#ifdef _DEBUG
		printf("WARNING: Writing direct\n");
#endif
		addr = ri.arch->firmware_base;
	}

	if ((err = rmt->EraseFlash(addr, FIRMWARE_SIZE, ri, cb, cb_arg))) {
		return LH_ERROR_ERASE;
	}

	return 0;
}

int read_firmware_from_remote(uint8_t **out, lh_callback cb,
	void *cb_arg)
{
	return _read_fw_from_remote(*out, ri.arch->firmware_base, cb, cb_arg);
}

int write_firmware_to_remote(uint8_t *in, int direct, lh_callback cb,
	void *cb_arg)
{
	uint32_t addr = ri.arch->firmware_update_base;

	if (direct) {
#ifdef _DEBUG
		printf("WARNING: Writing direct\n");
#endif
		addr = ri.arch->firmware_base;
	}

	return _write_fw_to_remote(in, addr, cb, cb_arg);
}

int write_firmware_to_file(uint8_t *in, char *file_name, int binary)
{
	binaryoutfile of;
	if (of.open(file_name) != 0) {
		return LH_ERROR_OS_FILE;
	}

	if (binary) {
		of.write(in, FIRMWARE_SIZE);
	} else {
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
		//TRACE1("Checksum: %04X\n",wc);

		/*
		 * Kevin saw Harmony do this somewhere,
		 * but it bricks the firmware every time,
		 * so I'm nuking it.
		 */
		//in[0] = in[1] = in[2] = in[3] = 0xFF;

		uint8_t *pf = in;
		const uint8_t *fwend = in + FIRMWARE_SIZE;
		do {
			of.write("\t\t\t<DATA>");
			char hex[16];
			int u = 32;
			while (u--) {
				// convert to hex
				sprintf(hex, "%02X", *pf++);
				of.write(hex);
			}
			of.write("</DATA>\n");
		} while (pf < fwend);
	}

	if (of.close() != 0) {
		return LH_ERROR_OS_FILE;
	}

	return 0;
}

/*
 * Chunk the hex into words, tack on an '0x', and then throw
 * it through strtoul to get binary data (int) back.
 */
int convert_to_binary(string hex, uint8_t *&ptr)
{
	size_t size = hex.length();
	for (size_t i = 0; i < size; i += 2) {
		char tmp[6];
		sprintf(tmp, "0x%s ", hex.substr(i,2).c_str());
		ptr[0] = (uint8_t)strtoul(tmp, NULL, 16);
		ptr++;
	}
	return 0;
}

int read_firmware_from_file(char *file_name, uint8_t **out, int binary)
{
	binaryinfile file;

	if (file.open(file_name) != 0) {
#ifdef _DEBUG
		printf("Failed to open %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
	}

	uint32_t size = 0;
	*out = new uint8_t[FIRMWARE_SIZE];

	if (binary) {
		size = FIRMWARE_SIZE;
		file.read(*out, FIRMWARE_SIZE);
	} else {
		uint8_t *tmp = new uint8_t[file.getlength() + 1];
		file.read(tmp, file.getlength());
		tmp[file.getlength() + 1] = 0;
	
		string hex;
		uint8_t *ptr = *out;
		while (GetTag("DATA", tmp, &hex) == 0) {
			convert_to_binary(hex, ptr);
		}
	}

	if (file.close() != 0) {
#ifdef _DEBUG
		printf("Failed to close %s\n", file_name);
#endif
		return LH_ERROR_OS_FILE;
	}

	return 0;
}

/*
 * IR stuff
 */
int learn_ir_commands(char *file_name, int post)
{
	if (file_name) {
		binaryinfile file;
		if (file.open(file_name)) {
			return LH_ERROR_OS_FILE;
		}
		uint32_t size=file.getlength();
		uint8_t * const x=new uint8_t[size+1];
		file.read(x,size);
		if (file.close() != 0) {
			return LH_ERROR_OS_FILE;
		}
		// Prevent GetTag() from going off the deep end
		x[size]=0;

		uint8_t *t=x;
		string keyname;
		do {
			GetTag("KEY",t,&keyname);
		} while (*t && keyname!="KeyName");
		GetTag("VALUE",t,&keyname);
		printf("Key Name: %s\n",keyname.c_str());

		string ls;
		rmt->LearnIR(&ls);
		//printf("%s\n",ls.c_str());

		if (post)
			Post(x, "POSTOPTIONS", ri, &ls, &keyname);

	} else {
		rmt->LearnIR();
	}

	return 0;
}


/*
 * INTERNAL FUNCTIONS
 */

void report_net_error(const char *msg)
{
	int err;
#ifdef WIN32
	err = WSAGetLastError();
#   ifdef _DEBUG
	printf("Net error: %s failed with error %i\n", msg, err);
#   endif
#else
#   ifdef _DEBUG
	printf("Net error: %s failed with error %s\n", msg, strerror(errno));
#   endif
#endif
}

