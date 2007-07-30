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
 *  (C) Copyright Kevin Timmerman 2007
 *  (C) Copyright Phil Dibowitz 2007
 */

#include "harmony.h"
#include "hid.h"
#include "protocol.h"
#include "remote.h"
#include "remote_info.h"

#include <iostream>

#ifdef WIN32
#include <conio.h>
#include <winsock.h>
extern HANDLE con;
extern CONSOLE_SCREEN_BUFFER_INFO sbi;
#endif


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

	ri.model = (ri.skin<max_model)
			? &ModelList[ri.skin] : &ModelList[max_model];
}

void make_guid(const uint8_t * const in, string& out)
{
	char x[48];
	sprintf(x,
	"{%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		in[3],in[2],in[1],in[0],in[5],in[4],in[7],in[6],
		in[8],in[9],in[10],in[11],in[12],in[13],in[14],in[15]);
	out=x;
}

void make_serial(uint8_t *ser, TRemoteInfo &ri)
{
	make_guid(ser   ,ri.serial[0]);
	make_guid(ser+16,ri.serial[1]);
	make_guid(ser+32,ri.serial[2]);
}


int CRemote::Reset(uint8_t kind)
{
	uint8_t reset_cmd[]={ 0, COMMAND_RESET, kind };

	return HID_WriteReport(reset_cmd);
}

/*
 * Send the GET_VERSION command to the remote, and read the response.
 *
 * Then populate our struct with all the relevant info.
 */
int CRemote::GetIdentity(TRemoteInfo &ri, THIDINFO &hid)
{
	int err = 0;

	const uint8_t qid[]={ 0x00, COMMAND_GET_VERSION };

	printf("Requesting Identity...\n");

	if ((err = HID_WriteReport(qid))) {
		cerr << "Failed to talk to remote\n";
		return 1;
	}

	uint8_t rsp[68];
	if ((err = HID_ReadReport(rsp))) {
		cerr << "Failed to talk to remote\n";
		return 1;
	}

	const unsigned int rx_len = rsp[1]&0x0F;

	if ((rsp[1]&0xF0) != RESPONSE_VERSION_DATA ||
	    (rx_len != 7 && rx_len != 5)) {
		printf("Bogus ident response: %02X\n",rsp[1]);
		err=-3;
		return 1;
	}

	ri.fw_ver_major = rsp[2]>>4;
	ri.fw_ver_minor = rsp[2]&0x0F;
	ri.hw_ver_major = rsp[3]>>4;
	ri.hw_ver_minor = rsp[3]&0x0F;
	ri.flash_id = rsp[4];
	ri.flash_mfg = rsp[5];
	ri.architecture = rx_len<6?2:rsp[6]>>4;
	ri.fw_type = rx_len<6?0:rsp[6]&0x0F;
	ri.skin = rx_len<6?2:rsp[7];
	ri.protocol = rx_len<7?0:rsp[8];

	setup_ri_pointers(ri);

	printf("\nReading Flash... ");
	uint8_t rd[1024];
	if ((err=ReadFlash(ri.arch->config_base,1024,rd,ri.protocol))) {
		printf("Failed to read flash\n");
		return 1;
	}

	// Calculate cookie
	const uint32_t cookie = (ri.arch->cookie_size == 2)
			? rd[0]|(rd[1]<<8)
			: rd[0]|(rd[1]<<8)|(rd[2]<<16)|(rd[3]<<24);

	ri.valid_config = (cookie == ri.arch->cookie);

	if(ri.valid_config) {
		ri.max_config_size = (ri.flash->size<<10)
			- (ri.arch->config_base - ri.arch->flash_base);
		const uint32_t end = rd[ri.arch->end_vector]
				| (rd[ri.arch->end_vector+1]<<8)
				| (rd[ri.arch->end_vector+2]<<16);
		ri.config_bytes_used =
			(end - (ri.arch->config_base - ri.arch->flash_base)) + 4;
	} else {
		ri.config_bytes_used=0;
		ri.max_config_size=1;
	}
		
	// read serial
	if ((err = ri.architecture==2 ?
		// The old 745 stores the serial number in EEPROM
		ReadMiscByte(0x10,48,COMMAND_MISC_EEPROM,rsp) :
		// All newer models store it in Flash
		ReadFlash(0x000110,48,rsp,ri.protocol))) {
		printf("Failed to read flash\n");
		return 1;
	}

	make_serial(rsp,ri);

	printf("\n");

	return 0;
}

