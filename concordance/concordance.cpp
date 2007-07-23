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
#include <iostream>
#include <getopt.h>

#ifdef WIN32
#include <conio.h>
#include <winsock.h>
HANDLE con;
CONSOLE_SCREEN_BUFFER_INFO sbi;
#endif

#define VERSION "0.8"

struct options_t {
	bool config;
	bool firmware;
	bool connect;
	bool safemode;
	bool write;
	bool binary;
	bool learn;
	bool reset;
	bool verbose;
	bool help;
};


/*
 * Parse our options.
 *
 *   This probably isn't very cross-platform. Dunno if gnu's getopt
 *   is available in windows - we may have to include getopt with our
 *   distribution.
 *
 *   There's also stuff like SimpleOpt thats cross-platform...
 *
 *   Dunno...
 */
void parse_options(struct options_t &options, char *file_name, int argc,
	char *argv[])
{

	static struct option long_options[] = {
		{"binary", no_argument, 0, 'b'},
		{"config", no_argument, 0, 'c'},
		{"connect", required_argument, 0, 'C'},
		{"firmware", no_argument, 0, 'f'},
		{"help", no_argument, 0, 'h'},
		{"learn", optional_argument, 0, 'l'},
		{"reset", no_argument, 0, 'r'},
		{"safemode", no_argument, 0, 's'},
		{"verbose", no_argument, 0, 'v'},
		{"write", required_argument, 0, 'w'},
		{0,0,0,0} // terminating null entry
	};

	options.config = false;
	options.firmware = false;
	options.connect = false;
	options.safemode = false;
	options.write = false;
	options.binary = false;
	options.learn = false;
	options.reset = false;
	options.verbose = false;
	options.help = false;

	int tmpint = 0, option_index = 0;

	while ((tmpint = getopt_long(argc, argv, "bcC:fhl:rv", long_options,
					&option_index)) != EOF) {
		switch (tmpint) {
		case 0:
			if (long_options[option_index].flag != 0)
				break;

			/* Long-only options go here */
			break;
		case 'b':
			options.binary = true;
			break;
		case 'c':
			options.config = true;
			break;
		case 'C':
			if (optarg == NULL) {
				cerr << "Missing argument for -C\n";
				exit(1);
			}
			file_name = optarg;
			options.connect = true;
			break;
		case 'f':
			options.firmware = true;
			break;
		case 'h':
			options.help = true;
			break;
		case 'l':
			if (optarg == NULL) {
				cerr << "Missing argument for -l\n";
				exit(1);
			}
			file_name = optarg;
			options.learn = true;
			break;
		case 'r':
			options.reset = true;
			break;
		case 's':
			options.safemode = true;
			break;
		case 'v':
			options.verbose = true;
			break;
		case 'w':
			if (optarg == NULL) {
				cerr << "Missing argument for -w\n";
				exit(1);
			}
			file_name = optarg;
			options.write = true;
			break;
		default:
			exit(1);
		}
	}

	/*
	 * For starters, lets make sure we don't have conflicting options.
	 * If we're writing, we can only write one thing at a time, and
	 * certain things can't be written.
	 */

	/*
	 *   FIXME: Rather than track all of this and trying to allow a user
	 *      to do multiple things and potentially do bad we should have
	 *      modes... see the TODO for the possible UI setups being
	 *      considered.
	 *
	 *      - Phil
	 */
	if (options.write && options.config && options.firmware) {
		cerr << "Please specify only one thing to write at a time.\n";
		exit(1);
	}

	if (options.write && (options.learn || options.connect
							|| options.reset)) {
		cerr << "You specified write with something unwritable,"
			<< " ignoring write flag\n";
		options.write = false;
	}

	/*
	 * Now, in case someone passed in just a filename, check arguments
	 */
	if (optind < argc) {
		if (option_index + 1 < argc) {
			cerr << "Multiple arguments were specified after the"
				<< " options - not sure what to do, exiting\n";
			exit(1);
		}
		if (!file_name) {
			file_name = argv[option_index];
			/*
			 * If no filename is set, and we have a filename,
			 * we're being expected to figure it out on our own!
			 */
			if (!stricmp(file_name,"connectivity.ezhex")) {
				options.connect=true;
			} else if (!stricmp(file_name,"update.ezhex")) {
				options.config=true;
				options.write=true;
			} else if (!stricmp(file_name,"learnir.eztut")) {
				options.learn=true;
			} else {
				cerr << "Don't know what to do with"
					<< file_name << endl;
				exit(1);
			}
		}
	}

/*
 * The following is commented out because I don't know of getopt.h
 * is easily available in winblows, and we want to keep this cross-platform.
 * So I've moved to getopt in hopes that it is, but kept the original code
 * around incase it isn't. TDB.
 * 	- Phil
 */
#if 0
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

#endif
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

void help()
{

	//cout << "Harmony Control " << VERSION << endl;
	cout << "Usage:\n\tharmony <options>\n\tharmony <file>\n\n";
	cout << "If a file is passed in instead of options, harmony will"
		<< "attempt\nto determine the filetype and do the right"
		<< "thing with it.\n\n";

	cout << "Options:\n"
		<< "\t--config\t\tRead the remote's config and write it to\n"
		<< "\t\t\t\tconfig.EZHex. If --write is specified, read a"
		<< " config\n"
		<< "\t\t\t\tfrom the specified file and write it to the"
		<< " remote\n";

	cout << "\t--firmware\t\tRead and remote's firmware and write it"
		<< " to\n"
		<< "\t\t\t\tfirmware.EZUp. If --write is specified,\n"
		<< "\t\t\t\tread the firmware from the specified file and"
		<< " write\n"
		<< "\t\t\t\tit to the remote. Writing is not currently"
		<< " supported.\n";

	cout << "\t--safemode\t\tRead safemode firmware and write it to\n"
		<< "\t\t\t\tsafe.bin.\n";

	cout << "\t--connect <file>\tDo a connectivity test.\n";

	cout << "\t--write <file>\t\tDefines another option as being in"
		<< " write mode\n\t\t\t\tinstead of read mode\n";

	cout << "\t--binary\t\tThe config file is already truncated\n"
		<< "\t\t\t\tto just the binary portions. If in --write mode\n"
		<< "\t\t\t\tthen write binary only portion out to .bin file\n";

	cout<< "\t--learn [<file>]\tLearn IR commands from other remotes\n";

	cout << "\t--reset\t\t\tReboot the remote\n";

	cout << "\t--verbose\t\tVerbose mode\n";
}

/*
 * Send the GET_VERSION command to the remote, and read the response.
 *
 * Then populate our struct with all the relevant info.
 */
int get_version_info(TRemoteInfo &ri, uint8_t *ser, uint32_t &cookie,
			uint32_t &end)
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

	printf("\nReading Flash... ");
	uint8_t rd[1024];
	if ((err=ReadFlash(ri.arch->config_base,1024,rd,ri.protocol))) {
		printf("Failed to read flash\n");
		return 1;
	}

	// Calculate end
	end = rd[ri.arch->end_vector]
			| (rd[ri.arch->end_vector+1]<<8)
			| (rd[ri.arch->end_vector+2]<<16);

	// Calculate cookie
	cookie = (ri.arch->cookie_size == 2)
			? rd[0]|(rd[1]<<8)
			: rd[0]|(rd[1]<<8)|(rd[2]<<16)|(rd[3]<<24);
	//TRACE1("Cookie: %08X\n",cookie);
	//ASSERT(cookie == ri.arch->cookie);

	// read serial
	if ((err=ReadFlash(0x000110,48,ser,ri.protocol))) {
		printf("Failed to read flash\n");
		return 1;
	}

	printf("\n");

	return 0;
}


