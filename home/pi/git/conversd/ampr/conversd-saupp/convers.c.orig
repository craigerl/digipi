static const char rcsid[] = "@(#) $Id: convers.c,v 1.3 2005/02/15 16:31:28 dl9sau Exp $";

/*
 * This is Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@hpfcmdd.fc.hp.com>
 *
 * Modifications by Fred Baumgarten <dc6iq@insl1.etec.uni-karlsruhe.de>
 * $Revision: 1.3 $$Date: 2005/02/15 16:31:28 $
 *
 * $Log: convers.c,v $
 * Revision 1.3  2005/02/15 16:31:28  dl9sau
 * 	made conversd-saupp compile on macosx (should also compile on
 * 	freebsd again)
 *
 * Revision 1.2  2003/11/25 11:58:28  dl9sau
 *         - gcc3.3 now compiles saupp without warnings; had to change the
 *           name of the function log()
 *         - conversd.c: trigger message for users which are not in a channel
 *         - README.saupp: fixed typo for cvs checkout
 *
 * Revision 1.1.1.1  2003/06/09 11:10:08  dl9sau
 * DEVEL 1.62a now in HEAD
 *
 * Revision 1.11.8.1.4.16  2003/06/09 11:10:08  dl9sau
 * 	small bugfix for rawconvers with specified connect string commands
 *
 * Revision 1.11.8.1.4.15  2003/01/04 23:55:25  dl9sau
 * 	convers client:
 * 	  - fixed potential overflow
 * 	  - typo in RL_READLINE_VERSION check
 *
 * Revision 1.11.8.1.4.14  2003/01/03 22:36:07  dl9sau
 * 	older versions of libreadline have no RL_READLINE_VERSION :(
 *
 * Revision 1.11.8.1.4.13  2003/01/03 22:29:21  dl9sau
 * 	there's an error with libreadline4 where rl_event_hook is
 * 	buggy when multible lines are read at once. it's unknown
 * 	for how long this bug exists. it's documented in
 *   	http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=144585
 * 	this patch fixes the problem, which may occur again if
 *   	libreadline will not be fixed in future versions:
 *   	rl_event_hook = 0;
 *   	if (RL_READLINE_VERSION <= 0x042)
 *     	  rl_event_hook = (Function *) do_select_call;
 *
 * Revision 1.11.8.1.4.12  2003/01/03 17:52:57  dl9sau
 * 	cleaned up getopt() args for the various compiletime versions
 *
 * Revision 1.11.8.1.4.11  2003/01/03 14:02:12  dl9sau
 * 	c_cc[VMIN] = 1 initialization necessary for readline() to work
 * 	properly. debian woody on sparc has (errornously?) stty min 0
 * 	by default
 *
 * Revision 1.11.8.1.4.9  2002/10/06 20:43:30  dl9sau
 * 	fixes for linux on mips CPU
 *
 * Revision 1.11.8.1.4.8  2002/09/30 22:43:57  dl9sau
 * 	show login messages
 *
 * Revision 1.11.8.1.4.7  2002/09/30 22:21:24  dl9sau
 * 	more work on do_select_call locations
 *
 * Revision 1.11.8.1.4.6  2002/09/21 13:39:53  dl9sau
 * 	better so
 *
 * Revision 1.11.8.1.4.5  2002/09/20 22:57:34  dl9sau
 * 	conversd.c:
 * 	  - bugfix (buffer overvlow) while reading from CT_USER
 * 	  - sul problem with delay of CT_UNKNOWN -> CT_USER
 * 	  - now reading input buffer even if CT_CLOSED (it's for the kernel
 * 	    health ;)
 * 	  - version.h: built date is now set automatically by the compiler
 * 	convers.c:
 * 	  - now safe to signals (tty input INTR, SUSP or QUIT)
 * 	  - do_select_call() after initial writes -> now display of
 * 	    conversd greating is more robust (esp. for deny messages)
 *
 * Revision 1.11.8.1.4.4  2002/05/26 14:26:20  dl9sau
 * 	new special channel: -3 -> ask for /lo <channel>
 *
 * Revision 1.11.8.1.4.3  2002/05/15 01:19:49  dl9sau
 * 	- a linux write() really sucks fix.
 * 	  hope now we are conform to how kernel 2.4.x likes peaceful programs
 *
 * Revision 1.11.8.1.4.2  2002/05/08 15:14:45  dl9sau
 * 	linux 2.4.x uses -EAGAIN on write if the internal socket stack
 * 	of the kernel is full. this is no error, thus no reason to
 * 	disconnect, but we have to try until it succeeds.
 *
 * Revision 1.11.8.1  2002/02/23 15:51:16  dl9sau
 * 	extended convers.c
 *
 * Revision 1.11  2002/01/08 15:08:41  dl9sau
 * 	- convers user client now with libreadline
 *
 * Revision 1.10  2001/12/20 11:30:12  dl9sau
 * 	- bugfix in ax25 permlink connection code
 * 	- convers is now fully ax25 socket compatible
 *
 * Revision 1.9  2001/12/20 10:18:12  dl9sau
 * 	- testing: ax25 sockets for rawconvers
 *
 * Revision 1.8  2001/12/16 05:03:31  dl9sau
 * 	- made it compile again on kernel 2.0.x on very old linuxes
 * 	  (tested on old slackware: libc5, gcc272, kernel-2.0.38)
 * 	  don't forget to comment out in Makefile:
 * 		-DFORKPTY and  -lutil if you don't have pty.h / libutil.so
 * 	- cleaned up the makefile
 * 	- changed subversion to 1045
 *
 * Revision 1.7  2001/12/03 01:42:04  dl9sau
 *         - modified rawconvers.c with a -DNO_IOCTL flag. it now may
 *           be run directly out of axspawn for deligating for e.g.
 *           permlinks (in the case you don't want directly use the ax25 Listener
 *           of conversd)
 *         - please read doc/TIPS.txt on that topic
 *         - adapted the makefile
 *
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <pwd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>

#include "config.h"
#include "ba_stub.h"

#ifdef	NO_IOCTL
#ifndef	RAW
#define	RAW
#endif
#ifndef	NO_SHELL
#define	NO_SHELL
#endif
#undef	HAVE_LIBSPEAK
#endif

#ifdef HAVE_LIBSPEAK
#include <linux/autoconf.h>
#if CONFIG_PCSP
#include <linux/pcsp.h>
extern int speak_open(char *device, int now);
extern int speak_string(int fd, char *str);
extern int speak_close(int fd);
extern int speak_load_samples(char *dir);
#else
#undef HAVE_LIBSPEAK
#endif
#endif

#ifndef	NO_IOCTL
#ifdef linux
#include <sys/ioctl.h>
#endif
#endif

#ifdef RAW
#undef HAVE_LIBREADLINE
#endif

#ifdef HAVE_LIBREADLINE
#if HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#include <readline/history.h>
#elif HAVE_READLINE_H
#include <readline.h>
#include <history.h>
#endif
extern char *rl_display_prompt;
#endif

#if defined(__TURBOC__) || defined(__OS2__) || defined(STDC_HEADERS) || defined(linux)
#define __ARGS(x)       x
#else
#define __ARGS(x)       ()
#endif

extern char *optarg;
extern int optind;

#if defined(RAW) || defined(AX25)
static int issue_running = 0;
#else
//static int issue_running = 1;	/* ignore these Issue messages */
static int issue_running = 0;	/* no! */
#endif

