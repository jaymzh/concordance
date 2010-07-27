#
# vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=4 expandtab:
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
from ctypes import _Pointer
import platform
import os
import sys
import traceback

# Debug request parsing

debug = (os.environ.get("LIBCONCORD_PY_TRACE", "0") == "1")

# Define the libconcord ABI this libconcord.py corresponds to
# Bump this when the .so file version gets bumped

ABI_VERSION = 1

# Load the DLL

if platform.system() == 'Windows':
    _dll = cdll.LoadLibrary('libconcord.dll')
else:
    _dll = cdll.LoadLibrary('libconcord.so.' + str(ABI_VERSION))

# Public libconcord API: Custom types

# typedef void (*lc_callback)(uint32_t, uint32_t, uint32_t, void*);
callback_type = CFUNCTYPE(None, c_uint, c_uint, c_uint, py_object)

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

# Public libconcord API: Exception types

class LibConcordException(Exception):
    """This exception may be raised by any function that returns an LC_ERROR_*."""
    def __init__(self, func, result):
        self.func = func
        self.result = result
        try:
            self.result_str = lc_strerror(self.result)
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
    raise TypeError, "Output parameters should be byref/pointers"

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
        print ")"
        try:
            ret = self.callable(*args)
            if isinstance(self.callable._lcpy_rettype, _ret):
                valstr = self.callable._lcpy_rettype.dumper(ret)
            else:
                valstr = repr(ret)
            print "    Returned: " + valstr
            for (val, proto) in zip(args, self.callable._lcpy_protos):
                if not isinstance(proto, _out):
                    continue
                name = proto.name
                type = proto.ctypes_type
                try:
                    value = _dumpers[type](from_param(val))
                except:
                    value = "???"
                print "    <<out>> %s=%s" % (name, value)
            return ret
        except:
            print "    Threw: "
            s = traceback.format_exc()
            for sl in s.split('\n'):
                print "        " + sl
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
    c_int
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

# void delete_blob(uint8_t *ptr);
delete_blob = _create_func(
    'delete_blob',
    _ret_void(),
    _in('ptr', POINTER(c_ubyte))
);

LC_FILE_TYPE_CONNECTIVITY  = 0
LC_FILE_TYPE_CONFIGURATION = 1
LC_FILE_TYPE_FIRMWARE      = 2
LC_FILE_TYPE_LEARN_IR      = 3

