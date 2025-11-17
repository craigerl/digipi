dnl Process this file with autoconf to produce a configure script.
dnl $LastChangedDate: 2024-05-19 14:01:40 +0200 (Sun, 19 May 2024) $

AC_PREREQ([2.69])

# we'll move forward, one day or another
#AC_PREREQ([2.69])

# for future use, too
#AC_PROG_CC(cc gcc)
#AC_PROG_CXX(CC g++ gcc c++ cxx)

define([svnversion],esyscmd([sh -c "svnversion | tr  -c '[0-9]' ' ' | sed 's/ *//g;s/^$/-1/g' "]))dnl
# WARNING : here 0.0 denotes the package version, which will be corrected some lines after this.
AC_INIT([fim],[0.7.1],[dezperado_FOobAr_autistici_Baz_org, by replacing _FOobAr_ with a @ and _Baz_ with a .])
AC_CONFIG_SRCDIR([src/fim.cpp])

if test x"svnversion" != x ; then
	# The above test gets evaluated when creating the configure script.
	AC_DEFINE(SVN_REVISION,"svnversion",[SVN REVISION])
	AC_SUBST(SVN_REVISION,"svnversion")
	AC_DEFINE(SVN_REVISION_NUMBER,svnversion,[SVN REVISION NUMBER])
	AC_SUBST(SVN_REVISION_NUMBER,svnversion)
fi

AH_TEMPLATE([FIM_CONFIGURATION])
FIM_CONFIGURATION="$0 $@"
AC_SUBST(FIM_CONFIGURATION)
AC_DEFINE_UNQUOTED([FIM_CONFIGURATION],"$FIM_CONFIGURATION")

AC_MSG_NOTICE([
        Welcome to the Fim (revision $SVN_REVISION) configure script!
	This script will create the right Makefile for building Fim on your system.
	But first, it will check if your system has the right configuration for Fim to build.

	There are options to this script, please run :
	  ./configure --help=short
	for a complete list of these options.

	Please read BUGS and README and report any bug found to the Fim maintainer!
])


#AM_CONFIG_HEADER([config.h])
#AC_CONFIG_SRCDIR(pinentry/pinentry.h)

#
#AM_INIT_AUTOMAKE($PACKAGE_NAME, $PACKAGE_VERSION)
AC_USE_SYSTEM_EXTENSIONS
#AC_MANTAINER_MODE
#AC_CANONICAL_HOST

#missing_dir=`cd $ac_aux_dir && pwd`
#AM_MISSING_PROG(ACLOCAL, aclocal, $missing_dir)
#AM_MISSING_PROG(AUTOCONF, autoconf, $missing_dir)
#AM_MISSING_PROG(AUTOMAKE, automake, $missing_dir)
#AM_MISSING_PROG(AUTOHEADER, autoheader, $missing_dir)
#AM_MISSING_PROG(MAKEINFO, makeinfo, $missing_dir)



AC_PROG_CC
AC_PROG_CPP
AC_PROG_YACC
AC_PROG_LEX(noyywrap)

AC_PROG_INSTALL
AC_PROG_RANLIB

AC_REVISION([$Revision$])

AC_PROG_CXX
AC_PROG_CXXCPP
AC_CHECK_TOOL(AR, ar, ar)
fim_cv_version=`cat $srcdir/VERSION`
AC_LANG([C++])
#AC_LANG_PUSH([C++])
AM_INIT_AUTOMAKE
#AM_INIT_AUTOMAKE(fim, $fim_cv_version)
#AM_INIT_AUTOMAKE(fim, 1.0)
AC_SUBST([CONFIG_STATUS_DEPENDENCIES], ['$(top_srcdir)/VERSION'])


AC_SUBST(FIM_LIBS)
AC_SUBST(VERSION)
dnl AC_SUBST(HTMLDOCS)
#AC_LANG_POP([C++])

fim="fim"
AC_MSG_CHECKING([for prefix])
if test x$prefix = xNONE; then
  fim_cv_prefix=$ac_default_prefix
else
  fim_cv_prefix=$prefix
fi
AC_MSG_RESULT($fim_cv_prefix)

AC_SEARCH_LIBS([strerror],[cposix])
#AM_C_PROTOTYPES
if test "x$U" != "x"; then
  AC_MSG_ERROR(Compiler not ANSI compliant)
fi
#AC_PROG_CPP
AC_PROG_MAKE_SET

AC_C_INLINE
AC_C_CONST

AC_SYS_LARGEFILE
AC_FUNC_FSEEKO
AC_CHECK_SIZEOF(off_t)
AC_C_CHAR_UNSIGNED
AM_MAINTAINER_MODE

AH_TEMPLATE([HAVE_GETENV])
AC_CHECK_FUNC([getenv], AC_DEFINE([HAVE_GETENV], [1], [If present, the getenv function allows fim to read environment variables.]))
AC_CHECK_FUNC([getline], AC_DEFINE([HAVE_GETLINE], [1], [If present, will be used for stdin reading. (on GNU)]))
AC_CHECK_FUNC([getdelim], AC_DEFINE([HAVE_GETDELIM], [1], [If present, will be used for stdin reading. (on GNU)]))
AC_CHECK_FUNC([fgetln], AC_DEFINE([HAVE_FGETLN], [1], [If present, fgetln() will be used as a replacement for getline(). (on BSD)]))
AC_CHECK_FUNC([get_current_dir_name], AC_DEFINE([HAVE_GET_CURRENT_DIR_NAME], [1], [Should live when _GNU_SOURCE, but it could be not the case.]))
AC_CHECK_FUNC([rsb_lib_init])
AC_CHECK_FUNC([pipe])
AH_TEMPLATE([HAVE_MEMMEM])
AC_CHECK_FUNC([memmem], AC_DEFINE([HAVE_MEMMEM], [1], [Have memmem]))
AH_TEMPLATE([HAVE_SYNC])
AC_CHECK_FUNC([sync], AC_DEFINE([HAVE_SYNC], [1], [Have sync]))

AC_CHECK_LIB([m],[pow])
AC_CHECK_LIB([fl],[yylex])
AC_ARG_ENABLE(graphicsmagick, AS_HELP_STRING([--enable-graphicsmagick],[Enable GraphicsMagick library support (EXPERIMENTAL).]), [ if test x$enableval != xno ; then fim_handle_graphicsmagick=yes ; fi ] )
AC_CHECK_LIB([GraphicsMagick],[GetMagickVersion],
  [
if test x"$fim_handle_graphicsmagick" = x"yes" ; then 
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffMagick.o";
  LIBS="$LIBS `GraphicsMagick-config --cppflags --ldflags --libs|tr '\n' ' '` ";
  CXXFLAGS="$CXXFLAGS `GraphicsMagick-config --cppflags |tr '\n' ' '` ";
  AC_DEFINE([HAVE_LIBGRAPHICSMAGICK],[1],[We have libGraphicsMagick])
fi
  ], [fim_handle_graphicsmagick=no;])
AM_CONDITIONAL(WITH_LIBGRAPHICSMAGICK, test x$fim_handle_graphicsmagick = xyes)

AC_ARG_ENABLE(imlib2, AS_HELP_STRING([--enable-imlib2],[Enable Imlib2 library support (EXPERIMENTAL,INCOMPLETE).]), [ if test x$enableval != xno ; then fim_handle_imlib2=yes ; fi ] )
AC_CHECK_LIB([Imlib2],[imlib_create_image],
  [
if test x"$fim_handle_imlib2" = x"yes" ; then 
  AC_MSG_WARN([Imlib2 support is still INCOMPLETE, you should better use sdl!])
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS Imlib2Device.o";
  LIBS="$LIBS `imlib2-config --libs|tr '\n' ' '` ";
  CXXFLAGS="$CXXFLAGS `imlib2-config --cflags |tr '\n' ' '` ";
  AC_DEFINE([FIM_WITH_LIBIMLIB2],[1],[We have libImlib])
fi
  ], [fim_handle_imlib2=no;])
AM_CONDITIONAL(ENABLE_IMLIB2, test x"$fim_handle_imlib2" = x"yes")

AC_CHECK_PROG([HAVE_FLEX],[flex],[yes],[no])
AC_CHECK_PROG([HAVE_BISON],[bison],[yes],[no])
if test x$HAVE_FLEX  != x"yes" ; then
       	AC_MSG_ERROR([No flex (lexical analyser generator) executable detected!]) ;
fi
if test x$HAVE_BISON != x"yes" ; then
       	AC_MSG_ERROR([No bison (parser generator) executable detected!]) ;
fi

AC_CHECK_PROG([HAVE_ECHO],[echo],[yes],[no])
AC_CHECK_PROG([HAVE_RM],[rm],[yes],[no])
AC_CHECK_PROG([HAVE_SED],[sed],[yes],[no])
AC_CHECK_PROG([HAVE_GREP],[grep],[yes],[no])
AC_CHECK_PROG([HAVE_SORT],[sort],[yes],[no])
AC_CHECK_PROG([HAVE_CPP],[cpp],[yes],[no])
AC_CHECK_PROG([HAVE_AWK],[awk],[yes],[no])
AC_CHECK_PROG([HAVE_CAT],[cat],[yes],[no])
AC_CHECK_PROG([HAVE_MAN],[man],[yes],[no])
AC_CHECK_PROG([HAVE_GROFF],[groff],[yes],[no])
#AC_CHECK_PROG([HAVE_WHICH],[which],[yes],[no])
#AC_CHECK_PROG([HAVE_GPG],[gpg],[yes],[no])
#AC_CHECK_PROG([HAVE_LS],[ls],[yes],[no])
#AC_CHECK_PROG([HAVE_CD],[cd],[yes],[no])
#AC_CHECK_PROG([HAVE_CC],[cc],[yes],[no])
dnl
AC_ARG_VAR([LIBTIFF],[libtiff library (contributes to LIBS)])
AC_ARG_VAR([LIBGIF],[libgif library (contributes to LIBS)])
AC_ARG_VAR([LIBJPEG],[libjpeg library (contributes to LIBS)])
AC_ARG_VAR([LIBPS],[libspectre library (contributes to LIBS)])
AC_ARG_VAR([LIBDJVU],[libdjvu libdjvulibre (contributes to LIBS)])
AC_ARG_VAR([LIBPOPPLER],[poppler library (contributes to LIBS)])
dnl
AC_PATH_PROG([RM],[rm],[no])
AC_PATH_PROG([SED],[sed],[no])
AC_PATH_PROG([GREP],[grep],[no])
AC_PATH_PROG([ECHO],[echo],[no])
AC_PATH_PROG([SORT],[sort],[no])
AC_PATH_PROG([CPP],[cpp],[no])
AC_PATH_PROG([AWK],[awk],[no])
AC_PATH_PROG([CAT],[cat],[no])
AC_PATH_PROG([MAN],[man],[no])
AC_PATH_PROG([PERL],[perl],[no])
AC_PATH_PROG([SHELLCHECK],[shellcheck],[no])
AM_CONDITIONAL(WITH_SHELLCHECK, test x$ac_cv_path_SHELLCHECK != xno)
AM_CONDITIONAL(WITH_MAN2HTML, test x$ac_cv_path_MAN != xno)
AM_CONDITIONAL(WITH_PERL, test x$ac_cv_path_PERL != xno)
AM_CONDITIONAL(WITH_GROFF, test x$ac_cv_prog_HAVE_GROFF != xno)
AM_CONDITIONAL(WITH_MAN, test x$ac_cv_prog_HAVE_MAN != xno)
#AC_PATH_PROG([CC],[cc],[no])
#AC_PATH_PROG([WHICH],[which],[no])
#AC_PATH_PROG([GPG],[gpg],[no])
#AC_PATH_PROG([LS],[ls],[no])
#AC_PATH_PROG([CD],[cd],[no])
AC_PATH_PROG([EXPECT],[expect],[no])
AC_PATH_TOOL(PKG_CONFIG,pkg-config)
AM_CONDITIONAL(WITH_EXPECT, test x$ac_cv_path_EXPECT != xno)
# FIXME: what if we have no grep and/or no sed ?
# (echo is often a shell builtin)
have_default_bindings=yes
have_default_mapping=yes
have_default_config=yes

AC_ARG_ENABLE(default_bindings, AS_HELP_STRING([--disable-default-bindings],[Builtin default minimal key binding]),
[       if test x$enableval = xyes; then
		have_default_bindings=yes
	else
                have_default_bindings=no
        fi
])

AC_ARG_ENABLE(default_mapping, AS_HELP_STRING([--disable-default-mapping],[Builtin default minimal key mapping]),
[       if test x$enableval = xyes; then
                have_default_mapping=yes
	else
		have_default_mapping=no
        fi
])

AC_ARG_ENABLE(default_config, AS_HELP_STRING([--disable-default-config],[Builtin default minimal configuration]),
[       if test x$enableval = xyes; then
                have_default_config=yes
	else
		have_default_config=no
        fi
])

	AH_TEMPLATE([FIM_DEFAULT_CONFIG])
	if test x$have_default_bindings != xno; then
		FIM_DEFS="$FIM_DEFS -D FIM_DEFAULT_CONFIG"
		AC_DEFINE([FIM_DEFAULT_CONFIG])
	fi
	AH_TEMPLATE([FIM_DEFAULT_KEY_CONFIG])
	if test x$have_default_mapping != xno; then
		FIM_DEFS="$FIM_DEFS -D FIM_DEFAULT_KEY_CONFIG"
		AC_DEFINE([FIM_DEFAULT_KEY_CONFIG])
	fi
	AH_TEMPLATE([FIM_DEFAULT_CONFIGURATION])
	if test x$have_default_config != xno; then
		FIM_DEFS="$FIM_DEFS -D FIM_DEFAULT_CONFIGURATION"
		AC_DEFINE([FIM_DEFAULT_CONFIGURATION])
	fi

#CPPFLAGS=$DEPS
#AC_SUBST(LIBSDL_CXXFLAGS)
#AC_SUBST(LIBPNG_CXXFLAGS)
#AC_SUBST(LIBSDL_CXXFLAGS)
#AC_SUBST(LIBPNG_CXXFLAGS)
#AC_SUBST(LIBPNG_LIBS)

AC_ARG_ENABLE(system, AS_HELP_STRING([--disable-system],[Disable system() and popen() calls.]),
[       if test x$enableval = xno ; then
                fim_system_calls=no
        fi
])
AM_CONDITIONAL(ENABLE_SYSTEM_CALLS, test x$fim_system_calls != xno)
AH_TEMPLATE([FIM_WANT_SYSTEM_CALLS])
if test x$fim_system_calls != xno; then
	AC_DEFINE([FIM_WANT_SYSTEM_CALLS])
fi
AH_TEMPLATE([FIM_PIPE_IMAGE_READ])
if test x$fim_system_calls != xno; then
	AC_DEFINE([FIM_PIPE_IMAGE_READ])
