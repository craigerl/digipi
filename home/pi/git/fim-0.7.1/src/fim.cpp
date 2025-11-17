/* $LastChangedDate: 2024-04-29 14:03:11 +0200 (Mon, 29 Apr 2024) $ */
/*
 fim.cpp : Fim main program and accessory functions

 (c) 2007-2024 Michele Martone

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "fim.h"
#include <signal.h>
#include <getopt.h>
#ifdef FIM_READLINE_H
#include "readline.h"	/* readline stuff */
#endif /* FIM_READLINE_H */

/*
 * (nearly) all Fim stuff is in the fim namespace.
 * */
namespace fim
{
	/*
	 * Globals : should be encapsulated.
	 * */
	static fim::string g_fim_output_device;

	bool is_using_output_device(const fim::string & ds)
	{
		return (0==g_fim_output_device.find(ds));
	}
#if FIM_WITH_DEBUG  /* poor man's debugging mechanism */
	ssize_t g_allocs_bytes{0};
	ssize_t g_allocs_n{0};
	std::map<void*,size_t> g_allocs;
#endif
	fim_char_t g_sc = '\t'; /* separation character for --load-image-descriptions-file */
	fim::CommandConsole cc;
	fim_char_t *default_fbdev=FIM_NULL,*default_fbmode=FIM_NULL;
	int default_vt=-1;
	fim_float_t default_fbgamma=-1.0;
	fim_stream cout/*(1)*/;
	fim_stream cerr(2);
#if FIM_WANT_NEXT_ACCEL
	fim_int same_keypress_repeats=0;
#endif /* FIM_WANT_NEXT_ACCEL */
}

struct fim_options_t{
  const fim_char_t *name;
  int has_arg;
  int *flag;
  int val;
  const fim_char_t *desc;/* this is fim specific */
  const fim_char_t *optdesc;/* this is fim specific */
  const fim_char_t *mandesc;/* this is fim specific */
  char cVal(void)const{return static_cast<char>(val);}
};

