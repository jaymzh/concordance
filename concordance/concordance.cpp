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
#include "usblan.h"
#include "remote.h"
#include "web.h"
#include "protocol.h"
#include "time.h"
#include <iostream>

#ifdef WIN32
#include "win/getopt/getopt.h"
#include <conio.h>
#include <winsock.h>
HANDLE con;
CONSOLE_SCREEN_BUFFER_INFO sbi;
#else
#include <getopt.h>
#endif

const char * const VERSION = "0.12-dev";

enum {
	MODE_UNSET,
	MODE_CONNECTIVITY,
	MODE_DUMP_CONFIG,
	MODE_WRITE_CONFIG,
	MODE_DUMP_FIRMWARE,
	MODE_WRITE_FIRMWARE,
	MODE_DUMP_SAFEMODE,
	MODE_HELP,
	MODE_LEARN_IR,
	MODE_RESET,
	MODE_GET_TIME,
	MODE_SET_TIME
};

static CRemoteBase *rmt = NULL;

void set_mode(int &mode, int val)
{
	if (mode == MODE_UNSET) {
		mode = val;
	} else {
		cerr << "ERROR: More than one mode specified. Please specify"
			<< " only one.\n";
		exit(1);
	}
}
		
		
/*
 * Parse our options.
 */
void parse_options(struct options_t &options, int &mode, char *&file_name,
	int argc, char *argv[])
{

	static struct option long_options[] = {
		{"binary", no_argument, 0, 'b'},
		{"dump-config", optional_argument, 0, 'c'},
		{"write-config", required_argument, 0, 'C'},
		{"dump-firmware", optional_argument, 0, 'f'},
		{"write-firmware", required_argument, 0, 'F'},
		{"help", no_argument, 0, 'h'},
		{"learn-ir", required_argument, 0, 'l'},
		{"reset", no_argument, 0, 'r'},
		{"dump-safemode", optional_argument, 0, 's'},
		{"connectivity-test", required_argument, 0, 't'},
		{"get-time", no_argument, 0, 'k' },
		{"set-time", no_argument, 0, 'K' },
		{"verbose", no_argument, 0, 'v'},
		{"no-web", no_argument, 0, 'w'},
		{0,0,0,0} // terminating null entry
	};

	options.verbose = false;
	options.binary = false;
	options.noweb = false;

	mode = MODE_UNSET;

	int tmpint = 0;

	while ((tmpint = getopt_long(argc, argv, "bc::C:f::F:hl:rs::t:kKvw",
				long_options, NULL)) != EOF) {
		switch (tmpint) {
		case 0:
			/* Long-only options go here */
			break;
		case 'b':
			options.binary = true;
			break;
		case 'c':
			set_mode(mode, MODE_DUMP_CONFIG);
			if (optarg != NULL) {
				file_name = optarg;
			}
			break;
		case 'C':
			if (optarg == NULL) {
				cerr << "Missing config file to read from.\n";
				exit(1);
			}
			set_mode(mode, MODE_WRITE_CONFIG);
			file_name = optarg;
			break;
		case 'f':
			set_mode(mode, MODE_DUMP_FIRMWARE);
			if (optarg != NULL)
				file_name = optarg;
			break;
		case 'F':
			if (optarg == NULL) {
				cerr << "Missing firmware file to read from.\n";
				exit(1);
			}
			set_mode(mode, MODE_WRITE_FIRMWARE);
			break;
		case 'h':
			set_mode(mode, MODE_HELP);
			break;
		case 'l':
			if (optarg == NULL) {
				cerr << "Missing config file to read from.\n";
				exit(1);
			}
			set_mode(mode, MODE_LEARN_IR);
			file_name = optarg;
			break;
		case 'r':
			set_mode(mode, MODE_RESET);
			break;
		case 's':
			set_mode(mode, MODE_DUMP_SAFEMODE);
			if (optarg != NULL)
				file_name = optarg;
			break;
		case 't':
			if (optarg == NULL) {
				cerr << "Missing connectivity file to read from.\n";
				exit(1);
			}
			set_mode(mode, MODE_CONNECTIVITY);
			file_name = optarg;
			break;
		case 'k':
			set_mode(mode, MODE_GET_TIME);
			break;
		case 'K':
			set_mode(mode, MODE_SET_TIME);
			break;
		case 'v':
			options.verbose = true;
			break;
		case 'w':
			options.noweb = true;
			break;
		default:
			exit(1);
		}
	}

	/*
	 * Now, in case someone passed in just a filename, check arguments
	 */
	if (mode == MODE_UNSET) {
		if (optind < argc) {
			if (optind + 1 < argc) {
				cerr << "Multiple arguments were specified"
					<< " after the options - not sure what"
					<< " to do, exiting\n";
				exit(1);
			}

			file_name = argv[optind];
			/*
			 * If no filename is set, and we have a filename,
			 * we're being expected to figure it out on our own!
			 */
			if (!stricmp(file_name,"connectivity.ezhex")) {
				mode = MODE_CONNECTIVITY;
			} else if (!stricmp(file_name,"update.ezhex")) {
				mode = MODE_WRITE_CONFIG;
			} else if (!stricmp(file_name,"learnir.eztut")) {
				mode = MODE_LEARN_IR;
			} else {
				cerr << "Don't know what to do with"
					<< file_name << endl;
				exit(1);
			}
			/*
		} else {
			cerr << "No file specified, and no mode specified..."
				<< " nothing to do.\n";
			exit(0);
			*/
		}
	}
}

