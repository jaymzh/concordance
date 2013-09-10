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
 * (C) Copyright Phil Dibowitz 2007
 * (C) Copyright Kevin Timmerman 2007
 */

#ifndef LIBCONCORD_H
#define LIBCONCORD_H

#include <stdio.h>

#ifdef WIN32
typedef unsigned char uint8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#define sleep(x) Sleep((x) * 1000)
#define snprintf _snprintf

#else

#include <stdint.h>

#endif /* end if win32/else */

#define LC_ERROR 1
#define LC_ERROR_INVALID_DATA_FROM_REMOTE 2
#define LC_ERROR_READ 3
#define LC_ERROR_WRITE 4
#define LC_ERROR_INVALIDATE 5
#define LC_ERROR_ERASE 6
#define LC_ERROR_VERIFY 7
#define LC_ERROR_POST 8
#define LC_ERROR_GET_TIME 9
#define LC_ERROR_SET_TIME 10
#define LC_ERROR_CONNECT 11
#define LC_ERROR_OS 12
#define LC_ERROR_OS_NET 13
#define LC_ERROR_OS_FILE 14
#define LC_ERROR_UNSUPP 15
#define LC_ERROR_INVALID_CONFIG 16
#define LC_ERROR_IR_OVERFLOW 17

/*
 * Filetypes, used by identity_file()
 */
#define LC_FILE_TYPE_CONNECTIVITY 1
#define LC_FILE_TYPE_CONFIGURATION 2
#define LC_FILE_TYPE_FIRMWARE 3
#define LC_FILE_TYPE_LEARN_IR 4
/*
 * Callback counter types
 */
#define LC_CB_COUNTER_TYPE_STEPS 5
#define LC_CB_COUNTER_TYPE_BYTES 6
/*
 * Callback stages
 */
#define LC_CB_STAGE_NUM_STAGES 0xFF
#define LC_CB_STAGE_GET_IDENTITY 7
/* for config updates... */
#define LC_CB_STAGE_INITIALIZE_UPDATE 8
#define LC_CB_STAGE_INVALIDATE_FLASH 9
#define LC_CB_STAGE_ERASE_FLASH 10
#define LC_CB_STAGE_WRITE_CONFIG 11
#define LC_CB_STAGE_VERIFY_CONFIG 12
#define LC_CB_STAGE_FINALIZE_UPDATE 13
#define LC_CB_STAGE_READ_CONFIG 14
/* firmware updates share most of the above, but need */
#define LC_CB_STAGE_WRITE_FIRMWARE 15
#define LC_CB_STAGE_READ_FIRMWARE 16
#define LC_CB_STAGE_READ_SAFEMODE 17
/* other... */
#define LC_CB_STAGE_RESET 18
#define LC_CB_STAGE_SET_TIME 19
#define LC_CB_STAGE_HTTP 20
#define LC_CB_STAGE_LEARN 21


/*
 * Actual C clients are not fully supported yet, but that's the goal...
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * CALLBACK INFORMATION
 *
 * There is currently only one kind of callback, and it's for status
 * information. It should be a void function and takes the following
 * arguments:
 *   uint32_t stage_id - the id of the stage
 *   uint32_t count - the amount of times this cb has been called in a
 *                    given call of a given functioin
 *   uint32_t curr  - current status (usually bytes read/written)
 *   uint32_t total - total goal status (usually bytes expected to read/write)
 *   uint32_t counter_type - the type of counter (bytes, steps, etc.)
 *   void *arg      - opaque object you can pass to functions to have them
 *                    pass back to your callback.
 *   const uint32_t* stages - a pointer to the stages that will be
 *                            performed for this operation.  Only used when
 *                            LC_CB_STAGE_NUM_STAGES is the callback stage.
 */
typedef void (*lc_callback)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
    void*, const uint32_t*);

/*
 * REMOTE INFORMATION ACCESSORS
 *
 * These take nothing, and return what they say. Simple stuff.
 */
const char *get_mfg();
const char *get_model();
const char *get_codename();
int get_skin();
int get_fw_ver_maj();
int get_fw_ver_min();
int get_fw_type();
int get_hw_ver_maj();
int get_hw_ver_min();
int get_hw_ver_mic();
int get_flash_size();
int get_flash_mfg();
int get_flash_id();
const char *get_flash_part_num();
int get_arch();
int get_proto();
const char *get_hid_mfg_str();
const char *get_hid_prod_str();
int get_hid_irl();
int get_hid_orl();
int get_hid_frl();
int get_usb_vid();
int get_usb_pid();
int get_usb_bcd();
char *get_serial(int p);
int get_config_bytes_used();
int get_config_bytes_total();

