
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cfgfile.h"
#include "config.h"
#include "log.h"
#include "conversd.h"
#include "access.h"
#include "hmalloc.h"
#include "noflood.h"

extern struct listeners listeners[256];
char *myhostname = NULL;
char *mysysinfo = NULL;
char *myemailaddr = NULL;
char *mytimezone = NULL;
char *secretpass = NULL;
struct mbuf *unix_sockets = (struct mbuf *) 0;

int listen_port = 3600;
int secretnumber = 0;
int ban_zero = 1;
int callvalidate = 1;
int defaultchannel = 0;
int debugchannel = 32767; /* for the pp convers privacy compromise */
int restrictedmode = 0;
int max_u_queue = 50 * 1024;
int max_h_queue = 100 * 1024;
int demonize = 1;
int tryonly = 0;
int BrandUser = 1;
int ForceAuthWhenOn = 1;
int AcceptSquit = 1;
int AcceptAPRSpass = 0;
char history_lines = CHIST_MAXLINES;
int history_expires = CHIST_EXPIRES;

char conffile[PATH_MAX];

int do_security(int *dest, int argc, char **argv);
int do_link(struct permlink ***pa, int argc, char **argv);
int do_maxlinks(struct permlink ***pa, int argc, char **argv);
int add_sockunix(int *ignore, int argc, char **argv);
int add_listener(int *ignore, int argc, char **argv);

/*
 *	Configuration file commands
 */

#define _CFUNC_ (int (*)(void *dest, int argc, char **argv))

static struct cfgcmd cfg_cmds[] = {
	{ "access",		_CFUNC_ do_access,	NULL		},
	{ "ban_zero",		_CFUNC_ do_int,		&ban_zero	},
	{ "banzero",		_CFUNC_ do_int,		&ban_zero	},
	{ "callvalidate",	_CFUNC_ do_int,		&callvalidate	},
	{ "defaultchannel",	_CFUNC_ do_int,		&defaultchannel	},
	{ "debugchannel",	_CFUNC_ do_int,		&debugchannel	},
	{ "history_lines",	_CFUNC_ do_int,		&history_lines	},
	{ "history_expires",	_CFUNC_ do_int,		&history_expires},
	{ "email",		_CFUNC_ do_string,	&myemailaddr	},
	{ "link",		_CFUNC_	do_link,	&permarray	},
	{ "log",		_CFUNC_ do_logconf,		NULL		},
	{ "logdest",		_CFUNC_ do_logdest,	NULL		},
	{ "maxhqueue",		_CFUNC_ do_int,		&max_h_queue	},
	{ "maxuqueue",		_CFUNC_ do_int,		&max_u_queue	},
	{ "maxlinks",		_CFUNC_ do_maxlinks,	&permarray	},
	{ "port",		_CFUNC_ do_int,		&listen_port	},
	{ "secretnum",		_CFUNC_ do_int,		&secretnumber	},
	{ "secretpass",		_CFUNC_ do_string,	&secretpass	},
	{ "BrandUser",		_CFUNC_ do_int,		&BrandUser	},
	{ "security",		_CFUNC_	do_security,	&restrictedmode },
	{ "server",		_CFUNC_ do_string,	&myhostname	},
	{ "sysinfo",		_CFUNC_ do_string,	&mysysinfo	},
	{ "syslog",		_CFUNC_ do_syslog,	NULL		},
	{ "timezone",		_CFUNC_ do_string,	&mytimezone	},
	{ "unix_sockets",	_CFUNC_ add_sockunix,	&unix_sockets	},
	{ "listeners",		_CFUNC_ add_listener,	NULL		},

