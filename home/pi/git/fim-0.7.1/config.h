/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* custom CXXFLAGS */
/* #undef CXXFLAGS */

/* Enable pdf support. */
/* #undef ENABLE_PDF */

/* program to use for shell commands */
#define EXECSHELL "/bin/sh"

/* */
#define FBI_AUTHOR "Gerd Hoffmann <kraxel _CUT_ bytesex.org>"

/* */
#define FBI_AUTHOR_NAME "Gerd Hoffmann"

/* Allows the user to specify a file loader */
#define FIM_ALLOW_LOADER_STRING_SPECIFICATION 1

/* */
#define FIM_AUTHOR "Michele Martone <dezperado _CUT_ autistici _CUT_ org>"

/* */
#define FIM_AUTHOR_EMAIL "<dezperado _CUT_ autistici _CUT_ org>"

/* */
#define FIM_AUTHOR_NAME "Michele Martone"

/* */
#define FIM_AUTOCMDS 1

/* */
#define FIM_AUTOSKIP_FAILED 1

/* */
/* #undef FIM_BIG_ENDIAN */

/* */
#define FIM_BOZ_PATCH 1

/* */
/* #undef FIM_CACHE_DEBUG */

/* */
#define FIM_CHECK_DUPLICATES 1

/* */
#define FIM_CHECK_FILE_EXISTENCE 1

/* */
#define FIM_COMMAND_AUTOCOMPLETION 1

/* */
#define FIM_CONFIGURATION "./configure --disable-readline"

/* */
#define FIM_DEFAULT_CONFIG 1

/* */
#define FIM_DEFAULT_CONFIGURATION 1

/* */
/* #undef FIM_DEFAULT_CONSOLEFONT */

/* */
#define FIM_DEFAULT_KEY_CONFIG 1

/* */
#define FIM_EXPERIMENTAL_ROTATION 1

/* */
/* #undef FIM_HANDLE_GIF */

/* */
/* #undef FIM_HANDLE_QOI */

/* */
/* #undef FIM_HANDLE_TIFF */

/* */
#define FIM_HAS_TIMEOUT 1

/* */
#define FIM_ITERATED_COMMANDS 1

/* */
/* #undef FIM_KEEP_BROKEN_CONSOLE */

/* */
#define FIM_LINUX_CONSOLEFONTS_DIR_SCAN 1

/* */
#define FIM_NAMESPACES 1

/* */
#define FIM_NOFB 1

/* */
/* #undef FIM_NOFIMRC */

/* */
#define FIM_NOHISTORY 1

/* */
#define FIM_PATHS_IN_MAN 1

/* */
#define FIM_PIPE_IMAGE_READ 1

/* */
#define FIM_RANDOM 1

/* */
#define FIM_READ_DIRS 1

/* */
#define FIM_READ_STDIN 1

/* */
#define FIM_READ_STDIN_IMAGE 1

/* */
#define FIM_RECORDING 1

/* */
#define FIM_RECURSIVE_DIRS 1

/* */
#define FIM_REMOVE_FAILED 1

/* */
#define FIM_SKIP_KNOWN_FILETYPES 1

/* */
#define FIM_SMART_COMPLETION 1

/* */
#define FIM_SWITCH_FIXUP 1

/* */
#define FIM_TMP_FILENAME ""

/* */
/* #undef FIM_TRY_CONVERT */

/* */
/* #undef FIM_TRY_DCRAW */

/* */
/* #undef FIM_TRY_DIA */

/* */
/* #undef FIM_TRY_INKSCAPE */

/* */
/* #undef FIM_TRY_XCF2PNM */

/* */
/* #undef FIM_TRY_XCFTOPNM */

/* */
/* #undef FIM_TRY_XFIG */

/* */
/* #undef FIM_USE_DESIGNATED_INITIALIZERS */

/* */
/* #undef FIM_USE_READLINE */

/* */
#define FIM_USE_ZCAT 1

/* */
#define FIM_VERSION "0.7.1"

/* */
/* #undef FIM_WANTS_SLOW_RESIZE */

/* */
#define FIM_WANT_CUSTOM_INFO_STATUS_BAR 1

/* If defined, exiftool will be used to get additional file information
   (EXPERIMENTAL). */
/* #undef FIM_WANT_EXIFTOOL */

/* */
/* #undef FIM_WANT_FB_CONSOLE_SWITCH_WHILE_LOADING */

/* Fim will be capable of marking files while viewing and outputting their
   names on exit. */
#define FIM_WANT_FILENAME_MARK_AND_DUMP 1

/* */
#define FIM_WANT_FONT_MAGNIFY_FACTOR 0