fi
AH_TEMPLATE([FIM_WANT_POPEN_CALLS])
if test x$fim_system_calls != xno; then
	AC_DEFINE([FIM_WANT_POPEN_CALLS])
fi

dnl new: 
AC_ARG_ENABLE(framebuffer, AS_HELP_STRING([--disable-framebuffer],[Disable the framebuffer device driver.]),
[       if test x$enableval = xno ; then
                fim_handle_framebuffer=no
        fi
])

AC_MSG_NOTICE([Checking for headers required by the Linux framebuffer device.])
AC_CHECK_HEADERS([linux/fb.h], [break], AC_MSG_WARN([[Cannot find linux/fb.h.  Fim Linux framebuffer driver requires it!.]]))
AC_CHECK_HEADERS([linux/vt.h], [break], AC_MSG_WARN([[Cannot find linux/vt.h.  Fim Linux framebuffer driver requires it!.]]))
AC_CHECK_HEADERS([linux/kd.h], [break], AC_MSG_WARN([[Cannot find linux/kd.h.  Fim Linux framebuffer driver requires it!.]]))

if test x$ac_cv_header_linux_fb_h != xyes -o  \
	 x$ac_cv_header_linux_vt_h != xyes -o \
	 x$ac_cv_header_linux_kd_h != xyes ; then
	AC_MSG_WARN([Turning off Linux framebuffer support due to missing headers])
	fim_handle_framebuffer=no 
fi

if test x$fim_handle_framebuffer = xno ; then
	AC_DEFINE(FIM_WITH_NO_FRAMEBUFFER, 1, [Defined when framebuffer device support is turned off.])
fi
AM_CONDITIONAL(ENABLE_FRAMEBUFFER, test x$fim_handle_framebuffer != xno)

m4_if(m4_version_compare(m4_defn([AC_AUTOCONF_VERSION]),[2.72]),-1, [],[AC_CHECK_INCLUDES_DEFAULT])
AC_PROG_EGREP

AC_FUNC_MMAP

AC_CHECK_FUNCS([bzero memset memcmp strncmp strcpy])
AC_CHECK_FUNCS([bcopy bcmp])
AC_CHECK_FUNCS([drand48 random rand])
AC_CHECK_FUNCS([fileno])
AC_CHECK_FUNCS([fmemopen])
AC_CHECK_HEADERS([wordexp.h glob.h fnmatch.h])
dnl AC_FUNC_REALLOC()	dnl triggers error in automake 1.10.1

#AC_CHECK_HEADERS([stddef.h])
#AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h strings.h], [AC_CHECK_FUNCS(strerror)])

AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([wchar.h])
AC_CHECK_HEADERS([emscripten.h])
AC_CHECK_HEADERS([fontconfig/fontconfig.h])
AC_CHECK_HEADERS([fontconfig/fcfreetype.h])

AC_HEADER_DIRENT()
AC_CHECK_HEADERS([stdarg.h sys/ioctl.h ioctl.h sysexits.h])
AC_CHECK_HEADERS([sys/time.h sys/resource.h])
AC_CHECK_HEADERS([sys/select.h])
AC_CHECK_HEADERS([sys/wait.h])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([unix.h])
AC_CHECK_HEADERS([libgen.h])
AC_CHECK_HEADERS([sys/user.h])
AC_CHECK_HEADERS([sys/mman.h])
AC_CHECK_HEADERS([cstdio stdio.h], [break],AC_MSG_ERROR([[Cannot find cstdio or stdio.h.  Fim requires at least one or the other.]]))

AC_CHECK_HEADERS([climits limits.h], [break], AC_MSG_ERROR([[Cannot find limits.h nor climits.  Fim requires it!.]]))
AC_CHECK_HEADERS([filesystem], AC_DEFINE([HAVE_FILESYSTEM], [1], [Define to 1 if have <filesystem>]))

AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([int *])
AC_CHECK_SIZEOF([size_t])
AC_C_BIGENDIAN([fim_big_endian=yes],[fim_big_endian=no],[fim_big_endian=hmmm])

if test "x$fim_big_endian" = "xyes" ; then
	AH_TEMPLATE([FIM_BIG_ENDIAN])
	AC_DEFINE([FIM_BIG_ENDIAN])
fi


dnl ============= begin misc external programs detection
AC_CHECK_HEADERS([regex])
if test x$ac_cv_header_regex = xyes; then
	AH_TEMPLATE([HAVE_REGEX])
	AC_DEFINE([HAVE_REGEX],[1],[We have <regex>])
fi
AC_CHECK_HEADERS([_regex.h])
if test x$ac_cv_header__regex_h = xyes; then
	AH_TEMPLATE([HAVE__REGEX_H])
	AC_DEFINE([HAVE__REGEX_H],[1],[We have _regex.h])
fi
AC_CHECK_HEADERS([regex.h])
if test x$ac_cv_header_regex_h = xyes; then
	AH_TEMPLATE([HAVE_REGEX_H])
	AC_DEFINE([HAVE_REGEX_H],[1],[We have regex.h])
fi
AC_CHECK_HEADERS([termios.h])
if test x$ac_cv_header_termios_h = xyes; then
	AH_TEMPLATE([HAVE_TERMIOS_H])
	AC_DEFINE([HAVE_TERMIOS_H],[1],[We have termios.h])
fi
if test x$ac_cv_header_sys_ioctl_h = xyes; then
	AH_TEMPLATE([HAVE_SYS_IOCTL_H])
	AC_DEFINE([HAVE_SYS_IOCTL_H],[1],[We have sys/ioctl.h])
fi
if test x$ac_cv_header_sys_select_h = xyes; then
	AH_TEMPLATE([HAVE_SYS_SELECT_H])
	AC_DEFINE([HAVE_SYS_SELECT_H],[1],[We have sys/select.h])
fi
if test x$ac_cv_header_sys_wait_h = xyes; then
	AH_TEMPLATE([HAVE_SYS_WAIT_H])
	AC_DEFINE([HAVE_SYS_WAIT_H],[1],[We have sys/wait.h])
fi
if test x$ac_cv_func_pipe = xyes; then
	AH_TEMPLATE([HAVE_PIPE])
	AC_DEFINE([HAVE_PIPE],[1],[We have pipe()])
fi
dnl ==================
if test x$CC = x'emcc' -a x$CXX = x'em++'; then
	using_wasm=yes;
else
	using_wasm=no;
fi
dnl AC_CHECK_HEADERS([FlexLexer.h], [break], AC_MSG_ERROR([[Cannot find FlexLexer.  Fim requires it!]]))

AC_CHECK_HEADERS([FlexLexer.h], [break], AC_MSG_WARN([[Cannot find FlexLexer.h.  Without it Fim won't be usable!]]))
if test x$ac_cv_header_FlexLexer_h = x"no" -a x"$using_wasm" = x"yes" -a -r /usr/include/FlexLexer.h ; then
	ac_cv_header_FlexLexer_h=yes;
	cp /usr/include/FlexLexer.h .;
	AC_MSG_WARN([As an exception for WASM mode, copied /usr/include/FlexLexer.h here (it is not provided by emscripten).])
fi
if test x$ac_cv_header_FlexLexer_h = xyes; then
	AH_TEMPLATE([HAVE_FLEXLEXER_H])
	AC_DEFINE([HAVE_FLEXLEXER_H],[1],[We have FlexLexer.h])
fi
AM_CONDITIONAL(WITH_FLEXLEXER, test x$ac_cv_header_FlexLexer_h = xyes)
dnl ============= begin misc external programs detection
dnl ============= end misc external programs detection

dnl if test "X$ac_cv_header_cstdio" != "Xyes" && test "X$ac_cv_header_stdio_h" != "Xyes"; then
dnl   AC_MSG_ERROR([[Cannot find cstdio or stdio.h.  Fim requires at least one or the other.]])
dnl fi

dnl if test "X$ac_cv_header_linux_fb_h" != "Xyes" ; then
dnl   AC_MSG_ERROR([[Cannot find linux/fb.h.  Fim requires it!.]])
dnl fi



dnl AC_CHECK_FUNCS(setrlimit getsid)

dnl m4_warn([obsolete],[your code may safely assume C89 semantics that RETSIGTYPE is void.
dnl Remove this warning and the `AC_CACHE_CHECK' when you adjust the code.])dnl
dnl AC_CACHE_CHECK([return type of signal handlers],[ac_cv_type_signal],[AC_COMPILE_IFELSE(
dnl [AC_LANG_PROGRAM([#include <sys/types.h>
dnl #include <signal.h>
dnl ],
dnl 		 [return *(signal (0, 0)) (0) == 1;])],
dnl 		   [ac_cv_type_signal=int],
dnl 		   [ac_cv_type_signal=void])])
dnl AC_DEFINE_UNQUOTED([RETSIGTYPE],[$ac_cv_type_signal],[Define as the return type of signal handlers
dnl 		    (`int' or `void').])


dnl ==================
AH_TEMPLATE([FIM_WANT_STATIC_BINARY])
AC_ARG_ENABLE(static, AS_HELP_STRING([--enable-static],[Want a static binary. (disabled by default)]), [       if test x$enableval = xyes; then fim_cv_want_static=1 ; else fim_cv_want_static=0 ; fi ],[ fim_cv_want_static=0 ])
if test x$fim_cv_want_static = x1 ; then 
	LIBS="$LIBS --static"
        LIBDJVU_STATIC_EXTRA="-lpthread"
	# too many dependencies for the following!
        LIBPOPPLER_STATIC_EXTRA=""
        LIBSPECTRE_STATIC_EXTRA=""
        #LIBPOPPLER_STATIC_EXTRA="-lfreetype -llcms"
        #LIBSPECTRE_STATIC_EXTRA="-Wl,--start-group -lgs -lfontconfig -ldl -ljbig2dec -ljasper -lpthread -lpng -lppd -lpaper -lavahi-common -lavahi-client -lexpat -lfreetype -lgnutls -ldbus-1 -ltasn1 -lcupsimage -lcups -lgcrypt -Wl,--end-group "
	LIBTIFF_STATIC_EXTRA="-ljpeg -lz"
	AC_DEFINE_UNQUOTED([FIM_WANT_STATIC_BINARY], $fim_cv_want_static [Want a static binary. (disabled by default)])
fi
dnl ==================


AC_ARG_WITH(regex, AS_HELP_STRING([--with-regex],[Obsolete switch -- do not use this. (deprecated)]),
        [fim_cv_regex=yes],
        [AC_CHECK_FUNCS(regcomp, fim_cv_regex=no, fim_cv_regex=yes)])

if test X$ac_cv_header__regex_h = Xno ; then
	fim_cv_regex=no;
fi
if test X$fim_cv_regex = Xno ; then
AC_CACHE_CHECK([whether your system regexp library is completely broken],
        [fim_cv_regex_broken],
        AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <regex.h>
int main() { regex_t blah ; /*regmatch_t p; p.rm_eo = p.rm_eo;*/ const int rr = regcomp(&blah, "foo.*bar", REG_NOSUB) || regexec (&blah, "foobar", 0, NULL, 0); regfree(&blah); return rr; }]])],[fim_cv_regex_broken=no],[fim_cv_regex_broken=yes],[fim_cv_regex_broken=yes]))
        if test X$fim_cv_regex_broken = Xyes ; then
                echo "Using the included GNU regex instead." >&AS_MESSAGE_FD
                fim_cv_regex=yes
        fi
fi

if test X$fim_cv_regex = Xyes -a X$ac_cv_header__regex_h != Xno; then
        AC_DEFINE(USE_GNU_REGEX,1,[ Define if you want to use the included (GNU) regex.c. ])
        AC_LIBOBJ(regex)
fi



AC_MSG_CHECKING(where to put the documentation...)
dnl AC_ARG_WITH(docdir, AS_HELP_STRING([--with-docdir=PATH],[Specify where to put the documentation]),
dnl         [fim_cv_docdir=$withval],
dnl        [fim_cv_docdir='${prefix}/doc/fim-${fim_cv_version}'])
# FIXME : the above line seems troublesome, BUT IT IS NOT (see the muttrc for this trick).

AC_MSG_RESULT($fim_cv_docdir)

dnl docdir=$fim_cv_docdir
dnl AC_SUBST(docdir)
AC_SUBST(fim_cv_version)

dnl ============= gif file handling

AC_ARG_ENABLE(gif, AS_HELP_STRING([--disable-gif],[Disable gif support.]),
[       if test x$enableval = xno ; then
		fim_handle_gif=no
        fi
])
if test x$fim_handle_gif != xno ; then
  fim_handle_gif=yes
  have_gif=yes
  if test x$with_libgif != xno && test -z "$LIBGIF"; then
    AC_CHECK_LIB(gif,DGifOpenFileHandle ,
      have_gif=yes,
      have_gif=no
      AC_MSG_WARN([*** GIF loader will not be built (GIF library not found) ***]))
    if test x"$have_gif" = xyes; then
      AC_MSG_CHECKING([for gif.h])
      AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <gif_lib.h>]])],[have_gif=yes],[have_gif=no])
      AC_MSG_RESULT($have_gif)
      if test x"$have_gif" = xyes; then
        LIBGIF='-lgif'
      else
          AC_MSG_WARN([*** GIF loader will not be built (GIF header file not found) ***])
      fi
    fi
  fi

#  if test x$with_libgif != xno ; then
# the following needs thorough testing
  if test x"$have_gif" = xyes ; then
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffGif.o"
  LIBS="$LIBS $LIBGIF"
  AC_DEFINE([FIM_HANDLE_GIF])
  else
    fim_handle_gif=no
  fi

#  if test x$with_libgif != xno && test -z "$LIBGIF"; then
  #
#  fi
else
  fim_handle_gif=no
fi
AC_SUBST(LIBGIF)
AM_CONDITIONAL(ENABLE_GIF, test x$have_gif = xyes)


dnl ============= qoi file handling

AC_ARG_ENABLE(qoi, AS_HELP_STRING([--disable-qoi],[Disable qoi support.]),
[       if test x$enableval = xno ; then
		fim_handle_qoi=no
        fi
])
if test x$fim_handle_qoi != xno ; then
  AC_CHECK_HEADERS([qoi.h], [fim_handle_qoi=yes;], [fim_handle_qoi=no;])
  if test x$fim_handle_qoi != xno ; then
    AC_MSG_NOTICE([Enabling support of QOI.]);
    AC_DEFINE([FIM_HANDLE_QOI])
    AC_DEFINE([FIM_WITH_QOI], 1, [QOI file support.])
    FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffQoi.o"
  fi
fi
AC_SUBST(LIBQOI)
AM_CONDITIONAL(ENABLE_QOI, test x$have_qoi = xyes)


dnl ============= tiff file handling

