#
# vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
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

from __future__ import absolute_import
from __future__ import print_function
from ctypes import *
from ctypes import _Pointer
import platform
import os
import sys
import traceback

# Debug request parsing

debug = (os.environ.get("LIBCONCORD_PY_TRACE", "0") == "1")

# Define the libconcord ABI this libconcord.py corresponds to
# Bump this when the .so file version gets bumped

ABI_VERSION = 6

# Load the DLL

if platform.system() == 'Windows':
    _dll = cdll.LoadLibrary('libconcord.dll')
elif platform.system() == 'Darwin':
    _dll = cdll.LoadLibrary('libconcord.%i.dylib' % ABI_VERSION)
else:
    _dll = cdll.LoadLibrary('libconcord.so.' + str(ABI_VERSION))

# Public libconcord API: Custom types

# typedef void (*lc_callback)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
#     void*, const uint32_t*);
callback_type = CFUNCTYPE(None, c_uint, c_uint, c_uint, c_uint, c_uint, \
                py_object, POINTER(c_uint))

# Public libconcord API: Error codes

LC_ERROR = 1
LC_ERROR_INVALID_DATA_FROM_REMOTE = 2
LC_ERROR_READ = 3
LC_ERROR_WRITE = 4
LC_ERROR_INVALIDATE = 5
LC_ERROR_ERASE = 6
LC_ERROR_VERIFY = 7
LC_ERROR_POST = 8
LC_ERROR_GET_TIME = 9
LC_ERROR_SET_TIME = 10
LC_ERROR_CONNECT = 11
LC_ERROR_OS = 12
LC_ERROR_OS_NET = 13
LC_ERROR_OS_FILE = 14
LC_ERROR_UNSUPP = 15
LC_ERROR_INVALID_CONFIG = 16
LC_ERROR_IR_OVERFLOW = 17

LC_FILE_TYPE_CONNECTIVITY  = 1
LC_FILE_TYPE_CONFIGURATION = 2
LC_FILE_TYPE_FIRMWARE      = 3
LC_FILE_TYPE_LEARN_IR      = 4

LC_CB_COUNTER_TYPE_STEPS = 5
LC_CB_COUNTER_TYPE_BYTES = 6

LC_CB_STAGE_NUM_STAGES = 0xFF
LC_CB_STAGE_GET_IDENTITY = 7
LC_CB_STAGE_INITIALIZE_UPDATE = 8
LC_CB_STAGE_INVALIDATE_FLASH = 9
LC_CB_STAGE_ERASE_FLASH = 10
LC_CB_STAGE_WRITE_CONFIG = 11
LC_CB_STAGE_VERIFY_CONFIG = 12
LC_CB_STAGE_FINALIZE_UPDATE = 13
LC_CB_STAGE_READ_CONFIG = 14
LC_CB_STAGE_WRITE_FIRMWARE = 15
LC_CB_STAGE_READ_FIRMWARE = 16
LC_CB_STAGE_READ_SAFEMODE = 17
LC_CB_STAGE_RESET = 18
LC_CB_STAGE_SET_TIME = 19
LC_CB_STAGE_HTTP = 20
LC_CB_STAGE_LEARN = 21

# Public libconcord API: Exception types

class LibConcordException(Exception):
    """This exception may be raised by any function that returns an LC_ERROR_*."""
    def __init__(self, func, result):
        self.func = func
        self.result = result
        try:
            self.result_str = lc_strerror(self.result).decode('utf-8')
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
        return result

# Internal helpers for result prototyping

class _ret(object):
    def __init__(self):
        pass

class _ret_void(_ret):
    def __init__(self):
        pass

    def real_ctypes_type(self):
        return c_int

    def checker(self):
        return None

    def dumper(self, value):
        return "void"

class _ret_lc_concord(_ret):
    def __init__(self):
        pass

    def real_ctypes_type(self):
        return c_int

    def checker(self):
        return _CheckRetCode

    def dumper(self, value):
        return repr(value)

# Internal helpers for parameter prototyping

class _param(object):
    def __init__(self, name, ctypes_type):
        self.name = name
        self.ctypes_type = ctypes_type

