#
# vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=8 noexpandtab:
#
# $Id$
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#


from ctypes import *
import platform
import sys

# Internal DLL handle
if platform.system() == 'Windows':
    _dll = cdll.LoadLibrary('libconcord.dll')
else:
    _dll = cdll.LoadLibrary('libconcord.so.0')

# This exception may be raised by any function that returns an LC_ERROR_*.
class LibConcordException(Exception):
    def __init__(self, func, result):
        self.func = func
        self.result = result
        try:
            self.result_str = c_char_p(_dll.lc_strerror(c_int(self.result))).value
        except:
            self.result_str = 'Unknown'

    def __str__(self):
        return 'libconcord function %s failed with error code %s (%s)' % (
            repr(self.func),
            repr(self.result),
            repr(self.result_str)
        )

# Internal function result checking functionality
class _CheckRetCode(object):
    def __init__(self, func_name):
        self.func_name = func_name

    def __call__(self, *args):
        result = args[0]
        if result != 0:
            raise LibConcordException(self.func_name, result)
        
# Internal ctypes function wrapper creation
def _create_func(
    func_name,
    error_checker,
    *args
):
    ftype = CFUNCTYPE(*args)
    f = ftype((func_name, _dll))
    if error_checker:
        f.errcheck = error_checker(func_name)
    return f

# typedef void (*lc_callback)(uint32_t, uint32_t, uint32_t, void*);
callback_type = CFUNCTYPE(None, c_uint, c_uint, c_uint, py_object)

# const char *get_mfg();
get_mfg = _create_func(
    'get_mfg',
    None,
    c_char_p
)

# const char *get_model();
get_model = _create_func(
    'get_model',
    None,
    c_char_p
)

# const char *get_codename();
get_codename = _create_func(
    'get_codename',
    None,
    c_char_p
)

# int get_skin();
get_skin = _create_func(
    'get_skin',
    None,
    c_int
)

# int get_fw_ver_maj();
get_fw_ver_maj = _create_func(
    'get_fw_ver_maj',
    None,
    c_int
)

# int get_fw_ver_min();
get_fw_ver_min = _create_func(
    'get_fw_ver_min',
    None,
    c_int
)

# int get_fw_type();
get_fw_type = _create_func(
    'get_fw_type',
    None,
    c_int
)

# int get_hw_ver_maj();
get_hw_ver_maj = _create_func(
    'get_hw_ver_maj',
    None,
    c_int
)

# int get_hw_ver_min();
get_hw_ver_min = _create_func(
    'get_hw_ver_min',
    None,
    c_int
)

# int get_flash_size();
get_flash_size = _create_func(
    'get_flash_size',
    None,
    c_int
)

# int get_flash_mfg();
get_flash_mfg = _create_func(
    'get_flash_mfg',
    None,
    c_int
)

# int get_flash_id();
get_flash_id = _create_func(
    'get_flash_id',
    None,
    c_int
)

# const char *get_flash_part_num();
get_flash_part_num = _create_func(
    'get_flash_part_num',
    None,
    c_char_p
)

# int get_arch();
get_arch = _create_func(
    'get_arch',
    None,
    c_int
)

# int get_proto();
get_proto = _create_func(
    'get_proto',
    None,
    c_int
)

# const char *get_hid_mfg_str();
get_hid_mfg_str = _create_func(
    'get_hid_mfg_str',
    None,
    c_char_p
)

# const char *get_hid_prod_str();
get_hid_prod_str = _create_func(
    'get_hid_prod_str',
    None,
    c_int
)

# int get_hid_irl();
get_hid_irl = _create_func(
    'get_hid_irl',
    None,
    c_int
)

# int get_hid_orl();
get_hid_orl = _create_func(
    'get_hid_orl',
    None,
    c_int
)

# int get_hid_frl();
get_hid_frl = _create_func(
    'get_hid_frl',
    None,
    c_int
)

# int get_usb_vid();
get_usb_vid = _create_func(
    'get_usb_vid',
    None,
    c_int
)

# int get_usb_pid();
get_usb_pid = _create_func(
    'get_usb_pid',
    None,
    c_int
)