void help()
{

	//cout << "Harmony Control " << VERSION << endl;
	cout << "There are two ways to invoke this software. You can specify"
		<< " what you want\nto do:\n";
	cout << "\tharmony <options>\n\n";
	cout << "Or you can let the software attempt to figure out what to"
		<< " do for you:\n";
	cout << "\tharmony <file>\n\n";

	cout << "In automatic mode, just pass in the file, and the software\n"
		<< " will use the filename to figure out the proper mode.\n\n";

	cout << "When specifying options, you must first choose a mode:\n\n";

	cout << "   -c, --dump-config [<filename>]\n"
		<< "\tRead the config from the remote and write it to a file."
		<< "\n\tIf no filename is specified, config.EZHex is used.\n\n";
	cout << "   -C, --write-config <filename>\n"
		<< "\tRead a config from <filename> and write it to the"
		<< " remote.\n\n";
	cout << "   -f, --dump-firmware [<filename>]\n"
		<< "\tRead firmware from the remote and write it to a file.\n"
		<< "\tIf no filename is specified firmware.EZUp is used.\n\n";
	cout << "   -F, --write-firmware <filename>\n"
		<< "\tRead firmware from <filename> and write it to the"
		<< " remote\n\n";
	cout << "   -s, --dump-safemode [<filename>]\n"
		<< "\tRead the safemode firmware from the remote and write it"
		<< " to a file.\n\tIf no filename is specified, safe.bin is"
		<< " used.\n\n";
	cout << "   -t, --connectivity-test <filename>\n"
		<< "\tDo a connectivity test using <filename>\n\n";
	cout << "   -r, --reset\n"
		<< "\tReset (power-cycle) the remote control\n\n";
	cout << "   -h, --help\n"
		<< "\tPrint this help message and exit.\n\n";
	cout << "   -l, --learn-ir <filename>\n"
		<< "\t Learn IR from other remotes. Use <filename>.\n\n";
	cout << "   -k, --get-time\n"
		<< "\t Get time from the remote\n";
	cout << "   -K, --set-time\n"
		<< "\t Set the remote's time clock\n\n";

	cout << "NOTE: If using the short options (e.g. -c), then *optional*"
		<< " arguements\nmust be adjacent: -c/path/to/file."
		<< " Required arguments don't have this\nlimitation, and can"
		<< " be specified -C /path/to/file or -C/path/to/file.\n\n";

	cout << "Additionally, you can specify options to adjust the behavior"
		<< "of the software:\n\n";

	cout << "  -b, --binary-only\n"
		<< "\tWhen dumping a config or firmware, this specifies to dump"
		<< " only the\n\tbinary portion. When use without a specific"
		<< " filename, the default\n\tfilename's extension is changed to"
		<< " .bin.\n"
		<< "\tWhen writing a config or firmware, this specifies the"
		<< " filename\n\tpassed in has just the binary blob, not the"
		<< " XML.\n\n";

	cout << "  -v, --verbose\n"
		<< "\tEnable verbose output.\n\n";

	cout << "  -w, --no-web\n"
		<< "\tDo not attempt to talk to the website. This is useful"
		<< " for\n\tre-programming the remote from a saved file, or for"
		<< " debugging.\n\n";

}


/*
 * Print all version info in a format readable by humans.
 */
int print_version_info(TRemoteInfo &ri, THIDINFO &hid_info,
					   struct options_t &options)
{
	if (ri.model->code_name) {
		printf("            Model: %s %s (%s)\n",ri.model->mfg,
			ri.model->model,ri.model->code_name);
	} else {
		printf("            Model: %s %s\n",ri.model->mfg,
			ri.model->model);
	}

