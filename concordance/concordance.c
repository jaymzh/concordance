/*
 * vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
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

/* Platform-agnostic includes */
#include <getopt.h>
#include <libconcord.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
/* Windows includes*/
#include <conio.h>
#include <winsock.h>

/*
 * (see below) Windows does not need this, since _getch already
 * does what we need - just make set_canon do nothing 
 * Note that _getch returns '\r' when user hits the <Enter> key
 */
int set_canon(int flag)
{
    return(1);
}

#define read_key _getch
#define ENTER_KEY '\r'

#else
/* NON-Windows */

#include <strings.h>
#include <libgen.h>
#include <unistd.h>
#include <termios.h>

/*
 * set_canon in LINUX modifies stdin such that getchar behaves like
 * _getch in Windows, i.e. the next key is returned immediately.
 * Note that getchar returns '\n' when user hits the <Enter> key
 */
int set_canon(int flag)
{
    struct termios t;

    tcgetattr(0, &t);
    if (flag) {
        t.c_lflag |= ICANON;
    } else {
        t.c_lflag &= ~ICANON;
    }
    tcsetattr(0, TCSANOW, &t); 
    return(1);
}
/* Thus, we can define: */
#define read_key getchar
#define ENTER_KEY '\n'

#endif

#define MAX_WAIT_FOR_BOOT 5
#define WAIT_FOR_BOOT_SLEEP 5

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <libgen.h>
#endif

#define DEFAULT_CONFIG_FILENAME "Update.EZHex"
#define DEFAULT_CONFIG_FILENAME_BIN "update.bin"
#define DEFAULT_FW_FILENAME "firmware.EZUp"
#define DEFAULT_FW_FILENAME_BIN "firmware.bin"
#define DEFAULT_SAFE_FILENAME "safe.bin"

const char * const VERSION = "1.5";

struct options_t {
    int binary;
    int verbose;
    int noweb;
    int direct;
    int noreset;
    int force;
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
    MODE_PRINT_INFO,
    MODE_VERSION
};

/*
 * Tack a colon onto the end, and then right-pad it out to 23-chars. This means
 * we can have strings of up to 21 chars (21 chars, 1 colon, and 1 space before
 * the percentages start).
 */
void print_stage_name(int stage_id)
{
    const char *stage;
    char output[24]; /* 21 + 1 + 1 + \0 */
    stage = lc_cb_stage_str(stage_id);
    strcpy(output, stage);
    strcat(output, ":");
    /* -22 is a 23-char-wide left-aligned string */
    printf("%-22s", output);
    return;
}

/*
 * Start callbacks
 *
 *   stage_id => integer of the stage
 *   count => number of times we've been invoked
 *   curr => current position (bytes, precent, etc.)
 *   total => the number 'curr' is trying to reach
 *   counter_type => what 'curr' represents (bytes, percent, etc.)
 *   arg => an extra value we can ask libconcord to pass us.
 *   stages => the array of stages that will be performed
 */
