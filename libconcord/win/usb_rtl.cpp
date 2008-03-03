
/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

    Copyright 2007 Kevin Timmerman
*/

#ifdef WINHID

#include "usb_rtl.h"

HMODULE hKernel32=NULL;
HMODULE hSetupapi=NULL;
HMODULE hHid=NULL;

TSetupDiDestroyDeviceInfoList* rtlSetupDiDestroyDeviceInfoList;		// 2x
TSetupDiEnumDeviceInterfaces* rtlSetupDiEnumDeviceInterfaces;		// 1x
TSetupDiGetClassDevsA* rtlSetupDiGetClassDevs;						// (A) 1x
TSetupDiGetDeviceInterfaceDetailA* rtlSetupDiGetDeviceInterfaceDetail; // (A) 2x

THidD_GetHidGuid* HidD_GetHidGuid;
THidD_GetAttributes* HidD_GetAttributes;
THidD_GetManufacturerString* HidD_GetManufacturerString;
THidD_GetProductString* HidD_GetProductString;
THidD_GetSerialNumberString* HidD_GetSerialNumberString;
THidD_GetIndexedString* HidD_GetIndexedString;
THidD_GetPreparsedData* HidD_GetPreparsedData;
THidD_FreePreparsedData* HidD_FreePreparsedData;
THidP_GetCaps* HidP_GetCaps;
THidD_GetFeature* HidD_GetFeature;
THidD_SetFeature* HidD_SetFeature;

/* see note in usb_rtl.h */
#if _MSC_VER <= 1020
TCancelIo* CancelIo;
#endif


int LinkUSB(void)
{
/* see note in usb_rtl.h */
#if _MSC_VER <= 1020
	hKernel32=GetModuleHandle("kernel32.dll");

	if(hKernel32) {
		CancelIo=reinterpret_cast<TCancelIo*>(GetProcAddress(hKernel32,"CancelIo"));
	} else {
		return -5;
	}

	if(CancelIo==NULL)
		return -6;
#endif

	hSetupapi=LoadLibrary("setupapi.dll");

	if(hSetupapi) {
		rtlSetupDiDestroyDeviceInfoList=(TSetupDiDestroyDeviceInfoList*)GetProcAddress(hSetupapi,"SetupDiDestroyDeviceInfoList");
		rtlSetupDiEnumDeviceInterfaces=(TSetupDiEnumDeviceInterfaces*)GetProcAddress(hSetupapi,"SetupDiEnumDeviceInterfaces");
		rtlSetupDiGetClassDevs=(TSetupDiGetClassDevsA*)GetProcAddress(hSetupapi,"SetupDiGetClassDevsA");
		rtlSetupDiGetDeviceInterfaceDetail=(TSetupDiGetDeviceInterfaceDetailA*)GetProcAddress(hSetupapi,"SetupDiGetDeviceInterfaceDetailA");
	} else
		return -1;

	if(rtlSetupDiDestroyDeviceInfoList==NULL ||
		rtlSetupDiEnumDeviceInterfaces==NULL ||
		rtlSetupDiGetClassDevs==NULL ||
		rtlSetupDiGetDeviceInterfaceDetail==NULL)
		return -2;

	hHid=LoadLibrary("hid.dll");
	
	if(hHid) {
		HidD_GetHidGuid=(THidD_GetHidGuid*)GetProcAddress(hHid,"HidD_GetHidGuid");
		HidD_GetAttributes=(THidD_GetAttributes*)GetProcAddress(hHid,"HidD_GetAttributes");
		HidD_GetManufacturerString=(THidD_GetManufacturerString*)GetProcAddress(hHid,"HidD_GetManufacturerString");
		HidD_GetProductString=(THidD_GetProductString*)GetProcAddress(hHid,"HidD_GetProductString");
		HidD_GetSerialNumberString=(THidD_GetSerialNumberString*)GetProcAddress(hHid,"HidD_GetSerialNumberString");
		HidD_GetIndexedString=(THidD_GetIndexedString*)GetProcAddress(hHid,"HidD_GetIndexedString");
		HidD_GetPreparsedData=(THidD_GetPreparsedData*)GetProcAddress(hHid,"HidD_GetPreparsedData");
		HidD_FreePreparsedData=(THidD_FreePreparsedData*)GetProcAddress(hHid,"HidD_FreePreparsedData");
		HidP_GetCaps=(THidP_GetCaps*)GetProcAddress(hHid,"HidP_GetCaps");
		HidD_GetFeature=(THidD_GetFeature*)GetProcAddress(hHid,"HidD_GetFeature");
		HidD_SetFeature=(THidD_SetFeature*)GetProcAddress(hHid,"HidD_SetFeature");
	} else
		return -3;
	
	if(HidD_GetHidGuid==NULL ||
		HidD_GetAttributes==NULL ||
		HidD_GetManufacturerString==NULL ||
		HidD_GetProductString==NULL ||
		HidD_GetSerialNumberString==NULL ||
		HidD_GetIndexedString==NULL ||
		HidD_GetPreparsedData==NULL ||
		HidD_FreePreparsedData==NULL ||
		HidP_GetCaps==NULL ||
		HidD_GetFeature==NULL ||
		HidD_SetFeature==NULL)
		return -4;

	return 0;
}

void UnlinkUSB(void)
{
	if(hSetupapi) FreeLibrary(hSetupapi);
	if(hHid) FreeLibrary(hHid);
	if(hKernel32) FreeLibrary(hKernel32);
	hSetupapi=hHid=hKernel32=NULL;
}

#endif
