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
 * These are all the commands and responses that can be sent to the remote.
 *
 * This lists all base commands (first nibble of first byte), and also the
 * sub-command for those commands that have them.
 *
 * The second nibble of the *first* byte is a size mapping.
 *
 * For more details please see specs/protocol.h
 *
 * Below, where the second half of the byte may change, it's always defined
 * as a 0. However, where the command will always be a static size, it's
 * defined with that value.
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

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
#define COMMAND_WRITE_MISC 0xA0
#define COMMAND_READ_MISC 0xB0
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
#define COMMAND_MISC_RESTART_CONFIG 0x0A
/*
 * Continuing with base commands...
 */
#define COMMAND_ERASE_FLASH 0xD3
#define COMMAND_RESET 0xE1
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
#define COMMAND_DONE 0xF1


/*
 * Responses from the Harmony
 */
#define RESPONSE_VERSION_DATA 0x20
#define RESPONSE_READ_FLASH_DATA 0x60
#define RESPONSE_IRCAP_DATA 0x90
#define RESPONSE_READ_MISC_DATA 0xC0
#define RESPONSE_DONE 0xF0

#endif

