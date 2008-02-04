
/*
    Copyright 2007 Kevin Timmerman

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
*/


#include "objbase.h"
#include "setupapi.h"


typedef WINSETUPAPI BOOL WINAPI TSetupDiDestroyDeviceInfoList(IN HDEVINFO DeviceInfoSet);

typedef WINSETUPAPI BOOL WINAPI TSetupDiEnumDeviceInterfaces(
    IN  HDEVINFO                  DeviceInfoSet,
    IN  PSP_DEVINFO_DATA          DeviceInfoData,     OPTIONAL
    IN  LPGUID                    InterfaceClassGuid,
    IN  DWORD                     MemberIndex,
    OUT PSP_DEVICE_INTERFACE_DATA DeviceInterfaceData);

typedef WINSETUPAPI HDEVINFO WINAPI TSetupDiGetClassDevsA(
    IN LPGUID ClassGuid,  OPTIONAL
    IN PCSTR  Enumerator, OPTIONAL
    IN HWND   hwndParent, OPTIONAL
    IN DWORD  Flags);

typedef WINSETUPAPI HDEVINFO WINAPI TSetupDiGetClassDevsW(
    IN LPGUID ClassGuid,  OPTIONAL
    IN PCWSTR Enumerator, OPTIONAL
    IN HWND   hwndParent, OPTIONAL
    IN DWORD  Flags);

typedef WINSETUPAPI BOOL WINAPI TSetupDiGetDeviceInterfaceDetailA(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_A DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL);

typedef WINSETUPAPI BOOL WINAPI TSetupDiGetDeviceInterfaceDetailW(
    IN  HDEVINFO                           DeviceInfoSet,
    IN  PSP_DEVICE_INTERFACE_DATA          DeviceInterfaceData,
    OUT PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData,     OPTIONAL
    IN  DWORD                              DeviceInterfaceDetailDataSize,
    OUT PDWORD                             RequiredSize,                  OPTIONAL
    OUT PSP_DEVINFO_DATA                   DeviceInfoData                 OPTIONAL);


// HID - hidsdi.h

typedef unsigned short int USAGE;
typedef struct _HIDP_PREPARSED_DATA * PHIDP_PREPARSED_DATA;
typedef unsigned int NTSTATUS;

typedef struct _HIDD_ATTRIBUTES {
  ULONG  Size;
  USHORT  VendorID;
  USHORT  ProductID;
  USHORT  VersionNumber;
} HIDD_ATTRIBUTES, *PHIDD_ATTRIBUTES;

typedef struct _HIDP_CAPS {
  USAGE  Usage;
  USAGE  UsagePage;
  USHORT  InputReportByteLength;
  USHORT  OutputReportByteLength;
  USHORT  FeatureReportByteLength;
  USHORT  Reserved[17];
  USHORT  NumberLinkCollectionNodes;
  USHORT  NumberInputButtonCaps;
  USHORT  NumberInputValueCaps;
  USHORT  NumberInputDataIndices;
  USHORT  NumberOutputButtonCaps;
  USHORT  NumberOutputValueCaps;
  USHORT  NumberOutputDataIndices;
  USHORT  NumberFeatureButtonCaps;
  USHORT  NumberFeatureValueCaps;
  USHORT  NumberFeatureDataIndices;
} HIDP_CAPS, *PHIDP_CAPS;

typedef VOID __stdcall THidD_GetHidGuid(OUT LPGUID  HidGuid);
typedef BOOLEAN __stdcall THidD_GetAttributes(IN HANDLE  HidDeviceObject, OUT PHIDD_ATTRIBUTES  Attributes);
typedef BOOLEAN __stdcall THidD_GetManufacturerString(IN HANDLE  HidDeviceObject, OUT PVOID  Buffer, IN ULONG  BufferLength);
typedef BOOLEAN __stdcall THidD_GetProductString(IN HANDLE  HidDeviceObject, OUT PVOID  Buffer, IN ULONG  BufferLength);
typedef BOOLEAN __stdcall THidD_GetSerialNumberString(IN HANDLE  HidDeviceObject, OUT PVOID  Buffer, IN ULONG  BufferLength);
typedef BOOLEAN __stdcall THidD_GetIndexedString(IN HANDLE  HidDeviceObject, IN ULONG  StringIndex, OUT PVOID  Buffer, IN ULONG  BufferLength) ;
typedef BOOLEAN __stdcall THidD_GetPreparsedData(IN HANDLE  HidDeviceObject, OUT PHIDP_PREPARSED_DATA  *PreparsedData);
typedef BOOLEAN __stdcall THidD_FreePreparsedData(IN PHIDP_PREPARSED_DATA  PreparsedData);
typedef NTSTATUS __stdcall THidP_GetCaps(IN PHIDP_PREPARSED_DATA  PreparsedData, OUT PHIDP_CAPS  Capabilities);
typedef BOOLEAN __stdcall THidD_GetFeature(IN HANDLE  HidDeviceObject, OUT PVOID  ReportBuffer, IN ULONG  ReportBufferLength);
typedef BOOLEAN __stdcall THidD_SetFeature(IN HANDLE  HidDeviceObject, IN PVOID  ReportBuffer, IN ULONG  ReportBufferLength);

/*
 * TCancelIo is provided in winbase.h which isn't availabe in *really*
 * old versions of VisualStudio (like 4.2), so do some funky tricks to
 * grab it. See the bits with the same #if in usb_rtl.cpp
 */
#if _MSC_VER <= 1020
typedef WINBASEAPI BOOL WINAPI TCancelIo(IN HANDLE hFile);
#endif

extern TSetupDiDestroyDeviceInfoList* rtlSetupDiDestroyDeviceInfoList;		// 2x
extern TSetupDiEnumDeviceInterfaces* rtlSetupDiEnumDeviceInterfaces;		// 1x
extern TSetupDiGetClassDevsA* rtlSetupDiGetClassDevs;						// (A) 1x
extern TSetupDiGetDeviceInterfaceDetailA* rtlSetupDiGetDeviceInterfaceDetail; // (A) 2x

extern THidD_GetHidGuid* HidD_GetHidGuid;
extern THidD_GetAttributes* HidD_GetAttributes;
extern THidD_GetManufacturerString* HidD_GetManufacturerString;
extern THidD_GetProductString* HidD_GetProductString;
extern THidD_GetSerialNumberString* HidD_GetSerialNumberString;
extern THidD_GetIndexedString* HidD_GetIndexedString;
extern THidD_GetPreparsedData* HidD_GetPreparsedData;
extern THidD_FreePreparsedData* HidD_FreePreparsedData;
extern THidP_GetCaps* HidP_GetCaps;
extern THidD_GetFeature* HidD_GetFeature;
extern THidD_SetFeature* HidD_SetFeature;

#if _MSC_VER <= 1020
extern TCancelIo* CancelIo;
#endif

int LinkUSB(void);
void UnlinkUSB(void);
