
/*
 *	log.c
 *
 *	logging facility with configurable log levels and
 *	logging destinations
 */

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <string.h>

#include "log.h"
#include "config.h"

int log_level = L_DEFLEVEL;	/* Logging level */
int log_dest = L_DEFDEST;	/* Logging destination */

int log_fac = LOG_DAEMON;
int log_lev = LOG_INFO;

static struct bitlist_t levellist[] = {
	{ "critical",	L_CRIT		},
	{ "error",	L_ERR		},
	{ "auth",	L_AUTH		},
	{ "routing",	L_R_ERR		},
	{ "link",	L_LINK		},
	{ "dest",	L_DEST		},
	{ "nuser",	L_NUSER		},
	{ "luser",	L_LUSER		},
	{ "info",	L_INFO		},
	{ "debug",	L_DEBUG		},
	{ "socket",	L_SOCKET	},
	{ "hostcmd",	L_HOSTCMD	},
	{ "config",	L_CONFIG	},
	{ "channel",	L_CHAN		},
	{ NULL,		-1		}
};

static struct bitlist_t destlist[] = {
	{ "syslog",	L_SYSLOG	},
	{ "stderr",	L_STDERR	},
	{ NULL,		-1		}
};

static struct bitlist_t sys_faclist[] = {
#ifdef LOG_AUTH
	{ "auth",	LOG_AUTH	},
#endif
#ifdef LOG_AUTHPRIV
	{ "authpriv",	LOG_AUTHPRIV	},
#endif
	{ "cron",	LOG_CRON	},
	{ "daemon",	LOG_DAEMON	},
	{ "local0",	LOG_LOCAL0	},
	{ "local1",	LOG_LOCAL1	},
	{ "local2",	LOG_LOCAL2	},
	{ "local3",	LOG_LOCAL3	},
	{ "local4",	LOG_LOCAL4	},
	{ "local5",	LOG_LOCAL5	},
	{ "local6",	LOG_LOCAL6	},
	{ "local7",	LOG_LOCAL7	},
	{ "lpr",	LOG_LPR		},
	{ "mail",	LOG_MAIL	},
	{ "news",	LOG_NEWS	},
	{ "user",	LOG_USER	},
	{ "uucp",	LOG_UUCP	},
	{ NULL,		-1		}
};

static struct bitlist_t sys_levellist[] = {
	{ "emerg",	LOG_EMERG	},
	{ "alert",	LOG_ALERT	},
	{ "crit",	LOG_CRIT	},
	{ "err",	LOG_ERR		},
	{ "warning",	LOG_WARNING	},
	{ "notice",	LOG_NOTICE	},
	{ "info",	LOG_INFO	},
	{ "debug",	LOG_DEBUG	},
	{ NULL,		-1		}
};

/*
 *	Open log
 */
 
int open_log(void)
{
	if (log_dest & L_SYSLOG) {
		closelog();
		openlog(prgname, LOG_NDELAY|LOG_PID, log_fac);
	}
		
	return 0;
}

/*
 *	Log a message (do_log - was log, but gcc3.x considers log() as being reserved)
 */
 
int do_log(int level, const char *fmt, ...)
{
	va_list args;
	char s[LOG_LEN];
	
	va_start(args, fmt);
#ifdef NO_SNPRINTF
	vsprintf(s, fmt, args);
#else
	vsnprintf(s, LOG_LEN, fmt, args);
#endif
	va_end(args);
	s[LOG_LEN-1] = '\0';
	
	if ((log_level & level) || (L_CRIT & level)) {
		if ((log_dest & L_STDERR) || (level & L_CRIT))
			fprintf(stderr, "%s\n", s);
		if (log_dest & L_SYSLOG)
			syslog(log_lev, "%s", s);
	}
	
	return 0;
}

/*
 *	Configuration file entry do_logconf
 */
 
int do_logconf(void *list, int argc, char **argv)
{
	char buf[256];
	
	if (argc < 2)
		return -1;
	
	if ((log_level = bitcfg(levellist, argc - 1, &argv[1])) == -1) {
		log_level = L_CRIT|L_ERR;
		return -1;
	}
	
	log_level |= L_CRIT|L_ERR; /* Won't go under this. */
	
	buf[0] = '\0';
	printbits(log_level, levellist, buf, 256);
	
	do_log(L_CONFIG, "Log:%s", buf);
	
	return 0;
}

/*
 *	Configuration file entry do_logdest
 */
 
int do_logdest(void *list, int argc, char **argv)
{
	char buf[256];
	
	if (argc < 2)
		return -1;
	
	if ((log_dest = bitcfg(destlist, argc - 1, &argv[1])) == -1) {
		log_dest = L_SYSLOG|L_STDERR;
		return -1;
	}
	
	buf[0] = '\0';
	printbits(log_dest, destlist, buf, 256);
	
	do_log(L_CONFIG, "LogDest:%s", buf);
	
	return 0;
}

/*
 *	Configuration file entry do_syslog
 */
 
int do_syslog(void *list, int argc, char **argv)
{
	char buf[256];
	
	if (argc != 3)
		return -1;
	
	if ((log_fac = numcfg(sys_faclist, argv[1])) == -1) {
		log_dest |= L_STDERR;
		return -1;
	}
	
	if ((log_lev = numcfg(sys_levellist, argv[2])) == -1) {
		log_dest |= L_STDERR;
		return -1;
	}
	
	open_log();
	
	buf[0] = '\0';
	printnum(log_fac, sys_faclist, buf, 256);
	printnum(log_lev, sys_levellist, buf, 256);
	
	do_log(L_CONFIG, "Syslog:%s", buf);
	
	return 0;
}

