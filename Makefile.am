## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = @OGG_CFLAGS@ @VORBIS_CFLAGS@ @GLIB_CFLAGS@

if SUBDIR_LIBMAD
  MAYBE_LIBMAD = libmad-0.15.1b
  AM_CPPFLAGS += -I@abs_top_srcdir@/libmad-0.15.1b
endif

# if SUBDIR_GLIB
#   MAYBE_GLIB = glib-2.16.6
#   AM_CPPFLAGS += -I@abs_top_srcdir@/glib-2.16.6/glib
#   AM_CPPFLAGS += -I@abs_top_srcdir@/glib-2.16.6
# endif

#if SUBDIR_CDK
#  MAYBE_CDK = cdk-5.0-20060507
#endif

AM_CPPFLAGS += -I@abs_top_srcdir@/lib

# SUBDIRS = $(MAYBE_LIBMAD) $(MAYBE_CDK) $(MAYBE_GLIB) lib
# SUBDIRS = $(MAYBE_LIBMAD) $(MAYBE_CDK) lib
SUBDIRS = $(MAYBE_LIBMAD) lib

bin_PROGRAMS = streamripper
if WANT_CSTREAMRIPPER
##    bin_PROGRAMS += cstreamripper
endif

EXTRA_PROGRAMS = streamripper ictest cstreamripper

ictest_SOURCES = \
	lib/ictest.c

streamripper_SOURCES = \
	console/streamripper.c

cstreamripper_SOURCES = \
	console/cstreamripper.c

if USE_INCLUDED_ARGV
streamripper_SOURCES += lib/argv.c
endif

libmad_manifest = \
	libmad-0.15.1b/README \
	libmad-0.15.1b/configure.ac \
	libmad-0.15.1b/aclocal.m4 \
	libmad-0.15.1b/mad.h \
	libmad-0.15.1b/Makefile.am \
	libmad-0.15.1b/Makefile.in \
	libmad-0.15.1b/config.h.in \
	libmad-0.15.1b/libmad.list.in \
	libmad-0.15.1b/configure \
	libmad-0.15.1b/COPYING \
	libmad-0.15.1b/INSTALL \
	libmad-0.15.1b/TODO \
	libmad-0.15.1b/config.guess \
	libmad-0.15.1b/config.sub \
	libmad-0.15.1b/depcomp \
	libmad-0.15.1b/install-sh \
	libmad-0.15.1b/ltmain.sh \
	libmad-0.15.1b/missing \
	libmad-0.15.1b/mkinstalldirs \
	libmad-0.15.1b/version.c \
	libmad-0.15.1b/fixed.c \
	libmad-0.15.1b/bit.c \
	libmad-0.15.1b/timer.c \
	libmad-0.15.1b/stream.c \
	libmad-0.15.1b/frame.c \
	libmad-0.15.1b/synth.c \
	libmad-0.15.1b/decoder.c \
	libmad-0.15.1b/layer12.c \
	libmad-0.15.1b/layer3.c \
	libmad-0.15.1b/huffman.c \
	libmad-0.15.1b/version.h \
	libmad-0.15.1b/fixed.h \
	libmad-0.15.1b/bit.h \
	libmad-0.15.1b/timer.h \
	libmad-0.15.1b/stream.h \
	libmad-0.15.1b/frame.h \
	libmad-0.15.1b/synth.h \
	libmad-0.15.1b/decoder.h \
	libmad-0.15.1b/global.h \
	libmad-0.15.1b/layer12.h \
	libmad-0.15.1b/layer3.h \
	libmad-0.15.1b/huffman.h \
	libmad-0.15.1b/D.dat \
	libmad-0.15.1b/imdct_s.dat \
	libmad-0.15.1b/qc_table.dat \
	libmad-0.15.1b/rq_table.dat \
	libmad-0.15.1b/sf_table.dat \
	libmad-0.15.1b/imdct_l_arm.S \
	libmad-0.15.1b/minimad.c \
	libmad-0.15.1b/mad.h.sed \
	libmad-0.15.1b/CHANGES \
	libmad-0.15.1b/COPYRIGHT \
	libmad-0.15.1b/CREDITS \
	libmad-0.15.1b/VERSION

libmad_dont_need_these = \
	libmad-0.15.1b/msvc++/Makefile.am \
	libmad-0.15.1b/msvc++/Makefile.in \
	libmad-0.15.1b/msvc++/mad.h \
	libmad-0.15.1b/msvc++/config.h \
	libmad-0.15.1b/msvc++/libmad.dsp