int CRemote::ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd, unsigned int protocol, bool verify)
{
#ifdef WIN32
	GetConsoleScreenBufferInfo(con,&sbi);
#else
	if (len>2048) printf("              ");
#endif

	const unsigned int max_chunk_len = 
		protocol == 0 ? 700 : 1022;

	/*
	 * This is a mapping of the lower-half of the first command byte to
	 * the size of the total command to be sent. See protocol.h for more
	 * info.
	 */
	// Protocol 0 (745, safe mode)
	static const unsigned int dl0[16] = { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
		11, 12, 13, 14 };
	// All other protocols
	static const unsigned int dlx[16] = { 0, 0, 1, 2, 3, 4, 5, 6, 14, 30, 62,
		0, 0, 0, 0, 0 };
	const unsigned int *rxlenmap = protocol ? dlx : dl0;

	uint8_t *pr=rd;
	const uint32_t end = addr+len;
	unsigned int bytes_read=0;
	int err = 0;

	do {
		static uint8_t cmd[8];
		cmd[0] = 0;
		cmd[1] = COMMAND_READ_FLASH | 0x05;
		cmd[2] = (addr>>16)&0xFF;
		cmd[3] = (addr>>8)&0xFF;
		cmd[4] = addr&0xFF;
		unsigned int chunk_len = end-addr;
		if (chunk_len > max_chunk_len) 
			chunk_len=max_chunk_len;
		cmd[5] = (chunk_len>>8)&0xFF;
		cmd[6] = chunk_len&0xFF;

		if ((err = HID_WriteReport(cmd)))
			break;

		uint8_t seq=1;

		do {
			uint8_t rsp[68];
			if ((err = HID_ReadReport(rsp)))
				break;

			const uint8_t r=rsp[1]&COMMAND_MASK;

			if (r==RESPONSE_READ_FLASH_DATA) {
				if (seq!=rsp[2]) {
					err = 1;
					printf("\nInvalid sequence %02X %02x\n",
						seq,rsp[2]);
					break;
				}
				seq+=0x11;
				const unsigned int rxlen=rxlenmap[rsp[1]&LENGTH_MASK];
				if (rxlen) {
					if (verify) {
						if (memcmp(pr,rsp+3,rxlen)) {
							printf("\nVerify error\n");
							err = 2;
							break;
						}
					} else {
						memcpy(pr,rsp+3,rxlen);
					}
					pr+=rxlen;
					addr+=rxlen;
					bytes_read+=rxlen;
				}
				if (len<=2048) printf("*");
			} else if (r==RESPONSE_DONE) {
				break;
			} else {
				printf("\nInvalid response [%02X]\n",rsp[1]);
				err = 1;
			}
		} while (err == 0);

		if (len > 2048 && err == 0) {
#ifdef WIN32
			SetConsoleCursorPosition(con,sbi.dwCursorPosition);
			printf("%3i%%  %4i KiB",
				bytes_read*100/len,bytes_read>>10);
#else
			printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b%3i%%  %4i KiB",
				bytes_read*100/len,bytes_read>>10);
			fflush(stdout);
			//printf("*");
#endif
		}
	} while (err == 0 && addr < end);
	if (len > 2048)
		printf("\n");
	
	return err;
}

int CRemote::InvalidateFlash(void)
{
	const uint8_t ivf[]={ 0x00, COMMAND_WRITE_MISC | 0x01, 
				COMMAND_MISC_INVALIDATE_FLASH };
	int err;
	if ((err=HID_WriteReport(ivf))) return err;

	uint8_t rsp[68];
	if ((err=HID_ReadReport(rsp))) return err;

	if (rsp[1]&COMMAND_MASK != RESPONSE_DONE ||
		rsp[2]&COMMAND_MASK != COMMAND_MISC_INVALIDATE_FLASH) {
		return 1;
	}

	return 0;
}