	{ "FeatureFLOOD",	_CFUNC_ do_int,		&FeatureFLOOD	},
	{ "RateLimitMAX",	_CFUNC_ do_int,		&RateLimitMAX	},
	{ "RateLimitLine",	_CFUNC_ do_int,		&RateLimitLine	},
	{ "RateLimitContent",	_CFUNC_ do_int,		&RateLimitContent},
	{ "FeatureBan",		_CFUNC_ do_int,		&FeatureBAN	},
	{ "BanTime",		_CFUNC_ do_int,		&BanTime	},
	{ "SulLifeTime",	_CFUNC_ do_int,		&SulLifeTime	},
	{ "SulCmdDelay",	_CFUNC_ do_int,		&SulCmdDelay	},
	{ "ForceAuthWhenOn",	_CFUNC_ do_int,		&ForceAuthWhenOn},
	{ "AcceptSquit",	_CFUNC_ do_int,		&AcceptSquit	},
	{ "AcceptAPRSpass",	_CFUNC_ do_int,		&AcceptAPRSpass	},
	{ NULL,			NULL,			NULL		}
};

/*
 *	Configure a bitmask
 */
 
int bitcfg(struct bitlist_t *bitlist, int argc, char **argv)
{
	struct bitlist_t *bl;
	int i;
	int mask = 0;
	
	for (i = 0; i < argc; i++) {
		bl = bitlist;
		while (bl->name) {
			if (!strcasecmp(argv[i], bl->name)) {
				mask |= bl->value;
				break;
			}
			bl++;
		}
		if (bl->name == NULL) {
			do_log(L_CRIT, "Unknown parameter: \"%s\"", argv[i]);
			return -1;
		}
	}
	
	return mask;
}

/*
 *	Print a list of enabled bits to a buffer
 */

int printbits(int mask, struct bitlist_t *bitlist, char *buf, size_t len)
{
	struct bitlist_t *bl;
	
	bl = bitlist;
	while (bl->name) {
		if (mask & bl->value) {
			strncat(buf, " ", len);
			strncat(buf, bl->name, len);
		}
		bl++;
	}
	
	buf[len-1] = '\0';
	
	return strlen(buf);
}

/*
 *	Configure an integer
 */
 
int numcfg(struct bitlist_t *bitlist, char *arg)
{
	struct bitlist_t *bl;
	int val = 0;
	
	bl = bitlist;
	while (bl->name) {
		if (!strcasecmp(arg, bl->name)) {
			val = bl->value;
			break;
		}
		bl++;
	}
	if (bl->name == NULL) {
		do_log(L_CRIT, "Unknown parameter: \"%s\"", arg);
		return -1;
	}
	
	return val;
}

/*
 *	Print the name of a number to a buffer
 */

int printnum(int val, struct bitlist_t *bitlist, char *buf, size_t len)
{
	struct bitlist_t *bl;
	
	bl = bitlist;
	while (bl->name) {
		if (val == bl->value) {
			strncat(buf, " ", len);
			strncat(buf, bl->name, len);
		}
		bl++;
	}
	
	buf[len-1] = '\0';
	
	return strlen(buf);
}

/*
 *	do_security
 */

int do_security(int *dest, int argc, char **argv)
{
	if (argc < 2)
		return -1;
		
	if (*argv[1] == 'r') {
		*dest = 1;
		do_log(L_CONFIG, "Security: Restricted");
	} else if (*argv[1] == 'o') {
		*dest = 2;
		do_log(L_CONFIG, "Security: Observer");
	} else {
		do_log(L_ERR, "Unknown Security mode: %s", argv[1]);
		return -1;
	}
	
	return 0;
}

/*
 *	do_link
 */