void cb_print_percent_status(uint32_t stage_id, uint32_t count, uint32_t curr,
    uint32_t total, uint32_t counter_type, void *arg,
    const uint32_t *stages)
{
    if (stage_id == LC_CB_STAGE_NUM_STAGES) {
#if _DEBUG
        printf("Num stages: %d\n", count);
#endif
        return;
    }

    if (count != 0) {
        printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
    } else {
        print_stage_name(stage_id);
    }

    if (counter_type == LC_CB_COUNTER_TYPE_BYTES) {
        printf("%3i%%  %4i KiB", curr*100/total, curr>>10);
    } else {
        printf("%3i%%          ", curr*100/total);
    }

    if (curr == total) {
        printf("   done\n");
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
    getchar();
}

void set_mode(int *mode, int val)
{
    if (*mode == MODE_UNSET) {
        *mode = val;
    } else {
        fprintf(stderr,
            "ERROR: More than one mode specified. Please specify");
        fprintf(stderr, " only one.\n");
        exit(1);
    }
}

void print_ir_burst(uint32_t length)
{
    if (length < 250) {
        printf("|");
    } else if (length < 1000) {
        printf("#");
    } else {
        printf("##");
    }
}

void print_ir_space(uint32_t length)
{
    if (length < 250) {
        printf(".");
    } else if (length < 1000) {
        printf("_");
    } else if (length < 10000) {
        printf("__");
    } else {
        printf("\n");
    }
}

void print_received_ir_signal(uint32_t carrier_clock, uint32_t *ir_signal,
    uint32_t ir_signal_length, struct options_t *options)
{
    uint32_t index;
    printf("\nASCII-graph of received IR signal:\n");
    for (index = 0; index < ir_signal_length; index += 2){
        print_ir_burst(ir_signal[index]);
        print_ir_space(ir_signal[index+1]);
    }
    printf("\n");
    printf("Carrier clock          : %u Hz\n", carrier_clock);
    printf("Total mark/space pairs : %u\n\n", ir_signal_length/2);
#ifdef _DEBUG
    /*
     * full dump of new IR signal:
     */
    for (index=0; index < ir_signal_length; index += 2){
        printf("\tP:%6u\tS:%6u\n", ir_signal[index],
            ir_signal[index+1]);
    }
#endif
}

char get_cmd(char *prompt, char *allowed, char def) {
    char result = 0;
    char got_key;
    uint32_t index;
    set_canon(0);
    while ( result == 0 ) {
        printf("%s (%c)?", prompt, def);
        got_key = read_key();
        printf("\n");
        if (got_key == ENTER_KEY) {
            result = def;
        } else {
            for (index = 0; index < strlen(allowed); index++) {
                if (allowed[index] == got_key) {
                    result = got_key;
                    break;
                }
            }
        }
    }
    set_canon(1);
    return result;
}    

char *mode_string(int mode)
{
    switch (mode) {
    case MODE_CONNECTIVITY:
        return "Connectivity Check";
        break;
    case MODE_WRITE_CONFIG:
        return "Write Configuration";
        break;
    case MODE_WRITE_FIRMWARE:
        return "Write Firmware";
        break;
    case MODE_LEARN_IR:
        return "Learn IR";
        break;
    default:
        return "Unknown";
        break;
    }
}

void report_mode_mismatch(int mode, int file_mode, int force)
{
    if (force) {
        fprintf(stderr, "WARNING:");
    } else {
        fprintf(stderr, "ERROR:");
    }
    fprintf(stderr, " Requested mode is: %s\n", mode_string(mode));
    fprintf(stderr, "       but file detected as: %s\n",
            mode_string(file_mode));
    fprintf(stderr, "Try not specifying a mode at all, I will figure it");
    fprintf(stderr, " out for you.\n");
}

/*
 * In certain cases we don't require filenames, this provides
 * sane defaults.
 */
void populate_default_filename(int mode, int binary, char **file_name)
{
    switch (mode) {
        case MODE_DUMP_CONFIG:
            if (binary) {
                *file_name = (char *)
                strdup(DEFAULT_CONFIG_FILENAME_BIN);
            } else {
                *file_name = (char *)
                strdup(DEFAULT_CONFIG_FILENAME);
            }
            break;

        case MODE_DUMP_FIRMWARE:
            if (binary) {
                *file_name = (char *)
                strdup(DEFAULT_FW_FILENAME_BIN);
            } else {
                *file_name = (char *)
                strdup(DEFAULT_FW_FILENAME);
            }
            break;

        case MODE_DUMP_SAFEMODE:
            *file_name = (char *) strdup(DEFAULT_SAFE_FILENAME);
            break;
    }
}

/*
 * Parse our options.
 */
void parse_options(struct options_t *options, int *mode, char **file_name,
                   int argc, char *argv[])
{
    int tmpint, option_index;

    static struct option long_options[] = {
        {"binary", no_argument, 0, 'b'},
        {"dump-config", optional_argument, 0, 'c'},
        {"write-config", required_argument, 0, 'C'},
        {"direct", no_argument, 0, 'd'},
        {"force", no_argument, 0, 0},
        {"dump-firmware", optional_argument, 0, 'f'},
        {"write-firmware", required_argument, 0, 'F'},
        {"help", no_argument, 0, 'h'},
        {"print-remote-info", no_argument, 0, 'i'},
        {"learn-ir", required_argument, 0, 'l'},
        {"reset", no_argument, 0, 'r'},
        {"no-reset", no_argument, 0, 'R'},
        {"dump-safemode", optional_argument, 0, 's'},
        {"connectivity-test", required_argument, 0, 't'},
        {"get-time", no_argument, 0, 'k' },
        {"set-time", no_argument, 0, 'K' },
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 'V'},
        {"no-web", no_argument, 0, 'w'},
        {0,0,0,0} /* terminating null entry */
    };

    (*options).verbose = 0;
    (*options).binary = 0;
    (*options).noweb = 0;
    (*options).direct = 0;
    (*options).force = 0;
    (*options).noreset = 0;

    *mode = MODE_UNSET;

    tmpint = 0;
    option_index = 0;

    while ((tmpint = getopt_long(argc, argv, "bc::C:df::F:hil:rs::t:kKvVw",
                long_options, &option_index)) != EOF) {
        switch (tmpint) {
        case 0:
            /* Long-only options go here */
            if (!strcmp(long_options[option_index].name, "force")) {
                (*options).force = 1;
                break;
            }
            break;
        case 'b':
            (*options).binary = 1;
            break;
        case 'c':
            set_mode(mode, MODE_DUMP_CONFIG);
            if (optarg != NULL) {
                *file_name = optarg;
            }
            break;
        case 'C':
            if (optarg == NULL) {
                fprintf(stderr, "Missing config file to read from.\n");
                exit(1);
            }
            set_mode(mode, MODE_WRITE_CONFIG);
            *file_name = optarg;
            break;
        case 'd':
            (*options).direct = 1;
            break;
        case 'f':
            set_mode(mode, MODE_DUMP_FIRMWARE);
            if (optarg != NULL)
                *file_name = optarg;
            break;
        case 'F':
            if (optarg == NULL) {
                fprintf(stderr, "Missing firmware file to read from.\n");
                exit(1);
            }
            set_mode(mode, MODE_WRITE_FIRMWARE);
            *file_name = optarg;
            break;
        case 'h':
            set_mode(mode, MODE_HELP);
            break;
        case 'i':
            set_mode(mode, MODE_PRINT_INFO);
            break;
        case 'l':
            if (optarg == NULL) {
                fprintf(stderr, "Missing config file to read from.\n");
                exit(1);
            }
            set_mode(mode, MODE_LEARN_IR);
            *file_name = optarg;
            break;
        case 'r':
            set_mode(mode, MODE_RESET);
            break;
        case 'R':
            (*options).noreset = 1;
            break;
        case 's':
            set_mode(mode, MODE_DUMP_SAFEMODE);
            if (optarg != NULL)
                *file_name = optarg;
            break;
        case 't':
            if (optarg == NULL) {
                fprintf(stderr, "Missing connectivity file to read from.\n");
                exit(1);
            }
            set_mode(mode, MODE_CONNECTIVITY);
            *file_name = optarg;
            break;
        case 'k':
            set_mode(mode, MODE_GET_TIME);
            break;
        case 'K':
            set_mode(mode, MODE_SET_TIME);
            break;
        case 'v':
            (*options).verbose = 1;
            break;
        case 'V':
            set_mode(mode, MODE_VERSION);
            break;
        case 'w':
            (*options).noweb = 1;
            break;
        default:
            exit(1);
        }
    }

    if (optind + 1 < argc) {
        fprintf(stderr, "Multiple arguments were");
        fprintf(stderr, " specified after the options");
        fprintf(stderr, " - not sure what to do,");
        fprintf(stderr, " exiting\n");
        exit(1);
    } else if (optind < argc) {
        /* one thing left over, lets hope its a filename */
        *file_name = argv[optind];
    }

}