int CRemote::EraseFlash(uint32_t addr, uint32_t len,  const TRemoteInfo &ri)
{
	const unsigned int *sectors=ri.flash->sectors;
	const unsigned int flash_base=ri.arch->flash_base;

	printf("\nErasing... ");

	const uint32_t end=addr+len;

#ifdef _DEBUG
	printf("%06X - %06X",addr,end);
#endif

	int err = 0;

	unsigned int n=0;
	uint32_t sector_begin=flash_base;
	uint32_t sector_end=sectors[n];

	do {
		sector_end += flash_base;
		if (sector_begin<end && sector_end>addr) {
			static uint8_t erase_cmd[8];
			erase_cmd[0] = 0;
			erase_cmd[1] = COMMAND_ERASE_FLASH;
			erase_cmd[2] = (sector_begin>>16)&0xFF;
			erase_cmd[3] = (sector_begin>>8)&0xFF;
			erase_cmd[4] = sector_begin&0xFF;

			if ((err = HID_WriteReport(erase_cmd)))
				break;

			uint8_t rsp[68];
			if ((err = HID_ReadReport(rsp,5000)))
				break;

#ifndef _DEBUG
			printf("*");
#else
			//printf("%02X %02X\n",rsp[1],rsp[2]);
			printf("\nerase sector %2i: %06X - %06X",n,sector_begin,
				sector_end);
		} else {
			printf("\n skip sector %2i: %06X - %06X",n,sector_begin,
				sector_end);
#endif
		}
		sector_begin=sector_end;
		sector_end=sectors[++n];
	} while (sector_end);

	printf("\n");

	return err;
}

int CRemote::WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr, unsigned int protocol)
{
#ifdef WIN32
	GetConsoleScreenBufferInfo(con,&sbi);
#else
	printf("              ");
#endif

	const unsigned int max_chunk_len = 
		protocol == 0 ? 749 : 1023;

	static const unsigned int txlenmap0[] =
		{ 0x07, 7, 6, 5, 4, 3, 2, 1 };
	static const unsigned int txlenmapx[] =
		{ 0x0A, 63, 31, 15, 7, 6, 5, 4, 3, 2, 1 };
	const unsigned int *txlenmap = protocol ? txlenmapx : txlenmap0;

	const uint8_t *pw=wr;
	const uint32_t end=addr+len;
	unsigned int bytes_written=0;
	int err = 0;

	do {
		static uint8_t write_setup_cmd[8];
		write_setup_cmd[0] = 0;
		write_setup_cmd[1] = COMMAND_WRITE_FLASH | 0x05;
		write_setup_cmd[2] = (addr>>16)&0xFF;
		write_setup_cmd[3] = (addr>>8)&0xFF;
		write_setup_cmd[4] = addr&0xFF;
		uint32_t chunk_len=end-addr;
		if (chunk_len>max_chunk_len)
			chunk_len=max_chunk_len;
		write_setup_cmd[5] = (chunk_len>>8)&0xFF;
		write_setup_cmd[6] = chunk_len&0xFF;

		if ((err = HID_WriteReport(write_setup_cmd)))
			break;

		while (chunk_len) {
			unsigned int n=txlenmap[0];
			unsigned int i=1;
			while (chunk_len < txlenmap[i]) ++i, --n;
			unsigned int block_len=txlenmap[i];
			uint8_t wd[68];
			wd[0] = 0;
			wd[1] = COMMAND_WRITE_FLASH_DATA | n;
			memcpy(wd+2,pw,block_len);
			//printf("%06X %4i\n",addr,chunk_len);
			HID_WriteReport(wd);
			pw += block_len;
			addr += block_len;
			bytes_written += block_len;
			chunk_len -= block_len;
		}
		
		uint8_t end_cmd[3] = { 0, COMMAND_DONE, COMMAND_WRITE_FLASH };
		HID_WriteReport(end_cmd);

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp,5000))) break;