struct fim_options_t fim_options[] = {
    {"autozoom",   no_argument,       FIM_NULL, 'a',
	"scale according to a best fit",FIM_NULL,
	"Enable autozoom. " "Automagically pick a reasonable zoom factor when displaying a new image (as in " FIM_MAN_fB("fbi") ")."
    },
    {FIM_OSW_BINARY,     optional_argument,       FIM_NULL, 'b',
	"view any file as either a 1 or 24 bpp bitmap", "24|1",
	"Display contents of binary files (of any filetype) as these were raw 24 or 1 bits per pixel pixelmaps.\n" 
	"The width of this image will not exceed the value of the " FIM_MAN_fB(FIM_VID_PREFERRED_RENDERING_WIDTH) " variable.\n"
	"Regard this as an easter bunny option.\n"
#if !FIM_WANT_RAW_BITS_RENDERING
	// "(disabled)"
#endif /* FIM_WANT_RAW_BITS_RENDERING */
    },
    {FIM_OSW_TEXT,     no_argument,       FIM_NULL, 0x74657874,
	"view any file as rendered text characters", FIM_NULL,
	"Display contents of files (of any filetype) as these were text.\n" 
	"The width of this image will not exceed the value of the " FIM_MAN_fB(FIM_VID_PREFERRED_RENDERING_WIDTH) " variable.\n"
	"Non-printable characters are then displayed as \"" FIM_SYM_UNKNOWN_STRING "\".\n"
	"Regard this as another easter bunny option.\n"
#if !FIM_WANT_RAW_BITS_RENDERING
	// "(disabled)"
#endif /* FIM_WANT_RAW_BITS_RENDERING */
    },
    {"cd-and-readdir", no_argument,       FIM_NULL, 0x4352,
	    "step in the directory of first file and push other files in that directory", FIM_NULL,
	    "Step in the directory of the first file to be loaded, push other files from that directory, and jump back to the first file."
	    FIM_CNS_CMDSEP "Useful when invoking from a desktop environment."
    },
    {FIM_OSW_EXECUTE_COMMANDS, required_argument,       FIM_NULL, 'c',
	"execute " FIM_CNS_EX_CMDS_STRING " after initialization", "" FIM_CNS_EX_CMDS_STRING "",
	"Execute " FIM_CNS_EX_CMDS_STRING " after reading the initialization file, just before entering the interactive mode.\n"
	"No semicolon (" FIM_MAN_fB(";") ") is required at the end of " FIM_CNS_EX_CMDS_STRING ".\n"
	"Do not forget quoting " FIM_CNS_EX_CMDS_STRING " in a manner suitable to your shell.\n"
	"So  -c next  is fine as it is.\n"
	"A more complicated example, with quotings:\n"
	"-c '*2;2pan_up;display;while(1){align \"bottom\";sleep \"1\" ; align \"top\"}'\n"
	"(with the single quotes) tells fim to: double the displayed image \n"
	"size, pan twice up, display the image, and finally \n"
	"do an endless loop consisting of bottom and top aligning, alternated.\n"
    },
    {FIM_OSW_EXECUTE_COMMANDS_EARLY, required_argument,       FIM_NULL, 'C',
	"execute " FIM_CNS_EX_CMDS_STRING " just after initialization", "" FIM_CNS_EX_CMDS_STRING "",
	"Similar to the " FIM_MAN_fB("--" FIM_OSW_EXECUTE_COMMANDS) " option, but "
	"execute " FIM_CNS_EX_CMDS_STRING " earlier, just before reading the initialization file.\n"
	"If " FIM_CNS_EX_CMDS_STRING " takes the special 'early' form " FIM_ARG_MAN_SYNTAX("=var=val") ", it assigns value " FIM_ARG_MAN_SYNTAX("val") " to variable " FIM_ARG_MAN_SYNTAX("var") " immediately, before the interpreter is started, and with no value quoting needed.\n"
	"\n"
	"For example,\n"
	"-C '" FIM_MAN_fB(FIM_VID_SCALE_STYLE) "=\" \"' starts fim no auto-scaling;\n"
	"the equivalent early form is:\n"
	"-C '=" FIM_MAN_fB(FIM_VID_SCALE_STYLE) "= '.\n"
	"\n"
    },
    {"device",     required_argument, FIM_NULL, 'd',
	"specify a framebuffer device " FIM_CNS_EX_FBDEV_STRING, FIM_CNS_EX_FBDEV_STRING,
	"Framebuffer device to use. Default is the one your vc is mapped to (as in fbi)."
    },
    {"dump-reference-help",      optional_argument /*no_argument*/,       FIM_NULL, 0x6472690a,
	"dump reference info","man",
	"Dump to stdout language reference help and quit."
    },
    {"dump-default-fimrc",      no_argument,       FIM_NULL, 0x64646672,
	"dump to standard output the default configuration", FIM_NULL,/*Before r1001,-D*/
	"Dump default configuration (the one hardcoded in the fim executable) to standard output and quit."
    },
    {FIM_OSW_EXECUTE_SCRIPT,   required_argument,       FIM_NULL, 'E',
	"execute " FIM_CNS_EX_SCRIPTFILE " after initialization", "" FIM_CNS_EX_SCRIPTFILE "",
	"Execute " FIM_CNS_EX_SCRIPTFILE " after the default initialization file is read, and before executing " FIM_MAN_fB("--" FIM_OSW_EXECUTE_COMMANDS) " commands."
    },
    {FIM_OSW_ETC_FIMRC,       required_argument, FIM_NULL, 'f',
	"specify custom initialization file", FIM_CNS_EX_FIMRC_STRING,
	"Specify an alternative system-wide initialization file (default: " FIM_CNS_SYS_RC_FILEPATH "), to be read prior to any other configuration file. "
	"See also " FIM_MAN_fB("--" FIM_OSW_NO_ETC_FIMRC) ". \n"
    },
    {FIM_OSW_FINAL_COMMANDS,   required_argument,       FIM_NULL, 'F',
	"execute " FIM_CNS_EX_CMDS_STRING " just before exit", "" FIM_CNS_EX_CMDS_STRING "",
	"Similar to the " FIM_MAN_fB("--" FIM_OSW_EXECUTE_COMMANDS) " option, but "
	"execute " FIM_CNS_EX_CMDS_STRING " after exiting the interactive mode, just before terminating the program."
    },
    {"help",       optional_argument,       FIM_NULL, 'h',
	"print program invocation help and exit"
		    ,FIM_HELP_EXTRA_OPTS,
	"Print program invocation help, and exit. Depending on the option, output can be: short, descriptive, long from man, or complete man."
#if FIM_WANT_HELP_ARGS
	    " For each further argument " FIM_CNS_EX_HELP_ITEM " passed to " FIM_MAN_fB("fim") ", an individual help message is shown."
	    " If " FIM_CNS_EX_HELP_ITEM " starts with a /, it is treated as a search string (not a regexp, though)."
#endif /* FIM_WANT_HELP_ARGS */
    },
#if FIM_WANT_CMDLINE_KEYPRESS
    { FIM_OSW_KEYSYM_PRESS,   required_argument,       FIM_NULL, 'k',
	"execute any command bound to keysym at startup", FIM_CNS_EX_KSY_STRING,
       	"Execute any command bound (via the " FIM_FLT_BIND " command) to a specified keysym at startup. A keysym can be prefixed by a repetition count number. You can specify the option multiple times to simulate multiple keystrokes. "
	"Presses entered via " FIM_MAN_fB("--" FIM_OSW_KEYSYM_PRESS) " are processed with the same priority as those entered via " FIM_MAN_fB("--" FIM_OSW_CHARS_PRESS) ", that is, as they appear. "
	"See man " FIM_MAN_fR("fimrc") "(5) for a list of keysyms and the use of " FIM_FLT_BIND "."
    },
    { FIM_OSW_CHARS_PRESS,   required_argument,       FIM_NULL, 'K',
	"input keyboard characters at startup",FIM_CNS_EX_CHARS_STRING,
       	"Input one or more keyboard characters at program startup (simulate keyboard presses). "
	"This option can be specified multiple times. "
	"Each additional time (or if the string is empty), a press of " FIM_KBD_ENTER " (ASCII code " FIM_XSTRINGIFY(FIM_SYM_ENTER) ") key is prepended. "
	"Examples: -K '' simulates press of an " FIM_KBD_ENTER "; "
	" -K ':next;' activates the command line and enter \"next;\" without executing it; "
	" -K \":next;\" -K \"next\" executes \"next\", stays in the command line and enter keys \"next\"; "
	" -K \":next;\" -K \"\" -K \"next\" executes \"next\", leaves the command line, and executes in sequence any command bound to keys 'n', 'e', 'x', 't'. "
	"Presses entered via " FIM_MAN_fB("--" FIM_OSW_CHARS_PRESS) " are processed with the same priority as those entered via " FIM_MAN_fB("--" FIM_OSW_KEYSYM_PRESS) ", that is, as they appear. "
    },
#endif /* FIM_WANT_CMDLINE_KEYPRESS */
#if FIM_WANT_PIC_CMTS
    {FIM_OSW_LOAD_IMG_DSC_FILE,       required_argument,       FIM_NULL, /*0x6c696466*/'D',
	"load image descriptions file", FIM_CNS_EX_FN_STRING,
	"Load image descriptions from file " FIM_CNS_EX_FN_STRING ". Each line begins with the basename of an image file, followed by a Tab character (or a different character if specified via " FIM_MAN_fB("--" FIM_OSW_IMG_DSC_FILE_SEPC) "), then the description text. "
	"The description text is copied into the " FIM_MAN_fB("i:" FIM_VID_COMMENT) " variable of the image at load time, overriding the comment possibly loaded from the file (e.g. JPEG, PNG or TIFF comment)."
#if FIM_WANT_DESC_VEXP
	" If a '@' followed by an identifier " FIM_CNS_EX_ID_STRING " is encountered, and i:" FIM_CNS_EX_VAR_STRING " is set, its value is substituted here. If \"@#\" is encountered, the remainder of the description line is ignored." 
#endif /* FIM_WANT_DESC_VEXP */
#if FIM_WANT_PIC_LVDN
      " Special comment lines like \"#!fim:" FIM_CNS_EX_VAR_STRING "=" FIM_CNS_EX_VAL_STRING "\" lead i:" FIM_CNS_EX_VAR_STRING " to be assigned value " FIM_CNS_EX_VAL_STRING " (unquoted) at image loading time (cached variable), unless " FIM_CNS_EX_VAR_STRING " starts with an underscore ('_')."
      " Special comment lines like \"#!fim:@" FIM_CNS_EX_VAR_STRING "=" FIM_CNS_EX_VAL_STRING "\" create a variable " FIM_CNS_EX_VAR_STRING " that are only valid in expanding @" FIM_CNS_EX_VAR_STRING " in comments."
#if FIM_WANT_DESC_VEXP
      " Special comment lines like \"#!fim:" FIM_CNS_EX_VAR_STRING "@=" FIM_CNS_EX_VAL_STRING "\" or \"#!fim:@" FIM_CNS_EX_VAR_STRING "@=" FIM_CNS_EX_VAL_STRING " (notice @ before =) also expand whatever @" FIM_CNS_EX_ID_STRING " encountered in " FIM_CNS_EX_VAL_STRING " ."
#endif /* FIM_WANT_DESC_VEXP */
      " Special comment lines like \"#!fim:+=" FIM_CNS_EX_VAL_STRING "\" add " FIM_CNS_EX_VAL_STRING " to current description." 
      " Special comment lines like \"#!fim:^=" FIM_CNS_EX_VAL_STRING "\" set " FIM_CNS_EX_VAL_STRING " to be the base of each description." 
      " Special comment lines like \"#!fim:!=\" reset all cached variables." 
      " Special comment lines like \"#!fim:/=" FIM_CNS_EX_DIR_STRING "\" prepend " FIM_CNS_EX_DIR_STRING " to each file's basename." 
      " Special comment lines like \"#!fim:\\e=" FIM_CNS_EX_DIR_STRING "\" prepend " FIM_CNS_EX_DIR_STRING " to each file's name."  /* '\e' stays for '\ ' in man pages */
#if FIM_WANT_PIC_RCMT 
      " Special description text (to be associated to an image) begins with markers: "
      " with \"#!fim:=\", the last description line is reused;"
      " with \"#!fim:+\", what follows is appended to the last description line;"
      " with \"#!fim:^\", what follows is prepended to the last description line;"
      " with \"#!fim:s/" FIM_CNS_EX_F "/" FIM_CNS_EX_T "\", the last description line is used and substituted substring " FIM_CNS_EX_T " to occurrences of substring " FIM_CNS_EX_F " (" FIM_CNS_EX_F " and " FIM_CNS_EX_T " cannot contain newlines or a '/')."
#endif /* FIM_WANT_PIC_RCMT  */
      " If " FIM_CNS_EX_VAL_STRING " is empty that variable is unset."
#if FIM_WANT_PIC_LBFL
      " These variables are stored also in an internal index used by the " FIM_FLT_LIMIT " command."
#endif /* FIM_WANT_PIC_LBFL */
#endif /* FIM_WANT_PIC_LVDN */
      " This option sets " FIM_MAN_fB(FIM_VID_COMMENT_OI) "=" FIM_XSTRINGIFY(FIM_OSW_LOAD_IMG_DSC_FILE_VID_COMMENT_OI_VAL) ", so that a caption is displayed over the image."
      " A description file beginning with \"" FIM_CNS_MAGIC_DESC "\" can be loaded without specifying this switch."
    },
    {FIM_OSW_IMG_DSC_FILE_SEPC,       required_argument,       FIM_NULL, /*0x69646673*/'S',
	    "image descriptions file separator character", FIM_CNS_EX_SEPCHAR,
	    "A character to be used as a separator between the filename and the description part of lines specified just before a --" FIM_OSW_LOAD_IMG_DSC_FILE "."
    },
#endif /* FIM_WANT_PIC_CMTS */
//#ifdef FIM_READ_STDIN_IMAGE
    {FIM_OSW_IMAGE_FROM_STDIN,      no_argument,       FIM_NULL, 'i',
	"read an image file from standard input", FIM_NULL,
	"Read one single image from the standard input (the image data, not the filename).  May not work with all supported file formats."
	"\nIn the image list, this image takes the special name \"" FIM_STDIN_IMAGE_NAME "\".\n"
    },
//#endif /* FIM_READ_STDIN_IMAGE */
#if FIM_WANT_PIC_CMTS
    {"mark-from-image-descriptions-file",       required_argument,       FIM_NULL, 0x6d666466,
	    "mark files from descriptionos list file", FIM_CNS_EX_FN_STRING,
	    "Set those files specified in " FIM_CNS_EX_FN_STRING " (see --" FIM_OSW_LOAD_IMG_DSC_FILE " for the file format) as marked (see the " FIM_FLT_LIST " command).\n"
    },
#endif /* FIM_WANT_PIC_CMTS */
    {"mode",       required_argument, FIM_NULL, 'm',
	"specify a video mode", FIM_CNS_EX_VMODE_STRING,
	"Name of the video mode to use video mode (must be listed in /etc/fb.modes).  Default is not to change the video mode.  In the past, the XF86 config file (/etc/X11/XF86Config) used to contain Modeline information, which could be fed to the modeline2fb perl script (distributed with fbset).  On many modern xorg based systems, there is no direct way to obtain a fb.modes file from the xorg.conf file.  So instead one could obtain useful fb.modes info by using the (fbmodes (no man page AFAIK)) tool, written by bisqwit.  An unsupported mode should make fim exit with failure.  But it is possible the kernel could trick fim and set a supported mode automatically, thus ignoring the user set mode."
    },
    {"no-rc-file",      no_argument,       FIM_NULL, 'N',
	"do not read the personal initialization file at startup", FIM_NULL,
	"No personal initialization file is read (default is " FIM_CNS_USR_RC_COMPLETE_FILEPATH ") at startup."
    },
    {FIM_OSW_NO_ETC_FIMRC,      no_argument,       FIM_NULL, 0x4E4E,
	"do not read the system-wide initialization file at startup", FIM_NULL,
	"No system-wide initialization file is read (default is " FIM_CNS_SYS_RC_FILEPATH ") at startup. "
	"See also " FIM_MAN_fB("--etc-fimrc") ". \n"
    },
    {"no-internal-config",      no_argument,       FIM_NULL, 0x4E4E4E,
	"do not read the internal default configuration at startup"
	,FIM_NULL,
	"No internal default configuration at startup (uses internal variable " FIM_MAN_fB(FIM_VID_NO_DEFAULT_CONFIGURATION) "). Will only provide a minimal working configuration. "
    },
    {"no-commandline",      no_argument,       FIM_NULL, 0x4E434C,
	"disable internal command line", FIM_NULL,
	"With internal command line mode disabled."
    },
#if FIM_WANT_HISTORY
    {"no-history-save",      no_argument,       FIM_NULL, 0x4E4853,
	"do not save execution history", FIM_NULL,
	"Do not save execution history at finalization (uses internal variable " FIM_MAN_fB(FIM_VID_SAVE_FIM_HISTORY) "). "
    },
    {"no-history-load",      no_argument,       FIM_NULL, 0x4E484C,
	"do not load execution history", FIM_NULL,
	"Do not load execution history at startup. "
    },
    {"no-history",      no_argument,       FIM_NULL, 0x4E48,
	"do not load/save execution history", FIM_NULL,
	"Do not load or save execution history at startup. "
    },
#endif /* FIM_WANT_HISTORY */
    {FIM_OSW_SCRIPT_FROM_STDIN,      no_argument,       FIM_NULL, 'p',
	"read commands from standard input", FIM_NULL,
	"Read commands from stdin before entering in interactive mode."
    },
    {FIM_OSW_OUTPUT_DEVICE,      required_argument,       FIM_NULL, 'o',
	"specify the desired output driver (aka graphic mode) ", FIM_DDN_VARS,
"Use the specified \\fBdevice\\fP (one among " FIM_DDN_VARS_NB ") as fim video output device, overriding automatic checks.\n"
"If the \\fBdevice\\fP is empty and followed by " FIM_CNS_EX_GOPTS_STRING ", it will be selected automatically.\n"
"The available devices depend on the current environment and on the configuration and compilation options.\n"
"You can get the list of available output devices issuing \\fBfim --version\\fP.\n"
"The possible values to " FIM_CNS_EX_GOPTS_STRING " that we describe here can also be passed as \"display 'reinit' " FIM_CNS_EX_GOPTS_STRING "\" -- see man " FIM_MAN_fR("fimrc") " for this.\n"
"The device name with options (perhaps with modifications due to auto-detection) is stored in variable " FIM_MAN_fB(FIM_VID_DEVICE_DRIVER) ".\n"
// #ifndef FIM_WITH_NO_FRAMEBUFFER
"The " FIM_MAN_fB(FIM_DDN_INN_FB) " option selects the Linux framebuffer. Presence of " FIM_CNS_EX_GOPTS_STRING " with value " FIM_MAN_fB("'S'") " (e.g. '" FIM_MAN_fB("fb=S") "') makes framebuffer initialization more picky: it does not tolerate running in a screen session.\n"
// #endif /* FIM_WITH_NO_FRAMEBUFFER */
//#ifdef FIM_WITH_LIBCACA
"The " FIM_MAN_fB(FIM_DDN_INN_CACA) " option (coloured ASCII-art) can be specified as " FIM_MAN_fB(FIM_DDN_INN_CACA) FIM_MAN_fB("[" FIM_SYM_DEVOPTS_SEP_STR "{['w']['h']['H']['d:'DITHERMODE]}] ") "; if supplied, " FIM_MAN_fB("'w'") " selects windowed mode, provided libcaca is running under X; by default (or with " FIM_MAN_fB("'W'") "), windowed mode is being turned off internally during initialization by unsetting the DISPLAY environment variable." " If " FIM_MAN_fB("'d:'") " is present, the " FIM_MAN_fB("DITHERMODE") " following it will be passed as dither algorithm string (alternatively, it can be a non-negative number, too).\n"
//#endif /* FIM_WITH_LIBCACA */
//#ifdef FIM_WITH_AALIB
"The " FIM_MAN_fB(FIM_DDN_INN_AA) " (monochrome ASCII-art) option can be specified as " FIM_MAN_fB(FIM_DDN_INN_AA "[" FIM_SYM_DEVOPTS_SEP_STR "{['w'|'W']}]") "; if supplied, " FIM_MAN_fB("'w'") " selects windowed mode, provided aalib is running under X; by default (or with " FIM_MAN_fB("'W'") "), windowed mode is being turned off internally during initialization by unsetting the DISPLAY environment variable.\n"
//#endif /* FIM_WITH_AALIB */
"Please note that the readline (internal command line) functionality in " FIM_MAN_fB(FIM_DDN_INN_CACA) " and " FIM_MAN_fB(FIM_DDN_INN_AA) " modes is limited.\n"
#if defined(FIM_WANT_SDL_OPTIONS_STRING) || defined(FIM_WITH_LIBGTK)
"If the graphical windowed mode is "
#if FIM_WANT_SDL_OPTIONS_STRING
" \\fBsdl\\fP "
#if FIM_WITH_LIBGTK
" or "
#endif
#endif
#if FIM_WITH_LIBGTK
" \\fBgtk\\fP "
#endif
" it can be followed by \\fB " FIM_MAN_iB FIM_SYM_DEVOPTS_SEP_STR "{['w']['m']['r']['h']['W']['M']['R']['H'][width[:height]]['%']}\\fP,"
" where " FIM_MAN_fB("width") " and " FIM_MAN_fB("height") " are integer numbers specifying the desired resolution "
" (if " FIM_MAN_fB("height") " not specified, it takes the value of " FIM_MAN_fB("width") ");"
" the " FIM_MAN_fB("'w'") " character requests windowed mode (instead of " FIM_MAN_fB("'W'") " for fullscreen);"
" the " FIM_MAN_fB("'m'") " character requests mouse pointer display;"
" the " FIM_MAN_fB("'h'") " character requests help grid map draw (can be repeated for variants);"
" the " FIM_MAN_fB("'r'") " character requests support for window resize;"
" the " FIM_MAN_fB("'%'") " character requests to treat " FIM_MAN_fB("width") " and " FIM_MAN_fB("height")
" as percentage of possible window resolution."
" The same letters uppercase request explicit negation of the mentioned features.\n"
"Additionally, in \\fB" "gtk" "\\fP mode: "
FIM_MAN_fB("'b'") " hides the menu bar, "
FIM_MAN_fB("'e'") " starts with empty menus, "
FIM_MAN_fB("'" FIM_CNS_GTK_MNRBCHR_STR "'") " rebuilds the menus, and "
FIM_MAN_fB("'" FIM_CNS_GTK_MNDLCHR_STR "'") " removes the menus."
" " FIM_MAN_fB("Note") ": the \\fB" "gtk" "\\fP mode is a recent addition and may have defects."
#endif /* FIM_WANT_SDL_OPTIONS_STRING FIM_WITH_LIBGTK */
//#ifdef FIM_WITH_LIBIMLIB2
" The " FIM_MAN_fB(FIM_DDN_INN_SDL) " mode works best with libsdl-2; libsdl-1.2 support is being discontinued.\n"
" The " FIM_MAN_fB(FIM_DDN_INN_IL2) " option requests imlib2 and is unfinished: do not use it.\n"
" The " FIM_MAN_fB(FIM_DDN_INN_DUMB) " test mode is there only for test purposes and is not interactive.\n"
//#endif /* FIM_WITH_LIBIMLIB2 */
    },
    {"offset",      required_argument,       FIM_NULL,  0x6f66660a,
	"open at specified byte offset",
	FIM_ARG_MAN_SYNTAX("{bytes-offset[{:upper-offset}|{+offset-range}]}"),
"Use the specified " FIM_ARG_MAN_SYNTAX("offset") " (in bytes) for opening the specified files. If " FIM_ARG_MAN_SYNTAX(":upper-offset") " is specified, further bytes until " FIM_ARG_MAN_SYNTAX("upper-offset") " are probed. If " FIM_ARG_MAN_SYNTAX("+offset-range") " is specified instead, that many additional bytes are to be probed.  Use this option to search damaged file systems for image files. Appending a modifier among 'K','M','G' (case irrelevant) to an offset number changes the unit to be respectively 2^10, 2^20, or 2^30 bytes."
    },
    {"pread-cmd",      required_argument,       FIM_NULL,  0x70726561, //prea(d)
	"read image file in stdin from pipeline",
	FIM_CNS_EX_CMDFLTPL,
	"Specify a shell command with " FIM_CNS_EX_CMDFLTPL ". If the current filename matches \"" FIM_CNS_PIPEABLE_PATH_RE "\", it is be substituted to any occurrence of '{}'. The resulting command output is assumed to be file data, which is read, decoded, and displayed.\n"
	"This works by setting the internal " FIM_MAN_fB(FIM_VID_PREAD) " variable (empty by default)."
    },
    {"text-reading",      no_argument,       FIM_NULL, 'P',
	"enable space-based scrolling through the document", FIM_NULL,
	"Enable textreading mode.  This has the effect that fim displays images scaled to the width of the screen, and aligned to the top.  If the images you are watching are text pages, all you have to do to get the next piece of text is to press space (in the default key configuration, of course)."
    },
    {"scroll",     required_argument, FIM_NULL, 's',
	"set scroll variable value", FIM_CNS_EX_VALUE,
	"Set scroll steps for internal variable " FIM_MAN_fB(FIM_VID_STEPS) " (default is \"" FIM_CNS_STEPS_DEFAULT "\")."
    },
    {"slideshow",     required_argument, FIM_NULL, 0x7373,
	"interruptible slideshow mode", FIM_CNS_EX_NUM_STRING,
	"Interruptible slideshow mode. "
	"Wait for " FIM_CNS_EX_NUM_STRING " of seconds (can have a decimal part, and is assigned to the " FIM_VID_WANT_SLEEPS " variable) after each image. "
	"Implemented by executing " FIM_CNS_SLIDESHOW_CMD " as a first command. "
	"Can be interrupted by " FIM_KBD_COLON " or " FIM_KBD_ESC ". "
	"The other keys execute accordingly to their function but do not interrupt the slideshow. "
	"Like in fbi, this cycles forever, unless " FIM_MAN_fB("--once") " is specified."
    },
    {"sanity-check",      no_argument,       FIM_NULL, 0x70617363,
	"only perform a sanity check", FIM_NULL, /* Was -S until r1001 */
	"Perform a quick sanity check, just after the initialization, and terminate."
    },	/* NEW */
    {"no-framebuffer",      no_argument,       FIM_NULL, 't',
	"display images in text mode (as -o " FIM_DDN_INN_AA ")",FIM_NULL,
	FIM_MAN_fB("fim") " Use an ASCII Art driver. If present, use either of libcaca (coloured), or aalib (monochrome). For more, see (man fimrc), (info aalib) or (apropos caca)).\n"
	"If no ASCII Art driver had been enabled at compile time, fim does not display any image at all."
    },
    {"vt",         required_argument, FIM_NULL, 'T',
	"specify a virtual terminal for the framebufer", FIM_CNS_EX_TERMINAL,
	"The " FIM_CNS_EX_TERMINAL " is to be used as virtual terminal device file (as in fbi).\n"
	"See (chvt (1)), (openvt (1)) for more info about this.\n"
	"Use (con2fb (1)) to map a terminal to a framebuffer device.\n"
    },
    {"reverse",     no_argument,       FIM_NULL, 0x7772666c,
	"reverse images list", FIM_NULL,
	"Reverse files list before browsing (can be combined with the other sorting options)."
    },
    {"sort",     no_argument,       FIM_NULL, 0x736f7274,
	"sort images by pathname", FIM_NULL,
	"Sort files list before browsing according to full filename."
    },
    {"sort-basename",     no_argument,       FIM_NULL, 0x736f626e,
	"sort images by basename", FIM_NULL,
	"Sort files list before browsing according to file basename's."
    },
//#if FIM_WANT_SORT_BY_STAT_INFO
    {FIM_OSW_SORT_MTIME,     no_argument,       FIM_NULL, 0x7369626d, 
	"sort images by modification time",
	FIM_NULL,
	"Sort files list before browsing according to file modification time."
    },
    {FIM_OSW_SORT_FSIZE,     no_argument,       FIM_NULL, 0x73696273,
	"sort images by file size",FIM_NULL,
	"Sort files list before browsing according to file size."
    },
//#endif /* FIM_WANT_SORT_BY_STAT_INFO */
    {"random",     no_argument,       FIM_NULL, 'u',
	"randomize images order", FIM_NULL,
"Randomly shuffle the files list before browsing (seed depending on time() function)."
    },
    {"random-no-seed",     no_argument,       FIM_NULL, 0x7073,
	"randomize images order",FIM_NULL,
"Pseudo-random shuffle the files list before browsing (no seeding)."
    },
    {"verbose",    no_argument,       FIM_NULL, 'v',
	"be verbose", FIM_NULL,
	"Be verbose: show status bar."
    },
    {"verbose-load",    no_argument,       FIM_NULL, 0x766c,
	"verbose file loading", FIM_NULL,
	"Load files verbosely (repeat option to increase verbosity)."
    },
    {"verbose-font-load",    no_argument,       FIM_NULL, 0x7666,
	"verbose font loading", FIM_NULL,
"Load font verbosely (sets " FIM_MAN_fB(FIM_VID_FB_VERBOSITY) ")." /* (repeat option to increase verbosity)." */
    },
    {"verbose-interpreter",    no_argument,       FIM_NULL, 0x76696d0a,
	"verbose interpreter", FIM_NULL,
	"Execute interpreter verbosely (Sets immediately " FIM_MAN_fB(FIM_VID_DBG_COMMANDS) "=\"" FIM_CNS_DBG_CMDS_MID "\" if specified once, " FIM_MAN_fB(FIM_VID_DBG_COMMANDS) "=\"" FIM_CNS_DBG_CMDS_MAX "\" if specified  twice)."
    },
    {"version",    no_argument,       FIM_NULL, 'V',
	"print program version", FIM_NULL,
	"Print to stdout program version, compile flags, enabled features, linked libraries information, supported filetypes/file loaders, and then exit."
    },
    {"autowidth",   no_argument,       FIM_NULL, 'w',
	"scale fitting to screen width",FIM_NULL,
	"Scale the image according to the screen width."
    },
    {"no-auto-scale",   no_argument,   FIM_NULL, '=' /*0x4E4053*/,
	"do not use any auto-scaling", FIM_NULL,
	"Do not scale the images after loading (sets '" FIM_MAN_fB(FIM_VID_SCALE_STYLE) "=\" \"';)."
    },
    {"autowindow",   no_argument,   FIM_NULL, 0x61757769,
	"adapt window to image size",FIM_NULL,
	"Resize the window size (if supported by the video mode) to the image size. Don't use this with other image scaling options."
    },
    {FIM_OSW_NO_STAT_PUSH,   no_argument,   FIM_NULL, 0x6e7363,
	"do not check file/dir existence with stat(2) at push time", FIM_NULL,
	"Sets " FIM_MAN_fB(FIM_VID_PRELOAD_CHECKS) "=0 before initialization, thus disabling file/dir existence checks with stat(2) at push push time (and speeding up startup)."
    },
    {"autoheight",   no_argument,       FIM_NULL, 'H',
	"scale according to height", FIM_NULL,
	"Scale the image according to the screen height."
    },
    {FIM_OSW_DUMP_SCRIPTOUT,      required_argument,       FIM_NULL, 'W',
	"record any executed command to the a " FIM_CNS_EX_SCRIPTFILE "", "" FIM_CNS_EX_SCRIPTFILE "",
	"All the characters that you type are recorded in the file " FIM_CNS_EX_SCRIPTFILE ", until you exit " FIM_MAN_fB("fim") ".  This is useful if you want to create a script file to be used with \"fim -c\" or \":exec\" (analogous to Vim's -s and \":source!\").  If the " FIM_CNS_EX_SCRIPTFILE " file exists, it is not touched (as in Vim's -w). "
    },
    {"read-from-file",      required_argument,       FIM_NULL, 'L',
	"read an image list from a file", FIM_ARG_MAN_SYNTAX("{fileslistfile}"),
	"Read file list from a file: each line one file to load (similar to " FIM_MAN_fB("--read-from-stdin") "; use " FIM_MAN_fB("--read-from-stdin-elds") " to control line breaking).\n"
"\n"
    },
#ifdef FIM_READ_STDIN
    {FIM_OSW_READ_FROM_STDIN,      no_argument,       FIM_NULL, '-',
	"read an image list from standard input", FIM_NULL,
	"Read file list from stdin: each line one file to load; use with --read-from-stdin-elds to control line breaking).\n"

"\n"
"Note that these three standard input reading functionalities (-i,-p and -) conflict : if two or more of them occur in fim invocation, fim exits with an error and warn about the ambiguity.\n"
"\n"
"See the section\n"
".B INVOCATION EXAMPLES\n"
"below to read some useful (and unique) ways of employing fim.\n"
    },
#endif /* FIM_READ_STDIN */
    {"read-from-stdin-elds",      required_argument,       FIM_NULL, 0x72667373,
	"endline delimiter character for --read-from-stdin/--read-from-file", FIM_ARG_MAN_SYNTAX("{delimiter-char}"),
	"Specify an endline delimiter character for breaking lines read via -/--read-from-stdin/--read-from-file (which shall be specified after this). Line text before the delimiter are be treated as names of files to load; the text after is ignored. This is also useful e.g. to load description files (see --" FIM_OSW_LOAD_IMG_DSC_FILE ") as filename list files. Default is the newline character (0x0A)"
#ifdef HAVE_GETDELIM
	"; to specify an ASCII NUL byte (0x00) use ''"
#endif /* HAVE_GETDELIM */
	".\n"
    },
    {"autotop",   no_argument,       FIM_NULL, 'A' ,
	    "align image to top border", FIM_NULL,
	    "Align images to the top border (by setting '" FIM_MAN_fB(FIM_VID_AUTOTOP) "=1' after initialization)."
    },
//    {"gamma",      required_argument, FIM_NULL, 'g',"set gamma (UNFINISHED)","{gamma}",
//" gamma correction.  Can also be put into the FBGAMMA environment variable.  Default is 1.0.  Requires Pseudocolor or Directcolor visual, doesn't work for Truecolor."
//    },
    {"quiet",      no_argument,       FIM_NULL, 'q',
	"quiet execution mode", FIM_NULL,
	"Quiet execution mode. Sets " FIM_CNS_QUIET_CMD ".\n"
    },
#if FIM_WANT_R_SWITCH
    {"resolution", required_argument, FIM_NULL, 'r',
	"set resolution in GTK and SDL ", FIM_CNS_EX_WLF,
	"Set resolution specification in pixels dimensions. Supported only by GTK and SDL. Will be appended to the argument to --" FIM_OSW_OUTPUT_DEVICE ". "
	"Shorthand value 'fullscreen' is translated as 'W'. "
    },
#else /* FIM_WANT_R_SWITCH */
    {"resolution", required_argument, FIM_NULL, 'r', "Set resolution (UNFINISHED).", "{width:height}",
	    FIM_NULL
    },
#endif /* FIM_WANT_R_SWITCH */
#if FIM_WANT_RECURSE_FILTER_OPTION
    {FIM_OSW_RECURSIVE, optional_argument, FIM_NULL, 'R',
	"push paths recursively", FIM_CNS_EX_EXP_STRING,
	"Push files/directories to the files list recursively. "
	"The expression in variable " FIM_MAN_fB(FIM_VID_PUSHDIR_RE) " (default: \"" FIM_CNS_PUSHDIR_RE "\") lists extensions of filenames which are loaded in the list. "
	"You can overwrite its value by optionally passing an expression " FIM_CNS_EX_EXP_STRING " here as argument. "
	"If starting with '+' or '|', the expression following is to be appended to it. "
    },
#else /* FIM_WANT_RECURSE_FILTER_OPTION */
    {FIM_OSW_RECURSIVE, no_argument, FIM_NULL, 'R',
	    "push paths recursively", FIM_NULL,
	    "Push files/directories to the files list recursively. See variable " FIM_MAN_fB(FIM_VID_PUSHDIR_RE) " for extensions of filenames which are loaded in the list. "			
    },
#endif /* FIM_WANT_RECURSE_FILTER_OPTION */
#if FIM_WANT_NOEXTPROPIPE
    {FIM_OSW_NOEXTPIPLOA, no_argument, FIM_NULL, 'X',
	"only use built-in file decoders", FIM_NULL,
	"Do not load via external converter programs: only use built-in file decoders."
    },
#endif /* FIM_WANT_NOEXTPROPIPE */
//#if FIM_WANT_BACKGROUND_LOAD
    {FIM_OSW_BGREC, no_argument, FIM_NULL, 'B',
	    "push paths recursively in background", FIM_NULL,
	    "Push files/directories to the files list recursively, in background during program execution. Any sorting options are ignored."
	    " Experimental feature, unfinished."
    },
//#endif /* FIM_WANT_BACKGROUND_LOAD */
//#if FIM_EXPERIMENTAL_SHADOW_DIRS
    {"load-shadow-directory",   required_argument, FIM_NULL, 0x68696768,
	    "add shadow directory for 'scale \"shadow\"'", FIM_CNS_EX_DIRNAME,
	    "Add " FIM_CNS_EX_DIRNAME " to the shadow directory list. Then 'scale \"shadow\"' temporarily substitutes the image being displayed with that of the first same-named file located under a shadow directory. Useful to browse low-res images, but still being able to quickly view the hi-res original residing in a shadow directory. This works as intended as long as unique filenames are involved."
    },
//#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
    { "/",   required_argument,       FIM_NULL, '/',
	    "jump to file name matching pattern", FIM_CNS_EX_PAT_STRING,
	    "After startup jump to pattern; short for -c '" FIM_SYM_FW_SEARCH_KEY_STR "' " FIM_CNS_EX_PAT_STRING "."
    },
    {"//",   required_argument,       FIM_NULL, 0x2f2f0000,
	    "jump to file path matching pattern", FIM_CNS_EX_PAT_STRING, "After startup jump to pattern; as -c '" FIM_SYM_FW_SEARCH_KEY_STR "'" FIM_CNS_EX_PAT_STRING " but with search on the full path (with " FIM_MAN_fB(FIM_SYM_CMD_SLSL) ")."
    },
/*    {"timeout",    required_argument, FIM_NULL, 't',"",FIM_NULL},*/  /* timeout value */	/* fbi's */
    {"once",       no_argument,       FIM_NULL, '1',
	    "if running --slideshow, loop only once", FIM_NULL,
	    "If running --slideshow, loop only once (as in fbi)." },  /* loop only once */ /* fbi's and fim's */
/*    {"font",       required_argument, FIM_NULL, 'f',"",FIM_NULL},*/  /* font */
/*    {"edit",       no_argument,       FIM_NULL, 'e',"",FIM_NULL},*/  /* enable editing */	/* fbi's */
/*    {"list",       required_argument, FIM_NULL, 'l',"",FIM_NULL},*/
//    {"backup",     no_argument,       FIM_NULL, 'b',"",FIM_NULL},	/* fbi's */
//    {"debug",      no_argument,       FIM_NULL, 'D',"",FIM_NULL},
//    {"preserve",   no_argument,       FIM_NULL, 'p',"",FIM_NULL},	/* fbi's */