void help()
{
    printf("There are two ways to invoke this software. The primary");
    printf(" way is \"automatic\nmode\" where you just pass in a file");
    printf(" and the software figures out what to\ndo for you:\n");
    printf("\tconcordance <file>\n\n");
    printf("Or you can specify what to do:\n");
    printf("\tconcordance <options>\n\n");

    printf("When specifying options, you must first choose a mode:\n\n");

    printf("   -c, --dump-config [<filename>]\n");
    printf("\tRead the config from the remote and write it to a file.");
    printf("\n\tIf no filename is specified, config.EZHex is used.\n\n");
    printf("   -C, --write-config <filename>\n");
    printf("\tRead a config from <filename> and write it to the");
    printf(" remote.\n\n");
    printf("   -f, --dump-firmware [<filename>]\n");
    printf("\tRead firmware from the remote and write it to a file.\n");
    printf("\tIf no filename is specified firmware.EZUp is used.\n\n");
    printf("   -F, --write-firmware <filename>\n");
    printf("\tRead firmware from <filename> and write it to the");
    printf(" remote\n\n");
    printf("   -h, --help\n");
    printf("\tPrint this help message and exit.\n\n");
    printf("   -i, --print-remote-info\n");
    printf("\tPrint information about the remote. Additional");
    printf(" information will\n\tbe printed if -v is also used.\n\n");
    printf("   -k, --get-time\n");
    printf("\t Get time from the remote\n\n");
    printf("   -K, --set-time\n");
    printf("\t Set the remote's time clock\n\n");
    printf("   -l, --learn-ir <filename>\n");
    printf("\t Learn IR from other remotes. Use <filename>.\n\n");
    printf("   -r, --reset\n");
    printf("\tReset (power-cycle) the remote control\n\n");
    printf("   -s, --dump-safemode [<filename>]\n");
    printf("\tRead the safemode firmware from the remote and write it");
    printf(" to a file.\n\tIf no filename is psecified, safe.bin is");
    printf(" used.\n\n");
    printf("   -t, --connectivity-test <filename>\n");
    printf("\tDo a connectivity test using <filename>\n\n");
    printf("   -V, --version\n");
    printf("\t Print the version and exit\n\n");

    printf("NOTE: If using the short options (e.g. -c), then *optional*");
    printf(" arguments\nmust be adjacent: -c/path/to/file.");
    printf(" Required arguments don't have this\nlimitation, and can");
    printf(" be specified -C /path/to/file or -C/path/to/file.\n\n");

    printf("Additionally, you can specify options to adjust the behavior");
    printf(" of the software:\n\n");

    printf("  -b, --binary-only\n");
    printf("\tWhen dumping a config or firmware, this specifies to dump");
    printf(" only the\n\tbinary portion. When use without a specific");
    printf(" filename, the default\n\tfilename's extension is changed to");
    printf(" .bin.\n");
    printf("\tWhen writing a config or firmware, this specifies the");
    printf(" filename\n\tpassed in has just the binary blob, not the");
    printf(" XML.\n\n");

    printf("   --force\n");
    printf("\tForce. This forces concordance to use the file the way\n");
    printf("\tyou specified, even if it doesn't think that's the kind\n");
    printf("\tof file it is. This is necessary for files dumped by\n");
    printf("\tconcordance.\n\n");

    printf("  -R, --no-reset\n");
    printf("\tFor config or firmware updates, do not reboot the device");
    printf(" when done.\n\tThis is generally only for debugging.\n\n");

    printf("  -v, --verbose\n");
    printf("\tEnable verbose output.\n\n");

    printf("  -w, --no-web\n");
    printf("\tDo not attempt to talk to the website. This is useful");
    printf(" for\n\tre-programming the remote from a saved file, or for");
    printf(" debugging.\n\n");

}