tre072_manifest = \
	tre-0.7.2/m4/tre_prog_cc_optimizations.m4 \
	tre-0.7.2/m4/ax_check_sign.m4 \
	tre-0.7.2/m4/Makefile.am \
	tre-0.7.2/m4/Makefile.in \
	tre-0.7.2/m4/vl_prog_cc_warnings.m4 \
	tre-0.7.2/m4/ac_libtool_tags.m4 \
	tre-0.7.2/m4/ax_decl_wchar_max.m4 \
	tre-0.7.2/m4/ax_check_funcs_comp.m4 \
	tre-0.7.2/po/Makevars \
	tre-0.7.2/po/fi.po \
	tre-0.7.2/po/LINGUAS \
	tre-0.7.2/po/en@boldquot.header \
	tre-0.7.2/po/en@quot.header \
	tre-0.7.2/po/Rules-quot \
	tre-0.7.2/po/remove-potcdate.sin \
	tre-0.7.2/po/boldquot.sed \
	tre-0.7.2/po/POTFILES.in \
	tre-0.7.2/po/tre.pot \
	tre-0.7.2/po/insert-header.sin \
	tre-0.7.2/po/quot.sed \
	tre-0.7.2/po/Makefile.in.in \
	tre-0.7.2/doc/tre-api.html \
	tre-0.7.2/doc/default.css \
	tre-0.7.2/doc/Makefile.am \
	tre-0.7.2/doc/Makefile.in \
	tre-0.7.2/doc/agrep.1.in \
	tre-0.7.2/doc/tre-syntax.html \
	tre-0.7.2/lib/tre-config.h.in \
	tre-0.7.2/lib/regex.h \
	tre-0.7.2/lib/tre-filter.c \
	tre-0.7.2/lib/tre-filter.h \
	tre-0.7.2/lib/tre-match-approx.c \
	tre-0.7.2/lib/README \
	tre-0.7.2/lib/tre-match-utils.h \
	tre-0.7.2/lib/xmalloc.c \
	tre-0.7.2/lib/xmalloc.h \
	tre-0.7.2/lib/tre-parse.c \
	tre-0.7.2/lib/tre-parse.h \
	tre-0.7.2/lib/tre-match-backtrack.c \
	tre-0.7.2/lib/tre-internal.h \
	tre-0.7.2/lib/regerror.c \
	tre-0.7.2/lib/tre-compile.c \
	tre-0.7.2/lib/tre-compile.h \
	tre-0.7.2/lib/Makefile.am \
	tre-0.7.2/lib/Makefile.in \
	tre-0.7.2/lib/tre-match-parallel.c \
	tre-0.7.2/lib/gettext.h \
	tre-0.7.2/lib/tre-ast.c \
	tre-0.7.2/lib/tre-ast.h \
	tre-0.7.2/lib/tre-mem.c \
	tre-0.7.2/lib/tre-mem.h \
	tre-0.7.2/lib/regcomp.c \
	tre-0.7.2/lib/regexec.c \
	tre-0.7.2/lib/tre-stack.c \
	tre-0.7.2/lib/tre-stack.h \
	tre-0.7.2/src/Makefile.am \
	tre-0.7.2/src/Makefile.in \
	tre-0.7.2/src/agrep.c \
	tre-0.7.2/NEWS \
	tre-0.7.2/TODO \
	tre-0.7.2/LICENSE \
	tre-0.7.2/aclocal.m4 \
	tre-0.7.2/tests/test-str-source.c \
	tre-0.7.2/tests/randtest.c \
	tre-0.7.2/tests/retest.c \
	tre-0.7.2/tests/Makefile.am \
	tre-0.7.2/tests/Makefile.in \
	tre-0.7.2/tests/build-tests.sh \
	tre-0.7.2/tests/bench.c \
	tre-0.7.2/utils/compile \
	tre-0.7.2/utils/depcomp \
	tre-0.7.2/utils/build-rpm.sh \
	tre-0.7.2/utils/ltmain.sh \
	tre-0.7.2/utils/config.guess \
	tre-0.7.2/utils/config.rpath \
	tre-0.7.2/utils/install-sh \
	tre-0.7.2/utils/autogen.sh \
	tre-0.7.2/utils/config.sub \
	tre-0.7.2/utils/missing \
	tre-0.7.2/utils/mkinstalldirs \
	tre-0.7.2/utils/Makefile.am \
	tre-0.7.2/utils/Makefile.in \
	tre-0.7.2/win32/tre-config.h \
	tre-0.7.2/win32/retest.dsp \
	tre-0.7.2/win32/config.h \
	tre-0.7.2/win32/tre.def \
	tre-0.7.2/win32/tre.dsp \
	tre-0.7.2/win32/tre.dsw \
	tre-0.7.2/README \
	tre-0.7.2/THANKS \
	tre-0.7.2/configure \
	tre-0.7.2/configure.ac \
	tre-0.7.2/tre.spec.in \
	tre-0.7.2/Makefile.am \
	tre-0.7.2/Makefile.in \
	tre-0.7.2/config.h.in \
	tre-0.7.2/python/example.py \
	tre-0.7.2/python/tre-python.c \
	tre-0.7.2/python/setup.py \
	tre-0.7.2/AUTHORS \
	tre-0.7.2/INSTALL \
	tre-0.7.2/tre.pc.in \
	tre-0.7.2/ABOUT-NLS \
	tre-0.7.2/ChangeLog \
	tre-0.7.2/COPYING