    /* long-only options */
//    {"autoup",     no_argument,       &autoup,   1 },
//    {"autodown",   no_argument,       &autodown, 1 },
//    {"comments",   no_argument,       &comments, 1 },
    {0,0,0,0,0,0}
};

#if 0
// leftovers from the old man file; shall adapt these using .\"
 .TP
 .B -f font
 Set font.  This can be either a pcf console font file or a X11 font
 spec.  Using X11 fonts requires a font server (The one specified in
 the environment variable FONTSERVER or on localhost).  The FBFONT
 environment variable is used as default.  If unset, fim will
 fallback to 10x20 (X11) / lat1u-16.psf (console).
 .TP
 .B --autoup
 Like autozoom, but scale up only.
 .TP
 .B --autodown
 Like autozoom, but scale down only.
 .TP
 .B -u
 Randomize the order of the filenames.
 .TP
 .B -e
 Enable editing commands.
 .TP
 .B -b
 create backup files (when editing images).
 .TP
 .B -p
 preserve timestamps (when editing images).
 .TP
 .B --comments
 Display comment tags (if present) instead of the filename.  Probaby
 only useful if you added reasonable comments yourself (using wrjpgcom
 for example), otherwise you likely just find texts pointing to the
 software which created the image.
 P               pause the slideshow (if started with -t, toggle)
 {number}g    jump to image {number}
 .SH EDIT IMAGE
 fim also provides some very basic image editing facilities.  You have
 to start fim with the -e switch to use them.
 .P
 .nf
 Shift+D         delete image
 R               rotate 90 degrees clockwise
 L               rotate 90 degrees counter-clock wise
 .fi
 .P
 The delete function actually wants a capital letter 'D', thus you have
 to type Shift+D.  This is done to avoid deleting images by mistake
 because there are no safety bells:  If you ask fim to delete the image,
 it will be deleted without questions asked.
 .P
 The rotate function actually works for JPEG images only because it
 calls the jpegtran command to perform a lossless rotation if the image.
 It is especially useful if you review the images of your digital
 camera.
#endif

FIM_CONSTEXPR int fim_options_count=sizeof(fim_options)/sizeof(fim_options_t);
struct option options[fim_options_count];

fim::string fim_help_opt(const char*qs, const char dl)
{
	std::ostringstream oss;

	if( qs && qs[0] == '-' && !qs[1] )
	{
		oss << "The short command options of fim are: ";
		for(size_t i=0;i<fim_options_count-1;++i)
		if( isascii( fim_options[i].val ) )
		{
			if( fim_options[i].val != '-' )
				oss << "-";
			oss << fim_options[i].cVal() << " ";
		}
		goto ret;
	}

	if( qs && qs[0] == '-' && qs[1] == '-' && !qs[2] )
	{
		oss << "The long command options of fim are: ";
		for(size_t i=0;i<fim_options_count-1;++i)
		if( fim_options[i].name ) 
			oss << "--" << fim_options[i].name << " ";
		goto ret;
	}

	if( !qs || qs[0] != '-' || !qs[1] || (qs[1]!='-' && qs[2]) || (qs[1]=='-' && !qs[2])  )
	{
		goto ret;
	}

	for(size_t i=0;i<fim_options_count-1;++i)
		if(
				( qs[0] == '-' && isascii( fim_options[i].val ) && !qs[2] && qs[1] == fim_options[i].val ) ||
				( qs[1] == '-' && *fim_options[i].name && 0 == strcmp(qs+2,fim_options[i].name) )
		  )
		{
			oss << qs << " is a fim command line option: ";
			if( isascii( fim_options[i].val ) )
			{
				oss << "-" << fim_options[i].cVal();
				if( fim_options[i].optdesc && fim_options[i].has_arg == optional_argument )
			       		oss << "[" << fim_options[i].optdesc << "]";
				if( fim_options[i].optdesc && fim_options[i].has_arg != optional_argument )
			       		oss << " " << fim_options[i].optdesc << "";
				oss << ", ";
			}
			if( fim_options[i].name )
			{
				oss << "--" << fim_options[i].name;
				if( fim_options[i].optdesc && fim_options[i].has_arg == optional_argument )
			       		oss << "[=" << fim_options[i].optdesc << "]";
				if( fim_options[i].optdesc && fim_options[i].has_arg != optional_argument )
			       		oss << "=" << fim_options[i].optdesc << "";
				//oss << " ";
			}

			if (dl == 's')
				; // enough
			if (dl == 'd')
				oss << ": " << fim_options[i].desc;
			if (dl == 'l')
				oss << std::endl << fim_man_to_text(fim_options[i].mandesc);
			if (dl == 'm')
				oss << std::endl << fim_options[i].mandesc;
			goto ret;
		}
		//oss << fim_options[i].cVal() << "\n";
ret:	return oss.str();
}