/*
 * Print all version info in a format readable by humans.
 */
int print_version_info(TRemoteInfo &ri, bool &option_verbose,
			THIDINFO &hid_info, uint8_t *ser)
{
	if (ri.model->code_name) {
		printf("            Model: %s %s (%s)\n",ri.model->mfg,
			ri.model->model,ri.model->code_name);
	} else {
		printf("            Model: %s %s\n",ri.model->mfg,
			ri.model->model);
	}

	if (option_verbose)
		printf("             Skin: %i\n",ri.skin);

	printf(" Firmware Version: %i.%i\n",ri.fw_ver>>4, ri.fw_ver&0x0F);

	if (option_verbose)
		printf("    Firmware Type: %i\n",ri.fw_type);

	printf(" Hardware Version: %i.%i\n",ri.hw_ver>>4, ri.hw_ver&0x0F);

	if (option_verbose) {
		if ((ri.flash->size&0x03FF) == 0 && (ri.flash->size>>10)!=0) {
			printf("   External Flash: %i MiB - %02X:%02X %s\n",
				ri.flash->size>>10,ri.flash_mfg,
				ri.flash_id,ri.flash->part);
		} else {
			printf("   External Flash: %i KiB - %02X:%02X %s\n",
				ri.flash->size,ri.flash_mfg,ri.flash_id,
				ri.flash->part);
		}
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
		return 1;
	}

	if (ri.arch == NULL || ri.arch->cookie==0) {
		printf("Unsupported architecure\n");
		return 1;
	}

	make_guid(ser,ri.serial[0]);
	make_guid(ser+16,ri.serial[1]);
	make_guid(ser+32,ri.serial[2]);
	if (option_verbose) {
		printf("\n    Serial Number: %s\n",
			ri.serial[0].c_str());
		printf("                   %s\n",ri.serial[1].c_str());
		printf("                   %s\n",ri.serial[2].c_str());
	}

	return 0;
}

