/*
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

#ifdef WINHID

#include "../libconcord.h"
#include "../concordance.h"

#define TRACE0
#define TRACE1
#define TRACE2

#include <string>

#include "../hid.h"
#include "usb_rtl.h"


OVERLAPPED ol;
HIDP_CAPS caps;
HANDLE h_hid=NULL;

int InitUSB()
{
	debug("Using Windows HID stack");

	ol.Offset=ol.OffsetHigh=0;
	ol.hEvent=CreateEvent(NULL,FALSE,FALSE,NULL);

	return LinkUSB();
}

void ShutdownUSB()
{
	if(h_hid) {
		CloseHandle(h_hid);
		h_hid=NULL;
	}

	UnlinkUSB();

	CloseHandle(ol.hEvent);
	ol.hEvent=NULL;
}

int FindRemote(THIDINFO &hid_info)
{
	if(h_hid) { CloseHandle(h_hid); h_hid=NULL; }

	GUID guid;
	HidD_GetHidGuid(&guid);

	const HDEVINFO HardwareDeviceInfo = rtlSetupDiGetClassDevs (
		&guid,
		NULL,						// Define no enumerator (global)
		NULL,						// Define no parent window
		(DIGCF_PRESENT |			// Only Devices present
		DIGCF_DEVICEINTERFACE));	// Function class devices.

	/*
		SetupDiEnumDeviceInterfaces() returns information about device interfaces
		exposed by one or more devices. Each call returns information about one interface;
		the routine can be called repeatedly to get information about several interfaces
		exposed by one or more devices.
	*/
	if(HardwareDeviceInfo!=INVALID_HANDLE_VALUE) {
		SP_DEVICE_INTERFACE_DATA DeviceInfoData;
		DeviceInfoData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);
		int i=0;
		while(rtlSetupDiEnumDeviceInterfaces(HardwareDeviceInfo, 0, &guid, i, &DeviceInfoData)) {

			TRACE1("\n\nInterface # %i\n",i);

			ULONG device_data_length = 0;
			rtlSetupDiGetDeviceInterfaceDetail(
				HardwareDeviceInfo,
				&DeviceInfoData,
				NULL,	// probing so no output buffer yet
				0,		// probing so output buffer length of zero
				&device_data_length,
				NULL); // not interested in the specific dev-node

			PSP_DEVICE_INTERFACE_DETAIL_DATA functionClassDeviceData=
				(PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(device_data_length);
			if(functionClassDeviceData==NULL) return FALSE;
			functionClassDeviceData->cbSize=sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);	// This is correct - strange but true

			ULONG required_length=0;

			if(!rtlSetupDiGetDeviceInterfaceDetail(HardwareDeviceInfo, &DeviceInfoData, functionClassDeviceData,
				device_data_length, &required_length, NULL) || (device_data_length < required_length)) {

				TRACE0("Call to SetupDiGetDeviceInterfaceDetail failed!\n");

				free(functionClassDeviceData);

				return -1;
			}

			TRACE1("Attempting to open %s\n", functionClassDeviceData->DevicePath);

			const HANDLE h = CreateFile(
				functionClassDeviceData->DevicePath,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			if(h==INVALID_HANDLE_VALUE) {
				TRACE1( "FAILED to open %s\n", functionClassDeviceData->DevicePath);
			} else {
				HIDD_ATTRIBUTES attr;
				HidD_GetAttributes(h,&attr);
				TRACE1("            Vendor ID: %04X\n",attr.VendorID);
				TRACE1("           Product ID: %04X\n",attr.ProductID);
				TRACE1("       Version Number: %04X\n",attr.VersionNumber);

				if((attr.VendorID==0x046D && attr.ProductID>=0xC110 && attr.ProductID<=0xC14F) ||
						(attr.VendorID==0x0400 && attr.ProductID==0xC359)) {
					WCHAR s[127];
					char ts[128];

					HidD_GetManufacturerString(h, s, sizeof(s));
					WideCharToMultiByte(CP_ACP,0,s,-1,ts,sizeof(ts),NULL,NULL);
					TRACE1("  Manufacturer String: %s\n",ts);
					hid_info.mfg=ts;

					HidD_GetProductString(h, s, sizeof(s));
					WideCharToMultiByte(CP_ACP,0,s,-1,ts,sizeof(ts),NULL,NULL);
					TRACE1("       Product String: %s\n",ts);
					hid_info.prod=ts;

#if 0
					HidD_GetSerialNumberString(h, s, sizeof(s));
					WideCharToMultiByte(CP_ACP,0,s,-1,ts,sizeof(ts),NULL,NULL);
					TRACE1(" Serial Number String: %s\n",ts);

					for(int i=0;i<6;++i) {
						HidD_GetIndexedString(h,i,s,sizeof(s));
						WideCharToMultiByte(CP_ACP,0,s,-1,ts,sizeof(ts),NULL,NULL);
						TRACE2("       Index String %i: %s\n",i,ts);
					}
#endif

					PHIDP_PREPARSED_DATA ppd;
					HidD_GetPreparsedData(h,&ppd);
					UINT u=HidP_GetCaps(ppd,&caps);
					HidD_FreePreparsedData(ppd);

					TRACE1("  Input Report Length: %i\n",caps.InputReportByteLength);
					TRACE1(" Output Report Length: %i\n",caps.OutputReportByteLength);
					TRACE1("Feature Report Length: %i\n",caps.FeatureReportByteLength);
					TRACE1("  LinkCollectionNodes: %i\n",caps.NumberLinkCollectionNodes);
					TRACE1("      InputButtonCaps: %i\n",caps.NumberInputButtonCaps);
					TRACE1("       InputValueCaps: %i\n",caps.NumberInputValueCaps);
					TRACE1("     InputDataIndices: %i\n",caps.NumberInputDataIndices);
					TRACE1("     OutputButtonCaps: %i\n",caps.NumberOutputButtonCaps);
					TRACE1("      OutputValueCaps: %i\n",caps.NumberOutputValueCaps);
					TRACE1("    OutputDataIndices: %i\n",caps.NumberOutputDataIndices);
					TRACE1("    FeatureButtonCaps: %i\n",caps.NumberFeatureButtonCaps);
					TRACE1("     FeatureValueCaps: %i\n",caps.NumberFeatureValueCaps);
					TRACE1("   FeatureDataIndices: %i\n",caps.NumberFeatureDataIndices);

					CloseHandle(h);

					hid_info.vid=attr.VendorID;
					hid_info.pid=attr.ProductID;
					hid_info.ver=attr.VersionNumber;

					hid_info.irl=caps.InputReportByteLength;
					hid_info.orl=caps.OutputReportByteLength;
					hid_info.frl=caps.FeatureReportByteLength;

					h_hid = CreateFile(
						functionClassDeviceData->DevicePath,
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_FLAG_OVERLAPPED,
						NULL);

					free(functionClassDeviceData);

					return (h_hid==INVALID_HANDLE_VALUE)?-2:0;
				}

				CloseHandle(h);

			}

			free(functionClassDeviceData);

			++i;
		}

		TRACE1("Interface enum ended with error %u\n",GetLastError());

		// SetupDiDestroyDeviceInfoList() destroys a device information set and frees all associated memory.
		rtlSetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
	} else {
		TRACE0("Unable to get device handle\n");
	}

	return 1;
}