/* Enable a hardcoded font in the executable. */
#define FIM_WANT_HARDCODED_FONT 1

/* Internals shall use 64 bit integers. */
#define FIM_WANT_LONG_INT 1

/* Scripting. (enabled by default) */
#define FIM_WANT_MOUSE 1

/* Scripting. (enabled by default) */
/* #undef FIM_WANT_NOSCRIPTING */

/* Output console. (enabled by default) */
/* #undef FIM_WANT_NO_OUTPUT_CONSOLE */

/* */
#define FIM_WANT_POPEN_CALLS 1

/* Enables raw bits rendering */
#define FIM_WANT_RAW_BITS_RENDERING 1

/* */
#define FIM_WANT_SEEK_MAGIC 1

/* */
/* #undef FIM_WANT_STATIC_BINARY */

/* */
#define FIM_WANT_SYSTEM_CALLS 1

/* Enables text rendering */
#define FIM_WANT_TEXT_RENDERING 1

/* Fim windowing support */
#define FIM_WINDOWS 1

/* */
/* #undef FIM_WITH_AALIB */

/* Defined, if (EXPERIMENTAL, INCOMPLETE) libarchive support is enabled */
/* #undef FIM_WITH_ARCHIVE */

/* BMP file support. */
#define FIM_WITH_BMP 1

/* */
/* #undef FIM_WITH_DEBUG */

/* AVIF file support. */
#define FIM_WITH_LIBAVIF 0

/* */
#define FIM_WITH_LIBCACA 1

/* */
/* #undef FIM_WITH_LIBEXIF */

/* We have GTK */
/* #undef FIM_WITH_LIBGTK */

/* We have libImlib */
/* #undef FIM_WITH_LIBIMLIB2 */

/* */
/* #undef FIM_WITH_LIBJASPER */

/* Defined, if libpng support is enabled */
#define FIM_WITH_LIBPNG 1

/* Defined if libsdl support is enabled, and set to 1 or 2. */
#define FIM_WITH_LIBSDL 1

/* WebP file support. */
#define FIM_WITH_LIBWEBP 0

/* Defined if sample bogus 'xyz' loader support is enabled. */
/* #undef FIM_WITH_LIBXYZ */

/* Defined when framebuffer device support is turned off. */
/* #undef FIM_WITH_NO_FRAMEBUFFER */

/* PCX file support. */
#define FIM_WITH_PCX 1

/* QOI file support. */
/* #undef FIM_WITH_QOI */

/* Defined, if UFRaw support is enabled */
/* #undef FIM_WITH_UFRAW */

/* Define to 1 if you have the <avif/avif.h> header file. */
/* #undef HAVE_AVIF_AVIF_H */

/* Define to 1 if you have the `bcmp' function. */
#define HAVE_BCMP 1

/* Define to 1 if you have the `bcopy' function. */
#define HAVE_BCOPY 1

/* Define to 1 if you have the `bzero' function. */
#define HAVE_BZERO 1

/* Define to 1 if you have the <climits> header file. */
#define HAVE_CLIMITS 1

/* Define to 1 if you have the <cstdio> header file. */
#define HAVE_CSTDIO 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the `drand48' function. */
#define HAVE_DRAND48 1

/* Define to 1 if you have the <emscripten.h> header file. */
/* #undef HAVE_EMSCRIPTEN_H */

/* If present, fgetln() will be used as a replacement for getline(). (on BSD)
   */
/* #undef HAVE_FGETLN */

/* Define to 1 if you have the `fileno' function. */
#define HAVE_FILENO 1

/* Define to 1 if you have the <filesystem> header file. */
#define HAVE_FILESYSTEM 1

/* We have FlexLexer.h */
#define HAVE_FLEXLEXER_H 1

/* Define to 1 if you have the `fmemopen' function. */
#define HAVE_FMEMOPEN 1

/* Define to 1 if you have the <fnmatch.h> header file. */
#define HAVE_FNMATCH_H 1

/* Define to 1 if you have the <fontconfig/fcfreetype.h> header file. */
/* #undef HAVE_FONTCONFIG_FCFREETYPE_H */

/* Define to 1 if you have the <fontconfig/fontconfig.h> header file. */
#define HAVE_FONTCONFIG_FONTCONFIG_H 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#define HAVE_FSEEKO 1

/* If present, will be used for stdin reading. (on GNU) */
#define HAVE_GETDELIM 1

/* If present, the getenv function allows fim to read environment variables.
   */
#define HAVE_GETENV 1

/* If present, will be used for stdin reading. (on GNU) */
#define HAVE_GETLINE 1

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Should live when _GNU_SOURCE, but it could be not the case. */
#define HAVE_GET_CURRENT_DIR_NAME 1

