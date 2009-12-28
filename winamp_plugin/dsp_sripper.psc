Name Streamripper for Winamp 1.52
Text This will install Nullsoft Streamripper for Winamp 1.52 on your computer
OutFile sripper152.exe
SetOutPath $DSPDIR
AddFile \Program Files\Winamp\Plugins\gen_sripper.dll
AddFile HOWTO.txt

SetOutPath Skins\SrSkins
Addfile srskin.bmp
Addfile srskin_winamp.bmp
Addfile srskin_XP.bmp

SetOutPath $INSTDIR
Addfile \Program Files\Winamp\winamp.exe.manifest
ExecFile "$WINDIR\notepad.exe" "$DSPDIR\HOWTO.txt"
; Silent
