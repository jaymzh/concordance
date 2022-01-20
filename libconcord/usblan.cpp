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
 * (C) Copyright Kevin Timmerman 2007
 * (C) Copyright Kevin Timmerman 2008
 */

#include "usblan.h"

#include <string.h>
#include <errno.h>
#include <fcntl.h>

#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#define closesocket close
#define SOCKET int
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif

#ifdef __FreeBSD__
#include <netinet/in.h>
#endif

#include "libconcord.h"
#include "lc_internal.h"

static SOCKET sock = INVALID_SOCKET;

const char * const remote_ip_address = "169.254.1.2";
const uint16_t remote_port = 3074;
const int connect_timeout = 1; // try to connect for 1 seconds

const char * const http_get_cmd = "\
GET /xmluserrfsetting HTTP/1.1\r\n\
User-Agent: Jakarta Commons-HttpClient/3.1\r\n\
Host: 169.254.1.2\r\n\
\r\n";

int InitializeUsbLan(void)
{
    return 0;
}

int ShutdownUsbLan(void)
{
    int err=0;

    // Close the socket
    if (sock != INVALID_SOCKET) {
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
    sa.sin_family = AF_INET;        // TCP/IP
    sa.sin_port = htons(remote_port);    // Port 3074

    sock = socket(sa.sin_family, SOCK_STREAM, 0);    // TCP
    //sock = socket(sa.sin_family, SOCK_DGRAM, 0);    // UDP

    // Make the socket non-blocking so it doesn't hang on systems that 
    // don't have a usbnet remote.
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(sock, &wset);
    struct timeval tv;
    tv.tv_sec = connect_timeout;
    tv.tv_usec = 0;
#ifdef _WIN32
    u_long non_blocking = 1;
    if(ioctlsocket(sock, FIONBIO, &non_blocking) != 0) {
        report_net_error("ioctlsocket()");
        return LC_ERROR_OS_NET;
    }
#else
    int flags = 0;
    if((flags = fcntl(sock, F_GETFL, 0)) < 0) {
        report_net_error("fcntl()");
        return LC_ERROR_OS_NET;
    }
    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
        report_net_error("fcntl()");
        return LC_ERROR_OS_NET;
    }
#endif

    if ((err = connect(sock,(struct sockaddr*)&sa,sizeof(sa)))) {
#ifdef _WIN32
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
        if (errno != EINPROGRESS) {
#endif
            report_net_error("connect()");
            return LC_ERROR_OS_NET;
        }
    }

    if ((err = select(sock+1, NULL, &wset, NULL, &tv)) <= 0) {
        report_net_error("select()");
        return LC_ERROR_OS_NET;
    }

    // Change the socket back to blocking which should be fine now that we
    // connected.
#ifdef _WIN32
    non_blocking = 0;
    if(ioctlsocket(sock, FIONBIO, &non_blocking) != 0) {
        report_net_error("ioctlsocket()");
        return LC_ERROR_OS_NET;
    }
#else
    if((flags = fcntl(sock, F_GETFL, 0)) < 0) {
        report_net_error("fcntl()");
        return LC_ERROR_OS_NET;
    }
    if(fcntl(sock, F_SETFL, flags & ~O_NONBLOCK) < 0) {
        report_net_error("fcntl()");
        return LC_ERROR_OS_NET;
    }
#endif

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

int GetXMLUserRFSetting(char **data)
{
    int err;
    int web_sock;
    char buf[4096];

    hostent* addr = gethostbyname(remote_ip_address);

    if (!addr) {
        report_net_error("gethostbyname()");
        return LC_ERROR_OS_NET;
    }

    sockaddr_in sa;
    memcpy(&(sa.sin_addr), addr->h_addr, addr->h_length);
    sa.sin_family = AF_INET;        // TCP/IP
    sa.sin_port = htons(80);                // Web Server port

    web_sock = socket(sa.sin_family, SOCK_STREAM, 0);    // TCP

    if ((err = connect(web_sock,(struct sockaddr*)&sa,sizeof(sa)))) {
        report_net_error("connect()");
        return LC_ERROR_OS_NET;
    }
    debug("Connected to USB LAN web server!");

    err = send(web_sock, http_get_cmd, strlen(http_get_cmd), 0);
    if (err == SOCKET_ERROR) {
        report_net_error("send()");
        return LC_ERROR_OS_NET;
    }
    debug("%i bytes sent", err);

    unsigned int len = 0;
    char* buf_ptr = buf;
    do {
        err = recv(web_sock, buf_ptr, sizeof(buf)-len, 0);
        if (err == SOCKET_ERROR) {
            report_net_error("recv()");
            len = 0;
            return LC_ERROR_OS_NET;
        }
        len += err;
        buf_ptr += err;
        debug("%i bytes received", err);
    } while (err > 0); // recv will return 0 when the message is done.
    buf[len] = '\0';

    buf_ptr = strstr(buf, "\r\n\r\n"); // search for end of the http header
    if (buf_ptr == NULL) {
        report_net_error("strstr()");
        return LC_ERROR_OS_NET;
    }
    buf_ptr += 4;
    *data = new char[strlen(buf_ptr)+1];
    strncpy(*data, buf_ptr, strlen(buf_ptr)+1);

    return 0;
}