class FimInstance FIM_FINAL
{
	static void show_version(FILE * stream);

string fim_dump_man_page_snippets(void)
{
	std::ostringstream oss;
	const fim_char_t *helparg="m";
	const fim_char_t *slob;
	const fim_char_t *sloe;
	const fim_char_t *slwb;
	const fim_char_t *slwe;
	const fim_char_t *slol;
	const fim_char_t *slom;
	oss << 
".TP\n"
FIM_MAN_fB("--")"\n"
"Treat arguments after "
FIM_MAN_fB("--")"\n"
"as filenames.\n"
"Treat arguments before "
FIM_MAN_fB("--")"\n"
"as command line options if these begin with\n"
FIM_MAN_fB("-")", \n"
"and as filenames otherwise.\n"
"\n"
;
	if(*helparg=='m')
	{
		slob="--";
		slom="\n";
		sloe="\n";
#if 0
		slol=".TP\n.B "; // entire line bold
		slwb="";
		slwe="";
#else
		slol=".TP\n"; // individual switches bold
		slwb="\\fB";
		slwe="\\fP";
#endif
	}
	else
	{
		slol="\t-";
		slob="\t\t--";
		slom=FIM_CNS_EMPTY_STRING;
		sloe="\n";
		slwb="";
		slwe="";
	}
	for(size_t i=0;i<fim_options_count-1;++i)
	{	
		if(isascii(fim_options[i].val))
		{
	   		if((fim_options[i].val)!='-')
			{
				oss << slol << slwb << "-" << fim_options[i].cVal();
				if( fim_options[i].optdesc && fim_options[i].has_arg == optional_argument )
			       		oss << "[" << fim_options[i].optdesc << "]";
				if( fim_options[i].optdesc && fim_options[i].has_arg == required_argument )
			       		oss << " " << fim_options[i].optdesc << "";
				oss << slwe << ", ";
			}
	 	  	else
			       	oss << slol << slwb << " -, " << slwe ;
		}
		else
		       	//oss << ".TP\n.B \t";
		       	oss << slol;
		oss << slwb << slob << fim_options[i].name;
		switch(fim_options[i].has_arg)
		{
			case no_argument:
			break;
			case required_argument:
			//std::cout << " <arg>";
			if(fim_options[i].optdesc)
			      	oss << " " << fim_options[i].optdesc << "";
			else
			       	oss << " <arg>";
			break;
			case optional_argument:
			if(fim_options[i].optdesc)
			       	oss << "[=" << fim_options[i].optdesc << "]";
			else
			       	oss << "[=" << "arg" << "]";
			break;
			default:
			;
		}
		oss << slwe;
		oss << slom;
		if(helparg&&*helparg=='d')
			oss << "\t\t " << fim_options[i].desc;
		if(helparg&&*helparg=='m')
		{
			if(fim_options[i].mandesc)
				oss << fim_options[i].mandesc;
			else
			{
				oss << "\t\t ";
				if(fim_options[i].desc)
					oss << fim_options[i].desc;
			}
		}
		//if(helparg||*helparg!='m') oss << FIM_SYM_ENDL;
		oss << sloe;
		//if(helparg&&*helparg=='l') std::cout << "TODO: print extended help here\n";
	}
	oss << "\n";
	return oss.str();
}

int fim_dump_man_page(void)
{
	string mp=
			string(".\\\"\n"
			".\\\" $Id""$\n"
			".\\\"\n"
			".TH FIM 1 \"(c) 2007-" FIM_CNS_LCY " " FIM_AUTHOR_NAME "\"\n"
			".SH NAME\n"
			"fim - \\fBF\\fPbi (linux \\fBf\\fPrame\\fBb\\fPuffer \\fBi\\fPmageviewer) \\fBIM\\fPproved, an universal image viewer\n"
			".SH SYNOPSIS\n"
			FIM_MAN_Bh("fim", "[" FIM_CNS_EX_OPTIONS "] [--] " FIM_CNS_EX_IMPA_STRING " [" FIM_CNS_EX_IMPS_STRING "]")
			FIM_MAN_Bh("fim", "--" FIM_OSW_OUTPUT_DEVICE " "  FIM_DDN_VARS	 "")
			FIM_MAN_Bh("... | fim", "[" FIM_CNS_EX_OPTIONS "] [--] [" FIM_CNS_EX_IMPS_STRING "] -"))+
#ifdef FIM_READ_STDIN
			string(FIM_MAN_Bh("fim", "[" FIM_CNS_EX_OPTIONS "] [--] [" FIM_CNS_EX_FILES_STRING "] - < " FIM_CNS_EX_FNLTF_STRING ))+
#endif /* FIM_READ_STDIN */
#ifdef FIM_READ_STDIN_IMAGE
			string(FIM_MAN_Bh("fim", "--" FIM_OSW_IMAGE_FROM_STDIN " [" FIM_CNS_EX_OPTIONS "] < " FIM_CNS_EX_IMFL_STRING ))+
#endif /* FIM_READ_STDIN_IMAGE */
#ifdef FIM_READ_STDIN
			string(FIM_MAN_Bh("fim", "--" FIM_OSW_SCRIPT_FROM_STDIN " [" FIM_CNS_EX_OPTIONS "] < " FIM_CNS_EX_SCRIPTFILE ))+
#endif /* FIM_READ_STDIN */
#if FIM_WANT_HELP_ARGS
			string(FIM_MAN_Bh("fim", "--help[=" FIM_HELP_EXTRA_OPTS "] [" FIM_CNS_EX_HELP_ITEM " ...] "))+
#endif /* FIM_WANT_HELP_ARGS */
			string("\n"
			".SH DESCRIPTION\n"
			".B\nfim\nis a `swiss army knife' for displaying image files.\n"
			"It is capable of displaying image files using different graphical devices while offering a uniform look and feel.\n"
			"Key bindings are customizable and specified in an initialization file.\n"
			"Interaction with standard input and output is possible, especially in shell scripts.\n"
			"An internal scripting language specialized for image viewing allows image navigation, scaling, manipulation of internal variables, command aliases, and Vim-like autocommands.\n"
			"The internal language can be interacted with via a command line mode capable of autocompletion and history (the readline mode).\n"
			"Further features are display of EXIF tags, JPEG comments, EXIF rotation/orientation, load of \"description files\", faster load via image caching, command recording, and much more.\n\n"
			"As a default,\n.B\nfim\ndisplays the specified file(s) on the detected, most convenient graphical device. "
			"This can be with SDL if running under X, an ASCII-art driver (aalib or libcaca) if running behind ssh without X forwarding, or the linux framebuffer device. "
			"Graphical file formats " FIM_CNS_DSFF_SN " are supported natively, while " FIM_CNS_DSFF_SL " are supported via third party libraries. \n"
			"Further formats are supported via external converters. \n"
			"For XCF (Gimp's) images, '" FIM_EPR_XCFTOPNM "' or '" FIM_EPR_XCF2PNM "' is used.\n"
			"For FIG vectorial images, '" FIM_EPR_FIG2DEV "' is used.\n"
			"For DIA vectorial images, '" FIM_EPR_DIA "' is used.\n"
			"For NEF raw camera images, '" FIM_EPR_DCRAW "' is used.\n"
			"For SVG vectorial images, '" FIM_EPR_INKSCAPE "' is used.\n"
			"For other formats ImageMagick's '" FIM_EPR_CONVERT "' is used.\n"
#if FIM_HAS_TIMEOUT
			"The converter is given " FIM_EXECLP_TIMEOUT " seconds for the conversion before a timeout.\n"
#else /* FIM_HAS_TIMEOUT */
			"During conversion fim pauses for a second.\n"
#endif /* FIM_HAS_TIMEOUT */
			"\n")+
			string("\n""If " FIM_CNS_EX_IMPA_STRING	" is a file, its format is guessed not by its name but by its contents. "
				"See the " FIM_MAN_fB(FIM_VID_FILE_LOADER) " variable to change this default.\n\n")
#ifdef FIM_READ_DIRS
			+
			string("\n""If " FIM_CNS_EX_IMPA_STRING " is a directory, load files of supported formats contained there. If " FIM_CNS_EX_IMPA_STRING " contains a trailing slash (" FIM_CNS_SLASH_STRING "), it is treated as a directory; otherwise that is checked via " FIM_MAN_fB("stat(2)") ". To change this default, see description of the " FIM_MAN_fB(FIM_VID_PUSHDIR_RE) " variable and the " FIM_MAN_fB("--" FIM_OSW_NO_STAT_PUSH) " and " FIM_MAN_fB("--" FIM_OSW_RECURSIVE) " options.\n\n")
#endif /* FIM_READ_DIRS */
			;

	//		string("Please note that a user guide of \n.B fim\nis in the " FIM_CNS_FIM_TXT " file distributed in the source package.\n\n")+
			mp+=string("This man page describes \n.B fim\ncommand line options and usage.\n"
			"See man " FIM_MAN_fR("fimrc") "(5) for a full specification of the \n.B\nfim\nlanguage, commands, keysyms, autocommands, variables, aliases, examples for a configuration file and readline usage samples.\n"
			"\n"
			".SH USAGE\n"
			"You may invoke\n.B\nfim\nfrom an interactive shell and control it with the keyboard, as you would do with any image viewer with reasonable key bindings.\n"
			"\n.B\nfim\nis keyboard oriented: there are no user menus or buttons available.\n"
			"If you need some feature or setting which is not accessible from the default keyboard configuration, you probably need a custom configuration or simply need to type a custom command. For these, you can use the internal command and configuration language.\n"
			"\nSee options " FIM_MAN_fB("--" FIM_OSW_READ_FROM_STDIN) ", " FIM_MAN_fB("--" FIM_OSW_SCRIPT_FROM_STDIN) ", and " FIM_MAN_fB("--" FIM_OSW_IMAGE_FROM_STDIN) " for more script-oriented usages.\n"
			"\nThe full commands specification is also accessible at runtime using the internal help system (typing :help).\n"
			"\n"
			"\n.SH OPTIONS\n"
			"Accepted command line " FIM_MAN_fB("" FIM_CNS_EX_OPTIONS "") ":\n");
			mp+=fim_dump_man_page_snippets();
			mp+=string(".SH PROGRAM RETURN STATUS\n");
			mp+=string("The program return status is ")+string(FIM_ERR_NO_ERROR)+string(" on correct operation; ");
			mp+=string(FIM_PERR_UNSUPPORTED_DEVICE)+string(" on unsupported device specification; ");
			mp+=string(FIM_PERR_BAD_PARAMS)+string(" on bad input; ");
			mp+=string(FIM_PERR_GENERIC)+string(" on a generic error; ");
			mp+=string(FIM_PERR_OOPS)+string(" on a signal-triggered program exit; ");
			mp+=string(" or a different value in case of an another error.\n"
			" The return status may be controlled by the use of the " FIM_FLT_QUIT " command.\n"
			".SH COMMON KEYS AND COMMANDS\n"
".nf\n"
"The following keys and commands are default hardcoded in the minimal configuration. These are working by default before any configuration file loading, and before the hardcoded config loading (see variable " FIM_MAN_fB(FIM_VID_FIM_DEFAULT_CONFIG_FILE_CONTENTS) ").\n\n"
);
//"cursor keys     scroll large images\n"
//"h,j,k,l		scroll large images left,down,up,right\n"
//"+, -            zoom in/out\n"
//"ESC, q          quit\n"
//"Tab             toggle output console visualization\n"
//"PgUp,p            previous image\n"
//"PgDn,n            next image\n"
//"Space  	        next image if on bottom, scroll down instead\n"
//"Return          next image, write the filename of the current image to stdout on exit from the program.\n"

#define FIM_SROC(VAR,VAL) { if( VAR != VAL && VAR != Nothing ) std::cerr<<FIM_EMSG_DSMO; VAR = VAL; }
#define FIM_ADD_DOCLINE_FOR_CMD(REP,CMD) \
			if(!cc.find_key_for_bound_cmd(CMD).empty()) \
			{ \
				if(REP!=1)mp+=FIM_XSTRINGIFY(REP); \
				else mp+=" "; \
				mp+=cc.find_key_for_bound_cmd(CMD); \
				mp+="    "; \
				if(REP!=1)mp+=FIM_XSTRINGIFY(REP); \
				if(!cc.aliasRecall(CMD).empty())mp+=cc.aliasRecall(CMD); /* new: one-level expansion (may rather want function for this..) */ \
				else mp+=CMD; \
				mp+="\n"; \
			}
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_NEXT)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLA_NEXT_FILE)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_PREV)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_NEXT_FILE)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_PREV_FILE)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_NEXT_PAGE)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_PREV_PAGE)
			/* TODO: may use a search-based method for locating keys to other commands... */
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLA_MAGNIFY)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLA_REDUCE)
			//FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_MIRROR)
			//FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_FLIP)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_PAN_LEFT)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_PAN_RIGHT)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_SCROLL_UP)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_SCROLL_DOWN)
			//FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_PAN_UP);
			//FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLC_PAN_DOWN);
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLT_ROTATE)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLT_LIST)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLT_SCROLL)
			FIM_ADD_DOCLINE_FOR_CMD(1,FIM_FLT_QUIT)
			mp+="You can type a number before a command binding to iterate the assigned command:\n";
			//FIM_ADD_DOCLINE_FOR_CMD(3,FIM_FLC_PAN_UP);
			FIM_ADD_DOCLINE_FOR_CMD(3,FIM_FLC_SCROLL_UP)
//			mp+=string(
//"d,x,D,X		diagonal scroll\n"
//"C-w			scale to the screen width\n"
//"H			scale to the screen heigth\n"
//);
			mp+=string(
"\n"
FIM_INTERNAL_LANGUAGE_SHORTCUT_SHORT_HELP
"\n"
			);
