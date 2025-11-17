
#ifndef LOG_H
#define LOG_H

#define LOG_LEN	2048

#define L_CRIT		1	/* Critical conditions */
#define L_ERR           2	/* Error conditions */
#define L_AUTH		4	/* Auth errors */
#define L_R_ERR         8	/* Routing errors */
#define L_INFO		16	/* General system information (up/down) */
#define L_LINK		32	/* Link state changes */
#define L_SOCKET	64	/* Incoming/outgoing network connections */
#define L_CONFIG	128	/* Config file parsing */
#define L_DEST		256	/* Dest changes */
#define L_CHAN		512	/* Channel creation etc */
#define L_NUSER		1024	/* Network users */
#define L_LUSER		2048	/* Local users */
#define L_HOSTCMD	4096	/* Log host commands */
#define L_DEBUG         8192	/* Debugging messages (verbose) */

#define L_DEFLEVEL	(L_CRIT | L_ERR | L_R_ERR | L_AUTH | L_LUSER | L_INFO)

#define L_STDERR        1  /* Log to stderror */
#define L_SYSLOG        2  /* Log to syslog */

#define L_DEFDEST	(L_SYSLOG)

extern int log_level;	/* Logging level */
extern int log_dest;	/* Logging destination */

extern int log_fac;	/* syslog facility */
extern int log_lev;	/* syslog level */

extern int do_logconf(void *list, int argc, char **argv);
extern int do_logdest(void *list, int argc, char **argv);
extern int do_syslog(void *list, int argc, char **argv);

extern int open_log(void);
extern int do_log(int level, const char *fmt, ...);

extern char *prgname;	/* the name of this program */

#endif
