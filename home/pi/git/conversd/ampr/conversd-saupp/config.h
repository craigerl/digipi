
#ifndef CONFIG_H
#define CONFIG_H

#define UTMP__FILE      UTMP_FILE
#if !defined(UTMP_FILE)
#define UTMP_FILE _PATH_UTMP
#endif
#ifdef	FORKPTY
#define	HAVE_FORKPTY	1
#endif

#ifdef	linux
#include <linux/version.h>
#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
#ifndef LINUX_VERSION_CODE
#define LINUX_VERSION_CODE KERNEL_VERSION(2,0,0)-1
#endif
#endif

/* supported protocols */
#define HAVE_AF_INET    1
#define HAVE_AF_UNIX    1
#ifdef	linux
#define HAVE_AF_AX25    1
#endif

#ifdef  HAVE_AF_UNIX
#include <sys/un.h>
#endif

#ifdef  HAVE_AF_AX25
#if defined (linux) && LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0)
#include <linux/ax25.h>
#else
#include <netax25/ax25.h>
#endif
#endif

struct bitlist_t {
	char	*name;
	int	value;
};

extern char *myhostname;
extern char *mysysinfo;
extern char *myemailaddr;
extern char *mytimezone;
extern char *secretpass;
extern int listen_port;
extern int secretnumber;
extern int ban_zero;
extern int callvalidate;
extern int defaultchannel;
extern int debugchannel;
extern int restrictedmode;
extern int max_u_queue;
extern int max_h_queue;
extern int demonize;
extern int tryonly;
extern int BrandUser;
extern int ForceAuthWhenOn;
extern int AcceptSquit;
extern int AcceptAPRSpass;
extern char history_lines;
extern int history_expires;

extern struct mbuf *unix_sockets;

extern char conffile[];

extern int bitcfg(struct bitlist_t *bitlist, int argc, char **argv);
extern int printbits(int mask, struct bitlist_t *bitlist, char *buf, size_t len);
extern int numcfg(struct bitlist_t *bitlist, char *arg);
extern int printnum(int val, struct bitlist_t *bitlist, char *buf, size_t len);

extern void parse_cmdl(int argc, char **argv);
extern void read_config(void);

#endif