#if 0
			mp+=string(
"C-n		 after entering in search mode (/) and submitting a pattern, C-n (pressing the Control and the n key together) will jump to the next matching filename\n"
"C-c		 terminate instantaneously fim\n"
"T		 split horizontally the current window\n"
"V		 split vertically the current window\n"
"C		 close  the currently focused window\n"
"H		 change the currently focused window with the one on the left\n"
"J		 change the currently focused window with the lower\n"
"K		 change the currently focused window with the upper\n"
"L		 change the currently focused window with the one on the right\n"
"U		 swap the currently focused window with the split sibling one (it is not my intention to be obscure, but precise: try V, m,  U and see by yourself)\n"
"d		move the image diagonally north-west\n"
"D		move the image diagonally south-east\n"
"x		move the image diagonally north-east\n"
"X		move the image diagonally south-west\n"
"m		mirror\n"
"f		flip\n"
"r		rotate\n"
			);
#endif
			mp+=string(
"\n"
"You can visualize all of the default bindings invoking fim --dump-default-fimrc | grep bind .\n"
"You can visualize all of the default aliases invoking fim  --dump-default-fimrc | grep alias .\n"
"\n"
".fi\n"
".P\n"
"The Return vs. Space key thing can be used to create a file list while\n"
"reviewing the images and use the list for batch processing later on.\n"
"\n"
"All of the key bindings are reconfigurable; see the default \n"
".B fimrc\n"
"file for examples on this, or read the complete manual: the FIM.TXT file\n"
"distributed with fim.\n"
				);
			mp+=string(
".SH AFFECTING ENVIRONMENT VARIABLES\n"
".nf\n"
//"" FIM_ENV_FBFONT "		(just like in fbi) a consolefont or a X11 (X Font Server - xfs) font file.\n"
"" FIM_ENV_FBFONT "		(just like in fbi) a Linux consolefont font file.\n"
"If using a gzipped font file, the " FIM_EPR_ZCAT " program is used to uncompress it (via " FIM_MAN_fB("execvp(3)") ").\n"
"If " FIM_ENV_FBFONT " is unset, the following files are probed and the first existing one is selected:\n\n");
mp+="@FIM_CUSTOM_DEFAULT_CONSOLEFONT@\n";
mp+=get_default_font_list();
#if FIM_WANT_HARDCODED_FONT
mp+="\nIf the special " FIM_DEFAULT_HARDCODEDFONT_STRING " string is specified, a hardcoded font is used.";
#endif /* FIM_WANT_HARDCODED_FONT */
mp+="\n";
mp+=string(
//"			For instance,  /usr/share/consolefonts/LatArCyrHeb-08.psf.gz is a Linux console file.\n"
//"			Consult 'man setfont' for your current font paths.\n"
//"			NOTE: Currently xfs is disabled.\n"
"" FIM_ENV_FBGAMMA "		(just like in fbi) gamma correction (applies to dithered 8 bit mode only). Default is " FIM_CNS_GAMMA_DEFAULT_STR ".\n"
"" FIM_ENV_FRAMEBUFFER "	(just like in fbi) user set framebuffer device file (applies only to the " FIM_DDN_INN_FB " mode).\n"
"If unset, fim probes for " FIM_DEFAULT_FB_FILE ".\n"
"" FIM_CNS_TERM_VAR "		(only in fim) influences the output device selection algorithm, especially if $" FIM_CNS_TERM_VAR "==\"screen\".\n"
"" FIM_ENV_SSH "	if set and no output device specified, assume we're over " FIM_MAN_fB("ssh") ", and give precedence to " FIM_DDN_INN_CACA ", then " FIM_DDN_INN_AA " (if present).\n"
"" FIM_CNS_TERMUX_VERSION "	if set and no output device specified, assume we're over " FIM_MAN_fB("termux") ", and give precedence to " FIM_DDN_INN_CACA ", then " FIM_DDN_INN_AA " (if present).\n"
"" FIM_ENV_WAYLAND_DISPLAY "	if set and no output device specified, assume we're over " FIM_MAN_fB("Wayland") ", and give precedence to " FIM_DDN_INN_GTK ", then " FIM_DDN_INN_SDL ", then " FIM_DDN_INN_CACA ", then " FIM_DDN_INN_AA " (if present).\n"
#if defined(FIM_WITH_LIBSDL) || defined(FIM_WITH_LIBGTK)
"" FIM_ENV_DISPLAY "	If this variable is set, then the " FIM_DDN_INN_GTK " driver has precedence, then "  FIM_DDN_INN_SDL ".\n"
#elif defined(FIM_WITH_LIBIMLIB2)
"" FIM_ENV_DISPLAY "	If this variable is set, then the " FIM_DDN_INN_IL2 " driver is probed by default.\n"
#endif /* FIM_WITH_LIBSDL */
".SH COMMON PROBLEMS\n"
".B fim -o " FIM_DDN_INN_FB "\n"
"needs read-write access to the framebuffer devices (/dev/fbN or /dev/fb/N), i.e you (our\n"
"your admin) have to make sure fim can open the devices in rw mode.\n"
"The IMHO most elegant way is to use pam_console (see\n"
"/etc/security/console.perms) to chown the devices to the user logged\n"
"in on the console.  Another way is to create some group, chown the\n"
"special files to that group and put the users which are allowed to use\n"
"the framebuffer device into the group.  You can also make the special\n"
"files world writable, but be aware of the security implications this\n"
"has.  On a private box it might be fine to handle it this way\n"
"through.\n"
"\n"
"If using udev, you can edit:\n"
"/etc/udev/permissions.d/50-udev.permissions\n"
"and set these lines like here:\n"
" # fb devices\n"
" fb:root:root:0600\n"
" fb[0-9]*:root:root:0600\n"
" fb/*:root:root:0600\n"
".P\n"
"\n"
".B fim -o " FIM_DDN_INN_FB "\n"
"also needs access to the linux console (i.e. /dev/ttyN) for sane\n"
"console switch handling.  That is obviously no problem for console\n"
"logins, but any kind of pseudo tty (xterm, ssh, screen, ...) will\n"
".B not\n"
"work.\n"
".SH INVOCATION EXAMPLES\n"
FIM_MAN_Bn("fim --help -R -B")
"# get help for options " FIM_MAN_fB("-R") " and \n.B -B\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("fim media/ ")
"# load files from the directory \n.B media/\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("fim -R media/ --sort ")
"# open files found by recursive traversal of directory media, then sorting the list\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("find /mnt/media/ -name *.jpg | fim - ")
"# read input files list from standard input\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("find /mnt/media/ -name *.jpg | shuf | fim -")
"# read input files list from standard input, randomly shuffled\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("cat script.fim | fim -p images/*")
"# read a script file\n"
".B script.fim\n"
"from standard input before displaying files in the directory\n"
".B images\n"
".P\n"
".P\n"
#ifdef FIM_READ_STDIN_IMAGE
"\n"
FIM_MAN_Bn("scanimage ... | tee scan.ppm | fim -i")
"# read the image scanned from a flatbed scanner as soon as it is read\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("h5topng -x 1 -y 2 dataset.hdf -o /dev/stdout | fim -i")
"# visualize a slice from an HDF5 dataset file\n"
".P\n"
".P\n"
#endif /* FIM_READ_STDIN_IMAGE */
"\n"
FIM_MAN_Bn("fim * > selection.txt")
"# output the file names marked interactively with the '" FIM_FLT_LIST " \"mark\"' command in fim to a file\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("fim * | fim -")
"# output the file names marked with 'm' in fim to a second instance of fim, in which these could be marked again\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("fim\n-c 'pread \"vgrabbj -d /dev/video0 -o png\";reload'")
"# display an image grabbed from a webcam\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("fim\n-o " FIM_DDN_INN_AA " -c 'pread \"vgrabbj -d /dev/video0 -o png\";reload;system \"fbgrab\" \"asciime.png\"'")
".fi\n"
"# if running in framebuffer mode, saves a png screenshot with an ASCII rendering of an image grabbed from a webcam\n"
".P\n"
".P\n"
"\n"
FIM_MAN_Bn("fim\n" "-c 'while(1){pread \"vgrabbj -d /dev/video0 -o png\";reload;sleep 1;};'\n")
"# display a sequence of images grabbed from a webcam; circa 1 per second\n"
".P\n"
".P\n"
"\n"
);
mp+=string(
".SH NOTES\n"
"This manual page is neither accurate nor complete. In particular, issues related to driver selection shall be described more accurately. Also the accurate sequence of autocommands execution, variables application is critical to understanding fim, and should be documented.\n"
#ifdef FIM_READ_STDIN_IMAGE
"The filename \"" FIM_STDIN_IMAGE_NAME "\" is reserved for images read from standard input (view this as a limitation), and thus handling files with such name may incur in limitations.\n"
#endif /* FIM_READ_STDIN_IMAGE */
);
mp+=string(
".SH BUGS\n"
FIM_MAN_fB("fim")
" has bugs. Please read the \n"
".B BUGS\n"
"file shipped in the documentation directory to discover the known ones.\n"
"There are also inconsistencies in the way the internal command line works across the different graphical devices.\n"
".SH  FILES\n"
"\n"
".TP 15\n"
".B " FIM_CNS_DOC_PATH "\n"
"The directory with "
FIM_MAN_fB("fim")
" documentation files.\n"
".TP 15\n"
".B " FIM_CNS_SYS_RC_FILEPATH "\n"
"The system-wide "
FIM_MAN_fB("fim")
" initialization file (executed at startup, after executing the hardcoded configuration).\n"

".TP 15\n"
".B " FIM_CNS_USR_RC_COMPLETE_FILEPATH "\n"
"The personal "
FIM_MAN_fB("fim")
" initialization file (executed at startup, after the system-wide initialization file).\n"

".TP 15\n"
".B " FIM_CNS_HIST_COMPLETE_FILENAME "\n"
"File where to load from or save command history. See"
" (man " FIM_MAN_fR("fimrc") "(5),"
"  man " FIM_MAN_fR("readline") "(3))."
"\n"
	".TP 15\n"
".B ~/.inputrc\n"
"If "
FIM_MAN_fB("fim")
" is built with GNU readline support, it is susceptible to changes in the user set ~/.inputrc configuration file contents.  For details, see"
" (man " FIM_MAN_fR("readline") "(3))."
"\n"
);
mp+=string(
".SH SEE ALSO\n"
"Other " FIM_MAN_fB("fim") " man pages: " FIM_MAN_fR("fimgs") "(1), " FIM_MAN_fR("fimrc") "(1).\n"
".fi\n"
"Conversion programs: " FIM_MAN_fR("convert") "(1), " FIM_MAN_fR("dia") "(1), " FIM_MAN_fR("xcftopnm") "(1), " FIM_MAN_fR("fig2dev") "(1), " FIM_MAN_fR("inkscape") "(1).\n"
".fi\n"
"Related programs: " FIM_MAN_fR("fbset") "(1), " FIM_MAN_fR("con2fb") "(1), " FIM_MAN_fR("vim") "(1), " FIM_MAN_fR("mutt") "(1), " FIM_MAN_fR("exiftool") "(1), " FIM_MAN_fR("exiftags") "(1), " FIM_MAN_fR("exiftime") "(1), " FIM_MAN_fR("exifcom") "(1), " FIM_MAN_fR("fbi") "(1), " FIM_MAN_fR("fbida") "(1), " FIM_MAN_fR("feh") "(1), " FIM_MAN_fR("qiv") "(1), " FIM_MAN_fR("sxiv") "(1), " FIM_MAN_fR("fbgrab") "(1).\n"
".fi\n"
"Related documentation: " FIM_MAN_fR("fbdev") "(4), " FIM_MAN_fR("vcs") "(4), " FIM_MAN_fR("fb.modes") "(8), " FIM_MAN_fR("fbset") "(8), " FIM_MAN_fR("setfont") "(8)" ".\n"
".fi\n"
);
mp+=string(
".SH AUTHOR\n"
".nf\n"
FIM_AUTHOR " is the author of fim, \"Fbi IMproved\". \n"
".fi\n"
".SH COPYRIGHT\n"
".nf\n"
"Copyright (C) 2007-" FIM_CNS_LCY " " FIM_AUTHOR " (author of fim)\n"
".fi\n"
"Copyright (C) 1999-2004 " FBI_AUTHOR " is the author of \"fbi\", upon which\n.B fim\nwas originally based. \n"
".P\n"
"This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.\n"
".P\n"
"This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.\n"
".P\n"
"You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.\n"
);
	mp+=string("\n");
	std::cout << mp;
	return 0;
}

FIM_NORETURN fim_perr_t help_and_exit(const fim_char_t *argv0, fim_perr_t code=FIM_PERR_NO_ERROR, const fim_char_t*helparg=FIM_NULL)
{
	if(helparg&&*helparg=='b')
	{
	    	std::cout << "fim - No loadable images specified.\nUse `fim --help' for detailed usage information.\n";
		goto done;
	}
	if(helparg&&*helparg=='m')
	{
		fim_dump_man_page(); 
		goto done;
	}
	    cc.printHelpMessage(argv0);
	    std::cout << " where OPTIONS are taken from:\n";
	    for(size_t i=0;i<fim_options_count-1;++i)
	    {	
		if(isascii(fim_options[i].val)){
	   	if((fim_options[i].val)!='-')
			std::cout << "\t-"<<(fim_options[i].cVal()) ;
	   	else std::cout << "\t-";}else std::cout<<"\t";
		std::cout << "\t\t";
	    	std::cout << "--"<<fim_options[i].name ;
		switch(fim_options[i].has_arg){
		case no_argument:
		break;
		case required_argument:
		//std::cout << " <arg>";
		if(fim_options[i].optdesc)
		       	std::cout << " =" << fim_man_to_text(fim_options[i].optdesc,true);
		else std::cout << ( (fim_options[i].has_arg==required_argument) ? " =<arg>" : "[=arg]" );
		break;
		default:
		;
		}
		if(helparg&&*helparg=='d')
			std::cout << "\t\t " << fim_options[i].desc;
		if(helparg&&*helparg=='l')
			std::cout << "\t\t " << fim_man_to_text(fim_options[i].mandesc,true);
		std::cout << FIM_SYM_ENDL;
		//if(helparg&&*helparg=='l') std::cout << "TODO: print extended help here\n";
		}
		std::cout << "\n Please read the documentation distributed with the program.\n"
			  << " For further help, consult the online help in fim (:" FIM_FLT_HELP "), and man fim (1), fimrc (5).\n"
			  << " Pre-configuration online help also available invoking --help " << fim_man_to_text(FIM_CNS_EX_HELP_ITEM,true) << ".\n"
			  << " For bug reporting read the " FIM_CNS_BUGS_FILE " file.\n";
done:
	    std::exit(code);
	    return code;
}

