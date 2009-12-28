; sr163.nsi
; Modified for version 1.63

;---------------------
;Header files

  !include "MUI.nsh"
  !include "InstallOptions.nsh"
  !include "FileFunc.nsh"
  !insertmacro GetParent
  !insertmacro DirState

;--------------------------------
; Script preprocessing
  !execute "zip -j srskin.zip skins\srskin.bmp skins\srskin.txt"
  !execute "zip -j srskin_winamp.zip skins\srskin_winamp.bmp skins\srskin_winamp.txt"
  !execute "zip -j srskin_XP.zip skins\srskin_XP.bmp skins\srskin_XP.txt"

;--------------------------------
;General

  ;Name and file
  Name "Streamripper for Windows and Winamp v1.65.0-alpha"
  OutFile "streamripper-windows-installer-1.65.0-alpha.exe"

  ;Default installation folder
  ;InstallDir "$PROGRAMFILES\Streamripper-1.65.0-alpha"
  InstallDir "$PROGRAMFILES\Streamripper"
  
  ;Get installation folder from registry if available
  ; InstallDirRegKey HKCU "Software\Streamripper" ""


;--------------------------------
;Pages

;  !insertmacro MUI_PAGE_LICENSE "${NSISDIR}\Docs\Modern UI\License.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  Page custom CustomPage
;  Page custom CustomPageB
;  !insertmacro MUI_PAGE_DIRECTORY
;  Page custom CustomPageC
  !insertmacro MUI_PAGE_INSTFILES
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Interface Settings

;  !define MUI_ABORTWARNING
  
;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Reserve Files
  
  ;If you are using solid compression, files that are required before
  ;the actual installation should be stored first in the data block,
  ;because this will make your installer start faster.
  
  ReserveFile "${NSISDIR}\Plugins\InstallOptions.dll"

  ReserveFile "sr163_directories.ini"

;--------------------------------
;Variables

  Var IniValue
  Var WADIR
  Var WADIR_TMP

;--------------------------------
;Installer Sections


Section "Winamp Plugin" sec_plugin

  SetOutPath "$INSTDIR"
  
  File "C:\program files\winamp\plugins\gen_sripper.dll"

  SetOutPath "$WADIR\Plugins"

  File "C:\program files\winamp\plugins\gen_sripper.dll"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "DisplayName" "Streamripper (Remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoRepair" 1
  
  ;Read a value from an InstallOptions INI file
  !insertmacro INSTALLOPTIONS_READ $IniValue "sr163_directories.ini" "Field 2" "State"
  
SectionEnd

Section "Console Application" sec_console

  SetOutPath "$INSTDIR"

  File "..\win32\Release\streamripper.exe"
  
  ;Store installation folder
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "DisplayName" "Streamripper (Remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoRepair" 1
  
SectionEnd

Section "GUI Application" sec_gui

  SetOutPath "$INSTDIR"
  File "..\win32\Release\wstreamripper.exe"

  SetOutPath $INSTDIR\Skins
  ;; We delete these .bmp's because user may be installing over 
  ;; an existing install
  Delete srskin.bmp
  Delete srskin_winamp.bmp
  Delete srskin_XP.bmp
  File srskin.zip
  File srskin_winamp.zip
  File srskin_XP.zip
  File "skins\srskin_BigBento.zip"

  ;Store installation folder
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "DisplayName" "Streamripper (Remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoRepair" 1
  
SectionEnd

Section "Core Libraries" sec_core

  SetOutPath "$INSTDIR"
  
  File "..\README"
  File "..\COPYING"
  File "..\CHANGES"
  File "..\THANKS"
  File "sripper_howto.txt"
  SetOverwrite off
  File "..\parse_rules.txt"
  SetOverwrite on
  File "..\fake_external_metadata.pl"
  File "..\fetch_external_metadata.pl"
  ;File "D:\sripper_1x\tre-0.7.2\win32\Release\tre.dll"
  ;File "D:\sripper_1x\win32\glib-2.12.12\bin\libglib-2.0-0.dll"
  ;File "D:\sripper_1x\iconv-win32\dll\iconv.dll"
  ;File "D:\sripper_1x\intl.dll"
  File "..\win32\glib-2.16.6-1\bin\libglib-2.0-0.dll"
  File "..\win32\libogg-1.1.3\ogg.dll"
  File "..\win32\libvorbis-1.1.2\vorbis.dll"
  File "..\win32\microsoft\unicows.dll"
  File "..\win32\mingw\libiconv-2.dll"
  File "..\win32\mingw\libintl-8.dll"
  File "..\win32\Release\streamripper.dll"
  File "..\win32\zlib-1.2.3\zlib1.dll"
  
  ;Store installation folder(s)
  WriteRegStr HKCU "Software\Streamripper" "" $INSTDIR
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "DisplayName" "Streamripper (Remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper" "NoRepair" 1
  
SectionEnd

;--------------------------------
;Installer Functions

Function .onInit

  ;Extract InstallOptions INI files
  !insertmacro INSTALLOPTIONS_EXTRACT "sr163_directories.ini"

  # set section 'test' as selected and read-only
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${sec_core} $0
  SectionSetFlags ${sec_gui} $0

FunctionEnd

