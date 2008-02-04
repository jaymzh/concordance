/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General License for more details.
 *
 *  You should have received a copy of the GNU General License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  (C) Copyright Kevin Timmerman 2007
 */

#include "harmony.h"

#ifdef WIN32
#include <winsock.h>
#else
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>
#define closesocket close
#define SOCKET int
#define SOCKET_ERROR -1
#endif

static SOCKET sock = SOCKET_ERROR;

const char * const harmony_ip_address = "169.254.1.2";
const u_short harmony_port            = 3074;

int InitializeUsbLan(void)
{
	return 0;
}

int ShutdownUsbLan(void)
{
	int err=0;

	// Close the socket
	if (sock!=SOCKET_ERROR) {
		if ((err = closesocket(sock))) {
#ifdef WIN32
			err = WSAGetLastError();
			printf("Close Error: %i\n",err);
#else
			printf("Close Error: %s\n",strerror(errno));
#endif
		}
	}

	return err;
}

int FindUsbLanRemote(void)
{
	int err;

	hostent* addr = gethostbyname(harmony_ip_address);

	if (!addr) {
#ifdef WIN32
		err = WSAGetLastError();
		printf("gethostbyname() Error: %i\n", err);
		return err;
#else
		printf("gethostbyname() Error: %s\n", strerror(errno));
		return -1;
#endif
	}

	sockaddr_in sa;
	memcpy(&(sa.sin_addr), addr->h_addr, addr->h_length);
	sa.sin_family = AF_INET;		// TCP/IP
	sa.sin_port = htons(harmony_port);	// Port 3074

	sock = socket( sa.sin_family, SOCK_STREAM, 0 );	// TCP
	//sock=socket( sa.sin_family, SOCK_DGRAM, 0 );	// UDP

	if ((err = connect(sock, (struct sockaddr*) &sa, sizeof(sa)))) {
#ifdef WIN32
		err = WSAGetLastError();
		printf("Connect Error: %i\n", err);
#else
		printf("Connect Error: %s\n", strerror(errno));
#endif
		return err;
	}

	printf("\nConnected to USB LAN driver!\n\n");

	return err;
}

int UsbLan_Write(unsigned int len, uint8_t *data)
{
	int err = send(sock, reinterpret_cast<char*>(data), len, 0);

	if (err == SOCKET_ERROR) {
#ifdef WIN32
		err = WSAGetLastError();
		printf("send() failed with error %i\n", err);
#else
		printf("send() error: %s\n", strerror(errno));
#endif
		return err;
	}
	printf("%i bytes sent\n", err);

	return err;
}


int UsbLan_Read(unsigned int &len, uint8_t *data)
{
	int err = recv(sock, reinterpret_cast<char*>(data), len, 0);

	if (err == SOCKET_ERROR) {
#ifdef WIN32
		err = WSAGetLastError();
		printf("recv() failed with error %i\n", err);
#else
		printf("recv() error: %s\n", strerror(errno));
#endif
		len = 0;
	} else {
		len = static_cast<unsigned int>(err);
		printf("%i bytes received\n", len);
		err = 0;
	}

	return err;
}