static fim_err_t fim_load_filelist(const char *fn, const char sac, fim_flags_t pf)
{
			/* TODO: move to CommandConsole */
	    		/*cc.pre_autocmd_add(FIM_VID_DISPLAY_STATUS"=...;");*/ /* or verbose ... */
			const char sa [2] = { sac ? sac : FIM_SYM_CHAR_ENDL, FIM_SYM_CHAR_NUL };
			fim_char_t *lineptr=FIM_NULL;
			size_t bs=0;
			int fc=0;
			FILE * fd = FIM_NULL;
		
			if(fn)
				fd = fim_fopen(fn,"r");
			else
				fd = stdin;
	
			if(!fd)
			{
				// note: shall print a warning
				std::cerr<<"Warning: the " << fn << " file could not be opened.\n";
				goto ret;
			}

			while(fim_getline(&lineptr,&bs,fd,*sa)>0)
			{
				if(*sa=='\n')
					chomp(lineptr); // BAD

				if(*sa && lineptr && strstr(lineptr,sa))
					*strstr(lineptr,sa) = FIM_SYM_CHAR_NUL;
				cc.push(lineptr,pf);
				// printf("%s\n",lineptr);
				fim_getline(&lineptr,FIM_NULL,FIM_NULL,0); // proper free
				lineptr=FIM_NULL;
				if(false)
					++fc, printf("%s %d\n",FIM_CNS_CLEARTERM,fc);
			}

			fim_getline(&lineptr,FIM_NULL,FIM_NULL,0); // proper free
			if(fn && fd)
				fim_fclose(fd);
ret:
			return 0;
}

bool fim_args_from_desc_file(args_t& argsc, const fim_fn_t& dfn, const fim_char_t sc)
{
	/* dfn: descriptions file name */
	/* sc: separator char */
	std::ifstream mfs (dfn.c_str(),std::ios::in);
	std::string ln;

	if( !mfs.is_open() )
	{
		std::cerr << "Description file " << dfn << " could not be opened!" << std::endl;
		return false; // a proper FIM_ERR.. would be better
	}

	while( std::getline(mfs,ln))
	{
		std::stringstream  ls(ln);
		std::string fn;
		fim_fn_t ds;

		if( ls.peek() == FIM_SYM_PIC_CMT_CHAR )
			;
		else
			if(std::getline(ls,fn,sc))
				argsc.push_back(fn);
	}
	mfs.close();
	return true;
}

std::string guess_output_device(void)const
{
	// TODO : need better output device probing mechanism
	std::string output_device;

	if( fim_getenv_is_nonempty(FIM_ENV_WAYLAND_DISPLAY) ) /* are we under Wayland ? */
	{
#ifdef FIM_WITH_AALIB
		output_device=FIM_DDN_INN_AA;
#endif /* FIM_WITH_AALIB */
#ifdef FIM_WITH_LIBCACA
		output_device=FIM_DDN_INN_CACA;
#endif /* FIM_WITH_LIBCACA */
#ifdef FIM_WITH_LIBSDL
		output_device=FIM_DDN_INN_SDL;
#endif /* FIM_WITH_LIBSDL */
#ifdef FIM_WITH_LIBGTK
		output_device=FIM_DDN_INN_GTK;
#endif /* FIM_WITH_LIBGTK */
	}
	else
	if( fim_getenv_is_nonempty(FIM_ENV_SSH) || fim_getenv_is_nonempty(FIM_CNS_TERMUX_VERSION) ) /* is this a ssh or termux session ? */
	{
#ifdef FIM_WITH_AALIB
		output_device=FIM_DDN_INN_AA;
#endif /* FIM_WITH_AALIB */
#ifdef FIM_WITH_LIBCACA
		output_device=FIM_DDN_INN_CACA;
#endif /* FIM_WITH_LIBCACA */
	}
	else
	#if defined(FIM_WITH_LIBSDL) || defined(FIM_WITH_LIBIMLIB2) || defined(FIM_WITH_LIBGTK)
	/* check to see if we are under X */
	if( fim_getenv_is_nonempty(FIM_ENV_DISPLAY) )
	{
#ifdef FIM_WITH_LIBIMLIB2
		output_device=FIM_DDN_INN_IL2;
#endif /* FIM_WITH_LIBIMLIB2 */
#ifdef FIM_WITH_LIBSDL
		output_device=FIM_DDN_INN_SDL;
#endif /* FIM_WITH_LIBSDL */
#ifdef FIM_WITH_LIBGTK
		output_device=FIM_DDN_INN_GTK;
#endif /* FIM_WITH_LIBGTK */
	}
	else
	#endif
#ifndef FIM_WITH_NO_FRAMEBUFFER
		output_device=FIM_DDN_INN_FB;
#else /* FIM_WITH_NO_FRAMEBUFFER */
#ifdef FIM_WITH_AALIB
		output_device=FIM_DDN_INN_AA;
#else /* FIM_WITH_AALIB */
#ifdef FIM_WITH_LIBCACA
		output_device=FIM_DDN_INN_CACA;
#else /* FIM_WITH_LIBCACA */
		output_device=FIM_DDN_INN_DUMB ;
#endif /* FIM_WITH_LIBCACA */
#endif /* FIM_WITH_AALIB */
#endif	/* FIM_WITH_NO_FRAMEBUFFER */
	return output_device;
}

	public:
	int main(int argc,char *argv[])
	{
		fim_perr_t retcode=FIM_PERR_NO_ERROR;
		/*
		 * an adapted version of the main function
		 * of the original version of the fbi program
		 */
		int              opt_index = 0;
		int              i;
		int		 want_random_shuffle=0;
		int              want_reverse_list=0;
	#ifdef FIM_READ_STDIN
		enum rsc { Nothing = 0, ImageFile = 1, FilesList = 2, Script = 3/*, Desc = 4*/ };
		int              read_stdin_choice = Nothing;
		int perform_sanity_check=0;
#if FIM_WASM_DEFAULTS 
		std::string pcxfn="0.pcx";
		for (int i=0;i<10;++i)
		{
			cc.push(pcxfn.c_str(),FIM_FLAG_DEFAULT | FIM_FLAG_PUSH_ALLOW_DUPS);
			pcxfn[0]++;
		}
	    	//cc.setVariable(FIM_VID_WANT_SLEEPS,1);
    		//cc.autocmd_add(FIM_ACM_PREEXECUTIONCYCLE,"",FIM_CNS_SLIDESHOW_CMD);
		//cc.setVariable(FIM_VID_DBG_COMMANDS,FIM_CNS_DBG_CMDS_MID);
#endif /* FIM_WASM_DEFAULTS */
	#endif /* FIM_READ_STDIN */
		int c;
		int ndd=0;/*  on some systems, we get 'int dup(int)', declared with attribute warn_unused_result */
		bool appendedPostInitCommand=false;
		bool appendedPreConfigCommand=false;
#if FIM_WANT_NOEXTPROPIPE
		bool np=false;
#endif /* FIM_WANT_NOEXTPROPIPE */
		char sac = FIM_SYM_CHAR_ENDL;
#if FIM_WANT_R_SWITCH
		const char * rs=FIM_NULL;
#endif /* FIM_WANT_R_SWITCH */
		fim_flags_t pf = FIM_FLAG_DEFAULT; /* push flags */
#if FIM_WANT_PIC_LISTUNMARK
		args_t argsc;
#endif /* FIM_WANT_PIC_LISTUNMARK */
#if ( FIM_WANT_PIC_CMTS || FIM_WANT_PIC_LISTUNMARK )
		/* TODO: shall merge the above in a FIM_WANT_PIC_DSCFILEREAD  */
#endif /* FIM_WANT_PIC_CMTS */
		int load_verbosity=0;
		int font_verbosity=0;
		int internal_verbosity=0;

	    	g_fim_output_device=FIM_CNS_EMPTY_STRING;
	
		setlocale(LC_ALL,"");	//uhm..

		{
			int foi;

			for(foi=0;foi<fim_options_count;++foi)
			{
				options[foi].name=fim_options[foi].name;
				options[foi].has_arg=fim_options[foi].has_arg;
				options[foi].flag=fim_options[foi].flag;
				options[foi].val=fim_options[foi].val;
			}
		}

	    	for (;;) {
		    c = getopt_long(argc, argv, "=/:1C:HAb::wc:uvah::PqVr:m:d:g:s:T:E:f:D:NhF:tfipW:o:S:L:B"
#if FIM_WANT_RECURSE_FILTER_OPTION
	   	"R::"
#else /* FIM_WANT_RECURSE_FILTER_OPTION */
	   	"R"
#endif /* FIM_WANT_RECURSE_FILTER_OPTION */
#if FIM_WANT_CMDLINE_KEYPRESS
		"k:"
		"K:"
#endif /* FIM_WANT_CMDLINE_KEYPRESS */
#if FIM_WANT_NOEXTPROPIPE
		"X"
#endif /* FIM_WANT_NOEXTPROPIPE */
				    ,
				options, &opt_index);
		if (c == -1)
		    break;
		switch (c) {
	/*	case 0:
		    // long option, nothing to do
		    break;*/
		case '1':
		    //fbi's
		    cc.setVariable(FIM_VID_LOOP_ONCE,1);
	//	    FIM_FPRINTF(stderr, "sorry, this feature will be implemented soon\n");
		    break;
		case 'a':
		    //fbi's
		    //cc.setVariable(FIM_VID_AUTOTOP,1);
		    //TODO: still needs some tricking .. 
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_SCALE_STYLE"='a';");
	#else /* FIM_AUTOCMDS */
		    cout << FIM_EMSG_NO_SCRIPTING;
	#endif /* FIM_AUTOCMDS */
		    break;
#if FIM_WANT_TEXT_RENDERING
		case 0x74657874:
		    	cc.setVariable(FIM_VID_TEXT_DISPLAY,1);
#else /* FIM_WANT_TEXT_RENDERING */
			std::cerr<<"Warning: the --" FIM_OSW_TEXT " option was disabled at compile time.\n";
#endif /* FIM_WANT_TEXT_RENDERING */

#if FIM_WANT_RAW_BITS_RENDERING
			FIM_FALLTHROUGH
		case 'b':
		    //fim's
		    if(optarg && strstr(optarg,"1")==optarg && !optarg[1])
		    	cc.setVariable(FIM_VID_BINARY_DISPLAY,1);
		    else
		    if(optarg && strstr(optarg,"24")==optarg && !optarg[2])
		    	cc.setVariable(FIM_VID_BINARY_DISPLAY,24);
                    else
		    {
			if(optarg)
				std::cerr<<"Warning: the --" FIM_OSW_BINARY " option supports 1 or 24 bpp depths. Using "<<FIM_DEFAULT_AS_BINARY_BPP<<".\n";
		    	cc.setVariable(FIM_VID_BINARY_DISPLAY,FIM_DEFAULT_AS_BINARY_BPP);
                    }
		    break;
#else /* FIM_WANT_RAW_BITS_RENDERING */
		case 'b':
			std::cerr<<"Warning: the --" FIM_OSW_BINARY " option was disabled at compile time.\n";
		    break;
#endif /* FIM_WANT_RAW_BITS_RENDERING */
		case 'A':
		    //fbi's
		    //cc.setVariable(FIM_VID_AUTOTOP,1);
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_AUTOTOP "=1;");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 'q':
		    //fbi's
		    //FIM_FPRINTF(stderr, "sorry, this feature will be implemented soon\n");
		    //cc.setVariable(FIM_VID_DISPLAY_STATUS,0);
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_CNS_QUIET_CMD);
	#endif /* FIM_AUTOCMDS */
		    break;
		case 'f':
	#ifndef FIM_WANT_NOSCRIPTING
		    cc.setVariable(FIM_VID_DEFAULT_ETC_FIMRC,optarg);
	#else /* FIM_WANT_NOSCRIPTING */
		    cout << FIM_EMSG_NO_SCRIPTING;
	#endif /* FIM_WANT_NOSCRIPTING */
		    break;
		case 0x7373:
	#ifndef FIM_WANT_NOSCRIPTING
		    	cc.setVariable(FIM_VID_WANT_SLEEPS,optarg);
	    		cc.autocmd_add(FIM_ACM_PREEXECUTIONCYCLE,"",FIM_CNS_SLIDESHOW_CMD);
	#else /* FIM_WANT_NOSCRIPTING */
		    cout << FIM_EMSG_NO_SCRIPTING;
	#endif /* FIM_WANT_NOSCRIPTING */
		    break;
		case 0x70617363:
		    //fim's
	#ifdef FIM_AUTOCMDS
		    cc.setVariable(FIM_VID_SANITY_CHECK,1);
		    perform_sanity_check=1;
	#endif /* FIM_AUTOCMDS */
		    break;
		case 0x76696d0a: // verbose-interpreter
		    //fim's
		    if(internal_verbosity++)
			cc.setVariable(FIM_VID_DBG_COMMANDS,FIM_CNS_DBG_CMDS_MAX);
		    else
			cc.setVariable(FIM_VID_DBG_COMMANDS,FIM_CNS_DBG_CMDS_MID);
		    break;
		case 0x7666: // verbose-fonts-load
		    //fim's
	#ifdef FIM_AUTOCMDS
		    font_verbosity++;
		    cc.setVariable(FIM_VID_FB_VERBOSITY,1/*font_verbosity*/);
	#endif /* FIM_AUTOCMDS */
		    break;
		case 0x766c: // verbose-load
		    //fim's
	#ifdef FIM_AUTOCMDS
		{
		    std::ostringstream tmp;
		    load_verbosity++;
		    tmp << FIM_VID_VERBOSITY << "=" << load_verbosity << ";";
		    cc.pre_autocmd_add(tmp.str());
		}
	#endif /* FIM_AUTOCMDS */
		    break;
		case 'v':
		    //fbi's
		    //cc.setVariable(FIM_VID_DISPLAY_STATUS,1);
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_DISPLAY_STATUS"=1;");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 'w':
		    //fbi's
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_SCALE_STYLE"='w';");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 0x6e7363:
		    //fim's
		    cc.setVariable(FIM_VID_PRELOAD_CHECKS,0);
		    break;
		case '=': /*0x4E4053*/
		    //fbi's
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_SCALE_STYLE"=' ';");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 'H':
		    //fbi's
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_SCALE_STYLE"='h';");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 'P':
		    //fbi's
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_SCALE_STYLE"='w';" FIM_VID_AUTOTOP "=1;");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 0x61757769:
		    //fim's
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_VID_SCALE_STYLE"='1';" "autocmd \"" FIM_ACM_POSTSCALE "\" \"\" \"" FIM_FLT_DISPLAY "'resize';\";");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 0x6f66660a:
		    //fim's
	#ifdef FIM_AUTOCMDS
		{
			fim_int read_offset_l = fim_atoi(optarg);
			if(read_offset_l<0)
				std::cerr<< "Warning: ignoring user set negative offset value (" << read_offset_l << ").\n";
			else
			if(read_offset_l>=0 && isdigit(*optarg))
			{
				std::ostringstream tmp;
				fim_int read_offset_u=0;
				if(strchr(optarg,':'))
					read_offset_u=fim_util_atoi_km2(strchr(optarg,':')+1)-read_offset_l;
				if(strchr(optarg,'+'))
					read_offset_u=fim_util_atoi_km2(strchr(optarg,'+')+1);

				tmp << FIM_VID_OPEN_OFFSET_L << "=" << read_offset_l << ";";
				tmp << FIM_VID_OPEN_OFFSET_U << "=" << read_offset_u << ";";
				cc.pre_autocmd_add(tmp.str());
			}
		}
	#endif /* FIM_AUTOCMDS */
		    break;
		case 0x70726561:
		    //fim's
#ifdef FIM_TRY_CONVERT
		    cc.setVariable(FIM_VID_PREAD,optarg);
