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
 */

#ifndef XML_HEADERS_H
#define XML_HEADERS_H

const char *config_header="\
<?xml version=\"1.0\"?>\r\n\
<INFORMATION>\r\n\
	<USERMESSAGES>\r\n\
		<USERMESSAGE>\r\n\
			<VERSIONS>\r\n\
				<VERSION>\r\n\
					<PROTOCOL>%i</PROTOCOL>\r\n\
					<SKIN>%i</SKIN>\r\n\
					<FLASH>0x%02X:0x%02X</FLASH>\r\n\
					<BOARD>%i.%i.0</BOARD>\r\n\
					<SOFTWARETYPE>%i</SOFTWARETYPE>\r\n\
				</VERSION>\r\n\
			</VERSIONS>\r\n\
			<TYPE>DoNothing</TYPE>\r\n\
			<ABORTPROCESSING>True</ABORTPROCESSING>\r\n\
		</USERMESSAGE>\r\n\
		<USERMESSAGE>\r\n\
			<VERSIONS>\r\n\
				<VERSION></VERSION>\r\n\
			</VERSIONS>\r\n\
			<TYPE>Warning</TYPE>\r\n\
			<TEXT>This configuration file is not compatible with your Harmony Remote.</TEXT>\r\n\
		</USERMESSAGE>\r\n\
		<USERMESSAGE>\r\n\
			<VERSIONS>\r\n\
				<VERSION></VERSION>\r\n\
			</VERSIONS>\r\n\
			<TYPE>Abort</TYPE>\r\n\
		</USERMESSAGE>\r\n\
	</USERMESSAGES>\r\n\
	<INTENDEDVERSION>\r\n\
		<PROTOCOL>%i</PROTOCOL>\r\n\
		<SKIN>%i</SKIN>\r\n\
		<FLASH>0x%02X:0x%02X</FLASH>\r\n\
		<BOARD>%i.%i.0</BOARD>\r\n\
		<SOFTWARETYPE>%i</SOFTWARETYPE>\r\n\
	</INTENDEDVERSION>\r\n\
	<TRANSFERTYPE>Winsock</TRANSFERTYPE>\r\n\
	<CONFIGURATION>\r\n\
		<CONFIGOPTION>\r\n\
			<KEY>InstructionNoHarmony</KEY>\r\n\
			<TITLE>No Harmony Detected</TITLE>\r\n\
			<IMAGE>0</IMAGE>\r\n\
			<VALUE>Plug the USB cable into your Harmony, or press any button on your Harmony to wake it up.</VALUE>\r\n\
		</CONFIGOPTION>\r\n\
	</CONFIGURATION>\r\n"

"	<POSTOPTIONS>\r\n\
		<SERVER>members.harmonyremote.com</SERVER>\r\n\
		<PATH>EasyZapper/New/ProcUpdate/Receive_Zaps.asp</PATH>\r\n\
		<TIMEOUT>60000</TIMEOUT>\r\n\
		<HEADERS>\r\n\
			<HEADER>\r\n\
				<KEY>Cookie</KEY>\r\n\
				<VALUE>Monster</VALUE>\r\n\
			</HEADER>\r\n\
		</HEADERS>\r\n\
		<PARAMETERS>\r\n\
			<PARAMETER>\r\n\
				<KEY>UserId</KEY>\r\n\
				<VALUE>0</VALUE>\r\n\
			</PARAMETER>\r\n\
		</PARAMETERS>\r\n\
	</POSTOPTIONS>\r\n"

"	<TIPPOSTOPTIONS>\r\n\
		<SERVER>members.harmonyremote.com</SERVER>\r\n\
		<TIMEOUT>60000</TIMEOUT>\r\n\
		<HEADERS>\r\n\
			<HEADER>\r\n\
				<KEY>Cookie</KEY>\r\n\
				<VALUE>Monster</VALUE>\r\n\
			</HEADER>\r\n\
		</HEADERS>\r\n\
	</TIPPOSTOPTIONS>\r\n"
	
	
"	<TIMEPOSTOPTIONS>\r\n\
		<SERVER>members.harmonyremote.com</SERVER>\r\n\
		<PATH>EasyZapper/GetTime.asp</PATH>\r\n\
		<TIMEOUT>60000</TIMEOUT>\r\n\
	</TIMEPOSTOPTIONS>\r\n"

"	<COMPLETEPOSTOPTIONS>\r\n\
		<SERVER>members.harmonyremote.com</SERVER>\r\n\
		<PATH>EasyZapper/New/ProcUpdate/Receive_Complete.asp</PATH>\r\n\
		<TIMEOUT>60000</TIMEOUT>\r\n\
		<HEADERS>\r\n\
			<HEADER>\r\n\
				<KEY>Cookie</KEY>\r\n\
				<VALUE>Monster</VALUE>\r\n\
			</HEADER>\r\n\
		</HEADERS>\r\n\
		<PARAMETERS>\r\n\
			<PARAMETER>\r\n\
				<KEY>UserId</KEY>\r\n\
				<VALUE>0</VALUE>\r\n\
			</PARAMETER>\r\n\
		</PARAMETERS>\r\n\
	</COMPLETEPOSTOPTIONS>\r\n"


"	<BINARYDATASIZE>%i</BINARYDATASIZE>\r\n\
	<CHECKSUM>%i</CHECKSUM>\r\n\
</INFORMATION>\r\n";


const char *post_header="\
POST /%s HTTP/1.1\r\n\
User-Agent: HarmonyBrowser/7.3.0 (Build 15; UpdatedFrom 7.3.0.15; Skin logitech; Windows XP 5.1; x86; en; rv: 1.8.0.2) Gecko/20060125\r\n\
Content-Type:  application/x-www-form-urlencoded\r\n\
Host: %s\r\n\
Cookie: %s\r\n\
Content-Length: %i\r\n\r\n";

const char *post_xml="\
<EASYZAPPERDATA>\
<DATAVALUES/>\
<EVENTS/>\
<VERSIONINFORMATION>\
<VERSION>\
<CLIENTSOFTWARE>7.3.0</CLIENTSOFTWARE>\
<CLIENTSOFTWARETYPE>Windows XP (x86.1):Java</CLIENTSOFTWARETYPE>\
<SOFTWARE>%i.%i</SOFTWARE>\
<SOFTWARETYPE>%i</SOFTWARETYPE>\
<ID>%s</ID>\
<BOARD>%i.%i.0</BOARD>\
<FLASH>0x%02X:0x%02X</FLASH>\
<PROTOCOL>%i</PROTOCOL>\
<ARCHITECTURE>%i</ARCHITECTURE>\
<SKIN>%i</SKIN>\
<REGIONS/>\
</VERSION>\
</VERSIONINFORMATION>\
</EASYZAPPERDATA>\r\n";

#endif
