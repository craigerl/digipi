
FIM - Fbi IMproved    README document.
--------------------------------------------------------------------------------
 - [	1		Overview](#1overview)
 - [	2		Description](#2description)
 - [	3		Features, comparison to other image viewers](#3features-comparison-to-other-image-viewers)
 - [	4		Build, test, example](#4build-test-example)
 - [	5		Run time requirements](#5run-time-requirements)
 - [	6		Original Idea](#6original-idea)
 - [	7		Notes for patch writers](#7notes-for-patch-writers)
 - [	8		Availability in Linux distributions](#8availability-in-linux-distributions)
 - [	9		License](#9license)
 - [	10		Contacts, mailing lists, URLs](#10contacts-mailing-lists-urls)

## <a id="1overview"></a>[	1		Overview](#1overview) ##
--------------------------------------------------------------------------------

FIM (Fbi IMproved) is a highly customizable and scriptable image viewer targeted
at the users who are comfortable with software like the Vim text editor or the
Mutt mail user agent, or keyboard oriented, full screen programs.
FIM aims to be a "Swiss Army knife" for viewing images.

FIM is multidevice: it has X11/Wayland support (via the SDL and GTK libraries),
it supports ASCII art output (via the aalib and libcaca libraries), and because
it derives from the Fbi image viewer (by Gerd Hoffmann), it can display images 
in the Linux framebuffer console, too.

FIM is free software, distributed under the terms of the GPL software license.


## <a id="2description"></a>[	2		Description](#2description) ##
--------------------------------------------------------------------------------

FIM invocation is documented in `man fim` (/#man_fim).
The FIM language (syntax, commands, variables, key bindings) is documented in
 `man fimrc` (/#man_fimrc).

FIM offers many ways for scaling, orienting, listing or rearranging the
ordering of images.
FIM is capable of regular expressions based (on filename) vim-like autocommands,
comment-based search and filtering, display of EXIF tags, customizable command
lines, and use a custom font (from console fonts).
FIM offers GNU `readline` command line autocompletion and history,
completely customizable key bindings, internal/external scriptability
(through return codes, standard input/output, commands given at invocation time,
 and an initialization file), internal filename-based image search, and more.
Each documentation item is accessible via the internal `help` command.


## <a id="3features-comparison-to-other-image-viewers"></a>[	3		Features, comparison to other image viewers](#3features-comparison-to-other-image-viewers) ##
--------------------------------------------------------------------------------

Features:

 * modes: interactive (default), and command line
 * lots of the functionality is scripted in an internal domain specific language
 * on-screen debug and information console in command mode
 * per-image variables (e.g.: `i:var="value"`)
 * regular expression filename search (`/*.png`) and view
 * command line history (enter the console with `':'`, then use arrows)
 * command line Tab-based command autocompletion (thanks to the GNU readline)
 * command line history file save/load support (`~/.fim_history`)
 * key-action bindings, with Shift (uppercase and Control ('C-') key combinations
 * simple `if/while` scriptability, with integer/float/string variables and arithmetic evaluation
 * command aliases support (macros)
 * event autocommands (styled after Vim's `autocmd` )
 * regular expressions on filenames to enrich autocommand behaviour
 * image descriptions display ('expando' sequences)
 * regular expressions on image descriptions
 * configuration (initialization) file support (`~/.fimrc`)
 * usable built-in default configuration
 * wrapper script (`fimgs`) to convert pdf,ps,eps,dvi,cbr(rar),cbz(zip),tar,tar.gz,tgz,http://...
   contents into images that can be displayed
 * embedded piping through `convert`,`inkscape`,`xcftopnm`,`dia`... to visualize further file formats
 * view a Matrix Market file rendering via librsb
 * index-based `goto` styled after Vim (`:<number>`)
 * image search with regular expressions on filenames (`goto` command)
 * other stuff   usual for an image viewer (`scale`, `pan,` etc..)
 * other stuff unusual for an image viewer (setting scale factor, auto width scale,
   auto height scale, marking of 'interesting' files, sorting of files in the
   list, recording/replaying of sessions, repeat last action )
 * image flip, mirror, rotation, stretch (asymmetric scaling)
 * command iteration (with the `[n]<command key>` syntax
 * external script file execution
 * system call support
 * image cache and mipmaps
 * image prefetch
 * pipe commands from an input program
 * many features could be enabled or disabled at compile time editing the Makefile
 * run under screen or ssh
 * read files list via standard input
 * read image file via standard input
 * read commands script via standard input
 * support for GTK (`-o gtk`) (X11/Wayland)
 * support for SDLlib (Simple Directmedia Layer) (`-o sdl`) (X11/Wayland)
 * support for AAlib (ASCII Art rendering) (`-o aa`)
 * support for libcaca (Colour ASCII Art rendering) (`-o ca`)
 * experimental (incomplete, discontinued) support for Imlib2 (`-o imlib2`)
 * experimental support for file formats: PDF, PS, DJVU
 * view any file as a binary/RGB pixelmap
 * view any file as (rendered) text
 * open files at a specified byte offset
 * image list limiting (temporary list shrinking)
 * comments / captions / description files
 * search and reordering based on descriptions and internal variables
 * decode files based on magic number (contents), not file extension
 * recursive load from directory
 * sort files list on file name, file size or name date
 * text font control (selection of size and consolefont)
 * dump list of selected images to `stdout`
 * jump between last two images repeatedly with one key

 The next table is a comparison (very outdated...) of popular image viewers 
 available on Linux, focusing on FIM-like free software ones.

|  Program:                  |kuickshow|eog|xz|gv|fbi|FIM|GQview|dfbsee|pv|qiv|
|--------------------------------|-----|---|--|--|---|---|------|------|--|---|
|status:(a)lpha/(m)ature         |   m | m | m|  | a |  m|   m  |      |m | m |
|X=X,f=framebuffer,s=SVGA,a=ascii|   X | X |Xs| X| f |fXa|   X  |  ?   |f | X |
|linux specific                  |     |   | ?|  | * |  *|      |      |* | ? |
|key rebindings                  |   * |   |  |  |   |  *|   *  |      |  | ? |
|external scriptability          |     |   |  |  | ~ |  *|      |      |  | ? |
|internal scriptability          |     |   |  |  |   |  *|      |      |  | ? |
|internal scriptability          |     |   |  |  |   |  *|      |      |  | ? |
|printing support                |   * | * | ?|  |   |   |   *  |      |  | ? |
|slideshow                       |   * | * |  |  | * |  *|   *  |      |* | * |
|caching                         |   * | ? |  |  |   |  *|   *  |      |  | ? |
|preview browser                 |   * |   |  |  |   |   |   *  |      |  | ? |
|EXIF tag display                |   * |   |  |  | * |  *|   *  |      |  | ? |
|internal windowing              |     |   |  |  |   |  X|      |      |  | ? |
|internal image search           |     |   |  |  |   |  *|      |      |  | ? |
|external image search           |     |   |  |  |   |   |   *  |      |  | ? |
|'pan views' ('rich' views)      |     |   |  |  |   |   |   *  |      |  | ? |
|system interaction              |   * |   |  |  |   |  *|   *  |      |  | ? |
|system interaction safe         |   * |   |  |  |   |   |      |      |  | ? |
|remote commands                 |     |   |  |  |   |   |   *  |      |  | ? |
|saves last folder               |   * |   |  |  |   |   |   *  |      |  | ? |
|runs under screen               |     |   |  |  |   |  *|      |      |  | ? |
|standard input interaction      |     |   |  |  |   |  *|      |      |  | ? |
|rotation                        |   * | ? | ?|  | ? |  *|   ?  |  ?   |* | * |
|history                         |     |   |  |  |   |  *|      |      |  |   |
|multi-device                    |     |   |  |  |   |  *|      |      |  |   |
|mirroring                       |     |   |  |  |   |  *|      |      |  | * |
|description files               |     |   |  |  |   |  *|      |      |  |   |


Other nice command line picture viewers:  pv (http://www.trashmail.net/pv/),
 zgv, feh, sxiv, mirage...

 ______________________________________________________________________________
 


## <a id="4build-test-example"></a>[	4		Build, test, example](#4build-test-example) ##
--------------------------------------------------------------------------------


Requirements are:

 * the GNU readline library ( https://www.gnu.org/software/readline/ )
 * GNU flex  (NOT any lex ) ( https://www.gnu.org/software/flex/  )
   (some systems need also 'libfl-dev'  to find `FlexLexer.h`)
 * GNU bison (NOT any yacc) ( https://www.gnu.org/software/bison/)
 * the GCC ( GNU Compiler Collection ) ( http://gcc.gnu.org/ )
   or clang ( https://clang.llvm.org/ )
 * and optionally:
	* GTK-3	                  ( https://gtk.org/ )
	* libsdl-1.2		  ( https://libsdl.org/ )
	* libsdl-2.0		  ( https://libsdl.org/ )
	* libexif		  ( https://libexif.github.io/ )
	* libjpeg		  ( https://www.ijg.org/ )
	* libpng		  (  http://www.libpng.org/ )
	* giflib		  ( https://sourceforge.net/projects/giflib/ )
	* libtiff		  (  http://www.libtiff.org/ )
	* libdjvulibre	  ( https://djvu.sourceforge.net/ )
	* libwebp         ( https://github.com/webmproject/libwebp/ )
	* libavif         ( https://github.com/AOMediaCodec/libavif/ )
	* libqoi          ( https://qoiformat.org/ )
	* ...


 Libraries originally required by Fbi-1.31 but not by FIM:
 	libFS, libCURL, libLIRC


Usually, building and installing FIM proceeds like:

	./configure --help # get options you may pass to configure
	./configure
	make
	make test
	sudo make install

If you wish to run `./configure` again with different options, it is
recommended to `make clean` before that.

--------------------------------------------------------------------------------
## <a id="41test"></a>[	4.1		Test](#41test) ##

 
	# run fim in interactive mode (press q to quit)
	make test

	# run the test suite
	make tests


You can also run separately parts of `make tests`:

	# run only -o fbdev tests (if disabled, fail)
	make fbtests

	# run only -o gtk tests (if disabled, fail)
	make gtktests

	# run only -o sdl tests (if disabled, fail)
	make sdltests

	# run only -o aa tests (if disabled, fail)
	make aatests

	# run only -o ca tests (if disabled, fail)
	make cacatests

--------------------------------------------------------------------------------
## <a id="42example-on-debian"></a>[	4.2		Example on Debian](#42example-on-debian) ##

	sudo apt install subversion
	sudo apt install flex libfl-dev bison
	sudo apt install automake autoconf libtool # to generate configure, etc
	sudo apt install pkg-config                # to generate configure
	sudo apt install libreadline-dev libexif-dev
	sudo apt install libjpeg-dev libpng-dev libtiff-dev libgif-dev
	sudo apt install libsdl2-dev libaa1-dev libcaca-dev libgtk-3-dev
	# the following are experimental, less recommended:
	sudo apt install libpoppler-cpp-dev libpoppler-dev libpoppler-private-dev
	sudo apt install libdjvulibre-dev libspectre-dev
	sudo apt install libqoi-dev libwebp-dev libavif-dev
	sudo apt install libarchive-dev
	svn co http://svn.savannah.nongnu.org/svn/fbi-improved/trunk/ fim
	cd fim
	autoreconf -i
	./configure --enable-aa --enable-caca --enable-sdl --enable-gtk
	make
	sudo make install
 

## <a id="5run-time-requirements"></a>[	5		Run time requirements](#5run-time-requirements) ##
--------------------------------------------------------------------------------

 * Linux (not sure if it is necessary for it to be an x86; i think not)
 * The framebuffer device ( e.g.: `/dev/fb0` or `/dev/fb/0` ) enabled in the kernel
   ( and usually found in `"/usr/src/linux/Documentation/fb"` in the kernel source
    code tree ).
 * unless disabled, FIM will create a history (`~/.fim_history`) file in the
    `${HOME}` directory when leaving (so it assumes ${HOME} to exist)
 * shared library files for e.g.: libpng, libjpeg, libgif, libtiff, libreadline,
   libexif, ...


## <a id="6original-idea"></a>[	6		Original Idea](#6original-idea) ##
--------------------------------------------------------------------------------

FIM derives from the wonderful fbi-1.31, written by Gerd Hoffmann/Knorr:
 ( Fbi can be obtained at https://www.kraxel.org/blog/linux/fbida/ ).

It all started in 2005, developing a small 'vim-like fbi patch', and following
features to enrich Fbi with a command line and more customizations.

FIM is a significant reorganization and expansion of the Fbi code, and aims at
obtaining the most scriptable and configurable image viewer ever.


## <a id="7notes-for-patch-writers"></a>[	7		Notes for patch writers](#7notes-for-patch-writers) ##
--------------------------------------------------------------------------------

To run, FIM requires a Linux box with X, or the framebuffer device enabled in
the kernel, and some popular image file decoding libraries.

Information about the framebuffer can be found under the directory
 "./Documentation/fb"
 inside the kernel tree 
 (usually "/usr/src/linux/Documentation/fb" ).

The file decoding libraries are listed in an earlier section.
Tested and working with library SDL-1.2.12 through SDL-1.2.15.
Experimental support for SDL-2.0 and GTK-3.0.

From the original Fbi README, it reads that Gerd himself wrote FBI hacking
 "a svgalib PhotoCD viewer", so regard this software as a big, dirty code
potpourri :) .
For the sake of completeness, FIM started as a fork of version 1.31 of fbi,
available from http://dl.bytesex.org/releases/fbida/fbi_1.31.tar.gz .

Useful documents I've read and consulted during the coding of Fim, and 
useful for hacking it:

 * Thomas Niemann's tutorial to yacc & lex
 * The yacc & lex HOWTO
 * GNU readline manual
 * Ray Lischner, STL Pocket Reference, O'Reilly, 2004
 * Herbert Schildt "C++ - Complete Reference" Third Edition, Osborne McGraw-Hill, 1999
 * http://www.gnu.org/software/m4/manual/
 * http://www.gnu.org/software/autoconf/manual/
 * http://www.gnu.org/software/automake/manual/
 * http://www.gnu.org/software/make/manual/
 * http://www.gnu.org/software/bash/manual/
 * http://www.gnu.org/software/bison/manual/
 * http://cppreference.com/
 * `man console_codes`
 * `man fb.modes`
 * `man fbi`
 * `man console_ioctl`
 * `man resizecons`

Further useful documents:
 * `vim -c ':help'`
 * `man 3 history`
 * `man readline`
 * http://www.gnu.org/prep/standards/

Useful URLs:
 * http://www.tldp.org/HOWTO/Framebuffer-HOWTO.html
and mirrors
 * http://www.linux.org/docs/ldp/howto/Framebuffer-HOWTO.html
 * http://www.pluto.it/files/ildp/HOWTO/Framebuffer-HOWTO/Framebuffer-HOWTO.html
and
 * http://bisqwit.iki.fi/source/fbmodes.html
 * http://asm.sourceforge.net/articles/fb.html
 * http://www.linuxjournal.com/article/2783

Platforms tested in the past versions (fim-0.3...):
 * 2.6.25-2-686 Linux Kernel, GCC-4.3.1, flex 2.5.35, bison 2.3, Debian Lenny, x86
 * 2.6.17 Linux Kernel, GCC-3.4.6, gentoo, x86
 * 2.6.17 Linux Kernel, GCC-4.1.1, gentoo, x86
 * 2.6.17 Linux Kernel, GCC-3.3.6, flex 2.5.4, bison 1.875d, gentoo, x86
 * 2.6.17 Linux Kernel, GCC-3.3.6, flex 2.5.4, bison 2.2   , gentoo, x86
 * 2.6.17 Linux Kernel, GCC-3.3.6, flex 2.5.33,bison 2.2   , gentoo, x86
 * 2.6.19 Linux Kernel, GCC-4.1.1, flex 2.5.33,bison 2.2   , gentoo, powerpc
 * other ones, but non documented here.


 If you intend to write patches or contribute to the code, be sure of reading 
 all of the documentation and _write me an email first_ (I will give you some
 advice).


--------------------------------------------------------------------------------
## <a id="71hacking-maintenance-guidelines"></a>[	7.1		Hacking, maintenance guidelines](#71hacking-maintenance-guidelines) ##

If you hack FIM in an interesting way, consider submitting your changes as a
patch.

 * If you introduce support for a new file format: `fim -V` should list the
   your new format, too (so you should update it).
 * Same for a new output device.
 * FIM should continue passing the tests after your patch, and your patch should
   be robust, too. Consider writing a new test case.
 * In particular, the developer tests should pass:
   `make devtests dist dist-tests`.
 * If you're experimenting on Android's Termux, for gtk3 you may need
   'xorgproto' to provide you 'xproto' (applies to Termux version 0.118.0).


## <a id="8availability-in-linux-distributions"></a>[	8		Availability in Linux distributions](#8availability-in-linux-distributions) ##
--------------------------------------------------------------------------------

--------------------------------------------------------------------------------
## <a id="81debian-ubuntu"></a>[	8.1		Debian, Ubuntu](#81debian-ubuntu) ##
 
 You should find fim in Debian and Ubuntu, and install it with:

	sudo apt install fim

 But beware: the deb packages may be outdated.
 

 In order to build from repository, you are advised to install packages from
 the following (overly complete) list:
  
 autoconf autoconf-archive autoheader automake autotools-dev bison ctags
 flex groff g++ libaa1-dev libcaca-dev libdjvulibre-dev libgif4 libgif-dev
 libjpeg-dev libncurses5-dev libpng-dev libtiff-dev libexif-dev
 libpoppler-cpp-dev libpoppler-dev libpoppler-private-dev libreadline-dev 
 libsdl2-dev libspectre-dev libtool m4 make svn txt2html

 The above list was valid on a Debian Jessie installation; it's possible that
 these packages names change with time.
 

## <a id="9license"></a>[	9		License](#9license) ##
--------------------------------------------------------------------------------

 FIM is free software, and is licensed under the GPLv2 or later.
 FIM has been written by Michele Martone.

 FIM started as a fork of fbi-1.31 by Gerd Hoffmann, which is "GPLv2 or later".
 FIM uses the PCX reading code contributed by Mohammed Isam.
 FIM also uses a regex.c file from the GNU Regular Expressions library, 
distributed on http://directory.fsf.org/regex.html, in the version shipped with
the Mutt mail user agent ( http://www.mutt.org ).
 It also uses scripts shipped with the Vim text editor (http://www.vim.org),
which is licensed compatibly with the GPL.
 The FIM source code package includes the Lat15-Terminus16.psf file, originally
from the GPL licensed Terminus Font package, version 4.30 authored by
Dimitar Toshkov Zhekov.


## <a id="10contacts-mailing-lists-urls"></a>[	10		Contacts, mailing lists, URLs](#10contacts-mailing-lists-urls) ##
--------------------------------------------------------------------------------

FIM is (C) 2007-2024 Michele Martone.

 Email: "dezperado_FOobAr_autistici_Baz_org", just care replacing
         _FOobAr_ with a '@' and _Baz_ with a '.'.
 GPG Key: 0xE0E669C8EF1258B8
 GitHub:  https://github.com/michelemartone

If it is for a bug report or installation help, be sure of reading the
documentation and the BUGS file first.
FIM is not perfect: a number of weaknesses are summarized in the man pages,
the TODO file, and the BUGS file.


Homepage     : https://www.nongnu.org/fbi-improved/

Savannah Page: http://savannah.nongnu.org/projects/fbi-improved/

Announcements: http://freecode.com/projects/fbi-improved/

Mailing List : http://savannah.nongnu.org/mail/?group=fbi-improved
	(or http://lists.nongnu.org/mailman/listinfo/fbi-improved-devel)

Releases     : http://download.savannah.nongnu.org/releases/fbi-improved

Repository   : http://svn.savannah.nongnu.org/svn/fbi-improved

Off.  Mirror : http://www.autistici.org/dezperado/fim/

ChangeLog    : http://svn.savannah.nongnu.org/svn/fbi-improved/trunk/ChangeLog

Bugs (official) : http://savannah.nongnu.org/bugs/?group=fbi-improved