static char convtype[512];
static int outcnt = 0;
static char outbuf[2048];
static struct timeval sel_timeout;
#ifndef	NO_IOCTL
static struct termios prev_termios;
static struct termios curr_termios;
#endif
static int echo;

#ifndef	__linux__
#if defined(mips)	/* select(2) is not POSIX-compatible !!! */
#undef POSIX
#endif
#endif

#ifdef HAVE_LIBREADLINE
static char prompt[256];
static int prompt_len = 0;
static int chars_avail = 0;
static int output = 0;
static int screenwidth;
static struct winsize window_size;
#else
#ifndef	__MACH__
static void stop __ARGS((char *arg));
#endif
#endif

// pre-define a login channel (default is -1: conversd's configured default
// channel)
#define	DEFAULT_CHAN	-1
// join a random free channel
//#define	DEFAULT_CHAN	-2
// ask for a channel
//#define	DEFAULT_CHAN	-3

#ifdef LIBSPEAK
static char *audiodir = "/usr/local/lib/phonemes";
static int speak_fd = 0;
static int do_talk = 0;
#endif

#define	OUT_INET	1
#define	OUT_AX25	2
static int out_type = 0;

void write_buffer(char *buf, int cnt);

// hack for ba_stub.c
char *hstrdup(const char *s) {
  return strdup(s);
}
void hfree(void *ptr) {
  free(ptr);
}
int do_log(int level, const char *fmt, ...) {
  if (level > 2)
	  return 0;
  fprintf(stderr, fmt);
  return fflush(stderr);
}