# int get_usb_bcd();
get_usb_bcd = _create_func(
    'get_usb_bcd',
    None,
    c_int
)

SERIAL_COMPONENT_1 = 1
SERIAL_COMPONENT_2 = 2
SERIAL_COMPONENT_3 = 3

# char *get_serial(int p);
get_serial = _create_func(
    'get_serial',
    None,
    c_char_p,
    c_int
)

# int get_config_bytes_used();
get_config_bytes_used = _create_func(
    'get_config_bytes_used',
    None,
    c_int
)

# int get_config_bytes_total();
get_config_bytes_total = _create_func(
    'get_config_bytes_total',
    None,
    c_int
)

# int get_time_second();
get_time_second = _create_func(
    'get_time_second',
    None,
    c_int
)

# int get_time_minute();
get_time_minute = _create_func(
    'get_time_minute',
    None,
    c_int
)

# int get_time_hour();
get_time_hour = _create_func(
    'get_time_hour',
    None,
    c_int
)

# int get_time_day();
get_time_day = _create_func(
    'get_time_day',
    None,
    c_int
)

# int get_time_dow();
get_time_dow = _create_func(
    'get_time_dow',
    None,
    c_int
)

# int get_time_month();
get_time_month = _create_func(
    'get_time_month',
    None,
    c_int
)

# int get_time_year();
get_time_year = _create_func(
    'get_time_year',
    None,
    c_int
)

# int get_time_utc_offset();
get_time_utc_offset = _create_func(
    'get_time_utc_offset',
    None,
    c_int
)

# const char *get_time_timezone();
get_time_timezone = _create_func(
    'get_time_timezone',
    None,
    c_char_p
)

# int delete_blob(uint8_t *ptr);
delete_block = _create_func(
    'delete_blob',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte)
);

LC_FILE_TYPE_CONNECTIVITY  = 0
LC_FILE_TYPE_CONFIGURATION = 1
LC_FILE_TYPE_FIRMWARE      = 2
LC_FILE_TYPE_LEARN_IR      = 3

# int identify_file(uint8_t *in, uint32_t size, int *type);
identify_file = _create_func(
    'identify_file',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    POINTER(c_int)
)

# int init_concord();
init_concord = _create_func(
    'init_concord',
    _CheckRetCode,
    c_int,
)

# int deinit_concord();
deinit_concord = _create_func(
    'deinit_concord',
    _CheckRetCode,
    c_int,
)

# int get_identity(lc_callback cb, void *cb_arg);
get_identity = _create_func(
    'get_identity',
    _CheckRetCode,
    c_int,
    callback_type,
    py_object
)

# int reset_remote();
reset_remote = _create_func(
    'reset_remote',
    _CheckRetCode,
    c_int,
)

# int get_time();
get_time = _create_func(
    'get_time',
    _CheckRetCode,
    c_int,
)

# int set_time();
set_time = _create_func(
    'set_time',
    _CheckRetCode,
    c_int,
)

# int post_connect_test_success(uint8_t *data, uint32_t size);
post_connect_test_success = _create_func(
    'post_connect_test_success',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint
)

# int post_preconfig(uint8_t *data, uint32_t size);
post_preconfig = _create_func(
    'post_preconfig',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint
)

# int post_postconfig(uint8_t *data, uint32_t size);
post_postconfig = _create_func(
    'post_postconfig',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint
)

# int post_postfirmware(uint8_t *data, uint32_t size);
post_postfirmware = _create_func(
    'post_postfirmware',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint
)

# int invalidate_flash();
invalidate_flash = _create_func(
    'invalidate_flash',
    _CheckRetCode,
    c_int,
)

# int read_config_from_remote(uint8_t **out, uint32_t *size,
#	lc_callback cb, void *cb_arg);
read_config_from_remote = _create_func(
    'read_config_from_remote',
    _CheckRetCode,
    c_int,
    POINTER(POINTER(c_ubyte)),
    POINTER(c_uint),
    callback_type,
    py_object
)

