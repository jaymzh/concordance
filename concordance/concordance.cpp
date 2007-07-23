/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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
#include "binaryfile.h"
#include "hid.h"
#include "remote.h"
#include "remote_info.h"
#include "web.h"
#include "protocol.h"

#ifdef WIN32
#include <conio.h>
#include <winsock.h>
HANDLE con;
CONSOLE_SCREEN_BUFFER_INFO sbi;
#endif

#define VERSION "0.6"

void MakeGuid(const uint8_t * const in, string& out)
{
	char x[48];
	sprintf(x,"{%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
		in[3],in[2],in[1],in[0], in[5],in[4], in[7],in[6],
		in[8],in[9], in[10],in[11],in[12],in[13],in[14],in[15]);
	out=x;
}

int main(int argc, char *argv[])
{
#ifdef WIN32
	con=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(con,FOREGROUND_BLUE|FOREGROUND_INTENSITY);
#endif

	printf("Harmony Utility %s\n", VERSION);
	printf("Copyright 2007 Kevin Timmerman\n\n");

#ifdef WIN32
	SetConsoleTextAttribute(con,FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN);

	// Initialize WinSock
	WSADATA wsainfo;
	int error=WSAStartup(1*256 + 1, &wsainfo);
	if (error) {
		printf("WSAStartup() Error: %i\n",error);
		return error;
	}
#endif

	bool option_config=false;
	bool option_firmware=false;
	bool option_connect=false;
	bool option_safemode=false;
	bool option_write=false;
	bool option_binary=false;
	bool option_learn=false;
	bool option_reset=false;
	bool option_verbose=false;

	char *file_name=NULL;

	for (int arg=1; arg<argc; ++arg) {
		char *a=argv[arg];
		if (*a == '-') {
			++a;
			if (!strcmp("config",a)) {
				option_config=true;
			} else if (!strcmp("firmware",a)) {
				option_firmware=true;
			} else if (!strcmp("safemode",a)) {
				option_safemode=true;
			} else if (!strcmp("connect",a)) {
				option_connect=true;
			} else if (!strcmp("write",a)) {
				option_write=true;
			} else if (!strcmp("binary",a)) {
				option_binary=true;
			} else if (!strcmp("learn",a)) {
				option_learn=true;
			} else if (!strcmp("reset",a)) {
				option_reset=true;
			} else if (!strcmp("v",a)) {
				option_verbose=true;
			}
		} else {
			if (!file_name) file_name=a;
		}
	}

	if (file_name) {
		if (!stricmp(file_name,"connectivity.ezhex")) {
			option_connect=true;
		} else if (!stricmp(file_name,"update.ezhex")) {
			option_config=true;
			option_write=true;
		} else if (!stricmp(file_name,"learnir.eztut")) {
			option_learn=true;
		} else {
			printf("WARNING: Don't know what to do with %s\n\n",
				file_name);
		}
	}

	int err=0;

	do {
		if (InitUSB()) {
			printf("Unable to initialize USB\n");
			err=-1;
			break;
		}

		THIDINFO hid_info;

		if ((err=FindRemote(hid_info))) {
			printf("Unable to find a remote (error %i)\n",err);
			break;
		}

		if (option_reset) {
			uint8_t reset_cmd[]={ 0x00, COMMAND_RESET,
				COMMAND_RESET_DEVICE };
			if ((err=HID_WriteReport(reset_cmd))) break;
		}

		const uint8_t qid[]={ 0x00, COMMAND_GET_VERSION };

		printf("Requesting Identity...\n");

		if ((err=HID_WriteReport(qid))) break;

		uint8_t rsp[68];
		if ((err=HID_ReadReport(rsp))) break;

		const unsigned int rx_len=rsp[1]&0x0F;

		if ((rsp[1]&0xF0) != RESPONSE_VERSION_DATA ||
		    (rx_len != 7 && rx_len != 5)) {
			printf("Bogus ident response: %02X\n",rsp[1]);
			err=-3;
			break;
		}

		TRemoteInfo ri;

		ri.fw_ver = rsp[2];
		ri.hw_ver = rsp[3];
		ri.flash_id = rsp[4];
		ri.flash_mfg = rsp[5];
		ri.architecture = rx_len<6?2:rsp[6]>>4;
		ri.fw_type = rx_len<6?0:rsp[6]&0x0F;
		ri.skin = rx_len<6?2:rsp[7];
		ri.protocol = rx_len<7?0:rsp[8];

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

		printf("\n");
		if (ri.model->code_name)
			printf("            Model: %s %s (%s)\n",ri.model->mfg,
				ri.model->model,ri.model->code_name);
		else
			printf("            Model: %s %s\n",ri.model->mfg,
				ri.model->model);
		if (option_verbose)
			printf("             Skin: %i\n",ri.skin);
		printf(" Firmware Version: %i.%i\n",ri.fw_ver>>4,
				ri.fw_ver&0x0F);
		if (option_verbose)
			printf("    Firmware Type: %i\n",ri.fw_type);
		printf(" Hardware Version: %i.%i\n",ri.hw_ver>>4,
			ri.hw_ver&0x0F);
		if (option_verbose) {
			if ((ri.flash->size&0x03FF) == 0
			    && (ri.flash->size>>10)!=0)
				printf("   External Flash: %i MiB - %02X:%02X %s\n",
					ri.flash->size>>10,ri.flash_mfg,
					ri.flash_id,ri.flash->part);
			else
				printf("   External Flash: %i KiB - %02X:%02X %s\n",
					ri.flash->size,ri.flash_mfg,ri.flash_id,
					ri.flash->part);
		}
		if (option_verbose)
			printf("     Architecture: %i\n",ri.architecture);
		if (option_verbose)
			printf("         Protocol: %i\n",ri.protocol);
		if (option_verbose) {
			printf("\n");
			printf("     Manufacturer: %s\n",hid_info.mfg.c_str());
			printf("          Product: %s\n",hid_info.prod.c_str());
			printf("    IRL, ORL, FRL: %i, %i, %i\n",
				hid_info.irl,hid_info.orl,hid_info.frl);
			printf("          USB VID: %04X\n",hid_info.vid);
			printf("          USB PID: %04X\n",hid_info.pid);
			printf("          USB Ver: %04X\n",hid_info.ver);
		}

		if (ri.flash->size == 0) {
			printf("Unsupported flash type\n");
			break;
		}

		if (ri.arch == NULL || ri.arch->cookie==0) {
			printf("Unsupported architecure\n");
			break;
		}

		printf("\nReading Flash ");
		uint8_t rd[1024];
		if ((err=ReadFlash(ri.arch->config_base,1024,rd,ri.protocol)))
			break;
		uint8_t ser[48];
		if ((err=ReadFlash(0x000110,48,ser,ri.protocol)))
			break;
		printf("\n");

		MakeGuid(ser,ri.serial[0]);
		MakeGuid(ser+16,ri.serial[1]);
		MakeGuid(ser+32,ri.serial[2]);
		if (option_verbose) {
			printf("\n    Serial Number: %s\n",
				ri.serial[0].c_str());
			printf("                   %s\n",ri.serial[1].c_str());
			printf("                   %s\n",ri.serial[2].c_str());
		}

		const uint32_t cookie= (ri.arch->cookie_size == 2)
				? rd[0]|(rd[1]<<8)
				: rd[0]|(rd[1]<<8)|(rd[2]<<16)|(rd[3]<<24);
		//TRACE1("Cookie: %08X\n",cookie);
		//ASSERT(cookie == ri.arch->cookie);

		binaryoutfile of;

		if (cookie!=ri.arch->cookie) {
			printf("\nInvalid cookie %X %X\n",cookie,ri.arch->cookie);
			if (option_config && !option_write)
				printf("Can not read config\n");
		} else {
			const uint32_t end = rd[ri.arch->end_vector]
					| (rd[ri.arch->end_vector+1]<<8)
					| (rd[ri.arch->end_vector+2]<<16);
			const uint32_t flash_config_bytes = (ri.flash->size<<10)
				- (ri.arch->config_base - ri.arch->flash_base);
			const uint32_t flash_config_bytes_used =
				(end -
				  (ri.arch->config_base - ri.arch->flash_base))
				+ 4;
			const uint32_t flash_config_percent_used =
				(flash_config_bytes_used*100+99)
				/ flash_config_bytes;

			printf("\nConfig Flash Used: %i%% (%i of %i KiB)\n",
				flash_config_percent_used,
				(flash_config_bytes_used+1023)>>10,
				(flash_config_bytes+1023)>>10);

			if (option_config && !option_write) {
				printf("\nReading Config ");

				uint8_t *config = new uint8_t[flash_config_bytes_used];
				if ((err = ReadFlash(ri.arch->config_base,
						flash_config_bytes_used,
						config,ri.protocol)))
					break;

				if (option_binary) {
					of.open("config.bin");
					of.write(config,flash_config_bytes_used);
				} else {
					uint32_t u=flash_config_bytes_used;
					uint8_t chk=0x69;
					uint8_t *pc=config;
					while (u--) chk^=*pc++;
					
					extern const char *config_header;
					char *ch = new char[strlen(config_header)
							+200];
					const int chlen = sprintf(ch,
						config_header,ri.protocol,
						ri.skin,ri.flash_mfg,
						ri.flash_id,ri.hw_ver>>4,
						ri.hw_ver&0x0F,ri.fw_type,
						ri.protocol,ri.skin,
						ri.flash_mfg,ri.flash_id,
						ri.hw_ver>>4,ri.hw_ver&0x0F,
						ri.fw_type,
						flash_config_bytes_used,chk);

					of.open("config.EZHex");
					of.write(reinterpret_cast<uint8_t*>(ch),
						chlen);
					of.write(config,flash_config_bytes_used);
				}
				of.close();

				delete[] config;
			}
		}

		if (option_connect && file_name!=NULL) {
			binaryinfile file;
			file.open(file_name);

			const unsigned int size=file.getlength();
			uint8_t * const buf=new uint8_t[size+1];
			file.read(buf,size);
			// Prevent GetTag() from going off the deep end
			buf[size]=0;

			Post(buf,"POSTOPTIONS",ri);

			file.close();
		}

		if (option_config && option_write) {
			binaryinfile file;
			if (option_binary) {
				file.open(file_name?file_name:"config.bin");
			} else {
				file.open(file_name?file_name:"Config.EZHex");
			}
			uint32_t size=file.getlength();
			uint8_t * const x=new uint8_t[size+1];
			file.read(x,size);
			file.close();
			// Prevent GetTag() from going off the deep end
			x[size]=0;

			uint8_t *y=x;
			if (!option_binary) {
				uint8_t *t=x;
				string s;
				GetTag("BINARYDATASIZE",t,&s);
				uint32_t data_size=atoi(s.c_str());
				printf("data size %i\n",data_size);
				GetTag("CHECKSUM",t,&s);
				const uint8_t checksum=atoi(s.c_str());
				printf("checksum %i\n",checksum);

				GetTag("/INFORMATION",y);
				y+=2;
				size-=(y-x);
				printf("size %i\n",size);

				if (size!=data_size)
					printf("Data size mismatch %i %i\n",
						size,data_size);

				uint32_t u=size;
				uint8_t chk=0x69;
				uint8_t *pc=y;
				while (u--) chk^=*pc++;
				if (chk != checksum)
					printf("Bad checksum %02X %02X\n",chk,
						checksum);

				if (file_name) Post(x,"POSTOPTIONS",ri);
			}

			if ((err = EraseFlash(ri.arch->config_base,size,
						ri.flash->sectors))) {
				delete[] x;
				break;
			}

			printf("\nWrite Config ");
			if ((err=WriteFlash(ri.arch->config_base,size,y,
					ri.protocol))) {
				delete[] x;
				break;
			}

			printf("\nVerify Config ");
			if ((err=ReadFlash(ri.arch->config_base,size,y,
					ri.protocol,true))) {
				delete[] x;
				break;
			}

			if (file_name && !option_binary)
				Post(x,"COMPLETEPOSTOPTIONS",ri);

			delete[] x;
		}

		if (option_safemode) {
			if (option_write) {
				/*
				 * note: It *can* be written, but it would be a
				 * bad idea to allow it
				 */
				printf("\nSafe mode firmware may not be written\n");
			} else {
				uint8_t * const safe = new uint8_t[64*1024];
				printf("\nReading Safe Mode Firmware ");
				if ((err = ReadFlash(ri.arch->flash_base,
						64*1024,safe,ri.protocol))) {
					delete[] safe;
					break;
				}

				of.open("safe.bin");
				of.write(safe,64*1024);
				of.close();

				delete[] safe;
			}
		}

		if (option_firmware) {
			if (option_write) {
				printf("This isn't supported yet.\n");
				/// todo
			} else {
				uint8_t * const firmware = new uint8_t[64*1024];
				printf("\nReading Firmware ");
				if ((err = ReadFlash(ri.arch->firmware_base,
						64*1024,firmware,ri.protocol)))
					break;
				if (option_binary) {
					of.open("firmware.bin");
					of.write(firmware,64*1024);
				} else {
					/// todo: file header
					uint16_t *pw =
					   reinterpret_cast<uint16_t*>(firmware);
					uint16_t wc = 0x4321;
					unsigned int n = 32*1024;
					while (n--) wc ^= *pw++;
					//TRACE1("Checksum: %04X\n",wc);

					firmware[0] = firmware[1] = firmware[2]
						=firmware[3] = 0xFF;

					of.open("firmware.EZUp");

					uint8_t *pf=firmware;
					const uint8_t *fwend=firmware+64*1024;
					do {
						of.write("\t\t\t<DATA>");
						char hex[16];
						u=32;
						while (u--)
							sprintf(hex,"%02X",*pf++);
							of.write(hex);
						of.write("</DATA>\n");
					} while (pf < fwend);
				}
				of.close();
				delete[] firmware;
			}
		}

		if (option_learn) {
			if (file_name) {
				binaryinfile file;
				file.open(file_name);
				uint32_t size=file.getlength();
				uint8_t * const x=new uint8_t[size+1];
				file.read(x,size);
				file.close();
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
				LearnIR(&ls);
				//printf("%s\n",ls.c_str());

				Post(x,"POSTOPTIONS",ri,&ls,&keyname);
			} else {
				LearnIR();
			}
		}

	} while (false);

	ShutdownUSB();

	if (err) printf("Failed with error %i\n",err);
		
#ifdef WIN32
	// Shutdown WinSock
	WSACleanup();
#ifdef _DEBUG
	printf("\nPress any key to exit");
	_getch();
#endif
#endif

	return err;
}

