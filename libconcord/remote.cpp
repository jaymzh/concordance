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
 * (C) Copyright Phil Dibowitz 2007
 */

#include "remote.h"

#include <string.h>
#include <errno.h>

#include <iostream>

#include "libconcord.h"
#include "lc_internal.h"
#include "hid.h"
#include "protocol.h"
#include "remote_info.h"

void setup_ri_pointers(TRemoteInfo &ri)
{
	unsigned int u;
	for (u = 0; u < sizeof(FlashList)/sizeof(TFlash)-1; ++u) {
		if (ri.flash_id == FlashList[u].id
		    && ri.flash_mfg == FlashList[u].mfg)
			break;
	}
	ri.flash = &FlashList[u];

	ri.arch = (ri.architecture < sizeof(ArchList)/sizeof(TArchInfo))
			? &ArchList[ri.architecture] : NULL;

	ri.model = (ri.skin < max_model)
			? &ModelList[ri.skin] : &ModelList[max_model];
}

void make_guid(const uint8_t * const in, char*&out)
{
	char x[48];
	// usbnet remotes seem to use a more normal byte ordering for serial #'s
	if (is_usbnet_remote()) {
		sprintf(x,
		"{%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			in[0], in[1], in[2], in[3], in[4], in[5], in[6], in[7],
			in[8], in[9], in[10], in[11], in[12], in[13], in[14], in[15]);
	}
	else {
		sprintf(x,
		"{%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
			in[3], in[2], in[1], in[0], in[5], in[4], in[7], in[6],
			in[8], in[9], in[10], in[11], in[12], in[13], in[14], in[15]);
	}
	out = strdup(x);
}

void make_serial(uint8_t *ser, TRemoteInfo &ri)
{
	make_guid(ser   , ri.serial1);
	make_guid(ser+16, ri.serial2);
	make_guid(ser+32, ri.serial3);
}

int CRemote::Reset(uint8_t kind)
{
	uint8_t reset_cmd[64] = { COMMAND_RESET, kind };
	int err;

	err = HID_WriteReport(reset_cmd);
	/*
	 * Certain remotes (e.g., the 785) do not get a successful return from
	 * HID_WriteReport even though the reset succeeds.  Ignore this.
	 */
	if (err == -ENODEV) {
		debug("Ignoring error from reset command");
		err = 0;
	}
	return err;
}

/*
 * Send the GET_VERSION command to the remote, and read the response.
 *
 * Then populate our struct with all the relevant info.
 */
