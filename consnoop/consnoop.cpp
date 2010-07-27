/*
 * vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=8 noexpandtab:
 *
 * $Id$
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

#include <string>
#include <string.h>
#include <fstream>
#include <stdlib.h>
#ifdef WIN32
typedef unsigned char uint8_t;
#include "../concordance/win/getopt/getopt.h"
#else
#include <getopt.h>
#endif

// TODO: Once we figure this stuff out, move it to someplace more useful.
#define TYPE_TCP_ACK 0x40
#define TYPE_TCP_FIN 0x20
#define TYPE_TCP_SYN 0x80

using namespace std;

#include "../libconcord/protocol.h"
#include "../libconcord/protocol_z.h"

static const unsigned int rxlenmap0[16] =
	{  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14 };
static const unsigned int rxlenmapx[16] =
	{  0,  0,  1,  2,  3,  4,  5,  6, 14, 30, 62,  0,  0,  0,  0,  0 };
static const unsigned int txlenmap0[16] =
	{  0,  1,  2,  3,  4,  5,  6,  7,  0,  0,  0,  0,  0,  0,  0,  0 };
static const unsigned int txlenmapx[16] =
	{  0,  1,  2,  3,  4,  5,  6,  7, 15, 31, 63,  0,  0,  0,  0,  0 };

bool verbose = false;
bool debug = false;

int hex_to_int(char c)
{
	if (c >= '0' && c <= '9')
		return c-'0';
	if (c >= 'a' && c <= 'f')
		return c-'a'+10;
	if (c >= 'A' && c <= 'F')
		return c-'A'+10;
	return -1;
}

const char* get_misc(uint8_t x)
{
	switch (x) {
		case COMMAND_MISC_EEPROM:
			return "EEPROM";
		case COMMAND_MISC_STATE:
			return "State";
		case COMMAND_MISC_INVALIDATE_FLASH:
			return "Invalidate Flash";
		case COMMAND_MISC_QUEUE_ACTION:
			return "Queue Action";
		case COMMAND_MISC_PROGRAM:
			return "Program";
		case COMMAND_MISC_INTERRUPT:
			return "Interrupt";
		case COMMAND_MISC_RAM:
			return "RAM";
		case COMMAND_MISC_REGISTER:
			return "Register";
		case COMMAND_MISC_CLOCK_RECALCULATE:
			return "Clock Recalculate";
		case COMMAND_MISC_QUEUE_EVENT:
			return "Queue Event";
		case COMMAND_MISC_RESTART_CONFIG:
			return "Restart Config";
	}
	return "Unknown";
}

void print_data(const uint8_t * const data, unsigned int const * lenmap)
{
	if (!verbose)
		return;
	const uint8_t length_enc = data[0] & LENGTH_MASK;
	const uint8_t length = lenmap[length_enc];
	printf("   DATA: ");
	for (int i = 1; i <= length; i++) {
		printf("%02X", data[i]);
	}
	printf("\n");
}

void decode(const uint8_t * const data, int proto)
{
	// Note: The uninteresting stuff is commented out
	const uint8_t length = data[0] & LENGTH_MASK;
	switch (data[0] & COMMAND_MASK) {
		case COMMAND_GET_VERSION & COMMAND_MASK:
			printf("Get   Version\n");
			break;
		case RESPONSE_VERSION_DATA & COMMAND_MASK:
			printf("Get   Version Response %02X %02X %02X %02X %02X"
				"%02X %02X\n",
				data[1],data[2],data[3],data[4],data[5],
				data[6],data[7]);
			break;
		case COMMAND_WRITE_FLASH & COMMAND_MASK:
			printf("Write Flash %06X %i\n",
				data[1]<<16|data[2]<<8|data[3],
				data[4]<<8|data[5]);
			break;
		case COMMAND_WRITE_FLASH_DATA & COMMAND_MASK:
			printf("Write Flash Data\n");
			print_data(data, proto ? txlenmapx : txlenmap0);
			break;
		case COMMAND_READ_FLASH & COMMAND_MASK:
			printf("Read  Flash %06X %i\n",
				data[1]<<16|data[2]<<8|data[3],
				data[4]<<8|data[5]);
			break;
		case RESPONSE_READ_FLASH_DATA & COMMAND_MASK:
			printf("Read  Flash Data\n");
			print_data(data, proto ? rxlenmapx : rxlenmap0);
			break;
		case COMMAND_START_IRCAP & COMMAND_MASK:
			printf("Start IR capture\n");
			break;
		case COMMAND_STOP_IRCAP & COMMAND_MASK:
			printf("Stop  IR capture\n");
			break;
		case RESPONSE_IRCAP_DATA & COMMAND_MASK:
			printf("IR Capture Data\n");
			break;
		case COMMAND_WRITE_MISC & COMMAND_MASK:
			if(length==1)
				printf("Write %s\n",get_misc(data[1]));
			else if(length==3)
				printf("Write %s %02X %02X\n",
					get_misc(data[1]), data[2], data[3]);
			else
				printf("Write %s %02X %02X %02X %02X\n",
					get_misc(data[1]), data[2], data[3],
					data[4], data[5]);
			break;
		case COMMAND_READ_MISC & COMMAND_MASK:
			if(length==2)
				printf("Read  %s %02X\n",
					get_misc(data[1]), data[2]);
			else
				printf("Read  %s %02X %02X\n",
					get_misc(data[1]), data[2], data[3]);
			break;
		case RESPONSE_READ_MISC_DATA & COMMAND_MASK:
			if(length==2)
				printf("Read  %s Data %02X\n",
					get_misc(data[1]), data[2]);
			else
				printf("Read  %s Data %02X %02X\n",
					get_misc(data[1]), data[2], data[3]);
			break;
		case COMMAND_ERASE_FLASH & COMMAND_MASK:
			printf("Erase Flash %06X\n",
				data[1]<<16|data[2]<<8|data[3]);
			break;
		case COMMAND_RESET & COMMAND_MASK:
			printf("Reset ");
			switch (data[1]) {
				case COMMAND_RESET_USB:
					printf("USB (01)\n");
					break;
				case COMMAND_RESET_DEVICE:
					printf("Device (02)\n");
					break;
				case COMMAND_RESET_DEVICE_DISCONNECT:
					printf("Device Disconnect (03)\n");
					break;
				case COMMAND_RESET_TEST_FINISH:
					printf("Test Finish (04)\n");
					break;
				default:
					printf("Unknown (%02X)\n", data[1]);
					break;
			}
			break;
		case COMMAND_DONE & COMMAND_MASK:
			printf("Done %02X\n", data[1]);
			break;
	}
}

void print_z_params(const uint8_t * const data, const uint8_t length)
{
	for (int i = 4; i <= length; i++) {
		printf(" %02X", data[i]);
	}
}

void decode_z_net_tcp(int *mode, const uint8_t * const data)
{
	//const uint8_t numparams = data[3];
	if ((data[0] & 0xF0) != 0x20) {
		printf("Skipping family %02X command %02X\n", data[0], data[1]);
		return;
	}

	switch (data[1]) {
		case COMMAND_INITIATE_ZWAVE_TCP_CHANNEL:
			if (!data[2]) {
				printf("Initiate ZWave TCP Channel\n");
				break;
			}
			printf("Initiate ZWave TCP Channel Response");
			//print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_EXECUTE_ACTION:
			if (!data[2]) {
				printf("Execute Action\n");
				break;
			}
			printf("Execute Action Response");
			//print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_INITIATE_UPDATE_TCP_CHANNEL:
			if (!data[2]) {
				printf("Initiate Update TCP Channel\n");
				break;
			}
			printf("Initiate Update TCP Channel Response");
			//print_z_params(data, length);
			printf("\n");
			break;


			
		default:
			printf("Unknown TCP command: %02X\n", data[1]);
			break;
			
	}
}

void decode_z_hid_tcp(int *mode, const uint8_t * const data)
{
	
	printf("LEN: %02X, ", data[0]);
	printf("FLAGS:");

	if ((data[1] & 0xE0 & TYPE_TCP_SYN) != 0) {
		printf(" SYN");
	}
	if ((data[1] & 0xE0 & TYPE_TCP_FIN) != 0) {
		printf(" FIN");
	}
	if ((data[1] & 0xE0 & TYPE_TCP_ACK) != 0) {
		printf(" ACK");
	}
	printf(",");

	printf(" SEQ: %02X, ACK: %02X\n", data[2], data[3]);

	printf("   DATA:");
	// starts printing at data+4
	print_z_params(data, data[0]);

	printf("\n");
}

void decode_z_hid_udp(int *mode, const uint8_t * const data)
{
	// Note: The uninteresting stuff is commented out
	const uint8_t length = data[0];
	switch (data[3]) {
		case COMMAND_GET_SYSTEM_INFO:
			if (!data[2]) {
				printf("Get System Info\n");
				break;
			}
			printf("Get System Info Response:");
			print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_GET_GUID:
			if (!data[2]) {
				printf("Get GUID\n");
				break;
			}
			printf("Get GUID Response:");
			print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_GET_REGION_IDS:
			if (!data[2]) {
				printf("Get Region IDs\n");
				break;
			}
			printf("Get Region IDs Response:");
			print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_GET_REGION_VERSION:
			if (!data[2]) {
				// has params
				printf("Get Region Version:");
			} else {
				printf("Get Region Version Response:");
			}
			print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_GET_HOME_ID:
			if (!data[2]) {
				printf("Get Home ID\n");
				break;
			}
			printf("Get Home ID Response:");
			print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_GET_NODE_ID:
			if (!data[2]) {
				printf("Get Node ID\n");
				break;
			}
			printf("Get Node ID Response:\n");
			print_z_params(data, length);
			break;
		case COMMAND_UDP_PING:
			if (!data[2]) {
				printf("Get UDP\n");
				break;
			}
			printf("Get UDP Response\n");
			break;
		case COMMAND_START_UPDATE:
			if (!data[2]) {
				printf("Start Update:");
				print_z_params(data, length);
				printf("\n");
				break;
			}
			printf("Start Update Response\n");
			break;
		case COMMAND_WRITE_UPDATE_HEADER:
			if (!data[2]) {
				printf("Write Update Header:");
				print_z_params(data, length);
				printf("\n");
				break;
			}
			printf("Write Update Header Response\n");
			break;
		case COMMAND_WRITE_UPDATE_DATA:
			if (!data[2]) {
				printf("Write Update Data:");
				printf(" Omitting data\n");
				break;
			}
			printf("Write Update Data Response:\n");
			break;
		case COMMAND_WRITE_UPDATE_DATA_DONE:
			if (!data[2]) {
				printf("Write Update Data Done:");
				print_z_params(data, length);
				printf("\n");
				break;
			}
			printf("Write Update Data Done Response\n");
			break;
		case COMMAND_GET_UPDATE_CHECKSUM:
			if (!data[2]) {
				printf("Get Update Checksum:");
			} else {
				printf("Get Update Checksum Response:");
			}
			print_z_params(data, length);
			printf("\n");
			break;
		case COMMAND_FINISH_UPDATE:
			if (!data[2]) {
				printf("Write Finish Update:");
				print_z_params(data, length);
				printf("\n");
				break;
			}
			printf("Finish Update Response\n");
			break;
		case COMMAND_RESET:
			if (!data[2]) {
				printf("Reset\n");
				break;
			}
			printf("Reset Response\n");
			break;
		case COMMAND_UPDATE_TIME:
			if (!data[2]) {
				printf("Update Time:");
				print_z_params(data, length);
				printf("\n");
				break;
			}
			printf("Update Time Response\n");
			break;
		case COMMAND_GET_CURRENT_TIME:
			if (!data[2]) {
				printf("Get Time\n");
				break;
			}
			printf("Get Time Response:");
			print_z_params(data, length);
			printf("\n");
			break;
		/* still needs to be documented */
		case COMMAND_INITIATE_UPDATE_TCP_CHANNEL:
			*mode = 1;
			if (!data[2]) {
				printf("Initiate Update TCP Channel\n");
				break;
			}
			printf("Initiate Update TCP Channel Response");
			print_z_params(data, length);
			printf("\n");
			break;
		default:
			printf("Unknown UDP command: %02X\n", data[3]);
			break;
	}
}