class _in(_param):
    def real_ctypes_type(self):
        return self.ctypes_type

class _out(_param):
    def real_ctypes_type(self):
        return POINTER(self.ctypes_type)

# Internal helpers for parameter tracing

def _dump_simple(type):
    def _dump_simple_imp(value):
        if not isinstance(value, type):
            value = type(value)
        return repr(value.value)
    return _dump_simple_imp

def _dump_pointer(name):
    def _dump_pointer_imp(value):
        return "<<%s @0x%08x>>" % (name, addressof(value))
    return _dump_pointer_imp

_dumpers = {
    c_char_p:          _dump_simple(c_char_p),
    c_int:             _dump_simple(c_int),
    c_long:            _dump_simple(c_long),
    c_uint:            _dump_simple(c_uint),
    c_ulong:           _dump_simple(c_ulong),
    POINTER(c_char_p): _dump_pointer('buffer'),
    POINTER(c_ubyte):  _dump_pointer('buffer'),
    POINTER(c_uint):   _dump_pointer('buffer'),
    callback_type:     _dump_pointer('function'),
    py_object:         _dump_simple(py_object),
}

# Internal function tracing functionality

_byref_type = type(byref(c_int()))
def from_param(obj):
    if type(obj) == _byref_type:
        return obj._obj
    if isinstance(obj, c_char_p):
        return obj.value
    if isinstance(obj, _Pointer):
        return obj.contents
    raise TypeError("Output parameters should be byref/pointers")

class _DebugWrapper(object):
    def __init__(self, callable):
        self.callable = callable

    def __call__(self, *args):
        sys.stdout.write("libconcord." + self.callable.__name__ + "(")
        delim = ""
        for (val, proto) in zip(args, self.callable._lcpy_protos):
            if isinstance(proto, _param):
                name = proto.name
                if isinstance(proto, _in):
                    try:
                        type = proto.ctypes_type
                        value = _dumpers[type](val)
                    except:
                        value = "???"
                else:
                    value = "<<out>>"
            else:
                name = "???"
                value = "???"
            sys.stdout.write("%s%s=%s" % (delim, name, value))
            delim = ", "
        print(")")
        try:
            ret = self.callable(*args)
            if isinstance(self.callable._lcpy_rettype, _ret):
                valstr = self.callable._lcpy_rettype.dumper(ret)
            else:
                valstr = repr(ret)
            print("    Returned: " + valstr)
            for (val, proto) in zip(args, self.callable._lcpy_protos):
                if not isinstance(proto, _out):
                    continue
                name = proto.name
                type = proto.ctypes_type
                try:
                    value = _dumpers[type](from_param(val))
                except:
                    value = "???"
                print("    <<out>> %s=%s" % (name, value))
            return ret
        except:
            print("    Threw: ")
            s = traceback.format_exc()
            for sl in s.split('\n'):
                print("        " + sl)
            raise

# Internal ctypes function wrapper creation

def _create_func(
    func_name,
    rettype,
    *protos
):
    def real_ctypes_type(type):
        if not isinstance(type, _param):
            return type
        return type.real_ctypes_type()
    if isinstance(rettype, _ret):
        real_rettype = rettype.real_ctypes_type()
    else:
        real_rettype = rettype
    real_protos = [real_ctypes_type(x) for x in protos]
    ftype = CFUNCTYPE(real_rettype, *real_protos)
    f = ftype((func_name, _dll))
    f._lcpy_rettype = rettype
    f._lcpy_protos = protos
    f.__name__ = func_name
    if isinstance(rettype, _ret):
        checker = rettype.checker()
        if checker:
            f.errcheck = checker(func_name)
    if debug:
        f = _DebugWrapper(f)
    f.__name__ = func_name
    return f

# Public libconcord API: Function prototypes

# const char *get_mfg();
get_mfg = _create_func(
    'get_mfg',
    c_char_p
)

# const char *get_model();
get_model = _create_func(
    'get_model',
    c_char_p
)

# const char *get_codename();
get_codename = _create_func(
    'get_codename',
    c_char_p
)

# int get_skin();
get_skin = _create_func(
    'get_skin',
    c_int
)

