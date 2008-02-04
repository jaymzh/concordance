/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General License for more details.
 *
 *  You should have received a copy of the GNU General License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  (C) Copyright Kevin Timmerman 2007
 */

#include "harmony.h"
#include "hid.h"
#include "remote.h"
#include "usblan.h"
#include "protocol_z.h"

int CRemoteZ_HID::UDP_Write(uint8_t typ, uint8_t cmd, unsigned int len,
	uint8_t *data)
{
	if (len > 60)
		return 1;
	uint8_t pkt[68];
	pkt[0] = 0;
	pkt[1] = 3 + len;
	pkt[2] = 1; // UDP
	pkt[3] = typ;
	pkt[4] = cmd;
	if (data && len)
		memcpy(pkt + 5, data, len);

	return HID_WriteReport(pkt);
}

int CRemoteZ_HID::UDP_Read(uint8_t &status, unsigned int &len, uint8_t *data)
{
	uint8_t pkt[68];
	int err;
	if ((err = HID_ReadReport(pkt)))
		return err;
	if (pkt[1] < 4)
		return 1;
	status = pkt[5];
	len = pkt[1] - 4;
	//if (!len) return 0;
	//memcpy(data, pkt + 6, len);
	memcpy(data, pkt + 2, len + 4);
	return 0;
}

// Should never be needed - psuedo-net over over HID only uses UDP
int CRemoteZ_HID::TCP_Write(uint8_t typ, uint8_t cmd, unsigned int len,
	uint8_t *data)
{
	return 0;
}

// Should never be needed - psuedo-net over over HID only uses UDP
int CRemoteZ_HID::TCP_Read(uint8_t &status, unsigned int &len, uint8_t *data)
{
	return 0;
}

int CRemoteZ_HID::Write(uint8_t typ, uint8_t cmd, unsigned int len,
	uint8_t *data)
{
	return UDP_Write(typ, cmd, len, data);
}

int CRemoteZ_HID::Read(uint8_t &status, unsigned int &len, uint8_t *data)
{
	return UDP_Read(status, len, data);
}

int CRemoteZ_HID::ParseParams(unsigned int len, uint8_t *data, TParamList &pl)
{
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
			pl.p[7] = data +12;
			pl.p[8] = data + 14;
			pl.p[9] = data + 16;
			pl.p[10] = data + 18;
			pl.p[11] = data + 20;
			break;
		case COMMAND_GET_GUID:
			pl.count = 1;
			pl.p[0] = data+4;
			break;
	}
	return 0;
}


int CRemoteZ_TCP::Write(uint8_t typ, uint8_t cmd, unsigned int len,
	uint8_t *data)
{
	if (len > 60)
		return 1;

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

int CRemoteZ_TCP::Read(uint8_t &status, unsigned int &len, uint8_t *data)
{
	uint8_t buf[1600];
	len = sizeof(buf);
	int err;
	if ((err = UsbLan_Read(len, buf)))
		return err;

#if 0
	// Show the received received data
	for(unsigned int i=0; i<len; ++i) printf("%02X ",buf[i]);
	printf("\n\n");
#endif

	memcpy(data, buf, len);

	return err;
}

int CRemoteZ_TCP::ParseParams(unsigned int len, uint8_t *data, TParamList &pl)
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
#if 1
		printf("%3i:", param_len);
		for(unsigned int j = 0; j < param_len; ++j)
			printf(" %02X",data[i+j]);
		printf("\n");
#endif
		i += param_len;
	}

	//data[3];	// Number of parameters
	pl.count = n;

	return 0;
}


int CRemoteZ_Base::Reset(uint8_t kind)
{
	if (kind != 2)
		return 1;

	Write(TYPE_REQUEST, COMMAND_RESET);
	uint8_t rsp[60];
	unsigned int len;
	uint8_t status;
	Read(status, len, rsp);
	return 0;
}

int CRemoteZ_Base::GetIdentity(TRemoteInfo &ri, THIDINFO &hid,
	void (*cb)(int,int,int,void*), void *arg)
{
	Write(TYPE_REQUEST, COMMAND_GET_SYSTEM_INFO);
	uint8_t rsp[60];
	unsigned int len;
	uint8_t status;
	Read(status, len, rsp);
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
	
	Write(TYPE_REQUEST, COMMAND_GET_GUID);
	Read(status, len, rsp);
	ParseParams(len, rsp, pl);

	make_serial(pl.p[0], ri);

	ri.config_bytes_used = 0;
	ri.max_config_size = 1;

#if 0	// Get region info - 1000 only!
	uint8_t rr[] = { 1, 1, 1 }; // AddByteParam(1);
	Write(TYPE_REQUEST, COMMAND_GET_REGION_IDS, 3, rr);
	uint8_t rgn[64];
	Read(status,len,rgn);
	ParseParams(len,rgn,pl);
	if (pl.count==1) {
		const unsigned int rc = *(pl.p[0]-1) & 0x3F;
		for(unsigned int r = 0; r<rc; ++r) {
			const uint8_t rn = *(pl.p[0]+r);
			printf("Region %i\n",rn);
			uint8_t rv[] = { 1, 1, rn }; // AddByteParam(rn);
			Write(TYPE_REQUEST, COMMAND_GET_REGION_VERSION, 3, rv);
			uint8_t rgv[64];
			Read(status,len,rgv);
			CRemoteZ_Base::TParamList rp;
			ParseParams(len,rgv,rp);
		}
	}
#endif

	return 0;
}

int CRemoteZ_Base::ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd,
	unsigned int protocol, bool verify, void (*cb)(int,int,int,void*),
	void *arg)
{
	return 0;
}

int CRemoteZ_Base::InvalidateFlash(void)
{
	return 0;
}

int CRemoteZ_Base::EraseFlash(uint32_t addr, uint32_t len, const TRemoteInfo &ri,
	void (*cb)(int,int,int,void*), void *cb_arg)
{
	return 0;
}

int CRemoteZ_Base::WriteFlash(uint32_t addr, const uint32_t len,
	const uint8_t *wr, unsigned int protocol, void (*cb)(int,int,int,void*),
	void *arg)
{
	return 0;
}

int CRemoteZ_Base::GetTime(const TRemoteInfo &ri, THarmonyTime &ht)
{
	Write(TYPE_REQUEST, COMMAND_GET_CURRENT_TIME);
	uint8_t time[60];
	unsigned int len;
	uint8_t status;
	Read(status,len,time);
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
	if(pl.count > 11)
		ht.timezone = reinterpret_cast<char*>(pl.p[11]);
	else
		ht.timezone = "";

	return 0;
}

int CRemoteZ_Base::SetTime(const TRemoteInfo &ri, const THarmonyTime &ht)
{
	printf("Set time is not currently supported for this mode remote\n");
	return 1;
}

int CRemoteZ_Base::LearnIR(string *learn_string)
{
	return 0;
}