/*
 * Begin functions to actually do work
 */
int learn_ir_commands(struct options_t *options, lc_callback cb, void *cb_arg)
{
    int err = 0;
    uint32_t carrier_clock = 0;
    uint32_t *ir_signal = NULL;
    uint32_t ir_signal_length = 0;
    uint32_t index = 0;
    char **key_names;
    uint32_t key_names_length = 0;
    char *post_string = NULL;
    char user_cmd;

    err = get_key_names(&key_names, &key_names_length);
    if ((err != 0) || (key_names_length == 0))
        return err;
    
    printf("Received file contains %u key names to be learned.\n",
        key_names_length);
    
    while (1) {
        if (index >= key_names_length) {
            index--;
            printf("Last key in list!\n");
        }
        printf("\nKey name : <%s> : \n", key_names[index]);
        user_cmd = get_cmd("[L]earn, [N]ext, [P]revious, [Q]uit", "LlHhNnPpQq",
                           'L');
        err = -1; 
        /* have no code yet */
        switch (user_cmd) {
            case 'L':
            case 'l':
                /* learn from remote: */
                printf("press corresponding key ");
                printf("on original remote within 5 sec:\n");
                err = learn_from_remote(&carrier_clock,
                    &ir_signal, &ir_signal_length,
                    cb_print_percent_status, NULL);
                break;
            case 'P':
            case 'p':
                if (index > 0) {
                    index--;
                } else {
                    printf("First key in list!\n");
                }
                break;
            case 'N':
            case 'n':
                index++;
                break;
            default:
                break;
        }                
        if ( err == 0 ) {
            /* have new IR code: */
            if ((*options).verbose) {
                print_received_ir_signal(carrier_clock,
                    ir_signal, ir_signal_length, options);
            }
            err = encode_for_posting(carrier_clock, ir_signal,
                    ir_signal_length, &post_string);
            /* done with learned signal, free memory: */
            delete_ir_signal(ir_signal);
        }
            
        if ( err == 0 ) {
            /* have successfully encoded new code: */
#ifdef _DEBUG                
            if ((*options).verbose) {
                printf("%s\n\n", post_string );
            }
#endif
            if (!(*options).noweb) {
                user_cmd = get_cmd(
                    "[U]pload new code, [R]etry same key, [N]ext key, [Q]uit",
                    "UuRrNnQq", 'U');
            } else {
                /* no upload: just skip to next key */
                user_cmd = 'N';
            }
            switch (user_cmd) {
                case 'U':
                case 'u':
                    /*printf("Upload to website:   ");
                    cb_print_percent_status(
                        0, 0, 0, 1,
                        LC_CB_COUNTER_TYPE_STEPS, NULL);
                    */
                    err = post_new_code(key_names[index],
                        post_string, cb, cb_arg);
                    if ( err == 0 ) {
                    /*
                        cb_print_percent_status(
                            0, 1, 1, 1,
                            LC_CB_COUNTER_TYPE_STEPS,
                            NULL);
                        printf("       done\n");*/
                        index++;
                    } else {
                        printf("       failed\n");
                    }
                    break;
                case 'N':
                case 'n':
                    index++;
                    break;
                default:
                    break;
            }
            /* done, free memory */
            delete_encoded_signal(post_string);
        } else {
            if (err > 0) {
                fprintf(stderr, "\nERROR:Problemreceiving IR");
                fprintf(stderr, " signal:\n\t%s \n",
                    lc_strerror(err));
            }
        }
        
        if (user_cmd == 'Q' || user_cmd == 'q') {
            break;
        }
    }
    /* done, free memory */
    delete_key_names(key_names, key_names_length);
    return 0;
}

