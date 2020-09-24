##-----------------------------------------------------------------------------
##  Welcome to Streamripper
##  This CMakeLists.txt file is given to the public domain.
##  
##  The rest of streamripper is available under the GNU GENERAL PUBLIC 
##  LICENSE, v2 or later.  See COPYING for details.
##-----------------------------------------------------------------------------
PROJECT (streamripper_lib)

SET (STREAMRIPPER_LIB_SRC
	callback.c callback.h
	cbuf3.c cbuf3.h
	charset.c charset.h
	debug.c	debug.h
	errors.c errors.h
	external.c external.h
	filelib.c filelib.h
	findsep.c findsep.h
	findsep2.c
	http.c http.h
	iconvert.c
	mchar.c mchar.h
	parse.c parse.h
	prefs.c prefs.h
	relaylib.c relaylib.h
	ripaac.c
	ripogg.c ripogg.h
	ripstream.c ripstream.h
	ripstream_mp3.c ripstream_mp3.h
	ripstream_ogg.c ripstream_ogg.h
	ripstream_wav.c ripstream_wav.h
	rip_manager.c rip_manager.h
	socklib.c socklib.h
	threadlib.c threadlib.h
	track_info.c track_info.h
	utf8.c utf8.h

	charmaps.h
	confw32.h
	list.h
	sr_compat.h
	srtypes.h
	uce_dirent.h
	)

IF (NOT IBERTY_FOUND)
  SET (STREAMRIPPER_LIB_SRC
    ${STREAMRIPPER_LIB_SRC}
    argv.c)
ENDIF (NOT IBERTY_FOUND)

ADD_LIBRARY (streamripper1 ${STREAMRIPPER_LIB_SRC})
INSTALL (TARGETS streamripper1 DESTINATION lib)
