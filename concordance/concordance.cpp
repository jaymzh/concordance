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

#ifdef WIN32
#include "win/getopt/getopt.h"
#include <conio.h>
#include <winsock.h>
HANDLE con;
CONSOLE_SCREEN_BUFFER_INFO sbi;
#else
#include <getopt.h>
#endif

#define VERSION "0.9"

#define MODE_UNSET 0
#define MODE_CONNECTIVITY 1
#define MODE_DUMP_CONFIG 2
#define MODE_WRITE_CONFIG 3
#define MODE_DUMP_FIRMWARE 4
#define MODE_WRITE_FIRMWARE 5
#define MODE_DUMP_SAFEMODE 6
#define MODE_HELP 7
#define MODE_LEARN_IR 8
#define MODE_RESET 9

struct options_t {
	bool binary;
	bool verbose;
};

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
		{"verbose", no_argument, 0, 'v'},
		{0,0,0,0} // terminating null entry
	};

	options.verbose = false;
	options.binary = false;

	mode = MODE_UNSET;

	int tmpint = 0;

	while ((tmpint = getopt_long(argc, argv, "bc::C:f::F:hl:rs::t:v",
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
		case 'v':
			options.verbose = true;
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
		<< " to a file.\n\tIf no filename is psecified, safe.bin is"
		<< " used.\n\n";
	cout << "   -t, --connectivity-test <filename>\n"
		<< "\tDo a connectivity test using <filename>\n\n";
	cout << "   -r, --reset\n"
		<< "\tReset (power-cycle) the remote control\n\n";
	cout << "   -h, --help\n"
		<< "\tPrint this help message and exit.\n\n";
	cout << "   -l, --learn-ir <filename>\n"
		<< "\t Learn IR from other remotes. Use <filename>.\n\n";

	cout << "NOTE: If using the short options (e.g. -c), then *optional*"
		<< " arguements\nmust be adjacent: -c/path/to/file."
		<< " Required arguments don't have this\nlimitation, and can"
		<< " be specified -C /path/to/file or -C/path/to/file.\n\n";

	cout << "Additionally, you can specify options to adjust the behavior"
		<< "of the software:\n\n";

	cout << "  -b, --binary-only\n"
		<< "\tWhen dumping a config or firmware, this specifies to dump"
		<< "only the\n\tbinary portion. When use without a specific"
		<< "filename, the default\n\tfilename's extension is changed to"
		<< " .bin.\n"
		<< "\tWhen writing a config or firmware, this specifies the"
		<< "filename\n\tpassed in has just the binary blob, not the"
		<< "XML.\n\n";

	cout << "  -v, --verbose\n"
		<< "\tEnable verbose output.\n\n";
}

/*
 * Send the GET_VERSION command to the remote, and read the response.
 *
 * Then populate our struct with all the relevant info.
 */
int get_version_info(TRemoteInfo &ri)
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
	uint8_t ser[48];
	if ((err = ri.architecture==2 ?
		// The old 745 stores the serial number in EEPROM
		ReadMiscByte(0x10,48,COMMAND_MISC_EEPROM,ser) :
		// All newer models store it in Flash
		ReadFlash(0x000110,48,ser,ri.protocol))) {
		printf("Failed to read flash\n");
		return 1;
	}

	make_guid(ser,ri.serial[0]);
	make_guid(ser+16,ri.serial[1]);
	make_guid(ser+32,ri.serial[2]);


	printf("\n");

	return 0;
}

int get_time(const TRemoteInfo &ri)
{
	// NOTE: This is experimental
	printf("\n\n");
	if(ri.architecture < 8) {
		uint8_t tsv[8];
		ReadMiscByte(0,6,COMMAND_MISC_STATE,tsv);
		printf("Second %i\n",tsv[0]);
		printf("Minute %i\n",tsv[1]);
		printf("Hour   %i\n",tsv[2]);
		printf("Day    %i\n",1+tsv[3]);
		printf("Month  %i\n",1+tsv[4]);
		printf("Year   %i\n",2000+tsv[5]);
	} else {
		static const char * const dow[8] =
		{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "???" };
		uint16_t tsv[8];
		ReadMiscWord(0,7,COMMAND_MISC_STATE,tsv);
		printf("Second %i\n",tsv[0]);
		printf("Minute %i\n",tsv[1]);
		printf("Hour   %i\n",tsv[2]);
		printf("Day    %i\n",1+tsv[3]);
		printf("DOW    %s\n",dow[tsv[4]&7]);
		printf("Month  %i\n",1+tsv[5]);
		printf("Year   %i\n",2000+tsv[6]);
	}
	printf("\n");
	return 0;
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

	printf(" Firmware Version: %i.%i\n",ri.fw_ver>>4, ri.fw_ver&0x0F);

	if (options.verbose)
		printf("    Firmware Type: %i\n",ri.fw_type);

	printf(" Hardware Version: %i.%i\n",ri.hw_ver>>4, ri.hw_ver&0x0F);

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
	}

	if (options.verbose)
		printf("     Architecture: %i\n",ri.architecture);

	if (options.verbose)
		printf("         Protocol: %i\n",ri.protocol);

	if (options.verbose) {
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

	if (options.verbose) {
		printf("\n    Serial Number: %s\n",
			ri.serial[0].c_str());
		printf("                   %s\n",ri.serial[1].c_str());
		printf("                   %s\n",ri.serial[2].c_str());
	}

	if (ri.valid_config)
		printf("\nConfig Flash Used: %i%% (%i of %i KiB)\n",
			(ri.config_bytes_used*100+99) / ri.max_config_size,
			(ri.config_bytes_used+1023)>>10,
			(ri.max_config_size+1023)>>10);
	else
		printf("\nInvalid config!\n");

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
	if ((err = ReadFlash(ri.arch->config_base, ri.config_bytes_used,
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
			ri.flash_id,ri.hw_ver>>4,
			ri.hw_ver&0x0F,ri.fw_type,
			ri.protocol,ri.skin,
			ri.flash_mfg,ri.flash_id,
			ri.hw_ver>>4,ri.hw_ver&0x0F,
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
	 * We must invalidate flash before we erase and write so that
	 * nothing will attempt to reference it while we're working.
	 */
	if (options.verbose)
		printf("Invalidating flash... ");
	if ((err=InvalidateFlash())) {
		delete[] x;
		printf("Failed to invalidate flash! Bailing out!\n");
		return err;
	}
	/*
	 * Flash can be changed to 0, but not back to 1, so you must
	 * erase the flash (to 1) in order to write the flash.
	 */
	if ((err = EraseFlash(ri.arch->config_base,size, ri))) {
		delete[] x;
		printf("Failed to erase flash! Bailing out!\n");
		return err;
	}

	printf("\nWrite Config ");
	if ((err=WriteFlash(ri.arch->config_base,size,y,
			ri.protocol))) {
		delete[] x;
		printf("Failed to write flash! Bailing out!\n");
		return err;
	}

	printf("\nVerify Config ");
	if ((err=ReadFlash(ri.arch->config_base,size,y,
			ri.protocol,true))) {
		delete[] x;
		printf("Failed to read flash! Bailing out!\n");
		return err;
	}

	if (file_name && !options.binary)
		Post(x,"COMPLETEPOSTOPTIONS",ri);

	delete[] x;

	return 0;
}

int dump_safemode(TRemoteInfo &ri, char *file_name)
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
	if ((err = ReadFlash(ri.arch->firmware_base,64*1024,firmware,
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

	if ((err = FindRemote(hid_info))) {
		printf("Unable to find a remote (error %i)\n",err);
		goto cleanup;
	}

	/*
	 * If we're in reset mode, not only do we not need to read and print
	 * data from the device, but it may not even be fully functional, so
	 * we probably want to do as little as possible. This we do this
	 * near the beginning instead of the SWITCH below.
	 */
	if (mode == MODE_RESET) {
		if ((err = ResetHarmony(COMMAND_RESET_DEVICE)))
			goto cleanup;
	}

	/*
	 * Get and print all the version info
	 */

	if ((err = get_version_info(ri)))
		goto cleanup;

	if ((err = print_version_info(ri, hid_info, options)))
		goto cleanup;

	// NOTE: Experimental
	if (options.verbose) get_time(ri);

	/*
	 * Now do whatever we've been asked to do
	 */

	switch (mode) {
		case MODE_CONNECTIVITY:
			err = connect_test(ri, file_name);
			break;

		case MODE_DUMP_CONFIG:
			err = dump_config(ri, options, file_name);
			break;

		case MODE_WRITE_CONFIG:
			if ((err = write_config(ri, file_name, options)))
				break;
			err = ResetHarmony(COMMAND_RESET_DEVICE);
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
			err = learn_ir_commands(ri, file_name);
			break;
/*
		default:
			cerr << "ERROR: Got to a place I don't understand!\n";
			break;
*/
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