int dump_config(struct options_t *options, char *file_name, lc_callback cb,
                void *cb_arg)
{
    int err = 0;
    uint8_t *config;
    uint32_t size = 0;

    if ((err = read_config_from_remote(&config, &size, cb, NULL))) {
        return err;
    }

    if ((err = write_config_to_file(config, size, file_name,
                                    (*options).binary))) {
        return err;
    }

    delete_blob(config);

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

    printf("%04i/%02i/%02i %s %02i:%02i:%02i %+i %s\n", get_time_year(),
           get_time_month(), get_time_day(), dow[get_time_dow() & 7],
           get_time_hour(), get_time_minute(), get_time_second(),
           get_time_utc_offset(), get_time_timezone());
}

int upload_config(struct options_t *options, lc_callback cb, void *cb_arg)
{
    int err;

    /*
     * Tell the website we're going to start. This, it seems creates a
     * session object on their side, because if you miss the pre-config
     * communication, you get a no-session error.
     */
    if (!(*options).binary && !(*options).noweb) {
        if ((err = post_preconfig(cb, cb_arg)))
            return err;
    }

    if ((err = update_configuration(cb, cb_arg, (*options).noreset))) {
        return err;
    }

    /* Tell the website we're done */
    if (!(*options).binary && !(*options).noweb) {
        if ((err = post_postconfig(cb, cb_arg))) {
            return err;
        }
    }

    return 0;
}