/*
 * Support helpers
 */
/*
 * We don't yet support all things on all remotes, so these try to help you
 * figure out if something is supported.
 * NOTE WELL: If they are a remote we don't know about, the results are
 * meaningless.
 *
 * This will return 0 for yes and LC_ERROR_UNSUPP otherwise.
 */
int is_config_dump_supported();
int is_config_update_supported();
int is_fw_dump_supported();
int is_fw_update_supported(int direct);

/*
 * TIME ACCESSORS
 *
 * These can ONLY be called *after* get_time() or set_time(). Each call will
 * initialize the internal time structures to the time used, and these
 * accessors can be used to access that data.
 */
int get_time_second();
int get_time_minute();
int get_time_hour();
int get_time_day();
int get_time_dow();
int get_time_month();
int get_time_year();
int get_time_utc_offset();
const char *get_time_timezone();

/*
 * HELPER FUNCTIONS
 */

/*
 * Translate a return value into an actual error message. Pass in the int
 * you received, get back a string.
 */
const char *lc_strerror(int err);
const char *lc_cb_stage_str(int stage);
/*
 * Many functions require you to pass in a ptr which then gets pointed
 * to data that we allocate. You should then call this to clean that
 * data up when you are done with it.
 */
void delete_blob(uint8_t *ptr);

/*
 * Read an operations file from the website, parse it, and return a mode
 * of operations.
 */
int read_and_parse_file(char *filename, int *type);
/*
 * Free the memory used by the file as allocated in read_and_parse_file.
*/
void delete_opfile_obj();

/*
 * GENERAL REMOTE INTERACTIONS
 */

/*
 * Initialize USB (and WinSock if necessary) and find the remote
 * if possible.
 */
int init_concord();
/*
 * Release the USB device, and tear down anything else necessary.
 */
int deinit_concord();
/*
 * This is another initialization function. Generally speaking you always
 * want to call this before you do anything. It will query the remote about
 * it's data and fill in internal data structures with that information. This
 * counts as a successful "connectivity test"
 */
int get_identity(lc_callback cb, void *cb_arg);
/*
 * Reboot the remote.
 */
int reset_remote(lc_callback cb, void *cb_arg);
/*
 * Get the time from the remote. Use the time accessors above to access
 * the data.
 */
int get_time();
/*
 * Set the time on the remote to the system time. To find out what time was
 * used, use the time accessors above.
 */
int set_time(lc_callback cb, void *cb_arg);
/*
 * POST to the members.harmonyremote.com website that the connection test was
 * successful. A Connectivity.EZHex file must be passed in so that we
 * can get the URL, cookie information, etc.
 */
int post_connect_test_success(lc_callback cb, void *cb_arg);
/*
 * Prior to updating the config, if you want to interact with the website
 * you have to send it some initial data. This does that. The data passed
 * in here is a pointer to the config data config block (with XML - this
 * should NOT be the pointer result from find_binary_start().
 */
int post_preconfig(lc_callback cb, void *cb_arg);
/*
 * After writing the config to the remote, this should be called to tell
 * the members.harmonyremote.com website that it was successful.
 */
int post_postconfig(lc_callback cb, void *cb_arg);
/*
 * After writing a new firmware to the remote, this should be called to tell
 * the members.harmonyremote.com website that it was successful.
 */
int post_postfirmware(lc_callback cb, void *cb_arg);
/*
 * This sends the remote a command to tell it we're about to start
 * writing to it's flash area and that it shouldn't read from it.
 * This must be used before writing a config, firmware, or anything
 * else that touches flash.
 *
 * If something goes wrong, or you change your mind after invalidating
 * flash, you should reboot the device.
 */
int invalidate_flash(lc_callback cb, void *cb_arg);

/*
 * CONFIGURATION INTERACTIONS
 */

/*
 * NOTE: This is the only function you should need to update a remote's config.
 * This wraps calls to all other calls and handles all remote-specific logic for
 * you.
 *
 * The other functions are provided mostly for backwards compatibility, and for
 * backing up remote configs. If you don't need to do that, you can skip down to
 * the next section.
 */
int update_configuration(lc_callback cb, void *cb_arg, int noreset);