AC_ARG_ENABLE(tiff, AS_HELP_STRING([--disable-tiff],[Disable tiff support.]),
[       if test x$enableval = xno ; then
		fim_handle_tiff=no
        fi
])
if test x$fim_handle_tiff != xno ; then
  fim_handle_tiff=yes
  have_tiff=yes
  if test x$with_libtiff != xno && test -z "$LIBTIFF"; then
    AC_CHECK_LIB(tiff,TIFFOpen ,
      have_tiff=yes,
      have_tiff=no
      AC_MSG_WARN([*** TIFF loader will not be built (TIFF library not found) ***]) , [$LIBTIFF_STATIC_EXTRA])
    if test x"$have_tiff" = xyes; then
      AC_MSG_CHECKING([for tiff.h])
      AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <tiff.h>]])],[have_tiff=yes],[have_tiff=no])
      AC_MSG_RESULT($have_tiff)
      if test x"$have_tiff" = xyes; then
        LIBTIFF="-ltiff $LIBTIFF_STATIC_EXTRA"
      else
          AC_MSG_WARN([*** TIFF loader will not be built (TIFF header file not found) ***])
      fi
    fi
  fi

#  if test x$with_libtiff != xno ; then
# the following needs thorough testing
  if test x"$have_tiff" = xyes ; then
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffTiff.o"
  LIBS="$LIBS $LIBTIFF"
  AC_DEFINE([FIM_HANDLE_TIFF])
  else
    fim_handle_tiff=no
  fi

#  if test x$with_libtiff != xno && test -z "$LIBTIFF"; then
  #
#  fi
else
  fim_handle_tiff=no
fi
AC_SUBST(LIBTIFF)
AM_CONDITIONAL(ENABLE_TIFF, test x$have_tiff = xyes)


dnl ============= curses: optional
AC_CHECK_LIB([curses],[noecho])
dnl ============= ncurses: optional
AC_CHECK_LIB([ncurses],[noecho])

dnl ============= GNU gpm, unofficial yet

#    AC_CHECK_LIB(gpm,Gpm_Open,
#      ,true,
#      AC_MSG_ERROR(*** gpm lib not found ... no problem ... ***))
#LIBS="$LIBS -lgpm"

dnl ============= no readline case handling

fim_want_readline=yes
AC_ARG_ENABLE(readline, AS_HELP_STRING([--disable-readline],[Disable readline support (EXPERIMENTAL).]),
[       if test x$enableval = xno ; then
		fim_want_readline=no
	else
		fim_want_readline=yes
        fi
])

dnl ============= GNU readline : essential

dnl Please note that when the readline library is compiled with the curses library, this check will fail, because
dnl we cannot know in advance if it is so.
AH_TEMPLATE([FIM_USE_READLINE])
AH_TEMPLATE([FIM_HANDLE_GIF])
AH_TEMPLATE([FIM_HANDLE_QOI])
AH_TEMPLATE([FIM_HANDLE_TIFF])

have_readline=no

# Sometimes the readline library is linked to the curses library, and we here try to detect this case..
if test x$have_readline = xno; then
  if test x$fim_want_readline = xyes ; then
    AC_CHECK_LIB(readline, rl_replace_line,
      have_readline=yes
      LIBS="$LIBS -lreadline"
      AC_DEFINE([FIM_USE_READLINE])
      ,have_readline=no
      AC_MSG_ERROR(*** GNU readline (or curses library ?) not found (used LIBS=${LIBS}) ! please install them or specify them in e.g. LIBS=... or run ./configure --disable-readline ***)
      ,[]
      )
  else
      have_readline=no
      AC_MSG_NOTICE(*** Fim will be built withoud GNU readline library headers ! Are you sure ?  ***)
  fi
fi

dnl FIXME : we try to detecht curses, although we do not know if the preceding did use it
if test x$have_readline = xyes; then
  if test x$fim_want_readline = xyes ; then
    AC_CHECK_LIB(curses, tputs,
      have_curses=yes
      LIBS="$LIBS"
      ,have_curses=no
      )
  fi
fi
AM_CONDITIONAL(WANT_READLINE, test x$have_readline = xyes)

dnl ============= jpeg file handling

#AC_ARG_WITH(jpeg, [  --disable-jpeg ], [ disable jpeg file support])
AC_ARG_ENABLE(jpeg, AS_HELP_STRING([--disable-jpeg],[Disable jpeg support.]),
[       if test x$enableval = xno ; then
		fim_handle_jpeg=no
        fi
])
if test x$fim_handle_jpeg != xno ; then
  fim_handle_jpeg=yes
  have_jpeg=yes
  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
    AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
      have_jpeg=yes,
      have_jpeg=no
      AC_MSG_WARN([*** JPEG loader will not be built (JPEG library not found) ***]))
    if test x"$have_jpeg" = xyes; then
      AC_MSG_CHECKING([for jpeglib.h])
      AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>]])],[have_jpeg=yes],[have_jpeg=no])
      AC_MSG_RESULT($have_jpeg)
      if test x"$have_jpeg" = xyes; then
        LIBJPEG='-ljpeg'
      else
          AC_MSG_WARN([*** JPEG loader will not be built (JPEG header file not found) ***])
      fi
    fi
  fi

#  if test x$with_libjpeg != xno ; then
# the following needs thorough testing
  if test x"$have_jpeg" = xyes ; then
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffJpeg.o"
  LIBS="$LIBS $LIBJPEG"
  AC_DEFINE([HAVE_LIBJPEG],[1],[We have libjpeg])
  else
    fim_handle_jpeg=no
  fi

#  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
#     AC_MSG_ERROR([
#     Checks for JPEG loader failed. You can build without it by passing
#    --disable-jpeg to ./configure. ])
#  fi
else
  fim_handle_jpeg=no
fi
AC_SUBST(LIBJPEG)
AM_CONDITIONAL(ENABLE_JPEG, test x$fim_handle_jpeg = xyes)

dnl ============= dvi file handling (via TeX-Guy)
dnl 
dnl AC_ARG_ENABLE(dvi, AS_HELP_STRING([--disable-dvi],[Disable dvi support.]),
dnl [       if test x$enableval = xno ; then
dnl 		fim_handle_dvi=no
dnl         fi
dnl ])
dnl if test x$fim_handle_dvi != xno ; then
dnl   fim_handle_dvi=yes
dnl   have_dvi=yes
dnl   if test x$with_libdvi != xno && test -z "$LIBDVI"; then
dnl     AC_CHECK_LIB(dvilib2, DVI_init,
dnl       have_dvi=yes,
dnl       have_dvi=no
dnl       AC_MSG_WARN([*** DVI loader will not be built (libdvi29 library not found) ***]))
dnl     if test x"$have_dvi" = xyes; then
dnl       AC_MSG_CHECKING([for libdvi29.h])
dnl       AC_PREPROC_IFELSE([AC_LANG_SOURCE([[dnl #include <stdio.h>
dnl #undef PACKAGE
dnl #undef VERSION
dnl #undef HAVE_STDLIB_H
dnl #include <libdvi29.h>]])],[dnl         have_dvi=yes],[dnl         have_dvi=no])
dnl       AC_MSG_RESULT($have_dvi)
dnl       if test x"$have_dvi" = xyes; then
dnl         LIBDVI='-ldvilib2'
dnl       else
dnl           AC_MSG_WARN([*** DVI loader will not be built (libdvi29.h header file not found) ***])
dnl       fi
dnl     fi
dnl   fi
dnl 
dnl # the following needs thorough testing
dnl   if test x"$have_dvi" = xyes ; then
dnl   FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffDvi.o"
dnl   LIBS="$LIBS $LIBDVI"
dnl   AC_DEFINE([HAVE_LIBSPECTRE],[1],[We have libdvilib2])
dnl   else
dnl     fim_handle_dvi=no
dnl   fi
dnl 
dnl #  if test x$with_libdvi != xno && test -z "$LIBDVI"; then
dnl #     AC_MSG_ERROR([
dnl #     Checks for DVI loader failed. You can build without it by passing
dnl #    --disable-dvi to ./configure. ])
dnl #  fi
dnl else
dnl   fim_handle_dvi=no
dnl fi
dnl AC_SUBST(LIBDVI)
dnl AM_CONDITIONAL(ENABLE_DVI, test x$have_dvi = xyes)

dnl ============= ps file handling

AC_ARG_ENABLE(ps, AS_HELP_STRING([--disable-ps],[Disable ps support.]),
[       if test x$enableval = xno ; then
		fim_handle_ps=no
        fi
])
if test x$fim_handle_ps != xno ; then
  fim_handle_ps=yes
  have_ps=yes
  if test x$with_libps != xno && test -z "$LIBPS"; then
    AC_CHECK_LIB(spectre, spectre_page_render,
      have_ps=yes,
      have_ps=no
      AC_MSG_WARN([*** PS loader will not be built (spectre library not found) ***]) , [$LIBSPECTRE_STATIC_EXTRA])
    if test x"$have_ps" = xyes; then
      AC_MSG_CHECKING([for libspectre/spectre.h])
      AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <libspectre/spectre.h>]])],[have_ps=yes],[have_ps=no])
      AC_MSG_RESULT($have_ps)
      if test x"$have_ps" = xyes; then
        LIBPS="-lspectre $LIBSPECTRE_STATIC_EXTRA"
      else
          AC_MSG_WARN([*** PS loader will not be built (PS header file not found) ***])
      fi
    fi
  fi

# the following needs thorough testing
  if test x"$have_ps" = xyes ; then
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffPs.o"
  LIBS="$LIBS $LIBPS"
  AC_DEFINE([HAVE_LIBSPECTRE],[1],[We have libspectre])
  else
    fim_handle_ps=no
  fi

#  if test x$with_libps != xno && test -z "$LIBPS"; then
#     AC_MSG_ERROR([
#     Checks for PS loader failed. You can build without it by passing
#    --disable-ps to ./configure. ])
#  fi
else
  fim_handle_ps=no
fi
AC_SUBST(LIBPS)
AM_CONDITIONAL(ENABLE_PS, test x$have_ps = xyes)



dnl ============= djvu file handling

AC_ARG_ENABLE(djvu, AS_HELP_STRING([--disable-djvu],[Disable djvu support.]),
[       if test x$enableval = xno ; then
		fim_handle_djvu=no
        fi
])
if test x$fim_handle_djvu != xno ; then
  fim_handle_djvu=yes
  have_djvu=yes
  if test x$with_libdjvu != xno && test -z "$LIBDJVU"; then
    AC_CHECK_LIB(djvulibre, ddjvu_page_create_by_pageno,
      have_djvu=yes,
      have_djvu=no
      AC_MSG_WARN([*** DJVU loader will not be built (DJVU library not found) ***]), [$LIBDJVU_STATIC_EXTRA])
    if test x"$have_djvu" = xyes; then
      AC_MSG_CHECKING([for libdjvu/ddjvuapi.h])
      AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <libdjvu/ddjvuapi.h>]])],[have_djvu=yes],[have_djvu=no])
      AC_MSG_RESULT($have_djvu)
      if test x"$have_djvu" = xyes; then
        LIBDJVU="-ldjvulibre $LIBDJVU_STATIC_EXTRA"
      else
          AC_MSG_WARN([*** DJVU loader will not be built (DJVU header file not found) ***])
      fi
    fi
  fi

# the following needs thorough testing
  if test x"$have_djvu" = xyes ; then
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffDjvu.o"
  LIBS="$LIBS $LIBDJVU"
  AC_DEFINE([HAVE_LIBDJVU],[1],[We have libdjvulibre])
  else
    fim_handle_djvu=no
  fi

#  if test x$with_libdjvu != xno && test -z "$LIBDJVU"; then
#     AC_MSG_ERROR([
#     Checks for DJVU loader failed. You can build without it by passing
#    --disable-djvu to ./configure. ])
#  fi
else
  fim_handle_djvu=no
fi
AC_SUBST(LIBDJVU)
AM_CONDITIONAL(ENABLE_DJVU, test x$have_djvu = xyes)


dnl ============= pdf file handling with libpoppler 
dnl
fim_handle_pdf=no
AC_ARG_ENABLE(poppler, AS_HELP_STRING([--enable-poppler],[Enable poppler (PDF rendering) support. This will try to detect extra compilation flags. You can override this by passing configure with POPPLER_CFLAGS=... .]),
[       if test x$enableval = xyes ; then
		fim_handle_pdf=yes
        fi
], [$LIBPOPPLER_STATIC_EXTRA])
dnl FIXME : checking only for libpoppler-glib, should also check for libpoppler !
if test x$fim_handle_pdf != xno ; then
	CPPFLAGS_precheck="$CPPFLAGS";
        if test -z "$LIBPOPPLER"; then
    	# dear fim hackers, you should install pkg-config to get this macro expanded (p)
	# I admit I don't like this much
	CXXFLAGS="$CXXFLAGS `$PKG_CONFIG poppler        --cflags`";
	CPPFLAGS="$CPPFLAGS `$PKG_CONFIG poppler        --cflags-only-I`";
	CXXFLAGS="$CXXFLAGS `$PKG_CONFIG poppler-splash --cflags`";        dnl old debian
	CPPFLAGS="$CPPFLAGS `$PKG_CONFIG poppler-splash --cflags-only-I`"; dnl old debian
	CXXFLAGS="$CXXFLAGS `$PKG_CONFIG poppler-cpp    --cflags`";
	CPPFLAGS="$CPPFLAGS `$PKG_CONFIG poppler-cpp    --cflags-only-I`";
    	POPPLER_REQUIRED=0.8.7
    	PKG_CHECK_MODULES(POPPLER, poppler >= $POPPLER_REQUIRED,[],[]) dnl sets POPPLER_LIBS and POPPLER_CFLAGS
    	PKG_CHECK_MODULES(GLIB, glib-2.0) dnl sets GLIB_CFLAGS GLIB_LIBS
dnl keep next line empty or it may break configure.

          AC_CHECK_LIB(poppler, g_free,
            have_poppler=yes
            LIBS="$LIBS $GLIB_LIBS $POPPLER_LIBS";
            CXXFLAGS="$CXXFLAGS $GLIB_CFLAGS $POPPLER_CFLAGS";
            CPPFLAGS="$CPPFLAGS $GLIB_CFLAGS $POPPLER_CFLAGS";
	    ,
            have_poppler=no
      AC_MSG_WARN([*** POPPLER loader will not be built (POPPLER library not found) ***]), [$GLIB_LIBS $POPPLER_LIBS])
    if test x"$have_poppler" = xyes; then
      AC_MSG_CHECKING([for  <cpp/poppler-version.h> <splash/SplashBitmap.h> <splash/SplashTypes.h> <poppler-config.h> <OutputDev.h> <PDFDoc.h> <SplashOutputDev.h> <Page.h> <GlobalParams.h> <glib.h> ...])
       dnl requires proper CPPFLAGS
       AC_PREPROC_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
       #undef PACKAGE
       #undef VERSION
       #undef HAVE_STDLIB_H
       #include <cpp/poppler-version.h>
       #include <splash/SplashBitmap.h>
       #include <splash/SplashTypes.h>
       #include <poppler-config.h>
       #include <OutputDev.h>
       #include <PDFDoc.h>
       #include <SplashOutputDev.h>
       #include <Page.h>
       #include <GlobalParams.h>
       #include <glib.h>
       ]])],[have_poppler=yes],[have_poppler=no])
      AC_MSG_RESULT($have_poppler)
      if test x"$have_poppler" = xyes; then
        CXXFLAGS="$CXXFLAGS -DFIM_LIBPOPPLER_VERSION_MAYBE=$have_poppler_maybe_version"
      else
          AC_MSG_WARN([*** POPPLER/PDF loader will not be built (POPPLER header file not found) ***])
      fi
    fi
    CPPFLAGS="$CPPFLAGS_precheck"; # restore: it's in CXXFLAGS anyway
  fi

