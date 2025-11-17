static const char rcsid[] = "@(#) $Id: online.c,v 1.3 2022/07/09 20:09:32 dl9sau Exp $";

/*
 * This is Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@hpfcmdd.fc.hp.com>
 *
 * Modifications by Fred Baumgarten <dc6iq@insl1.etec.uni-karlsruhe.de>
 * $Revision: 1.3 $$Date: 2022/07/09 20:09:32 $
 */

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#if defined(_AIX) && defined (_IBMR2)
#include <time.h>
#include <sys/select.h>
#endif
#include <termios.h>
#include <unistd.h>

#if defined(__TURBOC__) || defined(__STDC__)
#define __ARGS(x)       x
#else
#define __ARGS(x)       ()
#define const
#endif

#if defined(mips)      /* no headerfiles for the whole stuff ? */
extern int select __ARGS((int nfsd, fd_set *readfds, fd_set *writefds,
                  fd_set *exceptfds, struct timeval *timeout));
#endif

#ifndef	__linux__
#if defined(mips) || defined(_AIX)    /* no headerfiles for the whole stuff ? */
extern void bzero __ARGS((char *b1, int length));
extern int socket __ARGS((int af, int type, int protocol));
extern int connect __ARGS((int s, struct sockaddr *name, int namelen));
extern int ioctl __ARGS((int d, int request, void *argp));
#endif
#endif


extern struct sockaddr *build_sockaddr __ARGS((const char *name, int *addrlen));

extern char *optarg;
extern int optind;

static const struct timeval select_timeout = {
  10, 0
};

struct timeval sel_timeout;
static struct termios prev_termios;
static void stop __ARGS((char *arg));

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

static void stop(arg)
char *arg;
{
  if (*arg) perror(arg);
  tcsetattr(0, TCSANOW, &prev_termios);
  exit(0);
}

/*---------------------------------------------------------------------------*/

static void call_conversd(char *name, char *cmd, char *opts)
{
  struct sockaddr *addr;
  int addrlen, i, size, ende = 0, outcnt = 0, is_issue = 1, nummer = 3600;
  char c, inbuf[2048], buffer[2048], outbuf[2048], server[128];
  fd_set mask;

  if (!strcmp(name, "lconvers")) {
    nummer = 6810;
  }
  if (!strcmp(name, "kaconvers")) {
    nummer = 6809;
  }
  if (!strcmp(name, "suconvers")) {
    nummer = 6811;
  }
  if (!strcmp(name, "wconvers")) {
    nummer = 3610;
  }
  sprintf(server, "%s:%d", CONVERSHOST, nummer);
  close(3);
  addr = build_sockaddr(server, &addrlen);
  if (socket(addr->sa_family, SOCK_STREAM, 0) == 3) {
    if (!connect(3, addr, addrlen)) {
      if (opts) {
	sprintf(inbuf, "/%s %s\n", cmd, opts);
      } else {
	sprintf(inbuf, "/%s\n", cmd);
      }
      printf("%s %s:\n", name, cmd);
#ifdef POSIX
      fflush(NULL);
#endif
      
      if (write(3, inbuf, strlen(inbuf)) < 0) stop("");
      for (; ; ) {
	FD_ZERO(&mask);
	FD_SET(3,&mask);
	memcpy(&sel_timeout, &select_timeout, sizeof(select_timeout));
	select(4, &mask, (fd_set *) 0, (fd_set *) 0, &sel_timeout);
	if (FD_ISSET(3, &mask)) {
	  size = read(3, buffer, sizeof(buffer));
	  if (size <= 0) stop("");
	  for (i = 0; i < size; i++) {
	    c = buffer[i];
	    if ((buffer[i] == '*') && (buffer[i+1] == '*') &&
		(buffer[i+2] == '*') && (buffer[i+3] == '\n')) ende++;
	    if (c != '\r') outbuf[outcnt++] = c;
	    if (c == '\n' || outcnt == sizeof(outbuf)) {
	      if (!is_issue) {
		if (write(1, outbuf, outcnt) < 0) stop("");
	      } else {
		if (outbuf[0] != '*') {
		  if (write(1, outbuf, outcnt) < 0) stop("");
		  is_issue = 0;
		}
	      }
	      outcnt = 0;
	    }
	  }
	  if (ende) break;
	} else break;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  int echo;
  struct termios curr_termios;
  char cname[64];
  char *t;

  signal(SIGPIPE, SIG_IGN);

  if (tcgetattr(0, &prev_termios)) stop(*argv);
  curr_termios = prev_termios;
  echo = curr_termios.c_lflag & ECHO;
  curr_termios.c_lflag = 0;
  curr_termios.c_cc[VMIN] = 1;
  curr_termios.c_cc[VTIME] = 0;
  if ((t = strrchr(argv[0], '/')) != NULL) {
    strcpy(cname, ++t);
  } else {
    if ((t = strrchr(argv[0], '\\')) != NULL) {
      strcpy(cname, ++t);
    } else {
      strcpy(cname, argv[0]);
    }
  }

  if (tcsetattr(0, TCSANOW, &curr_termios)) {
    stop(cname);
  }

  if ((argc > 1) && (strstr(argv[1], "convers"))) {
    call_conversd(argv[1], cname, argv[2]);
  } else {
    call_conversd("convers",   cname, argv[1]);
    call_conversd("lconvers",  cname, argv[1]);
    call_conversd("kaconvers", cname, argv[1]);
    call_conversd("suconvers", cname, argv[1]);
    call_conversd("wconvers",  cname, argv[1]);
  }

  stop("");
  return 0;
}