	if (options.verbose)
		printf("             Skin: %i\n",ri.skin);

	printf(" Firmware Version: %i.%i\n",ri.fw_ver_major, ri.fw_ver_minor);

	if (options.verbose)
		printf("    Firmware Type: %i\n",ri.fw_type);

	printf(" Hardware Version: %i.%i\n",ri.hw_ver_major, ri.hw_ver_minor);

	if (options.verbose) {
		if ((ri.flash->size&0x03FF) == 0 && (ri.flash->size>>10)!=0) {
			printf("   External Flash: %i MiB - %02X:%02X %s\n",
				ri.flash->size>>10,ri.flash_mfg,
				ri.flash_id,ri.flash->part);
		} else {
			printf("   External Flash: %i KiB - %02X:%02X %s\n",
				ri.flash->size,ri.flash_mfg,ri.flash_id,
				ri.flash->part);
		}

		printf("     Architecture: %i\n",ri.architecture);
		printf("         Protocol: %i\n",ri.protocol);

		printf("\n     Manufacturer: %s\n",hid_info.mfg.c_str());
		printf("          Product: %s\n",hid_info.prod.c_str());
		printf("    IRL, ORL, FRL: %i, %i, %i\n",
			hid_info.irl,hid_info.orl,hid_info.frl);
		printf("          USB VID: %04X\n",hid_info.vid);
		printf("          USB PID: %04X\n",hid_info.pid);
		printf("          USB Ver: %04X\n",hid_info.ver);

		printf("\n    Serial Number: %s\n",
			ri.serial[0].c_str());
		printf("                   %s\n",ri.serial[1].c_str());
		printf("                   %s\n",ri.serial[2].c_str());
	}

	if (ri.flash->size == 0) {
		printf("Unsupported flash type\n");
		return 1;
	}

	if (ri.arch == NULL || ri.arch->cookie==0) {
		printf("Unsupported architecure\n");
		return 1;
	}

	if (ri.valid_config) {
		printf("Config Flash Used: %i%% (%i of %i KiB)\n\n",
			(ri.config_bytes_used*100+99) / ri.max_config_size,
			(ri.config_bytes_used+1023)>>10,
			(ri.max_config_size+1023)>>10);
	} else {
		printf("\nInvalid config!\n");
	}

	return 0;
}

/*
 * Pull the config from the remote and write it to a file
 */
int dump_config(TRemoteInfo &ri, struct options_t &options, char *file_name)
{
	int err = 0;

	if (!ri.valid_config) {
		printf("\nInvalid config - can not read\n");
		return 1;
	}

	printf("\nReading Config ");

	uint8_t *config = new uint8_t[ri.config_bytes_used];
	if ((err = rmt->ReadFlash(ri.arch->config_base, ri.config_bytes_used,
			config,ri.protocol))) {
		printf("Failed to read flash\n");
		return 1;
	}

	binaryoutfile of;
	if (options.binary) {
		of.open((file_name) ? file_name : "config.bin");
		of.write(config,ri.config_bytes_used);
	} else {
		uint32_t u = ri.config_bytes_used;
		uint8_t chk = 0x69;
		uint8_t *pc = config;
		while (u--)
			chk ^= *pc++;

		extern const char *config_header;
		char *ch = new char[strlen(config_header) + 200];
		const int chlen = sprintf(ch,
			config_header,ri.protocol,
			ri.skin,ri.flash_mfg,
			ri.flash_id,ri.hw_ver_major,
			ri.hw_ver_minor,ri.fw_type,
			ri.protocol,ri.skin,
			ri.flash_mfg,ri.flash_id,
			ri.hw_ver_major,ri.hw_ver_minor,
			ri.fw_type,
			ri.config_bytes_used,chk);
		of.open(file_name ? file_name : "config.EZHex");
		of.write(reinterpret_cast<uint8_t*>(ch),
			chlen);
		of.write(config,ri.config_bytes_used);
	}
	of.close();

	delete[] config;

	return 0;
}

/*
 * Do a connectivity test
 */
int connect_test(TRemoteInfo &ri, char *file_name, struct options_t &options)
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

	Post(buf,"POSTOPTIONS",ri,options);

	file.close();

	return 0;
}

/*
 * Read the config from a file and write it to the remote
 */