#else /* FIM_TRY_CONVERT */
		    std::cerr<< "warning: the use of the convert utility has been disabled at configure time\n";
#endif /* FIM_TRY_CONVERT */
		    break;
		case 'g':
		    //fbi's
		    default_fbgamma = fim_atof(optarg);
		    break;
		case 'r':
#if FIM_WANT_R_SWITCH
		    rs=optarg;
#else /* FIM_WANT_R_SWITCH */
		    //fbi's:
		    //pcd_res = atoi(optarg);
		    cout << FIM_EMSG_UNFINISHED;
		    //fim's (different):
#endif /* FIM_WANT_R_SWITCH */
		    break;
		case 'R':
		    //fim's
		    pf |= FIM_FLAG_PUSH_REC ;
#if FIM_WANT_RECURSE_FILTER_OPTION
		    if(optarg)
		    { 
		    	fim::string npre;
		    	if(*optarg && ( *optarg=='+' || *optarg=='|' ) && optarg[1] )
				npre=fim::string( FIM_CNS_PUSHDIR_RE "|" ), optarg++;
			npre+=optarg;
			cc.setVariable(FIM_VID_PUSHDIR_RE,Var(npre));
		    }
#endif /* FIM_WANT_RECURSE_FILTER_OPTION */
		    break;
#if FIM_WANT_NOEXTPROPIPE
		case 'X':
		    //fim's
		    if(np == false)
		    {
		    	np = true;
	#ifdef FIM_AUTOCMDS
		    	cc.pre_autocmd_add(FIM_VID_NO_EXTERNAL_LOADERS"=1;");
	#endif /* FIM_AUTOCMDS */
		    }
		    break;
#endif /* FIM_WANT_NOEXTPROPIPE */
		case 'B':
		    //fim's
		    pf |= FIM_FLAG_PUSH_REC ;
#if FIM_WANT_BACKGROUND_LOAD
		    pf |= FIM_FLAG_PUSH_ONE ;
#else /* FIM_WANT_BACKGROUND_LOAD */
		    std::cerr<< "warning: background loading is not supported (need to compile with C++11 enabled for this).\n";
#endif /* FIM_WANT_BACKGROUND_LOAD */
		    break;
		case 's':
		    if(fim_atoi(optarg)>0)
		    {
		    	fim::string s=FIM_VID_STEPS"=";
			s+=optarg;
			s+=";";
	#ifdef FIM_AUTOCMDS
			cc.pre_autocmd_add(s);
	#endif /* FIM_AUTOCMDS */
		    }
		    break;
	//	case 't':
		    //fbi's
	//	    timeout = atoi(optarg);
	//	    FIM_FPRINTF(stderr, "sorry, this feature will be implemented soon\n");
	//	    break;
		case 'u':
		    want_random_shuffle=1;
		    break;
		case 0x7073:
		    //FIM_FPRINTF(stderr, "sorry, this feature will be implemented soon\n");
		    //fim's
		    want_random_shuffle=-1;
		    break;
		case 0x73696273:
		    //fim's
#if FIM_WANT_SORT_BY_STAT_INFO
		    want_random_shuffle=c;
#else /* FIM_WANT_SORT_BY_STAT_INFO */
		    std::cerr<< "warning: --" FIM_OSW_SORT_FSIZE " option unsupported (stat() missing).\n";
#endif /* FIM_WANT_SORT_BY_STAT_INFO */
		    break;
		case 0x7369626d:
		    //fim's
#if FIM_WANT_SORT_BY_STAT_INFO
		    want_random_shuffle=c;
#else /* FIM_WANT_SORT_BY_STAT_INFO */
		    std::cerr<< "warning: --" FIM_OSW_SORT_MTIME " option unsupported (stat() missing).\n";
#endif /* FIM_WANT_SORT_BY_STAT_INFO */
		    break;
		case 0x736f626e:
		    //fim's
		    want_random_shuffle=c;
		    break;
		case 0x736f7274:
		    //fim's
		    want_random_shuffle=c;
		    break;
		case 0x7772666c:
		    //fim's
		    want_reverse_list=1;
		    break;
		case 'd':
		    //fbi's
		    default_fbdev = optarg;
		    break;
		case 'i':
		    //fim's
#ifdef FIM_READ_STDIN_IMAGE
		    FIM_SROC(read_stdin_choice,ImageFile)
#else /* FIM_READ_STDIN_IMAGE */
		    FIM_FPRINTF(stderr, FIM_EMSG_NO_READ_STDIN_IMAGE);
#endif /* FIM_READ_STDIN_IMAGE */
		    break;
		case 'm':
		    //fbi's
		    default_fbmode = optarg;
		    break;
	//removed, editing features :
	/*	case 'f':
	//	    fontname = optarg;
		    break;
		case 'e':
	//	    editable = 1;
		    break;
		case 'b':
	//	    backup = 1;
		    break;
		case 'p':
	//	    preserve = 1;
		    break;*/
	//	case 'l':
		    //fbi's
	//	    flist_add_list(optarg);
	//	    FIM_FPRINTF(stderr, "sorry, this feature will be implemented soon\n");
	//	    break;
		case 'T':
		    //fbi's virtual terminal
		    default_vt = atoi(optarg);
		    break;
		case 'V':
		    //fbi's option, but unlike fbi using stdout instead of stderr
		    show_version(stdout);
		    return 0;
		    break;
		case 0x4352:
		    //fim's
		    cc.appendPostInitCommand( "if(" FIM_VID_FILELISTLEN "==1){_ffn=i:" FIM_VID_FILENAME ";" FIM_FLT_CD " i:" FIM_VID_FILENAME ";" FIM_FLT_LIST " 'remove' i:" FIM_VID_FILENAME ";" FIM_FLT_BASENAME " _ffn; _bfn='./'." FIM_VID_LAST_CMD_OUTPUT ";" FIM_FLT_LIST " 'pushdir' '.';" FIM_FLT_LIST " 'sort';" FIM_FLT_GOTO " '?'._bfn;" FIM_FLT_RELOAD ";" FIM_FLT_REDISPLAY ";}if(" FIM_VID_FILELISTLEN "==0){" FIM_FLT_STDOUT " 'No files loaded: exiting.';" FIM_FLT_QUIT " 0;}");
		    appendedPostInitCommand=true;
		    break;
		case 'c':
		    //fim's
		    cc.appendPostInitCommand(std::string(optarg));
		    appendedPostInitCommand=true;
		    break;
		case 'C':
		    //fim's
		    if (cc.regexp_match(optarg,"^=[_0-9A-Za-z]+=.+",true))
		    {
			const fim::string oa(optarg+1);
			const int ei = oa.find('=');
			const fim::string vi(oa.substr(0,ei));
			const fim::string vv(oa.substr(ei+1));
		    	cc.setVariable(vi,vv);
		    }
		    else
		    {
		    	cc.appendPreConfigCommand(optarg);
		    	appendedPreConfigCommand=true;
		    }
		    break;
		case 'W':
		    //fim's
		    cc.setVariable(FIM_VID_SCRIPTOUT_FILE,optarg);
	#ifdef FIM_AUTOCMDS
		    cc.pre_autocmd_add(FIM_FLT_RECORDING" 'start';");
		    cc.appendPostExecutionCommand(FIM_FLT_RECORDING" 'stop';");
	#endif /* FIM_AUTOCMDS */
		    break;
		case 'F':
		    //fim's
		    cc.appendPostExecutionCommand(optarg);
		    break;
		case 'E':
		    //fim's
	#ifndef FIM_WANT_NOSCRIPTING
		    cc.push_scriptfile(optarg);
	#else /* FIM_WANT_NOSCRIPTING */
		    cout << FIM_EMSG_NO_SCRIPTING;
	#endif /* FIM_WANT_NOSCRIPTING */
		    break;
		case 'p':
		    //fim's (differing semantics from fbi's)
	#ifndef FIM_WANT_NOSCRIPTING
		    FIM_SROC(read_stdin_choice,Script)
        #else /* FIM_WANT_NOSCRIPTING */
		    cout << FIM_EMSG_NO_SCRIPTING;
        #endif /* FIM_WANT_NOSCRIPTING */
		    break;
		case 0x64646672:
		    //fim's
	//	    cc.setNoFrameBuffer();	// no framebuffer (debug) mode
		    cc.dumpDefaultFimrc();
		    std::exit(0);
		    break;
		case 'N':
		    //fim's
			cc.setVariable(FIM_VID_NO_RC_FILE,1);
		    break;
		case 0x4E4E4E:// NNN
		    //fim's
		    	cc.setVariable(FIM_VID_NO_DEFAULT_CONFIGURATION,1);
		    break;
		case 0x4E4E:// NN
		    //fim's
		    	cc.setVariable(FIM_VID_LOAD_DEFAULT_ETC_FIMRC,0);
		    break;
		case 0x4E434C:// NCL
		    //fim's
		    	cc.setVariable(FIM_VID_CONSOLE_KEY,FIM_CNS_INVALID_KEY_VAL);
		    break;
#if FIM_WANT_HISTORY
		case 0x4E484C:// NHL
		    //fim's
		    	cc.setVariable(FIM_VID_LOAD_FIM_HISTORY,-1);
		    break;
		case 0x4E4853:// NHS
		    //fim's
		    	cc.setVariable(FIM_VID_SAVE_FIM_HISTORY,-1);
		    break;
		case 0x4E48:// NH
		    //fim's
		    	cc.setVariable(FIM_VID_SAVE_FIM_HISTORY,-1);
		    	cc.setVariable(FIM_VID_LOAD_FIM_HISTORY,-1);
		    break;
#endif /* FIM_WANT_HISTORY */
		case 't':
		    //fim's
			#ifdef FIM_USE_ASCII_ART_DEFAULT
		    	g_fim_output_device=FIM_USE_ASCII_ART_DEFAULT;
			#else /* FIM_USE_ASCII_ART_DEFAULT */
			std::cerr << "you should recompile fim with aalib or libcaca support!\n";
			g_fim_output_device=FIM_DDN_INN_DUMB;
			#endif /* FIM_USE_ASCII_ART_DEFAULT */
		    break;
		case 'o':
		    //fim's
#ifdef FIM_WITH_LIBCACA
			if(optarg && optarg == strstr(optarg,"caca"))
			{
				std::cerr << "ehm, it's 'ca', not 'caca', but fine for now.\n";
				optarg += 2;
			}
#endif /* FIM_WITH_LIBCACA */
		    	g_fim_output_device=optarg;
#if FIM_WANT_OUTPUT_DEVICE_STRING_CASE_INSENSITIVE
			{
				int si=g_fim_output_device.find(FIM_SYM_DEVOPTS_SEP_STR);
				if(si>0);else si=g_fim_output_device.end()-g_fim_output_device.begin();
				transform(g_fim_output_device.begin(), si+g_fim_output_device.begin(), g_fim_output_device.begin(),(int (*)(int))tolower);
			}
#endif /* FIM_WANT_OUTPUT_DEVICE_STRING_CASE_INSENSITIVE */
		    break;
		case 0x6472690a:
		    //fim's
		{
			args_t args;
			if(optarg)
				args.push_back(optarg);
			cc.dump_reference_manual(args);
			std::exit(0);
		}
		    break;
#if FIM_WANT_PIC_LISTUNMARK
		case 0x6d666466:
		    if(!fim_args_from_desc_file(argsc,optarg,g_sc))
		    {
			retcode=FIM_PERR_BAD_PARAMS;
			goto ret;
		    }
#endif /* FIM_WANT_PIC_LISTUNMARK */
#if FIM_WANT_PIC_CMTS
		case /*0x69646673*/'S': /* FIM_OSW_IMG_DSC_FILE_SEPC */
		    if(optarg)
			    g_sc = *optarg;
		    break;
		case /*0x6c696466*/'D': /* FIM_OSW_LOAD_IMG_DSC_FILE */
		    if(!cc.id_.fetch(optarg,g_sc))
		    {
			std::cerr << "Descriptions file could not be opened!" << std::endl;
			retcode=FIM_PERR_BAD_PARAMS;
			goto ret;
		    }
#if FIM_WANT_PIC_CMTS_RELOAD
		    cc.browser_.dfl_.push_back({optarg,g_sc});
#endif /* FIM_WANT_PIC_CMTS_RELOAD */
		    cc.setVariable(FIM_VID_COMMENT_OI,FIM_OSW_LOAD_IMG_DSC_FILE_VID_COMMENT_OI_VAL);
		    break;
#endif /* FIM_WANT_PIC_CMTS */
	#ifdef FIM_READ_STDIN
		case '-':
		    //fim's
		    FIM_SROC(read_stdin_choice,FilesList)
		    break;
	#endif /* FIM_READ_STDIN */
		case 0x72667373:
		    sac = *optarg;
		    break;
	#ifdef FIM_READ_STDIN
		case 0:
		    //fim's
		    FIM_SROC(read_stdin_choice,FilesList)
		    break;
	#endif /* FIM_READ_STDIN */
		case 'L':
			fim_load_filelist(optarg,sac,pf);
		    break;
		case '/':
		    //fim's
		    cc.appendPostInitCommand(string( string(FIM_SYM_FW_SEARCH_KEY_STR) + fim_shell_arg_escape(optarg,false) ).c_str());
		    appendedPostInitCommand=true;
		    break;
		case 0x2f2f0000:
		    //fim's
		    cc.appendPostInitCommand( ( string (
			FIM_I2BI(FIM_VID_RE_SEARCH_OPTS) "=" FIM_VID_RE_SEARCH_OPTS ";\n"
			FIM_SYM_CMD_SLSL ";\n"
		    	FIM_SYM_FW_SEARCH_KEY_STR) + fim_shell_arg_escape(optarg,false) + string(";\n"
		    	FIM_VID_RE_SEARCH_OPTS "=" FIM_I2BI(FIM_VID_RE_SEARCH_OPTS) "\n") 
					    ).c_str()
			    );
		    appendedPostInitCommand=true;
		    break;
		case 0x68696768:
#if FIM_EXPERIMENTAL_SHADOW_DIRS
			cc.browser_.push_shadow_dir(std::string(optarg));
#else /* FIM_EXPERIMENTAL_SHADOW_DIRS */
		    std::cerr<< "warning: shadow dir is not supported (need to compile with C++11 enabled for this).\n";
#endif /* FIM_EXPERIMENTAL_SHADOW_DIRS */
		    break;
#if FIM_WANT_CMDLINE_KEYPRESS
		case 'K':
			cc.push_chars_press(optarg);
		    break;
		case 'k':
			cc.push_key_press(optarg);
		    break;
#endif /* FIM_WANT_CMDLINE_KEYPRESS */
		default:
			retcode=FIM_PERR_NO_SUCH_OPTION;
		case 'h':
#if FIM_WANT_HELP_ARGS
		    if(optind < argc)
		    {
		    	for (i = optind; i < argc; i++)
		    		std::cout << cc.get_help(argv[i], optarg?*optarg:'l') << std::endl;
	    		std::exit(retcode);
		    }
