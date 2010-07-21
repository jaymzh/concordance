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
 * (C) Copyright Phil Dibowitz 2010
 */

#include <string.h>
#include "libconcord.h"
#include "lc_internal.h"
#include "hid.h"
#include "remote.h"
#include "usblan.h"
#include "protocol_z.h"

int CRemoteZ_HID::UDP_Write(uint8_t typ, uint8_t cmd, uint32_t len,
	uint8_t *data)
{
	if (len > 60)
		return LC_ERROR;
	uint8_t pkt[68];
	pkt[0] = 3+len;
	pkt[1] = 1; // UDP
	pkt[2] = typ;
	pkt[3] = 0xFF & cmd;
	if (data && len)
		memcpy(pkt + 4, data, len);

	debug("Writing packet. First 5 bytes: %02X %02X %02X %02X %02X\n",
		pkt[0], pkt[1], pkt[2], pkt[3], pkt[4]);

	return HID_WriteReport(pkt);
}

int CRemoteZ_HID::UDP_Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
	uint8_t pkt[68];
	int err;
	if ((err = HID_ReadReport(pkt))) {
		return LC_ERROR_READ;
	}
	debug("Reading packet. First 5 bytes: %02X %02X %02X %02X %02X\n",
		pkt[0], pkt[1], pkt[2], pkt[3], pkt[4]);
	if (pkt[0] < 4) {
		return LC_ERROR;
	}
	if (pkt[0] > 4) {
		status = pkt[4];
	}
	len = pkt[0] - 4;
	//if(!len) return 0;
	//memcpy(data, pkt + 6, len);
	memcpy(data, pkt + 1, len + 3);
	return 0;
}

int CRemoteZ_HID::TCP_Write(uint8_t typ, uint8_t cmd, uint32_t len,
	uint8_t *data)
{
	return 0;
}
int CRemoteZ_HID::TCP_Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
	return 0;
}

int CRemoteZ_HID::Write(uint8_t typ, uint8_t cmd, uint32_t len,
	uint8_t *data)
{
	return UDP_Write(typ, cmd, len, data);
}

int CRemoteZ_HID::Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
	return UDP_Read(status, len, data);
}

int CRemoteZ_HID::ParseParams(uint32_t len, uint8_t *data, TParamList &pl)
{
	debug("ParseParams, %02x %02x %02x %02x %02x %02x\n", data[0],
		data[1], data[2], data[3], data[4], data[5]);
	switch (data[2]) {
		case COMMAND_GET_SYSTEM_INFO:
			pl.count = 8;
			pl.p[0] = data + 4;
			pl.p[1] = data + 6;
			pl.p[2] = data + 8;
			pl.p[3] = data + 10;
			pl.p[4] = data + 12;
			pl.p[5] = data + 14;
			pl.p[6] = data + 15;
			pl.p[7] = data + 17;
			break;
		case COMMAND_GET_CURRENT_TIME:
			pl.count = (len > 16) ? 12 : 8;
			pl.p[0] = data + 4;
			pl.p[1] = data + 6;
			pl.p[2] = data + 7;
			pl.p[3] = data + 8;
			pl.p[4] = data + 9;
			pl.p[5] = data + 10;
			pl.p[6] = data + 11;
			pl.p[7] = data + 12;
			pl.p[8] = data + 14;
			pl.p[9] = data + 16;
			pl.p[10] = data + 18;
			pl.p[11] = data + 20;
			break;
		case COMMAND_GET_GUID:
			pl.count = 1;
			pl.p[0] = data + 4;
			break;
	}
	return 0;
}


int CRemoteZ_TCP::Write(uint8_t typ, uint8_t cmd, uint32_t len,
	uint8_t *data)
{
	if (len > 60) {
		return LC_ERROR;
	}

	static const uint8_t service_type = SERVICE_FAMILY_CLIENT;
	const bool request = (typ == TYPE_REQUEST);
	const uint8_t status = STATUS_OK;

	uint8_t pkt[68];
	pkt[0] = service_type << 4 | (cmd >> 8) & 0x0F;
	pkt[1] = cmd & 0xFF;
	pkt[2] = request ? 0x80 : (status & 0x7F);

	if (len && data) {
		memcpy(pkt + 3, data, len);
		len += 3;
	} else {
		pkt[3] = 0x00;	// Param count
		len = 4;
	}

	return UsbLan_Write(len, pkt);
}

