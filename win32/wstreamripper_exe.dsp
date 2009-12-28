# Microsoft Developer Studio Project File - Name="wstreamripper_exe" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=wstreamripper_exe - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "wstreamripper_exe.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "wstreamripper_exe.mak" CFG="wstreamripper_exe - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "wstreamripper_exe - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "wstreamripper_exe - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "wstreamripper_exe - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "wstreamripper_exe___Win32_Release"
# PROP BASE Intermediate_Dir "wstreamripper_exe___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "../win32/libogg-1.1.3" /I "../win32/libvorbis-1.1.2" /I "../lib" /I "../libmad-0.15.1b/msvc++" /I "..\win32\glib-2.16.6-1\include\glib-2.0" /I "..\win32\glib-2.16.6-1\lib\glib-2.0\include" /I "../iconv-win32/static" /I "../win32/zlib-1.2.3/include" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib comctl32.lib libmad.lib ogg.lib vorbis.lib charset.lib iconv.lib glib-2.0.lib streamripper.lib zdll.lib /nologo /subsystem:windows /machine:I386 /out:"Release/wstreamripper.exe" /libpath:"../libmad-0.15.1b/msvc++/release" /libpath:"Release" /libpath:"../win32/libogg-1.1.3" /libpath:"../win32/libvorbis-1.1.2" /libpath:"../iconv-win32/static" /libpath:"..\win32\glib-2.16.6-1\lib" /libpath:"../win32/zlib-1.2.3/lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy release\wstreamripper.exe "C:\Program Files\Streamripper"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "wstreamripper_exe - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "wstreamripper_exe___Win32_Debug"
# PROP BASE Intermediate_Dir "wstreamripper_exe___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /MD /W3 /Gm /GX /ZI /Od /I "../win32/libogg-1.1.3" /I "../win32/libvorbis-1.1.2" /I "../lib" /I "../libmad-0.15.1b/msvc++" /I "..\win32\glib-2.16.6-1\include\glib-2.0" /I "..\win32\glib-2.16.6-1\lib\glib-2.0\include" /I "../iconv-win32/static" /I "../win32/zlib-1.2.3/include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fr /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ws2_32.lib comctl32.lib msvcrt.lib libmad.lib ogg.lib vorbis.lib charset.lib iconv.lib glib-2.0.lib streamripper.lib zdll.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/wstreamripper.exe" /pdbtype:sept /libpath:"../libmad-0.15.1b/msvc++/debug" /libpath:"Debug" /libpath:"../win32/libogg-1.1.3" /libpath:"../win32/libvorbis-1.1.2" /libpath:"../iconv-win32/static" /libpath:"..\win32\glib-2.16.6-1\lib" /libpath:"../win32/zlib-1.2.3/lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=copy debug\wstreamripper.exe "C:\Program Files\Streamripper"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "wstreamripper_exe - Win32 Release"
# Name "wstreamripper_exe - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\winamp_plugin\crypt.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\debug_box.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\dock.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\ioapi.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\ioapi.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\iowin32.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\iowin32.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\options.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\options.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\plugin_main.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\plugin_main.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\registry.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\registry.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\render.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\render.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\render_2.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\render_2.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\Script.rc
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\unzip.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\unzip.h
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\winamp_exe.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\winamp_exe.h
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\winamp_plugin\debug_box.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\winamp_plugin\sricon.ico
# End Source File
# End Group
# End Target
# End Project