int CRemote::GetIdentity(TRemoteInfo &ri, THIDINFO &hid,
	lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;
	uint32_t cb_count = 0;

	const uint8_t qid[64] = { COMMAND_GET_VERSION };

	if ((err = HID_WriteReport(qid))) {
		debug("Failed to write to remote");
		return 1;
	}

	uint8_t rsp[68];
	if ((err = HID_ReadReport(rsp))) {
		debug("Failed to read from remote");
		return 1;
	}

	/*
 	 * See specs/protocol.txt for format
 	 */
	const unsigned int rx_len = rsp[0] & 0x0F;

	if ((rsp[0] & 0xF0) != RESPONSE_VERSION_DATA ||
	    (rx_len != 5 && rx_len != 7 && rx_len != 8)) {
		debug("Bogus ident response: %02X", rsp[1]);
		return LC_ERROR_INVALID_DATA_FROM_REMOTE;
	}

	ri.fw_ver_major = rsp[1] >> 4;
	ri.fw_ver_minor = rsp[1] & 0x0F;
	ri.hw_ver_major = rsp[2] >> 4;
	ri.hw_ver_minor = rsp[2] & 0x0F;
	ri.hw_ver_micro = 0; /* usbnet remotes have a non-zero micro version */
	ri.flash_id = rsp[3];
	ri.flash_mfg = rsp[4];
	ri.architecture = rx_len < 6 ? 2 : rsp[5] >> 4;
	ri.fw_type = rx_len < 6 ? 0 : rsp[5] & 0x0F;
	ri.skin = rx_len < 6 ? 2 : rsp[6];
	if (rx_len < 7) {
		ri.protocol = 0;
	} else if (rx_len < 8) {
		ri.protocol = rsp[7];
	} else {
		ri.protocol = ri.architecture;
	}

	setup_ri_pointers(ri);

	uint8_t rd[1024];
	if ((err=ReadFlash(ri.arch->config_base, 1024, rd, ri.protocol,
								false))) {
		debug("Error reading first k of config data");
		return LC_ERROR_READ;
	}
	if (cb) {
		cb(cb_stage, cb_count++, 1, 2,
			LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
	}

	/*
	 * Calculate cookie
	 *   see specs/protocol.txt for more info
	 */
	const uint32_t cookie = (ri.arch->cookie_size == 2)
			? rd[0] | (rd[1] << 8)
			: rd[0] | (rd[1] << 8) | (rd[2] << 16) | (rd[3] << 24);

	ri.valid_config = (cookie == ri.arch->cookie);

	if (ri.valid_config) {
		ri.max_config_size = (ri.flash->size << 10)
			- (ri.arch->config_base - ri.arch->flash_base);
		const uint32_t end = rd[ri.arch->end_vector]
				| (rd[ri.arch->end_vector + 1] << 8)
				| (rd[ri.arch->end_vector + 2] << 16);
		ri.config_bytes_used =
			(end - (ri.arch->config_base - ri.arch->flash_base))
			+ 4;
	} else {
		ri.config_bytes_used = 0;
		ri.max_config_size = 1;
	}

	// read serial (see specs/protocol.txt for details)
	switch (ri.arch->serial_location) {
	case SERIAL_LOCATION_EEPROM:
		err = ReadMiscByte(ri.arch->serial_address, SERIAL_SIZE,
			COMMAND_MISC_EEPROM, rsp);
		break;
	case SERIAL_LOCATION_FLASH:
		err = ReadFlash(ri.arch->serial_address, SERIAL_SIZE, rsp,
			ri.protocol);
		break;
	default:
		debug("Invalid TArchInfo\n");
		return LC_ERROR_READ;
	}
	if (err) {
		debug("Couldn't read serial\n");
		return LC_ERROR_READ;
	}

	if (cb) {
		cb(cb_stage, cb_count++, 2, 2,
			LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
	}

	make_serial(rsp, ri);

	return 0;
}

int CRemote::ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
	unsigned int protocol, bool verify, lc_callback cb,
	void *cb_arg, uint32_t cb_stage)
{
	uint32_t cb_count = 0;
	const unsigned int max_chunk_len = protocol == 0 ? 700 : 1022;

	/*
	 * This is a mapping of the lower-half of the first command byte to
	 * the size of the total command to be sent.
	 *
	 * See specs/protocol.txt for more * info.
	 */
	// Protocol 0 (745, safe mode)
	static const unsigned int dl0[16] =
		{ 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14 };
	// All other protocols
	static const unsigned int dlx[16] =
		{ 0, 0, 1, 2, 3, 4, 5, 6, 14, 30, 62, 0, 0, 0, 0, 0 };
	const unsigned int *rxlenmap = protocol ? dlx : dl0;

	uint8_t *pr = rd;
	const uint32_t end = addr+len;
	unsigned int bytes_read = 0;
	int err = 0;

	do {
		static uint8_t cmd[64] = {0};
		cmd[0] = COMMAND_READ_FLASH | 0x05;
		cmd[1] = (addr >> 16) & 0xFF;
		cmd[2] = (addr >> 8) & 0xFF;
		cmd[3] = addr & 0xFF;
		unsigned int chunk_len = end-addr;
		if (chunk_len > max_chunk_len)
			chunk_len = max_chunk_len;
		cmd[4] = (chunk_len >> 8) & 0xFF;
		cmd[5] = chunk_len & 0xFF;

		if ((err = HID_WriteReport(cmd)))
			break;

		uint8_t seq = 1;

		do {
			uint8_t rsp[68];
			if ((err = HID_ReadReport(rsp)))
				break;

			const uint8_t r = rsp[0] & COMMAND_MASK;

			if (r == RESPONSE_READ_FLASH_DATA) {
				if (seq != rsp[1]) {
					err = LC_ERROR;
					debug("Invalid sequence %02X %02x",
						seq, rsp[1]);
					break;
				}
				seq += 0x11;
				const unsigned int rxlen =
					rxlenmap[rsp[0] & LENGTH_MASK];
				if (rxlen) {
					if (verify) {
						if (memcmp(pr, rsp+2, rxlen)) {
							debug("Verify fail");
							err = LC_ERROR_VERIFY;
							break;
						}
					} else {
						memcpy(pr, rsp+2, rxlen);
					}
					pr += rxlen;
					addr += rxlen;
					bytes_read += rxlen;
				}
			} else if (r == RESPONSE_DONE) {
				break;
			} else {
				debug("Invalid response [%02X]", rsp[0]);
				err = LC_ERROR;
			}
		} while (err == 0);

		if (cb) {
			cb(cb_stage, cb_count++, bytes_read, len,
				LC_CB_COUNTER_TYPE_BYTES, cb_arg, NULL);
		}
	} while (err == 0 && addr < end);

	return err;
}

int CRemote::InvalidateFlash(lc_callback cb, void *cb_arg, uint32_t lc_stage)
{
	const uint8_t ivf[64]={ COMMAND_WRITE_MISC | 0x01,
				COMMAND_MISC_INVALIDATE_FLASH };
	int err;

	if (cb)
		cb(lc_stage, 0, 0, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	if ((err = HID_WriteReport(ivf)))
		return err;

	if (cb)
		cb(lc_stage, 1, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	uint8_t rsp[68];
	if ((err = HID_ReadReport(rsp)))
		return err;

	if ((rsp[0] & COMMAND_MASK) != RESPONSE_DONE ||
		(rsp[1] & COMMAND_MASK) != COMMAND_WRITE_MISC) {
		return 1;
	}

	if (cb)
		cb(lc_stage, 2, 2, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	return 0;
}


int CRemote::EraseFlash(uint32_t addr, uint32_t len,  const TRemoteInfo &ri,
	lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	const unsigned int *sectors = ri.flash->sectors;
	const unsigned int flash_base = ri.arch->flash_base;

	const uint32_t end = addr + len;

	int err = 0;
	uint32_t num_sectors = 0;

	uint32_t n = 0;

	// skip to where we need to start writing
	while (sectors[n] + flash_base < addr) {
		n++;
	}
	// start on the NEXT one
	n++;

	num_sectors = n;
	while (sectors[num_sectors] + flash_base < end) {
		num_sectors++;
	}
	num_sectors -= n - 1;

	uint32_t sector_begin = sectors[n-1] + flash_base;
	uint32_t sector_end = sectors[n] + flash_base;

	for (uint32_t i = 0; i < num_sectors; i++) {
		static uint8_t erase_cmd[64] = {0};
		erase_cmd[0] = COMMAND_ERASE_FLASH;
		erase_cmd[1] = (sector_begin >> 16) & 0xFF;
		erase_cmd[2] = (sector_begin >> 8) & 0xFF;
		erase_cmd[3] = sector_begin & 0xFF;

		if ((err = HID_WriteReport(erase_cmd)))
			break;

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp, 5000)))
			break;

		if (cb) {
			cb(cb_stage, i, i+1, num_sectors,
				LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
		}
		debug("erase sector %2i: %06X - %06X", n, sector_begin,
			sector_end);
		sector_begin = sector_end;
		sector_end = sectors[++n] + flash_base;
	}

	return err;
}

int CRemote::PrepFirmware(const TRemoteInfo &ri, lc_callback cb, void *cb_arg,
        uint32_t cb_stage)
{
	int err = 0;
	uint8_t data[1] = { 0x00 };

	if (cb)
		cb(cb_stage, 0, 0, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	if (ri.arch->firmware_update_base == ri.arch->firmware_base) {
		/*
		 * The preperation for where the staging area IS the config
		 * area.
		 *    restart config
		 *    write "1" to flash addr 200000
		 */
		if ((err = WriteMiscByte(0x09, 1, COMMAND_MISC_RESTART_CONFIG,
				data)))
			return LC_ERROR;

		if (cb)
			cb(cb_stage, 1, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
				NULL);

		if ((err = WriteFlash(0x200000, 1, data, ri.protocol, NULL,
				NULL, 0)))
			return LC_ERROR;
	} else {
		/*
		 * The preperation for where the staging area is distinct.
		 *    write "1" to ram addr 0
		 *    read it back
		 */
		if ((err = WriteRam(0, 1, data)))
			return LC_ERROR_WRITE;

		if (cb)
			cb(cb_stage, 1, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
				NULL);

		if ((err = ReadRam(0, 1, data)))
			return LC_ERROR_WRITE;
		if (data[0] != 0)
			return LC_ERROR_VERIFY;
	}
	if (cb)
		cb(cb_stage, 2, 2, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	return 0;
}

int CRemote::FinishFirmware(const TRemoteInfo &ri, lc_callback cb, void *cb_arg,
        uint32_t cb_stage)
{
	int err = 0;

	if (cb)
		cb(cb_stage, 0, 0, 3, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	uint8_t data[1];
	if (ri.arch->firmware_update_base == ri.arch->firmware_base) {
		data[0] = 0x02;
		if ((err = WriteFlash(0x200000, 1, data, ri.protocol, NULL,
			NULL, 0)))
			return LC_ERROR;
		if (cb)
			cb(cb_stage, 1, 1, 3, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
				NULL);
	} else {
		data[0] = 0x02;
		if ((err = WriteRam(0, 1, data))) {
			debug("Failed to write 2 to RAM 0");
			return LC_ERROR_WRITE;
		}
		if (cb)
			cb(cb_stage, 1, 1, 3, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
				NULL);
		if ((err = ReadRam(0, 1, data))) {
			debug("Failed to from RAM 0");
			return LC_ERROR_WRITE;
		}
		if (cb)
			cb(cb_stage, 2, 2, 3, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
				NULL);
		if (data[0] != 2) {
			printf("byte is %d\n", data[0]);
			debug("Finalize byte didn't match");
			return LC_ERROR_VERIFY;
		}
	}
	if (cb)
		cb(cb_stage, 3, 3, 3, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	return 0;
}

int CRemote::PrepConfig(const TRemoteInfo &ri, lc_callback cb, void *cb_arg,
	uint32_t cb_stage)
{
	int err;
	uint8_t data_zero[1] = { 0x00 };

	if (ri.architecture != 14) {
		if (cb) {
			cb(cb_stage, 0, 0, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
				NULL);
			cb(cb_stage, 1, 1, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
				NULL);
		}
		return 0;
	}

	if (cb)
		cb(cb_stage, 0, 0, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	if ((err = WriteMiscByte(0x02, 1, COMMAND_MISC_RESTART_CONFIG, data_zero))) {
		return err;
	}

	if (cb)
		cb(cb_stage, 1, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	if ((err = WriteMiscByte(0x05, 1, COMMAND_MISC_RESTART_CONFIG, data_zero))) {
		return err;
	}

	if (cb)
		cb(cb_stage, 2, 2, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	return 0;
}

int CRemote::FinishConfig(const TRemoteInfo &ri, lc_callback cb, void *cb_arg,
        uint32_t cb_stage)
{
	int err;
	uint8_t data_one[1]  = { 0x01 };
	uint8_t data_zero[1] = { 0x00 };

	if (ri.architecture != 14) {
		return 0;
	}

	if (cb)
		cb(cb_stage, 0, 0, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	if ((err = WriteMiscByte(0x03, 1, COMMAND_MISC_RESTART_CONFIG, data_one))) {
		return err;
	}

	if (cb)
		cb(cb_stage, 1, 1, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	if ((err = WriteMiscByte(0x06, 1, COMMAND_MISC_RESTART_CONFIG, data_zero))) {
		return err;
	}

	if (cb)
		cb(cb_stage, 2, 2, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);

	return 0;
}

int CRemote::WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr)
{
	return WriteMiscByte(addr, len, COMMAND_MISC_RAM, wr);
}

int CRemote::ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd)
{
	return ReadMiscByte(addr, len, COMMAND_MISC_RAM, rd);
}

int CRemote::WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr,
	unsigned int protocol, lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	uint32_t cb_count = 0;
	const unsigned int max_chunk_len =
		protocol == 0 ? 749 : 3150;

	/* mapping of lenghts - see specs/protocol.txt */
	static const unsigned int txlenmap0[] =
		{ 0x07, 7, 6, 5, 4, 3, 2, 1 };
	static const unsigned int txlenmapx[] =
		{ 0x0A, 63, 31, 15, 7, 6, 5, 4, 3, 2, 1 };
	const unsigned int *txlenmap = protocol ? txlenmapx : txlenmap0;

	const uint8_t *pw = wr;
	const uint32_t end = addr+len;
	unsigned int bytes_written = 0;
	int err = 0;

	do {
		static uint8_t write_setup_cmd[64] = {0};
		write_setup_cmd[0] = COMMAND_WRITE_FLASH | 0x05;
		write_setup_cmd[1] = (addr >> 16) & 0xFF;
		write_setup_cmd[2] = (addr >> 8) & 0xFF;
		write_setup_cmd[3] = addr & 0xFF;
		uint32_t chunk_len = end - addr;
		if (chunk_len > max_chunk_len)
			chunk_len = max_chunk_len;
		write_setup_cmd[4] = (chunk_len >> 8) & 0xFF;
		write_setup_cmd[5] = chunk_len & 0xFF;

		if ((err = HID_WriteReport(write_setup_cmd)))
			break;

		while (chunk_len) {
			unsigned int n = txlenmap[0];
			unsigned int i = 1;
			while (chunk_len < txlenmap[i]) {
				++i;
				--n;
			}
			unsigned int block_len = txlenmap[i];
			uint8_t wd[64] = {0};
			wd[0] = COMMAND_WRITE_FLASH_DATA | n;
			memcpy(wd+1, pw, block_len);
			HID_WriteReport(wd);
			pw += block_len;
			addr += block_len;
			bytes_written += block_len;
			chunk_len -= block_len;
		}

		uint8_t end_cmd[64] = { COMMAND_DONE, COMMAND_WRITE_FLASH };
		HID_WriteReport(end_cmd);

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp, 5000)))
			break;

		if (cb) {
			cb(cb_stage, cb_count++, bytes_written, len,
				LC_CB_COUNTER_TYPE_BYTES, cb_arg, NULL);
		}
	} while (addr < end);

	return err;
}

int CRemote::ReadMiscByte(uint8_t addr, uint32_t len,
		uint8_t kind, uint8_t *rd)
{
	uint8_t rmb[64] = { COMMAND_READ_MISC | 0x02, kind, 0 };

	while (len--) {
		rmb[2] = addr++;

		int err;
		if ((err = HID_WriteReport(rmb)))
			return err;

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp)))
			return err;

		if (rsp[0] != (RESPONSE_READ_MISC_DATA | 0x02) ||
			rsp[1] != kind)
			return 1;

		*rd++ = rsp[2];
	}
	return 0;
}

int CRemote::ReadMiscWord(uint16_t addr, uint32_t len,
		uint8_t kind, uint16_t *rd)
{
	uint8_t rmw[64] = { COMMAND_READ_MISC | 0x03, kind, 0, 0 };

	while (len--) {
		rmw[2] = addr >> 8;
		rmw[3] = addr & 0xFF;
		++addr;

		int err;
		if ((err = HID_WriteReport(rmw)))
			return err;

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp)))
			return err;

		// WARNING: The 880 responds with C2 rather than C3
		if ((rsp[0] & COMMAND_MASK) != RESPONSE_READ_MISC_DATA ||
			rsp[1] != kind) {
			return 1;
		}

		*rd++ = (rsp[2] << 8) | rsp[3];
	}
	return 0;
}

