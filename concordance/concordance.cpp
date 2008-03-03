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

// Platform-agnostic includes
#include "libharmony.h"
#include <iostream>
using namespace std;

#ifdef WIN32
// Windows includes
#include "win/getopt/getopt.h"
#include <conio.h>
#include <winsock.h>

#define strcasecmp stricmp
#define strncasecmp strnicmp

/*
 * Windows, in it's infinite awesomeness doesn't include POSIX things
 * like basename. This little hack will work for non-unicode filenames.
 * Thanks to Marco Bleich.
 */
char* basename(char* file_name)
{
	char* _basename = strrchr(file_name, '\\');

	return _basename ? _basename+1 : file_name;
}

HANDLE con;
CONSOLE_SCREEN_BUFFER_INFO sbi;

#else // non-Windows

// *nix includes
#include <getopt.h>
#include <strings.h>

#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <libgen.h>
#endif

#define DEFAULT_CONFIG_FILENAME "Update.EZHex"
#define DEFAULT_CONFIG_FILENAME_BIN "update.bin"
#define DEFAULT_FW_FILENAME "firmware.EZUp"
#define DEFAULT_FW_FILENAME_BIN "firmware.bin"
#define DEFAULT_SAFE_FILENAME "safe.bin"

const char * const VERSION = "0.13+CVS";

struct options_t {
	bool binary;
	bool verbose;
	bool noweb;
	bool direct;
};

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
	MODE_SET_TIME,
	MODE_PRINT_INFO
};

/*
 * Start callbacks
 */
void cb_print_percent_status(uint32_t count, uint32_t curr, uint32_t total,
	void *arg)
{
#ifdef WIN32
	GetConsoleScreenBufferInfo(con, &sbi);
	SetConsoleCursorPosition(con, sbi.dwCursorPosition);
#else                   
	if (count != 0) {
		printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
	}
#endif          
	bool is_bytes = false;
	if (arg) {
		is_bytes = static_cast<bool>(arg);
	}

	if (is_bytes) {
		printf("%3i%%  %4i KiB", curr*100/total, curr>>10);
	} else {
		printf("%3i%%          ", curr*100/total);
	}
       	fflush(stdout);
}

/*
 * Start helper functions
 */
void direct_warning()
{
	printf("WARNING: You have requested direct mode. This only affects\n");
	printf("\t firmware updates. To do this safely you MUST be in\n");
	printf("\t SAFEMODE! This is only for those devices where we\n");
	printf("\t don't yet support live firmware updates. See the docs\n");
	printf("\t for information on how to boot into safemode.\n");
	printf("\t Press <enter> to continue.\n");
}

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

int dump_config(struct options_t &options, char *file_name,
	lh_callback cb, void *cb_arg)
{
	int err = 0;

	uint8_t *config;
	uint32_t size = 0;

	if ((err = read_config_from_remote(&config, &size, cb, (void *)true))) {
		return err;
	}

	if ((err = write_config_to_file(config, file_name, size,
			(int)options.binary))) {
		return err;
	}

	delete[] config;

	return 0;
}

void print_time(int action)
{
	static const char * const dow[8] =
	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "" };

	if (action == 0) {
		printf("Remote time is ");
	} else {
		printf("Remote time has been set to ");
	}

	printf("%04i/%02i/%02i %s %02i:%02i:%02i %+i %s\n",
		get_time_year(), get_time_month(), get_time_day(),
		dow[get_time_dow() & 7], get_time_hour(), get_time_minute(),
		get_time_second(), get_time_utc_offset(), get_time_timezone());
}



/*
 * Read the config from a file and write it to the remote
 */
