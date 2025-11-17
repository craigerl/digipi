#ifndef BUILDSADDR_H
#define BUILDSADDR_H

extern struct sockaddr *build_sockaddr(const char *name, int *addrlen);
extern const int callvalid(char *call);

#ifdef	HAVE_AF_AX25
#if defined (linux) && LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0)
#include <linux/ax25.h>
#else
#include <netax25/ax25.h>
#endif
extern char *ax25_ntoa(const ax25_address *a);
#endif

#endif