int CRemote::WriteMiscByte(uint8_t addr, uint32_t len,
		uint8_t kind, uint8_t *wr)
{
	uint8_t wmb[64] = {0};
	wmb[0] = COMMAND_WRITE_MISC | 0x03;
	wmb[1] = kind;

	while (len--) {
		wmb[2] = addr++;
		wmb[3] = *wr++;

		int err;
		if ((err = HID_WriteReport(wmb)))
			return err;

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp)))
			return err;
		if ((rsp[0] & COMMAND_MASK) != RESPONSE_DONE ||
			rsp[1] != COMMAND_WRITE_MISC) {
			return 1;
		}
	}
	return 0;
}

int CRemote::WriteMiscWord(uint16_t addr, uint32_t len,
		uint8_t kind, uint16_t *wr)
{
	uint8_t wmw[64] = {0};
	wmw[0] = COMMAND_WRITE_MISC | 0x05;
	wmw[1] = kind;

	while (len--) {
		wmw[2] = addr >> 8;
		wmw[3] = addr & 0xFF;
		++addr;
		wmw[4] = *wr >> 8;
		wmw[5] = *wr & 0xFF;
		++wr;

		int err;
		if ((err = HID_WriteReport(wmw)))
			return err;

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp)))
			return err;
		if ((rsp[0] & COMMAND_MASK) != RESPONSE_DONE ||
			rsp[1] != COMMAND_WRITE_MISC) {
			return 1;
		}
	}
	return 0;
}