void decode_z(int *mode, const uint8_t * const data)
{
	if (*mode) {
		decode_z_hid_tcp(mode, data);
	} else {
		decode_z_hid_udp(mode, data);
	}
}


void help()
{
	printf("Usage: consnoop <options>\n\n");

	printf("Options:\n");
	printf("\t-v\tVerbose. Print bytes we write.\n");
	printf("\t-d\tDebug. Print full data for all decoded packets.\n");
	printf("\t-f <file>\tFilename to parse.\n");
	printf("\t-h\tThis help.\n\n");
	printf("\t-z\tDecode using z-wave HID.\n\n");
}

int main(int argc, char *argv[])
{
	int tmpint = 0;
	int proto = 1;
	int zwave = 0;
	int zwave_hid_mode = 0;
	char *file_name = NULL;
	while ((tmpint = getopt(argc, argv, "dhf:vz")) != EOF) {
		switch (tmpint) {
		case 'd':
			debug = true;
			break;
		case 'f':
			if (optarg == NULL) {
				fprintf(stderr, "Missing file name.\n");
			}
			file_name = optarg;
			break;
		case 'h':
			help();
			exit(0);
			break;
		case 'v':
			verbose = true;
			break;
		case '0':
			proto = 0;
			break;
		case 'z':
			zwave = 1;
			break;
		}
	}

	ifstream infile;
	infile.open(file_name);

	string s;
	string payloadbytes("<payloadbytes>");

	while(!infile.eof()) {
		getline(infile,s);
		if(!s.compare(0, payloadbytes.size(), payloadbytes)) {
			uint8_t data[260];
			memset(data, 0xcd, sizeof(data));
			unsigned int n = 0;
			for(unsigned int i = payloadbytes.size();
                            i<s.size(); ++i) {
				const char c=s[i];
				if (c == '<')
					break;
				const int h = hex_to_int(c);
				if (h < 0)
					continue;
				if (n & 1)
					data[n>>1]|=h;
				else
					data[n>>1]=h<<4;
				++n;
				if (n >= sizeof(data)*2)
					break;
			}
			n >>= 1;
			if (!n)
				continue;
			// hack - Ignore USB descriptor
			if (data[0] == 0x12)
				continue;
			if (debug) {
				for (unsigned int i = 0; i < n; ++i) {
					printf("%02X",data[i]);
				}
				printf("\n");
			}
			if (zwave) {
				decode_z(&zwave_hid_mode, data);
			} else {
				decode(data, proto);
			}
		}
	}
	infile.close();
	return 0;
}

