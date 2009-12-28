##-----------------------------------------------------------------------------
##  Welcome to Streamripper
##-----------------------------------------------------------------------------
PROJECT (STREAMRIPPER)

CMAKE_MINIMUM_REQUIRED (VERSION 2.6.0)

## Tell CMake to be quiet
CMAKE_POLICY (SET CMP0003 NEW)

## Default to release
IF (NOT CMAKE_BUILD_TYPE)
  SET (CMAKE_BUILD_TYPE Release)
ENDIF (NOT CMAKE_BUILD_TYPE)

##-----------------------------------------------------------------------------
##  Include files
##-----------------------------------------------------------------------------
INCLUDE (CheckFunctionExists)
INCLUDE (FindPkgConfig)
INCLUDE (FindOpenGL)  # Why?

##-----------------------------------------------------------------------------
##  Find libraries
##-----------------------------------------------------------------------------
IF (UNIX)
  SET (MATH_LIB -lm)
ELSE (UNIX)
  SET (MATH_LIB)
ENDIF (UNIX)

PKG_CHECK_MODULES (GLIB2 REQUIRED glib-2.0>=2.16)

SET (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
FIND_PACKAGE (IBERTY)
FIND_PACKAGE (MAD)
FIND_PACKAGE (OGG)
FIND_PACKAGE (VORBIS)

INCLUDE (${CMAKE_SOURCE_DIR}/cmake/SR_FindSockets.cmake)

FIND_LIBRARY (SEMAPHORE_LIBRARIES NAMES sem rt)

##-----------------------------------------------------------------------------
##  Include directories
##-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (BEFORE ${GLIB2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/lib)

IF (WIN32)
  INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/include/glib-2.0)
  INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/lib/glib-2.0/include)
  INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/libmad-0.15.1b/msvc++)
  INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/iconv-win32/static)
  INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/libogg-1.1.3)
  INCLUDE_DIRECTORIES (BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/win32/libvorbis-1.1.2)
ENDIF (WIN32)

##-----------------------------------------------------------------------------
##  Configure include file
##-----------------------------------------------------------------------------
CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/sr_config.h.cmake 
  ${CMAKE_CURRENT_BINARY_DIR}/sr_config.h)

##-----------------------------------------------------------------------------
##  Build subdirectories
##-----------------------------------------------------------------------------
ADD_SUBDIRECTORY (lib)

##-----------------------------------------------------------------------------
##  Source files
##-----------------------------------------------------------------------------
SET(STREAMRIPPER_SRC
	console/streamripper.c
	)

SET(WSTREAMRIPPER_SRC
	winamp_plugin/dock.c
	winamp_plugin/dock.h
	winamp_plugin/gen.h
	winamp_plugin/ipc_pe.h
	winamp_plugin/options.c
	winamp_plugin/options.h
#	winamp_plugin/plugin_dll_main.c
	winamp_plugin/plugin_main.c
	winamp_plugin/plugin_main.h
	winamp_plugin/registry.c
	winamp_plugin/registry.h
	winamp_plugin/render.c
	winamp_plugin/render.h
	winamp_plugin/resource.h
	winamp_plugin/wa_ipc.h
	winamp_plugin/wa_msgids.h
#	winamp_plugin/winamp_dll.c
#	winamp_plugin/winamp_dll.h
	winamp_plugin/winamp_exe.c
	winamp_plugin/winamp_exe.h
	winamp_plugin/wstreamripper.h
#	winamp_plugin/Script.rc
	)

##-----------------------------------------------------------------------------
##  Targets
##-----------------------------------------------------------------------------
IF (WIN32)
  LINK_DIRECTORIES(
	"${CMAKE_CURRENT_SOURCE_DIR}/libmad-0.15.1b/msvc++/Release"
	"${CMAKE_CURRENT_SOURCE_DIR}/iconv-win32/static"
	"${CMAKE_CURRENT_SOURCE_DIR}/tre-0.7.2/win32/Release"
#	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.12.12/lib"
#	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.3/lib"
#	"c:/gcs6/projects/sripper_1x/glib-gcs/no-unicows"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/glib-2.16.6-1/lib"
#	"c:/gcs6/pkg/libunicows/libunicows-1.1.2-msvc6"
#	"c:/gcs6/pkg/libunicows/libunicows-1.1.1-mingw"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/libogg-1.1.3"
	"${CMAKE_CURRENT_SOURCE_DIR}/win32/libvorbis-1.1.2"
	)
ENDIF (WIN32)

SET (STREAMRIPPER_LIBS ${MAD_LIBRARIES} ${GLIB2_LIBRARIES} ${MATH_LIB})

IF (WIN32)
  #SET(STREAMRIPPER_LIBS libmad iconv tre glib-2.0 ws2_32 ogg vorbis)
  #SET(STREAMRIPPER_LIBS libmad iconv glib-2.0 ws2_32 ogg vorbis)
  SET(STREAMRIPPER_LIBS libmad iconv unicows glib-2.0 unicows ws2_32 ogg vorbis)
ENDIF (WIN32)

IF (SEMAPHORE_LIBRARIES)
  SET (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${SEMAPHORE_LIBRARIES})
ENDIF (SEMAPHORE_LIBRARIES)

IF (IBERTY_FOUND)
  SET (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${IBERTY_LIBRARIES})
ENDIF (IBERTY_FOUND)

IF (OGG_FOUND AND VORBIS_FOUND)
  SET (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${OGG_LIBRARIES}
    ${VORBIS_LIBRARIES})
ENDIF (OGG_FOUND AND VORBIS_FOUND)

SET (STREAMRIPPER_LIBS ${STREAMRIPPER_LIBS} ${SR_EXTRA_LIBS} ${MATH_LIB})

##-----------------------------------------------------------------------------
##  Linking
##-----------------------------------------------------------------------------
## chkstk.o may be required for mingw
IF (WIN32 AND (MSYS OR MINGW))
  SET (STREAMRIPPER_SRC ${STREAMRIPPER_SRC} 
  		       ${CMAKE_CURRENT_SOURCE_DIR}/win32/chkstk.o)
ENDIF (WIN32 AND (MSYS OR MINGW))

#ADD_LIBRARY (streamripper1 ${STREAMRIPPER_LIB_SRC})
ADD_EXECUTABLE (streamripper ${STREAMRIPPER_SRC})
TARGET_LINK_LIBRARIES (streamripper ${MY_MATH_LIB} 
  streamripper1 ${STREAMRIPPER_LIBS})

IF (WIN32)
    ADD_EXECUTABLE (wstreamripper WIN32 ${WSTREAMRIPPER_SRC})
    TARGET_LINK_LIBRARIES (wstreamripper ${MY_MATH_LIB} 
      streamripper1 ${STREAMRIPPER_LIBS}
      comctl32.lib
      )
ENDIF (WIN32)

IF (MSVC)
  ADD_EXECUTABLE (glib_iconv_test lib/glib_iconv_test.c)
  TARGET_LINK_LIBRARIES (glib_iconv_test ${STREAMRIPPER_LIBS})

  ADD_EXECUTABLE (glib_win98_test lib/glib_win98_test.c)
  TARGET_LINK_LIBRARIES (glib_win98_test ${STREAMRIPPER_LIBS})
ENDIF (MSVC)

##-----------------------------------------------------------------------------
##  Install
##-----------------------------------------------------------------------------
INSTALL(TARGETS streamripper DESTINATION bin)