#ifdef WIN32
		SetConsoleCursorPosition(con,sbi.dwCursorPosition);
		printf("%3i%%  %4i KiB",bytes_written*100/len,
			bytes_written>>10);
#else
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b%3i%%  %4i KiB",
			bytes_written*100/len,bytes_written>>10);
		fflush(stdout);
		//printf("*");
#endif
	} while (addr < end);

	printf("\n");
	
	return err;
}

int CRemote::ReadMiscByte(uint8_t addr, unsigned int len, uint8_t kind, uint8_t *rd)
{
	uint8_t rmb[] = { 0, COMMAND_READ_MISC | 0x02, kind, 0 };

	while (len--) {
		rmb[3]=addr++;

		int err;
		if ((err=HID_WriteReport(rmb))) return err;

		uint8_t rsp[68];
		if ((err=HID_ReadReport(rsp))) return err;

		if(rsp[1] != (RESPONSE_READ_MISC_DATA | 0x02) ||
			rsp[2] != kind)
			return 1;

		*rd++=rsp[3];
	}
	return 0;
}

int CRemote::ReadMiscWord(uint16_t addr, unsigned int len, uint8_t kind, uint16_t *rd)
{
	uint8_t rmb[] = { 0, COMMAND_READ_MISC | 0x03, kind, 0, 0 };

	while (len--) {
		rmb[3] = addr>>8;
		rmb[4] = addr&0xFF;
		++addr;

		int err;
		if ((err=HID_WriteReport(rmb))) return err;

		uint8_t rsp[68];
		if ((err=HID_ReadReport(rsp))) return err;

		// WARNING: The 880 responds with C2 rather than C3
		if(rsp[1]&COMMAND_MASK != RESPONSE_READ_MISC_DATA ||
			rsp[2] != kind)
			return 1;

		*rd++ = rsp[3]<<8 | rsp[4];
	}
	return 0;
}

int CRemote::WriteMiscByte(uint8_t addr, unsigned int len, uint8_t kind, uint8_t *wr)
{
	/// todo
	return 0;
}

int CRemote::WriteMiscWord(uint16_t addr, unsigned int len, uint8_t kind, uint16_t *wr)
{
	/// todo
	return 0;
}


int CRemote::GetTime(const TRemoteInfo &ri, THarmonyTime &ht)
{
	// NOTE: This is experimental
	int err=0;

	if(ri.architecture < 8) {
		uint8_t tsv[8];
		err = ReadMiscByte(0,6,COMMAND_MISC_STATE,tsv);
		ht.second = tsv[0];
		ht.minute = tsv[1];
		ht.hour = tsv[2];
		ht.dow = 7;
		ht.day = 1+tsv[3];
		ht.month = 1+tsv[4];
		ht.year = 2000+tsv[5];
	} else {
		uint16_t tsv[8];
		err = ReadMiscWord(0,7,COMMAND_MISC_STATE,tsv);
		ht.second = tsv[0];
		ht.minute = tsv[1];
		ht.hour = tsv[2];
		ht.day = 1+tsv[3];
		ht.dow = tsv[4]&7;
		ht.month = 1+tsv[5];
		ht.year = 2000+tsv[6];
	}

	ht.utc_offset=0;
	ht.timezone="";

	return err;
}

int CRemote::SetTime(const TRemoteInfo &ri, const THarmonyTime &ht)
{
	// NOTE: This is experimental
	int err=0;

	if(ri.architecture < 8) {
		uint8_t tsv[8];
		tsv[0] = ht.second;
		tsv[1] = ht.minute;
		tsv[2] = ht.hour;
		tsv[3] = ht.day-1;
		tsv[4] = ht.month-1;
		tsv[5] = ht.year-2000;
		err = WriteMiscByte(0,6,COMMAND_MISC_STATE,tsv);
	} else {
		uint16_t tsv[8];
		tsv[0] = ht.second;
		tsv[1] = ht.minute;
		tsv[2] = ht.hour;
		tsv[3] = ht.day-1;
		tsv[4] = ht.dow;
		tsv[5] = ht.month-1;
		tsv[6] = ht.year-2000;
		err = WriteMiscWord(0,7,COMMAND_MISC_STATE,tsv);

		// todo: Send Recalc Clock command for 880 (not 360/520/550)
	}

	return err;
}

