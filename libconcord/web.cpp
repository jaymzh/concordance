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
 *  (C) Copyright Kevin Timmerman 2007
 */

#include "libconcord.h"
#include "concordance.h"
#include "hid.h"
#include "remote.h"
#include "xml_headers.h"

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

static const uint8_t urlencodemap[32]={
	0xFF, 0xFF, 0xFF, 0xFF,	//   0 Control chars
	0x7D,					//  32 & % $ # " <space>
	0x98,					//  40 / , +
	0x00,					//  48
	0xFC,					//  56 ? > = < ; :
	0x01,					//  64 @
	0x00,					//  72
	0x00,					//  80
	0x78,					//  88 ^ ] \ [
	0x01,					//  96 '
	0x00,					// 104
	0x00,					// 112
	0x78,					// 120 ~ } | {
	0xFF, 0xFF, 0xFF, 0xFF, // 128
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF
};

static void UrlEncode(const char *in, string &out)
{
	out = "";
	const size_t len = strlen(in);
	for(size_t i = 0; i < len; ++i) {
		const char c = in[i];
		if(urlencodemap[c>>3] & (1 << (c & 7))) {
			char hex[4];
			sprintf(hex, "%%%02X", c);
			out += hex;
		} else
			out += c;
	}
}

static int Zap(string &server, const char *s1, const char *s2)
{
	int err;

	// Get the numeric address of host
	hostent* addr = gethostbyname(server.c_str());
	if (!addr) {
		report_net_error("gethostbyname()");
		return LH_ERROR_OS_NET;
	}

	// Fill in the sockaddr structure
	sockaddr_in sa;
	memcpy(&(sa.sin_addr), addr->h_addr, addr->h_length);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(80);

	// Create a socket
	// SOCK_STREAM = Stream (TCP) socket
	SOCKET sock = socket(sa.sin_family, SOCK_STREAM, 0);

	// Connect
	if ((err = connect(sock,(struct sockaddr*)&sa, sizeof(sa)))) {
		report_net_error("connect()");
		return LH_ERROR_OS_NET;
	}

#ifdef _DEBUG
	printf("Connected!\n");
#endif

	err = send(sock, s1, strlen(s1), 0);
	if (err == SOCKET_ERROR) {
		report_net_error("send()");
		return LH_ERROR_OS_NET;
	}

#ifdef _DEBUG
		printf("%i bytes sent\n", err);
#endif

	err = send(sock, s2, strlen(s2), 0);
	if (err == SOCKET_ERROR) {
		report_net_error("send()");
		return LH_ERROR_OS_NET;
	}

#ifdef _DEBUG
		printf("%i bytes sent\n", err);
#endif

	char buf[1000];
	err = recv(sock,buf,999,0);

	if (err == SOCKET_ERROR) {
		report_net_error("recv()");
		return LH_ERROR_OS_NET;
	}

	// Show the received received data
	buf[err]=0;
#if _DEBUG
	printf("Received: %s\n", buf);
#endif

	// Close the socket
	if ((err = closesocket(sock))) {
		report_net_error("closesocket()");
		return LH_ERROR_OS_NET;
	}

#if _DEBUG
	printf(" done\n");
#endif
	
	return 0;
}

int GetTag(const char *find, uint8_t*& pc, string *s=NULL)
{
	const size_t len = strlen(find);
	do {
		/*
		 * Advance pointer until beginning of a tag, and then
		 * one more.
		 */
		while(*pc != '<' && *pc++);

		// Make sure we still have a valid pointer
		if (!pc)
			return -1;

		// Check to see if this is the tag we want
		if (*++pc == *find && pc[len]=='>'
		   && !strnicmp(find, reinterpret_cast<const char*>(pc), len)) {
			pc += strlen(find) + 1;

			/*
 			 * If a string pointer was passed in, then add the
 			 * contents of this entire tag to the string
 			 */
			if (s) {
				const uint8_t *p=pc;
				*s = "";
				/*
				 * Here we keep adding chars until the next tag
				 * which, in theory, should be the end-tag.
				 */
				while (*p != '<' && *p) {
					*s+=*p;
					++p;
				}
			}
			return 0;
		}
		/*
		 * Advance until we get to the closing bracket of the end-tag
		 * we just found, and advance on more.
		 */
		while (*pc != '>' && *pc++);
	/*
	 * If we found it, we'd have returned above, so keep looping until
	 * we do.
	 */
	} while(*pc++);

	// GACK! We didn't find it.
	return -1;
}

int Post(uint8_t *xml, const char *root, TRemoteInfo &ri,
	string *learn_seq = NULL, string *learn_key = NULL)
{

	uint8_t *x = xml;
	int err;
	if ((err = GetTag(root,x)))
		return err;

	string server, path, cookie, userid;

	if ((err = GetTag("SERVER", x, &server)))
		return err;
	if ((err = GetTag("PATH", x, &path)))
		return err;
	if ((err = GetTag("VALUE", x, &cookie)))
		return err;
	if ((err = GetTag("VALUE", x, &userid)))
		return err;

#if _DEBUG
	printf("Connecting to %s:", server.c_str());
#endif

#ifdef _DEBUG
		printf("\nPath: %s\n", path.c_str());
		printf("Cookie: %s\n", cookie.c_str());
		printf("UserId: %s\n", userid.c_str());
#endif

	string post;
	if(learn_seq == NULL) {
		char serial[144];
		sprintf(serial, "%s%s%s", ri.serial1, ri.serial1, ri.serial2);
		char post_data[2000];
		sprintf(post_data, post_xml,
			ri.fw_ver_major, ri.fw_ver_minor, ri.fw_type,
			serial, ri.hw_ver_major, ri.hw_ver_minor,
			ri.flash_mfg, ri.flash_id, ri.protocol,
			ri.architecture, ri.skin);
		//printf("\n%s\n",post_data);

		string post_data_encoded;
		UrlEncode(post_data, post_data_encoded);

		post = "Data=" + post_data_encoded + "&UserId=" + userid;
	} else {
		post = "IrSequence=" + *learn_seq + "&UserId=" + userid
			+ "&KeyName=" + *learn_key;
	}
#ifdef _DEBUG
	printf("\n%s\n", post.c_str());
#endif

	char http_header[1000];
	sprintf(http_header, post_header, path.c_str(), server.c_str(),
		cookie.c_str(), post.length());
#ifdef _DEBUG
	printf("\n%s\n", http_header);
#endif

	return Zap(server, http_header,post.c_str());
}