int dump_safemode(char *file_name, lc_callback cb, void *cb_arg)
{
    uint8_t * safe = 0;
    uint32_t safe_size;
    int err = 0;

    if ((err = read_safemode_from_remote(&safe, &safe_size, cb,
            cb_arg))) {
        delete_blob(safe);
        return err;
    }

    if ((err = write_safemode_to_file(safe, safe_size, file_name))) {
        delete_blob(safe);
        return err;
    }

    delete_blob(safe);
    return 0;
}

int upload_firmware(struct options_t *options, lc_callback cb, void *cb_arg)
{
    int err;

    if ((err = is_fw_update_supported((*options).direct))) {
        fprintf(stderr, "Sorry, firmware upgrades are not yet");
        fprintf(stderr, " on your remote model yet.\n");
        return err;
    }

    if ((*options).direct) {
        direct_warning();
    } else {
        if (is_config_safe_after_fw() != 0) {
            printf("NOTE: A firmware upgrade, will erase your");
            printf(" remote's config and you will need to update");
            printf(" it. You may want to make a backup with -c");
            printf(" or otherwise just use the website.\n");
            printf("Press <enter> to continue.\n");
            getchar();
        }
    }

    if ((err = update_firmware(cb, cb_arg, (*options).noreset,
                               (*options).direct)))
        return err;

    if (!(*options).binary && !(*options).noweb) {
        if ((err = post_postfirmware(cb, cb_arg)))
            return err;
    }

    return 0;
}

int dump_firmware(struct options_t *options, char *file_name,
    lc_callback cb, void *cb_arg)
{
    int err = 0;
    uint8_t *firmware = 0;
    uint32_t firmware_size;

    if ((err = read_firmware_from_remote(&firmware, &firmware_size, cb,
                                         cb_arg))) {
        delete_blob(firmware);
        return err;
    }

    if ((err = write_firmware_to_file(firmware, firmware_size, file_name,
                                      (*options).binary))) {
        delete_blob(firmware);
        return err;
    }

    delete_blob(firmware);
    return 0;
}

