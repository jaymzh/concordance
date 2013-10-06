#!Nsis Installer Command Script
#
# This is an NSIS Installer Command Script generated automatically
# by the Fedora nsiswrapper program.  For more information see:
#
#   http://fedoraproject.org/wiki/MinGW
#
# To build an installer from the script you would normally do:
#
#   makensis this_script
#
# which will generate the output file 'installer.exe' which is a Windows
# installer containing your program.

Name "Concordance"
OutFile "concordance-installer.exe"
InstallDir "$ProgramFiles\Concordance"
InstallDirRegKey HKLM SOFTWARE\Concordance "Install_Dir"

ShowInstDetails hide
ShowUninstDetails hide

# Uncomment this to enable BZip2 compression, which results in
# slightly smaller files but uses more memory at install time.
#SetCompressor bzip2

XPStyle on

Page components
Page directory
Page instfiles

ComponentText "Select which optional components you want to install."

DirText "Please select the installation folder."

Section "Concordance"
  SectionIn RO

  SetOutPath "$INSTDIR"
  File "$%MINGW_SYSROOT_BIN%/libgcc_s_sjlj-1.dll"
  File "$%MINGW_SYSROOT_BIN%/libstdc++-6.dll"
  File "$%MINGW_SYSROOT_BIN%/libzip-2.dll"
  File "$%MINGW_SYSROOT_BIN%/zlib1.dll"
  File "$%HIDAPI_LIB_PATH%/libhidapi-0.dll"
  File "../../libconcord/.libs/libconcord-3.dll"
  File "../.libs/concordance.exe"
SectionEnd

Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\Concordance"
  CreateShortCut "$SMPROGRAMS\Concordance\Uninstall Concordance.lnk" "$INSTDIR\Uninstall Concordance.exe" "" "$INSTDIR\Uninstall Concordance.exe" 0
  CreateShortCut "$SMPROGRAMS\Concordance\concordance.exe.lnk" "$INSTDIR\concordance.exe" "" "$INSTDIR\concordance.exe" 0
SectionEnd

Section "Desktop Icons"
  CreateShortCut "$DESKTOP\concordance.exe.lnk" "$INSTDIR\concordance.exe" "" "$INSTDIR\concordance.exe" 0
SectionEnd

Section "Uninstall"
  Delete /rebootok "$DESKTOP\concordance.exe.lnk"
  Delete /rebootok "$SMPROGRAMS\Concordance\concordance.exe.lnk"
  Delete /rebootok "$SMPROGRAMS\Concordance\Uninstall Concordance.lnk"
  RMDir "$SMPROGRAMS\Concordance"

  Delete /rebootok "$INSTDIR\zlib1.dll"
  Delete /rebootok "$INSTDIR\libzip-2.dll"
  Delete /rebootok "$INSTDIR\libstdc++-6.dll"
  Delete /rebootok "$INSTDIR\libgcc_s_sjlj-1.dll"
  Delete /rebootok "$INSTDIR\concordance.exe"
  Delete /rebootok "$INSTDIR\libconcord-3.dll"
  Delete /rebootok "$INSTDIR\libhidapi-0.dll"
  RMDir "$INSTDIR"
SectionEnd

Section -post
  WriteUninstaller "$INSTDIR\Uninstall Concordance.exe"
SectionEnd
