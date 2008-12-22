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
#include <fstream>
#include <stdlib.h>
#ifdef WIN32
typedef unsigned char uint8_t;
#include "../concordance/win/getopt/getopt.h"
#else
#include <getopt.h>
#endif

using namespace std;

#include "../libconcord/protocol.h"

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

void print_data(const uint8_t * const data)
{
	if (!verbose)
		return;
	const uint8_t length = data[0] & LENGTH_MASK;
	printf("   DATA: ");
	for (int i = 0; i < length; i++) {
		printf("%i", data[i]);
	}
	printf("\n");
}

void decode(const uint8_t * const data)
{
	// Note: The uninteresting stuff is commented out
	const uint8_t length = data[0] & LENGTH_MASK;
	switch (data[0] & COMMAND_MASK) {
		case COMMAND_GET_VERSION & COMMAND_MASK:
			printf("Get   Version\n");
			break;
		case RESPONSE_VERSION_DATA & COMMAND_MASK:
			printf("Get   Version Response %02X %02X %02X %02X %02X %02X %02X\n",
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
			print_data(data);
			break;
		case COMMAND_READ_FLASH & COMMAND_MASK:
			printf("Read  Flash %06X %i\n",
				data[1]<<16|data[2]<<8|data[3],
				data[4]<<8|data[5]);
			break;
		case RESPONSE_READ_FLASH_DATA & COMMAND_MASK:
			printf("Read  Flash Data\n");
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

void help()
{
	printf("Usage: consnoop <options>\n\n");

	printf("Options:\n");
	printf("\t-v\tVerbose. Print bytes we write.\n");
	printf("\t-d\tDebug. Print full data for all decoded packets.\n");
	printf("\t-f <file>\tFilename to parse.\n");
	printf("\t-h\tThis help.\n\n");
}

int main(int argc, char *argv[])
{
	int tmpint = 0;
	char *file_name = NULL;
	while ((tmpint = getopt(argc, argv, "dhf:v")) != EOF) {
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
		}
	}
	printf("filename is %s", file_name);

	ifstream infile;
	infile.open(file_name);

	string s;
	string payloadbytes("<payloadbytes>");

	while(!infile.eof()) {
		getline(infile,s);
		if(!s.compare(0, payloadbytes.size(), payloadbytes)) {
			uint8_t data[260];
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
			decode(data);
		}
	}
	infile.close();
	return 0;
}