tre070_manifest = \
	tre-0.7.0/m4/tre_prog_cc_optimizations.m4 \
	tre-0.7.0/m4/ax_check_sign.m4 \
	tre-0.7.0/m4/Makefile.am \
	tre-0.7.0/m4/Makefile.in \
	tre-0.7.0/m4/vl_prog_cc_warnings.m4 \
	tre-0.7.0/m4/ac_libtool_tags.m4 \
	tre-0.7.0/m4/ax_decl_wchar_max.m4 \
	tre-0.7.0/m4/ax_check_funcs_comp.m4 \
	tre-0.7.0/po/Makevars \
	tre-0.7.0/po/fi.po \
	tre-0.7.0/po/LINGUAS \
	tre-0.7.0/po/en@boldquot.header \
	tre-0.7.0/po/en@quot.header \
	tre-0.7.0/po/Rules-quot \
	tre-0.7.0/po/remove-potcdate.sin \
	tre-0.7.0/po/boldquot.sed \
	tre-0.7.0/po/POTFILES.in \
	tre-0.7.0/po/fi.gmo \
	tre-0.7.0/po/tre.pot \
	tre-0.7.0/po/insert-header.sin \
	tre-0.7.0/po/stamp-po \
	tre-0.7.0/po/quot.sed \
	tre-0.7.0/po/Makefile.in.in \
	tre-0.7.0/doc/tre-api.html \
	tre-0.7.0/doc/tre-syntax.html \
	tre-0.7.0/lib/tre-config.h.in \
	tre-0.7.0/lib/regex.h \
	tre-0.7.0/lib/tre-match-approx.c \
	tre-0.7.0/lib/README \
	tre-0.7.0/lib/tre-match-utils.h \
	tre-0.7.0/lib/xmalloc.c \
	tre-0.7.0/lib/xmalloc.h \
	tre-0.7.0/lib/tre-match-backtrack.c \
	tre-0.7.0/lib/tre-internal.h \
	tre-0.7.0/lib/regerror.c \
	tre-0.7.0/lib/tre-compile.c \
	tre-0.7.0/lib/Makefile.am \
	tre-0.7.0/lib/Makefile.in \
	tre-0.7.0/lib/tre-match-parallel.c \
	tre-0.7.0/lib/gettext.h \
	tre-0.7.0/lib/tre-mem.c \
	tre-0.7.0/lib/tre-mem.h \
	tre-0.7.0/lib/regcomp.c \
	tre-0.7.0/lib/regexec.c \
	tre-0.7.0/src/Makefile.am \
	tre-0.7.0/src/Makefile.in \
	tre-0.7.0/src/agrep.c \
	tre-0.7.0/NEWS \
	tre-0.7.0/TODO \
	tre-0.7.0/LICENSE \
	tre-0.7.0/aclocal.m4 \
	tre-0.7.0/tests/test-str-source.c \
	tre-0.7.0/tests/randtest.c \
	tre-0.7.0/tests/retest.c \
	tre-0.7.0/tests/Makefile.am \
	tre-0.7.0/tests/Makefile.in \
	tre-0.7.0/tests/bench.c \
	tre-0.7.0/tests/test-approx.c \
	tre-0.7.0/utils/compile \
	tre-0.7.0/utils/depcomp \
	tre-0.7.0/utils/build-rpm.sh \
	tre-0.7.0/utils/ltmain.sh \
	tre-0.7.0/utils/config.guess \
	tre-0.7.0/utils/config.rpath \
	tre-0.7.0/utils/install-sh \
	tre-0.7.0/utils/autogen.sh \
	tre-0.7.0/utils/config.sub \
	tre-0.7.0/utils/missing \
	tre-0.7.0/utils/mkinstalldirs \
	tre-0.7.0/utils/Makefile.am \
	tre-0.7.0/utils/Makefile.in \
	tre-0.7.0/win32/tre-config.h \
	tre-0.7.0/win32/retest.dsp \
	tre-0.7.0/win32/config.h \
	tre-0.7.0/win32/tre.def \
	tre-0.7.0/win32/tre.dsp \
	tre-0.7.0/win32/tre.dsw \
	tre-0.7.0/README \
	tre-0.7.0/THANKS \
	tre-0.7.0/configure \
	tre-0.7.0/configure.ac \
	tre-0.7.0/tre.spec.in \
	tre-0.7.0/Makefile.am \
	tre-0.7.0/Makefile.in \
	tre-0.7.0/config.h.in \
	tre-0.7.0/python \
	tre-0.7.0/python/setup.py.in \
	tre-0.7.0/AUTHORS \
	tre-0.7.0/INSTALL \
	tre-0.7.0/tre.pc.in \
	tre-0.7.0/ABOUT-NLS \
	tre-0.7.0/ChangeLog \
	tre-0.7.0/COPYING

