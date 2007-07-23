/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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
 *  (C) Copyright Phil Dibowitz 2007
 */

/*
 * The base command is always the first byte. This first byte is split into
 * two nibbles. The upper nibble determines the command. So commands can
 * always be thought of as 0x10, 0x20, 0x30, 0x40, etc. The other nibble is
 * an indicator of how many bytes will be sent. 
 *
 * However, it's not a linear mapping, in fact, this second nibble changes
 * depending on the mode, so see ReadFlash(), but it's usually something
 * like 0=0, 1=0, 1-1, 2=2, 3=3, 4=4, 5=5, 6=6, 7=14, 8=30, 9=62, and a-f
 * aren't used. Again, this is non-linear.
 *
 * Below, where the second half of the byte may change, it's always defined
 * as a 0. However, where the command will always be a static size, it's
 * defined with that value.
 */

/*
 * Masks for command nibble and length nibble
 */
#define COMMAND_MASK 0xF0
#define LENGTH_MASK 0x0F

/*
 * Base commands one can send to the Harmony
 */
#define COMMAND_INVALID 0x00
#define COMMAND_GET_VERSION 0x10
#define COMMAND_WRITE_FLASH 0x30
#define COMMAND_WRITE_FLASH_DATA 0x40
#define COMMAND_READ_FLASH 0x50
/*
 * Byte 2 for ... flash command?
 */
#define COMMAND_PROM_TYPE_MCU_FLASH 0x01
#define COMMAND_PROM_TYPE_MCU_EEPROM 0x02
#define COMMAND_PROM_TYPE_MCU_ID 0x03
#define COMMAND_PROM_TYPE_EXT_FLASH 0x04
/*
 * Continuing with our base commands
 */
#define COMMAND_START_IRCAP 0x70
#define COMMAND_STOP_IRCAP 0x80
#define COMMAND_IRCAP_DATA 0x90
#define COMMAND_WRITE_MISC 0xa0
#define COMMAND_READ_MISC 0xb0
#define COMMAND_READ_MISC_DATA 0xc0
/*
 * Byte 2 for COMMAND_*_MISC
 */
#define COMMAND_MISC_EEPROM 0x00
#define COMMAND_MISC_STATE 0x01
#define COMMAND_MISC_INVALIDATE_FLASH 0x02
#define COMMAND_MISC_QUEUE_ACTION 0x03
#define COMMAND_MISC_PROGRAM 0x04
#define COMMAND_MISC_INTERRUPT 0x05
#define COMMAND_MISC_RAM 0x06
#define COMMAND_MISC_REGISTER 0x07
#define COMMAND_MISC_CLOCK_RECALCULATE 0x08
#define COMMAND_MISC_QUEUE_EVENT 0x09
#define COMMAND_MISC_RESTART_CONFIG 0x0a
/*
 * Continuing with base commands...
 */
#define COMMAND_ERASE_FLASH 0xd3
#define COMMAND_RESET 0xe1
/*
 * Byte 2 for COMMAND_RESET
 */
#define COMMAND_RESET_USB 0x01
#define COMMAND_RESET_DEVICE 0x02
#define COMMAND_RESET_DEVICE_DISCONNECT 0x03
#define COMMAND_RESET_TEST_FINISH 0x04
/*
 * Continuing with base commands...
 */
#define COMMAND_DONE 0xf1


/*
 * Responses from the Harmony
 */
#define RESPONSE_VERSION_DATA 0x20
#define RESPONSE_READ_FLASH_DATA 0x60
#define RESPONSE_DONE 0xF0
