
/*
 * axconv.c - A simple AX.25 convers client - Version 1.1
 *
 * Copyright (C) 1997 Tomi Manninen, OH2BNS, <tomi.manninen@hut.fi>.
 *
 * Compile with: gcc -Wall -O6 -o axconv axconv.c
 *
 * Usage: axconv [-c <channel>] [-n <name>] [-p <paclen>] [-t <timeout>] <host> [<service>]
 *
 * Axconv is a simple AX.25 convers client intended to be launched
 * from ax25d. It connects to the conversd system specified and
 * optionally logs in with the user and channel specified. It does
 * the necessary end-of-line conversions and also buffers the traffic
 * in order to optimise the AX.25 channel usage slightly.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#define	BUFFERSIZE	4096
#define FLUSHTIMEOUT	500000		/* 0.5 sec */

static char *usage_string = "Usage: axconv [-c <channel>] [-n <name>] [-p <paclen>] [-t <timeout>] <host> [<service>]\r";

#define PERROR(s)	printf("%s: %s\r",(s),strerror(errno)),fflush(stdout)

static void alarm_handler(int sig)
{
	printf("*** Inactivity timeout\r");
	exit(0);
}

/*
 * Convert <CR> to <CR><LF>
 */
static int convert_cr_crlf(char *inbuf, char *outbuf, int len)
{
	int i = 0;

	while (len-- > 0) {
		if (inbuf[0] == '\r') {
			outbuf[i++] = '\r';
			outbuf[i++] = '\n';
		} else
			outbuf[i++] = inbuf[0];
		inbuf++;
	}
	return i;
}

/*
 * Convert <CR><LF>, <CR><NUL> or a plain <NL> to a <CR>
 */
static int convert_crlf_cr(char *inbuf, char *outbuf, int len)
{
	int i = 0;

	while (len-- > 0) {
		if (inbuf[0] == '\r' && (inbuf[1] == '\0' || inbuf[1] == '\n')) {
			outbuf[i++] = '\r';
			inbuf++;
			len--;
		} else if (inbuf[0] == '\n')
			outbuf[i++] = '\r';
		else
			outbuf[i++] = inbuf[0];
		inbuf++;
	}
	return i;
}

int main(int argc, char **argv)
{
	char inbuf[BUFFERSIZE], outbuf[BUFFERSIZE];
	char *name = NULL, *channel = NULL;
	int paclen = 256, timeout = 0, len, fd;
	FILE *fp;
	fd_set fdset;
	struct timeval tv;
	struct sockaddr_in sin;
        struct hostent *hp;
        struct servent *sp;

	signal(SIGALRM, alarm_handler);

	while ((len = getopt(argc, argv, "c:n:p:t:")) != -1) {
		switch (len) {
		case 'n':
			name = optarg;
			break;
		case 'c':
			channel = optarg;
			break;
		case 'p':
			paclen = atoi(optarg);
			break;
		case 't':
			timeout = atoi(optarg);
			break;
		case ':':
		case '?':
			fputs(usage_string, stdout);
			fflush(stdout);
			return 1;
		}
	}

	if (optind == argc) {
		fputs(usage_string, stdout);
		fflush(stdout);
		return 1;
	}

	setvbuf(stdout, NULL, _IOFBF, paclen);

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		PERROR("axconv: socket");
		return 1;
	}

	if ((hp = gethostbyname(argv[optind])) == NULL) {
		printf("Unknown host %s\r", argv[optind]);
		fflush(stdout);
		close(fd);
		return 1;
	}
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;

	sin.sin_port = htons(3600);
	if (optind == argc - 2) {
		sp = getservbyname(argv[optind + 1], "tcp");
		if (sp != NULL)
			sin.sin_port = sp->s_port;
		else
			sin.sin_port = htons(atoi(argv[optind + 1]));

		if (ntohs(sin.sin_port) == 0) {
			printf("Unknown service %s\r", argv[optind + 1]);
			fflush(stdout);
			return 1;
		}
	}

	printf("*** Connecting to %s, port %d\r", hp->h_name, ntohs(sin.sin_port));
	fflush(stdout);

	len = sizeof(struct sockaddr_in);
	if (connect(fd, (struct sockaddr *) &sin, len) == -1) {
		PERROR("axconv: connect");
		return 1;
	}
	if ((fp = fdopen(fd, "w+")) == NULL) {
		PERROR("axconv: fdopen");
		return 1;
	}

	if (fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK) == -1) {
		PERROR("axconv: fcntl");
		exit(1);
	}
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
		PERROR("axconv: fcntl");
		exit(1);
	}

	if (name != NULL) {
		printf("*** Logging in as %s", name);
		if (channel != NULL)
			printf(", channel %s", channel);
		printf("\r");
		fprintf(fp, "/n %s %s\r\n", name, channel ? channel : "");
		fflush(stdout);
		fflush(fp);
	}

	alarm(timeout);

	while (1) {
		FD_ZERO(&fdset);
		FD_SET(STDIN_FILENO, &fdset);
		FD_SET(fd, &fdset);
		tv.tv_sec = 0;
		tv.tv_usec = FLUSHTIMEOUT;

		len = select(fd + 1, &fdset, 0, 0, &tv);
		if (len == -1) {
			PERROR("axconv: select");
			return 1;
		}
		if (len == 0) {
			fflush(stdout);
			fflush(fp);
		}

		if (FD_ISSET(STDIN_FILENO, &fdset)) {
			alarm(timeout);
			len = read(STDIN_FILENO, inbuf, BUFFERSIZE);
			if (len < 0 && errno != EAGAIN) {
				PERROR("axconv: read");
				break;
			}
			if (len == 0)
				break;
			len = convert_cr_crlf(inbuf, outbuf, len);
			fwrite(outbuf, 1, len, fp);
		}

		if (FD_ISSET(fd, &fdset)) {
			alarm(timeout);
			len = read(fd, inbuf, BUFFERSIZE);
			if (len < 0 && errno != EAGAIN) {
				PERROR("axconv: read");
				break;
			}
			if (len == 0)
				break;
			len = convert_crlf_cr(inbuf, outbuf, len);
			fwrite(outbuf, 1, len, stdout);
		}
	}

	printf("*** Disconnected from %s\r", hp->h_name);
	fflush(stdout);
	fclose(fp);
	return 0;
}

