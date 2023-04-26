## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @OGG_CFLAGS@ @VORBIS_CFLAGS@ @GLIB_CFLAGS@

if SUBDIR_LIBMAD
  AM_CPPFLAGS += -I@abs_top_srcdir@/libmad-0.15.1b
endif

# if SUBDIR_GLIB
#   MAYBE_GLIB = glib-2.16.6
#   AM_CPPFLAGS += -I@abs_top_srcdir@/glib-2.16.6/glib
#   AM_CPPFLAGS += -I@abs_top_srcdir@/glib-2.16.6
# endif

AM_CPPFLAGS += -I@abs_top_srcdir@/lib

noinst_LIBRARIES = libstreamripper.a

libstreamripper_a_SOURCES = \
	cbuf2.c \
	charset.c \
	debug.c \
	errors.c \
	external.c \
	filelib.c \
	findsep.c \
	findsep2.c \
	http.c \
	iconvert.c \
	mchar.c \
	parse.c \
	prefs.c \
	relaylib.c \
	rip_manager.c \
	ripaac.c \
	ripogg.c \
	ripstream.c \
	socklib.c \
	threadlib.c \
	utf8.c \
	cbuf2.h \
	charmaps.h \
	charset.h \
	compat.h \
	debug.h \
	errors.h \
	external.h \
	filelib.h \
	findsep.h \
	http.h \
	list.h \
	mchar.h \
	parse.h \
	prefs.h \
	relaylib.h \
	rip_manager.h \
	ripogg.h \
	ripstream.h \
	socklib.h \
	srconfig.h \
	threadlib.h \
	srtypes.h \
	uce_dirent.h \
	utf8.h