/*
 * Pull the config from the remote and write it to a file
 */
int dump_config(TRemoteInfo &ri, uint32_t &cookie, bool &option_verbose,
		bool &option_binary, uint32_t &end)
{
	int err = 0;

	if (cookie!=ri.arch->cookie) {
		printf("\nInvalid cookie %X %X\n",cookie,ri.arch->cookie);
		printf("Can not read config\n");
		return 1;
	}

	const uint32_t flash_config_bytes = (ri.flash->size<<10)
		- (ri.arch->config_base - ri.arch->flash_base);
	const uint32_t flash_config_bytes_used =
		(end - (ri.arch->config_base - ri.arch->flash_base)) + 4;
	const uint32_t flash_config_percent_used =
		(flash_config_bytes_used*100+99) / flash_config_bytes;

	if (option_verbose) {
		printf("\nConfig Flash Used: %i%% (%i of %i KiB)\n",
			flash_config_percent_used,
			(flash_config_bytes_used+1023)>>10,
			(flash_config_bytes+1023)>>10);
	}

	printf("\nReading Config ");

	uint8_t *config = new uint8_t[flash_config_bytes_used];
	if ((err = ReadFlash(ri.arch->config_base, flash_config_bytes_used,
			config,ri.protocol))) {
		printf("Failed to read flash\n");
		return 1;
	}

	binaryoutfile of;
	if (option_binary) {
		of.open("config.bin");
		of.write(config,flash_config_bytes_used);
	} else {
		uint32_t u = flash_config_bytes_used;
		uint8_t chk = 0x69;
		uint8_t *pc = config;
		while (u--)
			chk ^= *pc++;

		extern const char *config_header;
		char *ch = new char[strlen(config_header) + 200];
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

	return 0;
}

/*
 * Do a connectivity test
 */
int connect_test(TRemoteInfo &ri, char *file_name)
{
	/*
	 * If we arrived, we can talk to the remote - so if it's
	 * just a connectivity test, tell the site we succeeded
	 */
	binaryinfile file;
	file.open(file_name);

	const unsigned int size = file.getlength();
	uint8_t * const buf = new uint8_t[size+1];
	file.read(buf,size);
	// Prevent GetTag() from going off the deep end
	buf[size]=0;

	Post(buf,"POSTOPTIONS",ri);

	file.close();

	return 0;
}

/*
 * Read the config from a file and write it to the remote
 */
int write_config(TRemoteInfo &ri, char *file_name, bool &option_binary)
{
	int err = 0;
	binaryinfile file;

	if (option_binary) {
		file.open(file_name?file_name:"config.bin");
	} else {
		file.open(file_name?file_name:"Config.EZHex");
	}
	uint32_t size = file.getlength();
	uint8_t * const x = new uint8_t[size+1];
	file.read(x,size);
	file.close();
	// Prevent GetTag() from going off the deep end
	x[size] = 0;

	uint8_t *y=x;

	if (!option_binary) {
		/*
		 * If we were passed in a full XML file, parse it, check
		 * that the size/checksum reported in it matches the binary
		 * blob, and print some data about it.
		 */
		uint8_t *t = x;
		string s;
		GetTag("BINARYDATASIZE",t,&s);
		uint32_t data_size = atoi(s.c_str());
		printf("data size %i\n",data_size);
		GetTag("CHECKSUM",t,&s);
		const uint8_t checksum = atoi(s.c_str());
		printf("checksum %i\n",checksum);

		GetTag("/INFORMATION",y);
		y += 2;
		size -= (y-x);
		printf("size %i\n",size);

		if (size != data_size)
			printf("Data size mismatch %i %i\n",size,data_size);

		uint32_t u=size;
		uint8_t chk=0x69;
		uint8_t *pc=y;
		while (u--)
			chk^=*pc++;
		if (chk != checksum)
			printf("Bad checksum %02X %02X\n",chk, checksum);

		if (file_name) Post(x,"POSTOPTIONS",ri);
	}

	/*
	 * Flash can be changed to 0, but not back to 1, so you must
	 * erase the flash (to 1) in order to write the flash.
	 */
	if ((err = EraseFlash(ri.arch->config_base,size,
				ri.flash->sectors))) {
		delete[] x;
		printf("Failed to erase flash! Bailing out!\n");
		return 1;
	}

	printf("\nWrite Config ");
	if ((err=WriteFlash(ri.arch->config_base,size,y,
			ri.protocol))) {
		delete[] x;
		printf("Failed to write flash! Bailing out!\n");
		return 1;
	}

	/*
	 * We don't really verify yet, just do a read as a basic test.
	 */
	printf("\nVerify Config ");
	if ((err=ReadFlash(ri.arch->config_base,size,y,
			ri.protocol,true))) {
		delete[] x;
		printf("Failed to read flash! Bailing out!\n");
		return 1;
	}

	if (file_name && !option_binary)
		Post(x,"COMPLETEPOSTOPTIONS",ri);

	delete[] x;

	return 0;
}

int dump_safemode(TRemoteInfo &ri)
{
	binaryoutfile of;
	uint8_t * const safe = new uint8_t[64*1024];
	int err = 0;

	printf("\nReading Safe Mode Firmware ");
	if ((err = ReadFlash(ri.arch->flash_base,64*1024,safe,
							ri.protocol))) {
		delete[] safe;
		printf("Failed to read Safe Mode firmware.\n");
		return 1;
	}

	of.open("safe.bin");
	of.write(safe,64*1024);
	of.close();

	delete[] safe;

	return 0;
}

int dump_firmware(TRemoteInfo &ri, bool &option_binary)
{
	binaryoutfile of;
	int err = 0;
	uint8_t * const firmware = new uint8_t[64*1024];

	printf("\nReading Firmware ");
	if ((err = ReadFlash(ri.arch->firmware_base,64*1024,firmware,
							ri.protocol))) {
		printf("Failed to read firmware.\n");
		return 1;
	}

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

		firmware[0] = firmware[1] = firmware[2] = firmware[3] = 0xFF;

		of.open("firmware.EZUp");

		uint8_t *pf = firmware;
		const uint8_t *fwend = firmware+64*1024;
		do {
			of.write("\t\t\t<DATA>");
			char hex[16];
			int u=32;
			while (u--) {
				sprintf(hex,"%02X",*pf++);
				of.write(hex);
			}
			of.write("</DATA>\n");
		} while (pf < fwend);
	}

	of.close();
	delete[] firmware;

	return 0;
}