# the following needs thorough testing
  if test x"$have_poppler" = xyes ; then
  FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffPdf.o"
  LIBS="$LIBS $LIBPOPPLER"
  AC_DEFINE([HAVE_LIBPOPPLER],[1],[We have libpoppler (Warning: the API could still break! (as of v.0.8.7--0.24.1))])
  else
    fim_handle_pdf=no
    AC_MSG_ERROR([*** Could not detect POPPLER, which was requested via --enable-poppler. Please do not use that, or use --disable-poppler ***])
  fi

else
  AC_MSG_NOTICE([POPPLER support (for PDF) not requested.])
  fim_handle_pdf=no
fi
AM_CONDITIONAL(ENABLE_POPPLER, test x$have_poppler = xyes)

dnl ================== pdf checks ===================================================
AC_ARG_ENABLE(pdf,
	      [AS_HELP_STRING([--enable-pdf],[Compile with pdf support (this does *not* imply detection of libpoppler)])],enable_pdf="$enableval",enable_pdf=yes)

if test x"$enable_pdf" = x"yes" -o x"$fim_handle_pdf" = x"yes"; then
    if test x"$enable_pdf" = x"yes" -a x"$fim_handle_pdf" != x"no" ; then
            AC_DEFINE([ENABLE_PDF], [1], [Enable pdf support.])
            SHELL_LIBS="$SHELL_LIBS $POPPLER_LIBS"
            SHELL_CXXFLAGS="$SHELL_CXXFLAGS $GLIB_CFLAGS $POPPLER_CFLAGS"

            fim_save_LIBS=$LIBS
            LIBS="$LIBS $GLIB_LIBS $POPPLER_LIBS"
            LIBS=$fim_save_LIBS
    fi
    if test x"$enable_pdf" = x"yes" -a x"$fim_handle_pdf" != x"yes" ; then
            AC_MSG_WARN(["PDF support can' be granted since poppler library version $POPPLER_REQUIRED or newer not found or not requested; perhaps can still open PDF with converters..."])
	    enable_pdf=no;
    fi
    if test x"$enable_pdf" != x"yes" -a x"$fim_handle_pdf" = x"yes" ; then
            AC_MSG_WARN(["Disabling PDF support as requested"])
	    fim_handle_pdf=no;
    fi
fi

AM_CONDITIONAL(ENABLE_PDF, test x$enable_pdf = xyes)
dnl ================== end of pdf checks ============================================

dnl ================================================================================
fim_handle_bmp=yes
AC_ARG_ENABLE(bmp, AS_HELP_STRING([--disable-bmp],[Disable BMP file support (enabled by default).]),
[       if test x$enableval = xno ; then
		fim_handle_bmp=no
        fi
])
if test x$fim_handle_bmp = xyes; then
	AC_MSG_NOTICE([Enabling BMP file format support.])
	AC_DEFINE(FIM_WITH_BMP, 1, [BMP file support.])
	FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffBmp.o"
else
	AC_MSG_NOTICE([Disabling BMP file format support.])
	AC_DEFINE(FIM_WITH_BMP, 0, [BMP file support.])
fi
AM_CONDITIONAL(ENABLE_BMP, test x$fim_handle_bmp = xyes)
dnl ================================================================================
fim_handle_pcx=yes
AC_ARG_ENABLE(pcx, AS_HELP_STRING([--disable-pcx],[Disable PCX file support (enabled by default).]),
[       if test x$enableval = xno ; then
		fim_handle_pcx=no
        fi
])
if test x$fim_handle_pcx = xyes; then
	AC_MSG_NOTICE([Enabling PCX file format support.])
	AC_DEFINE(FIM_WITH_PCX, 1, [PCX file support.])
	FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffPcx.o"
else
	AC_MSG_NOTICE([Disabling PCX file format support.])
	AC_DEFINE(FIM_WITH_PCX, 0, [PCX file support.])
fi
AM_CONDITIONAL(ENABLE_PCX, test x$fim_handle_pcx = xyes)

dnl ================================================================================
## webp file handling
AC_CHECK_HEADERS([webp/decode.h webp/demux.h], [fim_handle_webp=yes;], [fim_handle_webp=no;])
AC_ARG_ENABLE(webp, AS_HELP_STRING([--disable-webp],[Disable WebP file support.]),
[       if test x$enableval = xno ; then
		fim_handle_webp=no
        fi
])
if test x$fim_handle_webp = xyes; then
	LIBS="$LIBS `$PKG_CONFIG libwebpdemux --libs`";
	LIBS="$LIBS `$PKG_CONFIG libwebp      --libs`";
	CXXFLAGS="$CXXFLAGS `$PKG_CONFIG libwebpdemux --cflags`";
	CXXFLAGS="$CXXFLAGS `$PKG_CONFIG libwebp      --cflags`";
	AC_MSG_NOTICE([Enabling WebP file format support.])
	AC_DEFINE(FIM_WITH_LIBWEBP, 1, [WebP file support.])
	FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffWebP.o"
else
	AC_MSG_NOTICE([Disabling PCX file format support.])
	AC_DEFINE(FIM_WITH_LIBWEBP, 0, [WebP file support.])
fi
AM_CONDITIONAL(ENABLE_WEBP, test x$fim_handle_webp = xyes)

dnl ================================================================================
## avifile file handling
AC_CHECK_HEADERS([avif/avif.h], [fim_handle_avif=yes;], [fim_handle_avif=no;])
AC_ARG_ENABLE(avif, AS_HELP_STRING([--disable-avif],[Disable AVIF file support.]),
[       if test x$enableval = xno ; then
		fim_handle_avif=no
        fi
])
if test x$fim_handle_avif = xyes; then
	LIBS="$LIBS `$PKG_CONFIG libavif --libs`";
	CXXFLAGS="$CXXFLAGS `$PKG_CONFIG libavif --cflags`";
	AC_MSG_NOTICE([Enabling AVIF file format support.])
	AC_DEFINE(FIM_WITH_LIBAVIF, 1, [AVIF file support.])
	FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffAVIF.o"
else
	AC_MSG_NOTICE([Disabling AVIF file format support.])
	AC_DEFINE(FIM_WITH_LIBAVIF, 0, [AVIF file support.])
fi
AM_CONDITIONAL(ENABLE_AVIF, test x$fim_handle_avif = xyes)

dnl ================================================================================

## png file handling

AC_ARG_WITH(png-prefix, AS_HELP_STRING([--with-png-prefix=PFX],[prefix where libpng is installed (optional)]),
libpng_config_prefix="$withval", libpng_config_prefix="")

fim_handle_png=yes

AC_ARG_ENABLE(png, AS_HELP_STRING([--disable-png],[Disable png support.]),
[       if test x$enableval = xno ; then
		fim_handle_png=no
        fi
])

if test x$fim_handle_png = xyes; then
	# support for custom library configurator path
        if test x$libpng_config_prefix != x; then
                LIBPNG_CONFIG="$libpng_config_prefix/bin/libpng-config"
        else
		# default path configurator
                AC_PATH_PROG(LIBPNG_CONFIG, libpng-config, no)
        fi
        if test "x$LIBPNG_CONFIG" = "xno"; then
      		AC_MSG_WARN([*** PNG loader will not be built (PNG library not found) ***])
		fim_handle_png=no
        else
                CXXFLAGS="$CXXFLAGS `$LIBPNG_CONFIG --cflags`"
		if test x$fim_cv_want_static = x1 ; then 
                LIBS="$LIBS `$LIBPNG_CONFIG --static --libs`"
		else
                LIBS="$LIBS `$LIBPNG_CONFIG --libs`"
		fi
		# finish this !
                FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffPng.o"
                AC_DEFINE(FIM_WITH_LIBPNG, 1, [Defined, if libpng support is enabled])
        fi
fi
AC_SUBST(LIBPNG)
AM_CONDITIONAL(ENABLE_PNG, test x$fim_handle_png = xyes)

#AM_CONDITIONAL(BUILD_PNG, test x$fim_handle_png = xyes)

dnl ================================================================================

fim_handle_xyz=no

AC_ARG_ENABLE(xyz, AS_HELP_STRING([--enable-xyz],[Enable the sample bogus 'xyz' file loading support. This makes only sense in order to learn how to include a new file format and wish to play with src/FbiStuffXyz.cpp in the while.]),
[       if test x$enableval = xyes ; then
		fim_handle_xyz=yes
        fi
])

if test x$fim_handle_xyz = xyes; then
	LIBXYZ="" # one would set an actual value here
	LIBS="$LIBS $LIBXYZ"
	FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffXyz.o"
	AC_DEFINE(FIM_WITH_LIBXYZ, 1, [Defined if sample bogus 'xyz' loader support is enabled.])
fi

dnl ================================================================================

## raw/nef file handling via UFRaw

fim_handle_ufraw=no

AC_ARG_ENABLE(ufraw, AS_HELP_STRING([--enable-ufraw],[Enable UFRaw support (EXPERIMENTAL).]),
[       if test x$enableval = xno ; then
		fim_handle_ufraw=no
	else
		fim_handle_ufraw=yes
        fi
])

if test x$fim_handle_ufraw = xyes; then
	# support for custom library configurator path
	CXXFLAGS="$CXXFLAGS $UFRAW_CXXFLAGS"
	LIBS="$LIBS $UFRAW_LIBS"
	FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffUFRaw.o" # FIXME
	AC_DEFINE(FIM_WITH_UFRAW, 1, [Defined, if UFRaw support is enabled])
	AC_MSG_NOTICE([Enabling UFRaw / libufraw.])
fi

dnl ================================================================================

## archive file handling via libarchive

fim_handle_archive=no

AC_ARG_ENABLE(archive, AS_HELP_STRING([--enable-archive],[Enable libarchive support to view archive files as multipage documents (please provide ARCHIVE_LIBS, ARCHIVE_CXXFLAGS) (EXPERIMENTAL, INCOMPLETE).]),
[       if test x$enableval = xno ; then
		fim_handle_archive=no
	else
		fim_handle_archive=yes
        fi
])

if test x$fim_handle_archive = xyes; then
	# support for custom library configurator path
	CXXFLAGS="$CXXFLAGS $ARCHIVE_CXXFLAGS"
	LIBS="$LIBS $ARCHIVE_LIBS -larchive"
	AC_DEFINE(FIM_WITH_ARCHIVE, 1, [Defined, if (EXPERIMENTAL, INCOMPLETE) libarchive support is enabled])
	AC_MSG_NOTICE([Enabling libarchive.])
fi
AM_CONDITIONAL(ENABLE_ARCHIVE, test x$fim_handle_archive = xyes)

dnl ================================================================================

AC_CHECK_HEADERS([libexif/exif-data.h])
AC_CHECK_LIB(exif, exif_data_new_from_data,have_libexif=yes,have_libexif=no)
AC_ARG_ENABLE(exif, AS_HELP_STRING([--disable-exif],[JPEG EXIF tags support with libexif.]), [if test x$enableval != xno; then fim_cv_libexif=yes; fi],[fim_cv_libexif=yes;])


if test x$fim_cv_libexif = xyes; then
	if test x$have_libexif = xyes -a x$ac_cv_header_libexif_exif_data_h = xyes; then
      			AC_MSG_NOTICE([Enabling libexif usage.])
			CXXFLAGS="$CXXFLAGS";
			LIBS="$LIBS -lexif";
	else
          		AC_MSG_WARN([*** libexif not was not detected, so cannot use it ***])
			fim_cv_libexif=no;
	fi
else
	fim_cv_libexif=no;
fi

dnl ================================================================================

AC_ARG_ENABLE(jasper, AS_HELP_STRING([--enable-jasper],[Turns on JPEG-2000 files support using JasPer (EXPERIMENTAL)]),
	[if test x$enableval = xyes; then
		fim_cv_libjasper=yes;
		CXXFLAGS="$CXXFLAGS";
		LIBS="$LIBS -ljasper";
		AC_CHECK_LIB(jasper, jas_init,have_libjasper=yes,have_libjasper=no)
		if test x$have_libjasper = xyes; then
			AC_MSG_NOTICE([Enabling support of JPEG-2K files with JasPer.]);
  			FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS FbiStuffJasPer.o";
		else
			fim_cv_libjasper=no;
		        AC_MSG_ERROR([Sorry, it does not seem like libjasper is installed or detectable.
			Please run --disable-jasper or install libjasper in a system-wide directory instead.
			]);
		fi
	fi],[fim_cv_libjasper=no;])

dnl ================================================================================

AC_CHECK_LIB(aa, aa_getfirst,fim_cv_libaa=yes,fim_cv_libaa=no)
AC_ARG_ENABLE(aa, AS_HELP_STRING([--disable-aa],[ASCII-art (-o aa) support (via aalib)]), [if test x$enableval != xno; then fim_cv_libaa=yes; else fim_cv_libaa=no; fi],[fim_cv_libaa=yes;])

if test x$fim_cv_libaa != xno; then
	if test x$ac_cv_lib_aa_aa_getfirst = xyes; then
      		AC_MSG_NOTICE([Enabling aalib driver.])
		fim_cv_aalib=yes
	        if test x$libaa_config_prefix != x; then
			LIBAA_CONFIG="$libaa_config_prefix/bin/aalib-config"
		else
			# default path configurator
			AC_PATH_PROG(LIBAA_CONFIG, aalib-config, no)
		fi
		if test "x$LIBAA_CONFIG" = "xno"; then
			# uhm
	      		AC_MSG_WARN([***  no aalib-config ? ***])
			fim_cv_aalib=no
		else
			CXXFLAGS="$CXXFLAGS `$LIBAA_CONFIG --cflags`"
			LIBS="$LIBS `$LIBAA_CONFIG --libs`"
			# finish this !
		fi
	else
		fim_cv_aalib=no
	fi