/*---------------------------------------------------------------------------*/

static void stop(char *arg)
{
    if (*arg) perror(arg);
#ifndef	NO_IOCTL
    tcsetattr(0, TCSANOW, &prev_termios);
#endif
    exit(0);
}

/*---------------------------------------------------------------------------*/

// dl9sau
// non-blocking IO on linux 2.4.x uses EAGAIN quite often
ssize_t _write(int fd, const void *buf, size_t count)
{
	int len;
	while (((len = write(fd, buf, count)) == -1) || len < count) {
		if (len == -1) {
			if (errno != EAGAIN)
				return -1;
		} else {
			buf += len;
			count -= len;
		}
		sleep(1);
	}
	return len;
}

/*---------------------------------------------------------------------------*/

// non-blocking IO on linux 2.4.x uses EAGAIN quite often
ssize_t _read(int fd, void *buf, size_t count)
{
	int len;
	while ((len = read(fd, buf, count)) == -1) {
		if (errno != EAGAIN)
			return -1;
		sleep(1);
	}
	return len;
}

/*---------------------------------------------------------------------------*/

#ifdef HAVE_LIBSPEAK

void convers_speak_string(s, len)
char *s;
int len;
{
    char buffer[10*1024];
    int i,j;

    for (i = 0,j = 0; i < len; i++) {
	if (s[i] == ' ' || s[i] == '<' || s[i] == '>' ||
	    (s[i] >= 'a' && s[i] <= 'z') ||
	    (s[i] >= 'A' && s[i] <= 'Z') ||
	    (s[i] >= '0' && s[i] <= '9')) {
	    buffer[j++] = s[i];
	}
    }
    buffer[j] = '\0';
  
    /*
     * Now split the resulting string:
     *
     *   - if <something> look for callsign-sound
     *   - speak the rest
     */

    if (buffer[0] == '<') {
	char *rest, callsign[512], filename[512];
	rest = strchr(buffer, '>');
	if (rest && rest - buffer < 512) {
	    bzero(callsign, 20);
	    strncpy(callsign, buffer+1, rest-buffer-1);
	    rest++;
	    sprintf(filename, "%s/%s.au", audiodir, callsign);

	    /*
	     * now look if callsign is sampled
	     */

	    if (!access(filename, W_OK)) {                        /* Garf produced a horrible code here */
		sprintf(callsign, "cp %s /dev/audio", filename);             /* this NEEDS rewrite... :-( */
		speak_close(speak_fd);
		system(callsign);
		speak_fd = speak_open("/dev/audio", 1);
		speak_string(speak_fd, rest);
		return;
	    }
	}
    }
    speak_string(speak_fd, buffer);
}

#endif

/*---------------------------------------------------------------------------*/

static int select_read_only_from_socket = 1;

