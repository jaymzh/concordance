/*
 *  vi: formatoptions+=tc textwidth=80 tabstop=8 shiftwidth=8 noexpandtab:
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
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
 *  (C) Copyright Kevin Timmerman 2007
 */

#include "libconcord.h"
#include "concordance.h"
#include <string.h>
#include <errno.h>

#ifdef WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#define closesocket close
#define SOCKET int
#define SOCKET_ERROR -1
#endif

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

static SOCKET sock = SOCKET_ERROR;

const char * const remote_ip_address = "169.254.1.2";
const u_short remote_port            = 3074;

int InitializeUsbLan(void)
{
	return 0;
}

int ShutdownUsbLan(void)
{
	int err=0;

	// Close the socket
	if (sock != SOCKET_ERROR) {
		if ((err = closesocket(sock))) {
			report_net_error("closesocket()");
			return LC_ERROR_OS_NET;
		}
	}

	return 0;
}

int FindUsbLanRemote(void)
{
	int err;

	hostent* addr = gethostbyname(remote_ip_address);

	if (!addr) {
		report_net_error("gethostbyname()");
		return LC_ERROR_OS_NET;
	}

	sockaddr_in sa;
	memcpy(&(sa.sin_addr), addr->h_addr, addr->h_length);
	sa.sin_family = AF_INET;		// TCP/IP
	sa.sin_port = htons(remote_port);	// Port 3074

	sock = socket(sa.sin_family, SOCK_STREAM, 0);	// TCP
	//sock = socket(sa.sin_family, SOCK_DGRAM, 0);	// UDP

	if ((err = connect(sock,(struct sockaddr*)&sa,sizeof(sa)))) {
		report_net_error("connect()");
		return LC_ERROR_OS_NET;
	}

	debug("Connected to USB LAN driver!");

	return 0;
}

int UsbLan_Write(unsigned int len, uint8_t *data)
{
	int err = send(sock, reinterpret_cast<char*>(data), len, 0);

	if (err == SOCKET_ERROR) {
		report_net_error("send()");
		return LC_ERROR_OS_NET;
	}

	debug("%i bytes sent", err);

	return 0;
}


int UsbLan_Read(unsigned int &len, uint8_t *data)
{
	int err = recv(sock, reinterpret_cast<char*>(data), len, 0);

	if (err == SOCKET_ERROR) {
		report_net_error("recv()");
		len = 0;
		return LC_ERROR_OS_NET;
	} 

	len = static_cast<unsigned int>(err);
	debug("%i bytes received", len);

	return 0;
}