/*
 * Read the config from the remote and store it into the unit8_t array
 * *out. The callback is for status information. See above for CB info.
 *
 * NOTE: The pointer *out should not point to anything useful. We will
 * allocate a char array and point your pointer at it. Use delete_blob to
 * reclaim this memory.
 */
int read_config_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
                            void *cb_arg);
/*
 * Given a config block in the byte array *in that is size big, write
 * it to the remote. This should be *just* the binary blob (see
 * find_binary_start()). CB info above.
 *
 */
int write_config_to_remote(lc_callback cb, void *cb_arg);
/*
 * Given a binary-only config blob *in, write the config to a file. Unless
 * binary is true, the XML will be constructed and written to the file
 * as well.
 */
int write_config_to_file(uint8_t *in, uint32_t size, char *file_name,
                         int binary);
/*
 * After doing a write_config_to_remote(), this should be called to verify
 * that config. The data will be compared to what's in *in.
 */
int verify_remote_config(lc_callback cb, void *cb_arg);
/*
 * Preps the remote for a config upgrade.
 *
 * Note that this and finish_config are NO-OPs for most remotes, and even on
 * remotes where it is implemented, testing implies that it's not necessary.
 * However, calling these functions is necessary to completely match the
 * original Windows software, and future remotes may require these functions
 * to be executed to operate correctly.
 */
int prep_config(lc_callback cb, void *cb_arg);
/*
 * Tells the remote the config upgrade was successful and that it should
 * use the new config upon next reboot.
 */
int finish_config(lc_callback cb, void *cb_arg);
/*
 * Flash can be changed to 0, but not back to 1, so you must erase the
 * flash (to 1) in order to write the flash.
 */
int erase_config(lc_callback cb, void *cb_arg);

/*
 * SAFEMODE FIRMWARE INTERACTIONS
 */
/*
 * Make the safemode area of the flash all 1's so you can write
 * to it.
 */
int erase_safemode(lc_callback cb, void *cb_arg);
/*
 * Same as read_config_from_remote(), but reading the safemode firmware
 * instead.
 *
 * NOTE: The pointer *out should not point to anything useful. We will
 * allocate a char array and point your pointer at it. Use delete_blob to
 * reclaim this memory.
 */
int read_safemode_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
                              void *cb_arg);
/*
 * NOTE: You CAN NOT WRITE SAFEMODE FIRMWARE OVER USB!
 */

/*
 * Write aforementioned safemode data to a file. Note that this is always
 * written as pure binary.
 */
int write_safemode_to_file(uint8_t *in, uint32_t size, char *file_name);

/*
 * FIRMWARE INTERACTIONS
 */

/*
 * NOTE: This is the only function you should need.
 * This wraps calls to all other calls and handles all remote-specific logic for
 * you. The other functions are provided mostly for backwards compatibility.
 * You can skip down to the next section.
 */
int update_firmware(lc_callback cb, void *cb_arg, int noreset, int direct);

/*
 * This function tells you if the config will be wiped out by a live
 * firmware upgrade (some remotes use the config area in memory as a
 * staging area for the firmware). This will return 0 for yes and LC_ERROR
 * for no.
 */
int is_config_safe_after_fw();
/*
 * Preps the remote for a firmware upgrade
 */
int prep_firmware(lc_callback cb, void *cb_arg);
/*
 * Tells the remote the firmware upgrade was successful and that it should
 * copy the firmware from the "staging" area to the live area on next reboot.
 * Don't forget to reboot.
 */
int finish_firmware(lc_callback cb, void *cb_arg);
/*
 * Make the firmware area of the flash all 1's so you can write
 * to it.
 */
int erase_firmware(int direct, lc_callback cb, void *cb_arg);
/*
 * Same as read_config_from_remote(), but reading the firmware instead.
 *
 * NOTE: The pointer *out should not point to anything useful. We will
 * allocate a char array and point your pointer at it. Use delete_blob to
 * reclaim this memory.
 */
int read_firmware_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
                              void *cb_arg);
/*
 * Same as write_config_to_remote(), but with the firmware instead.
 */
int write_firmware_to_remote(int direct, lc_callback cb, void *cb_arg);
/*
 * Same as write_config_to_file(), but with firmware instead. Note
 * that unless binary is specified, the firmware is broken into chunks
 * and written in ASCII-encoded HEX in XML <DATA> blocks, the way
 * the members.harmonyremote.com website delivers it.
 */
