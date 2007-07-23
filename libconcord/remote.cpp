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
//#include "remote_info.h"

#ifdef WIN32
#include <conio.h>
#include <winsock.h>
extern HANDLE con;
extern CONSOLE_SCREEN_BUFFER_INFO sbi;
#endif

int ReadFlash(uint32_t addr, const uint32_t len, uint8_t *rd, unsigned int protocol, bool verify)
{
#ifdef WIN32
	GetConsoleScreenBufferInfo(con,&sbi);
#else
	if (len>2048) printf("              ");
#endif

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
		if (chunk_len > 1024) 
			chunk_len=1024;
		cmd[5] = (chunk_len>>8)&0xFF;
		cmd[6] = chunk_len&0xFF;

		if ((err = HID_WriteReport(cmd)))
			break;

		uint8_t seq=1;

		do {
			uint8_t rsp[68];
			if ((err = HID_ReadReport(rsp)))
				break;

			const uint8_t r=rsp[1]&0xF0;

			if (r==RESPONSE_READ_FLASH_DATA) {
				if (seq!=rsp[2]) {
					err = 1;
					printf("\nInvalid sequence %02X %02x\n",
						seq,rsp[2]);
					break;
				}
				seq+=0x11;
				const unsigned int rxlen=rxlenmap[rsp[1]&0x0F];
				//ASSERT(rxlen);
				//TRACE2("%06X %i\n",addr,rxlen);
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


int EraseFlash(uint32_t addr, uint32_t len, const uint32_t *sectors)
{
	printf("\nErasing... ");

	const uint32_t end=addr+len;

#ifdef _DEBUG
	printf("%06X - %06X",addr,end);
#endif

	int err = 0;

	unsigned int n=0;
	uint32_t sector_begin=0;
	uint32_t sector_end=sectors[n];

	do {
		if (sector_begin<end && sector_end>addr) {
			static uint8_t erase_cmd[8];
			erase_cmd[0] = 0;
			erase_cmd[1] = COMMAND_ERASE_FLASH;
			erase_cmd[2] = (addr>>16)&0xFF;
			erase_cmd[3] = (addr>>8)&0xFF;
			erase_cmd[4] = addr&0xFF;

			addr+=0x10000;

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

int WriteFlash(uint32_t addr, const uint32_t len, const uint8_t *wr, unsigned int protocol)
{
#ifdef WIN32
	GetConsoleScreenBufferInfo(con,&sbi);
#endif

	static const unsigned int txlenmap[] =
		{ 0x0A, 63, 30, 14, 6, 5, 4, 3, 2,1 };
	/// todo: map for safe mode (protocol 0)

	const uint8_t *pw=wr;
	const uint32_t end=addr+len;
	unsigned int bytes_written=0;
	int err = 0;

	do {
		/*
		 * FIXME: This can probably be abstracted and ReadFlash()
		 * and WriteFlash() can both use some function to set this up
		 */
		static uint8_t write_setup_cmd[8];
		write_setup_cmd[0] = 0;
		write_setup_cmd[1] = COMMAND_WRITE_FLASH | 0x05;
		write_setup_cmd[2] = (addr>>16)&0xFF;
		write_setup_cmd[3] = (addr>>8)&0xFF;
		write_setup_cmd[4] = addr&0xFF;
		uint32_t chunk_len=end-addr;
		if (chunk_len>1024)
			chunk_len=1024;
		write_setup_cmd[5] = (chunk_len>>8)&0xFF;
		write_setup_cmd[6] = chunk_len&0xFF;

		if ((err = HID_WriteReport(write_setup_cmd)))
			break;

		while (chunk_len) {
			unsigned int n=txlenmap[0];
			unsigned int i=1;
			//while (chunk_len < txlenmap[i]) ++i, --n;
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
			//chunk_len -= block_len;
			chunk_len = (chunk_len<block_len)
				? 0 : chunk_len-block_len;
		}
		
		uint8_t end_cmd[3] = { 0x00, COMMAND_DONE, 0x30 };
		HID_WriteReport(end_cmd);

		//printf("data sent\n");

		uint8_t rsp[68];
		if ((err = HID_ReadReport(rsp,5000))) break;
		//printf("%02X %02X\n",rsp[1],rsp[2]);

#ifdef WIN32
		SetConsoleCursorPosition(con,sbi.dwCursorPosition);
		printf("%3i%%  %4i KiB",bytes_written*100/len,
			bytes_written>>10);
#else
		//printf("%3i%%  %4i KiB",bytes_written*100/len,
		//	bytes_written>>10);
		printf("*");
#endif
	} while (addr < end);

	printf("\n");
	
	return err;
}

int LearnIR(string *learn_string)
{
	printf("\nLearn IR - Waiting...\n\n");

	int err = 0;

	const static uint8_t start_ir_learn[] = { 0x00, COMMAND_START_IRCAP };
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
		const uint8_t r=rsp[1]&0xF0;
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
		} else if (r == 0xF0) {
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