void do_select_call(void)
{
    fd_set mask;
    int size;
    int i;
#ifndef HAVE_LIBREADLINE
    int incnt = 0;
    char inbuf[sizeof(outbuf)+1];
    static int in_skip_next_lf = 0;
#endif
    static int out_skip_next_lf = 0;
    char buffer[sizeof(outbuf)];
    char c;

    FD_ZERO(&mask);
    FD_SET(0,&mask);
    FD_SET(3,&mask);

#ifdef	AX25
    sel_timeout.tv_sec = 0;
    sel_timeout.tv_usec = 200000;
#else
    sel_timeout.tv_sec = 10;
    sel_timeout.tv_usec = 0;
#endif
    select(4, &mask, (fd_set *) 0, (fd_set *) 0, &sel_timeout);
    if (FD_ISSET(0,&mask) && !select_read_only_from_socket) {
#ifdef HAVE_LIBREADLINE
	chars_avail = 1;
#else
	do {
	    if ((size = _read(0, buffer, sizeof(buffer))) <= 0) stop(convtype);
	    for (i = 0; i < size; i++) {
		c = buffer[i];
		if (c == '\r') {
			in_skip_next_lf = 1;
			c = '\n';
		} else {
			if (c == '\n') {
				if (in_skip_next_lf) {
					in_skip_next_lf = 0;
					continue;
				}
			}
			in_skip_next_lf = 0;
		}
#ifndef	NO_IOCTL
		if (c == (char) prev_termios.c_cc[VERASE]) {
		    if (incnt) {
			incnt--;
			if (echo && _write(1, "\b \b", 3) < 0) stop(convtype);
		    }
		} else if (c == (char) prev_termios.c_cc[VKILL]) {
		    for (; incnt; incnt--)
			if (echo && _write(1, "\b \b", 3) < 0) stop(convtype);
		} else if (echo && c == 18) {
		    if (_write(1, "^R\n", 3) < 0) stop(convtype);
		    if (_write(1, inbuf, incnt) < 0) stop(convtype);
		} else
#endif
		{
		    inbuf[incnt++] = c;
		    if (echo && _write(1, &c, 1) < 0) stop(convtype);
		}
		if (!incnt)
			continue;
		if (c == '\n' || incnt == sizeof(inbuf) - 1) {
		    if (c == '\n' && (inbuf[0] != '!' || (inbuf[0] == '!' && inbuf[1] == '!'))) {
		    	if (out_type == OUT_INET || out_type == OUT_AX25) {
				inbuf[incnt-1] = '\r';
				if (out_type == OUT_INET) {
					inbuf[incnt++] = '\n';
				}
			}
		    }

		    if (inbuf[0] == '!') {
#ifndef NO_SHELL
			if (inbuf[1] != '!') {
			    inbuf[incnt] = '\0';
			    if (tcsetattr(0, TCSANOW, &prev_termios)) stop(convtype);
			    system(inbuf + 1);
			    if (tcsetattr(0, TCSANOW, &curr_termios)) stop(convtype);
			    if (_write(3, "\n", 1) < 0) stop(convtype);
			} else {
			    if (_write(3, inbuf + 1, incnt - 1) < 0) stop(convtype);
			}
#endif
		    } else {
			if (_write(3, inbuf, incnt) < 0) stop(convtype);
		    }
		    incnt = 0;
#ifdef POSIX
		    fflush(NULL);
#endif
		}
	    }
	} while (incnt);
#endif
    }

    if (FD_ISSET(3,&mask)) {
	size = _read(3, buffer, sizeof(buffer));
	if (size <= 0) stop("");
#ifdef HAVE_LIBSPEAK
	if (speak_fd)
	    convers_speak_string(buffer,size);
#endif
#ifdef HAVE_LIBREADLINE
	if (rl_end && echo) {
	    int i;

	    if (_write(1, "\r", 1) < 0) stop("");
	    for(i = 0; i < screenwidth-1; i++)
		if (_write(1, " ", 1) < 0) stop("");
	    if (_write(1, "\r", 1) < 0) stop("");
	    write_buffer(prompt, prompt_len);
	}
	prompt[0] = 0;
	prompt_len = 0;
#endif
	for (i = 0; i < size; i++) {
	    c = buffer[i];
	    switch (c) {
	    case '\n':
		if (out_skip_next_lf)
			break;
		out_skip_next_lf = 0;
		// fallthrough
	    case '\r':
	    	if (c == '\r')
			out_skip_next_lf = 1;
		outbuf[outcnt++] = '\n';
		write_buffer(outbuf, outcnt);
		outcnt = 0;
#ifdef HAVE_LIBREADLINE
		output = 1;
#endif
		break;

	    default:
		out_skip_next_lf = 0;
		outbuf[outcnt++] = c;
	    }
	}
#ifdef POSIX
	fflush(NULL);
#endif
    } else {
	if (outcnt) {
#ifdef HAVE_LIBREADLINE
	    strncpy(prompt, outbuf, sizeof(prompt)-1);
	    prompt_len = (outcnt < sizeof(prompt) ? outcnt: sizeof(prompt)-1);
	    prompt[prompt_len] = 0;
#endif
	    write_buffer(outbuf, outcnt);
	    outcnt = 0;
	}
#ifdef HAVE_LIBREADLINE
	if (output) 
	    if (rl_end) {
		if (echo)
		    if (_write(1, "\r", 1) < 0) stop("");
		rl_forced_update_display();
		output = 0;
	    }
#endif
    }
}