int do_link(struct permlink ***pa, int argc, char **argv)
{
	char *host_name;
	char *sock_name = 0;
	char *primary_link = 0;
	char *p_command = 0;
#ifdef	notdef
	char *cp;
#endif
	struct permlink *p, *p2;

	if (argc < 2 || argc > 5)
		return -1;
	
	host_name = argv[1];
	if (argc > 2)
		sock_name = argv[2];
	if (argc > 3)
  		primary_link = argv[3];
  	if (argc > 4) {
    		p_command = argv[4];
		if (p_command && strlen(p_command) > 96)
			p_command[96] = 0;
	}

	if ((p = find_permlink(host_name))) {
		if (p->groups >= NR_GROUPS) {
			do_log(L_CRIT, "Maximum number of groups for permlink %s exceeded: %d. Hint: adjust NR_GROUPS in your config file.", p->name, NR_GROUPS);
			return -1;
		}
	} else {
		p = update_permlinks(host_name, NULLCONNECTION, 1);
		if (!p) {
			do_log(L_CRIT, "Maximum number of permlinks exceeded: %d. Hint: adjust Maxlinks in your config file.", NR_PERMLINKS);
			return -1;
		}
	}
		
	strncpy(p->name, host_name, HOSTNAMESIZE);
#ifdef	notdef
	// old tpp hack
	if ((cp = strchr(sock_name, ':'))) {
		*cp = '\0';
		cp++;
		p->port = atoi(cp);
	} else
		p->port = 3600;
	p->host = hstrdup(sock_name);
#endif

	p->s_groups[p->groups].socket = (sock_name && *sock_name) ? hstrdup(sock_name) : NULL;
	p->s_groups[p->groups].command = (p_command && *p_command) ? hstrdup(p_command) : NULL;
	p->s_groups[p->groups].quality = 0;
	p->groups += 1;
	p->primary = NULL;
	p->backup = NULL;

	// dl9sau bugfix: had a deadlock with a Link referencing to itself.
	if (primary_link && (!*primary_link || !strcasecmp(primary_link, host_name)))
		primary_link = NULL;
	
	/* If we have a primary link, find it and link it in */
	if (primary_link) {
		p2 = find_permlink(primary_link);
		if (p2) {
			p->waittime = 30;
			if (p2->backup) {
				/* dl9sau: there are good reasons for using
				 * a flat tree list from the primary down,
				 * as long as we don't keep a rtt list, on
				 * which based we could deceide wheater which
				 * client would be a better choice, and which
				 * one should be disconnected if a better one
				 * connects..
				 */
				while (p2->backup) {
					if (p2->backup == p)
						break;
					p2 = p2->backup;
				}
				if (!p2->backup)
					do_log(L_ERR, "Link: %s: %s has already a backup. considering as backup for %s.", host_name, primary_link, p2->name);
			}
			if (p2 != p) {
				p->primary = p2;
				p2->backup = p;
			}
		} else {
			do_log(L_ERR, "Link: %s: wants to be a backup for %s, which has not been defined yet! Discarding this link.",
				host_name, primary_link);
			
		}
	}
	
	do_log(L_CONFIG, "Link: %s [%s]%s%s%s", p->name,
		(p->s_groups[p->groups -1].socket ? p->s_groups[p->groups -1].socket : "<off>"),
		(p->primary) ? " (backup to " : "",
		(p->primary) ? p->primary->name : "",
		(p->primary) ? ")" : "");
	
	return 0;
}

/*
 *	do_maxlinks
 */

int do_maxlinks(struct permlink ***pa, int argc, char **argv)
{
	struct permlink **tmp;
	int links = NR_PERMLINKS;
	
	if (argc < 2)
		return -1;
	
	NR_PERMLINKS = atoi(argv[1]);
	tmp = permarray;
	permarray = hcalloc(NR_PERMLINKS, sizeof(struct permlink *));
	if (tmp != NULLPERMARRAY) {
		memcpy(permarray, tmp, links * sizeof(struct permlink *));
		hfree(tmp);
	}
	
	do_log(L_CONFIG, "MaxLinks: %d", NR_PERMLINKS);
	
	return 0;
}

/*
 * 	add_sockunix
 */

int add_sockunix (int *ignore, int argc, char **argv)
{

	if (argc == 2 && argv[1] && strlen(argv[1])) {
        	struct mbuf *bp;

		if (!(bp = (struct mbuf *) hmalloc(sizeof(struct mbuf)))) {
			do_log(L_CRIT, "no free memory");
			return -1;
		}
		bp->data = hstrdup(argv[1]);
		bp->next = unix_sockets;
		unix_sockets = bp;
		do_log(L_CONFIG, "Added path to trusted unix_socket: %s", bp->data);
		return 0;
	}
	do_log(L_CONFIG, "%s: bad argument", argv[0]);

	return -1;
}