/* Define to 1 if you have the <glob.h> header file. */
#define HAVE_GLOB_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <ioctl.h> header file. */
/* #undef HAVE_IOCTL_H */

/* Define to 1 if you have the `curses' library (-lcurses). */
/* #undef HAVE_LIBCURSES */

/* We have libdjvulibre */
/* #undef HAVE_LIBDJVU */

/* Define to 1 if you have the <libexif/exif-data.h> header file. */
/* #undef HAVE_LIBEXIF_EXIF_DATA_H */

/* Define to 1 if you have the `fl' library (-lfl). */
/* #undef HAVE_LIBFL */

/* Define to 1 if you have the <libgen.h> header file. */
#define HAVE_LIBGEN_H 1

/* We have libGraphicsMagick */
/* #undef HAVE_LIBGRAPHICSMAGICK */

/* We have libjpeg */
/* #undef HAVE_LIBJPEG */

/* Define to 1 if you have the `m' library (-lm). */
#define HAVE_LIBM 1

/* Define to 1 if you have the `ncurses' library (-lncurses). */
/* #undef HAVE_LIBNCURSES */

/* We have libpoppler (Warning: the API could still break! (as of
   v.0.8.7--0.24.1)) */
/* #undef HAVE_LIBPOPPLER */

/* We have libspectre */
/* #undef HAVE_LIBSPECTRE */

/* Define to 1 if you have the <limits.h> header file. */
/* #undef HAVE_LIMITS_H */

/* Define to 1 if you have the <linux/fb.h> header file. */
#define HAVE_LINUX_FB_H 1

/* Define to 1 if you have the <linux/kd.h> header file. */
#define HAVE_LINUX_KD_H 1

/* Define to 1 if you have the <linux/vt.h> header file. */
#define HAVE_LINUX_VT_H 1

/* */
/* #undef HAVE_MATRIX_MARKET_DECODER */

/* Define to 1 if you have the `memcmp' function. */
#define HAVE_MEMCMP 1

/* Have memmem */
#define HAVE_MEMMEM 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the <minix/config.h> header file. */
/* #undef HAVE_MINIX_CONFIG_H */

/* Define to 1 if you have a working `mmap' system call. */
#define HAVE_MMAP 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* We have pipe() */
#define HAVE_PIPE 1

/* Define to 1 if you have the <qoi.h> header file. */
/* #undef HAVE_QOI_H */

/* Define to 1 if you have the `rand' function. */
#define HAVE_RAND 1

/* Define to 1 if you have the `random' function. */
#define HAVE_RANDOM 1

/* Define to 1 if you have the `regcomp' function. */
#define HAVE_REGCOMP 1

/* We have <regex> */
#define HAVE_REGEX 1

/* We have regex.h */
#define HAVE_REGEX_H 1

/* Define to 1 if you have the <stdarg.h> header file. */
#define HAVE_STDARG_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
/* #undef HAVE_STDIO_H */

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcpy' function. */
#define HAVE_STRCPY 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncmp' function. */
#define HAVE_STRNCMP 1

/* Have sync */
#define HAVE_SYNC 1

/* Define to 1 if you have the <sysexits.h> header file. */
#define HAVE_SYSEXITS_H 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* We have sys/ioctl.h */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#define HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/resource.h> header file. */
#define HAVE_SYS_RESOURCE_H 1

/* We have sys/select.h */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/user.h> header file. */
#define HAVE_SYS_USER_H 1

/* We have sys/wait.h */
#define HAVE_SYS_WAIT_H 1

/* We have termios.h */
#define HAVE_TERMIOS_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <unix.h> header file. */
/* #undef HAVE_UNIX_H */

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* Define to 1 if you have the <webp/decode.h> header file. */
/* #undef HAVE_WEBP_DECODE_H */

/* Define to 1 if you have the <webp/demux.h> header file. */
/* #undef HAVE_WEBP_DEMUX_H */

/* Define to 1 if you have the <wordexp.h> header file. */
#define HAVE_WORDEXP_H 1

/* We have _regex.h */
/* #undef HAVE__REGEX_H */

/* Name of package */
#define PACKAGE "fim"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "dezperado_FOobAr_autistici_Baz_org, by replacing _FOobAr_ with a @ and _Baz_ with a ."

/* Define to the full name of this package. */
#define PACKAGE_NAME "fim"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "fim 0.7.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "fim"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.7.1"

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `int *', as computed by sizeof. */
#define SIZEOF_INT_P 4

/* The size of `off_t', as computed by sizeof. */
#define SIZEOF_OFF_T 8

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 4

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* SVN REVISION */
#define SVN_REVISION "22552256"

