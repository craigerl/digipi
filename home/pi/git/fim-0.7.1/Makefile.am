# $Id: Makefile.am 2255 2024-05-16 07:58:17Z dezperado $

# no-dependencies was due to problems with conditional sources
# nostdinc disables the standard -I. include which breaks a correct string.h include
AUTOMAKE_OPTIONS = nostdinc no-dependencies # dejagnu
SUBDIRS = src doc distros scripts media

dist_doc_DATA=$(top_srcdir)/README $(top_srcdir)/README.FIRST $(top_srcdir)/TODO $(top_srcdir)/VERSION $(top_srcdir)/BUGS  $(top_srcdir)/FAQ.TXT $(top_srcdir)/AUTHORS $(top_srcdir)/ChangeLog $(top_srcdir)/NEWS.in $(top_srcdir)/NEWS $(top_srcdir)/README.md
# no INSTALL INSTALL.TXT is necessary in rules
EXTRA_DIST=  COPYING  \
	Makefile.am configure.ac configure \
	 aclocal.m4 config.h.in \
	autogen.sh \
	$(top_srcdir)/media/image.jpg \
	$(top_srcdir)/media/image.png \
	$(top_srcdir)/media/sample.svg \
	$(top_srcdir)/media/fim.png \
	$(top_srcdir)/media/icon_smile.gif \
	$(top_srcdir)/media/numbers/0.gif \
	$(top_srcdir)/media/numbers/1.gif \
	$(top_srcdir)/media/numbers/2.gif \
	$(top_srcdir)/media/numbers/3.gif \
	$(top_srcdir)/media/numbers/4.gif \
	$(top_srcdir)/media/numbers/5.gif \
	$(top_srcdir)/media/numbers/6.gif \
	$(top_srcdir)/media/numbers/7.gif \
	$(top_srcdir)/media/numbers/8.gif \
	$(top_srcdir)/media/numbers/9.gif \
	$(top_srcdir)/media/numbers_pcx/0.pcx \
	$(top_srcdir)/media/numbers_pcx/1.pcx \
	$(top_srcdir)/media/numbers_pcx/2.pcx \
	$(top_srcdir)/media/numbers_pcx/3.pcx \
	$(top_srcdir)/media/numbers_pcx/4.pcx \
	$(top_srcdir)/media/numbers_pcx/5.pcx \
	$(top_srcdir)/media/numbers_pcx/6.pcx \
	$(top_srcdir)/media/numbers_pcx/7.pcx \
	$(top_srcdir)/media/numbers_pcx/8.pcx \
	$(top_srcdir)/media/numbers_pcx/9.pcx \
	$(top_srcdir)/media/special/icon_smile_gif_at_7.bin \
	$(top_srcdir)/media/special/images.desc \
	$(top_srcdir)/media/special/modifiers.desc \
	$(top_srcdir)/media/special/postscript.eps \
	$(top_srcdir)/media/special/images.list \
	$(top_srcdir)/media/special/dir.desc \
	$(top_srcdir)/media/special/collated.list \
	$(top_srcdir)/media/special/loadable.desc \
	$(top_srcdir)/media/special/0_8bit.pcx.bin \
	$(top_srcdir)/media/special/0_gray.tiff \
	$(top_srcdir)/media/special/0_rgba.tiff \
	$(top_srcdir)/media/special/0_monochrome_white_polarity.tiff \
	$(top_srcdir)/media/special/0_16bps_msb.tiff \
	$(top_srcdir)/media/special/0_interlaced.gif \
	$(top_srcdir)/media/special/0_truncated_bad.gif.bin \
	$(top_srcdir)/media/special/0.webp \
	$(top_srcdir)/media/special/0.avif \
	$(top_srcdir)/media/special/0.xcf \
	$(top_srcdir)/media/special/0.ppm \
	$(top_srcdir)/media/special/truncated_bad.png.bin \
	$(top_srcdir)/media/special/truncated_bad.jpg.bin \
	$(top_srcdir)/media/special/0_gray.qoi \
	$(top_srcdir)/media/special/0_gray.png \
	$(top_srcdir)/media/special/numbers_pcx.tar \
	$(top_srcdir)/media/special/numbers_pcx.tar.bz2 \
	$(top_srcdir)/media/special/numbers_pcx.tar.gz \
	$(top_srcdir)/media/special/numbers_pcx.tar.xz \
	$(top_srcdir)/media/special/ciao.fig \
	$(top_srcdir)/media/special/matrix.mtx \
	$(top_srcdir)/var/fonts/Lat15-Terminus16.psf \
	depcomp missing install-sh
FIM_EXE=./src/fim$(EXEEXT)
FIMGS_EXE=PATH=./src/:$(PATH) $(abs_srcdir)/src/fimgs
TESTS_TMPFILE=file.tmp

