# Microsoft Developer Studio Project File - Name="libconcord_winhid" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libconcord_winhid - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libconcord_winhid.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libconcord_winhid.mak" CFG="libconcord_winhid - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libconcord_winhid - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libconcord_winhid - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libconcord_winhid - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release/libconcord_winhid"
# PROP Intermediate_Dir "Release/libconcord_winhid"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCONCORD_WINHID_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCONCORD_WINHID_EXPORTS" /D "NOCRYPT" /D "WINHID" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"Release/libconcord_winhid/libconcord.dll"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copying libconcord.dll (Release/WinHID version)
PostBuild_Cmds=mkdir ..\..\concordance\win\Release	copy .\Release\libconcord_winhid\libconcord.lib ..\..\concordance\win\Release\libconcord.lib	copy .\Release\libconcord_winhid\libconcord.dll ..\..\concordance\win\Release\libconcord.dll
# End Special Build Tool

!ELSEIF  "$(CFG)" == "libconcord_winhid - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug/libconcord_winhid"
# PROP Intermediate_Dir "Debug/libconcord_winhid"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCONCORD_WINHID_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBCONCORD_WINHID_EXPORTS" /D "NOCRYPT" /D "WINHID" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 ws2_32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /out:"Debug/libconcord_winhid/libconcord.dll" /pdbtype:sept
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=copying libconcord.dll (Debug/WinHID version)
PostBuild_Cmds=mkdir ..\..\concordance\win\Debug	copy .\Debug\libconcord_winhid\libconcord.lib ..\..\concordance\win\Debug\libconcord.lib	copy .\Debug\libconcord_winhid\libconcord.dll ..\..\concordance\win\Debug\libconcord.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "libconcord_winhid - Win32 Release"
# Name "libconcord_winhid - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "WinHID"

# PROP Default_Filter ""
# Begin Source File

SOURCE=usb_rtl.cpp
# End Source File
# Begin Source File

SOURCE=winhid.cpp
# End Source File
# End Group
# Begin Group "module definitions"

# PROP Default_Filter ""
# Begin Source File

SOURCE=libconcord.def
# End Source File
# End Group
# Begin Source File

SOURCE=..\binaryfile.cpp
# End Source File
# Begin Source File

SOURCE=..\libconcord.cpp
# End Source File
# Begin Source File

SOURCE=..\remote.cpp
# End Source File
# Begin Source File

SOURCE=..\remote_z.cpp
# End Source File
# Begin Source File

SOURCE=..\usblan.cpp
# End Source File
# Begin Source File

SOURCE=..\web.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\binaryfile.h
# End Source File
# Begin Source File

SOURCE=..\lc_internal.h
# End Source File
# Begin Source File

SOURCE=..\hid.h
# End Source File
# Begin Source File

SOURCE=..\libconcord.h
# End Source File
# Begin Source File

SOURCE=..\protocol_z.h
# End Source File
# Begin Source File

SOURCE=..\remote.h
# End Source File
# Begin Source File

SOURCE=..\remote_info.h
# End Source File
# Begin Source File

SOURCE=usb_rtl.h
# End Source File
# Begin Source File

SOURCE=..\usblan.h
# End Source File
# Begin Source File

SOURCE=..\web.h
# End Source File
# Begin Source File

SOURCE=..\xml_headers.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