int HID_WriteReport(const uint8_t *data)
{
	DWORD err,dw;
	if (!WriteFile(h_hid,data,caps.OutputReportByteLength,&dw,&ol)) {
		err = GetLastError();
		if (err != ERROR_IO_PENDING) {
			debug("WriteFile() failed with error %i", err);
			return err;
		}
	}

	const DWORD ws=WaitForSingleObject(ol.hEvent,500);

	if (ws==WAIT_TIMEOUT) {
		debug("Write failed to complete within alloted time");
		CancelIo(h_hid);
		err=1;
	} else if(ws!=WAIT_OBJECT_0) {
		debug("Wait failed with code %i", ws);
		err=2;
	} else {
		err=0;
	}

	return err;
}

int HID_ReadReport(uint8_t *data, unsigned int timeout)
{
	DWORD err,dw;
	if(!ReadFile(h_hid,data,caps.InputReportByteLength,&dw,&ol)) {
		err=GetLastError();
		if(err!=ERROR_IO_PENDING) {
			debug("ReadFile() failed with error %i", err);
			return err;
		}
	}

	const DWORD ws=WaitForSingleObject(ol.hEvent,timeout);

	if(ws==WAIT_TIMEOUT) {
		debug("No response from remote");
		CancelIo(h_hid);
		err=1;
	} else if(ws!=WAIT_OBJECT_0) {
		debug("Wait failed with code %i", ws);
		err=2;
	} else {
		err=0;
	}
	return err;
}

#endif