/*---------------------------------------------------------------------------*/

void write_buffer(outbuf, outcnt)
char *outbuf;
int outcnt;
{
    char *cp;

    if (!issue_running) {
	if (_write(1, outbuf, outcnt) < 0) stop("");
    } else {
	cp = outbuf;
	while (outcnt) {
	    if (*cp != '*') {
		if (_write(1, cp, outcnt) < 0) stop("");
		issue_running = 0;
		break;
	    }
	    do {
		cp++;
		outcnt--;
	    } while (*cp != '\n' && outcnt != 0);
	    if (outcnt != 0) {
		cp++;
		outcnt--;
	    }
	}
    }
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
    char server[128];
    char buffer[512];
    char getopt_opts[64];
    char *sp;
    int ch;
    int errflag = 0;
#if !defined(AX25) && !defined(RAW)
    int channel = DEFAULT_CHAN;
    FILE *f;
    char fname[512];
    char *logname = NULL;
#endif
    int addrlen, daddrlen;
    struct sockaddr *addr, *daddr;
#ifdef HAVE_LIBREADLINE
    char *line;
#endif
    char *p;
#if	defined(AX25) || defined(RAW)
    char *init_string = 0;
#endif
    int i;

#ifdef AX25
    strcpy(server, "unix:/tcp/.sockets/netcmd");
#elif RAW
    *server = 0;
#else
    sprintf(server, "%s:3600", CONVERSHOST);
#endif
    sp = strrchr(argv[0],'/');
    if (sp) {
	sp++;
    } else {
	sp = argv[0];
    }

    if ((i = strlen(sp)) > sizeof(convtype)-1)
      i = sizeof(convtype)-1;
    strncpy(convtype, sp, i);
    convtype[i] = 0;
#if	!defined(AX25) && !defined(RAW)
#ifndef	HAVE_AF_UNIX
    if (strstr(convtype, "kaconvers")) sprintf(server, "%s:6809", CONVERSHOST);
    if (strstr(convtype, "lconvers")) sprintf(server, "%s:6810", CONVERSHOST);
    if (strstr(convtype, "suconvers")) sprintf(server, "%s:6811", CONVERSHOST);
    if (strstr(convtype, "wconvers")) sprintf(server, "%s:3610", CONVERSHOST);
#else
    sprintf(server, "unix:%s/sockets/%s", DATA_DIR, convtype);
#endif
#endif

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SIG_IGN);	/* interrupt */
    signal(SIGQUIT, SIG_IGN);	/* quit */

#ifdef	NO_IOCTL
    echo = 0;
#else
    signal(SIGTSTP, SIG_IGN);	/* suspend */
    if (tcgetattr(0, &prev_termios)) stop(convtype);
    memcpy(&curr_termios, &prev_termios, sizeof(struct termios));
    echo = curr_termios.c_lflag & ECHO;
    curr_termios.c_lflag = 0;
    curr_termios.c_cc[VMIN] = 1;
    curr_termios.c_cc[VTIME] = 0;
#ifdef HAVE_LIBREADLINE
    screenwidth = 80;
#if defined (TIOCGWINSZ)
    if (ioctl (0, TIOCGWINSZ, &window_size) == 0)
    {
	screenwidth = (int) window_size.ws_col;
    }
#endif
    curr_termios.c_lflag = curr_termios.c_lflag & ~ECHO;
    // at the time of writing, rl_event_hook is buggy
    // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=144585
    rl_event_hook = 0;
#ifdef	RL_READLINE_VERSION /* older version have no RL_READLINE_VERSION :( */
    if (RL_READLINE_VERSION < 0x0400)
    	rl_event_hook = (Function *) do_select_call;
#endif
#endif
    if (tcsetattr(0, TCSANOW, &curr_termios)) stop(convtype);
#endif

    *getopt_opts = 0;

    // options witht argument
#if	defined(AX25) || defined(RAW)
    strcat(getopt_opts, "i:");
#endif

#ifndef	AX25
    strcat(getopt_opts, "s:");
#ifndef	RAW
    strcat(getopt_opts, "c:");
#ifdef SETLOGNAMES
    strcat(getopt_opts, "l:");