int print_version_info(struct options_t *options)
{
    const char *cn;
    int used, total;

    printf("  Model: %s %s", get_mfg(), get_model());
    cn = get_codename();

    if (strcmp(cn,"")) {
        printf(" (%s)\n", cn);
    } else {
        printf("\n");
    }

    if ((*options).verbose)
        printf("  Skin: %i\n", get_skin());

    printf("  Firmware Version: %i.%i\n", get_fw_ver_maj(), get_fw_ver_min());

    if ((*options).verbose)
        printf("  Firmware Type: %i\n", get_fw_type());

    printf("  Hardware Version: %i.%i.%i\n", get_hw_ver_maj(), get_hw_ver_min(),
           get_hw_ver_mic());

    if ((*options).verbose) {
        int size = get_flash_size();
        printf("  External Flash: ");
        if (size >> 10 != 0) {
            printf("%i MiB", size >> 10);
        } else {
            printf("%i KiB", size);
        }
        printf(" - %02X:%02X %s\n", get_flash_mfg(), get_flash_id(),
               get_flash_part_num());

        printf("  Architecture: %i\n", get_arch());
        printf("  Protocol: %i\n", get_proto());

        printf("  Manufacturer: %s\n", get_hid_mfg_str());
        printf("  Product: %s\n", get_hid_prod_str());
        printf("  IRL, ORL, FRL: %i, %i, %i\n", get_hid_irl(), get_hid_orl(),
               get_hid_frl());
        printf("  USB VID: %04X\n", get_usb_vid());
        printf("  USB PID: %04X\n", get_usb_pid());
        printf("  USB Ver: %04X\n", get_usb_bcd());

       /*
        * Certain MH remotes (but not all) such as the Harmony Touch use a
        * different serial number format.  In this case, mh_get_serial() will
        * return a non-zero-length string which will have the correct serial.
        */
        if (strlen(mh_get_serial()) != 0)
            printf("  Serial Number: %s\n", mh_get_serial());
        else
            printf("  Serial Number: %s\n\t%s\n\t%s\n", get_serial(1),
                   get_serial(2), get_serial(3));
    }

    used = get_config_bytes_used();
    total = get_config_bytes_total();
    printf("  Config Flash Used: %i%% (%i of %i KiB)\n\n",
           (used*100+99) / total, (used+1023)>>10, (total+1023)>>10);

    return 0;
}

/*
 * MAIN
 */