else
	fim_cv_aalib=no
	AC_MSG_WARN([ASCII-art (-o aa) support disabled.])
        dnl AC_MSG_ERROR([Sorry, it does not seem like aalib is installed or detectable.
	dnl Please run --disable-aa or install aalib in a system-wide directory instead.
	dnl ])
fi

dnl ================================================================================
AC_CHECK_LIB([stdc++fs],[main],[LIBS="${LIBS} -lstdc++fs"])
dnl ================================================================================

## sdl driver

AC_ARG_WITH(sdl-prefix, AS_HELP_STRING([--with-sdl-prefix=PFX],[prefix where libsdl is installed (optional)]),
libsdl_config_prefix="$withval", libsdl_config_prefix="")

fim_handle_sdl='auto'

dnl AC_ARG_ENABLE(sdl, AS_HELP_STRING([--disable-sdl],[Disable libsdl support.]),
dnl [       if test x$enableval = xno ; then
dnl 		fim_handle_sdl=no
dnl         fi
dnl ])

sdl_version_preference=''
AC_ARG_ENABLE(sdl, AS_HELP_STRING([--disable-sdl],[Disable libsdl support. Or --enable-sdl=1 --enable-sdl=2 to give preference to one those two versions. Requires either sdl-config or sdl2-config. Some configurations require prepending PKG_CONFIG=pkg-config before configure, for sdl2-config to work.]),
sdl_version_preference="$enableval";
[       if test x$enableval != xno ; then fim_handle_sdl=yes ; fi ]
[       if test x$enableval = xno  ; then fim_handle_sdl=no  ; fi ])

# tested with sdl 1.2
if test x$fim_handle_sdl != xno; then
	# support for custom library configurator path
	if test x"${sdl_version_preference}" != x2; then
        	if test x$libsdl_config_prefix != x; then
        	        LIBSDL_CONFIG="$libsdl_config_prefix/bin/sdl-config"
        	else
        	        AC_PATH_PROG(LIBSDL_CONFIG, sdl-config, no)
        	fi
	fi
	if test x"${sdl_version_preference}" != x1; then
        	if test x$libsdl_config_prefix != x; then
        	        LIBSDL2_CONFIG="$libsdl_config_prefix/bin/sdl2-config"
        	else
        	        AC_PATH_PROG(LIBSDL2_CONFIG, sdl2-config, no)
        	fi
		if test x$LIBSDL2_CONFIG != x -a x$LIBSDL2_CONFIG != x"no" ; then
			LIBSDL_CONFIG="$LIBSDL2_CONFIG";
		fi
		LIBS_BEFORE_CHECK="$LIBS"
		export LIBS="$LIBS `$LIBSDL2_CONFIG --static-libs`"
		AC_CACHE_CHECK([Whether sdl2-config gives linkable output with $LIBS], [have_cv_sdl2_config_ok], AC_LINK_IFELSE([AC_LANG_PROGRAM([/*headers*/], [/*main*/])],[have_cv_sdl2_config_ok=yes],[have_cv_sdl2_config_ok=no],[have_cv_sdl2_config_ok=no]))
		LIBS="$LIBS_BEFORE_CHECK"
		unset LIBS_BEFORE_CHECK
dnl		if test x"$have_cv_sdl2_config_ok" = xno && test x"$LIBSDL2_CONFIG" != x && pkg-config --version; then
dnl			AC_MSG_NOTICE([Setting PKG_CONFIG=pkg-config, perhaps it can help sdl-config.])
dnl			export PKG_CONFIG=pkg-config;
dnl		fi
		unset LIBSDL2_CONFIG;
	fi
        if test "x$LIBSDL_CONFIG" = "xno"; then
		if test x$fim_handle_sdl = xyes; then
                	AC_MSG_ERROR([LIBSDL not found (failed detecting the sdl configuration (sdl-config/sdl2-config) program)])
		else
			fim_handle_sdl='no'
		fi
        else
		AC_MSG_NOTICE([Activating LIBSDL (use $LIBSDL_CONFIG).])
		fim_handle_sdl='yes'
                LIBSDL_CXXFLAGS=`$LIBSDL_CONFIG --cflags`
                CXXFLAGS="$CXXFLAGS $LIBSDL_CXXFLAGS"
		LIBSDL_CONFIG_FLAGS=--libs
		if $LIBSDL_CONFIG $LIBSDL_CONFIG_FLAGS ; then 
			LIBSDL_CONFIG_FLAGS=--libs
		else
			LIBSDL_CONFIG_FLAGS=--static-libs
		fi
		AC_MSG_NOTICE([Invoking $LIBSDL_CONFIG $LIBSDL_CONFIG_FLAGS.])
                LIBS="$LIBS `$LIBSDL_CONFIG $LIBSDL_CONFIG_FLAGS`"
		# finish this !
                #FIM_LIB_OBJECTS="$FIM_LIB_OBJECTS SDLDevice.o"
		if test x`$LIBSDL_CONFIG --version | head -c 1` = x2; then
			AC_DEFINE(FIM_WITH_LIBSDL, 2, [Defined if libsdl support is enabled, and set to 1 or 2.])
		else
			AC_DEFINE(FIM_WITH_LIBSDL, 1, [Defined if libsdl support is enabled, and set to 1 or 2.])
		fi
        fi
fi
AM_CONDITIONAL(ENABLE_SDL, test x$fim_handle_sdl = xyes)

dnl ================================================================================

## gtk driver