int write_config(TRemoteInfo &ri, char *file_name, struct options_t &options)
{
	int err = 0;
	binaryinfile file;

	if (options.binary) {
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

	if (!options.binary) {
		/*
		 * If we were passed in a full XML file, parse it, check
		 * that the size/checksum reported in it matches the binary
		 * blob, and print some data about it.
		 */
		uint8_t *t = x;
		string s;
		GetTag("BINARYDATASIZE",t,&s);
		uint32_t data_size = atoi(s.c_str());
		GetTag("CHECKSUM",t,&s);
		const uint8_t checksum = atoi(s.c_str());

		GetTag("/INFORMATION",y);
		y += 2;
		size -= (y-x);

		if (options.verbose) {
			printf("data size %i\n",data_size);
			printf("checksum %i\n",checksum);
			printf("size %i\n",size);
		}

		if (size != data_size)
			printf("Data size mismatch %i %i\n",size,data_size);

		uint32_t u=size;
		uint8_t chk=0x69;
		uint8_t *pc=y;
		while (u--)
			chk^=*pc++;
		if (chk != checksum)
			printf("Bad checksum %02X %02X\n",chk, checksum);

		if (file_name)
			Post(x,"POSTOPTIONS",ri,options);
	}

	/*
	 * We must invalidate flash before we erase and write so that
	 * nothing will attempt to reference it while we're working.
	 */
	if ((err = rmt->InvalidateFlash())) {
		delete[] x;
		printf("Failed to invalidate flash! Bailing out!\n");
		return err;
	}
	/*
	 * Flash can be changed to 0, but not back to 1, so you must
	 * erase the flash (to 1) in order to write the flash.
	 */
	if ((err = rmt->EraseFlash(ri.arch->config_base,size, ri))) {
		delete[] x;
		printf("Failed to erase flash! Bailing out!\n");
		return err;
	}

	if ((err = rmt->WriteFlash(ri.arch->config_base,size,y,
			ri.protocol))) {
		delete[] x;
		printf("Failed to write flash! Bailing out!\n");
		return err;
	}

	printf("Verifying Config:   ");
	if ((err = rmt->ReadFlash(ri.arch->config_base,size,y,
			ri.protocol,true))) {
		delete[] x;
		printf("Failed to read flash! Bailing out!\n");
		return err;
	}

	if (file_name && !options.binary)
		Post(x,"COMPLETEPOSTOPTIONS",ri,options);

	delete[] x;

	return 0;
}

int dump_safemode(TRemoteInfo &ri, char *file_name)
{
	binaryoutfile of;
	uint8_t * const safe = new uint8_t[64*1024];
	int err = 0;

	printf("\nReading Safe Mode Firmware ");
	if ((err = rmt->ReadFlash(ri.arch->flash_base,64*1024,safe,
							ri.protocol))) {
		delete[] safe;
		printf("Failed to read Safe Mode firmware.\n");
		return 1;
	}

	of.open((file_name) ? file_name : "safe.bin");
	of.write(safe,64*1024);
	of.close();

	delete[] safe;

	return 0;
}

int dump_firmware(TRemoteInfo &ri, struct options_t &options, char *file_name)
{
	binaryoutfile of;
	int err = 0;
	uint8_t * const firmware = new uint8_t[64*1024];

	printf("\nReading Firmware ");
	if ((err = rmt->ReadFlash(ri.arch->firmware_base,64*1024,firmware,
							ri.protocol))) {
		printf("Failed to read firmware.\n");
		return 1;
	}

	if (options.binary) {
		of.open((file_name) ? file_name : "firmware.bin");
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

		of.open((file_name) ? file_name : "firmware.EZUp");

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

int learn_ir_commands(TRemoteInfo &ri, char *file_name,
	struct options_t &options)
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
		rmt->LearnIR(&ls);
		//printf("%s\n",ls.c_str());

		Post(x,"POSTOPTIONS",ri,options,&ls,&keyname);
	} else {
		rmt->LearnIR();
	}

	return 0;
}

void print_harmony_time(const THarmonyTime &ht)
{
	static const char * const dow[8] =
	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "" };

#if 0
	printf("\nSecond      %i\n",ht.second);
	printf("Minute      %i\n",ht.minute);
	printf("Hour        %i\n",ht.hour);
	printf("Day         %i\n",ht.day);
	printf("DOW         %s\n",dow[ht.dow&7]);
	printf("Month       %i\n",ht.month);
	printf("Year        %i\n",ht.year);
	printf("UTC offset +%i\n",ht.utc_offset);
	printf("Timezone    %s\n",ht.timezone.c_str());
#endif

	printf("%04i/%02i/%02i %s %02i:%02i:%02i %+i %s\n",
		ht.year,ht.month,ht.day,dow[ht.dow&7],
		ht.hour,ht.minute,ht.second,ht.utc_offset,ht.timezone.c_str());
}