# int write_config_to_remote(uint8_t *in, uint32_t size,
#     lc_callback cb, void *cb_arg);
write_config_to_remote = _create_func(
    'write_config_to_remote',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    callback_type,
    py_object
)

# int read_file(char *file_name, uint8_t **out, uint32_t *size);
read_file = _create_func(
    'read_file',
    _CheckRetCode,
    c_int,
    c_char_p,
    POINTER(POINTER(c_ubyte)),
    POINTER(c_uint)
)

# int write_config_to_file(uint8_t *in, uint32_t size, char *file_name,
#     int binary);
write_config_to_file = _create_func(
    'write_config_to_file',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    c_char_p,
    c_int
)

# int verify_remote_config(uint8_t *in, uint32_t size, lc_callback cb,
#     void *cb_arg);
verify_remote_config = _create_func(
    'verify_remote_config',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    callback_type,
    py_object
)

# int erase_config(uint32_t size, lc_callback cb, void *cb_arg);
erase_config = _create_func(
    'erase_config',
    _CheckRetCode,
    c_int,
    c_uint,
    callback_type,
    py_object
)

# int find_config_binary(uint8_t *config, uint32_t config_size,
#     uint8_t **binary_ptr, uint32_t *binary_size);
find_config_binary = _create_func(
    'find_config_binary',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    POINTER(POINTER(c_ubyte)),
    POINTER(c_uint)
)

# int erase_safemode(lc_callback cb, void *cb_arg);
erase_safemode = _create_func(
    'erase_safemode',
    _CheckRetCode,
    c_int,
    callback_type,
    py_object
)

# int read_safemode_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
#     void *cb_arg);
read_safemode_from_remote = _create_func(
    'read_safemode_from_remote',
    _CheckRetCode,
    c_int,
    POINTER(POINTER(c_ubyte)),
    POINTER(c_ubyte),
    callback_type,
    py_object
)

# int write_safemode_to_file(uint8_t *in, uint32_t size,char *file_name);
write_safemode_to_file = _create_func(
    'write_safemode_to_file',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    c_char_p
)

# int is_fw_update_supported(int direct);
is_fw_update_supported = _create_func(
    'is_fw_update_supported',
    None,
    c_int,
    c_int
)

# int is_config_safe_after_fw();
is_config_safe_after_fw = _create_func(
    'is_config_safe_after_fw',
    None,
    c_int
)

# int prep_firmware();
prep_firmware = _create_func(
    'prep_firmware',
    _CheckRetCode,
    c_int
)

# int finish_firmware();
finish_firmware = _create_func(
    'finish_firmware',
    _CheckRetCode,
    c_int
)

# int erase_firmware(int direct, lc_callback cb, void *cb_arg);
erase_firmware = _create_func(
    'erase_firmware',
    _CheckRetCode,
    c_int,
    c_int,
    callback_type,
    py_object
)

# int read_firmware_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
#     void *cb_arg);
read_firmware_from_remote = _create_func(
    'read_firmware_from_remote',
    _CheckRetCode,
    c_int,
    POINTER(POINTER(c_ubyte)),
    POINTER(c_uint),
    callback_type,
    py_object
)

# int write_firmware_to_remote(uint8_t *in, uint32_t size, int direct,
#     lc_callback cb, void *cb_arg);
write_firmware_to_remote = _create_func(
    'write_firmware_to_remote',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    c_int,
    callback_type,
    py_object
)

# int write_firmware_to_file(uint8_t *in, uint32_t size, char *file_name,
#     int binary);
write_firmware_to_file = _create_func(
    'write_firmware_to_file',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    c_char_p,
    c_int
)

# int extract_firmware_binary(uint8_t *xml, uint32_t xml_size, uint8_t **out,
#     uint32_t *size);
extract_firmware_binary = _create_func(
    'extract_firmware_binary',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    POINTER(POINTER(c_ubyte)),
    POINTER(c_uint)
)

# int learn_ir_commands(uint8_t *data, uint32_t size, int post);
learn_ir_commands = _create_func(
    'learn_ir_commands',
    _CheckRetCode,
    c_int,
    POINTER(c_ubyte),
    c_uint,
    c_int
)