AC_ARG_ENABLE(gtk, AS_HELP_STRING([--disable-gtk],[Disable libgtk support. Some configurations require prepending PKG_CONFIG=pkg-config before configure, for pkg-config to work.]), [fim_want_gtk="$enableval";])
fim_handle_gtk=no;
if test x$fim_want_gtk != xno; then
PKG_CHECK_MODULES(GTK, gtk+-3.0,fim_handle_gtk=yes,fim_handle_gtk=no) dnl sets GTK_CFLAGS GTK_LIBS
# keep this line commented or next line empty or it may break configure. (PKG_CHECK_MODULES-related)
if test x$fim_handle_gtk = xyes; then
AC_CHECK_LIB([glib-2.0], [g_free],
  [ have_gtk=yes; fim_handle_gtk=yes; LIBS="$LIBS $GTK_LIBS"; CXXFLAGS="$CXXFLAGS $GTK_CFLAGS";
    AC_DEFINE([FIM_WITH_LIBGTK],[1],[We have GTK])],
  [ have_gtk=no; fim_handle_gtk=no; AC_MSG_NOTICE([*** GTK driver (-o gtk) won't be added (glib-2.0 library not found) ***])]
  )
else
	AC_MSG_WARN([*** GTK driver (-o gtk) won't be added (glib-2.0 library not found) ***])
fi
fi
AM_CONDITIONAL(ENABLE_GTK, [test x$fim_handle_gtk = xyes])

dnl ================================================================================

AH_TEMPLATE([FIM_WANT_EXIFTOOL])
fim_cv_exiftool=no
AC_ARG_ENABLE(exiftool, AS_HELP_STRING([--enable-exiftool],[Turns on exiftool support (EXPERIMENTAL, shall provide the EXIFTOOLLINK variable with library or object files and EXIFTOOLCXXFLAGS). Not recommended: slow. Use --enable-exif instead.]),
	[if test x$enableval = xyes; then
		fim_cv_exiftool=yes
		CXXFLAGS="$CXXFLAGS $EXIFTOOLCXXFLAGS"
		LIBS="$LIBS $EXIFTOOLLINK"
      		AC_MSG_NOTICE([Enabling exiftool interface.])
		AC_DEFINE(FIM_WANT_EXIFTOOL, 1, [If defined, exiftool will be used to get additional file information (EXPERIMENTAL).])
	fi],[fim_cv_exiftool=no])
dnl

#AC_DEFINE([FIM_DEFAULT_KEY_CONFIG]   ,1, [Defined, if GPGME support is enabled])
#AC_DEFINE([FIM_DEFAULT_CONFIG], 1, [Defined, if GPGME support is enabled])

#AC_DEFINE([FIM_DEFAULT_CONFIGURATION], 1, [Defined, if GPGME support is enabled])

#AC_MSG_CHECKING(whether struct dirent defines d_ino)

dnl ==================

fim_cv_autocommands=yes

AC_ARG_ENABLE(autocommands, AS_HELP_STRING([--disable-autocommands],[Fim will by default use autocommands.]),
[if test x$enableval = xno; then
        fim_cv_autocommands=no
fi])

AH_TEMPLATE([FIM_AUTOCMDS])
if test x$fim_cv_autocommands = xyes; then
	have_autocommands=yes
	AC_DEFINE([FIM_AUTOCMDS])
else
	have_autocommands=no
fi


dnl ==================

AC_ARG_ENABLE(hardcoded-font, AS_HELP_STRING([--enable-hardcoded-font],[Enable a hardcoded font in the executable (good as a fallback (FBFONT=fim://) when no PSF format consolefonts files are available).]),
[if test x$enableval = xyes; then
        fim_cv_hardcoded_font=yes;
else
        fim_cv_hardcoded_font=no;
fi],[ fim_cv_hardcoded_font=yes; ])

AH_TEMPLATE([FIM_WANT_HARDCODED_FONT])
if test x$fim_cv_hardcoded_font = xyes ; then
	have_hardcoded_font=yes
	AC_DEFINE([FIM_WANT_HARDCODED_FONT],1,[Enable a hardcoded font in the executable.])
else
	have_hardcoded_font=no
fi
AM_CONDITIONAL(FIM_WANT_HARDCODED_CONSOLEFONT, test x$have_hardcoded_font = xyes )
AM_CONDITIONAL(FIM_HAVE_ANY_CONSOLEFONT, test x$no = xyes ) dnl not yet active -- need check to detect if any consolefont present

dnl ==================

AC_CHECK_PROG(have_gs,gs,yes,no)
AM_CONDITIONAL(HAVE_GS, test x$have_gs = xyes)
AM_CONDITIONAL(HAVE_FIMGS, test x$have_gs = xyes)

dnl ==================

AC_ARG_ENABLE(long-int-vars, AS_HELP_STRING([--enable-long-int-vars],[Internals shall use 64 bit integers (recommended).]),
[if test x$enableval = xyes; then
        fim_cv_want_int64_int=yes;
else
        fim_cv_want_int64_int=no;
fi],[ fim_cv_want_int64_int=yes; ])

AH_TEMPLATE([FIM_WANT_LONG_INT])
if test x$fim_cv_want_int64_int = xyes; then
	AC_DEFINE([FIM_WANT_LONG_INT],1,[Internals shall use 64 bit integers.])
else
	AC_DEFINE([FIM_WANT_LONG_INT],0,[Internals shall use 64 bit integers.])
fi
dnl ==================

fim_cv_windows=yes

AC_ARG_ENABLE(windows, AS_HELP_STRING([--disable-windows],[Disable windowing system in Fim. (deprecated)]),
[if test x$enableval = xno; then
        fim_cv_windows=no
fi])

AH_TEMPLATE([FIM_WINDOWS])
if test x$fim_cv_windows = xyes; then
	have_windows=yes
	AC_DEFINE([FIM_WINDOWS],1,[Fim windowing support])
else
	have_windows=no
fi


dnl ==================
fim_cv_fimrc=yes

AC_ARG_ENABLE(fimrc, AS_HELP_STRING([--disable-fimrc],[By default, load the personal ~/.fimrc file and a system configuration file.]),
[if test x$enableval = xno; then
        fim_cv_fimrc=no
fi])

AH_TEMPLATE([FIM_NOFIMRC])
if test x$fim_cv_fimrc = xyes; then
	have_fimrc=yes
else
	AC_DEFINE([FIM_NOFIMRC])
fi
AM_CONDITIONAL(WANT_DEFAULT_FIMRC, test x$fim_cv_fimrc = xyes)

dnl ==================
AH_TEMPLATE([FIM_NOHISTORY])
fim_cv_history=yes

AC_ARG_ENABLE(history, AS_HELP_STRING([--disable-history],[Fim will by default load/save a ~/.fim_history file (note: depends on readline).]),
[if test x$enableval = xno ; then
        fim_cv_history=no
fi])
if test x$fim_want_readline = xno; then
        fim_cv_history=no;
fi
AM_CONDITIONAL(ENABLE_HISTORY, test x$fim_cv_history != xno)
AM_CONDITIONAL(ENABLE_NO_HISTORY_TESTS, test x$fim_cv_history = xno) dnl allow disable of ENABLE_HISTORY without enabling else case

AH_TEMPLATE([FIM_NOHISTORY])
if test x$fim_cv_history != xyes; then
	AC_DEFINE([FIM_NOHISTORY])
fi

dnl ==================

fim_cv_warnings=no
AC_ARG_ENABLE(screen, AS_HELP_STRING([--disable-screen],[Fim will try to work under screen by default (please read BUGS)]),
[       if test x$enableval = xno; then
		fim_cv_screen=no
	else
                fim_cv_screen=yes
        fi
],[ fim_cv_screen=yes ]
)

dnl ==================

fim_cv_resize_optimizations=yes
AC_ARG_ENABLE(resize-optimizations, AS_HELP_STRING([--disable-resize-optimizations],[Some old (fbi) code is optimized for speedup in fim.]),
[       if test x$enableval = xno; then
		fim_cv_resize_optimizations=no
	else
                fim_cv_resize_optimizations=yes
        fi
],[ fim_cv_resize_optimizations=yes ]
)

dnl ==================

fim_cv_read_stdin_image=yes
AC_ARG_ENABLE(stdin-image-reading, AS_HELP_STRING([--disable-stdin-image-reading],[Fim will read an image from stdin with the -i switch. (experimental)]),
[       if test x$enableval = xno; then
		fim_cv_read_stdin_image=no
	else
                fim_cv_read_stdin_image=yes
        fi
],[ fim_cv_read_stdin_image=yes ]
)

dnl ==================
fim_cv_scan_consolefonts=yes
AC_ARG_ENABLE(scan-consolefonts, AS_HELP_STRING([--enable-scan-consolefonts],[Fim will scan some predefined directory in look for console fonts as a fallback. (experimental)]),
[       if test x$enableval = xyes; then
		fim_cv_scan_consolefonts=yes
	else
		fim_cv_scan_consolefonts=no
        fi
],[ fim_cv_scan_consolefonts=yes ]
)
dnl ==================
fim_cv_fb_console_switch_while_loading=no
AC_ARG_ENABLE(framebuffer-switch-while-loading, AS_HELP_STRING([--enable-framebuffer-switch-while-loading],[Fim will be capable of redrawing while switching framebuffer console (makes only sense on very slow systems).]),
[       if test x$enableval = xyes; then
		fim_cv_fb_console_switch_while_loading=yes
	else
                fim_cv_fb_console_switch_while_loading=no
        fi
],[ fim_cv_fb_console_switch_while_loading=no ]
)
dnl ==================
fim_cv_seek_magic=yes
AC_ARG_ENABLE(seek-magic, AS_HELP_STRING([--disable-seek-magic],[Fim will be capable to seek on a specified magic string.]),
[       if test x$enableval = xno; then
		fim_cv_seek_magic=no
	else
                fim_cv_seek_magic=yes
        fi
],[ fim_cv_seek_magic=yes ]
)
dnl ==================
fim_cv_paths_in_man=yes
AC_ARG_ENABLE(paths-in-man, AS_HELP_STRING([--disable-paths-in-man],[Generate man pages with configure specified paths (e.g. sysconfdir, docdir).]),
[       if test x$enableval = xno; then
		fim_cv_paths_in_man=no
	else
                fim_cv_paths_in_man=yes
        fi
],[ fim_cv_paths_in_man=yes ]
)
dnl ==================
AC_ARG_ENABLE(pushdir, AS_HELP_STRING([--enable-pushdir],[Fim will scan directories for files.]),
[       if test x$enableval = xno; then
		fim_cv_read_dirs=no
	else
                fim_cv_read_dirs=yes
        fi
],[ fim_cv_read_dirs=yes ]
)
dnl ==================
AC_ARG_ENABLE(recursive-pushdir, AS_HELP_STRING([--disable-recursive-pushdir],[Fim will be able to push directories recursively (implies --enable-push-dirs )]),
[       if test x$enableval = xyes; then
		fim_cv_recursive_dirs=yes;
                fim_cv_read_dirs=yes;
	else
                fim_cv_recursive_dirs=no;
        fi
],[ fim_cv_recursive_dirs=yes;fim_cv_read_dirs=yes; ]
)
dnl ==================

AH_TEMPLATE([FIM_RECURSIVE_DIRS])
AH_TEMPLATE([FIM_WANTS_SLOW_RESIZE])
AH_TEMPLATE([FIM_READ_DIRS])
AH_TEMPLATE([FIM_BOZ_PATCH])
AH_TEMPLATE([FIM_READ_STDIN])
AH_TEMPLATE([FIM_READ_STDIN_IMAGE])
AH_TEMPLATE([FIM_SMART_COMPLETION])
AH_TEMPLATE([FIM_LINUX_CONSOLEFONTS_DIR_SCAN])
dnl ==================

if test x$fim_cv_resize_optimizations != xyes; then
  AC_DEFINE([FIM_WANTS_SLOW_RESIZE])
fi

dnl ==================

if test x$fim_cv_read_dirs = xyes; then
  AC_DEFINE([FIM_READ_DIRS])
  if test x$fim_cv_recursive_dirs = xyes; then
	  AC_DEFINE([FIM_RECURSIVE_DIRS])
  fi
fi
AM_CONDITIONAL(WANT_RECURSIVE_DIRS, test x$fim_cv_recursive_dirs = xyes)

dnl ==================

if test x$fim_cv_screen != xno; then
# boz patch is on
	AC_DEFINE([FIM_BOZ_PATCH])
fi

dnl ==================

fim_cv_read_stdin=yes

if test x$fim_cv_read_stdin != xno; then
	AC_DEFINE([FIM_READ_STDIN])
fi

dnl ==================

dnl NOTE THAT IN THE CODE FIM_READ_STDIN_IMAGE depends on FIM_READ_STDIN, too
if test x$fim_cv_read_stdin_image = xyes -a x$fim_cv_read_stdin = xyes; then
	AC_DEFINE([FIM_READ_STDIN_IMAGE])
else
	# we revert to no just for the printout
	fim_cv_read_stdin_image=no
fi
AM_CONDITIONAL(WANT_READ_STDIN_IMAGE, test x$fim_cv_read_stdin_image = xyes)

dnl ==================

fim_cv_warnings=no
AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug],[Turns on debug options (for debugging -- developer stuff)]),
[if test x$enableval = xyes; then
        fim_cv_debug=yes
fi])

dnl ==================
if test "x$fim_cv_debug" = xyes; then
	AC_CHECK_PROG([HAVE_VALGRIND],[valgrind],[yes],[no])
fi
AM_CONDITIONAL(HAVE_VALGRIND, test x$HAVE_VALGRIND = xyes)
dnl ==================

fim_cv_warnings=no
AC_ARG_ENABLE(warnings, AS_HELP_STRING([--enable-warnings],[Turns on compiler warnings (recommended, for debugging)]),
[if test x$enableval = xyes; then
        fim_cv_warnings=yes
fi])

if test x$GCC = xyes ; then
	if test x$fim_cv_debug = xyes; then
        	CXXFLAGS="${CXXFLAGS} -g"
	fi
fi

if test x$GCC = xyes ; then
	if test x$fim_cv_warnings = xyes ; then
        CXXFLAGS="-Wall -pedantic $CXXFLAGS"
	fi
fi

dnl ==================

dnl ==================

dnl ==== Notice the name difference: libcaca, as opposed to aalib.
AC_CHECK_LIB(caca, caca_create_canvas,fim_cv_lib_caca=yes,fim_cv_lib_caca=no)
AC_ARG_ENABLE(caca, AS_HELP_STRING([--disable-caca],[Colour ASCII-Art (-o ca) support (via libcaca)]), [if test x$enableval != xno; then fim_cv_lib_caca=yes; else fim_cv_lib_caca=no; fi],[fim_cv_lib_caca=yes;])

if test x$fim_cv_lib_caca != xno; then
	if test x$ac_cv_lib_caca_caca_create_canvas = xyes; then
      		AC_MSG_NOTICE([Enabling libcaca driver.])
		fim_cv_libcaca=yes
	        if test x$libcaca_config_prefix != x; then
			LIBCACA_CONFIG="$libcaca_config_prefix/bin/caca-config"
		else
			# default path configurator
			AC_PATH_PROG(LIBCACA_CONFIG, caca-config, no)
		fi
		if test "x$LIBCACA_CONFIG" = "xno"; then
			# uhm
	      		AC_MSG_WARN([***  no caca-config ? ***])
			fim_cv_libcaca=no
		else
			CXXFLAGS="$CXXFLAGS `$LIBCACA_CONFIG --cflags`"
			LIBS="$LIBS `$LIBCACA_CONFIG --libs`"
			# finish this !
		fi
	else
		fim_cv_libcaca=no
	fi
else
	fim_cv_libcaca=no
	AC_MSG_WARN([Colour ASCII-Art (-o ca) support disabled.])
        dnl AC_MSG_ERROR([Sorry, it does not seem like libcaca is installed or detectable.
	dnl Please run --disable-caca or install libcaca in a system-wide directory instead.
	dnl ])
fi

dnl ==================

# This code still awaits for integration ...
AC_ARG_WITH(cflags, AS_HELP_STRING([--with-cflags=CXXFLAGS],[Specify a custom CXXFLAGS append value.  e.g.: -pg
	 You cannot call this argument multiple times.
	 In case you want completely reset CXXFLAGS use :
	 CXXFLAGS='<all that you want>' ./configure ..
	 Note that Fim is a C++ program, so we set CXXFLAGS=CXXFLAGS.
	]),
        [if test "x$withval" != x; then
                AC_DEFINE_UNQUOTED(CXXFLAGS, "$withval",
                 [custom CXXFLAGS])
		CXXFLAGS="$withval"
		CXXFLAGS="$CXXFLAGS"
		CXXFLAGS="$CXXFLAGS"
                AC_DEFINE_UNQUOTED(CXXFLAGS, "$withval",
                 [custom CXXFLAGS])
		dnl ...else CXXFLAGS="$CXXFLAGS"
        fi],
        [AC_DEFINE_UNQUOTED(EXECSHELL, "/bin/sh")])


dnl ==================

# This code still awaits for integration ...
AC_ARG_WITH(exec-shell, AS_HELP_STRING([--with-exec-shell=SHELL],[Specify alternate shell (ONLY if /bin/sh is broken)]),
        [if test x$withval != xyes; then
                AC_DEFINE_UNQUOTED(EXECSHELL, "$withval",
                 [program to use for shell commands])
         else
                AC_DEFINE_UNQUOTED(EXECSHELL, "/bin/sh")
        fi],
        [AC_DEFINE_UNQUOTED(EXECSHELL, "/bin/sh")])

dnl ==================
AC_ARG_ENABLE(smartautocompletion, AS_HELP_STRING([--disable-smartautocompletion],[Fim will treat as a proper, unescaped string, nonquoted and nonevaluated strings which happen to match an existing filename.]),
[       if test x$enableval = xno; then
		fim_cv_smartautcompletion=no
	else
                fim_cv_smartautcompletion=yes
        fi
],[ fim_cv_smartautcompletion=yes ]
)
dnl ==================
if test x$fim_cv_smartautcompletion = xyes; then
	AC_DEFINE([FIM_SMART_COMPLETION])
fi
dnl ==================
if test x$fim_cv_scan_consolefonts = xyes; then
	AC_DEFINE([FIM_LINUX_CONSOLEFONTS_DIR_SCAN])
fi
dnl ==================

AC_ARG_ENABLE(convert, AS_HELP_STRING([--disable-convert],[Fim will try to use convert (ImageMagick) as a last resort for opening unknown format files.]),
[       if test x$enableval = xno; then
		fim_cv_convert=no
	else
                fim_cv_convert=yes
        fi
],[ fim_cv_convert=yes ]
)

dnl ==================

AC_ARG_ENABLE(dcraw, AS_HELP_STRING([--disable-dcraw],[Fim will try to use dcraw as a last resort for opening NEF files.]),
[       if test x$enableval = xno; then
		fim_cv_dcraw=no
	else
                fim_cv_dcraw=yes
        fi
],[ fim_cv_dcraw=yes ]
)

dnl ==================

AC_ARG_ENABLE(xcftopnm, AS_HELP_STRING([--disable-xcftopnm],[Fim will try to use xcftopnm (Gimp) for opening XCF format files.]),
[       if test x$enableval = xno; then
		fim_cv_xcftopnm=no
	else
                fim_cv_xcftopnm=yes
        fi
],[ fim_cv_xcftopnm=yes ]
)
AM_CONDITIONAL(WITH_XCFTOPNM, test x$fim_cv_xcftopnm = xyes)

AC_ARG_ENABLE(xcf2pnm, AS_HELP_STRING([--disable-xcf2pnm],[Fim will try to use xcf2pnm (Gimp) for opening XCF format files.]),
[       if test x$enableval = xno; then
		fim_cv_xcf2pnm=no
	else
                fim_cv_xcf2pnm=yes
        fi
],[ fim_cv_xcf2pnm=yes ]
)
AM_CONDITIONAL(WITH_XCF2PNM, test x$fim_cv_xcf2pnm = xyes)

dnl ==================

fim_cv_inkscape=yes
AC_ARG_ENABLE(inkscape, AS_HELP_STRING([--disable-inkscape],[Fim will try to use Inkscape for opening SVG format files.]),
[       if test x$enableval = xno; then
		fim_cv_inkscape=no
	else
                fim_cv_inkscape=yes
        fi
],[ fim_cv_inkscape=yes ]
)

dnl ==================

AC_ARG_ENABLE(xfig, AS_HELP_STRING([--disable-xfig],[Fim will try to use xfig (fig2dev) for opening FIG format files.]),
[       if test x$enableval = xno; then
		fim_cv_xfig=no
	else
                fim_cv_xfig=yes
        fi
],[ fim_cv_xfig=yes ]
)

dnl ==================

AC_ARG_ENABLE(dia, AS_HELP_STRING([--disable-dia],[Fim will try to use dia for opening DIA format files.]),
[       if test x$enableval = xno; then
		fim_cv_dia=no
	else
                fim_cv_dia=yes
        fi
],[ fim_cv_dia=yes ]
)

dnl ==================
AH_TEMPLATE([FIM_DEFAULT_CONSOLEFONT])
AC_ARG_WITH(default-consolefont, AS_HELP_STRING([--with-default-consolefont=FILE],[Specify a default consolefont (psf or psf.gz format) file.]), [ AC_DEFINE_UNQUOTED(FIM_DEFAULT_CONSOLEFONT, "$withval")
fim_cv_defaultconsolefont="$withval";
], [fim_cv_defaultconsolefont=""])
AC_SUBST(FIM_CUSTOM_DEFAULT_CONSOLEFONT, "$fim_cv_defaultconsolefont")
dnl ==================
AC_ARG_WITH(custom-hardcoded-consolefont, AS_HELP_STRING([--with-custom-hardcoded-consolefont=FILE],[Specify a custom consolefont to be hardcoded (uncompressed psf format, absolute path) at build time (requires --enable-hardcoded-font).]), [
fim_cv_want_custom_hardcoded_consolefont=true;
AC_SUBST(FIM_CUSTOM_HARDCODED_CONSOLEFONT, "$withval")
], [])
AM_CONDITIONAL(FIM_WANT_CUSTOM_HARDCODED_CONSOLEFONT, test x$fim_cv_want_custom_hardcoded_consolefont = xtrue )
dnl ==================
fim_cv_want_font_magnify_factor=0;
AC_ARG_WITH(font-magnifying-factor, AS_HELP_STRING([--with-font-magnifying-factor=FACTOR],[If FACTOR>0, Will render text magnified by (hardcoded) FACTOR times. If FACTOR<=0, will allow run-time control of it via the _fbfont_magnify_factor variable. If negative, -FACTOR will be used as default. Default is 0 ]), [
fim_cv_want_font_magnify_factor="$withval";
], [])
AM_CONDITIONAL(FIM_WANT_CUSTOM_HARDCODED_CONSOLEFONT, test x$fim_cv_want_custom_hardcoded_consolefont = xtrue )
dnl ==================
AM_CONDITIONAL([HAVE_RUNNABLE_TESTS],[test x"$host_cpu" = x"$build_cpu" -a x"$host_os" = x"$build_os" -a x"$using_wasm" = x"no" ])
dnl ==================
dnl # /tmp/__FIM_TEMPORARY_FILE
AC_ARG_WITH(tmpfile, AS_HELP_STRING([--with-tmpfile=FILE],[Specify where to put temporary files created when interacting with external conversion programs.]),
        [fim_cv_tmpfile=$withval],
        [fim_cv_tmpfile=""])
dnl ==================
AC_ARG_ENABLE(custom-status-bar, AS_HELP_STRING([--disable-custom-status-bar],[Customization of status bar via a printf-like string.]),
[       if test x$enableval = xno; then
		fim_cv_custom_status_bar=no;
	else
                fim_cv_custom_status_bar=yes;
        fi
],[ fim_cv_custom_status_bar=yes; ]
)
dnl ==================

AC_ARG_ENABLE(optimizations, AS_HELP_STRING([--enable-optimizations],[Will try to enhance compilation flags (EXPERIMENTAL)]),
[       if test x$enableval = xno; then
		fim_cv_optimizations=no
	else
                fim_cv_optimizations=yes
        fi
],[ fim_cv_optimizations=no ]
)

dnl ==================
AH_TEMPLATE([FIM_TRY_XCFTOPNM])
AH_TEMPLATE([FIM_TRY_XCF2PNM])
AH_TEMPLATE([FIM_TRY_CONVERT])
AH_TEMPLATE([FIM_TRY_DCRAW])
AH_TEMPLATE([FIM_TRY_INKSCAPE])
AH_TEMPLATE([FIM_TRY_XFIG])
AH_TEMPLATE([FIM_TRY_DIA])
AH_TEMPLATE([FIM_USE_ZCAT])

dnl ==================
AC_CHECK_PROG(have_tex,tex,yes,no)
AC_CHECK_PROG(have_dvips,dvips,yes,no)
AC_CHECK_PROG(have_pdf2djvu,pdf2djvu,yes,no)
AC_CHECK_PROG(have_dvipdf,dvipdf,yes,no)
AM_CONDITIONAL(WITH_TEX, test x$have_tex = xyes)
AM_CONDITIONAL(WITH_DVIPS, test x$have_dvips = xyes)
AM_CONDITIONAL(WITH_PDF2DJVU, test x$have_pdf2djvu = xyes)
AM_CONDITIONAL(WITH_DVIPDF, test x$have_dvipdf = xyes)
dnl ==================

AC_CHECK_PROG(have_zcat,zcat,yes,no)
        if test x$have_zcat = xyes ; then
		# legacy fbi feature : should not rely on this. FIXME
		AC_MSG_NOTICE([Fim will use 'zcat' to uncompress "gzipped" font files.])
		AC_DEFINE([FIM_USE_ZCAT])
	fi

dnl 20110602 will use man --html-cat instead of obsolete man2html 
dnl AC_CHECK_PROG(have_man2html,man2html,yes,no)
dnl        if test x$have_man2html = xyes ; then
dnl      		AC_MSG_NOTICE([The installer will try to invoke 'man2html' to build html versions of the man pages.])
dnl		HTMLDOCS="FIM.html fim.man.html fimgs.man.html"
dnl	fi

AC_CHECK_PROG(have_convert,convert,yes,no)
        if test x$have_convert = xyes -a x$fim_cv_convert = xyes ; then
		true
dnl      		AC_MSG_WARN([Fim will try to invoke 'convert' to handle unknown format files, even if no 'convert' is currently in the program path: please install the Imagemagick package to obviate the problem or re-configure with --disable-convert.])
	else
		fim_cv_convert=no
	fi
AM_CONDITIONAL(WITH_CONVERT, test x$fim_cv_convert = xyes)

AC_CHECK_PROG(have_dcraw,dcraw,yes,no)
        if test x$have_dcraw = xyes -a x$fim_cv_dcraw = xyes ; then
		true
dnl      		AC_MSG_WARN([Fim will try to invoke 'dcraw' to handle NEF files, even if no 'dcraw' is currently in the program path: please install the dcraw package to obviate the problem or re-configure with --disable-dcraw.])
	else
		fim_cv_dcraw=no
	fi
AM_CONDITIONAL(WITH_DCRAW, test x$fim_cv_dcraw = xyes)

AC_CHECK_PROG(have_xcftopnm,xcftopnm,yes,no)
        if test x$have_xcftopnm = xyes -a x$fim_cv_xcftopnm = xyes ; then
		true
dnl      		AC_MSG_WARN([Fim will try to invoke 'xcftopnm' to handle XCF format files, even if no 'xcftopnm' is currently in the program path: please install the Gimp package to obviate the problem or re-configure with --disable-xcftopnm.])
	else
		fim_cv_xcftopnm=no
	fi

AC_CHECK_PROG(have_xcf2pnm,xcf2pnm,yes,no)
        if test x$have_xcf2pnm = xyes -a x$fim_cv_xcf2pnm = xyes ; then
		true
dnl      		AC_MSG_WARN([Fim will try to invoke 'xcf2pnm' to handle XCF format files, even if no 'xcf2pnm' is currently in the program path: please install the Gimp package to obviate the problem or re-configure with --disable-xcf2pnm.])
	else
		fim_cv_xcf2pnm=no
	fi

AC_CHECK_PROG(have_inkscape,inkscape,yes,no)
        if test x$have_inkscape = xyes -a x$fim_cv_inkscape = xyes ; then
		true
dnl      		AC_MSG_WARN([Fim will try to invoke 'inkscape' to handle SVG format files, even if no 'inkscape' is currently in the program path: please install the Inkscape package to obviate the problem or re-configure with --disable-inkscape.])
	else
		fim_cv_inkscape=no
	fi
AM_CONDITIONAL(WITH_INKSCAPE, test x$fim_cv_inkscape = xyes)

AC_CHECK_PROG(have_xfig,fig2dev,yes,no)
        if test x$have_xfig = xyes -a x$fim_cv_xfig = xyes ; then
		true
dnl      		AC_MSG_WARN([Fim will try to invoke 'xfig' to handle FIG format files, even if no 'fig2dev' is currently in the program path: please install the Xfig package to obviate the problem or re-configure with --disable-xfig.])
	else
		fim_cv_xfig=no
	fi
AM_CONDITIONAL(WITH_XFIG, test x$have_xfig = xyes -a x$fim_cv_xfig = xyes)

AC_CHECK_PROG(have_dia,dia,yes,no)
        if test x$have_dia = xyes -a x$fim_cv_dia = xyes ; then
		true
dnl      		AC_MSG_WARN([Fim will try to invoke 'dia' to handle DIA format files, even if no 'dia' is currently in the program path: please install the Dia package to obviate the problem or re-configure with --disable-dia.])
	else
		fim_cv_dia=no
	fi

dnl ==================

        if test x$fim_cv_optimizations = xyes ; then
		uname=`uname`
        	if test x$uname = xLinux ; then
dnl			AC_MSG_NOTICE([we run on linux! Good.])
	     		if grep "^model name.*Pentium(R) 4" /proc/cpuinfo > /dev/null; then
				CXXFLAGS="$CXXFLAGS -march=native"
		     	fi

		else
			AC_MSG_NOTICE([uhm. try to specify your own optimizations..])

		fi
	fi

dnl ==================

        if test x$fim_cv_convert = xyes ; then
		AC_DEFINE([FIM_TRY_CONVERT])
	fi

        if test x$fim_cv_dcraw = xyes ; then
		AC_DEFINE([FIM_TRY_DCRAW])
	fi

        if test x$fim_cv_xcftopnm = xyes ; then
		AC_DEFINE([FIM_TRY_XCFTOPNM])
	fi

        if test x$fim_cv_xcf2pnm = xyes ; then
		AC_DEFINE([FIM_TRY_XCF2PNM])
	fi

        if test x$fim_cv_inkscape = xyes ; then
		AC_DEFINE([FIM_TRY_INKSCAPE])
	fi

	if test x$have_xfig = xyes -a x$fim_cv_xfig = xyes; then
		AC_DEFINE([FIM_TRY_XFIG])
	fi

        if test x$have_dia = xyes ; then
		AC_DEFINE([FIM_TRY_DIA])
	fi

dnl ==================

    AH_TEMPLATE([FIM_WITH_LIBJASPER])
            if test x$fim_cv_libjasper = xyes ; then
    		AC_DEFINE([FIM_WITH_LIBJASPER])
    	fi
dnl ==================

    AH_TEMPLATE([FIM_WITH_LIBEXIF])
            if test x$fim_cv_libexif = xyes ; then
    		AC_DEFINE([FIM_WITH_LIBEXIF])
    	fi
dnl ==================

    AH_TEMPLATE([FIM_WITH_AALIB])
            if test x$fim_cv_aalib = xyes ; then
    		AC_DEFINE([FIM_WITH_AALIB])
    	fi
	AM_CONDITIONAL(ENABLE_AA, test x$fim_cv_aalib = xyes)

dnl ==================

    AH_TEMPLATE([FIM_WITH_LIBCACA])
            if test x$fim_cv_libcaca = xyes ; then
    		AC_DEFINE([FIM_WITH_LIBCACA])
    	fi
	AM_CONDITIONAL(ENABLE_CACA, test x$fim_cv_libcaca = xyes)

dnl ==================
    AH_TEMPLATE([FIM_WANT_FB_CONSOLE_SWITCH_WHILE_LOADING])
            if test x$fim_cv_fb_console_switch_while_loading = xyes ; then
    		AC_DEFINE([FIM_WANT_FB_CONSOLE_SWITCH_WHILE_LOADING])
    	fi

dnl ==================
    AH_TEMPLATE([FIM_WANT_SEEK_MAGIC])
            if test x$fim_cv_seek_magic = xyes -a x$ac_cv_func_memmem = xyes; then
    		AC_DEFINE([FIM_WANT_SEEK_MAGIC])
    	fi

dnl ==================
    AH_TEMPLATE([FIM_PATHS_IN_MAN])
            if test x$fim_cv_paths_in_man = xyes ; then
    		AC_DEFINE([FIM_PATHS_IN_MAN])
    	fi

dnl ==================
    AH_TEMPLATE([FIM_WANT_FONT_MAGNIFY_FACTOR])
    AC_DEFINE_UNQUOTED([FIM_WANT_FONT_MAGNIFY_FACTOR],$fim_cv_want_font_magnify_factor)

dnl ==================
    AH_TEMPLATE([FIM_WANT_CUSTOM_INFO_STATUS_BAR])
            if test x$fim_cv_custom_status_bar = xyes ; then
    		AC_DEFINE([FIM_WANT_CUSTOM_INFO_STATUS_BAR])
    	fi

dnl ==================
AH_TEMPLATE([FIM_WANT_FILENAME_MARK_AND_DUMP])
AC_ARG_ENABLE(mark-and-dump, AS_HELP_STRING([--enable-mark-and-dump],[Fim will be capable of marking files while viewing and outputting their names on exit. (enabled by default)]), [       if test x$enableval = xyes; then fim_cv_want_mark_and_dump=1 ; else fim_cv_want_mark_and_dump=0 ; fi ],[ fim_cv_want_mark_and_dump=1 ])
AC_DEFINE_UNQUOTED([FIM_WANT_FILENAME_MARK_AND_DUMP], $fim_cv_want_mark_and_dump, [Fim will be capable of marking files while viewing and outputting their names on exit.])
dnl ==================
AH_TEMPLATE([FIM_WANT_MOUSE])
AC_ARG_ENABLE(mouse, AS_HELP_STRING([--enable-mouse],[Mouse support. (enabled by default)]), [       if test x$enableval = xyes; then fim_cv_want_mouse=1 ; else fim_cv_want_mouse=0 ; fi ],[ fim_cv_want_mouse=1 ])
AC_DEFINE_UNQUOTED([FIM_WANT_MOUSE], $fim_cv_want_mouse, [Scripting. (enabled by default)])
AM_CONDITIONAL(ENABLE_MOUSE, test x$fim_cv_want_mouse = x1)
if test x$fim_cv_want_mouse = x1; then fim_cv_want_mouse=yes ; else fim_cv_want_mouse=no ; fi
dnl ==================
AH_TEMPLATE([FIM_WANT_NOSCRIPTING])
AC_ARG_ENABLE(scripting, AS_HELP_STRING([--enable-scripting],[Scripting. (enabled by default)]), [       if test x$enableval = xyes; then fim_cv_want_noscripting=0 ; else fim_cv_want_noscripting=1 ; fi ],[ fim_cv_want_noscripting=0 ])
if test x$fim_cv_want_noscripting = x1 ; then 
AC_DEFINE_UNQUOTED([FIM_WANT_NOSCRIPTING], $fim_cv_want_noscripting, [Scripting. (enabled by default)])
fi
dnl ==================
AH_TEMPLATE([FIM_WANT_NO_OUTPUT_CONSOLE])
AC_ARG_ENABLE(output-console, AS_HELP_STRING([--enable-output-console],[Output console. (enabled by default)]), [       if test x$enableval = xyes; then fim_cv_want_no_output_console=0 ; else fim_cv_want_no_output_console=1 ; fi ],[ fim_cv_want_no_output_console=0 ])
if test x$fim_cv_want_no_output_console = x1 ; then 
AC_DEFINE_UNQUOTED([FIM_WANT_NO_OUTPUT_CONSOLE], $fim_cv_want_no_output_console, [Output console. (enabled by default)])
fi
dnl ==================
dnl AH_TEMPLATE([FIM_WANT_UNICODE])
dnl AC_ARG_ENABLE(unicode, AS_HELP_STRING([--enable-unicode],[Unicode. (Unimplemented, disabled by default)]), [       if test x$enableval = xyes; then fim_cv_want_unicode=1 ; else fim_cv_want_unicode=0 ; fi ],[ fim_cv_want_unicode=0 ])
dnl if test x$fim_cv_want_unicode = x1 ; then 
dnl 	# CXXFLAGS="$CXXFLAGS `freetype-config --cflags`"
dnl 	# LIBS="$LIBS `freetype-config --libs`"
dnl 	AC_DEFINE_UNQUOTED([FIM_WANT_UNICODE], $fim_cv_want_unicode, [Check for freetype library, with no feature behind: do not use this. (disabled by default)])
dnl fi
dnl ==================
AH_TEMPLATE([FIM_WANT_TEXT_RENDERING])
fim_want_astext=yes
AC_ARG_ENABLE(as-text-rendering, AS_HELP_STRING([--disable-as-text-rendering],[Disable as-text rendering (viewing of any file as character bytes -- not to be confused with the ASCII art output).]),
[       if test x$enableval = xno ; then
		fim_want_astext=no
        fi
])
if test x$fim_want_astext = xyes ; then
	AC_DEFINE_UNQUOTED([FIM_WANT_TEXT_RENDERING], 1, [Enables text rendering])
fi
dnl ==================
AH_TEMPLATE([FIM_WANT_RAW_BITS_RENDERING])
fim_want_rbr=yes
AC_ARG_ENABLE(raw-bits-rendering, AS_HELP_STRING([--disable-raw-bits-rendering],[Disable raw bits rendering (viewing of any file as either a 1 or 24 bpp bitmap).]),
[       if test x$enableval = xno ; then
		fim_want_rbr=no
        fi
])
if test x$fim_want_rbr = xyes ; then
	AC_DEFINE_UNQUOTED([FIM_WANT_RAW_BITS_RENDERING], 1, [Enables raw bits rendering])
fi
AM_CONDITIONAL(ENABLE_RAW_BITS_RENDERING, test x$fim_want_rbr = xyes)
dnl ==================
AH_TEMPLATE([FIM_ALLOW_LOADER_STRING_SPECIFICATION])
fim_want_lss=yes
AC_ARG_ENABLE(loader-string-specification, AS_HELP_STRING([--disable-loader-string-specification],[Disables the chance to specify a file loader.]),
[       if test x$enableval = xno ; then
		fim_want_lss=no
        fi
])
if test x$fim_want_lss = xyes ; then
	AC_DEFINE_UNQUOTED([FIM_ALLOW_LOADER_STRING_SPECIFICATION], 1, [Allows the user to specify a file loader])
fi
dnl ==================

dnl uhmmmmmm : does not seem to work
dnl AC_ARG_ENABLE(verbose_make,[Enables verbose macros passing to the compiler (for a funnier compilation process.)], 
dnl         if test x$enableval = xyes; then
dnl		AC_SUBST(DEFS)
dnl	else
dnl		AC_CONFIG_HEADERS([config.h])
dnl        fi,[])
AC_CONFIG_HEADERS([config.h])

AC_CHECK_PROG([HAVE_TIMEOUT],[timeout],[yes],[no])
AH_TEMPLATE([FIM_HAS_TIMEOUT])
if test x"$ac_cv_prog_HAVE_TIMEOUT" = x"yes"; then
	AC_MSG_NOTICE(Will use 'timeout ' in certain tests.)
	AC_SUBST(TIMEOUT_SMALL,"timeout --foreground 5")
	AC_SUBST(TIMEOUT_LARGE,"timeout --foreground 30")
	AC_DEFINE([FIM_HAS_TIMEOUT])
fi

AH_TEMPLATE([FIM_RANDOM])
AC_DEFINE([FIM_RANDOM])
AH_TEMPLATE([FIM_DEFAULT_CONFIG])
AC_DEFINE([FIM_DEFAULT_CONFIG])
AH_TEMPLATE([FIM_NOFB])
AC_DEFINE([FIM_NOFB])
AH_TEMPLATE([FIM_RECORDING])
AC_DEFINE([FIM_RECORDING])
AH_TEMPLATE([FIM_SWITCH_FIXUP])
AC_DEFINE([FIM_SWITCH_FIXUP])
AH_TEMPLATE([FIM_CHECK_DUPLICATES])
AC_DEFINE([FIM_CHECK_DUPLICATES])
AH_TEMPLATE([FIM_CHECK_FILE_EXISTENCE])
AC_DEFINE([FIM_CHECK_FILE_EXISTENCE])
AH_TEMPLATE([FIM_REMOVE_FAILED])
AC_DEFINE([FIM_REMOVE_FAILED])
AH_TEMPLATE([FIM_AUTOSKIP_FAILED])
AC_DEFINE([FIM_AUTOSKIP_FAILED])
AH_TEMPLATE([FIM_COMMAND_AUTOCOMPLETION])
AC_DEFINE([FIM_COMMAND_AUTOCOMPLETION])
AH_TEMPLATE([FIM_NAMESPACES])
AC_DEFINE([FIM_NAMESPACES])
AH_TEMPLATE([FIM_CACHE_DEBUG])
dnl AC_DEFINE([FIM_CACHE_DEBUG])
AH_TEMPLATE([FIM_VERSION])
AC_DEFINE_UNQUOTED([FIM_VERSION],"${VERSION}")
dnl AH_TEMPLATE([FIM_USE_X11_FONTS])
dnl AC_DEFINE([FIM_USE_X11_FONTS])
AH_TEMPLATE([FIM_TMP_FILENAME])
AC_DEFINE_UNQUOTED([FIM_TMP_FILENAME],"$fim_cv_tmpfile")
AH_TEMPLATE([FIM_WITH_DEBUG])
if test "x$fim_cv_debug" = xyes; then
	AC_DEFINE([FIM_WITH_DEBUG])
else
	true;
fi
AM_CONDITIONAL(WITH_DEBUG, test "x$fim_cv_debug" = xyes)
AH_TEMPLATE([FIM_ITERATED_COMMANDS])
AC_DEFINE([FIM_ITERATED_COMMANDS])
AH_TEMPLATE([FIM_SKIP_KNOWN_FILETYPES])
AC_DEFINE([FIM_SKIP_KNOWN_FILETYPES])
AH_TEMPLATE([FIM_EXPERIMENTAL_ROTATION])
AC_DEFINE([FIM_EXPERIMENTAL_ROTATION])
AH_TEMPLATE([FIM_KEEP_BROKEN_CONSOLE])
dnl AC_DEFINE([FIM_KEEP_BROKEN_CONSOLE])
AH_TEMPLATE([FIM_USE_DESIGNATED_INITIALIZERS])
dnl AC_DEFINE([FIM_USE_DESIGNATED_INITIALIZERS])
dnl AH_TEMPLATE([FIM_IS_SLOWER_THAN_FBI])
dnl AC_DEFINE([FIM_IS_SLOWER_THAN_FBI])
dnl AH_TEMPLATE([FIM_WITH_LIBSDL])
dnl AC_DEFINE([FIM_WITH_LIBSDL])
AH_TEMPLATE([FBI_AUTHOR])
AH_TEMPLATE([FBI_AUTHOR_NAME])
AH_TEMPLATE([FIM_AUTHOR])
AH_TEMPLATE([FIM_AUTHOR_NAME])
AH_TEMPLATE([FIM_AUTHOR_EMAIL])
AC_DEFINE([FIM_AUTHOR_NAME],["Michele Martone"])
AC_DEFINE([FIM_AUTHOR_EMAIL],["<dezperado _CUT_ autistici _CUT_ org>"])
AC_DEFINE([FIM_AUTHOR],["Michele Martone <dezperado _CUT_ autistici _CUT_ org>"])
# Note: Gerd changed his surname from Knorr to Hoffmann, so I report here his actual surname
AC_DEFINE([FBI_AUTHOR_NAME],["Gerd Hoffmann"])
AC_DEFINE([FBI_AUTHOR],["Gerd Hoffmann <kraxel _CUT_ bytesex.org>"])
dnl ###############################################################################
dnl this is experimental stuff
dnl ###############################################################################
want_matrices_rendering=no;
AC_ARG_ENABLE(matrices-rendering, AS_HELP_STRING([--enable-matrices-rendering],[Uses librsb for rendering of Matrix Market files (must have librsb-config in the PATH).]),
[       if test x$enableval = xyes; then
        	if test x$ac_cv_func_rsb_lib_init = xyes; then
AH_TEMPLATE([HAVE_MATRIX_MARKET_DECODER])
AC_DEFINE([HAVE_MATRIX_MARKET_DECODER])
#CXXFLAGS="$CXXFLAGS `librsb-config  --cflags`"
#LIBS="$LIBS `librsb-config --static --ldflags | sed s/-static//g`"
			want_matrices_rendering=yes;
		else
			AC_MSG_ERROR([Cannot detect librsb! Either disable matrix rendering or supply the right LIBS and CXXFLAGS!])
        	fi
	else
		true;
        fi
])
AM_CONDITIONAL(ENABLE_MATRIX_MARKET_DECODER, test x$ac_cv_func_rsb_lib_init = xyes -a x$want_matrices_rendering = xyes)
dnl ###############################################################################
AM_CONDITIONAL(WANT_WASM, test x"$using_wasm" != x"no")
dnl ###############################################################################
AC_CACHE_CHECK([if you have c++11 ...], [have_cv_cxx11], AC_COMPILE_IFELSE([AC_LANG_SOURCE([[
#if __cplusplus<201103L
#error using pre-c++11
#endif
int main() { }
]])],[fim_cxx_standard=new],[fim_cxx_standard=old],[]))

if test x"$fim_cxx_standard" = x"new"; then
AC_LINK_IFELSE([AC_LANG_PROGRAM([
 #include <thread>
void thread_function() { }
],[
    std::thread t(&thread_function);
    t.join();
])],[have_cv_thread_libs=yes],[have_cv_thread_libs=no],[have_cv_thread_libs=no;])

if test x$have_cv_thread_libs != xyes; then
	AC_MSG_NOTICE([Seemingly C++ threads need an extra library. Checking if -pthread solves this.])
	AC_CHECK_LIB(pthread, pthread_create, 
	LIBS="$LIBS -lpthread"; 
[AC_LINK_IFELSE([AC_LANG_PROGRAM([
 #include <thread>
void thread_function() { }
],[
    std::thread t(&thread_function);
    t.join();
])],[AC_MSG_NOTICE([Yes, adding -lpthread allows using C++ threads.])],[],[;])
fim_cv_have_pthread_added=yes;], fim_cv_have_pthread_added=no)
fi
fi
dnl ###############################################################################


dnl ==================

AC_SUBST(LIBS)
AC_SUBST(FIM_DEFS)

AC_SUBST(FIM_LIB_OBJECTS)
AC_SUBST(FIM_SVN_REPOSITORY,[http://svn.savannah.nongnu.org/svn/fbi-improved/])

dnl we want these symbols defined in our code
AC_SUBST(CXXFLAGS)

#AC_CONFIG_FILES([Makefile],[src/Makefile])

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile distros/Makefile scripts/Makefile
	 media/Makefile
	 media/multipage/Makefile
	 scripts/maintenance/cron-build.sh
	 scripts/maintenance/remote-build.sh
	 scripts/maintenance/cron-svndump.pl
	 scripts/maintenance/cron-rsync.sh
	])
AC_OUTPUT
#AC_OUTPUT

AC_MSG_NOTICE([
	Fim v${VERSION} has been configured as follows (features marked with * are not recommended: either unfinished or broken: please do NOT use them when building for a distro):

   	default minimal key mapping         : ${have_default_mapping:-no}
   	default minimal key bindings        : ${have_default_bindings:-no}
	default configuration file builtin  : ${have_default_config:-no}
dnl     compile time warnings               : ${fim_cv_warnings:-no} (additional CXXFLAGS: ${CXXFLAGS} )
        resize optimizations                : ${fim_cv_resize_optimizations:-no}
        system,popen calls wrappers enabled : ${fim_system_calls:-yes}
   	will allow push of directories      : ${fim_cv_read_dirs}
   	will recurse directories (-R switch): ${fim_cv_recursive_dirs}
dnl	dvi  file support (dvilib29)        : ${fim_handle_dvi:-no}
	patch for running under screen      : ${fim_cv_screen:-no}
	~/.fimrc startup file support       : ${have_fimrc:-no}
	~/.fim_history load/save support    : ${fim_cv_history:-no}
	autocommands enabled                : ${have_autocommands:-no}
	mouse enabled                       : ${fim_cv_want_mouse}
dnl	internal windowing system           : ${have_windows:-no}
	hardcoded PSF font (FBFONT=fim://)  : ${have_hardcoded_font:-no}
	will read image files from stdin(-i): ${fim_cv_read_stdin_image:-no}
dnl	GNU regexps (default is Posix)      :*${fim_cv_regex:-no}
	<regex> header                      : ${ac_cv_header_regex}
        seek magic signatures               : ${fim_cv_seek_magic:-no}
	man page with user configured paths : ${fim_cv_paths_in_man}
        framebuffer console load switchable :*${fim_cv_fb_console_switch_while_loading:-no}
        hardcoded font magnifying factor    : ${fim_cv_want_font_magnify_factor}
	customizable status bar             : ${fim_cv_custom_status_bar:-no}
dnl	use C++ standard (98/11/14/17/20/23): ${fim_cxx_standard} dnl may revive this with *detected* language level one day
dnl
	readline (-lrl) library support     : ${fim_want_readline}
	use 'xcftopnm' for XCF files        :*${fim_cv_xcftopnm}
	use 'xcf2pnm' for XCF files         :*${fim_cv_xcf2pnm}
	use 'inkscape' for SVG files        : ${fim_cv_inkscape}
	use 'fig2dev' for FIG files         : ${fim_cv_xfig}
	use 'convert' for unrecognized files:*${fim_cv_convert}
	use 'dcraw' for NEF files           :*${fim_cv_dcraw}
	use 'exiftool' for EXIF tags in JPEG:*${fim_cv_exiftool}
dnl
        framebuffer (built-in) support      : ${fim_handle_framebuffer:-yes}
	SDL      (sdllib) support (-o sdl)  : ${fim_handle_sdl}
	GTK        (GTK3) support (-o gtk)  : ${fim_handle_gtk}
	ASCII art (aalib) support (-o aa)   : ${fim_cv_aalib}
	Colour ASCII art (libcaca) support  : ${fim_cv_libcaca}
dnl	X        (Imlib2) support(-o imlib2):*${fim_handle_imlib2:-no}
dnl
   	EXIF (JPEG) tags support (libexif)  : ${fim_cv_libexif}
	BMP      file support (builtin)     : ${fim_handle_bmp:-no}
	JP2 (JPEG-2K) file support (JasPer) : ${fim_cv_libjasper}
	PCX      file support (builtin)     : ${fim_handle_pcx:-no}
	PNG      file support (libpng)      : ${fim_handle_png:-no}
	JPEG     file support (libjpeg)     : ${fim_handle_jpeg:-no}
	TIFF     file support (libtiff)     : ${fim_handle_tiff:-no}
	GIF      file support (libgif)      : ${fim_handle_gif:-no}
	QOI      file support (libqoi)      :*${fim_handle_qoi:-no}
	DJVU     file support (libdjvu)     :*${fim_handle_djvu:-no}
	PDF      file support (libpoppler)  :*${fim_handle_pdf:-no}
	PS       file support (libspectre)  :*${fim_handle_ps:-no}
	NEF/RAW  file support (libufraw)    :*${fim_handle_ufraw:-no}
	TAR/ZIP/.. file support (libarchive):*${fim_handle_archive:-no}
	AVIF     file support (libavif)     :*${fim_handle_avif:-no}
	WEBP     file support (libwebp)     :*${fim_handle_webp:-no}
	GraphicsMagick support (libGraphicsMagick):*${fim_handle_graphicsmagick:-no}

        YACC     : ${YACC}
        LEX      : ${LEX}
        CXX      : ${CXX}
        CXXFLAGS : ${CXXFLAGS}
        LIBS     : ${LIBS}
        prefix   : ${prefix}
dnl        LDFLAGS  : ${LDFLAGS}
dnl Still out:
dnl LEXLIB
dnl LIBAA_CONFIG 
dnl LIBPNG_CONFIG 
dnl LIBSDL_CONFIG 
dnl jasper ?
dnl archive ?

	If you are not satisfied with this configuration, please invoke
	  ./configure --help=short
	for a complete list of configure options and variables specific to Fim.
	When you are satisfied, run 'make' to build Fim and its documentation.
	To install the program and its documentation, run 'make install' if you are root,
	or run 'su -c '"make install"' if you are not root.
	RTFM: README, man fim, man fimrc.  Enjoy!
])
dnl
if test x$ac_cv_header_FlexLexer_h != xyes; then
	AC_MSG_WARN([
	********************************************************************
	It seems you don't have a working FlexLexer.h !
       	This is NOT what you want: Fim will build, but not work as intended!
	********************************************************************
	])
fi
