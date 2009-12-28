; waplugin.nsi
;
; This script will generate an installer that installs a Winamp plug-in.
; It also puts a license page on, for shits and giggles.
;
; This installer will automatically alert the user that installation was
; successful, and ask them whether or not they would like to make the 
; plug-in the default and run Winamp.
;

; The name of the installer
Name "Streamripper for Winamp 2.x/5.x v1.63-beta-6"

; The file to write
OutFile "srwa5-1.63-beta-6.exe"

; License page
; LicenseText "This installer will install the Nullsoft Tiny Visualization 2000 Plug-in for Winamp. Please read the license below."
; use the default makensis license :)
; LicenseData ../COPYING

; The default installation directory
InstallDir $PROGRAMFILES\Winamp
; detect winamp path from uninstall string if available
InstallDirRegKey HKLM \
                 "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \
                 "UninstallString"

; The text to prompt the user to enter a directory
DirText "Please select your Winamp path below (you will be able to proceed when Winamp is detected):"
;DirShow hide

; automatically close the installer when done.
AutoCloseWindow true
; hide the "show details" box
;ShowInstDetails nevershow

Function .onVerifyInstDir
!ifndef WINAMP_AUTOINSTALL
  IfFileExists $INSTDIR\Winamp.exe Good
    Abort
  Good:
!endif ; WINAMP_AUTOINSTALL
FunctionEnd

Function QueryWinampPluginPath ; sets $1 with vis path
  StrCpy $1 $INSTDIR\Plugins
FunctionEnd


!ifdef WINAMP_AUTOINSTALL
Function GetWinampInstPath
  Push $0
  Push $1
  Push $2
  ReadRegStr $0 HKLM \
     "Software\Microsoft\Windows\CurrentVersion\Uninstall\Winamp" \ 
     "UninstallString"
  StrCmp $0 "" fin

    StrCpy $1 $0 1 0 ; get firstchar
    StrCmp $1 '"' "" getparent 
      ; if first char is ", let's remove "'s first.
      StrCpy $0 $0 "" 1
      StrCpy $1 0
      rqloop:
        StrCpy $2 $0 1 $1
        StrCmp $2 '"' rqdone
        StrCmp $2 "" rqdone
        IntOp $1 $1 + 1
        Goto rqloop
      rqdone:
      StrCpy $0 $0 $1
    getparent:
    ; the uninstall string goes to an EXE, let's get the directory.
    StrCpy $1 -1
    gploop:
      StrCpy $2 $0 1 $1
      StrCmp $2 "" gpexit
      StrCmp $2 "\" gpexit
      IntOp $1 $1 - 1
      Goto gploop
    gpexit:
    StrCpy $0 $0 $1

    StrCmp $0 "" fin
    IfFileExists $0\winamp.exe fin
      StrCpy $0 ""
  fin:
  Pop $2
  Pop $1
  Exch $0
FunctionEnd



Function MakeSureIGotWinamp
  Call GetWinampInstPath
  Pop $0
  StrCmp $0 "" getwinamp
    Return
  getwinamp:
  StrCpy $1 $TEMP\porearre1.dll 
  StrCpy $2 "$TEMP\Winamp Installer.exe"
  File /oname=$1 nsisdl.dll
  Push http://download.nullsoft.com/winamp/client/winamp281_lite.exe
  Push $2
  CallInstDLL $1 download
  Delete $1
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint "download failed: $0"
    Abort
  success:
    ExecWait '"$2" /S'
    Delete $2
    Call GetWinampInstPath
    Pop $0
    StrCmp $0 "" skip
    StrCpy $INSTDIR $0
  skip:
FunctionEnd

!endif ; WINAMP_AUTOINSTALL
; The stuff to install
Section "Installer Section"
!ifdef WINAMP_AUTOINSTALL
  Call MakeSureIGotWinamp
!endif

  Call QueryWinampPluginPath
  SetOutPath $1

  ; File to extract
  File "C:\program files\winamp\plugins\gen_sripper.dll"
  File "sripper_howto.txt"
  SetOverwrite off
  File "..\parse_rules.txt"
  SetOverwrite on
  File "..\fake_external_metadata.pl"
  File "..\fetch_external_metadata.pl"

  SetOutPath $INSTDIR\Skins\SrSkins
  File srskin.bmp
  File srskin_winamp.bmp
  File srskin_XP.bmp

  WriteUninstaller $INSTDIR\streamripper_uninstall.exe
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper.Plugin" "DisplayName" "Streamripper Plugin 1.63-beta-6 (Remove only)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper.Plugin" "UninstallString" "$INSTDIR\streamripper_uninstall.exe"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper.Plugin" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper.Plugin" "NoRepair" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Streamripper.Plugin" "EstimatedSize" 1800

  SetOutPath $INSTDIR
  ;; File "c:\program files\winamp\winamp.exe.manifest"
  ExecShell open "$WINDIR\notepad.exe" "$1\SRIPPER_HOWTO.TXT"

  ; prompt user, and if they select no, skip the following 3 instructions.
  MessageBox MB_YESNO|MB_ICONQUESTION \
             "The plug-in was installed. Would you like to run Winamp now with Streamripper as the default plug-in?" \
             IDNO NoWinamp
    WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "genplugin_name" "gen_sripper.dll"
    WriteINIStr "$INSTDIR\Winamp.ini" "Winamp" "genplugin_num" "0"
    ExecShell open  '"$INSTDIR\Winamp.exe"'
  NoWinamp:
SectionEnd

;; Section "un.Installer Section"
Section "Uninstall"
  Delete $INSTDIR\streamripper_uninstall.exe
  Delete $INSTDIR\Plugins\gen_sripper.dll
  Delete $INSTDIR\Plugins\SRIPPER_HOWTO.TXT
  Delete $INSTDIR\Plugins\sripper.ini
  Delete $INSTDIR\Plugins\parse_rules.txt
  Delete $INSTDIR\Plugins\fake_external_metadata.pl
  Delete $INSTDIR\Plugins\fetch_external_metadata.pl
  RMDir /r $INSTDIR\Skins\SrSkins
SectionEnd

; eof
