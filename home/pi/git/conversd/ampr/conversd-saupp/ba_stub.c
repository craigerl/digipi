/*
 * buildsaddr.c - return a struct sockaddr
 * examples:
 * build_sockaddr("unix:/tmp/foo");
 * build_sockaddr("localhost:3600");
 * build_sockaddr("ax25:db0tud-10");
 * build_sockaddr("ax25:db0tud-10,db0tud");
 * build_sockaddr("netrom:db0fc");
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#if defined (__OS2__)
#include <types.h>
#include <utils.h>
#elif defined (WIN32)
#include <winsock.h>
#else
#include <netdb.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#endif

#include "config.h"
#include "log.h"
#include "hmalloc.h"

static union {
    struct sockaddr sa;
    struct sockaddr_in si;
#if !defined (__OS2__) && !defined (WIN32)
    struct sockaddr_un su;
#endif
#ifdef HAVE_AF_AX25
    struct full_sockaddr_ax25 sax;
#endif
} addr;

#if defined (_AIX)          /* This should be in netdb.h but thats AIX :-( */
#ifdef __STDC__
void endhostent(void);
void endservent(void);
#else
void endhostent();
void endservent();
#endif
#endif


#ifdef HAVE_AF_AX25
char *ax25_ntoa(const ax25_address *a)
{
	static char buf[11];
	char c, *s;
	int n;

	for (n = 0, s = buf; n < 6; n++) {
		c = (a->ax25_call[n] >> 1) & 0x7F;

		if (c != ' ') *s++ = c;
	}
	
	*s++ = '-';

	if ((n = ((a->ax25_call[6] >> 1) & 0x0F)) > 9) {
		*s++ = '1';
		n -= 10;
	}
	
	*s++ = n + '0';
	*s++ = '\0';

	return buf;
}

int convert_call_entry(const char *name, char *buf)
{
	int ct   = 0;
	int ssid = 0;
	const char *p = name;
	char c;

	while (ct < 6) {
		c = toupper(*p & 0xff);

		if (c == '-' || c == '\0')
			break;

		if (!isalnum(c)) {
			do_log(L_ERR, "axutils: invalid symbol in callsign %s\n", name);
			return -1;
		}

		buf[ct] = c << 1;

		p++;
		ct++;
	}
	if (ct < 1) {
		// oops, no call, only ssid?
		do_log(L_ERR, "axutils: no callsign in %s\n", name);
		return -1;
	}

	while (ct < 6) {
		buf[ct] = ' ' << 1;
		ct++;
	}

	if (*p != '\0') {
		p++;

		if (sscanf(p, "%d", &ssid) != 1 || ssid < 0 || ssid > 15) {
			do_log(L_ERR, "axutils: SSID must follow '-' and be numeric in the range 0-15 - %s\n", name);
			return -1;
		}
	}

	buf[6] = ((ssid + '0') << 1) & 0x1E;

	return 0;
}

int convert_call(char *call, struct full_sockaddr_ax25 *sax)
{
	char *bp, *np;
	char *addrp;
	int n = 0;
	char *tmp = hstrdup(call);
	
	if (tmp == NULL)
		return -1;
		
	bp = tmp;
	
	addrp = sax->fsa_ax25.sax25_call.ax25_call;

	do {
		/* Fetch one callsign token */
		while (*bp != '\0' && (isspace(*bp & 0xff) || *bp == ','))
			bp++;

		np = bp;

		while (*np != '\0' && (!isspace(*np & 0xff) && *np != ','))
			np++;

		if (*np != '\0')
			*np++ = '\0';
	
		/* Check for the optional 'via' syntax and skip */
		if (n == 1 && (strcasecmp(bp, "V") == 0 || strcasecmp(bp, "VIA") == 0)) {
			bp = np;
			continue;
		}
		
		/* Process the token */
		if (convert_call_entry(bp, addrp) == -1) {
			hfree(tmp);
			return -1;
		}
			
		/* Move along */
		bp = np;
		n++;

		if (n == 1)
			addrp  = sax->fsa_digipeater[0].ax25_call;	/* First digipeater address */
		else
			addrp += sizeof(ax25_address);

	} while (n < AX25_MAX_DIGIS && *bp);

	hfree(tmp);

	/* Tidy up */
	sax->fsa_ax25.sax25_ndigis = n - 1;
	sax->fsa_ax25.sax25_family = AF_AX25;	

	return sizeof(struct full_sockaddr_ax25);
}
#endif