int main(int argc, char *argv[])
{
    struct options_t options;
    char *file_name;
    int mode, file_mode, err;

    printf("Concordance %s\n", VERSION);
    printf("Copyright 2007 Kevin Timmerman and Phil Dibowitz\n");
    printf("This software is distributed under the GPLv3.\n\n");

    file_name = NULL;
    mode = MODE_UNSET;

    parse_options(&options, &mode, &file_name, argc, argv);

    /*
     * Handle the few simple modes that don't require any files
     * to be read in or any realy work to be done first...
     */
    if (mode == MODE_VERSION) {
        exit(0);
    }

    if (mode == MODE_HELP) {
        help();
        exit(0);
    }

    /*
     * We used to delay remote initialization until after all the figuring
     * out the mode and read the file stuff... but with zwave support we
     * need to know what type of remote we're dealing with early on.
     */

    err = init_concord();
    if (err != 0) {
        fprintf(stderr, "ERROR: Couldn't initializing libconcord: %s\n",
                lc_strerror(err));
        exit(1);
    }

    /*
      * Alright, at this point, if there's going to be a filename,
      * we have one, so lets read the file.
      */
    if (file_name && (mode != MODE_DUMP_CONFIG && mode != MODE_DUMP_FIRMWARE &&
        mode != MODE_DUMP_SAFEMODE)) {

        int type;
        if ((err = read_and_parse_file(file_name, &type))) {
            fprintf(stderr, "ERROR: Cannot read input file: %s\n", file_name);
            exit(1);
        }

        switch (type) {
        case LC_FILE_TYPE_CONNECTIVITY:
            file_mode = MODE_CONNECTIVITY;
            break;
        case LC_FILE_TYPE_CONFIGURATION:
            file_mode = MODE_WRITE_CONFIG;
            break;
        case LC_FILE_TYPE_FIRMWARE:
            file_mode = MODE_WRITE_FIRMWARE;
            break;
        case LC_FILE_TYPE_LEARN_IR:
            file_mode = MODE_LEARN_IR;
            break;
        default:
            fprintf(stderr, "ERROR: Couldn't determine filetype");
            exit(1);
            break;
        }

        /*
         * If we don't have a mode, lets detect that mode based on
         * the file.
         */
        if (mode == MODE_UNSET) {
            mode = file_mode;
        } else if (mode != file_mode) {
            report_mode_mismatch(mode, file_mode, options.force);
            if (!options.force) {
                exit(1);
            }
        }
    }

    /*
     * We have checked the parameters and the input file:
     * If we still don't know what to do, failure is the only option:
     */
    if (mode == MODE_UNSET) {
        fprintf(stderr, "ERROR: No mode requested. No work to do.\n");
        exit(1);
    }

    /*
     * The is..supported() functions return 0 for yes, so 1 is NO
     */
    if ((mode == MODE_DUMP_CONFIG && is_config_dump_supported()) ||
        ((mode == MODE_DUMP_FIRMWARE || mode == MODE_DUMP_SAFEMODE) &&
         is_fw_dump_supported())) {
        fprintf(stderr, "Sorry, that mode is not yet supported on your"
            " remote by libconcord.\n");
        exit(1);
    }

    /*
     * In a few special cases, we populate a default filename. NEVER
     * if we're are writing to the device, however.
     *
     * But wait - we already read in the file! Indeed, if we have a default
     * filename, then we'll be writing to the file, not reading it, which
     * is why we do this down here.
     */
    if (!file_name)  {
        populate_default_filename(mode, options.binary, &file_name);
    }


    /*
     * If we're in reset mode, not only do we not need to read and print
     * data from the device, but it may not even be fully functional, so
     * we probably want to do as little as possible. This we do this
     * near the beginning instead of the SWITCH below.
     *
     * Note that we don't look at or save the return value of reset_remote()
     * because the usb write for reset doesn't always return successful on
     * all remotes even though it works.
     */
    if (mode == MODE_RESET) {
        printf("Resetting...\n");
        reset_remote(NULL, NULL);
        goto cleanup;
    }

    /*
     * Get and print all the version info
     */

    err = get_identity(cb_print_percent_status, NULL);
    if (err != 0 && err != LC_ERROR_INVALID_CONFIG) {
        fprintf(stderr, "ERROR: failed to requesting identity\n");
        goto cleanup;
    }
    if (err == LC_ERROR_INVALID_CONFIG) {
        printf("WARNING: Invalid config found.\n");
    }

    /*
     * Now do whatever we've been asked to do
     */

    switch (mode) {
        case MODE_PRINT_INFO:
            err = print_version_info(&options);
            break;

        case MODE_CONNECTIVITY:
            if (!options.noweb) {
                err = post_connect_test_success(cb_print_percent_status, NULL);
            }
            break;

        case MODE_DUMP_CONFIG:
            err = dump_config(&options, file_name, cb_print_percent_status,
                              NULL);
            if (err != 0) {
                printf("Failed to dump config: %s\n", lc_strerror(err));
            }
            break;

        case MODE_WRITE_CONFIG:
            err = upload_config(&options, cb_print_percent_status, NULL);
            if (err != 0) {
                printf("Failed to upload config: %s\n", lc_strerror(err));
            }
            break;

        case MODE_DUMP_FIRMWARE:
            err = dump_firmware(&options, file_name, cb_print_percent_status,
                                NULL);
            if (err != 0) {
                printf("Failed to dump firmware: %s\n", lc_strerror(err));
            }
            break;

        case MODE_WRITE_FIRMWARE:
            err = upload_firmware(&options, cb_print_percent_status, NULL);
            if (err != 0) {
                printf("Failed to upload firmware: %s\n", lc_strerror(err));
            }
            break;

        case MODE_DUMP_SAFEMODE:
            err = dump_safemode(file_name, cb_print_percent_status, NULL);
            if (err != 0) {
                printf("Failed to dump safemode: %s\n", lc_strerror(err));
            }
            break;

        case MODE_LEARN_IR:
            err = learn_ir_commands(&options, cb_print_percent_status, NULL);
            break;

        case MODE_GET_TIME:
            err = get_time();
            print_time(0);
            break;

        case MODE_SET_TIME:
            err = set_time(cb_print_percent_status, NULL);
            print_time(1);
            break;

        default:
            fprintf(stderr,
                "ERROR: Got to a place I don't understand!\n");
            break;
    }
            
cleanup:

    delete_opfile_obj();

    deinit_concord();

    if (err) {
        printf("Failed with error %i\n", err);
    } else {
        printf("Success!\n");
    }
        
#ifdef _WIN32
    /* Shutdown WinSock */
    WSACleanup();
#endif

#ifdef _DEBUG
    printf("Press <enter> key to exit\n");
    getchar();
#endif /* debug */

    return err;
}