# mhmhmhm 
# CLEANFILES = autom4te.cache
CLEANFILES = media/icon_smile.bmp src/testdir/test.log src/testdir/*.out

srcdir = src

# still unfinished
.PHONY: signed-tgzdist
signed-tgzdist: dist
	gpg -sbv -u 0xE0E669C8EF1258B8 $(distdir).tar.gz
	ls -l $(distdir).tar.gz  $(distdir).tar.gz.sig

# still unfinished
.PHONY: signed-dist
signed-dist: dist
	make dist-bzip2 && gpg -sbv -u 0xE0E669C8EF1258B8 $(distdir).tar.bz2
	ls -l $(distdir).tar.bz2 $(distdir).tar.bz2.sig
	gpg -sbv -u 0xE0E669C8EF1258B8 $(distdir).tar.gz
	ls -l $(distdir).tar.gz  $(distdir).tar.gz.sig

#%.sig: %
#	        gpg -sbv -u 0xE0E669C8EF1258B8 $@ $<
#	        gpg -sbav -u 0xE0E669C8EF1258B8 $@ $<


CXX11TEST=test ` $(FIM_EXE) -V | grep standard | head -1  | tr -d -c '[:digit:]'` -gt 200301
FIMNORCOPTS= --no-rc-file --no-etc-fimrc-file --no-history
FIMSCLTSTOPTS=-C '_downscale_huge_at_load=-1;_magnify_factor=2;_reduce_factor=2;' --no-auto-scale -o dumb -q 
FIMVIDEOTESTOPTS=-K ']]].[[fmfmwsk--++hhlltW' -K ':quit' --chars-press '' # --chars-press -K
NO_ASAN=ASAN_OPTIONS=detect_leaks=0

if ENABLE_CACA
FIM_USE_ASCII_ART_DEFAULT=ca
else
if ENABLE_AA
FIM_USE_ASCII_ART_DEFAULT=aa
endif
endif

# this is to prevent readline (e.g. 7.0 to 8.2) from emitting certain unexpected printouts
export INPUTRC = /dev/null

.PHONY: fbtests
fbtests: all
if WITH_EXPECT
	$(TIMEOUT_SMALL) \
		$(EXPECT) $(top_srcdir)/scripts/tests/spawn_and_quit.exp $(FIM_EXE) $(FIMNORCOPTS) -o fb $(top_srcdir)/media/
endif
	$(TIMEOUT_SMALL) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb $(FIMVIDEOTESTOPTS)
	FRAMEBUFFER="/dev/non-existing-framebuffer-device" \
		$(TIMEOUT_SMALL) \
			$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb $(FIMVIDEOTESTOPTS); test $$? = 1 # invalid FRAMEBUFFER device is detected as error
	FRAMEBUFFER="/dev/non-existing-framebuffer-device" \
		$(TIMEOUT_SMALL) \
			$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb $(FIMVIDEOTESTOPTS) |& grep open.*No.such # invalid FRAMEBUFFER is verbose
	$(TIMEOUT_SMALL) \
			$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb $(FIMVIDEOTESTOPTS) --device "/dev/non-existing-framebuffer-device"; test $$? = 1 # invalid --device option detected as error
	$(TIMEOUT_SMALL) \
			$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb $(FIMVIDEOTESTOPTS) --device "/dev/non-existing-framebuffer-device" |& grep open.*No.such # invalid FRAMEBUFFER is verbose
	if echo "$(TERM)" | grep 'screen\|tmux' ; then \
		$(TIMEOUT_SMALL) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb $(FIMVIDEOTESTOPTS) |& grep    Inap; \
	else \
		$(TIMEOUT_SMALL) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb $(FIMVIDEOTESTOPTS); # here no printout but "|& grep -v Inappropriate" would actually break it \
	fi
	if test "$(TERM)" != screen.linux && which screen; then \
		test `screen -q $(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb=S $(FIMVIDEOTESTOPTS) -c 'stdout "fail"' | wc -l` = 0; \
	fi # screen does not propagate commands's return code, therefore the stdout-based trick
	if test "$(TERM)" = screen.linux ; then \
		$(TIMEOUT_SMALL) \
			$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o fb=S $(FIMVIDEOTESTOPTS); test $$? = 255; \
	fi
if FIM_WANT_HARDCODED_CONSOLEFONT
	test `$(FIM_EXE) $(FIMNORCOPTS) -o fb $(top_srcdir)/media/ --verbose-font-load -c 'font "info"; quit' | wc -l` -ge 4 # font verbosity minimum
else
	test `$(FIM_EXE) $(FIMNORCOPTS) -o fb $(top_srcdir)/media/ --verbose-font-load -c 'font "info"; quit' | wc -l` -ge 3 # font verbosity minimum
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit !(_fbfont_as_screen_fraction=~"[0-9]")' # must be a number (0 if off); more tests needed
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit !(_fbfont_magnify_factor=~"[0-9]")' # must be a number (0 if off); more tests needed

.PHONY: aatests
aatests: all
if WITH_EXPECT
	$(NO_ASAN) $(TIMEOUT_SMALL) \
		$(EXPECT) $(top_srcdir)/scripts/tests/spawn_and_quit.exp $(FIM_EXE) $(FIMNORCOPTS) -o aa $(top_srcdir)/media/
endif
	$(NO_ASAN) $(TIMEOUT_LARGE) $(FIM_EXE) $(FIMNORCOPTS) -o aa --sanity-check | grep 's 98 b 1 1 1 c 99' # differently than -o dumb, this stresses DebugConsole's grow() mechanism
	$(NO_ASAN) $(FIM_EXE) $(top_srcdir)/media/ -t -c 'console_scroll_up;console_scroll_down;console_scroll_reset;quit' # placeholder for a better test
	$(NO_ASAN) $(TIMEOUT_SMALL) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o aa   $(FIMVIDEOTESTOPTS)
	if test -n "$(DISPLAY)"; then \
		$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o aa=w $(FIMVIDEOTESTOPTS); \
	fi
if WANT_READLINE
	if test -n "$(DISPLAY)"; then $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o aa=w -C '_debug_commands="k"' -K:quit -K '' | grep pressed.key; fi # readline mode key verbosity
endif
	@if test -n "$(DISPLAY)"; then make autodevtests; fi
# extras for readline and caption related:
if WANT_READLINE
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o aa $(top_srcdir)/media/ -K :'	'ne'	''	' -K '' -Kq # completion_display_matches_hook
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o aa $(top_srcdir)/media/ -K :'	'n=x'	''	' -K '' -Kq # cover completion of variables
	echo q | $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o aa $(top_srcdir)/media/ -K : # cover fim_rl_getc
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o aa $(top_srcdir)/media/ -C '_caption_over_image=3;_caption_over_image_fmt="fim:%M";_display_status=1;display' -kn -kq # cover Viewport::extra_top_height 
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o aa $(top_srcdir)/media/ -K :'	' -Kq | grep WELCOME 2>&1 | cat > /dev/null # cover what happens hitting Tab in console mode

.PHONY: cacatests
cacatests: all
if WITH_EXPECT
	$(NO_ASAN) $(TIMEOUT_SMALL) \
		$(EXPECT) $(top_srcdir)/scripts/tests/spawn_and_quit.exp $(FIM_EXE) $(FIMNORCOPTS) -o ca $(top_srcdir)/media/
endif
	$(NO_ASAN) $(TIMEOUT_SMALL) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o caca  $(FIMVIDEOTESTOPTS) # added caca falls back to ca
	if test -n "$(DISPLAY)"; then \
		$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o ca=w $(FIMVIDEOTESTOPTS); \
	fi
if WANT_READLINE
	if test -n "$(DISPLAY)"; then $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o ca=w -C '_debug_commands="k"' -K:quit -K '' | grep pressed.key; fi # readline mode key verbosity
endif
	@if test -n "$(DISPLAY)"; then make autodevtests; fi
if ENABLE_GIF
	test `SSH_TTY=1 $(NO_ASAN) $(FIM_EXE) -q $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif -o ca=h -c 'if(i:swidth<i:sheight){stderr i:swidth!=_screen_width;}else{stderr i:sheight!=_screen_height;}' -kq 2>&1 > /dev/null` = 0 # at once checking the width thing and the ca=h thing
endif
	SSH_TTY=1 $(NO_ASAN) $(FIM_EXE) -q $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif -o ca=d:10 -kq 2>&1 > /dev/null # 10 exceeds version 1's dithering options
	SSH_TTY=1 $(NO_ASAN) $(FIM_EXE) -q $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif -o ca=d:zz -kq 2>&1 > /dev/null # zz not in version 1's dithering options

# called from aatests and cacatests
.PHONY: autodevtests
autodevtests: all
	test "`SSH_TTY=1       $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/                     -c 'stderr _device_string' -kq   2>&1 > /dev/null`" = $(FIM_USE_ASCII_ART_DEFAULT) # may want to test for TERMUX_VERSION, too
	test "`(unset SSH_TTY; $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -t --no-framebuffer -c 'stderr _device_string' -kq ) 2>&1 > /dev/null`" = $(FIM_USE_ASCII_ART_DEFAULT) #
	test "`(      SSH_TTY= $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -t --no-framebuffer -c 'stderr _device_string' -kq ) 2>&1 > /dev/null`" = $(FIM_USE_ASCII_ART_DEFAULT) #

.PHONY: anyxtests
anyxtests: all
if ENABLE_JPEG
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o '=17x17' $(top_srcdir)/media/ -c 'quit !(_device_string=~"^(sdl|gtk)");' # auto selection of device
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) --output-device='=17x17' $(top_srcdir)/media/ -c 'quit !(_device_string=~"^(sdl|gtk)");' # auto selection of device
endif

.PHONY: gtktests
gtktests: all
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -K q
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd" "?";                        quit (_last_cmd_output=="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "menu";                    quit (_last_cmd_output=="")' # menu spec without '/' is not allowed
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "menu/";                   quit (_last_cmd_output!="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "menu/label";              quit (_last_cmd_output!="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd" "_menu/label";              quit (_last_cmd_output!="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd" "?menu/label";              quit (_last_cmd_output!="")' # weird but perhaps ok to keep
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd" "?menu/label?";             quit (_last_cmd_output=="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "menu/label  cmd  ?";      quit (_last_cmd_output!="")' # ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "menu/  label  cmd  ?";      quit (_last_cmd_output!="")' # ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "menu/||label||cmd||?";      quit (_last_cmd_output!="")' # ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "menu/label  cmd arg  ?";  quit (_last_cmd_output!="")' # ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menudel" "?";                        quit (_last_cmd_output=="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menudel"  "menu/label";              quit (_last_cmd_output=="")' # not ready yet
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menudel"  "menu/*";                  quit (_last_cmd_output=="")' # not ready yet
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menudel"  "*";                       quit (_last_cmd_output=="")' # not ready yet
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menudel";                            quit (_last_cmd_output!="")' # ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/v  toggle___v__1__2  "; quit (_last_cmd_output!="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/v  toggle___v__1__1  "; quit (_last_cmd_output=="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/v  toggle//_v//1//2  "; quit (_last_cmd_output!="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/||v||toggle___v__1__2||"; quit (_last_cmd_output!="")' # ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/||v||toggle||_v||1||2||"; quit (_last_cmd_output=="")' # not ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/  v||toggle||_v||1||2||"; quit (_last_cmd_output=="")' # not ok
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/v  toggle  _v  1  2  "; quit (_last_cmd_output=="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/v  toggle		_v		1		2  "; quit (_last_cmd_output!="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/choice: 1a  v=1  a  choice: 2b  v=2  b  choice: 3c  i:_scale_style=3  c"; quit (_last_cmd_output!="")' # i:var ok now
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/choice: 1a  v=1  a  choice: 2b  v=2  b  choice: 3c  _scale_style=3  c"; quit (_last_cmd_output!="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "menuadd"  "m/choice: 1a  v=1  a  choice: 1b  v=1  b  choice: 3c  _scale_style=3  c"; quit (_last_cmd_output=="")'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "v";display "menuadd"  "m/choice: 1a  v=1  a  choice: 1b  v=1  b  choice: 3c  _scale_style=3  c"; quit (_last_cmd_output=="")' 2>&1 | grep -q 'ERROR..repeated.*menu.specification'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "v";display "menuadd"  "menu"; quit (_last_cmd_output=="")' 2>&1 | grep -q 'ERROR../.*specification'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "v";display "menuadd"  "menu/bad  var+=0"; quit (_last_cmd_output=="")' 2>&1 | grep -q 'ERROR..bad.command.*menu.specification'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "menuadd" "(menu)/label  a==3/4"; quit (_last_cmd_output!="")' # slash ok in menu spec, parens ok in menu name
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "V";display "reinit" "f"; quit' # menu rebuild
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/numbers_pcx/*.pcx -o gtk -C 'display "v";' -c 'stdout _last_cmd_output;quit' --load-image-descriptions-file $(top_srcdir)/media/special/images.desc | grep menu: | wc -l` = 4
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'display "menuadd" "_Help/_Variables  FimMenuVariablesHelp/" "Help_/_Variables  FimMenuVariablesHelp/";quit 0;' # notice Help_ is a different menu than _Help
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'cl1=_console_lines;man_fimrc;cl2=_console_lines; quit!(cl1<cl2);'
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c 'cl1=_console_lines;man_fim  ;cl2=_console_lines; quit!(cl1<cl2);'
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -K ':q' -K '' | wc -c ` -lt 2 # with pre readline-8 I observe a 0x0a spilled
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c quit --verbose-interpreter --no-internal-config | grep verbose.menu.tooltip | wc -l ` -eq 1
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c quit --verbose-interpreter                      | grep verbose.menu.tooltip | wc -l ` -eq 1
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk=e       --verbose-interpreter -c quit              | grep menuadd              | wc -l ` -eq 0 # test starting with no menu
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk         --verbose-interpreter -c quit              | grep menuadd              | wc -l ` -eq 1 # test starting with one menu
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c quit                                            | grep verbose.menu.tooltip | wc -l ` -eq 0
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o gtk -c quit --verbose-interpreter --verbose-interpreter| grep setting.verbose.menus.build | wc -l ` -eq 1

.PHONY: sdltests
sdltests: all
	$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o sdl=h  --version | grep 'SDL[2]\?'
	$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o sdl=h  $(FIMVIDEOTESTOPTS);
	$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o sdl=hhMR20:10% -K '	' -c 'while(0){1;}quit;' ; # cover: draw help grid, unlock, input poll, misc
	$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o sdl=0:-1 ; test $$? = 252 # don't be scared of error messages -- we're testing error handling here
if FIM_HAVE_ANY_CONSOLEFONT
	$(NO_ASAN) FBFONT=''       \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o sdl -c quit # TODO: this is only a coverage test
endif
if FIM_WANT_HARDCODED_CONSOLEFONT
	$(NO_ASAN) FBFONT=fim:// \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o sdl -c quit # TODO: this is only a coverage test
endif
	test `$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o sdl -K ':q' -K '' | wc -c ` -lt 1 # no char is spilled, differently than with gtk

.PHONY: imlib2tests
imlib2tests: all
	$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o imlib2  $(FIMVIDEOTESTOPTS);
	$(NO_ASAN) $(TIMEOUT_LARGE) \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o imlib2=20:10% -K '	' -c 'while(0){1;}quit;' ; # cover: draw help grid, unlock, input poll, misc
#	$(NO_ASAN) $(TIMEOUT_LARGE) \
#		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o imlib2=0:-1 ; test $$? = 252 # don't be scared of error messages -- we're testing error handling here
if FIM_HAVE_ANY_CONSOLEFONT
	$(NO_ASAN) FBFONT=''       \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o imlib2 -c quit # TODO: this is only a coverage test
endif
if FIM_WANT_HARDCODED_CONSOLEFONT
	$(NO_ASAN) FBFONT=fim:// \
		$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o imlib2 -c quit # TODO: this is only a coverage test
endif

# this rule is deprecated, too: testing should be performed with 'make tests' (FIXME)
.PHONY: sanity-check
sanity-check:	all
if HAVE_RUNNABLE_TESTS
	@$(FIM_EXE) $(FIMNORCOPTS) -E $(top_srcdir)/scripts/tests/sanity.fim -t
else
	@echo "you are cross-compiling, so can't run tests..."
endif

.PHONY: check
check:	tests

.PHONY: dist-tests
dist-tests:	dist
	test -n $(distdir)
	! test -d $(distdir)
	test -f $(DIST_ARCHIVES)
	tar xaf $(DIST_ARCHIVES)
	test -d $(distdir)
	cd $(distdir) && scripts/maintenance/configure-minimal.sh --prefix=`pwd`/local --enable-readline CXXFLAGS=-O0\ -pipe\ -pedantic && make tests && make install
	cd $(distdir) && make clean
	cd $(distdir) && ./configure --prefix=`pwd`/local CXXFLAGS=-O0\ -pipe\ -pedantic && make tests && make install
	cd $(distdir) && make clean
	cd $(distdir) && LSAN_OPTIONS=verbosity=1:log_threads=1 ASAN_OPTS=detect_leaks=0 \
			 ./configure --prefix=`pwd`/local CXX=g++ CXXFLAGS=-O0\ -ggdb\ -pipe\ -pedantic\ -fsanitize=address\ -fno-omit-frame-pointer LDFLAGS=-lasan && make tests
	@echo "[*] Passed dist-tests -- good. Before re-running it, make dist-tests-clean"

.PHONY: dist-tests-clean
dist-tests-clean:
	test -n $(distdir)
	rm -fR $(distdir)

SNTZ = |tr -d -c '0-9A-Za-z_./:-'
.PHONY: devtests
devtests:	all
if WITH_SHELLCHECK
	$(SHELLCHECK) -e SC2235,SC1117,SC2230,SC2086,SC2006,SC2001 $(top_srcdir)/src/fimgs
endif
	! grep '^#' NEWS
	grep GNU.Linux var/index.html.in # wording check
	( ! test  -r $(INPUTRC) ) || test `wc -c < $(INPUTRC)` = 0 # must either not exist or deliver zero bytes
	! grep -n '^[^ ].\{107,\}' doc/FIM.TXT
	! grep -n '^[^ ].\{83,\}'  README
	! grep -n '^[^ ].\{175,\}' README.md test `grep '[^=]==[^=]' $(top_srcdir)/configure.ac | grep -v ^dnl | wc -l` = 0 # look out for bashisms
	if grep -q '^\s*PKG_CHECK_MODULES' configure.ac; then ! grep -q '^\s*PKG_CHECK_MODULES' configure; fi
	test `grep '[^=]==[^=]' $(top_srcdir)/src/fimgs | wc -l` = 0 # look out for more bashisms
	test `grep 'test.*[^=]==[^=]' $(top_srcdir)/configure.ac | wc -l` = 0 # look out for more bashisms
	test `grep '+=' $(top_srcdir)/configure.ac | wc -l` = 0 # look out for more bashisms
	test `grep 'test.* == ' $(top_srcdir)/Makefile.am | wc -l` = 1 # tolerate == on this line only
	test `grep 'SNTZ.*wc' $(top_srcdir)/Makefile.am | wc -l` = 1 # tolerate SNTZ.*wc on this line only
	! grep -n '^[^ ].\{83,\}' README
	! grep '\<'$(USER)'\>' doc/*man* # against namespace leaks
	! grep '\.o\>' $(top_srcdir)/src/Makefile.am
	! grep '\<OBJECT\>' $(top_srcdir)/src/Makefile.am
	for p in `grep '^\.PHONY:.*' Makefile.am | sed 's/.PHONY..//g'`; do grep "^$$p:" Makefile.am -q || exit 1 ; done # ensure each .PHONY target exists
if WITH_PERL
	test -s doc/FIM.html
if WITH_MAN2HTML
	test -s doc/fimgs.man.html -a -s doc/fim.man.html -a -s doc/fimrc.man.html
endif
endif
if HAVE_RUNNABLE_TESTS
if ENABLE_GIF
	$(FIMGS_EXE) ssh:$$(hostname):$(abs_top_srcdir)/media/numbers/0.gif -- $(FIMNORCOPTS) -o dumb -K q
	$(FIMGS_EXE) ssh:$$(hostname):$(abs_top_srcdir)/media/numbers/u.gif -- $(FIMNORCOPTS) -o dumb -K q; test $$? = 255
	export TMPDIR=`pwd`/tmpdir && $(FIMGS_EXE) ssh:$$(hostname):$(abs_top_srcdir)/media/numbers/0.gif -- $(FIMNORCOPTS) -o dumb -K q && test "`ls $$TMPDIR`" = ""
endif
if HAVE_VALGRIND
	if ! ldd $(FIM_EXE) | grep libasan.so.5; then \
		make clean -C src/testdir ; \
		VALGRIND='valgrind --tool=memcheck --leak-check=yes --num-callers=15 --exit-on-first-error=yes --error-exitcode=255' \
			make ltests; \
	fi
endif
endif

.PHONY: multipagetests_ps
multipagetests_ps: all
if ENABLE_PS
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/multipage/sample.ps -c 'list "sort";a=page; goto;b=page; goto "2p";c=page; goto "1p";d=page; goto _lastpageindex;e=page; stdout a.".".b.".".c.".".d.".".e;quit'   | grep 0.0.1.0.0;
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/multipage/sample.ps -c 'list "sort";a=page; goto;b=page; goto "+1p+1p";c=page; goto "+1p+1p";d=page; goto "+1p+1p";e=page; stdout a.".".b.".".c.".".d.".".e;quit' | grep 0.0.1.2.0;
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/multipage/sample.ps -c 'list "sort";a=page; goto;b=page; goto "+1";c=page; goto "+1+1";d=page; goto "+1+1";e=page; stdout a.".".b.".".c.".".d.".".e;quit' | grep 0.0.1.2.0;
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/multipage/sample.ps -c 'list "sort";a=page; goto;b=page; goto "+1f+1f";c=page; goto "+1f+1f";d=page; goto "+1f+1f";e=page; stdout a.".".b.".".c.".".d.".".e;quit' | grep 0.0.0.0.0;
endif

..PHONY: multipagetests_pdf
multipagetests_pdf: all
if ENABLE_POPPLER
if ENABLE_PDF
if ENABLE_PCX
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/multipage/sample.pdf -c 'goto "+1p" "+1f"; stdout _fileindex.".".page;quit' | grep -F 1.1 # page jump has precedence over file jump (good)
endif
endif
endif

.PHONY: multipagetests
multipagetests:	all
	if test -f $(top_srcdir)/media/multipage/sample.ps ; then \
		make multipagetests_ps; fi
	if test -f $(top_srcdir)/media/multipage/sample.pdf ; then \
		make multipagetests_pdf; fi

# this is the official way of testing fim
.PHONY: tests
tests:	all
	! ( echo $(abs_top_srcdir) | grep '[\\ :"'"'"']' ) # reject unreasonable base directory names
	! grep TODO doc/fimrc.man
if HAVE_RUNNABLE_TESTS
	ulimit -c unlimited || true
	ulimit -v 32000 || true
	ulimit -t 1000 || true
	$(TIMEOUT_LARGE) $(FIM_EXE) $(FIMNORCOPTS) -o dumb --sanity-check
	$(TIMEOUT_LARGE) $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'stdout random ; quit 0;' # make sure no leak from 'random'
	$(TIMEOUT_LARGE) $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'quit !(randomm == 0 && ( random != 0 || random != 0 || random != 0));' # hope to get three non-zeros
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c '_display_status_fmt="fim:%M%n%R%m%C%v"' --verbose-interpreter --verbose-interpreter -kn -kq # cover modifiers and interpreter
	wc0=`        $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -kq                              |wc -l`; \
		wc1=`$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -kq                --verbose-load|wc -l`; \
		wc2=`$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -kq --verbose-load --verbose-load|wc -l`; \
		test $$wc0 -lt $$wc1 -a $$wc1 -lt $$wc2 # ensure at least two levels of load verbosity
#
if ENABLE_PCX
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/numbers_pcx/1.pcx -c 'a=_filelistlen;limit "~="; list;b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 2.1
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/../media/numbers_pcx/1.pcx -c 'a=_filelistlen;limit "~="; list;b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 2.2
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/numbers_pcx/1.pcx -c 'a=_filelistlen;limit "~^"; list;b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 2.1
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/../media/numbers_pcx/1.pcx -c 'a=_filelistlen;limit "~^"; list;b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 2.1
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/../media/numbers_pcx/1.pcx -c 'a=_filelistlen;limit "~$$"; list;b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 2.1
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/../media/numbers_pcx/1.pcx -c 'a=_filelistlen;limit "~!"; list;b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 2.0
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/../media/numbers_pcx/2.pcx -c 'a=_filelistlen;limit "~!"; list;b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 2.2
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_filelistlen;limit "~i" "4"; b=_filelistlen; limit "~i" "4-4"; c=_filelistlen; limit "~i" "4-5"; d=_filelistlen; stdout a.".".b.".".c.".".d;quit' $(SNTZ)` = 10.1.1.2
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_filelistlen;limit "~z" "0-900"; b=_filelistlen; limit "~z" "0-909"; c=_filelistlen; stdout a.".".b.".".c;quit'` = 10.0.7 # needs stat
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_filelistlen;limit "!"; b=_filelistlen; stdout a.".".b;quit' $(SNTZ)` = 10.0 # needs stat
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_filelistlen;limit "~d" "01/01/1970-01/01/2007"; b=_filelistlen; limit "~d" "01/01/2007-19/01/2038"; c=_filelistlen; stdout a.".".b.".".c;quit' $(SNTZ)` = 10.0.10
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_filelistlen;limit "prime" "yes"; b=_filelistlen; limit "prime" "no" ; c=_filelistlen; limit "prime" "zzz" ; d=_filelistlen; stdout a.".".b.".".c.".".d;quit' --load-image-descriptions-file $(top_srcdir)/media/special/images.desc $(SNTZ)` = 10.3.2.0
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_filelistlen;limit "uno"; b=_filelistlen;limit "due"; c=_filelistlen; limit "boh"; d=_filelistlen; stdout a.".".b.".".c.".".d;quit' --load-image-descriptions-file $(top_srcdir)/media/special/images.desc $(SNTZ)` = 10.1.1.0
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'limit "-list";stdout _last_cmd_output;limit "-listall";stdout _last_cmd_output; quit;' --load-image-descriptions-file $(top_srcdir)/media/special/images.desc | grep there.are.*variable > /dev/null # TODO: need better test here
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'limit "-list" "prime";stdout _last_cmd_output;limit "-listall" "prime";stdout _last_cmd_output; quit;' --load-image-descriptions-file $(top_srcdir)/media/special/images.desc | grep there.are.*value > /dev/null # TODO: need better test here
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_8bit.pcx.bin -C 'stdout "Unsupported PCX file test.."' -c 'quit !(i:width==0)' # named so to not break dir-loading tests
endif
if ENABLE_PCX
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/0.pcx; quit !(i:_comment=="comments"&&i:T==""&&common=="s")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/1.pcx; quit !(i:_comment=="pre_comments")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/2.pcx; quit !(i:_comment=="comment")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/3.pcx; quit !(i:_comment=="comment_post")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/4.pcx; quit !(i:_comment=="komment")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/5.pcx; quit !(i:_comment=="kommentar")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/6.pcx; quit !(i:_comment=="comment_POST")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/7.pcx; quit !(i:_comment=="PRE_comment_POST")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/7.pcx; quit !(i:common=="s")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/8.pcx; quit !(i:_comment=="comment_POST")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/8.pcx; quit !(i:common=="s")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/9.pcx; quit !(i:common=="")'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file media/special/modifiers.desc -c '/9.pcx; quit !(i:_comment=="comment@@@common@5_")'
endif
if ENABLE_PCX
if ENABLE_PNG
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "-/5/";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.8.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "-/B";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.12.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+/b";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.1.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+/B";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.2.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+/s";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.2.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+/d";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.3.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+/u";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.1.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+1f+1f";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.2.12 # goto: only last subspec effective
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+1f+2f";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.3.12 # goto: only last subspec effective
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+2f+1f";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.2.12 # goto: only last subspec effective
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+2p+1p";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit' | grep 1.1.1.12 # goto: page jump idempotent on one-pager
	@if $(MAKE) -C $(top_srcdir)/media/multipage ; then make multipagetests; fi # the if is not really necessary
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "2f";c=_lastfileindex; goto c;d=_fileindex; stdout a.".".b.".".c.".".d."."._filelistlen;quit' | grep 1.1.1.1.12
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png $(top_srcdir)/media/fim.png $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; next;b=_fileindex; goto "2f";c=_lastfileindex; goto c;d=_fileindex; stdout a.".".b.".".c.".".d."."._filelistlen;quit' | grep 1.2.1.1.12
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort";a=_fileindex; goto;b=_fileindex; goto "+50%";c=_fileindex; stdout a.".".b.".".c."."._filelistlen;quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c 'list "sort"; a=_fileindex; goto "+prime"; b=_fileindex;goto "+prime"; c=_fileindex; goto "+prime"; d=_fileindex; goto "+prime"; e=_fileindex; stdout a.".".b.".".c.".".d.".".e; quit' | grep 1.2.5.6.1
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -C 'bind "n" "goto \"+1f\"";' -Knnnq -F 'quit !(_fileindex==4);'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -C 'bind "n" "goto \"+1F\"";' -Knnnq -F 'quit !(_fileindex==8);'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c 'list "sort_fsize"; goto 0; reload; stdout i:_filename; quit' | grep 1.pcx
endif
if ENABLE_PCX
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/      -c 'quit (i:_filename=~ "//")'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort"; list;l1=_last_cmd_output;list "random_shuffle";list;l2=_last_cmd_output;if(l1==l2)quit 255; quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'list "sort"; next;f=i:_filename;list "swap";     reload;if(f==i:_filename)quit 255; quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx $(top_srcdir)/media/numbers_pcx/*.pcx --sort -c 'list "pop"; if(_filelistlen!= 9) quit 255;quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx $(top_srcdir)/media/numbers_pcx/*.pcx        -c 'list "pop"; if(_filelistlen!=19) quit 255;quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/numbers_pcx/2.pcx --reverse -c 'quit!(i:_filename=~"2.pcx")'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/1.pcx $(top_srcdir)/media/numbers_pcx/2.pcx           -c 'quit (i:_filename=~"2.pcx")'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx                                              -c 'list "remove"; quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'next;list "pop";reload;quit    !(i:_filename == "$(top_srcdir)/media/numbers_pcx/0.pcx")'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'next;list "remove";reload;quit !(i:_filename == "$(top_srcdir)/media/numbers_pcx/2.pcx")'
	test `$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -k n -k Del -k q -F 'stdout i:_filename' $(SNTZ)` = "$(top_srcdir)/media/numbers_pcx/2.pcx"
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'a=1' -c 'a=a+1;' -c 'quit (1-(a==2))'
endif
if WANT_RECURSIVE_DIRS
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'cd "$(top_srcdir)/media/"' -c 'stdout _pwd;l=_filelistlen;list "pushdir"  _pwd;stdout l;if(l==_filelistlen)quit 255; quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'cd "$(top_srcdir)/media/"' -c 'stdout _pwd;l=_filelistlen;list "pushdirr" _pwd;stdout l;if(l==_filelistlen)quit 255; quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'cd "$(top_srcdir)/media/"' -c 'stdout _pwd;l=_filelistlen;list "pushdir"      ;stdout l;if(l==_filelistlen)quit 255; quit'
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'cd "$(top_srcdir)/media/"' -c 'stdout _pwd;l=_filelistlen;list "pushdirr"     ;stdout l;if(l==_filelistlen)quit 255; quit'
if ENABLE_PNG
if ENABLE_PCX
if ENABLE_JPEG
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir "$(top_srcdir)/media/image.jpg" -c 'quit 2' -kq; test $$? = 2 # --cd-and-readdir respects scripting
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb "$(top_srcdir)/media/" -c ' quit 2' -kq; test $$? = 2 # no heading semicolons: good
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb "$(top_srcdir)/media/" -c ';quit 2' -kq; test $$? = 0 #    heading semicolons: bad (FIXME in the long run)
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir "$(top_srcdir)/media/image.jpg" -c 'quit 2' -kq; test $$? = 2 # --cd-and-readdir respects scripting
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir $(abs_top_srcdir)/media/image.jpg -c 'if(_pwd=="$(abs_builddir)")quit 1; quit' # check that cd is effective
	test "`$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir $(abs_top_srcdir)/media/image.jpg -c 'stdout _pwd;quit'`" = `cd $(abs_top_srcdir)/media; pwd;` # check that cd is effective, again (yes, redundant)
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir $(abs_top_srcdir)/media/image.jpg -c 'if (_filelistlen<=1)quit 1;quit' # check that dir files are pushed
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir $(abs_top_srcdir)/media/image.jpg -c 'if (_filelistlen>20)quit 1;quit' # too many files means recursive push
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir $(abs_top_srcdir)/media/image.jpg -c 'l0=_filelistlen; list "pushdir"  "."; l1=_filelistlen; if(l0!=l1)quit 1; quit' # pushdir must be idempotent
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir $(abs_top_srcdir)/media/image.jpg -c 'l0=_filelistlen; list "pushdirr" "."; l1=_filelistlen; if(l0==l1)quit 1; quit' # pushdir and pushdirr must differ
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --cd-and-readdir $(abs_top_srcdir)/media/image.jpg -c 'if(i:_filename=~"image.jpg")quit 0; quit 1' # --cd-and-readdir does jumps to loaded file
endif
endif
endif
endif
if ENABLE_RAW_BITS_RENDERING
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit !(_all_file_loaders=~"Text")' # shall reflect supported file formats
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ --as-text -c quit
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ --as-text   -c 'stdout _last_file_loader;quit' $(SNTZ)` = Text  # mostly coverage
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ --binary=1  -c 'stdout _last_file_loader;quit' $(SNTZ)` = Bit1  # mostly coverage
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ --binary=24 -c 'stdout _last_file_loader;quit' $(SNTZ)` = Bit24 # mostly coverage
if WANT_READ_STDIN_IMAGE
	test `echo -n U | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --image-from-stdin --binary=1  -c 'magnify;stdout i:height."."._last_file_loader;quit' $(SNTZ)` = 1.Bit1  # coverage of tiny case; TODO: remove necessity of magnify
	test `echo -n U | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --image-from-stdin --binary=24 -c 'magnify;stdout i:height."."._last_file_loader;quit' $(SNTZ)` = 1.Bit24 # coverage of tiny case; TODO: remove necessity of magnify
	test `echo $(abs_srcdir)/media | $(FIM_EXE) $(FIMNORCOPTS) -o dumb - --binary=23 -c 'magnify;stdout _last_file_loader;quit' $(SNTZ)` = Bit24 # coverage of - and fallback to 24
endif
endif
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o dumb -c 'window;stdout _last_cmd_output;quit;' grep disabled > /dev/null
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ --dump-reference-help | wc -l $(SNTZ)` -gt 100
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=m | wc -c $(SNTZ)` -gt 28000 # man dump
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=m | grep fB | wc -l $(SNTZ)` -gt 40 # lots of troff
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=l | grep fB | wc -l $(SNTZ)` -eq 0  # no troff left (hopefully)
	test `$(FIM_EXE) $(FIMNORCOPTS) --NON-existent-OPTIONNN | wc -l $(SNTZ)` -gt 70 # help function
	$(FIM_EXE) $(FIMNORCOPTS)       --NON-existent-OPTIONNN; test $$? = 240 # help function failing
if ENABLE_BMP
	if which convert; then \
		convert $(top_srcdir)/media/icon_smile.gif -monochrome $(top_srcdir)/media/icon_smile.bmp && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.bmp -c 'stdout _last_file_loader;quit' $(SNTZ)` = bmp && rm -f $(top_srcdir)/media/icon_smile.bmp && \
		if which ppmtobmp ; then \
		convert $(top_srcdir)/media/icon_smile.gif ppm:- | ppmtobmp -bpp 8 > $(top_srcdir)/media/icon_smile.bmp && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.bmp -c 'stdout _last_file_loader;quit' $(SNTZ)` = bmp && rm -f $(top_srcdir)/media/icon_smile.bmp && \
		convert $(top_srcdir)/media/icon_smile.gif ppm:- | ppmtobmp -bpp 4 > $(top_srcdir)/media/icon_smile.bmp && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.bmp -c 'stdout _last_file_loader;quit' $(SNTZ)` = bmp && rm -f $(top_srcdir)/media/icon_smile.bmp ; \
		fi && \
		convert $(top_srcdir)/media/icon_smile.gif -depth 8 -alpha Disassociate $(top_srcdir)/media/icon_smile.bmp && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.bmp -c 'stdout _last_file_loader;quit' $(SNTZ)` = bmp && rm -f $(top_srcdir)/media/icon_smile.bmp; \
	fi
endif
	if which convert; then \
		convert $(top_srcdir)/media/icon_smile.gif -depth 8  $(top_srcdir)/media/icon_smile.ppm && \
		sed -i 's/P6$$/P6\n# comment/g'                      $(top_srcdir)/media/icon_smile.ppm && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.ppm -c 'stdout _last_file_loader;quit' $(SNTZ)` = ppm && rm -f $(top_srcdir)/media/icon_smile.ppm && \
		convert $(top_srcdir)/media/icon_smile.gif -depth 16 $(top_srcdir)/media/icon_smile.ppm && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.ppm -c 'stdout _last_file_loader;quit' $(SNTZ)` = ppm && rm -f $(top_srcdir)/media/icon_smile.ppm && \
		convert $(top_srcdir)/media/icon_smile.gif -depth 8  $(top_srcdir)/media/icon_smile.pgm && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm -c 'stdout _last_file_loader;quit' $(SNTZ)` = pgm && rm -f $(top_srcdir)/media/icon_smile.pgm; \
		convert $(top_srcdir)/media/icon_smile.gif -depth 16 $(top_srcdir)/media/icon_smile.pgm && \
		test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm -c 'stdout _last_file_loader;quit' $(SNTZ)` = pgm && rm -f $(top_srcdir)/media/icon_smile.pgm; \
	fi # cover-test FbiStuffPpm.cpp
if WITH_XFIG
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/ciao.fig -c 'stdout _last_file_loader;quit' $(SNTZ)` = ppm # 
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/ciao.fig -c 'stdout i:_external_decoder_program;quit' $(SNTZ)` = fig2dev # 
else
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/ciao.fig -c 'stdout _last_file_loader;quit' $(SNTZ)` = 0 # 
endif
if WITH_XCF2PNM
	if which xcf2pnm ; then test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0.xcf -c 'stdout i:_buffered_in_tmpfile;quit'` != '0'; fi
endif
if WITH_XCFTOPNM
	if which xcftopnm ; then test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0.xcf -c 'stdout i:_buffered_in_tmpfile;quit'` != '0'; fi
endif
if ENABLE_QOI
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb      $(top_srcdir)/media/special/0.qoi  -c 'stdout _last_file_loader;quit' $(SNTZ)` = libqoi
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb -i < $(top_srcdir)/media/special/0.qoi  -c 'stdout _last_file_loader;quit' $(SNTZ)` = libqoi
endif
if ENABLE_AVIF
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb $(top_srcdir)/media/special/0.avif -c 'stdout _last_file_loader;quit' $(SNTZ)` = libavif
else
if WITH_LIBGRAPHICSMAGICK
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb $(top_srcdir)/media/special/0.avif -c 'stdout _last_file_loader;quit' $(SNTZ)` = libGraphicsMagick
else
if WITH_CONVERT
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb $(top_srcdir)/media/special/0.avif -c 'stdout _last_file_loader;quit' $(SNTZ)` = 0 # assuming no other decoders exists, -X stops external converters
	if which convert && convert identify -list format | grep -i avif; then test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0.avif -c 'stdout _last_file_loader;quit' $(SNTZ)` = ppm ; fi # note: this would pass also with old pipe mechanism -- tiny file
endif
endif
endif
if ENABLE_WEBP
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb $(top_srcdir)/media/special/0.webp -c 'stdout _last_file_loader;quit' $(SNTZ)` = libwebp
else
if WITH_LIBGRAPHICSMAGICK
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb $(top_srcdir)/media/special/0.webp -c 'stdout _last_file_loader;quit' $(SNTZ)` = libGraphicsMagick
else
if WITH_CONVERT
	test `$(FIM_EXE) $(FIMNORCOPTS) -X -o dumb $(top_srcdir)/media/special/0.webp -c 'stdout _last_file_loader;quit' $(SNTZ)` = 0 # assuming no other decoders exists, -X stops external converters
	if which convert && convert identify -list format | grep -i webp; then test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0.webp -c 'stdout _last_file_loader;quit' $(SNTZ)` = ppm ; fi # note: this would pass also with old pipe mechanism -- tiny file
endif
endif
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0.ppm                      -c 'quit !(i:width> 0)' # make sure ppm is read
if WITH_CONVERT
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0.ppm --pread-cmd='cat {}' -c 'quit !(i:width> 0)' # make sure pread works
else
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0.ppm --pread-cmd='cat {}' -c 'quit !(i:width> 0)' # make sure pread works
endif
if ENABLE_SYSTEM_CALLS
	  rm -f $(TESTS_TMPFILE); $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c '! "touch" "'$(TESTS_TMPFILE)'";quit;';   test -f $(TESTS_TMPFILE); rm -f $(TESTS_TMPFILE); # make sure works
else
	  rm -f $(TESTS_TMPFILE); $(FIM_EXE) $(FIMNORCOPTS) -o dumb -c '! "touch" "'$(TESTS_TMPFILE)'";quit;'; ! test -f $(TESTS_TMPFILE); # make sure does not work
endif
if WITH_CONVERT
if ENABLE_JPEG
	if which convert && convert identify -list format | grep -i ppm; then $(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_downscale_huge_at_load=-1' $(top_srcdir)/media/image.jpg --pread-cmd='convert {} -background black -flop -fill white -pointsize 3 label:"mirrored {}" -gravity Center -append ppm:-' -c 'quit !(i:width==320)' ; fi # pointsize need to be small or text will exceed 320 pixels
	if which convert && convert identify -list format | grep -i ppm; then $(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_downscale_huge_at_load=-1' $(top_srcdir)/media/image.jpg --pread-cmd='convert {} -rotate 90 -append ppm:-' -c 'quit !(i:width==240)' ; fi
	if which convert && convert identify -list format | grep -i ppm; then cp $(top_srcdir)/media/image.jpg -- -image.jpg && $(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_downscale_huge_at_load=-1' --pread-cmd='convert {} -rotate 90 -append ppm:-' -c 'quit !(i:width==320)' -- -image.jpg && rm -- -image.jpg ; fi
endif
endif
if ENABLE_PCX
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '=_pushdir_re=.(PNG)'  $(top_srcdir)/media/numbers_pcx/ -kq; test $$? = 248 # _pushdir_re assignment is effective
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '=_pushdir_re=.(PCX)'  $(top_srcdir)/media/numbers_pcx/ -kq # _pushdir_re assignment is effective
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_pushdir_re=".(PNG)"' $(top_srcdir)/media/numbers_pcx/ -kq # _pushdir_re assignment comes late and is not effective
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c '2,4 stdout  "{}";q' | wc -l` = 3
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c '3;list "mark";6;list "mark";                   1,2 stdout "marked {}";q' | grep 'marked.*[01].pcx' | wc -l` = 2
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c '3;list "mark";6;list "mark"; toggleLimitMarked;1,2 stdout "marked {}";q' | grep 'marked.*[25].pcx' | wc -l` = 2
	cd $(top_srcdir)/media/numbers_pcx/; $(abs_builddir)/$(FIM_EXE) $(FIMNORCOPTS) -o dumb                            --read-from-file ../special/images.list   -c 'if(_filelistlen!=2)quit 255; quit'
	$(TIMEOUT_SMALL) $(abs_builddir)/$(FIM_EXE) $(FIMNORCOPTS) -o dumb --read-from-file ../special/collated-NON-EXISTING.list ; test $$? = 248
if WANT_READ_STDIN_IMAGE
	cd $(top_srcdir)/media/numbers_pcx/; $(abs_builddir)/$(FIM_EXE) $(FIMNORCOPTS) -o dumb --read-from-stdin-elds '!' --read-from-file ../special/images.list   -c 'if(_filelistlen!=0)quit 255; quit'
	cd $(top_srcdir)/media/numbers_pcx/; $(abs_builddir)/$(FIM_EXE) $(FIMNORCOPTS) -o dumb --read-from-stdin-elds '!' --read-from-file ../special/collated.list -c 'if(_filelistlen!=2)quit 255; quit'
	cat $(top_srcdir)/media/numbers_pcx/1.pcx | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --image-from-stdin -c 'quit !(i:width>0 && i:height>0)'
	echo | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --image-from-stdin -kq
	echo | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --read-from-stdin ; test $$? = 248
	echo $(top_srcdir)/media/numbers_pcx/* | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --read-from-stdin --read-from-stdin-elds ' ' -c 'quit !(_filelistlen==9)'
	echo 'quit 33' | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --script-from-stdin              ; test $$? = 33
	echo 'quit 33' | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --script-from-stdin -Kq          ; test $$? = 33
	echo 'quit 33' | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --script-from-stdin -kq          ; test $$? = 33
	echo 'quit 33' | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --script-from-stdin -C 'quit 22' ; test $$? = 22
	echo 'quit 33' | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --script-from-stdin -c 'quit 22' ; test $$? = 22
	echo 'quit 33' | $(FIM_EXE) $(FIMNORCOPTS) -o dumb --script-from-stdin -F 'quit 22' ; test $$? = 22
endif
if ENABLE_ARCHIVE
	tar czf archive.tgz $(top_srcdir)/media/numbers_pcx/ && $(TIMEOUT_SMALL) $(abs_builddir)/$(FIM_EXE) $(FIMNORCOPTS) -o dumb archive.tgz -c 'stdout i:pages." pages";if(i:pages==1)quit 255;quit' && rm -f archive.tgz
	echo 'not an archive' > archive.tgz; $(TIMEOUT_SMALL) $(abs_builddir)/$(FIM_EXE) $(FIMNORCOPTS) -o dumb archive.tgz -c 'stdout i:pages." pages";if(i:pages!=0)quit 255;quit' && rm -f archive.tgz
endif
if WANT_READ_STDIN_IMAGE
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'pread "cat $(top_srcdir)/media/numbers_pcx/1.pcx"; reload' -c 'quit !(i:width>0 && i:height>0)'
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'cd "$(abs_srcdir)/media/numbers_pcx/1.pcx"; pwd;d1=_last_cmd_output;  cd "-"; pwd;d2=_last_cmd_output;  cd "$(abs_srcdir)/media/numbers_pcx/";pwd;d3=_last_cmd_output; if(d1!=d3||d1==d2)quit 1;quit' $(SNTZ) # check that "cd file" works
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_file_loader="ppm"' $(top_srcdir)/media/numbers_pcx/1.pcx -c 'stdout i:width; stdout (i:width!=0 && i:height!=0);quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_file_loader="pcx"' $(top_srcdir)/media/numbers_pcx/1.pcx -c 'stdout i:width; stdout (i:width==0 && i:height==0);quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --mark-from-image-descriptions-file $(top_srcdir)/media/special/images.desc -c 'l=_filelistlen;toggleLimitMarked;if(_filelistlen!=l/2)quit 255; quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --image-descriptions-file-separator '	' $(top_srcdir)/media/special/loadable.desc $(top_srcdir)/media/numbers_pcx/ -c '/2.pcx;if(i:_comment=~"due")quit;quit 255'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb --image-descriptions-file-separator '?'    $(top_srcdir)/media/special/loadable.desc $(top_srcdir)/media/numbers_pcx/ -c '/2.pcx;if(i:_comment=~"due")quit 255;quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C 'cd "$(top_srcdir)"' $(top_srcdir)/media/special/dir.desc -c 'quit !(_filelistlen==2)'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --image-descriptions-file-separator '	' --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c '/2.pcx;if(i:_comment=~"due")quit;quit 255'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --image-descriptions-file-separator '?'    --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c '/2.pcx;if(i:_comment=~"due")quit 255;quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c '/1.pcx;if(i:prime=="no")quit 255;quit;'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c '/4.pcx;if(i:prime=="yes")quit 255;quit;'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c '/2.pcx;if(i:_comment!="due")quit 255;quit;'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c '/4.pcx;quit (i:_comment)'; test $$? = 44 
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c '/0.pcx;quit (i:_comment)'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c 'list "vars"; if(! _last_cmd_output =~ "vals.*files") quit 255;;quit;'
	rm -f "tmp.desc"; # in view of append test
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ -c 'desc "load" "$(top_srcdir)/media/special/images.desc"; desc "reload" "$(top_srcdir)/media/special/images.desc"; desc "-nooverw" "save" "tmp.desc"; quit;' && test `wc -w < tmp.desc` = 15 && rm tmp.desc # need proper 'reload' test
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ -c 'desc "load" "$(top_srcdir)/media/special/images.desc" "\t"; desc "reload" "$(top_srcdir)/media/special/images.desc" "\t"; desc "-nooverw" "save" "tmp.desc" "X"; quit;' && test `grep X tmp.desc |  wc -w` = 10 # need proper 'load' with separator test
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ -c 'desc "load" "$(top_srcdir)/media/special/images.desc" "\t"; desc "reload" "$(top_srcdir)/media/special/images.desc" "\t"; desc "-append" "save" "tmp.desc" "X"; quit;' && test `grep X tmp.desc |  wc -w` = 20 && rm tmp.desc # need proper 'load' with separator test
if WANT_RECURSIVE_DIRS
	if $(CXX11TEST); then $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-shadow-directory $(top_srcdir)/media/../ -c 'scale "shadow"; if (_last_cmd_output =~ "image substituted")quit 0; else quit 255;'; fi
	if $(CXX11TEST); then $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ --load-shadow-directory $(top_srcdir)/var/  -c 'scale "shadow"; if (_last_cmd_output =~ "image substituted")quit 0; else quit 255;'; test $$? = 255; fi # don't be scared of error messages -- we're testing error handling here
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* -k n -k q -K n -K n -K q -F 'stdout _fileindex;if(_fileindex!=2){quit 255}quit' # -k executes intermingled with -K
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* -k n -k n -k q -K n -K q -F 'stdout _fileindex;if(_fileindex!=3){quit 255}quit' # -k executes intermingled with -K
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* -k n -k q -K n -K n -K q -F 'stdout _fileindex;if(_fileindex!=2){quit 255}quit' # -k executes intermingled with -K
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* -k n -k n -k q -K n -K q -F 'stdout _fileindex;if(_fileindex!=3){quit 255}quit' # -k executes intermingled with -K
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*  -/  media/numbers_pcx/4.pcx -c 'stdout _fileindex;if(_fileindex!=1){quit 255}quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*  -/                    4.pcx -c 'stdout _fileindex;if(_fileindex!=5){quit 255}quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* --/  media/numbers_pcx/4.pcx -c 'stdout _fileindex;if(_fileindex!=1){quit 255}quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* --/                    4.pcx -c 'stdout _fileindex;if(_fileindex!=5){quit 255}quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* --// media/numbers_pcx/4.___ -c 'stdout _fileindex;if(_fileindex!=1){quit 255}quit'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/* --// media/numbers_pcx/4.pcx -c 'stdout _fileindex;if(_fileindex!=5){quit 255}quit'
endif
if ENABLE_HISTORY
if ENABLE_PCX
	test "`$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ -K /pcx -K q -F 'stdout _fileindex'`" -lt "`$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/ -K /pcx -K .q -F 'stdout _fileindex'`" # 2 -lt 3 (. after / does repeat action)
endif
endif
if ENABLE_JPEG
if WANT_READ_STDIN_IMAGE
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'stdout i:width.i:height; eval "quit"' -i < $(top_srcdir)/media/image.jpg $(SNTZ) | tail -1` != "00" # mostly coverage
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.jpg -C '_downscale_huge_at_load=-1' -= -c 'pan "0%" "100%"' -cq # pan_to; TODO: test properly
if ENABLE_SYSTEM_CALLS
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.jpg -C '_downscale_huge_at_load=-1' -K++++ -c 'flip;mirror;pan_left;pan_right;pan_up;pan_down;pan_ne;pan_sw;pan_se;pan_nw "100%"' -c 'popen "echo quit"' # mostly coverage
endif
endif
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o non-existing-reverting-to-dumb-without-error -Kq
if ENABLE_GIF
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 2:6 $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 000
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 2:7 $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 11'libungif'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 2:1K $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 11'libungif'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 2+4 $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 000
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 2+5 $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 11'libungif'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 0 $(top_srcdir)/media/icon_smile.gif -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 11'libungif'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 6 $(top_srcdir)/media/icon_smile.gif -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 000
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 6:1000 $(top_srcdir)/media/icon_smile.gif -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' | grep -v Reading $(SNTZ)` = 000
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 7 $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 11'libungif'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 6 $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 000
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_seek_magic="gif"' $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' | tr -d '\n' | tr ' ' _  $(SNTZ)` = 000
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -C '_seek_magic="GIF"' $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 11'libungif'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 8 -C '_seek_magic="GIF"' $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 000
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb --offset 6 -C '_seek_magic="GIF"' $(top_srcdir)/media/special/icon_smile_gif_at_7.bin -c 'stdout (i:width!=0).(i:height!=0)._last_file_loader;quit' $(SNTZ)` = 11'libungif'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_interlaced.gif -c 'quit !(i:width!=0)' # interlaced gif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_truncated_bad.gif.bin -c 'quit !(i:width==0)' # deliberately invalid gif
endif
if ENABLE_MATRIX_MARKET_DECODER
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/matrix.mtx -c 'stdout i:rows.i:columns.i:nnz._last_file_loader;quit' $(SNTZ)` = 222MatrixMarket;
endif
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c '_max_cached_images=1;stdout _fileindex;quit;' --no-stat-push a/b.png c/d.png e/f.png $(SNTZ)` = 1
if ENABLE_JPEG
if ENABLE_PCX
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/[0-4].pcx $(top_srcdir)/media/special/truncated_bad.jpg.bin  $(top_srcdir)/media/numbers_pcx/[5-9].pcx  -Knnnnnq  -F 'if(_fileindex!=6)quit 6;quit' # fim handles broken jpeg files graciously
endif
endif
if ENABLE_PNG
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/truncated_bad.png.bin -c 'quit'; test $$? = 134 # deliberately invalid png: reminder that need start to use png_set_error_fn
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_gray.png -c 'quit'; # just coverage so far
endif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit' --slideshow 0 -1 # if this does not hang is good
	if which timeout; then \
		timeout 2 $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit' --slideshow 0 ; R=$$?; \
		if which reset ; then reset; fi; \
		test $$R = 124; \
	fi;
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit 0; quit 1;' -F 'quit 2; quit 3'; test "$$?" = 2
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -C 'quit 0; quit 1;'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit 0; quit 1;'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit 3'; test "$$?" = 3
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit 2' -F 'quit 0; quit 1;'
	for r in 0 1 2 254 255 ; do $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c "quit $$r" || test $$? = $$r ; done ;
	for r in 0 1 2 254 255 ; do $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -C "quit $$r" || test $$? = $$r ; done ;
	for r in 0 1 2 254 255 ; do $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'quit' -F "quit $$r" || test $$? = $$r ; done ;
if ENABLE_SDL
if ENABLE_PCX
	if test -n "$(DISPLAY)"; then \
		$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o sdl --resolution w $(top_srcdir)/media/numbers_pcx/ -R -q -a -c 'display "reinit" "w200:200"' -c 'quit (i:swidth!=_screen_height) || (i:swidth!=_screen_width)'; # pcx needs no library\
	fi
	if test -n "$(DISPLAY)"; then $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o sdl=17x17 -A $(top_srcdir)/media/numbers_pcx/1.pcx  -q -c 'quit !(_autotop==1)'; fi # --autotop
	if test -n "$(DISPLAY)"; then $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o sdl=17x17 -P $(top_srcdir)/media/numbers_pcx/1.pcx  -q -c 'quit (_autotop!=1) || (i:swidth!=_screen_width)'; fi # --text-reading
	if test -n "$(DISPLAY)"; then $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o sdl=17x17    $(top_srcdir)/media/numbers_pcx/1.pcx  -q -c 'quit (_autotop!=1) || (i:swidth!=_screen_width)'; test $$? = 1; fi # --text-reading
	if test -n "$(DISPLAY)"; then $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o sdl=17:16 -q -P $(top_srcdir)/media/numbers_pcx/?.pcx -c 'align "info";a1=_last_cmd_output; scroll "forward";align "info";a2=_last_cmd_output; next; align "info";a3= _last_cmd_output; goto _lastfileindex; align "info";a4=_last_cmd_output; quit !(a1==a3 && a2==a4 && a1!=a2)' -kq ; fi # --text-reading
	if test -n "$(DISPLAY)"; then $(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o sdl=17:16 -q    $(top_srcdir)/media/numbers_pcx/?.pcx -c 'align "info";a1=_last_cmd_output; scroll "forward";align "info";a2=_last_cmd_output; next; align "info";a3= _last_cmd_output; goto _lastfileindex; align "info";a4=_last_cmd_output; quit !(a1==a3 && a2==a4 && a1==a2)' -kq ; fi # --text-reading
endif
endif
if ENABLE_GIF
	$(FIM_EXE) $(FIMNORCOPTS)         -V | grep upported.file.formats | grep gif
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/media/icon_smile.gif -c 'rotate 10' -c quit
	test "`$(FIM_EXE) $(FIMNORCOPTS) -o dumb -R $(top_srcdir)/media/numbers/ -k 1 -k n -k 1 -k n -k q -F 'stdout _fileindex' $(SNTZ)`" = 3 # 3 and not 11 # note: need only supported files along the path to pass
endif
	top_srcdir=$(top_srcdir) FIMNORCOPTS="$(FIMNORCOPTS)" $(SHELL) $(top_srcdir)/scripts/tests/version.sh
	top_srcdir=$(top_srcdir) FIMNORCOPTS="$(FIMNORCOPTS)" $(SHELL) $(top_srcdir)/scripts/tests/font.sh
	$(NO_ASAN) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'font; font "load" "non_existing"; font "next"; font "scan"; font "info"; font "prev"; quit'; true; # TODO: test more than tis mere coverage (btw, this one leaks, and in debug or asan mode fails) -- FIXME)
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/media -c '_caption_over_image=3;display;quit;'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X -c 'quit;'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X --execute-script $(top_srcdir)/scripts/example/oneline.fim -c quit
	$(FIM_EXE) $(FIMNORCOPTS)            $(top_srcdir)/media/ -o dumb -X --execute-script $(top_srcdir)/scripts/example/quit_23.fim -kq; test $$? = 23; # quit in script possible
if WANT_DEFAULT_FIMRC
	$(FIM_EXE) --no-history --no-rc-file $(top_srcdir)/media/ -o dumb -X --etc-fimrc      $(top_srcdir)/scripts/example/quit_23.fim -kq; test $$? = 23; # quit at init possible
	test `$(FIM_EXE) --no-history --no-rc-file $(top_srcdir)/media/ -o dumb -X --etc-fimrc $(top_srcdir)/scripts/example/set_var.fim -c 'stdout variable;quit' $(SNTZ)` = value # note: --no-rc-file and --etc-fimrc are incompatible
else
	$(FIM_EXE) --no-history --no-rc-file $(top_srcdir)/media/ -o dumb -X --etc-fimrc      $(top_srcdir)/scripts/example/quit_23.fim -kq; # etc-fimrc quietly ignored
endif
	test `$(FIM_EXE) $(FIMNORCOPTS)            $(top_srcdir)/media/ -o dumb -X --etc-fimrc $(top_srcdir)/scripts/example/set_var.fim -c 'stdout variable;quit' $(SNTZ)` = '0'    # note: --no-rc-file and --etc-fimrc are incompatible
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o dumb -X --no-internal-config -c 'width;quit !(i:width == 0)'
if WANT_READLINE
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o dumb --no-commandline -K ':quit 10' # quits at second char
	rm -f record.fim; $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o dumb -W record.fim -K ':quit 10' -K ''; grep -q quit.10 record.fim && rm record.fim
endif
if ENABLE_HISTORY
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o dumb --no-history --no-history-load --no-history-save -kq # FIXME: bogus test
	rm -f .fim_history && HOME=`pwd` $(FIM_EXE) -N --no-etc-fimrc-file $(top_srcdir)/media/ -o dumb -C 'next;next' -c quit && grep -q '^next;next' .fim_history && rm .fim_history # note: history file may contain spaces newlines etc
endif
if ENABLE_NO_HISTORY_TESTS
	rm -f .fim_history && HOME=`pwd` $(FIM_EXE) -N --no-etc-fimrc-file $(top_srcdir)/media/ -o dumb -C 'next;' -c quit && ! test -s .fim_history
endif
if ENABLE_PNG
	$(FIM_EXE) $(FIMNORCOPTS)         -V | grep upported.file.formats | grep png
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/media/image.png -c 'next'   -c quit # leak fixed
if WITH_INKSCAPE
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb    $(top_srcdir)/media/sample.svg -c 'quit !(i:width>0 && i:height>0)' #
endif
endif
if ENABLE_TIFF
	if $(FIM_EXE) $(FIMNORCOPTS)         -V | grep upported.file.formats | grep tiff && which convert ; then \
		convert $(top_srcdir)/media/image.png $(top_srcdir)/image.tiff && \
		$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/image.tiff -c 'next' -c quit; fi # leak fixed
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/ -c 'echo i:*' -k Tab -c quit # bogus test: should rather exercise MiniConsole
	if which convert ; then \
		convert $(top_srcdir)/media/numbers_pcx/*.pcx $(top_srcdir)/image.tiff && \
		$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/image.tiff -c 'quit !(i:pages == 10)'; fi # multipage code in Image and load
	rm -f $(top_srcdir)/image.tiff
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_gray.tiff                      -c 'i:width;quit !(i:width!=0)' # convert media/numbers/0.gif -grayscale average media/special/0_gray.tiff
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_rgba.tiff                      -c 'i:width;quit !(i:width!=0)' # convert media/numbers/0.gif -alpha transparent -depth 4 media/special/0_rgba.tiff
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_monochrome_white_polarity.tiff -c 'i:width;quit !(i:width!=0)' # convert media/numbers/0.gif -monochrome -define quantum:polarity=min-is-white -depth 1 media/special/0_monochrome_white_polarity.tiff
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/special/0_16bps_msb.tiff -c 'i:width;quit !(i:width!=0)' # convert media/numbers/0.gif -depth 16 -colorspace srgb -define tiff:endian=msb media/special/0_16bps_msb.tiff
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/media/multipage/numbers_one_to_ten.tiff -c 'goto 3; if (2!=page)quit 1; goto 3; if (2!=page)quit 1; quit 0;' # needs libtiff with multipage support
endif
if ENABLE_JPEG
	$(FIM_EXE) $(FIMNORCOPTS)         -V | grep upported.file.formats | grep jpeg
	if which wrjpgcom; then wrjpgcom -replace -comment 'add JPEG comment' $(top_srcdir)/media/image.jpg > image_commented.jpg && $(FIM_EXE) $(FIMNORCOPTS) -o dumb -X image_commented.jpg -c quit && rm -f image_commented.jpg; fi # leak fixed
	if which convert && which exiftool; then \
		convert $(top_srcdir)/media/icon_smile.gif $(top_srcdir)/media/icon_smile.jpg && \
		exiftool -exif:iso=42 -overwrite_original_in_place $(top_srcdir)/media/icon_smile.jpg && \
	      	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.jpg -c quit && \
	       	rm -f $(top_srcdir)/media/icon_smile.jpg ; \
	fi # only cover test exif
	cp $(top_srcdir)/media/image.jpg $(abs_srcdir)/media/upc_abs.JPG && $(FIM_EXE) $(FIMNORCOPTS) -o dumb --no-stat-push $(abs_srcdir)/media/upc_abs.JPG -c 'quit !(i:width>0 && i:height>0)' && rm -f $(abs_srcdir)/media/upc_abs.JPG # not really jpg-specific, exploits regexps & co
endif
if ENABLE_POPPLER
if ENABLE_PDF
	if which epstopdf && which epstopdf; then \
		epstopdf $(top_srcdir)/media/special/postscript.eps && \
		$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/media/special/postscript.pdf -c 'quit !(i:width>0 && i:height>0)' ; fi # libpoppler
endif
endif
if ENABLE_PS
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/media/special/postscript.eps -c 'quit !(i:width>0 && i:height>0)'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X -C '_downscale_huge_at_load=-1;_preferred_rendering_dpi=100' $(top_srcdir)/media/special/postscript.eps -c 'quit !(i:width>0 && i:height>0)'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X -C '_downscale_huge_at_load=-1;_preferred_rendering_dpi=100' $(top_srcdir)/media/special/postscript.eps -c 'stdout i:width;quit'` -lt `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X -C '_downscale_huge_at_load=-1;_preferred_rendering_dpi=200' $(top_srcdir)/media/special/postscript.eps -c 'stdout i:width;quit'`
endif
if ENABLE_PNG
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c 'info;stdout _last_cmd_output;quit' | grep 'media.*KB' # cover info
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X $(top_srcdir)/media/image.png -c '/*.jpg' -c quit # parser leak fixed
endif
	$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb -X -c 'identifier /*path...' -k q # parser leak fixed with destructor lines in yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -hs -h | grep h.s -q
	test `$(FIM_EXE) $(FIMNORCOPTS) --help   | wc -c` -eq `$(FIM_EXE) $(FIMNORCOPTS) --help=s | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=s | wc -l` -eq `$(FIM_EXE) $(FIMNORCOPTS) --help=d | wc -l`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=s | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help=d | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=d | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help=l | wc -c`
	! $(FIM_EXE) $(FIMNORCOPTS) --help=s      | grep '\\fR'
	! $(FIM_EXE) $(FIMNORCOPTS) --help=s desc | grep '\\fR'
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=l | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help=m | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=l | wc -l` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help=m | wc -l`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help _last_cmd_output | wc -l` = 5
	test `$(FIM_EXE) $(FIMNORCOPTS) --help _LAST_CMD_OUTPUT | wc -l` = 2
	test `$(FIM_EXE) $(FIMNORCOPTS) --help A | wc -l` = 4
	test `$(FIM_EXE) $(FIMNORCOPTS) -hs -h | wc -l` = 2
	test `$(FIM_EXE) $(FIMNORCOPTS) -hd -h | wc -l` = 2
	test `$(FIM_EXE) $(FIMNORCOPTS) -hl -h | wc -l` = 3
	test `$(FIM_EXE) $(FIMNORCOPTS) -hm -h | wc -l` = 3
	test `$(FIM_EXE) $(FIMNORCOPTS) -hs -h | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS) -hd -h | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS)  -h      -R          | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS)  -h      -R       -R | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS)  -hs     -R          | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS)  -hd     -R          | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS)  -hd     -R          | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS)  -hl     -R          | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS)  -hl     -R          | wc -c` -eq `$(FIM_EXE) $(FIMNORCOPTS)  -hm     -R          | wc -c`
if WANT_READ_STDIN_IMAGE
	test `$(FIM_EXE) $(FIMNORCOPTS) --help   --read-from-stdin | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help   --read-from-stdin --read-from-stdin | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=s --read-from-stdin | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help=d --read-from-stdin | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=d --read-from-stdin | wc -c` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help=l --read-from-stdin | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=l --read-from-stdin | wc -c` -eq `$(FIM_EXE) $(FIMNORCOPTS) --help=m --read-from-stdin | wc -c`
	test `$(FIM_EXE) $(FIMNORCOPTS) --help=l --read-from-stdin | wc -l` -lt `$(FIM_EXE) $(FIMNORCOPTS) --help=m --read-from-stdin | wc -l`
endif
#	if $(CXX11TEST); then $(FIM_EXE) $(FIMNORCOPTS) -o dumb --background-recursive -B . -kK -kq -F 'if(_filelistlen==1)quit 255'; fi # -kK triggers a yield (often, not always)
#	if $(CXX11TEST); then $(FIM_EXE) $(FIMNORCOPTS) -o dumb --background-recursive -B .     -kq -F 'if(_filelistlen!=1)quit 255'; fi # no -kK -> no background images in batch mode (often, not always)
	if $(CXX11TEST); then $(FIM_EXE) $(FIMNORCOPTS) -o dumb --background-recursive -B $(top_srcdir)/ -kn -kq -F 'if(_filelistlen< 1)quit 255'; fi # difficult to write a reliable test for -B, but -kn can help a bit with timing
	test `$(FIM_EXE) $(FIMNORCOPTS) --help /a /q | wc -c` -gt 900
	test `$(FIM_EXE) $(FIMNORCOPTS) --help help magnify _TERM jfngjfngjfn | wc -c` -gt 500
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'dump_key_codes; stdout _last_cmd_output; quit' | wc -c` -gt 1000
	test `$(FIM_EXE) $(FIMNORCOPTS) --dump-reference-help     | wc -c` -gt 30000
	test `$(FIM_EXE) $(FIMNORCOPTS) --dump-reference-help     | wc -c` -lt 60000
	test `$(FIM_EXE) $(FIMNORCOPTS) --dump-reference-help=man | wc -c` -gt 60000
	test `$(FIM_EXE) $(FIMNORCOPTS) --dump-default-fimrc      | wc -c` -gt 15000
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'exec "src/fimrc"; echo; stdout; pwd; status "status" unalias' -c quit #; cover test misc corner cases (note: no functionality check here)
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'autocmd; autocmd "PostDisplay";autocmd "PostDisplay" "pattern";autocmd "1" "2" "toomuch"' -c quit #; cover test misc corner cases (note: no functionality check here)
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'autocmd_del; autocmd_del "PostDisplay";autocmd_del "PostDisplay" "pattern";autocmd_del "1" "2" "3";autocmd_del "1" "2" "3" "4"' -c quit #; cover test misc corner cases (note: no functionality check here)
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'alias "a" "quit 11"; unalias "a"; alias "a" "quit 23"; a; quit 13' -c quit ; test $$? = 23
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'alias "a" "quit 23"; unalias "-a"; a; quit 13' -c quit ; test $$? = 13
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'unalias; stdout _last_cmd_output; unalias "NotExistent"; stdout _last_cmd_output;quit' | sed 's/unalias.*$$/Un/g' | tr -d '\n' $(SNTZ)` = UnUn
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c "bind 'F1' 'stdout 1';_max_iterated_commands=2;" -k 10F1 -k q | tr -d '\n' $(SNTZ)` = 11 # test binding repeat and F1 binding
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c "bind 'F1' 'stdout 1';_max_iterated_commands=1;" -k 10F1 -k q | tr -d '\n' $(SNTZ)` = 1 # test binding repeat and F1 binding
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c "bind 'F1' 'stdout 1';_max_iterated_commands=0;" -k 10F1 -k q | tr -d '\n' $(SNTZ)` = 1111111111 # test binding repeat and F1 binding
if WANT_READLINE
if ENABLE_GIF
	cp -p $(top_srcdir)/media/icon_smile.gif $(top_srcdir)/media/icon_smile.pgm
	cp -p $(top_srcdir)/media/icon_smile.gif $(top_srcdir)/media/icon_smile.ppm
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -K /smile.ppm -K '' -K q -F 'quit !(_fileindex==2);' # test slash search
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -K /smole.ppm -K '' -K q -F 'quit !(_fileindex==1);' # test slash search
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -c "goto '?$(top_srcdir)/media/icon_smile.ppm'" -K '' -K q -F 'quit !(_fileindex==2);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -c "_re_search_opts='i';goto '/ICON_SMILE.ppm/'"       -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==2);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -c "_re_search_opts='I';goto '/ICON_SMILE.ppm/'"       -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==1);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -c "_re_search_opts=' ';goto '/media/icon_smile.ppm/'" -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==1);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -c "                    goto '/media/icon_smile.ppm/'" -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==1);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -c "_re_search_opts='b';goto '/media/icon_smile.ppm/'" -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==1);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm -c "_re_search_opts='b';goto '/icon_smile.ppm/'"       -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==2);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c "_re_search_opts=' ';goto '/due/'" -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==1);' # test goto query
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file $(top_srcdir)/media/special/images.desc -c "_re_search_opts='D';goto '/due/'" -K '' -K q -F 'stdout _fileindex; quit !(_fileindex==3);' # test goto query
	rm $(top_srcdir)/media/icon_smile.pgm $(top_srcdir)/media/icon_smile.ppm
endif
endif
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c 'v=1; bind "_" "recording;stdout _last_cmd_output;"; bind "S" "recording \"start\""' -K _S_rQ_E_q -F 'stdout i:_orientation' | wc -w ` -gt 70
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c 'v=1; bind "_" "recording;                        "; bind "S" "recording \"start\""' -K _S_rQ_E_q -F 'stdout i:_orientation' | wc -w ` -lt 70
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c 'v=1; bind "_" "recording;stdout _last_cmd_output;"; bind "S" "recording \"start\""' -K _S_rQ_E_q -F 'stdout i:_orientation' | grep ^.$$ | tr -d '\n' $(SNTZ)` = 2
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -C 'v=1; bind "_" "recording;stdout _last_cmd_output;"; bind "S" "recording \"start\""' -K _S_rQ_E_q -F 'stdout i:_orientation' | grep ^.$$ | tr -d '\n' $(SNTZ)` = 1
if ENABLE_GIF
	test "`$(TIMEOUT_SMALL) $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers/* -K 2n.q -F 'stdout i:_filename;' $(SNTZ)`" = $(top_srcdir)/media/numbers/3.gif
endif
	test "`$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c 'stderr; stdout; quit' 2>&1 > /dev/null | grep echo`" = "echo command" # stdout vs stderr
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -c 'stderr "out:due";stdout "out:uno";quit' 2>&1 > /dev/null | grep out $(SNTZ)` = "out:due" # stdout vs stderr
if WANT_READLINE
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media -C set_commandline_mode -K 'stdout "cli"; quit;' -K '' $(SNTZ)` = cli
endif
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'set; stdout _last_cmd_output; quit' | wc -w` -ge 50 # test set
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'set "_TERM"; stdout _last_cmd_output; quit' | wc -w` -eq 1 # test set
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'set "_TERM" "WhatWhat"; stdout _TERM _last_cmd_output; set 1 2 3 ; stdout _last_cmd_output; quit' | sed "s/set.*$$/Set/g" | tr -d '\n' $(SNTZ)` = WhatWhatSetSetSet
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -c 'unbind; unbind "?"; bind "~"; bind "n"; bind "n" ""' -c quit
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/ -c 'color "desaturate";color "negate";color "identity";color "colorblind" "daltonize";color "deuteranopia" "daltonize";color "protanopia" "daltonize";color "tritanopia" "daltonize";color "colorblind"' -c quit # only cover test
	if which timeout; then if ! timeout 2 $(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/ -o dumb -c '/media' -c 'quit' ; then echo 'Seems like -c "/<filename>" is broken!'; false; fi; fi
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb -X -c 'quit 14' ; if test $$? = 14 ; then echo '[*] Error code return test PASSED' ; else echo 'Error code return test FAILED'; false ; fi
#
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c 'a=../' -k q # TODO: test missing. for lex.lex
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c 'a=./'  -k q # TODO: test missing. for lex.lex
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c $$'\b' -k q # TODO: test missing. for lex.lex
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c 'stdout 0100' -k q # TODO: test missing. for lex.lex
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c 'stdout 0x10' -k q # TODO: test missing. for lex.lex
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c 'quit' -F '|'  # TODO: test missing. for lex.lex
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/image.png -c 'i=3;do  { i=i-1; stdout i; } while (i>=1) stdout "ok";quit i;' # seldom used language feats (TODO): this is buggy anyways..)
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c '-1;stdout _fileindex;quit!(_fileindex==10);' # FIXME
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c '-3;stdout _fileindex;quit!(_fileindex==8);'
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/?.pcx -c ' 0;stdout _fileindex;quit!(_fileindex==1);'
	test `$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/-.pcx -c '3 stdout "y"; quit ((2+2)%(1+1))' | tr -d '\n' $(SNTZ)` = yyy # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c '+"30.0"%;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c '+ 30.0% ;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c '*"30.0" ;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c '* 30.0  ;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c ' "30.0"%;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c   '30.0 %;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c '-"30.0"%;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c '- 30.0% ;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c  '"30.0"%;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c  ' 30.0 %;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c  '1,2a;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c  '1z;quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c  'stdout exce.ption' -k q  # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/0.pcx -c  '{1;;;}quit' # FIXME: add actual test # stress yacc.ypp
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c           'cache; quit ! ( _want_prefetch && _cache_status =~ "count:1/5")' # on all loadable files
	$(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx -c 'prefetch; cache; quit ! ( _want_prefetch && _cache_status =~ "count:3/5")' # on all loadable files
if ENABLE_PNG
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale       ;          w2=i:swidth;   quit !(w1==w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "Z"   ;          w2=i:swidth;   quit !(w1==w2)' # Z is unknown
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "H"   ;          w2=i:swidth;   quit !(w1!=w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale ">"   ;          w2=i:swidth;   quit !(2*w1==w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "<"   ;          w2=i:swidth;   quit !(w1/2==w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "*0.5";          w2=i:swidth;   quit !(w1/2==w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "*0"  ;          w2=i:swidth;   quit !(w1 < w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "+"   ;          w2=i:swidth;   quit !(w1 < w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'm1=_magnify_factor;scale "++"; m2=_magnify_factor;quit !(m1 < m2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'm1=_magnify_factor;scale "+-"; m2=_magnify_factor;quit !(m1 > m2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'r1=_reduce_factor;scale "++"; r2=_reduce_factor;  quit !(r1 < r2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'r1=_reduce_factor;scale "+-"; r2=_reduce_factor;  quit !(r1 > r2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'm1=_magnify_factor;scale "+*"; m2=_magnify_factor;quit !(m1 < m2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'm1=_magnify_factor;scale "+/"; m2=_magnify_factor;quit !(m1 > m2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'r1=_reduce_factor;scale "+*"; r2=_reduce_factor;  quit !(r1 < r2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'r1=_reduce_factor;scale "+/"; r2=_reduce_factor;  quit !(r1 > r2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "-0"   ;         w2=i:swidth;   quit !(w1==w2)' # cover
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "0"    ;         w2=i:swidth;   quit !(w1==w2)' # cover
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "*"    ;         w2=i:swidth;   quit !(w1==w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "+"    ;         w2=i:swidth;   quit !(w1 < w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "-"    ;         w2=i:swidth;   quit !(w1 > w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "-0.5";          w2=i:swidth;   quit !(w1 > w2)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/image.png $(FIMSCLTSTOPTS) -c 'w1=i:swidth;scale "-10%";          w2=i:swidth;   quit !(w1 > w2)'
endif
	test ! -f nonexistent.desc && ! $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --load-image-descriptions-file nonexistent.desc # ensure error on missing description file
	test ! -f nonexistent.desc && ! $(FIM_EXE) $(FIMNORCOPTS) -o dumb $(top_srcdir)/media/numbers_pcx/*.pcx --mark-from-image-descriptions-file nonexistent.desc # ensure error on missing description file
if ENABLE_MOUSE
	if $(FIM_EXE) --help crop | grep -q is.a.command; then make croptests; fi
endif
if ENABLE_AA
	@make aatests
endif
if ENABLE_CACA
	@make cacatests
endif
if ENABLE_FRAMEBUFFER
	@if test -z "$(DISPLAY)$(SSH_TTY)" -a -w "/dev/fb0"; then make fbtests; fi
endif
if ENABLE_IMLIB2
	@make imlib2tests
endif
if ENABLE_SDL
	@if test -n "$(DISPLAY)"; then make sdltests; fi
endif
if ENABLE_GTK
	@if test -n "$(DISPLAY)"; then make gtktests; fi
endif
	@if test -n "$(DISPLAY)" -a -z "$(SSH_TTY)" && $(FIM_EXE) -V | grep ' \(gtk\|sdl\)' ; then make anyxtests; fi # space is there to avoid confusion with e.g. --disable-sdl
if HAVE_FIMGS
	make fimgstests
endif
	make ltests
	@echo "[*] TESTS: SUCCESS"
else
	@echo "[!] TESTS: Skipped (you are cross-compiling, aren't you?)"
endif

.PHONY: croptests
croptests:
if ENABLE_GIF
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif $(FIMSCLTSTOPTS) -c 'w1=i:width;_crop_once="0 0 10 10";crop;w2=i:width;quit !(w1+w2==25)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif $(FIMSCLTSTOPTS) -c 'w1=i:width;crop            ;w2=i:width;quit !(w1+w2==22)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif $(FIMSCLTSTOPTS) -c 'w1=i:width;crop 10         ;w2=i:width;quit !(w1+w2==16)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif $(FIMSCLTSTOPTS) -c 'w1=i:width;crop 40 40 60 60;w2=i:width;quit !(w1+w2==18)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif $(FIMSCLTSTOPTS) -c 'w1=i:width;crop 25 25      ;w2=i:width;quit !(w1+w2==18)'
	$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/icon_smile.gif $(FIMSCLTSTOPTS) -c 'w1=i:width;crop 50 50      ;w2=i:width;quit !(w1+w2==22)'
endif

.PHONY: ltests
ltests:
if ENABLE_JPEG
	$(FIM_EXE) $(FIMNORCOPTS)         -V | grep upported.file.formats | grep jpeg
	if test $(abs_builddir) = $(abs_srcdir) ; then make -C src/testdir ; else echo "out-of-dir? skipping ltests..." ; fi
endif

.PHONY: fimgstests
fimgstests:
	$(FIMGS_EXE) --help -- $(FIMNORCOPTS) >/dev/null 2>&1 ; echo $$? = 255 # no fimgs --help option yet (so we make sure it ends with error)
	test `$(FIMGS_EXE) -h -- $(FIMNORCOPTS) | wc -l` -gt 14
	test `$(FIMGS_EXE) -m -- $(FIMNORCOPTS) | wc -w` -gt 53
if HAVE_FIMGS
	$(FIMGS_EXE) -- $(FIMNORCOPTS) -o dumb ./media/special/postscript.eps -c 'stdout _last_file_loader;quit 1' -K quit; test $$? = 0 # spaces not accepted by fimgs
endif
if ENABLE_PCX
	if which tar                  ; then $(FIMGS_EXE) $(top_srcdir)/media/special/numbers_pcx.tar     -- $(FIMNORCOPTS) -o dumb -c 'quit !(_last_file_loader=="pcx")' ; fi
	if which tar && which bunzip2 ; then $(FIMGS_EXE) $(top_srcdir)/media/special/numbers_pcx.tar.bz2 -- $(FIMNORCOPTS) -o dumb -c 'quit !(_last_file_loader=="pcx")' ; fi
	if which tar && which gunzip  ; then $(FIMGS_EXE) $(top_srcdir)/media/special/numbers_pcx.tar.gz  -- $(FIMNORCOPTS) -o dumb -c 'quit !(_last_file_loader=="pcx")' ; fi
	if which tar && which unxz    ; then $(FIMGS_EXE) $(top_srcdir)/media/special/numbers_pcx.tar.xz  -- $(FIMNORCOPTS) -o dumb -c 'quit !(_last_file_loader=="pcx")' ; fi
endif


FIMSCRIPTSDIR=`pwd`/scripts/maintenance/

# new target, still not working
.PHONY: tests-all
tests-all: tests
	cd src && which cppcheck && cppcheck *cpp *.h || true
	$(SHELL) $(FIMSCRIPTSDIR)/configure-brute-check.sh
	
#news-dump:
#	$(AWK)  -- 'v=0;($$1 == "Version" && $$2=="'`cat VERSION`'"){while((input=getline) && $$1!="Version"){print}};'  NEWS > f

# this rule is deprecated, too
.PHONY: test
test:	all
	@$(FIM_EXE) $(FIMNORCOPTS) $(top_srcdir)/media/* #~/M*s/*g

# this rule is old and deprecated
#tgz:	clean
#	tar -czf ../fim.`date +%Y%m%d%H%M`.tgz ../fim/*
#	ls -l ../fim*.tgz  -v

# this rule is encouraged
.PHONY: report
report:
	@cat VERSION
	@echo '-'
	@$(LEX) -V   2>&1
	@echo '-'
	@$(YACC) -V  2>&1
	@echo '-'
	@$(CC) -v    2>&1
	@echo '-'
	@echo 'now please report the bug with this information to the author via email' ;

# this rule is redundant
.PHONY: exec
exec:   test

# this rule is smart, isn't it ? I love it !
.PHONY: edit
edit:
	$(EDITOR) $(srcdir)/fim.cpp +':split $(srcdir)/fim.h' # Vim ! :)

#.PHONY:
#	@true

# this rule is informative
.PHONY: wc
wc:
	wc $(srcdir)/*.cpp $(srcdir)/*.h $(srcdir)/yacc.ypp $(srcdir)/lex.lex
	@#wc $(FIM) # missing headers..

.PHONY: devhelp
devhelp:
	@echo -----------------------------------------------
	@echo '			short Makefile dev help'
	@echo -----------------------------------------------
	@echo
	@echo 'make uptrunk' will upload trunk tarballs to 
	@echo   http://download.savannah.nongnu.org/releases/fbi-improved/
	@echo
	@make help

.PHONY: help
help:
	@echo -----------------------------------------------
	@echo '			short Makefile help'
	@echo -----------------------------------------------
	@echo
	@echo 'Please read the documentation file before complaining!'
	@echo
	@echo 'The first documentation file you should read is README'
	@echo
	@echo 'If you are experiencing problems, please contact the code author.'
	@echo
	@echo 'His mail box resides as dezperado, then a dot, then autistici dot org.'
	@echo
	@echo 'You are encouraged attaching the output of `make report` in your bug reports.'
	@echo 'Thanks for your collaboration.'
	@echo

.PHONY: ai
ai: $(distdir).tar.gz.sig  $(distdir).tar.gz $(distdir).tar.bz2.sig $(distdir).tar.bz2
	make up -C var/

.PHONY: up
up: upload news

.PHONY: news
news: freshmeat mail-announce

.PHONY: dox
dox: doc/fim.man doc/fimrc.man doc/fimgs.man

MANSUBST = sed -e 's,@sysconfdir\@,$(sysconfdir),g' -e 's,@docdir\@,$(docdir),g' -e 's,@FIM_CUSTOM_DEFAULT_CONSOLEFONT\@,$(FIM_CUSTOM_DEFAULT_CONSOLEFONT),g' -e 's,-,\\-,g' -e 's,TODO.*$$,,g'

doc/fim.man.in doc/fimrc.man.in: $(FIM_EXE)
if HAVE_RUNNABLE_TESTS
	$(FIM_EXE) $(FIMNORCOPTS) --help=m                    > doc/fim.man.in
	$(FIM_EXE) $(FIMNORCOPTS) --dump-reference-help=man   > doc/fimrc.man.in
endif

doc/fimgs.man.in: $(FIM_EXE)
if HAVE_RUNNABLE_TESTS
if HAVE_GS
	$(FIMGS_EXE) -m -- $(FIMNORCOPTS)                     > doc/fimgs.man.in
endif
endif

doc/fim.man doc/fimrc.man doc/fimgs.man: doc/fim.man.in doc/fimrc.man.in doc/fimgs.man.in
	cat doc/fim.man.in   |$(MANSUBST) > doc/fim.man
	cat doc/fimrc.man.in |$(MANSUBST) > doc/fimrc.man
	cat doc/fimgs.man.in |$(MANSUBST) > doc/fimgs.man

# fixme : missing integration with ChangeLog file
.PHONY: mail-announce
mail-announce:
	@ figlet fim | mutt -s "fim-`cat VERSION` release" "fbi-improved-devel@nongnu.org"  -i -

.PHONY: log
log: changelog

# from trunk, we should call svn2cl .. to get the full fim changelog history 
.PHONY: changelog
changelog:
	svn2cl -i ..

.PHONY: release
release: all-recursive
	make signed-dist
#	false # reactivate this once fim-0.7.tar.gz is released and trunk offers anything of interest
	make dist-tests
	make -C var all
	make -C var ci
	make uptrunk
 
#release: upload freshmeat

.PHONY: upload
upload: savannah 

.PHONY: uptrunk
uptrunk:
	read && scp $(distdir).tar.gz.sig  $(distdir).tar.gz $(distdir).tar.bz2.sig $(distdir).tar.bz2 dezperado@dl.sv.nongnu.org:/releases/fbi-improved/

# echo -en "cd /releases/fbi-improved\nput $(distdir).tar.gz.sig\n put fim-${fim_cv_version}.tar.gz\n put fim-${fim_cv_version}.tar.bz2\n put fim-${fim_cv_version}.tar.bz2.sig"  | sftp "dezperado@dl.sv.gnu.org" -b 
.PHONY: savannah_
savannah_: signed-dist
	grep trunk VERSION || echo do
	lynx -dump dl.sv.gnu.org/releases/fbi-improved

.PHONY: savannah-lookup
savannah-lookup:
	lynx -dump dl.sv.gnu.org/releases/fbi-improved

# fixme : missing integration with ChangeLog file and such information
.PHONY: freshmeat_old
freshmeat_old: 
	@ $(AWK)  -- 'v=0;($$1 == "Version" && $$2=="'`cat VERSION`'"){while((input=getline) && $$1!="Version"){print}};'  NEWS 
	@echo freshmeat-submit -v "fbi-`cat VERSION`" \
	--project fbi-improved \
	--license GPL \
	--mailing-list-url      $$(cat README | grep "^Mailing List :"|sed s/^.*\ :.//g) \
	--home-page-url         $$(cat README | grep "^Web Page  "|sed s/^.*\ :.//g) \
	--cvs-url               $$(cat README | grep "^Repository"|sed s/^.*\ :.//g) \
	--gzipped-tar-url       $$(cat README | grep "^Releases  "|sed s/^.*\ :.//g)/"fim-`cat VERSION`".tar.gz \
	--mirror-site-url       $$(cat README | grep "^Off.*rror : "|sed s/^.*\ :.//g) \
	--changelog-url         $$(cat README | grep "^ChangeLog  "|sed s/^.*\ :.//g)
	@ echo "are you sure ? (any key to abort, yes to continue)"  && read yes && [[ "$${yes}" == "yes" ]] && \
	$(AWK)  -- 'v=0;($$1 == "Version" && $$2=="'`cat VERSION`'"){while((input=getline) && $$1!="Version"){print}};'  NEWS | \
	 freshmeat-submit -v "fbi-`cat VERSION`" \
	--project fbi-improved \
	--license GPL \
	--mailing-list-url      $$(cat README | grep "^Mailing List :"|sed s/^.*\ :.//g) \
	--home-page-url         $$(cat README | grep "^Web Page  "|sed s/^.*\ :.//g) \
	--cvs-url               $$(cat README | grep "^Repository"|sed s/^.*\ :.//g) \
	--gzipped-tar-url       $$(cat README | grep "^Releases  "|sed s/^.*\ :.//g)/"fim-`cat VERSION`".tar.gz \
	--mirror-site-url       $$(cat README | grep "^Off.*rror : "|sed s/^.*\ :.//g) \
	--changelog-url         $$(cat README | grep "^ChangeLog  "|sed s/^.*\ :.//g)
#	--url-demo         $$(cat README | grep "^Web Page  "|sed s/^.*:.//g)
#	--bzipped-tar-url  $$(cat README | grep "^Web Page  "|sed s/^.*:.//g)

.PHONY: freshmeat-submit
freshmeat-submit:
	@echo Project: fim
	@echo Version: `cat VERSION`
	@echo Release-Focus: Major feature enhancements
	@echo Hide: N
	@echo Home-Page-URL: $$(cat README | grep "^Web Page  "|sed s/^.*\ :.//g)
	@echo Mailing-List-URL: $$(cat README | grep "^Mailing List :"|sed s/^.*\ :.//g)
	@echo Gzipped-Tar-URL: $$(cat README | grep "^Releases  "|sed s/^.*\ :.//g)/"fim-`cat VERSION`".tar.gz
	@echo Bzipped-Tar-URL: $$(cat README | grep "^Releases  "|sed s/^.*\ :.//g)/"fim-`cat VERSION`".tar.bz2
	@echo License: GPL
	@echo Mirror-Site-URL: $$(cat README | grep "^Off.*rror : "|sed s/^.*\ :.//g)
	@echo ChangeLog-URL: $$(cat README | grep "^ChangeLog  "|sed s/^.*\ :.//g)
	@echo CVS-URL: $$(cat README | grep "^Repository"|sed s/^.*\ :.//g)
	@echo 
	@ $(AWK)  -- 'v=0;($$1 == "Version" && $$2=="'`cat VERSION`'"){while((input=getline) && $$1!="Version"){print}};'  NEWS 

.PHONY: freshmeat
freshmeat:
	@ echo "are you sure you want to freshmeat-submit ? (any key to abort, yes to continue)"  && read yes && [[ "$${yes}" == "yes" ]] && \
	make freshmeat-submit | grep -v '^make\[' | freshmeat-submit

.PHONY: commit
commit: log
	svn commit

# This awaits completion.
#rpm: dist
#        rpmbuild -ta @PACKAGE@-@VERSION@.tar.gz

NEWS: NEWS.in
	grep -v '^#' < $< > $@

README.md: README
	cp $< $@.tmp;
	$(GREP) '^	[0-9][.]\?[0-9]\?		' README | while read; do \
		ID=$${REPLY/ - /}; \
		ID=$${ID//, /-}; \
		ID=$${ID// /-}; \
		ID=$${ID//,/}; \
		ID=$${ID,,}; \
		ID=$${ID// /}; \
		ID=$${ID//	/}; \
		ID=$${ID//(/}; \
		ID=$${ID//)/}; \
		ID=$${ID//:/}; \
		ID=$${ID//./}; \
		ID=$${ID//\//}; \
		$(GREP) "^$${REPLY/ - /}" README -C0 | $(SED) "s/\(^$${REPLY/ - /}.*\)/## <a id=\"$${ID}\"><\/a>\1 ##/g;s/$${REPLY}/[$${REPLY/ - /}]\(#$${ID}\)/g;" -i $@.tmp; done;
	$(SED) -i "s|doc.fim.man.html|$(WWW)/\#man_fim|g" $@.tmp;
	$(SED) -i "s|doc.fimrc.man.html|$(WWW)/\#man_fimrc|g" $@.tmp;
	$(SED) -i "s|^-===\+-$$||g" $@.tmp;
	$(SED) -i "s|^		FIM|FIM|g" $@.tmp;
	$(SED) -i "s|^================================================================================|--------------------------------------------------------------------------------|g" $@.tmp;
	$(GREP) -v '^\(.vim:\|..Id\)' $@.tmp > $@;
	rm $@.tmp