int CRemote::GetTime(const TRemoteInfo &ri, THarmonyTime &ht)
{
	int err = 0;

	if (ri.architecture < 8) {
		uint8_t tsv[8];
		err = ReadMiscByte(0, 6, COMMAND_MISC_STATE, tsv);
		ht.second = tsv[0];
		ht.minute = tsv[1];
		ht.hour = tsv[2];
		ht.dow = 7;
		ht.day = 1 + tsv[3];
		ht.month = 1 + tsv[4];
		ht.year = 2000 + tsv[5];
	} else {
		uint16_t tsv[8];
		err = ReadMiscWord(0, 7, COMMAND_MISC_STATE, tsv);
		ht.second = tsv[0];
		ht.minute = tsv[1];
		ht.hour = tsv[2];
		ht.day = 1 + tsv[3];
		ht.dow = tsv[4] & 7;
		ht.month = 1 + tsv[5];
		ht.year = 2000 + tsv[6];
	}

	ht.utc_offset = 0;
	ht.timezone = "";

	return err;
}

int CRemote::SetTime(const TRemoteInfo &ri, const THarmonyTime &ht,
	lc_callback cb, void *cb_arg, uint32_t cb_stage)
{
	int err = 0;
	uint8_t rsp[68];
	int cb_count = 0;

	if (cb)
		cb(cb_stage, cb_count++, 0, 2, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
			NULL);

	if (ri.architecture < 8) {
		uint8_t tsv[8];
		tsv[0] = 0;
		tsv[1] = ht.minute;
		tsv[2] = ht.hour;
		tsv[3] = ht.day - 1;
		tsv[4] = ht.month  -1;
		tsv[5] = ht.year - 2000;
		if ((err = WriteMiscByte(0, 6, COMMAND_MISC_STATE, tsv)))
			return err;
		if (cb)
			cb(cb_stage, cb_count++, 1, 3, LC_CB_COUNTER_TYPE_STEPS,
				cb_arg, NULL);

		tsv[0] = ht.second;
		err = WriteMiscByte(0, 1, COMMAND_MISC_STATE, tsv);
	} else {
		uint16_t tsv[8];
		tsv[0] = 0;
		tsv[1] = ht.minute;
		tsv[2] = ht.hour;
		tsv[3] = ht.day-1;
		tsv[4] = ht.dow;
		tsv[5] = ht.month-1;
		tsv[6] = ht.year-2000;
		if ((err = WriteMiscWord(0, 7, COMMAND_MISC_STATE, tsv)))
			return err;
		if (cb)
			cb(cb_stage, cb_count++, 1, 3, LC_CB_COUNTER_TYPE_STEPS,
				cb_arg, NULL);
		tsv[0] = ht.second;
		if ((err = WriteMiscWord(0, 1, COMMAND_MISC_STATE, tsv)))
			return err;

		// Send Recalc Clock command for 880 only (not 360/520/550)
		if (ri.architecture == 8) {
			static const uint8_t rcc[64] = {
				COMMAND_WRITE_MISC | 0x01,
				COMMAND_MISC_CLOCK_RECALCULATE };
			err = HID_WriteReport(rcc);
		}
	}
	if (cb)
		cb(cb_stage, cb_count++, 2, 3, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
			NULL);

        if (err != 0) {
		return err;
	}

	/*
	 * For some models, they return a RESPONSE_DONE, and we have to read
	 * it otherwise otherwise next command will fail.
	 * However, other devices don't return this, in which case the read
	 * failes.
	 * So, if the read succeeds, we check the response, otherwise we just
	 * move on with life.
	 */
	err = HID_ReadReport(rsp);
	if (err == 0) {
		if ((rsp[0] & COMMAND_MASK) != RESPONSE_DONE) {
			err = 1;
		}
	} else {
		err = 0;
	}
	if (cb)
		cb(cb_stage, cb_count++, 3, 3, LC_CB_COUNTER_TYPE_STEPS, cb_arg,
			NULL);

	return err;
}