int write_firmware_to_file(uint8_t *in, uint32_t size, char *file_name,
                           int binary);

/*
 * IR-stuff
 * ===========================
 * Data structure information:
 *
 * carrier_clock    : in Hz, usually ~36000..40000
 * ir_signal        : IR mark/space durations (alternating) in microsconds
 * ir_signal_length : total number of mark/space durations in ir_signal
 *      ir_signal will start with a mark and end with a space duration,
 *      hence ir_signal_length will always be an even number.
 * 
 * They are usually filled in by calling learn_from_remote(...),
 * to learn IR signals from an existing other remote, but may also
 * be set by the application, e.g. be derived from Pilips Pronto Hex
 * codes or RC5/NEC/... command codes (separate conversion library required).
 *
 * encoded posting format : IR code data converted to Logitech 
 *     posting string format, returned by encode_for_posting.
 *     Having the encoding separate from the posting keeps the
 *     parameter list of post_new_code() tidy and allows the
 *     application to display the encoded signal when desired.
 */

/*
 * Scan the contents of the received LearnIR.EZTut file
 * (read into *data[size]) for the key names to be learned.
 *
 * Fills key_names with the found names and key_names_length
 * with the number of found key names.
 * Returns 0 for success, or an error code in case of failure.
 *
 * Memory allocated for the strings must be freed by the caller
 * via delete_key_names() when not needed any longer.
 */
int get_key_names(char ***key_names, uint32_t *key_names_length);

void delete_key_names(char **key_names, uint32_t key_names_length);

/*
 * Fill ir_data with IR code learned from other remote
 * via Harmony IR receiver.
 *
 * Returns 0 for success, error code for failure.
 *
 * Memory allocated for ir_signal must be freed by the caller
 * via delete_ir_signal() when not needed any longer.
 */
int learn_from_remote(uint32_t *carrier_clock, uint32_t **ir_signal,
                      uint32_t *ir_signal_length, lc_callback cb, void *cb_arg);

void delete_ir_signal(uint32_t *ir_signal);

/*
 * Fill encoded_signal with IR code encoded to Logitech
 * posting string format.
 *
 * Returns 0 for success, error code in case of failure.
 *
 * Memory allocated for the string must be freed by the caller
 * via delete_post_string() when not needed any longer.
 */
int encode_for_posting(uint32_t carrier_clock, uint32_t *ir_signal,
                       uint32_t ir_signal_length, char **encoded_signal);

void delete_encoded_signal(char *encoded_signal);

/*
 * Post encoded IR-code with key_name and additional 
 * information from XML data[size] to Logitech.
 *
 * Logitech will only accept keynames already present in the
 * database or user-defined via 'Learn new Key' web page
 * for the current device.
 *
 * Returns 0 for success, error code for failure.
 */
int post_new_code(char *key_name, char *encoded_signal, lc_callback cb,
                  void *cb_arg);

/*
 * Special structures and methods for the Harmony Link
 */
#define MH_STRING_LENGTH 255 /* arbitrary */
#define MH_MAX_WIFI_NETWORKS 30 /* arbitrary */
struct mh_cfg_properties {
    char host_name[MH_STRING_LENGTH];
    char email[MH_STRING_LENGTH];
    char service_link[MH_STRING_LENGTH];
};
struct mh_wifi_config {
    char ssid[MH_STRING_LENGTH];
    char encryption[MH_STRING_LENGTH];
    char password[MH_STRING_LENGTH];
    char connect_status[MH_STRING_LENGTH];
    char error_code[MH_STRING_LENGTH];
};
struct mh_wifi_network {
    char ssid[MH_STRING_LENGTH];
    char signal_strength[MH_STRING_LENGTH];
    char channel[MH_STRING_LENGTH];
    char encryption[MH_STRING_LENGTH];
};
struct mh_wifi_networks {
    struct mh_wifi_network network[MH_MAX_WIFI_NETWORKS];
};
int mh_get_cfg_properties(struct mh_cfg_properties *properties);
int mh_set_cfg_properties(const struct mh_cfg_properties *properties);
int mh_get_wifi_networks(struct mh_wifi_networks *networks);
int mh_get_wifi_config(struct mh_wifi_config *config);
int mh_set_wifi_config(const struct mh_wifi_config *config);

#ifdef __cplusplus
}
#endif

#endif /* LIBCONCORD_H */