Function .onSelChange

  SectionGetFlags ${sec_plugin} $0
  SectionGetFlags ${sec_gui} $1
  IntOp $0 $0 & ${SF_SELECTED}
  IntCmp $0 ${SF_SELECTED} with_plugin no_plugin no_plugin
with_plugin:
  IntOp $1 $1 | ${SF_SELECTED}
  IntOp $1 $1 | ${SF_RO}
  goto set_flags
no_plugin:
  IntOp $2 ${SF_RO} ~
  IntOp $1 $1 & $2
set_flags:
  SectionSetFlags ${sec_gui} $1

FunctionEnd

LangString TEXT_IO_TITLE ${LANG_ENGLISH} "Choose directories"
LangString TEXT_IO_SUBTITLE ${LANG_ENGLISH} "Choose the install directories for streamripper and winamp"

Function CustomPage

  ;; ReadINIStr $0 "$PLUGINSDIR\test.ini" "Field 8" "State"
  ;; WriteINIStr "$PLUGINSDIR\test.ini" "Settings" "RTL" "1"
  ;; !insertmacro INSTALLOPTIONS_READ $IniValue "sr163_directories.ini" "Field 2" "State"

  StrCpy $WADIR $PROGRAMFILES\Winamp

  ; detect winamp path from uninstall string if available
  ; otherwise default to $PROGRAMFILES\Winamp
  ReadRegStr $WADIR_TMP \
                 HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"
  IfErrors WADIR_DEFAULT 0
    StrCpy $WADIR_TMP $WADIR_TMP -1 1
    ${GetParent} "$WADIR_TMP" $R0
    ${DirState} "$R0" $R1
    IntCmp $R1 -1 WADIR_DEFAULT
      StrCpy $WADIR $R0
WADIR_DEFAULT:

  SectionGetFlags ${sec_plugin} $0
  IntOp $0 $0 & ${SF_SELECTED}
  IntCmp $0 ${SF_SELECTED} with_plugin
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 3" "Flags" "DISABLED"
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 4" "Flags" "DISABLED"
  goto set_directories
with_plugin:
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 3" "Flags" ""
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 4" "Flags" ""
set_directories:
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 2" "State" $INSTDIR
  !insertmacro INSTALLOPTIONS_WRITE "sr163_directories.ini" "Field 4" "State" $WADIR
  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
  !insertmacro INSTALLOPTIONS_DISPLAY "sr163_directories.ini"
  !insertmacro INSTALLOPTIONS_READ $INSTDIR "sr163_directories.ini" "Field 2" "State"
  !insertmacro INSTALLOPTIONS_READ $WADIR "sr163_directories.ini" "Field 4" "State"

FunctionEnd

;Function CustomPageB

;  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
;  !insertmacro INSTALLOPTIONS_DISPLAY "ioB.ini"

;FunctionEnd

;Function CustomPageC

;  !insertmacro MUI_HEADER_TEXT "$(TEXT_IO_TITLE)" "$(TEXT_IO_SUBTITLE)"
;  !insertmacro INSTALLOPTIONS_DISPLAY "sr163_directories.ini"

;FunctionEnd

;--------------------------------
;Descriptions

  ;Language strings
  LangString DESC_SecDummy1 ${LANG_ENGLISH} "Streamripper core.  You need this."
  LangString DESC_SecDummy2 ${LANG_ENGLISH} "The Winamp plugin.  This uses the GUI application for actual ripping."
  LangString DESC_SecDummy3 ${LANG_ENGLISH} "The GUI version of streamripper.  You need this for the winamp plugin.  But you can run it standalone too."
  LangString DESC_SecDummy4 ${LANG_ENGLISH} "The streamripper command-line application.  Very handy for advanced users."

  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_core} $(DESC_SecDummy1)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_plugin} $(DESC_SecDummy2)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_gui} $(DESC_SecDummy3)
    !insertmacro MUI_DESCRIPTION_TEXT ${sec_console} $(DESC_SecDummy4)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;; We still delete things like tre.dll in case someone 
  ;; installed on top of an old version.
  Delete $WADIR\Plugins\gen_sripper.dll
  Delete $INSTDIR\streamripper.exe
  Delete $INSTDIR\wstreamripper.exe
  Delete $INSTDIR\gen_sripper.dll
  Delete $INSTDIR\iconv.dll
  Delete $INSTDIR\intl.dll
  Delete $INSTDIR\libglib-2.0-0.dll
  Delete $INSTDIR\libiconv-2.dll
  Delete $INSTDIR\libintl-8.dll
  Delete $INSTDIR\ogg.dll
  Delete $INSTDIR\streamripper.dll
  Delete $INSTDIR\tre.dll
  Delete $INSTDIR\unicows.dll
  Delete $INSTDIR\vorbis.dll
  Delete $INSTDIR\zlib1.dll
  Delete $INSTDIR\README
  Delete $INSTDIR\COPYING
  Delete $INSTDIR\THANKS
  Delete $INSTDIR\CHANGES
  Delete $INSTDIR\SRIPPER_HOWTO.TXT
  Delete $INSTDIR\sripper.ini
  Delete $INSTDIR\parse_rules.txt
  Delete $INSTDIR\fake_external_metadata.pl
  Delete $INSTDIR\fetch_external_metadata.pl
  RMDir /r $INSTDIR\Skins
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"

;;  DeleteRegKey /ifempty HKCU "Software\Streamripper"

SectionEnd