#endif /* FIM_WANT_HELP_ARGS */
		    else
		    {
		    	return help_and_exit(argv[0],retcode,optarg);
		    }
		}
	    }
		// fim_fms_t dt;
		// dt = - getmilliseconds();
		for (i = optind; i < argc; i++)
		{
	#ifdef FIM_READ_STDIN
			if(*argv[i]=='-'&&!argv[i][1])
				read_stdin_choice = FilesList;
			else
	#endif /* FIM_READ_STDIN */
			{
				cc.push(argv[i],pf | FIM_FLAG_PUSH_ALLOW_DUPS);
			}
		}
		// dt += getmilliseconds();
		// std::cout << fim::string("fetched images list in ") << dt << " ms" << std::endl;
	
	#ifdef FIM_READ_STDIN	
#if FIM_WANT_OBSOLETE
		if( ( read_stdin_choice == FilesList ) +
		#ifdef FIM_READ_STDIN_IMAGE
		( read_stdin_choice == ImageFile ) + 
		#endif /* FIM_READ_STDIN_IMAGE */
		( read_stdin_choice == Script ) > 1 )
		{
			/* FIXME: this case is now useless. Shall rather implement a warning for when -p and -i overlap. */
			FIM_FPRINTF(stderr, "error : you shouldn't specify more than one standard input reading options among (-, -p"
#ifdef FIM_READ_STDIN_IMAGE
					", -i"
#endif /* FIM_READ_STDIN_IMAGE */
					")!\n\n");
			retcode=help_and_exit(argv[0],FIM_PERR_NO_ERROR,"b");/* should return 0 or -1 ? */
			goto ret;
		}
#endif /* FIM_WANT_OBSOLETE */
		/*
		 * this is Vim's solution for stdin reading
		 * */
		if( read_stdin_choice == FilesList )
		{
			fim_load_filelist(FIM_NULL,sac,pf);
			close(0);
			ndd=dup(2);
		}
		#ifdef FIM_READ_STDIN_IMAGE
		else
		if( read_stdin_choice == ImageFile )
		{
#if !FIM_WANT_STDIN_FILELOAD_AFTER_CONFIG
			cc.fpush(fim_fread_tmpfile(stdin));
			close(0);
			ndd=dup(2);
#endif /* FIM_WANT_STDIN_FILELOAD_AFTER_CONFIG */
		}
		#endif /* FIM_READ_STDIN_IMAGE */
		else
		if( read_stdin_choice == Script )
		{
			if( fim_char_t *buf = slurp_binary_fd(0,FIM_NULL) )
			       	cc.appendPostInitCommand(buf),
			       	appendedPostInitCommand=true,
	//			appendedPreConfigCommand=true,
			       	fim_free(buf);
			close(0);
			ndd=dup(2);
		}
	#endif
#if FIM_WANT_PIC_LISTUNMARK
		cc.browser_.mark_from_list(argsc);
#endif /* FIM_WANT_PIC_LISTUNMARK */
#ifdef FIM_CHECK_DUPLICATES
		if(want_random_shuffle!= 0)
		{
			// if any sorting method is requested, we use chance to sort and remove duplicates
			cc.browser_._sort();
			cc.browser_._unique();
			if( want_random_shuffle==0x736f7274 )
				want_random_shuffle=0; // no need to perform this twice
		}
#endif /* FIM_CHECK_DUPLICATES */
		if(want_random_shuffle== 1)
			cc.browser_._random_shuffle(true);
		if(want_random_shuffle==-1)
			cc.browser_._random_shuffle(false);
		if(want_random_shuffle==0x736f7274)
			cc.browser_._sort();
		if(want_random_shuffle==0x736f626e)
			cc.browser_._sort(FIM_SYM_SORT_BN);
#if FIM_WANT_SORT_BY_STAT_INFO
		if(want_random_shuffle==0x7369626d)
			cc.browser_._sort(FIM_SYM_SORT_MD);
		if(want_random_shuffle==0x73696273)
			cc.browser_._sort(FIM_SYM_SORT_SZ);
#endif /* FIM_WANT_SORT_BY_STAT_INFO */
		if(want_reverse_list==1)
			cc.browser_._reverse();

		if(ndd==-1)
			fim_perror(FIM_NULL);
	
		if(cc.browser_.empty_file_list()
#ifndef FIM_WANT_NOSCRIPTING
			       	&& !cc.with_scriptfile()
#endif /* FIM_WANT_NOSCRIPTING */
			       	&& !appendedPostInitCommand 
			       	&& !appendedPreConfigCommand 
		#ifdef FIM_READ_STDIN_IMAGE
		&& ( read_stdin_choice != ImageFile )
		#endif /* FIM_READ_STDIN_IMAGE */
		&& !perform_sanity_check
		)
		{
			retcode=help_and_exit(argv[0],FIM_PERR_BAD_PARAMS,"b");goto ret;
		}
	
#if FIM_WASM_DEFAULTS
		g_fim_output_device="sdl=w800:400";
#endif /* FIM_WASM_DEFAULTS */
	
		/* output device guess */

		if( FIM_CNS_EMPTY_STRING == g_fim_output_device || '=' == g_fim_output_device[0] )
			g_fim_output_device = guess_output_device() + g_fim_output_device;

#if FIM_WANT_R_SWITCH
		if(rs)
		if(0==g_fim_output_device.find(FIM_DDN_INN_SDL) || 0==g_fim_output_device.find(FIM_DDN_INN_GTK))
		{
			if(g_fim_output_device.find('=')==std::string::npos)
				g_fim_output_device+="=";
			if(fim::string("fullscreen")==rs)
				g_fim_output_device+='W'; // fullscreen
			else
				g_fim_output_device+=rs;
		}
#endif /* FIM_WANT_R_SWITCH */

		if((retcode=FIM_ERR_TO_PERR(cc.init(g_fim_output_device)))!=FIM_PERR_NO_ERROR)
	       	{
			goto ret;
		}
#ifdef FIM_READ_STDIN_IMAGE
#if FIM_WANT_STDIN_FILELOAD_AFTER_CONFIG
		if( read_stdin_choice == ImageFile )
		{
			cc.fpush(fim_fread_tmpfile(stdin));
			close(0);
			//fclose(stdin);
			ndd=dup(2);
		}
#endif /* FIM_WANT_STDIN_FILELOAD_AFTER_CONFIG */
#endif /* FIM_READ_STDIN_IMAGE */
		retcode=cc.execution();/* note that this could not return */
ret:
		return retcode;
	}
};

fim_perr_t main(int argc,char *argv[])
{
	/*
	 * FimInstance will encapsulate all of the fim's code someday.
	 * ...someday.
	 * */
	FimInstance fiminstance;

	fim::fim_var_help_db_init();
	return fiminstance.main(argc,argv);
}

/* 
 * The function FimInstance::show_version declared here needs the following inclusions.
 * Would be cleaner to move the rest of the file to a separate file.
 * */
#ifdef FIM_WITH_LIBPNG
#include <png.h>
#endif /* FIM_WITH_LIBPNG */
#ifdef HAVE_LIBJPEG
#include <jpeglib.h>
#endif /* HAVE_LIBJPEG */
#ifdef FIM_HANDLE_TIFF
#include <tiffio.h>
#endif /* FIM_HANDLE_TIFF */
#ifdef FIM_HANDLE_GIF
#include <gif_lib.h>
#endif /* FIM_HANDLE_GIF */
#ifdef FIM_USE_READLINE
#include "readline.h"
#endif /* FIM_USE_READLINE */
#if 0 /* namespace clashes */
#ifdef HAVE_LIBGRAPHICSMAGICK
#include <magick/api.h>
#include <magick/version.h>
#endif /* HAVE_LIBGRAPHICSMAGICK */
#endif
#ifdef HAVE_LIBSPECTRE
extern "C" {
#include <libspectre/spectre.h>
}
#endif /* HAVE_LIBSPECTRE */

///#ifdef HAVE_LIBPOPPLER
//#include <poppler/PDFDoc.h> // getPDFMajorVersion getPDFMinorVersion
//#endif /* HAVE_LIBPOPPLER */

	void FimInstance::show_version(FILE * stream)
	{
	    FIM_FPRINTF(stream, 
			    FIM_CNS_FIM" "
	#ifdef FIM_VERSION
			    FIM_VERSION
	#endif /* FIM_VERSION */
	#ifdef SVN_REVISION
			    " ( repository version "
		SVN_REVISION
			    " )"
	#else /* SVN_REVISION */
	/* obsolete */
	# define FIM_REPOSITORY_VERSION  "$LastChangedDate: 2024-04-29 14:03:11 +0200 (Mon, 29 Apr 2024) $"
	# ifdef FIM_REPOSITORY_VERSION 
			    " ( repository version "
		FIM_REPOSITORY_VERSION 	    
			    " )"
	# endif /* FIM_REPOSITORY_VERSION  */
	#endif /* SVN_REVISION */
	#ifdef FIM_AUTHOR 
			    ", by "
			    FIM_AUTHOR
	#endif /* FIM_AUTHOR  */
#define FIM_WANT_REPRODUCIBLE_BUILDS 1
#if !FIM_WANT_REPRODUCIBLE_BUILDS
			    ", built on %s\n",
			    __DATE__
#endif
	    		    " ( based on fbi version 1.31 (c) by 1999-2004 " FBI_AUTHOR_NAME " )\n"
	#ifdef __cplusplus
	"Compiled with C++ standard: " FIM_XSTRINGIFY(__cplusplus) "\n"
	#else /* __cplusplus */
	""
	#endif /* __cplusplus */
	#ifdef FIM_WITH_LIBPNG
	#ifdef PNG_HEADER_VERSION_STRING 
	"Compiled with " PNG_HEADER_VERSION_STRING ""
	#endif /* PNG_HEADER_VERSION_STRING */
	#endif /* FIM_WITH_LIBPNG */
	#ifdef FIM_HANDLE_GIF
	#if defined(GIFLIB_MAJOR) && defined(GIFLIB_MINOR) && defined(GIFLIB_RELEASE)
	"Compiled with libgif, " FIM_XSTRINGIFY(GIFLIB_MAJOR) "." FIM_XSTRINGIFY(GIFLIB_MINOR) "." FIM_XSTRINGIFY(GIFLIB_RELEASE) ".\n"
	#else /* GIFLIB_MAJOR GIFLIB_MINOR GIFLIB_RELEASE */
	#ifdef GIF_LIB_VERSION
	"Compiled with libgif, " GIF_LIB_VERSION ".\n"
	#endif /* GIF_LIB_VERSION */
	#endif /* GIFLIB_MAJOR GIFLIB_MINOR GIFLIB_RELEASE */
	#endif /* FIM_HANDLE_GIF */
	#ifdef HAVE_LIBJPEG
	#ifdef JPEG_LIB_VERSION
	"Compiled with libjpeg, v." FIM_XSTRINGIFY(JPEG_LIB_VERSION) ".\n"
	#endif /* JPEG_LIB_VERSION */
	#endif /* HAVE_LIBJPEG */
	#ifdef HAVE_LIBSPECTRE
	#ifdef SPECTRE_VERSION_STRING
	"Compiled with libspectre, v." SPECTRE_VERSION_STRING ".\n"
	#endif /* SPECTRE_VERSION_STRING */
	#endif /* HAVE_LIBSPECTRE */
#if 0 /* namespace clashes */
	#ifdef HAVE_LIBGRAPHICSMAGICK
	#ifdef MagickLibVersionText
	"Compiled with GraphicsMagick, v."MagickLibVersionText".\n"
	#endif /* MagickLibVersionText */
	#endif /* HAVE_LIBGRAPHICSMAGICK */
#endif
	#ifdef FIM_USE_READLINE
	// TODO: shall use RL_READLINE_VERSION instead
	#if defined(RL_VERSION_MINOR) && defined(RL_VERSION_MAJOR) && ((RL_VERSION_MAJOR)>=6)
	"Compiled with readline, v." FIM_XSTRINGIFY(RL_VERSION_MAJOR) "." FIM_XSTRINGIFY(RL_VERSION_MINOR) ".\n"
	#else
	"Compiled with readline, version unknown.\n"
	#endif /* defined(RL_VERSION_MINOR) && defined(RL_VERSION_MAJOR) && ((RL_VERSION_MAJOR)>=6) */
	#endif /* FIM_USE_READLINE */
	// for TIFF need TIFFGetVersion
	#ifdef FIM_CONFIGURATION
			"Configuration invocation: " FIM_CONFIGURATION "\n" 
	#endif /* FIM_CONFIGURATION */
#if FIM_WITH_DEBUG
	#ifdef CXXFLAGS
			"Compile flags: CXXFLAGS=" CXXFLAGS
	#ifdef CFLAGS
			"  CFLAGS=" CFLAGS
	#endif /* CFLAGS */
			"\n"
	#endif /* CXXFLAGS */
#endif /* FIM_WITH_DEBUG */
			"Fim options (features included (+) or not (-)):\n"
	#include "version.h"
	/* i think some flags are missing .. */
		"\nSupported output devices (for --" FIM_OSW_OUTPUT_DEVICE "): "
	#ifdef FIM_WITH_AALIB
		" " FIM_DDN_INN_AA
	#endif /* FIM_WITH_AALIB */
	#ifdef FIM_WITH_LIBCACA
		" " FIM_DDN_INN_CACA
	#endif /* FIM_WITH_LIBCACA */
	#ifdef FIM_WITH_LIBIMLIB2
		" " FIM_DDN_INN_IL2
	#endif /* FIM_WITH_LIBIMLIB2 */
	#ifdef FIM_WITH_LIBSDL
		" " FIM_DDN_INN_SDL
	#endif /* FIM_WITH_LIBSDL */
	#ifdef FIM_WITH_LIBGTK
		" " FIM_DDN_INN_GTK
	#endif /* FIM_WITH_LIBGTK */
#ifndef FIM_WITH_NO_FRAMEBUFFER
		" " FIM_DDN_INN_FB
#endif //#ifndef FIM_WITH_NO_FRAMEBUFFER
	#if 1
		" " FIM_DDN_INN_DUMB
	#endif
		"\n"
		"\nSupported file formats: "
#ifdef ENABLE_PDF
		" pdf"
#endif /* ENABLE_PDF */
#ifdef HAVE_LIBSPECTRE
		" ps"
#endif /* HAVE_LIBSPECTRE */
#ifdef HAVE_LIBDJVU
		" djvu"
#endif /* HAVE_LIBDJVU */
#ifdef HAVE_LIBJPEG
		" jpeg"
#endif /* HAVE_LIBJPEG */
#ifdef FIM_HANDLE_TIFF
		" tiff"
#endif /* FIM_HANDLE_TIFF */
#ifdef FIM_HANDLE_GIF
		" gif"
#endif /* FIM_HANDLE_GIF */
#ifdef FIM_WITH_LIBPNG
		" png"
#endif /* FIM_WITH_LIBPNG */
		" ppm"	/* no library is needed for these */
		" bmp"
#ifdef HAVE_MATRIX_MARKET_DECODER
		" mtx (Matrix Market)"
#endif /* HAVE_MATRIX_MARKET_DECODER */
		"\n"
			    );
		
		fim_loaders_to_stderr(stream);
	} /* FimInstance::show_version */

/* Please do not write any more code after THIS declaration */