int upload_config(char *file_name, struct options_t &options,
	lh_callback cb, void *cb_arg)
{
	int err = 0;

	uint8_t *data;
	uint32_t size = 0;

	read_config_from_file(file_name, &data, &size);

	uint8_t *place_ptr = data;
	uint32_t binsize = size;

	if (!options.binary) {

		if ((err = verify_xml_config(data, size)))
			return LH_ERROR;

		if ((err = find_binary_size(data, &binsize)))
			return LH_ERROR;

		// We no longer need size, let it get munged...
		if ((err = find_binary_start(&place_ptr, &size)))
			return LH_ERROR;

		if (size < binsize)
			return LH_ERROR;

		if (!options.noweb)
			post_preconfig(data);
	}

	/*
	 * FIXME:
	 * 	While having top-level functions like this is useful
	 * 	we probably want to expose functions for invalidate,
	 * 	read, write, etc., and then have this just be a short
	 * 	cut for those.
	 */

	/*
	 * We must invalidate flash before we erase and write so that
	 * nothing will attempt to reference it while we're working.
	 */
	printf("Invalidating Flash:  ");
	if ((err = invalidate_flash())) {
		delete[] data;
		return err;
	}
	printf("                     done\n");
	/*
	 * Flash can be changed to 0, but not back to 1, so you must
	 * erase the flash (to 1) in order to write the flash.
	 */
	printf("Erasing Flash:       ");
	if ((err = erase_config(binsize, cb, (void *)false))) {
		delete[] data;
		return err;
	}
	printf("       done\n");

	printf("Writing config:      ");
	if ((err = write_config_to_remote(place_ptr, binsize, cb,
			(void *)true))) {
		delete[] data;
		return err;
	}
	printf("       done\n");

	printf("Verifying Config:    ");
	if ((err = verify_remote_config(place_ptr, binsize, cb, (void *)true))) {
		delete[] data;
		return err;
	}
	printf("       done\n");

	if (!options.binary && !options.noweb) {
		printf("Contacting website:  ");
		if ((err = post_postconfig(data))) {
			delete[] data;
			return err;
		}
		printf("                     done\n");
	}

	delete[] data;

	return 0;
}

int dump_safemode(char *file_name, lh_callback cb, void *cb_arg)
{
	uint8_t * safe = 0;
	int err = 0;

	if ((err = read_safemode_from_remote(&safe, cb, cb_arg))) {
		delete[] safe;
		return err;
	}

	if ((err = write_safemode_to_file(safe, file_name))) {
		delete[] safe;
		return err;
	}

	delete[] safe;
	return 0;
}

int upload_firmware(char *file_name, struct options_t &options,
	lh_callback cb, void *cb_arg)
{
	int err = 0;

	if ((err = is_fw_update_supported()) && !options.direct) {
		return err;
	}

	if (options.direct)
		direct_warning();

	uint8_t *firmware = 0;

	/*
	 * It may take a second for the device to initialize after the user
	 * plugs into USB. As such rather than a useless sleep(), we'll
	 * read in the firmware from the file while we wait.
	 */
	if ((err = read_firmware_from_file(file_name, &firmware,
			options.binary))) {
		delete[] firmware;
		return err;
	}

	if (!options.direct) {
		if ((err = prep_firmware())) {
			printf("Failed to prepare remote for FW update\n");
			delete[] firmware;
			return err;
		}
	}

	printf("Invalidating Flash:  ");
	if ((err = invalidate_flash())) {
		delete[] firmware;
		return err;
	}
	printf("                     done\n");

	printf("Erasing Flash:       ");
	if ((err = erase_firmware(options.direct, cb, (void *)false))) {
		delete[] firmware;
		return err;
	}
	printf("       done\n");

	printf("Writing firmware:    ");
	if ((err = write_firmware_to_remote(firmware, options.direct, cb,
			cb_arg))) {
		delete[] firmware;
		return err;
	}
	printf("       done\n");

	if (!options.direct) {
		if ((err = finish_firmware())) {
			printf("Failed to finalize FW update\n");
			delete[] firmware;
			return err;
		}
	}
	delete[] firmware;
	return 0;
}

