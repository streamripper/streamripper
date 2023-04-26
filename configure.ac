dnl Process this file with autoconf to produce a configure script.
dnl ---------------------
dnl I found a great tutorial on autoconf:
dnl    http://mi.eng.cam.ac.uk/~er258/code/autoconf/index.html
dnl ---------------------
dnl These macros create entries in config.h
dnl   AC_CHECK_HEADERS, AC_CHECK_TYPES, AC_CHECK_FUNCS
dnl These macros do not
dnl   AC_CHECK_LIB:   prepends the library to LIBS
dnl   AC_CHECK_FUNC:  defines a shell variable

AC_PREREQ(2.59)
AC_INIT(streamripper,1.64.6)
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([lib/config.h])

dnl Checks for programs.
AC_PROG_CC

if test -n "$GCC"; then
        CFLAGS="$CFLAGS -Wall"
fi

AC_PROG_INSTALL
AC_PROG_RANLIB

dnl test for inline
AC_C_INLINE

dnl Checks for libraries.
dnl Replace `main' with a function in -lm:
AC_CHECK_LIB(m, main)

dnl Yummy alloca()
AC_FUNC_ALLOCA

dnl Configure options

AC_ARG_WITH(included-libmad,
 [  --with-included-libmad use the libmad library included with streamripper])

dnl AC_ARG_WITH(included-glib,
dnl  [  --with-included-glib use the glib library included with streamripper])

dnl AC_ARG_WITH(included-tre,
dnl   [  --with-included-tre use the tre library included with streamripper])

AC_ARG_WITH(included-argv,
  [  --with-included-argv use argv.c included with streamripper])

dnl AC_ARG_WITH(included-cdk,
dnl   [  --with-included-cdk use the cdk library included with streamripper])

AC_ARG_WITH(curses, 
  [  --with-curses(=XXX) compile curses front-end (XXX is curses (default), ncurses, ncursesw, pdcurses or slang)])


CPPFLAGS="-D__UNIX__"
AC_SUBST(CPPFLAGS)


dnl -------- Tests for using included glib ------

dnl use_included_glib=no
dnl if test "x$with_included_glib" = xyes; then
dnl   if test ! -d glib-2.16.6; then
dnl     AC_MSG_ERROR([Using included glib not available from tarball.])
dnl   fi
dnl   use_included_glib=yes
dnl   GLIB_LIBS="glib-2.16.6/glib/.libs/libglib-2.0.a"
dnl   AC_CONFIG_SUBDIRS(glib-2.16.6)
dnl else
dnl   AM_PATH_GLIB_2_0(2.16.0,,AC_MSG_ERROR([Glib 2.16 or greater required]))
dnl fi
dnl AM_CONDITIONAL(SUBDIR_GLIB, test "x$with_included_glib" = xyes)

AM_PATH_GLIB_2_0(2.16.0,,AC_MSG_ERROR([Glib 2.16 or greater required]))

dnl -------- Tests for using included libmad ------
LIBMAD=""
AC_SUBST(LIBMAD)
use_included_libmad=yes
if test "x$with_included_libmad" != xyes; then
  sr_save_CPPFLAGS="$CPPFLAGS"
  CPPFLAGS="-Ilib -D__UNIX__"
  AC_SUBST(CPPFLAGS)
  AC_CHECK_LIB(mad, mad_stream_buffer,
    use_included_libmad=no,
    use_included_libmad=yes)
  CPPFLAGS="$sr_save_CPPFLAGS"
  AC_SUBST(CPPFLAGS)
fi
AM_CONDITIONAL(SUBDIR_LIBMAD, test "$use_included_libmad" = yes)
if test "$use_included_libmad" = yes; then
  LIBMAD="libmad-0.15.1b/.libs/libmad.a"
  AC_SUBST(LIBMAD)
  AC_CONFIG_SUBDIRS(libmad-0.15.1b)
else
  LIBMAD=-lmad
  AC_SUBST(LIBMAD)
fi

dnl -------- Tests for threading library ----------
THREADLIBS=no
AC_CHECK_LIB(pthread,pthread_create,THREADLIBS="-lpthread",)
if test "$THREADLIBS" = no; then
        AC_CHECK_LIB(c_r,pthread_create,THREADLIBS="-pthread",)
fi
if test "$THREADLIBS" = no; then
        AC_CHECK_LIB(dce,pthread_create,THREADLIBS="-ldce",)
fi
if test "$THREADLIBS" = no; then
        AC_CHECK_LIB(pthreads,pthread_create,THREADLIBS="-lpthreads",)
fi
if test "$THREADLIBS" = no; then
        AC_MSG_ERROR(Your system doesn't seem to support posix threads)
        exit
fi
AC_SUBST(THREADLIBS)

dnl Socklen
TYPE_SOCKLEN_T

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

dnl -------- Tests for wchar support ---------
dnl Wchar support is either all or nothing.  All the following 
dnl must be present:
dnl   1) Working wchar in C library
dnl   2) Regular expression library with wide character support
dnl   3) iconv library

dnl It seems that it could be in wchar.h, stddef.h, or builtin
AC_CHECK_HEADERS(wchar.h)
AC_CHECK_HEADERS(wctype.h)
AC_CHECK_TYPES([wchar_t],[],[],[
#if HAVE_WCHAR_H
#include <wchar.h>
#endif
#if STDC_HEADERS
#include <stddef.h>
#endif
])
dnl -- More wide character madness.
dnl FreeBSD doesn't have an iswcntrl()
AC_CHECK_FUNCS(iswcntrl)
dnl Linux is missing vswprintf prototype
AC_CHECK_FUNCS(vswprintf)
AC_FUNC_MBRTOWC

dnl -------- Check for iconv & related
AM_ICONV
AM_LANGINFO_CODESET
SR_FIND_LOCALE_CHARSET

dnl -------- Check for wchar support
dnl  This is will be used and updated in the regex test below
have_wchar=no
if test "$ac_cv_type_wchar_t" = yes; then
if test "$ac_cv_func_mbrtowc" = yes; then
if test "$ac_cv_func_iswcntrl" = yes; then
if test "$am_cv_func_iconv" = yes; then
  have_wchar=yes
fi
fi
fi
fi

dnl -------- Tests for regular expressions ---------
dnl Preference:
dnl   1)  Use system tre if installed
dnl   2a) If we don't have wchar, use system regex
dnl   2b) If we have wchar, and system regex has regwcomp, use system regex
dnl   3)  Use included tre
dnl Note: If using included tre, it's not easy to know whether or not 
dnl   it will compile with regwcomp() or not.  We will assume it does.
dnl Note: Solaris now has a regwcomp() in system regex.  Is it compatible?
dnl LIBREGEX=""
dnl AC_SUBST(LIBREGEX)
dnl use_included_tre=no
dnl if test "$with_included_tre" = "yes"; then
dnl   use_included_tre=yes
dnl else
dnl   AC_CHECK_LIB(tre,regwcomp)
dnl   if test "$ac_cv_lib_tre_regwcomp" = "no"; then
dnl     if test "$have_wchar" = "yes"; then
dnl       if test "$with_included_tre" = "no"; then
dnl         AC_CHECK_FUNC(regwcomp,,have_wchar=no)
dnl       else
dnl         AC_CHECK_FUNC(regwcomp,,use_included_tre=yes)
dnl       fi
dnl     fi
dnl     if test "$have_wchar" = "no"; then
dnl       AC_CHECK_LIB(tre,regcomp)
dnl       if test "$ac_cv_lib_tre_regcomp" = "no"; then
dnl         if test "$with_included_tre" = "no"; then
dnl           AC_CHECK_FUNC(regcomp,,AC_MSG_ERROR([cannot find regular expression library]))
dnl         else
dnl           AC_CHECK_FUNC(regcomp,use_included_tre=no)
dnl         fi
dnl       fi
dnl     fi
dnl   fi
dnl fi
dnl AM_CONDITIONAL(SUBDIR_TRE, test "$use_included_tre" = yes)
dnl if test "$use_included_tre" = "yes"; then
dnl   LIBREGEX="tre-0.7.2/lib/.libs/libtre.a"
dnl   AC_SUBST(LIBREGEX)
dnl   AC_CONFIG_SUBDIRS(tre-0.7.2)
dnl fi

dnl -------- WCHAR support revisited
dnl if test "$have_wchar" = yes; then
dnl   AC_DEFINE(HAVE_WCHAR_SUPPORT,1,[Define to 1 if you have wide characters, iconv, and regwcomp])
dnl fi

dnl -------- Tests for using libiberty or argv.c
use_included_argv=yes
if test "x$with_included_argv" != xyes; then
  AC_CHECK_LIB(iberty, buildargv,
    use_included_argv=no,
    use_included_argv=yes)
fi
AM_CONDITIONAL(USE_INCLUDED_ARGV, test "$use_included_argv" = yes)
if test "$use_included_argv" = yes; then
  LIBIBERTY_LIBS=""
  AC_SUBST(LIBIBERTY_LIBS)
else
  LIBIBERTY_LIBS="-liberty"
  AC_SUBST(LIBIBERTY_LIBS)
fi

dnl -------- Tests for using faad2 ------
AC_SUBST(FAAD_LIBS)
AC_CHECK_LIB(faad,NeAACDecDecode2,[
	FAAD_LIBS="-lfaad"
        AC_DEFINE(HAVE_FAAD,1,[Define to 1 if you have faad2 libraries installed])
	],
        AC_DEFINE(HAVE_FAAD,0,[Define to 1 if you have faad2 libraries installed]))
AC_SUBST(FAAD_LIBS)

dnl -------- Tests for curses ------
if test x$with_curses = xyes || test x$with_curses = x ; then
  found_curses=no
  if test x$found_curses = xno; then
    AC_CHECK_LIB(ncurses , initscr, [
      found_curses=yes
      with_curses=ncurses
    ])
  fi
  if test x$found_curses = xno; then
    AC_CHECK_LIB(curses  , initscr, [
      found_curses=yes
      with_curses=curses
    ])
  fi
  if test x$found_curses = xno; then
    AC_CHECK_LIB(ncursesw, initscr, [
      found_curses=yes
      with_curses=ncursesw
    ])
  fi
  if test x$found_curses = xno; then
    AC_CHECK_LIB(pdcurses, initscr, [
      found_curses=yes
      with_curses=pdcurses
    ])
  fi
  if test x$found_curses = xno; then
    with_curses=no
  fi
fi

case x$with_curses in
xcurses)
	CF_WITH_CURSES_DIR
	AC_MSG_NOTICE([Request for curses])
	;;
xncurses)
	CF_WITH_CURSES_DIR
	AC_MSG_NOTICE([Request for ncurses])
	;;
xncursesw)
	CF_WITH_CURSES_DIR
	AC_MSG_NOTICE([Request for ncursesw])
	;;
xpdcurses)
	AC_MSG_NOTICE([Request for pdncurses])
	;;
xno)
	AC_MSG_NOTICE([Request no curses])
	;;
esac

dnl << GCS -- left off debugging here >>
dnl exit

dnl -------- Tests for using included cdk ------
dnl CDK_LIBS=""
dnl use_included_cdk=no
dnl use_cdk=no
dnl if test "$with_included_cdk" = "yes"; then
dnl   use_included_cdk=yes
dnl   use_cdk=yes
dnl   CDK_LIBS="-lcdk -lncurses"
dnl   AC_DEFINE(HAVE_CDK,1,[Define to 1 if you have cdk libraries installed])
dnl else
dnl   dnl  LDFLAGS="$LDFLAGS -L/usr/local/lib"
dnl   dnl  INCLUDES="$INCLUDES -I/usr/local/include"
dnl   AC_CHECK_LIB(cdk,initCDKScreen,[
dnl     use_cdk=yes
dnl     CDK_LIBS="-lcdk -lncurses"
dnl     AC_DEFINE(HAVE_CDK,1,[Define to 1 if you have cdk libraries installed])
dnl   ],[],[-lncurses])
dnl fi
dnl AM_CONDITIONAL(SUBDIR_CDK, test "x$use_included_cdk" = "xyes")
dnl AC_SUBST(CDK_LIBS)

AM_CONDITIONAL([WANT_CSTREAMRIPPER],[test x$use_cdk != xno])

dnl -------- Tests for using ogg/vorbis ------
AC_SUBST(VORBIS_CFLAGS)
AC_SUBST(VORBIS_LIBS)
AC_SUBST(OGG_CFLAGS)
AC_SUBST(OGG_LIBS)
sr_have_ogg=no
XIPH_PATH_OGG([
  XIPH_PATH_VORBIS([
    AC_SUBST(OGG_CFLAGS)
    AC_SUBST(OGG_LIBS)
    AC_SUBST(VORBIS_CFLAGS)
    AC_SUBST(VORBIS_LIBS)
    AC_DEFINE(HAVE_OGG_VORBIS,1,[Define to 1 if you have both the ogg and vorbis libraries installed])
    sr_have_ogg=yes
    ],
    AC_DEFINE(HAVE_OGG_VORBIS,0,[Define to 1 if you have both the ogg and vorbis libraries installed]))],
  AC_DEFINE(HAVE_OGG_VORBIS,0,[Define to 1 if you have both the ogg and vorbis libraries installed])
  )

dnl -------- Check for semaphore library ------
AC_CHECK_FUNC(sem_init, , AC_CHECK_LIB(sem, sem_init,))
dnl Solaris needs this: -lrt
AC_CHECK_FUNC(sem_init, , AC_CHECK_LIB(rt, sem_init,))

dnl -------- Check for integer sizes ------
dnl uint32_t vs. u_int32_t
dnl ISO C 99 requires uint32_t, but u_int32_t is more common(?)
AC_CHECK_HEADERS(stdint.h)
AC_CHECK_HEADERS(inttypes.h)
AC_CHECK_TYPES([uint32_t, u_int32_t])
dnl (maybe these are needed in the future)
dnl AC_CHECK_SIZEOF(short)
dnl AC_CHECK_SIZEOF(int)
dnl AC_CHECK_SIZEOF(long)

dnl Sockets header needed on Solaris
AC_CHECK_HEADERS(sys/sockio.h)

dnl -- GCS: Taken from distcc --
dnl The following test taken from the cvs sources via Samba:
dnl If we can't find connect, try looking in -lsocket, -lnsl, and -linet.
dnl The Irix 5 libc.so has connect and gethostbyname, but Irix 5 also has
dnl libsocket.so which has a bad implementation of gethostbyname (it
dnl only looks in /etc/hosts), so we only look for -lsocket if we need
dnl it.
AC_CHECK_FUNCS(connect)
if test x"$ac_cv_func_connect" = x"no"; then
    case "$LIBS" in
    *-lnsl*) ;;
    *) AC_CHECK_LIB(nsl_s, printf) ;;
    esac
    case "$LIBS" in
    *-lnsl*) ;;
    *) AC_CHECK_LIB(nsl, printf) ;;
    esac
    case "$LIBS" in
    *-lsocket*) ;;
    *) AC_CHECK_LIB(socket, connect) ;;
    esac
    case "$LIBS" in
    *-linet*) ;;
    *) AC_CHECK_LIB(inet, connect) ;;
    esac
    dnl We can't just call AC_CHECK_FUNCS(connect) here, because the value
    dnl has been cached.
    if test x"$ac_cv_lib_socket_connect" = x"yes" || 
       test x"$ac_cv_lib_inet_connect" = x"yes"; then
        dnl ac_cv_func_connect=yes
        dnl don't!  it would cause AC_CHECK_FUNC to succeed next time configure is run
        AC_DEFINE(HAVE_CONNECT,1,[Whether the system has connect()])
    fi
fi
dnl (GCS: I don't think I need these)
dnl AC_CHECK_LIB(resolv, hstrerror, , , [-lnsl -lsocket])
dnl AC_CHECK_LIB(resolv, inet_aton, , , [-lnsl -lsocket])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(mkdir socket strerror strstr)

AC_OUTPUT(lib/Makefile Makefile)

cat <<EOF
* Streamripper configuration
    Using included MAD library?      $use_included_libmad
dnl    Using included CDK library?      $use_included_cdk
dnl    Using included GLIB library?     $use_included_glib
    Ogg/vorbis support?              $sr_have_ogg
EOF