int CRemoteZ_TCP::Read(uint8_t &status, uint32_t &len, uint8_t *data)
{
	uint8_t buf[1600];
	len = sizeof(buf);
	int err;
	if ((err = UsbLan_Read(len, buf)))
		return err;

	memcpy(data, buf, len);

	return err;
}

int CRemoteZ_TCP::ParseParams(uint32_t len, uint8_t *data, TParamList &pl)
{
	unsigned int n = 0;
	unsigned int i = 4;
	while (i < len) {
		unsigned int param_len = data[i];
		switch (param_len & 0xC0) {
			case 0x00:
			case 0x80:
				param_len &= 0x3F;
				break;
			case 0x40:
				param_len = (param_len & 0x3F) * 4;
				break;
			case 0xC0:
				param_len = (param_len & 0x3F) * 512;
				break;
		}
		++i;
		pl.p[n++] = data+i;
#ifdef _DEBUG
		fprintf(stderr, "DEBUG (%s): %3i:", __FUNCTION__, param_len);
		for(unsigned int j = 0; j < param_len; ++j)
			fprintf(stderr, " %02X",data[i+j]);
		fprintf(stderr, "\n");
#endif
		i += param_len;
	}

	//data[3];	// Number of parameters
	pl.count = n;

	return 0;
}


int CRemoteZ_Base::Reset(uint8_t kind)
{
	int err = 0;
	if (kind != 2) {
		return LC_ERROR;
	}

	if ((err = Write(TYPE_REQUEST, COMMAND_Z_RESET))) {
		debug("Failed to write to remote");
		return LC_ERROR_WRITE;
	}
	uint8_t rsp[60];
	unsigned int len;
	uint8_t status;
	if ((err = Read(status, len, rsp))) {
		debug("Failed to read to remote");
		return LC_ERROR_READ;
	}
	return 0;
}

int CRemoteZ_Base::GetIdentity(TRemoteInfo &ri, THIDINFO &hid,
	lc_callback cb, void *cb_arg)
{
	int err = 0;
	if ((err = Write(TYPE_REQUEST, COMMAND_GET_SYSTEM_INFO))) {
		debug("Failed to write to remote");
		return LC_ERROR_WRITE;
	}
	uint8_t rsp[60];
	unsigned int len;
	uint8_t status;
	if ((err = Read(status, len, rsp))) {
		debug("Failed to read from remote");
		return LC_ERROR_READ;
	}

	CRemoteZ_Base::TParamList pl;
	ParseParams(len, rsp, pl);

	if (hid.pid == 0) {		// 1000
		hid.vid = GetWord(pl.p[0]);
		hid.pid = GetWord(pl.p[1]);
		hid.ver = 0;
		hid.irl = 0;
		hid.orl = 0;
		hid.frl = 0;
		hid.mfg = "Logitech";
		hid.prod = "Harmony 1000";
		ri.flash_mfg = 0;	// todo
		ri.flash_id = 0;	// todo
	} else {				// Not 1000
		ri.flash_mfg = 0x01;// todo
		ri.flash_id = 0x49;	// todo
	}
	ri.architecture = GetWord(pl.p[2]);
	ri.fw_ver_major = GetWord(pl.p[3]);
	ri.fw_ver_minor = GetWord(pl.p[4]);
	ri.fw_type = *pl.p[5];
	ri.skin = GetWord(pl.p[6]);
	const unsigned int hw = GetWord(pl.p[7]);
	ri.hw_ver_major = hw >> 8;		// ???
	ri.hw_ver_minor = (hw >> 4) & 0x0F;	// ???
	// hw_ver_micro = hw & 0x0F		// ???
	//ri.hw_ver_major = hw >> 4;
	//ri.hw_ver_minor = hw & 0x0F;
	ri.protocol = 0;

	setup_ri_pointers(ri);
	
	if ((err = Write(TYPE_REQUEST, COMMAND_GET_GUID))) {
		debug("Failed to write to remote");
		return LC_ERROR_WRITE;
	}
	if ((err = Read(status, len, rsp))) {
		debug("Failed to read from remote");
		return LC_ERROR_READ;
	}

	ParseParams(len, rsp, pl);

	make_serial(pl.p[0], ri);

	ri.config_bytes_used = 0;
	ri.max_config_size = 1;

#if 0	// Get region info - 1000 only!
	uint8_t rr[] = { 1, 1, 1 }; // AddByteParam(1);
	if ((err = Write(TYPE_REQUEST, COMMAND_GET_REGION_IDS, 3, rr))) {
		debug("Failed to write to remote");
		return LC_ERROR;
	}
	uint8_t rgn[64];
	if ((err = Read(status, len, rgn))) {
		debug("Failed to read from remote");
		return LC_ERROR;
	}
	ParseParams(len, rgn, pl);
	if (pl.count == 1) {
		const unsigned int rc = *(pl.p[0]-1) & 0x3F;
		for(unsigned int r = 0; r < rc; ++r) {
			const uint8_t rn = *(pl.p[0]+r);
			printf("Region %i\n", rn);
			uint8_t rv[] = { 1, 1, rn }; // AddByteParam(rn);
			if ((err = Write(TYPE_REQUEST,
					COMMAND_GET_REGION_VERSION, 3, rv))) {
				debug("Failed to write to remote");
				return LC_ERROR;
			}
			uint8_t rgv[64];
			if ((err = Read(status, len, rgv))) {
				debug("Failed to read from remote");
				return LC_ERROR;
			}
			CRemoteZ_Base::TParamList rp;
			ParseParams(len,rgv,rp);
		}
	}
#endif

	return 0;
}