int get_time(TRemoteInfo &ri)
{
	THarmonyTime ht;
	int err;
	if ((err = rmt->GetTime(ri,ht))) return err;
	printf("The remote's time is currently ");
	print_harmony_time(ht);
	return 0;
}

int set_time(TRemoteInfo &ri)
{
	const time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	THarmonyTime ht;
	ht.second = lt->tm_sec;
	ht.minute = lt->tm_min;
	ht.hour = lt->tm_hour;
	ht.day = lt->tm_mday;
	ht.dow = lt->tm_wday;
	ht.month = lt->tm_mon + 1;
	ht.year = lt->tm_year + 1900;
	ht.utc_offset = 0;
	ht.timezone = "";

	printf("The remote's time has been set to ");
	print_harmony_time(ht);
	return rmt->SetTime(ri,ht);
}


int main(int argc, char *argv[])
{

#ifdef WIN32
	con=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(con,FOREGROUND_BLUE|FOREGROUND_INTENSITY);
#endif

	printf("Harmony Control %s\n", VERSION);
	printf("Copyright 2007 Kevin Timmerman and Phil Dibowitz\n");
	printf("This software is distributed under the GPLv3.\n\n");

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
	char *file_name = NULL;
	int mode = MODE_UNSET;
	parse_options(options, mode, file_name, argc, argv);
/*
	printf("filename is %s\n",file_name);
	exit(1);
*/

	/*
	 * If we're in "help" mode, do that and exit before we set too much up.
	 */
	if (mode == MODE_HELP) {
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

	if ((err = FindRemote(hid_info, options))) {
		printf("Unable to find a HID remote (error %i)\n",err);
		hid_info.pid=0;

#ifndef linux
		if ((err = FindUsbLanRemote())) {
			printf("Unable to find a TCP remote (error %i)\n",err);
			goto cleanup;
		}
		rmt = new CRemoteZ_TCP;
#else
		goto cleanup;
#endif
	}

	if(hid_info.pid == 0xC11F) {
		printf("Harmony 1000 requires Belcarra USB LAN driver\n");
		goto cleanup;
	}

	if (!rmt) {
		if (hid_info.pid == 0xC112)	// 890
			rmt = new CRemoteZ_HID;
		else
			rmt = new CRemote;
	}

	/*
	 * If we're in reset mode, not only do we not need to read and print
	 * data from the device, but it may not even be fully functional, so
	 * we probably want to do as little as possible. This we do this
	 * near the beginning instead of the SWITCH below.
	 */
	if (mode == MODE_RESET) {
		if ((err = rmt->Reset(COMMAND_RESET_DEVICE)))
			goto cleanup;
	}

	/*
	 * Get and print all the version info
	 */

	if ((err = rmt->GetIdentity(ri, hid_info)))
		goto cleanup;

	if ((err = print_version_info(ri, hid_info, options)))
		goto cleanup;

	/*
	 * Now do whatever we've been asked to do
	 */

	switch (mode) {
		case MODE_CONNECTIVITY:
			err = connect_test(ri, file_name, options);
			break;

		case MODE_DUMP_CONFIG:
			err = dump_config(ri, options, file_name);
			break;

		case MODE_WRITE_CONFIG:
			if ((err = write_config(ri, file_name, options)))
				break;
			err = rmt->Reset(COMMAND_RESET_DEVICE);
			break;

		case MODE_DUMP_FIRMWARE:
			err = dump_firmware(ri, options, file_name);
			break;

		case MODE_WRITE_FIRMWARE:
			printf("This isn't supported yet.\n");
			/*
			 * FIXME: Implement this.
			 */
			break;

		case MODE_DUMP_SAFEMODE:
			err = dump_safemode(ri, file_name);
			break;

		case MODE_LEARN_IR:
			err = learn_ir_commands(ri, file_name, options);
			break;

		case MODE_GET_TIME:
			err = get_time(ri);
			break;

		case MODE_SET_TIME:
			err = set_time(ri);
			break;
/*
		default:
			cerr << "ERROR: Got to a place I don't understand!\n";
			break;
*/
	}
			
cleanup:
	ShutdownUSB();

	delete rmt;

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