bool check_seq(int received_seq, uint8_t &expected_seq)
{
	if (received_seq == expected_seq) {
		return true;
	}

	if (received_seq == 0x1f && expected_seq == 0x10) {
		/*
		 * Handle 880 SNAFU
		 *
		 * Does this indicate a bad learn???
		 * Needs more testing
		 */
		debug("sequence glitch!");
		expected_seq += 0x0F;
		return true;
	} else {
		debug("\nInvalid sequence %02X %02x", expected_seq,
			received_seq);
		return false;
	}
}

int _handle_ir_response(uint8_t rsp[64], uint32_t &ir_word,
	uint32_t &t_on, uint32_t &t_off, uint32_t &t_total,
	uint32_t &ir_count, uint32_t *&ir_signal, uint32_t &freq)
{
	const uint32_t len = rsp[63];
	if ((len & 1) != 0) {
		return 3;	// Invalid length
	}

	for (uint32_t u = 2; u < len; u += 2) {
		const uint32_t t = rsp[u] << 8 | rsp[1+u];
		if (ir_word > 2) {
			/*
			 * For ODD words, t is the total time, we'll
			 * update the total OFF time and be done.
			 *
			 * For EVEN words, t is just the ON time
			 * -- IF we have any ON time, then we go ahead
			 *    and record the off and on times we should
			 *    have now gathered.
			 *
			 * Why do we differentiate between even/odd?
			 * Perhaps just to make sure we've had two
			 * cycles, and thus have off/on?
			*/
			if (ir_word & 1) {
				// t == on + off time
				if (t_on) {
					t_off = t - t_on;
				} else {
					t_off += t;
				}
			} else {
				// t == on time
				t_on = t;
				if (t_on) {
					debug("-%i\n", t_off);
					if (ir_count < MAX_IR_SIGNAL_LENGTH) {
						ir_signal[ir_count++] = t_off;
					}
					debug("+%i\n", t_on);
					if (ir_count < MAX_IR_SIGNAL_LENGTH) {
						ir_signal[ir_count++] = t_on;
					}
					t_total += t_off + t_on;
				}
			}
		} else {
			/*
			 * For the first 3 words...
			 *  the first one, we ignore, apparently
			 *  the second one, we start keeping track of ON time
			 *  the third one, we have enough data to calculate the 
			 *    frequency and record the on time we've calculated
			 */
			switch (ir_word) {
				case 0:	// ???
					break;
				case 1:	// on time of first burst
					t_on = t;
					break;
				case 2:	// carrier cycle count of first burst
					if (t_on) {
						freq = static_cast<uint32_t>(
							static_cast<uint64_t>(t)
								*1000000/(t_on));
						debug("%i Hz", freq);
						debug("+%i", t_on);
						ir_signal[ir_count++] = t_on;
					}
					break;
			}
		}
		++ir_word;
	}
	return 0;
}