man_MANS = \
	streamripper.1

lib_manifest = \
	lib/Makefile.am \
	lib/Makefile.in \
	lib/config.h.in \
	lib/argv.c \
	lib/cbuf2.c \
	lib/cbuf2.h \
	lib/charmaps.h \
	lib/charset.c \
	lib/charset.h \
	lib/compat.h \
	lib/config.h \
	lib/confw32.h \
	lib/debug.c \
	lib/debug.h \
	lib/errors.c \
	lib/errors.h \
	lib/external.c \
	lib/external.h \
	lib/filelib.c \
	lib/filelib.h \
	lib/findsep2.c \
	lib/findsep.c \
	lib/findsep.h \
	lib/http.c \
	lib/http.h \
	lib/iconvert.c \
	lib/ictest.c \
	lib/list.h \
	lib/mchar.c \
	lib/mchar.h \
	lib/parse.c \
	lib/parse.h \
	lib/prefs.c \
	lib/prefs.h \
	lib/relaylib.c \
	lib/relaylib.h \
	lib/rip_manager.c \
	lib/rip_manager.h \
	lib/ripaac.c \
	lib/ripogg.c \
	lib/ripogg.h \
	lib/ripstream.c \
	lib/ripstream.h \
	lib/socklib.c \
	lib/socklib.h \
	lib/srconfig.h \
	lib/srtypes.h \
	lib/threadlib.c \
	lib/threadlib.h \
	lib/uce_dirent.h \
	lib/utf8.c \
	lib/utf8.h

EXTRA_DIST = \
	config.rpath \
	CHANGES \
	parse_rules.txt \
	fake_external_metadata.pl \
	fetch_external_metadata.pl \
	$(lib_manifest) \
	$(libmad_manifest) \
	$(man_MANS)

DIST_SUBDIRS = 

streamripper_LDADD = \
	lib/libstreamripper.a \
	@FAAD_LIBS@ \
	@LIBMAD@ \
	@THREADLIBS@ \
	@LTLIBICONV@ @LIBCHARSET@ \
	@LIBIBERTY_LIBS@ \
	@OGG_LIBS@ @VORBIS_LIBS@ \
	@GLIB_LIBS@

#        @CDK_LIBS@

cstreamripper_LDADD = \
	lib/libstreamripper.a \
	@FAAD_LIBS@ \
	@LIBMAD@ \
	@THREADLIBS@ \
	@LTLIBICONV@ @LIBCHARSET@ \
	@LIBIBERTY_LIBS@ \
	@OGG_LIBS@ @VORBIS_LIBS@ \
	@GLIB_LIBS@

#        @CDK_LIBS@

ACLOCAL_AMFLAGS = -I m4