# int get_fw_ver_maj();
get_fw_ver_maj = _create_func(
    'get_fw_ver_maj',
    c_int
)

# int get_fw_ver_min();
get_fw_ver_min = _create_func(
    'get_fw_ver_min',
    c_int
)

# int get_fw_type();
get_fw_type = _create_func(
    'get_fw_type',
    c_int
)

# int get_hw_ver_maj();
get_hw_ver_maj = _create_func(
    'get_hw_ver_maj',
    c_int
)

# int get_hw_ver_min();
get_hw_ver_min = _create_func(
    'get_hw_ver_min',
    c_int
)

# int get_hw_ver_mic();
get_hw_ver_mic = _create_func(
    'get_hw_ver_mic',
    c_int
)

# int get_flash_size();
get_flash_size = _create_func(
    'get_flash_size',
    c_int
)

# int get_flash_mfg();
get_flash_mfg = _create_func(
    'get_flash_mfg',
    c_int
)

# int get_flash_id();
get_flash_id = _create_func(
    'get_flash_id',
    c_int
)

# const char *get_flash_part_num();
get_flash_part_num = _create_func(
    'get_flash_part_num',
    c_char_p
)

# int get_arch();
get_arch = _create_func(
    'get_arch',
    c_int
)

# int get_proto();
get_proto = _create_func(
    'get_proto',
    c_int
)

# const char *get_hid_mfg_str();
get_hid_mfg_str = _create_func(
    'get_hid_mfg_str',
    c_char_p
)

# const char *get_hid_prod_str();
get_hid_prod_str = _create_func(
    'get_hid_prod_str',
    c_char_p
)

# int get_hid_irl();
get_hid_irl = _create_func(
    'get_hid_irl',
    c_int
)

# int get_hid_orl();
get_hid_orl = _create_func(
    'get_hid_orl',
    c_int
)

# int get_hid_frl();
get_hid_frl = _create_func(
    'get_hid_frl',
    c_int
)

# int get_usb_vid();
get_usb_vid = _create_func(
    'get_usb_vid',
    c_int
)

# int get_usb_pid();
get_usb_pid = _create_func(
    'get_usb_pid',
    c_int
)

# int get_usb_bcd();
get_usb_bcd = _create_func(
    'get_usb_bcd',
    c_int
)

SERIAL_COMPONENT_1 = 1
SERIAL_COMPONENT_2 = 2
SERIAL_COMPONENT_3 = 3

# char *get_serial(int p);
get_serial = _create_func(
    'get_serial',
    c_char_p,
    _in('p', c_int)
)

# int get_config_bytes_used();
get_config_bytes_used = _create_func(
    'get_config_bytes_used',
    c_int
)

# int get_config_bytes_total();
get_config_bytes_total = _create_func(
    'get_config_bytes_total',
    c_int
)

# int is_config_dump_supported();
is_config_dump_supported = _create_func(
    'is_config_dump_supported',
    c_int
)

# int is_config_update_supported();
is_config_update_supported = _create_func(
    'is_config_update_supported',
    c_int
)

# int is_fw_dump_supported();
is_fw_dump_supported = _create_func(
    'is_fw_dump_supported',
    c_int
)

# int is_fw_update_supported(int direct);
is_fw_update_supported = _create_func(
    'is_fw_update_supported',
    c_int,
    _in('direct', c_int)
)

# int get_time_second();
get_time_second = _create_func(
    'get_time_second',
    c_int
)

# int get_time_minute();
get_time_minute = _create_func(
    'get_time_minute',
    c_int
)

# int get_time_hour();
get_time_hour = _create_func(
    'get_time_hour',
    c_int
)

# int get_time_day();
get_time_day = _create_func(
    'get_time_day',
    c_int
)

# int get_time_dow();
get_time_dow = _create_func(
    'get_time_dow',
    c_int
)

# int get_time_month();
get_time_month = _create_func(
    'get_time_month',
    c_int
)

# int get_time_year();
get_time_year = _create_func(
    'get_time_year',
    c_int
)

# int get_time_utc_offset();
get_time_utc_offset = _create_func(
    'get_time_utc_offset',
    c_int
)

# const char *get_time_timezone();
get_time_timezone = _create_func(
    'get_time_timezone',
    c_char_p
)