# int identify_file(uint8_t *in, uint32_t size, int *type);
identify_file = _create_func(
    'identify_file',
    _ret_lc_concord(),
    _in('in', POINTER(c_ubyte)),
    _in('size', c_uint),
    _out('type', c_int)
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

# int reset_remote();
reset_remote = _create_func(
    'reset_remote',
    _ret_lc_concord()
)

# int get_time();
get_time = _create_func(
    'get_time',
    _ret_lc_concord()
)

# int set_time();
set_time = _create_func(
    'set_time',
    _ret_lc_concord()
)

# int post_connect_test_success(uint8_t *data, uint32_t size);
post_connect_test_success = _create_func(
    'post_connect_test_success',
    _ret_lc_concord(),
    _in('data', POINTER(c_ubyte)),
    _in('size', c_uint)
)

# int post_preconfig(uint8_t *data, uint32_t size);
post_preconfig = _create_func(
    'post_preconfig',
    _ret_lc_concord(),
    _in('data', POINTER(c_ubyte)),
    _in('size', c_uint)
)

# int post_postconfig(uint8_t *data, uint32_t size);
post_postconfig = _create_func(
    'post_postconfig',
    _ret_lc_concord(),
    _in('data', POINTER(c_ubyte)),
    _in('size', c_uint)
)

# int post_postfirmware(uint8_t *data, uint32_t size);
post_postfirmware = _create_func(
    'post_postfirmware',
    _ret_lc_concord(),
    _in('data', POINTER(c_ubyte)),
    _in('size', c_uint)
)

# int invalidate_flash();
invalidate_flash = _create_func(
    'invalidate_flash',
    _ret_lc_concord(),
)

# int read_config_from_remote(uint8_t **out, uint32_t *size,
#	lc_callback cb, void *cb_arg);
read_config_from_remote = _create_func(
    'read_config_from_remote',
    _ret_lc_concord(),
    _out('out', POINTER(c_ubyte)),
    _out('size', c_uint),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int write_config_to_remote(uint8_t *in, uint32_t size,
#     lc_callback cb, void *cb_arg);
write_config_to_remote = _create_func(
    'write_config_to_remote',
    _ret_lc_concord(),
    _in('in', POINTER(c_ubyte)),
    _in('size', c_uint),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int read_file(char *file_name, uint8_t **out, uint32_t *size);
read_file = _create_func(
    'read_file',
    _ret_lc_concord(),
    _in('file_name', c_char_p),
    _out('out', POINTER(c_ubyte)),
    _out('size', c_uint)
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

# int verify_remote_config(uint8_t *in, uint32_t size, lc_callback cb,
#     void *cb_arg);
verify_remote_config = _create_func(
    'verify_remote_config',
    _ret_lc_concord(),
    _in('in', POINTER(c_ubyte)),
    _in('size', c_uint),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int prep_config();
prep_config = _create_func(
    'prep_config',
    _ret_lc_concord()
)

# int finish_config();
finish_config = _create_func(
    'finish_config',
    _ret_lc_concord()
)

# int erase_config(uint32_t size, lc_callback cb, void *cb_arg);
erase_config = _create_func(
    'erase_config',
    _ret_lc_concord(),
    _in('size', c_uint),
    _in('cb', callback_type),
    _in('cb_arg', py_object)
)

# int find_config_binary(uint8_t *config, uint32_t config_size,
#     uint8_t **binary_ptr, uint32_t *binary_size);
find_config_binary = _create_func(
    'find_config_binary',
    _ret_lc_concord(),
    _in('config', POINTER(c_ubyte)),
    _in('config_size', c_uint),
    _out('binary_ptr', POINTER(c_ubyte)),
    _out('binary_size', c_uint)
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

# int is_fw_update_supported(int direct);
is_fw_update_supported = _create_func(
    'is_fw_update_supported',
    c_int,
    _in('direct', c_int)
)

# int is_config_safe_after_fw();
is_config_safe_after_fw = _create_func(
    'is_config_safe_after_fw',
    c_int
)

# int prep_firmware();
prep_firmware = _create_func(
    'prep_firmware',
    _ret_lc_concord()
)

# int finish_firmware();
finish_firmware = _create_func(
    'finish_firmware',
    _ret_lc_concord()
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

# int write_firmware_to_remote(uint8_t *in, uint32_t size, int direct,
#     lc_callback cb, void *cb_arg);
write_firmware_to_remote = _create_func(
    'write_firmware_to_remote',
    _ret_lc_concord(),
    _in('in', POINTER(c_ubyte)),
    _in('size', c_uint),
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

# int extract_firmware_binary(uint8_t *xml, uint32_t xml_size, uint8_t **out,
#     uint32_t *size);
extract_firmware_binary = _create_func(
    'extract_firmware_binary',
    _ret_lc_concord(),
    _in('xml', POINTER(c_ubyte)),
    _in('xml_size', c_uint),
    _out('out', POINTER(c_ubyte)),
    _out('size', c_uint)
)

# int get_key_names(uint8_t *xml, uint32_t xml_size,
#     char ***key_names, uint32_t *key_names_length);
get_key_names = _create_func(
    'get_key_names',
    _ret_lc_concord(),
    _in('xml', POINTER(c_ubyte)),
    _in('xml_size', c_uint),
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

# int post_new_code(uint8_t *xml, uint32_t xml_size, 
#     char *key_name, char *encoded_signal);
post_new_code = _create_func(
    'post_new_code',
    _ret_lc_concord(),
    _in('xml', POINTER(c_ubyte)),
    _in('xml_size', c_uint),
    _in('key_name', c_char_p),
    _in('encoded_signal', c_char_p)
)