/* SVN REVISION NUMBER */
#define SVN_REVISION_NUMBER 22552256

/* Define if you want to use the included (GNU) regex.c. */
/* #undef USE_GNU_REGEX */

/* Enable extensions on AIX 3, Interix.  */
#ifndef _ALL_SOURCE
# define _ALL_SOURCE 1
#endif
/* Enable general extensions on macOS.  */
#ifndef _DARWIN_C_SOURCE
/* # undef _DARWIN_C_SOURCE */
#endif
/* Enable general extensions on Solaris.  */
#ifndef __EXTENSIONS__
# define __EXTENSIONS__ 1
#endif
/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1
#endif
/* Enable X/Open compliant socket functions that do not require linking
   with -lxnet on HP-UX 11.11.  */
#ifndef _HPUX_ALT_XOPEN_SOCKET_API
/* # undef _HPUX_ALT_XOPEN_SOCKET_API */
#endif
/* Identify the host operating system as Minix.
   This macro does not affect the system headers' behavior.
   A future release of Autoconf may stop defining this macro.  */
#ifndef _MINIX
/* # undef _MINIX */
#endif
/* Enable general extensions on NetBSD.
   Enable NetBSD compatibility extensions on Minix.  */
#ifndef _NETBSD_SOURCE
/* # undef _NETBSD_SOURCE */
#endif
/* Enable OpenBSD compatibility extensions on NetBSD.
   Oddly enough, this does nothing on OpenBSD.  */
#ifndef _OPENBSD_SOURCE
/* # undef _OPENBSD_SOURCE */
#endif
/* Define to 1 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_SOURCE
/* # undef _POSIX_SOURCE */
#endif
/* Define to 2 if needed for POSIX-compatible behavior.  */
#ifndef _POSIX_1_SOURCE
/* # undef _POSIX_1_SOURCE */
#endif
/* Enable POSIX-compatible threading on Solaris.  */
#ifndef _POSIX_PTHREAD_SEMANTICS
# define _POSIX_PTHREAD_SEMANTICS 1
#endif
/* Enable extensions specified by ISO/IEC TS 18661-5:2014.  */
#ifndef __STDC_WANT_IEC_60559_ATTRIBS_EXT__
/* # undef __STDC_WANT_IEC_60559_ATTRIBS_EXT__ */
#endif
/* Enable extensions specified by ISO/IEC TS 18661-1:2014.  */
#ifndef __STDC_WANT_IEC_60559_BFP_EXT__
/* # undef __STDC_WANT_IEC_60559_BFP_EXT__ */
#endif
/* Enable extensions specified by ISO/IEC TS 18661-2:2015.  */
#ifndef __STDC_WANT_IEC_60559_DFP_EXT__
/* # undef __STDC_WANT_IEC_60559_DFP_EXT__ */
#endif
/* Enable extensions specified by ISO/IEC TS 18661-4:2015.  */
#ifndef __STDC_WANT_IEC_60559_FUNCS_EXT__
/* # undef __STDC_WANT_IEC_60559_FUNCS_EXT__ */
#endif
/* Enable extensions specified by ISO/IEC TS 18661-3:2015.  */
#ifndef __STDC_WANT_IEC_60559_TYPES_EXT__
/* # undef __STDC_WANT_IEC_60559_TYPES_EXT__ */
#endif
/* Enable extensions specified by ISO/IEC TR 24731-2:2010.  */
#ifndef __STDC_WANT_LIB_EXT2__
/* # undef __STDC_WANT_LIB_EXT2__ */
#endif
/* Enable extensions specified by ISO/IEC 24747:2009.  */
#ifndef __STDC_WANT_MATH_SPEC_FUNCS__
/* # undef __STDC_WANT_MATH_SPEC_FUNCS__ */
#endif
/* Enable extensions on HP NonStop.  */
#ifndef _TANDEM_SOURCE
# define _TANDEM_SOURCE 1
#endif
/* Enable X/Open extensions.  Define to 500 only if necessary
   to make mbstate_t available.  */
#ifndef _XOPEN_SOURCE
/* # undef _XOPEN_SOURCE */
#endif


/* Version number of package */
#define VERSION "0.7.1"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* Define to 1 if `lex' declares `yytext' as a `char *' by default, not a
   `char[]'. */
#define YYTEXT_POINTER 1

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to 1 if type `char' is unsigned and your compiler does not
   predefine this macro.  */
#ifndef __CHAR_UNSIGNED__
/* # undef __CHAR_UNSIGNED__ */
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