int CRemote::LearnIR(uint32_t *freq, uint32_t **ir_signal,
		uint32_t *ir_signal_length, lc_callback cb, void *cb_arg,
		uint32_t cb_stage)
{
	int err = 0;
	uint8_t rsp[68];

	static const uint8_t start_ir_learn[64] = { COMMAND_START_IRCAP };
	static const uint8_t stop_ir_learn[64] = { COMMAND_STOP_IRCAP };

	if (cb) {
		cb(cb_stage, 0, 0, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
	}

	if (HID_WriteReport(start_ir_learn) != 0) {
		return LC_ERROR_WRITE;
	}

	uint8_t seq = 0;
	// Count of how man IR words we've received.
	uint32_t ir_word = 0;
	// Time button is on and off
	uint32_t t_on = 0;
	uint32_t t_off = 0;
	// total duration of received signal:
	// abort when > MAX_IR_SIGNAL_DURATION
	uint32_t t_total = 0;

	*ir_signal_length = 0;
	*ir_signal = new uint32_t[MAX_IR_SIGNAL_LENGTH];
	/*
	 * Caller is responsible for deallocation of *ir_signal after use.
	 *
	 * Loop while we haven not:
	 * - any error (including signal duration and buffer overflow)
	 * - signal interrupted for IR_LEARN_DONE_TIMEOUT or longer
	 */
	while ((err == 0) && (t_off < IR_LEARN_DONE_TIMEOUT * 1000)) {
		if ((err = HID_ReadReport(rsp, ir_word ?
			IR_LEARN_DONE_TIMEOUT : IR_LEARN_START_TIMEOUT))) {
			err = LC_ERROR_READ;
			break;
		}
		const uint8_t r = rsp[0] & COMMAND_MASK;
		if (r == RESPONSE_IRCAP_DATA) {
			if (!check_seq(rsp[1], seq)) {
				err = LC_ERROR;
				break;
			}
			seq += 0x10;
			/*
			 * This will handle the IR response including updating
			 * t_off so we can exit the loop if long enough time
			 * goes by without action.
			 */
			err = _handle_ir_response(rsp, ir_word, t_on, t_off,
				t_total, *ir_signal_length, *ir_signal,	*freq);
			if (err != 0) {
				break;
			}
		} else if (r == RESPONSE_DONE) {
			break;
		} else {
			debug("Invalid response [%02X]", rsp[1]);
			err = LC_ERROR;
		}
		/* check for overflow: */
		if ((t_total > MAX_IR_SIGNAL_DURATION * 1000)
			|| (*ir_signal_length > MAX_IR_SIGNAL_LENGTH)) {
			err = LC_ERROR_IR_OVERFLOW;
		}
	}

	if ((err == 0) && (*ir_signal_length > 0)) {
		/* we have actually got some signal */
		if (t_off) {
			debug("-%i", t_off);
		}
		/* make sure we record a final off */
		if (*ir_signal_length < MAX_IR_SIGNAL_LENGTH) {
			(*ir_signal)[(*ir_signal_length)++] = t_off;
		}
	}

	if (HID_WriteReport(stop_ir_learn) != 0) {
		err = LC_ERROR_WRITE;
	}

	/* flush HID buffer until empty or RESPONSE_DONE: */
	do {
		if (HID_ReadReport(rsp, IR_LEARN_DONE_TIMEOUT) != 0) {
			err = LC_ERROR_READ;
			break;
		}
	} while ((rsp[0] & COMMAND_MASK) != RESPONSE_DONE);

	if (cb && !err) {
		cb(cb_stage, 1, 1, 1, LC_CB_COUNTER_TYPE_STEPS, cb_arg, NULL);
	}

	return err;
}