# This is not prototyped using _create_func
# to avoid error checker and tracing loops etc.
# const char *lc_strerror(int err);
lc_strerror = _dll.lc_strerror
lc_strerror.restype = c_char_p
lc_strerror.argtypes = (c_int,)

# const char *lc_cb_stage_str(int stage);
lc_cb_stage_str = _create_func(
    'lc_cb_stage_str',
    c_char_p,
    _in('stage', c_int)
)

# void delete_blob(uint8_t *ptr);
delete_blob = _create_func(
    'delete_blob',
    _ret_void(),
    _in('ptr', POINTER(c_ubyte))
);

# int read_and_parse_file(char *filename, int *type);
read_and_parse_file = _create_func(
    'read_and_parse_file',
    _ret_lc_concord(),
    _in('filename', c_char_p),
    _out('type', c_int)
)

# void delete_opfile_obj();
delete_opfile_obj = _create_func(
    'delete_opfile_obj',
    _ret_void()
)

# int init_concord();
init_concord = _create_func(
    'init_concord',
    _ret_lc_concord()
)

# int deinit_concord();
deinit_concord = _create_func(
    'deinit_concord',
    _ret_lc_concord()
)

# int get_identity(lc_callback cb, void *cb_arg);
get_identity = _create_func(
    'get_identity',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int reset_remote(lc_callback cb, void *cb_arg);
reset_remote = _create_func(
    'reset_remote',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int get_time();
get_time = _create_func(
    'get_time',
    _ret_lc_concord()
)

# int set_time(lc_callback cb, void *cb_arg);
set_time = _create_func(
    'set_time',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int post_connect_test_success(lc_callback cb, void *cb_arg);
post_connect_test_success = _create_func(
    'post_connect_test_success',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int post_preconfig(lc_callback cb, void *cb_arg);
post_preconfig = _create_func(
    'post_preconfig',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int post_postconfig(lc_callback cb, void *cb_arg);
post_postconfig = _create_func(
    'post_postconfig',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int post_postfirmware(lc_callback cb, void *cb_arg);
post_postfirmware = _create_func(
    'post_postfirmware',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int invalidate_flash(lc_callback cb, void *cb_arg);
invalidate_flash = _create_func(
    'invalidate_flash',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int update_configuration(lc_callback cb, void *cb_arg, int noreset);
update_configuration = _create_func(
    'update_configuration',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object),
    _in('noreset', c_int)
)

# int read_config_from_remote(uint8_t **out, uint32_t *size,
#     lc_callback cb, void *cb_arg);
read_config_from_remote = _create_func(
    'read_config_from_remote',
    _ret_lc_concord(),
    _out('out', POINTER(c_ubyte)),
    _out('size', c_uint),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int write_config_to_remote(lc_callback cb, void *cb_arg);
write_config_to_remote = _create_func(
    'write_config_to_remote',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int write_config_to_file(uint8_t *in, uint32_t size, char *file_name,
#     int binary);
write_config_to_file = _create_func(
    'write_config_to_file',
    _ret_lc_concord(),
    _in('in', POINTER(c_ubyte)),
    _in('size', c_uint),
    _in('file_name', c_char_p),
    _in('binary', c_int)
)

# int verify_remote_config(lc_callback cb, void *cb_arg);
verify_remote_config = _create_func(
    'verify_remote_config',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int prep_config(lc_callback cb, void *cb_arg);
prep_config = _create_func(
    'prep_config',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int finish_config(lc_callback cb, void *cb_arg);
finish_config = _create_func(
    'finish_config',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int erase_config(lc_callback cb, void *cb_arg);
erase_config = _create_func(
    'erase_config',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int erase_safemode(lc_callback cb, void *cb_arg);
erase_safemode = _create_func(
    'erase_safemode',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int read_safemode_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
#     void *cb_arg);
read_safemode_from_remote = _create_func(
    'read_safemode_from_remote',
    _ret_lc_concord(),
    _out('out', POINTER(c_ubyte)),
    _out('size', c_ubyte),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int write_safemode_to_file(uint8_t *in, uint32_t size, char *file_name);
write_safemode_to_file = _create_func(
    'write_safemode_to_file',
    _ret_lc_concord(),
    _in('in', POINTER(c_ubyte)),
    _in('size', c_uint),
    _in('file_name', c_char_p)
)

# int update_firmware(lc_callback cb, void *cb_arg, int noreset, int direct);
update_firmware = _create_func(
    'update_firmware',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object),
    _in('noreset', c_int),
    _in('direct', c_int)
)

# int is_config_safe_after_fw();
is_config_safe_after_fw = _create_func(
    'is_config_safe_after_fw',
    c_int
)

# int prep_firmware(lc_callback cb, void *cb_arg);
prep_firmware = _create_func(
    'prep_firmware',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object),
)

# int finish_firmware(lc_callback cb, void *cb_arg);
finish_firmware = _create_func(
    'finish_firmware',
    _ret_lc_concord(),
    _in('cb', callback_type),
    _in('cb_arg', py_object),
)

# int erase_firmware(int direct, lc_callback cb, void *cb_arg);
erase_firmware = _create_func(
    'erase_firmware',
    _ret_lc_concord(),
    _in('direct', c_int),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int read_firmware_from_remote(uint8_t **out, uint32_t *size, lc_callback cb,
#     void *cb_arg);
read_firmware_from_remote = _create_func(
    'read_firmware_from_remote',
    _ret_lc_concord(),
    _out('out', POINTER(c_ubyte)),
    _out('size', c_uint),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int write_firmware_to_remote(int direct, lc_callback cb, void *cb_arg);
write_firmware_to_remote = _create_func(
    'write_firmware_to_remote',
    _ret_lc_concord(),
    _in('direct', c_int),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int write_firmware_to_file(uint8_t *in, uint32_t size, char *file_name,
#     int binary);
write_firmware_to_file = _create_func(
    'write_firmware_to_file',
    _ret_lc_concord(),
    _in('in', POINTER(c_ubyte)),
    _in('size', c_uint),
    _in('file_name', c_char_p),
    _in('binary', c_int)
)

# int get_key_names(char ***key_names, uint32_t *key_names_length);
get_key_names = _create_func(
    'get_key_names',
    _ret_lc_concord(),
    _out('key_names', POINTER(c_char_p)),
    _out('key_names_length', c_uint)
)

# void delete_key_names(char **key_names, uint32_t key_names_length);
delete_key_names = _create_func(
    'delete_key_names',
    _ret_void(),
    _in('key_names', POINTER(c_char_p)),
    _in('key_names_length', c_uint)
)

# int learn_from_remote(uint32_t *carrier_clock,
#     uint32_t **ir_signal, uint32_t *ir_signal_length,
#     lc_callback cb, void *cb_arg);
learn_from_remote = _create_func(
    'learn_from_remote',
    _ret_lc_concord(),
    _out('carrier_clock', c_uint),
    _out('ir_signal', POINTER(c_uint)),
    _out('ir_signal_length', c_uint),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# void delete_ir_signal(uint32_t *ir_signal);
delete_ir_signal = _create_func(
    'delete_ir_signal',
    _ret_void(),
    _in('ir_signal', POINTER(c_uint))
)

# int encode_for_posting(uint32_t carrier_clock,
#     uint32_t *ir_signal, uint32_t ir_signal_length,
#     char **encoded_signal);
encode_for_posting = _create_func(
    'encode_for_posting',
    _ret_lc_concord(),
    _in('carrier_clock', c_uint),
    _in('ir_signal', POINTER(c_uint)),
    _in('ir_signal_length', c_uint),
    _out('encoded_signal', c_char_p)
)

# void delete_encoded_signal(char *encoded_signal);
delete_encoded_signal = _create_func(
    'delete_encoded_signal',
    _ret_void(),
    _in('encoded_signal', c_char_p),
)

# int post_new_code(char *key_name, char *encoded_signal, lc_callback cb,
#     void *cb_arg);
post_new_code = _create_func(
    'post_new_code',
    _ret_lc_concord(),
    _in('key_name', c_char_p),
    _in('encoded_signal', c_char_p),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

#define MH_STRING_LENGTH 255 /* arbitrary */
MH_STRING_LENGTH = 255

#define MH_MAX_WIFI_NETWORKS 30 /* arbitrary */
MH_MAX_WIFI_NETWORKS = 30

#struct mh_cfg_properties {
#    char host_name[MH_STRING_LENGTH];
#    char email[MH_STRING_LENGTH];
#    char service_link[MH_STRING_LENGTH];
#};
class mh_cfg_properties(Structure):
    _fields_ = [("host_name", c_char * MH_STRING_LENGTH),
                ("email", c_char * MH_STRING_LENGTH),
                ("service_link", c_char * MH_STRING_LENGTH)]

#struct mh_wifi_config {
#    char ssid[MH_STRING_LENGTH];
#    char encryption[MH_STRING_LENGTH];
#    char password[MH_STRING_LENGTH];
#    char connect_status[MH_STRING_LENGTH];
#    char error_code[MH_STRING_LENGTH];
#};
class mh_wifi_config(Structure):
    _fields_ = [("ssid", c_char * MH_STRING_LENGTH),
                ("encryption", c_char * MH_STRING_LENGTH),
                ("password", c_char * MH_STRING_LENGTH),
                ("connect_status", c_char * MH_STRING_LENGTH),
                ("error_code", c_char * MH_STRING_LENGTH)]

#struct mh_wifi_network {
#    char ssid[MH_STRING_LENGTH];
#    char signal_strength[MH_STRING_LENGTH];
#    char channel[MH_STRING_LENGTH];
#    char encryption[MH_STRING_LENGTH];
#};
class mh_wifi_network(Structure):
    _fields_ = [("ssid", c_char * MH_STRING_LENGTH),
                ("signal_strength", c_char * MH_STRING_LENGTH),
                ("channel", c_char * MH_STRING_LENGTH),
                ("encryption", c_char * MH_STRING_LENGTH)]

#struct mh_wifi_networks {
#    struct mh_wifi_network network[MH_MAX_WIFI_NETWORKS];
#};
class mh_wifi_networks(Structure):
    _fields_ = [("network", mh_wifi_network * MH_MAX_WIFI_NETWORKS)]

#int mh_get_cfg_properties(struct mh_cfg_properties *properties);
mh_get_cfg_properties = _create_func(
    'mh_get_cfg_properties',
    _ret_lc_concord(),
    _in('properties', POINTER(mh_cfg_properties))
)

#int mh_set_cfg_properties(const struct mh_cfg_properties *properties);
mh_set_cfg_properties = _create_func(
    'mh_set_cfg_properties',
    _ret_lc_concord(),
    _in('properties', POINTER(mh_cfg_properties))
)

#int mh_get_wifi_networks(struct mh_wifi_networks *networks);
mh_get_wifi_networks = _create_func(
    'mh_get_wifi_networks',
    _ret_lc_concord(),
    _in('networks', POINTER(mh_wifi_networks))
)

#int mh_get_wifi_config(struct mh_wifi_config *config);
mh_get_wifi_config = _create_func(
    'mh_get_wifi_config',
    _ret_lc_concord(),
    _in('config', POINTER(mh_wifi_config))
)

#int mh_set_wifi_config(const struct mh_wifi_config *config);
mh_set_wifi_config = _create_func(
    'mh_set_wifi_config',
    _ret_lc_concord(),
    _in('config', POINTER(mh_wifi_config))
)

# const char *mh_get_serial();
mh_get_serial = _create_func(
    'mh_get_serial',
    c_char_p
)

#int mh_read_file(const char *filename, uint8_t *buffer, const uint32_t buflen,
#                 uint32_t *data_read);
mh_read_file = _create_func(
    'mh_read_file',
    _ret_lc_concord(),
    _in('filename', c_char_p),
    _in('buffer', POINTER(c_ubyte)),
    _in('buflen', c_uint),
    _out('data_read', c_uint)
)

#int mh_write_file(const char *filename, uint8_t *buffer,
#                  const uint32_t buflen);
mh_write_file = _create_func(
    'mh_write_file',
    _ret_lc_concord(),
    _in('filename', c_char_p),
    _in('buffer', POINTER(c_ubyte)),
    _in('buflen', c_uint)
)