int learn_ir_commands(TRemoteInfo &ri, char *file_name)
{
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

	return 0;
}


int main(int argc, char *argv[])
{

#ifdef WIN32
	con=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(con,FOREGROUND_BLUE|FOREGROUND_INTENSITY);
#endif

	printf("Harmony Control %s\n", VERSION);
	printf("Copyright 2007 Kevin Timmerman\n\n");

#ifdef WIN32
	SetConsoleTextAttribute(con,
		FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_GREEN);

	// Initialize WinSock
	WSADATA wsainfo;
	int error=WSAStartup(1*256 + 1, &wsainfo);
	if (error) {
		printf("WSAStartup() Error: %i\n",error);
		return error;
	}
#endif

	struct options_t options;
	char *file_name=NULL;
	parse_options(options, file_name, argc, argv);

	if (options.help) {
		help();
		exit(0);
	}

	int err=0;
	TRemoteInfo ri;
	THIDINFO hid_info;

	if (InitUSB()) {
		printf("Unable to initialize USB\n");
		err=-1;
		goto cleanup;
	}

	if ((err = FindRemote(hid_info))) {
		printf("Unable to find a remote (error %i)\n",err);
		goto cleanup;
	}

	if (options.reset) {
		uint8_t reset_cmd[]={ 0x00, COMMAND_RESET,
			COMMAND_RESET_DEVICE };
		if ((err=HID_WriteReport(reset_cmd)))
			goto cleanup;
	}

	/*
	 * Get and print all the version info
	 */
	uint32_t cookie, end;
	uint8_t ser[48];

	/*
	 * FIXME:
	 * 	This should be just a simple case statement. This
	 * 	test-then-goto is to preserve the original behavior.
	 * 	
	 * 	See the TODO file for possible UI options being considered.
	 * 	Once that's chosen, this will get changed.
	 *
	 */
	if ((err = get_version_info(ri, ser, cookie, end)))
		goto cleanup;

	if ((err = print_version_info(ri, options.verbose, hid_info, ser)))
		goto cleanup;

	if (options.connect && file_name != NULL) {
		if ((err = connect_test(ri, file_name)))
			goto cleanup;
	}

	if (options.config) {
		if (options.write) {
			if ((err = write_config(ri, file_name, options.binary)))
				goto cleanup;
		} else {
			if ((err = dump_config(ri, cookie, options.verbose,
					options.binary, end)))
				goto cleanup;
		}
	}

	if (options.safemode) {
		if (options.write) {
			/*
			 * note: It *can* be written, but it would be a
			 * bad idea to allow it
			 */
			printf("\nSafe mode firmware may not be written\n");
		} else {
			if ((err = dump_safemode(ri)))
				goto cleanup;
		}
	}

	if (options.firmware) {
		if (options.write) {
			printf("This isn't supported yet.\n");
			/*
			 * FIXME: Implement this.
			 */
		} else {
			if ((err = dump_firmware(ri, options.binary)))
				goto cleanup;
		}
	}

	if (options.learn) {
		if ((err = learn_ir_commands(ri, file_name)))
			goto cleanup;
	}

cleanup:
	ShutdownUSB();

	if (err) printf("Failed with error %i\n",err);
		
#ifdef WIN32
	// Shutdown WinSock
	WSACleanup();
#ifdef _DEBUG
	printf("\nPress any key to exit");
	_getch();
#endif // debug
#endif // WIN32

	return err;
}