/*
 * 	allowed_unix_paths
 */

int allowed_unix_paths(char *s)
{

	struct mbuf *bp;

	// default is a very restrictive setting: nothing allowed
	if (!s || !*s)
		return 0;
	for (bp = unix_sockets; bp; bp = bp->next) {
		if (!bp->data || !*bp->data)
			continue;

		/* unix_sockets may be a substring, for e.g. /tcp/.socket,
	 	 * while client requests for /tcp/.socket/netcmd
	 	 * be case sensitive
	 	 */
		if (strncmp(s, bp->data, strlen(bp->data)) == 0)
			return 1;
	}
	return 0;
}

/*
 * 	allowed_unix_paths
 */
int add_listener (int *ignore, int argc, char **argv)
{
	int i;

	if (argc == 2 && argv[1] && strlen(argv[1])) {
		for (i = 2; i < 256; i++)
			if (!listeners[i].name)
				break;
		if (i > sizeof(listeners) -1) {
			do_log(L_CONFIG, "too many listeners - %d max", 256 -1);
			return -1;
		}
		listeners[i].name = hstrdup(argv[1]);
		listeners[i].fd = -1;
		if (i < 256) {
			listeners[i+1].name = NULL;
			listeners[i+1].fd = -1;
		}
		do_log(L_CONFIG, "Added listener #%d: %s", i-1, listeners[i].name);
		return 0;
	}
	do_log(L_CONFIG, "%s: bad argument", argv[0]);
	return -1;
}

/*
 *	Parse command line arguments
 */

void parse_cmdl(int argc, char **argv)
{
	int c;
	
	while ((c = getopt(argc, argv, "ft")) != -1) {
	switch (c) {
		case 'f':
			demonize = 0;
			break;
		case 't':
			tryonly = 1;
			break;
		default:
			fprintf(stderr, "Ugh: Unknown parameter '%c'. Known: -f (don't fork), -t (try only)\n", c);
			exit(1);
	}
	}
	
}

/*
 *	Read configuration files
 */

void read_config(void)
{
	char buffer[1024];
	char *cp;

	defaultchannel = 0;
	secretnumber = 0;
	restrictedmode = 0;
	
	if (mysysinfo) {
		hfree(mysysinfo);
		mysysinfo = NULL;
	}
	if (myemailaddr) {
		hfree(mysysinfo);
		mysysinfo = NULL;
	}
	if (myhostname) {
		hfree(myhostname);
		myhostname = NULL;
	}
	
	if (read_cfgfile(conffile, cfg_cmds))
		exit(1);
	
	if (!myhostname) {
		gethostname(buffer, sizeof(buffer));
		myhostname = hstrdup(buffer);
	}
	if ((cp = strchr(myhostname, '.')))
			*cp = 0;

	if (!( (myhostname) && (myemailaddr) && (mysysinfo) )) {
	   	do_log(L_CRIT, "Missing entries in configuration file!");
	   	exit(1);
	}
	
	if (secretnumber >= INT_MAX) {
		do_log(L_CRIT, "Secret number is larger than MAX_INT (%d)!", INT_MAX);
		exit(1);
	}
	
	if ((defaultchannel < 0) || (defaultchannel > 32767)) {
		do_log(L_CRIT, "Default channel is smaller than 0 or larger than 32767.");
		exit(1);
	}
	
	if (strlen(myhostname) > 9) {
		do_log(L_CRIT, "Server hostname is longer than 9 characters!");
		exit(1);
	}
	
	if (strpbrk(myhostname, " \t\b\r\n")) {
		do_log(L_CRIT, "Server hostname contains whitespace!");
		exit(1);
	}

	for (cp = myhostname; *cp; cp++) {
		if (isalnum(*cp & 0xff))
			continue;
		if (*cp == '-' || *cp == '_')
			continue;
		break;
	}
	if (*cp) {
		do_log(L_CRIT, "Server hostname contains invalid char: %c", cp);
		exit(1);
	}
}