int CRemote::LearnIR(string *learn_string)
{
	printf("\nLearn IR - Waiting...\n\n");

	int err = 0;

	const static uint8_t start_ir_learn[] = { 0, COMMAND_START_IRCAP };
	if ((err = HID_WriteReport(start_ir_learn))) return err;

	uint8_t seq=0;
	unsigned int ir_word=0;
	unsigned int t_on=0;
	unsigned int t_off=0;

	unsigned int freq=0;
	const unsigned int max_pulse_count=1000;
	unsigned int *pulses=new unsigned int[max_pulse_count];
	unsigned int pulse_count=0;

	do {
		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp,ir_word ? 500 : 4000)))
			break;
		const uint8_t r=rsp[1]&COMMAND_MASK;
		if (r == COMMAND_IRCAP_DATA) {
			if (rsp[2] != seq) {
				if (rsp[2] == 0x1F && seq == 0x10) {
					/*
					 * Handle 880 SNAFU
					 *
					 * Does this indicate a bad learn???
					 * Needs more testing
					 */
					printf("sequence glitch!\n");
					seq+=0x0F;
				} else {
					err = 1;
					printf("\nInvalid sequence %02X %02x\n",
						seq,rsp[2]);
					break;
				}
			}
			seq+=0x10;
			const unsigned int len=rsp[64];
			if ((len&1)==0) {
				for (unsigned int u=2; u<len; u+=2) {
					const unsigned int t=rsp[1+u]<<8 | rsp[2+u];
					if (ir_word>2) {
						if (ir_word&1) {
							// t == on + off time
							if (t_on)
								t_off = t-t_on;
							else
								t_off += t;
						} else {
							// t == on time
							t_on=t;
							if (t_on) {
								printf("-%i\n",t_off);
								if (pulse_count < max_pulse_count)
									pulses[pulse_count++] = t_off;
								printf("+%i\n",t_on);
								if (pulse_count < max_pulse_count)
									pulses[pulse_count++] = t_on;
							}
						}
					} else {
						switch(ir_word) {
							case 0:
								// ???
								break;
							case 1:
								// on time of
								// first burst
								t_on=t;
								break;
							case 2:
								// pulse count of
								// first burst
								if (t_on) {
									freq=static_cast<unsigned int>(static_cast<uint64_t>(t)*1000000/t_on);
									printf("%i Hz\n",freq);
									printf("+%i\n",t_on);
									pulses[pulse_count++] = t_on;
								}
								break;
						}
					}
					++ir_word;
				}
			} else {
				// Invalid length
				err = 3;
				break;
			}
		} else if (r == RESPONSE_DONE) {
			break;
		} else {
			printf("\nInvalid response [%02X]\n",rsp[1]);
			err = 1;
		}
	} while (err == 0 && t_off < 500000);

	if (t_off) printf("-%i\n",t_off);
	if (pulse_count<max_pulse_count) pulses[pulse_count++] = t_off;

	const static uint8_t stop_ir_learn[] = { 0x00, COMMAND_STOP_IRCAP };
	HID_WriteReport(stop_ir_learn);

	printf("\n");

	if (err == 0 && learn_string != NULL) {
		char s[32];
		sprintf(s,"F%04X",freq);
		*learn_string=s;
		for (unsigned int n = 0; n < pulse_count; ) {
			sprintf(s,"P%04X",pulses[n++]);
			*learn_string+=s;
			sprintf(s,"S%04X",pulses[n++]);
			*learn_string+=s;
		}
	}
	
	return err;
}
