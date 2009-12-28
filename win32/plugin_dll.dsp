# Microsoft Developer Studio Project File - Name="plugin_dll" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=plugin_dll - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "plugin_dll.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "plugin_dll.mak" CFG="plugin_dll - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "plugin_dll - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "plugin_dll - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "plugin_dll - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "plugin_dll_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I "..\win32\glib-2.16.6-1\lib\glib-2.0\include" /I "../win32/libogg-1.1.3" /I "../win32/libvorbis-1.1.2" /I "../lib" /I "../libmad-0.15.1b/msvc++" /I "..\win32\glib-2.16.6-1\include\glib-2.0" /I "../iconv-win32/static" /I "../tre-0.7.2/lib" /I "../tre-0.7.2/win32" /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "plugin_dll_EXPORTS" /D "USE_LAYER_2" /D "USE_LAYER_1" /D "HAVE_MPGLIB" /D "NOANALYSIS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 msvcprt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib advapi32.lib /nologo /dll /machine:I386 /out:"Release\gen_sripper.dll" /libpath:"../libmad-0.15.1b/msvc++/release" /libpath:"../tre-0.7.2/win32/Release" /libpath:"../win32/libogg-1.1.3" /libpath:"../win32/libvorbis-1.1.2" /libpath:"../iconv-win32/static" /libpath:"..\win32\glib-2.16.6-1\lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=del "C:\Program Files\Winamp\Plugins\gen_sripperd.dll"	copy Release\gen_sripper.dll "c:\Program Files\Winamp\Plugins\gen_sripper.dll"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "plugin_dll - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "plugin_dll_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /Zi /Od /I "..\win32\glib-2.12.12\lib\glib-2.0\include" /I "../win32/libogg-1.1.3" /I "../win32/libvorbis-1.1.2" /I "../lib" /I "../libmad-0.15.1b/msvc++" /I "..\win32\glib-2.16.6-1\include\glib-2.0" /I "../iconv-win32/static" /I "../tre-0.7.2/lib" /I "../tre-0.7.2/win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "HAVE_MPGLIB" /D "NOANALYSIS" /D "HAVE_MEMCPY" /D "DEBUG_TO_FILE" /D "_DEBUG_" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 msvcprt.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib comctl32.lib advapi32.lib /nologo /dll /debug /machine:I386 /out:"Debug\gen_sripperd.dll" /pdbtype:sept /libpath:"../libmad-0.15.1b/msvc++/debug" /libpath:"../tre-0.7.2/win32/debug" /libpath:"../win32/libogg-1.1.3" /libpath:"../win32/libvorbis-1.1.2" /libpath:"../iconv-win32/static" /libpath:"..\win32\glib-2.16.6-1\lib"
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Cmds=del "C:\Program Files\Winamp\Plugins\gen_sripper.dll"	copy Debug\gen_sripperd.dll "c:\Program Files\Winamp\Plugins\gen_sripperd.dll"
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "plugin_dll - Win32 Release"
# Name "plugin_dll - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\winamp_plugin\plugin_dll_main.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\registry.c
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\Script.rc
# End Source File
# Begin Source File

SOURCE=..\winamp_plugin\winamp_dll.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Source File

SOURCE=..\winamp_plugin\sricon.ico
# End Source File
# End Target
# End Project