int CRemoteZ_Base::ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
	unsigned int protocol, bool verify, lc_callback cb,
	void *cb_arg)
{
	return 0;
}

int CRemoteZ_Base::InvalidateFlash(void)
{
	return 0;
}

int CRemoteZ_Base::EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri,
	lc_callback cb, void *cb_arg)
{
	return 0;
}

int CRemoteZ_Base::WriteFlash(uint32_t addr, const uint32_t len,
	const uint8_t *wr, unsigned int protocol, lc_callback cb,
	void *arg)
{
	return 0;
}

int CRemoteZ_Base::WriteRam(uint32_t addr, const uint32_t len, uint8_t *wr)
{
	return 0;
}

int CRemoteZ_Base::ReadRam(uint32_t addr, const uint32_t len, uint8_t *rd)
{
	return 0;
}

int CRemoteZ_Base::RestartConfig()
{
	return 0;
}

int CRemoteZ_Base::GetTime(const TRemoteInfo &ri, THarmonyTime &ht)
{
	int err = 0;
	if ((err = Write(TYPE_REQUEST, COMMAND_GET_CURRENT_TIME))) {
		debug("Failed to write to remote");
		return LC_ERROR_WRITE;
	}
	uint8_t time[60];
	unsigned int len;
	uint8_t status;
	if ((err = Read(status, len, time))) {
		debug("Failed to read to remote");
		return LC_ERROR_READ;
	}
	CRemoteZ_Base::TParamList pl;
	ParseParams(len, time, pl);

	ht.year = GetWord(pl.p[0]);
	ht.month = *pl.p[1];
	ht.day = *pl.p[2];
	ht.hour = *pl.p[3];
	ht.minute = *pl.p[4];
	ht.second = *pl.p[5];
	ht.dow = *pl.p[6]&7;
	ht.utc_offset = static_cast<int16_t>(GetWord(pl.p[7]));
	if (pl.count > 11) {
		ht.timezone = reinterpret_cast<char*>(pl.p[11]);
	} else {
		ht.timezone = "";
	}

	return 0;
}

int CRemoteZ_Base::SetTime(const TRemoteInfo &ri, const THarmonyTime &ht)
{
	int err = 0;

	uint8_t tsv[16] = { ht.year, 0, // 2 bytes
		ht.month,
		ht.day,
		ht.hour,
		ht.minute,
		ht.second,
		ht.dow,
		// utcOffset
		ht.utc_offset, 0, // 2 bytes
		// 0s
		0, 0, // 2 bytes
		0, 0, // 2 bytes
		0, 0, // 2 bytes
		// ht.timezone.c_str()
		};

	if ((err = Write(TYPE_REQUEST, COMMAND_UPDATE_TIME, 16, tsv))) {
		debug("Failed to write to remote");
		return LC_ERROR_WRITE;
	}

	uint8_t rsp[60];
	unsigned int len;
	uint8_t status;
	if ((err = Read(status, len, rsp))) {
		debug("failed to read from remote");
		return LC_ERROR_READ;
	}

	if (rsp[1] != TYPE_RESPONSE ||
	    rsp[2] != COMMAND_UPDATE_TIME) {
	    	return LC_ERROR;
	}

	return 0;
}

int CRemoteZ_Base::LearnIR(uint32_t *freq, uint32_t **ir_signal,
	uint32_t *ir_signal_length, lc_callback cb, void *cb_arg)
{
	return 0;
}