/*
 *
 */
struct sockaddr *build_sockaddr(const char *name, int *addrlen)
{
    char *host_name;
    char *serv_name;
    char buf[1024];

    errno = 0;

    memset((char *) &addr, 0, sizeof(addr));
    *addrlen = 0;

    host_name = strcpy(buf, name);
    serv_name = strchr(buf, ':');
    if (!serv_name) {
	if (!errno)
	    errno = EINVAL;
	return NULL;
    }

    *serv_name++ = 0;

    if (!*host_name || !*serv_name) {
	if (!errno)
	    errno = EINVAL;
	return NULL;
    }
  
    if (!strcmp(host_name, "local") || !strcmp(host_name, "unix")) {
#if !defined(__OS2__) && !defined(WIN32) && defined(HAVE_AF_UNIX)
	addr.su.sun_family = AF_UNIX;
	strncpy(addr.su.sun_path, serv_name, sizeof(addr.su.sun_path) - 1);
	addr.su.sun_path[sizeof(addr.su.sun_path)-1] = 0;
#ifdef RISCiX
        *addrlen = sizeof(addr.su.sun_family) + strlen(addr.su.sun_path);
#else
	//*addrlen = sizeof(addr.su)-sizeof(addr.su.sun_path)+strlen(serv_name);
        *addrlen = sizeof(struct sockaddr_un);
#endif
	return &addr.sa;
#else
    	if (!errno)
	    errno = EINVAL;
	return NULL;
#endif
    }

    if (!strcmp(host_name, "ax25")) {
#ifdef HAVE_AF_AX25
	*addrlen = convert_call(serv_name, &addr.sax);
	if (!errno && (*addrlen > 0)) {
		return &addr.sa;
	}
#endif
	if (!errno)
		errno = EINVAL;
	return NULL;
    }
    addr.si.sin_family = AF_INET;
    if (!strcmp(host_name, "*")) {
	addr.si.sin_addr.s_addr = htonl(INADDR_ANY);
    } else if (!strcmp(host_name, "loopback") || !strcmp(host_name, "localhost")) {
	addr.si.sin_addr.s_addr = inet_addr("127.0.0.1");
    } else if (!inet_aton(host_name, &addr.si.sin_addr)) {
	struct hostent *hp = gethostbyname(host_name);
	endhostent();
	if (!hp) {
    	    if (!errno) {
	        errno = EINVAL;
	    }
	    return NULL;
	}
	// for e.g. if dns failed but hosts file matches, then errno is
	// set even if the resolving was successfull:
	if (errno)
		errno = 0;
	addr.si.sin_addr.s_addr = ((struct in_addr *) (hp->h_addr))->s_addr;
    }
    if (isdigit(*serv_name & 0xff)) {
	addr.si.sin_port = htons(atoi(serv_name));
    } else {
	struct servent *sp = getservbyname(serv_name, (char *) 0);
	endservent();
	if (!sp)  {
    	    if (!errno) {
	        errno = EINVAL;
	    }
	    return NULL;
	}
	if (errno)
		errno = 0;
	addr.si.sin_port = sp->s_port;
    }
    *addrlen = sizeof(struct sockaddr_in);
    if (errno) {
        return NULL;
    }
    return &addr.sa;
}

int callvalid(const char *call)
{
	int d, l;
	char *p_ssid;

	l = strlen(call);
	if (l && (p_ssid = strpbrk(call, "-/_"))) {
		if (strlen(p_ssid) > 9+1)
			return 0;
		l = (p_ssid - call);
	}
	if (l < 3 || l > 6) return 0;
	if (isdigit(call[0] & 0xff) && isdigit(call[1] & 0xff)) return 0;
	if (!(isdigit(call[1] & 0xff) || isdigit(call[2] & 0xff))) return 0;
	if (!isalpha(call[l-1] & 0xff)) return 0;
	d = 0;
	for (; *call; call++) {
		if (*call == '-' || *call == '/' || *call == '_') break;
		if (!isalnum(*call & 0xff)) return 0;
		if (isdigit(*call & 0xff)) d++;
	}
	if (d < 1 || d > 2) return 0;
	return 1;
}