int dump_firmware(struct options_t &options, char *file_name,
	lh_callback cb, void *cb_arg)
{
	int err = 0;
	uint8_t *firmware = 0;

	if ((err = read_firmware_from_remote(&firmware, cb, cb_arg))) {
		delete[] firmware;
		return err;
	}

	if ((err = write_firmware_to_file(firmware, file_name,
			(int)options.binary))) {
		delete[] firmware;
		return err;
	}

	delete[] firmware;
	return 0;
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
		{"direct", no_argument, 0, 'd'},
		{"dump-firmware", optional_argument, 0, 'f'},
		{"write-firmware", required_argument, 0, 'F'},
		{"help", no_argument, 0, 'h'},
		{"print-remote-info", no_argument, 0, 'i'},
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
	options.direct = false;

	mode = MODE_UNSET;

	int tmpint = 0;

	while ((tmpint = getopt_long(argc, argv, "bc::C:df::F:hil:rs::t:kKvw",
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
		case 'd':
			options.direct = true;
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
			file_name = optarg;
			break;
		case 'h':
			set_mode(mode, MODE_HELP);
			break;
		case 'i':
			set_mode(mode, MODE_PRINT_INFO);
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
			 * FIXME: We'll attempt to figure out what to
			 *        do based on filename. This is fragile
			 *        and should be done based on some
			 *        metadata int he file... but this will
			 *        do for now.
			 */

			/*
			 * Dup our string since POSIX basename()
			 * may modify it.
			 */
			char *file_name_copy = strdup(file_name);
			char *file = basename(file_name_copy);

			if (!strcasecmp(file,"connectivity.ezhex")) {
				mode = MODE_CONNECTIVITY;
			} else if (!strcasecmp(file,"update.ezhex")) {
				mode = MODE_WRITE_CONFIG;
			} else if (!strcasecmp(file,"learnir.eztut")) {
				mode = MODE_LEARN_IR;
			} else {
				cerr << "Don't know what to do with"
					<< file_name << endl;
				exit(1);
			}
			free(file_name_copy);
			/*	
			 * Since basename returns a pointer to
			 * file_name_copy, if we free(file), we get
			 * a double-free bug.
			 */
		}
	}
}

void help()
{
	cout << "There are two ways to invoke this software. You can specify"
		<< " what you want\nto do:\n";
	cout << "\tharmony <options>\n\n";
	cout << "Or you can let the software attempt to figure out what to"
		<< " do for you:\n";
	cout << "\tharmony <file>\n\n";

	cout << "In automatic mode, just pass in the file, and the software"
		<< " will use the\nfilename to figure out the proper mode.\n\n";

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
	cout << "   -i, --print-remote-info\n"
		<< "\tPrint information about the remote. Additional"
		<< " information will\n\tbe printed if -v is also used.\n\n";
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
	cout << "   -k, --get-time\n"
		<< "\t Get time from the remote\n\n";
	cout << "   -K, --set-time\n"
		<< "\t Set the remote's time clock\n\n";

	cout << "NOTE: If using the short options (e.g. -c), then *optional*"
		<< " arguements\nmust be adjacent: -c/path/to/file."
		<< " Required arguments don't have this\nlimitation, and can"
		<< " be specified -C /path/to/file or -C/path/to/file.\n\n";

	cout << "Additionally, you can specify options to adjust the behavior"
		<< " of the software:\n\n";

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
int print_version_info(struct options_t &options)
{

	printf("  Model: %s %s", get_mfg(), get_model());
	const char *cn = get_codename();

	if (strcmp(cn,"")) {
		printf(" (%s)\n", cn);
	} else {
		printf("\n");
	}

	if (options.verbose)
		printf("  Skin: %i\n", get_skin());

	printf("  Firmware Version: %i.%i\n", get_fw_ver_maj(),
		get_fw_ver_min());

	if (options.verbose)
		printf("  Firmware Type: %i\n", get_fw_type());

	printf("  Hardware Version: %i.%i\n", get_hw_ver_maj(),
		get_hw_ver_min());

	if (options.verbose) {
		//if ((ri.flash->size & 0x03FF) == 0 && (ri.flash->size>>10)!=0) {
		int size = get_flash_size();
		printf("  External Flash: ");
		if (size >> 10 != 0) {
			printf("%i MiB", size >> 10);
		} else {
			printf("%i KiB", size);
		}
		printf(" - %02X:%02X %s\n", get_flash_mfg(),
			get_flash_id(), get_flash_part_num());

		printf("  Architecture: %i\n", get_arch());
		printf("  Protocol: %i\n", get_proto());

		printf("  Manufacturer: %s\n", get_hid_mfg_str());
		printf("  Product: %s\n", get_hid_prod_str());
		printf("  IRL, ORL, FRL: %i, %i, %i\n", get_hid_irl(),
			get_hid_orl(), get_hid_frl());
		printf("  USB VID: %04X\n", get_usb_vid());
		printf("  USB PID: %04X\n", get_usb_pid());
		printf("  USB Ver: %04X\n", get_usb_bcd());

		printf("  Serial Number: %s\n\t%s\n\t%s\n", get_serial(1),
			get_serial(2), get_serial(3));
	}

	int used = get_config_bytes_used();
	int total = get_config_bytes_total();
	printf("  Config Flash Used: %i%% (%i of %i KiB)\n\n",
		(used*100+99) / total, (used+1023)>>10, (total+1023)>>10);

	return 0;
}

/*
 * In certain cases we don't require filenames, this provides
 * sane defaults.
 */
void populate_default_filename(int mode, struct options_t &options,
	char *&file_name)
{
	switch (mode) {
		case MODE_DUMP_CONFIG:
			if (options.binary) {
				file_name = strdup(DEFAULT_CONFIG_FILENAME_BIN);
			} else {
				file_name = strdup(DEFAULT_CONFIG_FILENAME);
			}
			break;

		case MODE_DUMP_FIRMWARE:
			if (options.binary) {
				file_name = strdup(DEFAULT_FW_FILENAME_BIN);
			} else {
				file_name = strdup(DEFAULT_FW_FILENAME);
			}
			break;

		case MODE_DUMP_SAFEMODE:
			file_name = strdup(DEFAULT_SAFE_FILENAME);
			break;
	}
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
#endif

	struct options_t options;
	char *file_name = NULL;
	int mode = MODE_UNSET;
	//struct CRemoteBase *rmt = NULL;

	parse_options(options, mode, file_name, argc, argv);

	if (mode == MODE_UNSET) {
		printf("No mode requested. No work to do.\n");
		exit(1);
	}

	/*
	 * If we're in "help" mode, do that and exit before we set too much up.
	 */
	if (mode == MODE_HELP) {
		help();
		exit(0);
	}

	/*
	 * In a few special cases, we populate a default filename. NEVER
	 * if we're are writing to the device, however.
	 */
	if (!file_name) {
		populate_default_filename(mode, options, file_name);
	}

	int err = 0;

	err = init_harmony();
	if (err != 0) {
		printf("Error initializing harmony libraries: %s\n",
			lh_strerror(err));
		exit(1);
	}

	/*
	 * If we're in reset mode, not only do we not need to read and print
	 * data from the device, but it may not even be fully functional, so
	 * we probably want to do as little as possible. This we do this
	 * near the beginning instead of the SWITCH below.
	 */
	if (mode == MODE_RESET) {
		printf("Reseting...\n");
		reset_remote();
		goto cleanup;
	}

	/*
	 * Get and print all the version info
	 */

	printf("Requesting Identity: ");
	if (get_identity(cb_print_percent_status, NULL) != 0) {
		printf("Error requesting identity\n");
		goto cleanup;
	}
	printf("       done\n");

	/*
	 * Now do whatever we've been asked to do
	 */

	switch (mode) {
		case MODE_PRINT_INFO:
			err = print_version_info(options);
			break;

		case MODE_CONNECTIVITY:
			if (!options.noweb)
				printf("Contacting website:  ");
				err = post_connect_test_success(file_name);
				printf("       done\n");
			break;

		case MODE_DUMP_CONFIG:
			printf("Dumping config:      ");
			err = dump_config(options, file_name,
				cb_print_percent_status, NULL);
			if (err != 0) {
				printf("Failed to dump config: %s\n",
					lh_strerror(err));
			} else {
				printf("       done\n");
			}
			break;

		case MODE_WRITE_CONFIG:
			err = upload_config(file_name, options,
				cb_print_percent_status, NULL);
			if (err != 0)
				break;
			err = reset_remote();
			break;

		case MODE_DUMP_FIRMWARE:
			printf("Dumping firmware:    ");
			err = dump_firmware(options, file_name,
				cb_print_percent_status, NULL);
			if (err != 0) {
				printf("Failed to dump firmware: %s\n",
					lh_strerror(err));
			} else {
				printf("       done\n");
			}
			break;

		case MODE_WRITE_FIRMWARE:
			err = upload_firmware(file_name, options,
				cb_print_percent_status, NULL);
			if (err != 0) {
				printf("Failed to upload firmware: %s\n",
					lh_strerror(err));
				break;
			}
			err = reset_remote();
			//printf("This isn't supported yet.\n");
			/*
			 * FIXME: Implement this.
			 */
			break;

		case MODE_DUMP_SAFEMODE:
			printf("Dumping safemode fw: ");
			err = dump_safemode(file_name, cb_print_percent_status,
				NULL);
			if (err != 0) {
				printf("Failed to dump safemode: %s\n",
					lh_strerror(err));
			} else {
				printf("       done\n");
			}
			break;

		case MODE_LEARN_IR:
			err = learn_ir_commands(file_name, (int)!options.noweb);
			break;

		case MODE_GET_TIME:
			err = get_time();
			print_time(0);
			break;

		case MODE_SET_TIME:
			err = set_time();
			print_time(1);
			break;

		default:
			cerr << "ERROR: Got to a place I don't understand!\n";
			break;
	}
			
cleanup:

	deinit_harmony();

	if (err) {
		printf("Failed with error %i\n",err);
	} else {
		printf("Success!\n");
	}
		
#ifdef WIN32
	// Shutdown WinSock
	WSACleanup();
#endif

#ifdef _DEBUG
	printf("\nPress any key to exit");
	getchar();
#endif // debug

	return err;
}