#endif
#endif/*RAW*/
#ifdef HAVE_LIBSPEAK
    strcat(getopt_opts, "a:");
#endif
#endif/*AX25*/

    // options without argument
#ifndef AX25
#ifdef HAVE_LIBSPEAK
    getopt_opts = "t";
#endif
#endif/*AX25*/

    while ((ch = getopt(argc, argv, getopt_opts)) != EOF)
	switch (ch) {
#ifdef HAVE_LIBSPEAK
	case 'a':
	    audiodir = optarg;
	    speak_load_samples(audiodir);
	    break;
#endif
#if !defined(AX25) && !defined(RAW)
	case 'c':
	    channel = atoi(optarg);
	    break;
#ifdef SETLOGNAMES
	case 'l':
	    logname = optarg;
	    break;
#endif
#endif
	case 's':
	    if ((i = strlen(optarg)) > sizeof(server)-1)
	      i = sizeof(server)-1;
	    strncpy(server, optarg, i);
	    server[i] = 0;
	    break;
#ifdef HAVE_LIBSPEAK
	case 't':
	    do_talk = 1;
	    break;
#endif
#if	defined(AX25) || defined(RAW)
	    case 'i':
	    init_string = optarg;
	    break;
#endif
	case '?':
	    errflag = 1;
	    break;
    }

    if (errflag ||
#ifdef	AX25
		optind != argc -1
#else
		optind < argc
#endif
			|| !*server) {
	fprintf(stderr, "usage: %s", convtype);
#if	defined(AX25) || defined(RAW)
	fprintf(stderr, " [-i initstring]");
#endif
#ifdef	AX25
        fprintf(stderr, " call");
#else
#ifdef HAVE_LIBSPEAK
	fprintf(stderr, " [-t [-a audiodir]]");
#endif
#ifdef RAW
        fprintf(stderr, " -s host:service");
#else
        fprintf(stderr, " [-c channel]");
#ifdef SETLOGNAMES
        fprintf(stderr, " [-l callsign]");
#endif
        fprintf(stderr, " [-s host:service]");
#endif/*RAW*/
#endif/*AX25*/
	fprintf(stderr, "\n");

#if	defined(AX25) || defined(RAW)
	fprintf(stderr, "         initstring example:");
	fprintf(stderr, " foo\\nbar\\nfoo bar\\n\\60\\nfoobar\n");
	fprintf(stderr, "           writes the lines 'foo', then 'bar', then 'foo bar',\n");
	fprintf(stderr, "           waits 60s, then finaly writes the line 'foobar'\n");
#endif
	stop("");
    }

    if (!strncmp(server, "ax25:", 5)) {
	out_type = OUT_AX25;
	for (p = server; *p; p++)
		if (*p == ',')
			*p = ' ';
    } else if (strncmp(server, "unix:", 5)) {
	out_type = OUT_INET;
    }

    close(3);
    if ((p = strchr(server, '<'))) {
	*p++ = 0;
    }
    if (!(addr = build_sockaddr(server, &addrlen))) {
	stop("build_sockaddr()");
    }
#ifdef HAVE_AF_AX25
    if (socket(addr->sa_family, (addr->sa_family == AF_AX25 ? SOCK_SEQPACKET : SOCK_STREAM), 0) != 3) stop("socket");
#else
    if (socket(addr->sa_family, SOCK_STREAM, 0) != 3) stop("socket");
#endif

    /* source address? */
    daddr = calloc(1, addrlen);
    memcpy(daddr, addr, addrlen);
    daddrlen = addrlen;

    if (p && *p) {
	char tmpbuf[1024];
	if (!strncmp(server, "ax25:", 5)) {
		sprintf(tmpbuf, "ax25:%.120s", p);
	} else if (!strncmp(server, "unix:", 5))
		sprintf(tmpbuf, "unix:%.120s", p);
	else {
		if ((i = strlen(p)) > sizeof(tmpbuf)-1)
		  i = sizeof(tmpbuf-1);
		strncpy(tmpbuf, p, i);
		tmpbuf[i] = 0;
	}
    	if (!(addr = build_sockaddr(tmpbuf, &addrlen))) {
		stop("build_sockaddr()");
	}
	if (bind(3, addr, addrlen))
		stop("bind()");
    }
    if (connect(3, daddr, daddrlen)) stop("connect");

#ifdef HAVE_LIBSPEAK
    if (do_talk) {
	speak_fd = speak_open("/dev/audio",1);
    }
#endif

#ifdef AX25
    sprintf(buffer, "connect ax25 %s\n", argv[optind]);
    if (_write(3, buffer, strlen(buffer)) < 0) stop("write(\"connect ax25\")");
#endif

#if defined(AX25) || defined(RAW)
    for (p = init_string, i = 0; p; p++) {
	    if (*p && *p == '\\' && *(p+1) && isdigit(*(p+1))) {
		    int sleeptime = 0;
		    while (*(p+1) && isdigit(*(p+1))) {
			    sleeptime = sleeptime * 10 + (*(p+1) - '0');
			    p++;
		    }
		    sleep(sleeptime);
		    continue;
	    }
	    if (!*p || (*p == '\\' && *(p+1) == 'n') || (i == (sizeof(buffer)-3))) {
		    buffer[i] = 0;
                    if (out_type == OUT_AX25 || out_type == OUT_INET)
	                    strcat(buffer, "\r");
                    if (out_type != OUT_AX25)
	                    strcat(buffer, "\n");
                    if (_write(3, buffer, strlen(buffer)) < 0) stop("write(init_string)");
		    do_select_call();
		    sleep(1);
		    if (!*p) {
			    break;
		    }
		    p++; // skip 'n'
		    i = 0;
		    continue;
	    }
	    buffer[i++] = *p;
    }
#else
#ifdef	SETLOGNAMES
    if (logname == NULL)
	logname = getenv("LOGNAME");
#endif
    if (logname == NULL) {
	logname = getlogin();
	if (!logname) logname = getpwuid(getuid())->pw_name;
    }
    if (logname == NULL)
	logname = "n0call";
    // if not forced with -c 0, we let conversd decide where to join (default-channel)
    //if (channel >= 0) {
    	sprintf(buffer, "/NAME %s %d\n", logname, channel);
    //} else {
    	//sprintf(buffer, "/NAME %s\n", logname);
    //}

    if (_write(3, buffer, strlen(buffer)) < 0) stop(convtype);

    do_select_call();

    /* read .conversrc */
    sprintf(fname,"%s/.%src",getenv("HOME"),convtype);
    if (!access(fname,R_OK)) {
	f = fopen(fname,"r");
	if (f != NULL) {
	    while (fgets(buffer, sizeof(buffer), f)) {
		if (_write(3, buffer, strlen(buffer)) < 0)
			stop(convtype);
                do_select_call();
	    }
	    fclose(f);
	}
    }
#endif/*AX25||RAW*/

#ifdef HAVE_LIBREADLINE
    rl_variable_bind("horizontal-scroll-mode", "On");
    rl_display_prompt = prompt;
#endif

    select_read_only_from_socket = 0;
    for (; ; ) {
#ifdef HAVE_LIBREADLINE
	if (chars_avail) {
	    if (echo)
		if (_write(1, "\r", 1) < 0) stop("");
#ifndef	NO_IOCTL
	    curr_termios.c_lflag = curr_termios.c_lflag | echo;
	    if (tcsetattr(0, TCSANOW, &curr_termios)) stop(convtype);
#endif
	    line = readline(prompt);
#if 0
	    signal(SIGALRM, SIG_IGN);
#endif
#ifndef	NO_IOCTL
	    curr_termios.c_lflag = curr_termios.c_lflag & ~ECHO;
	    if (tcsetattr(0, TCSANOW, &curr_termios)) stop(convtype);
#endif
	    if (line) {
		if (*line) {
		    add_history(line);
		}
		if (line[0] == '!') {
#ifndef NO_SHELL
		    if (line[1] != '!') {
			if (tcsetattr(0, TCSANOW, &prev_termios)) stop(convtype);
			system(line + 1);
			if (tcsetattr(0, TCSANOW, &curr_termios)) stop(convtype);
		    } else {
			if (_write(3, line + 1, strlen(line) - 1) < 0) stop(convtype);
		    }
#endif
		} else {
		    if (_write(3, line, strlen(line)) < 0 || _write(3, "\n", 1) < 1) stop(convtype);
		}
		chars_avail = 0;
		free (line);
		*prompt = 0;
		prompt_len = 0;
	    }
	    rl_end = 0;
	}
#endif
	do_select_call();
    }
}
