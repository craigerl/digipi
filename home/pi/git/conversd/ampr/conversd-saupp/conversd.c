static const char rcsid[] = "@(#) $Id: conversd.c,v 1.5 2005/04/17 14:42:43 dl9sau Exp $";

/*
 * This is Tampa Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@mdddhd.fc.hp.com>
 * Modifications by Fred Baumgarten <dc6iq@insu1.etec.uni-karlsruhe.de>
 *                  Matthias Welwarsky <dg2fef@uet.th-darmstadt.de>
 *
 * Modifications by Brian A. Lantz/KO4KS <brian@lantz.com> 
 *
 * Further mods by Tony Jenkins G8TBF Jan 2000. Changed to V1.23
 * /Nickname & /nonickname commands replaced, various bugs fixed.
 * Further mods by Thomas Osterried <dl9sau>, various bugs, raceconditions
 * and incompatibilities fixed; some extensions, some backpatches.1359
 */

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#ifdef _AIX
#include <sys/select.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <utmp.h>
#include <syslog.h>
#include <pwd.h>

#include "conversd.h"
#include "user.h"
#include "irc.h"
#include "host.h"
#include "tnos.h"
#include "convert.h"
#include "version.h"
#include "log.h"
#include "cfgfile.h"
#include "config.h"
#include "access.h"
#include "hmalloc.h"
#include "ba_stub.h"
#include "noflood.h"

#ifdef	HAVE_FORKPTY
#include <pty.h>
#endif

char *prgname = "conversd";
char *convcmd = "convers";

fd_set chkread;
fd_set chkwrite;

char convtype[512];
time_t boottime;
time_t currtime;
char userfile[128];
char motdfile[128];
char restrictfile[128];
char accessfile[128];
char noaccessfile[128];
char helpfile[128];
char observerfile[128];
char issuefile[128];
char pidfile[128];
char listener[128];
int NR_PERMLINKS = DEF_NR_PERMLINKS;

#ifdef	WANT_FILTER
char myfeatures[] = "Aadmpunfi";
#else
char myfeatures[] = "Aadmpuni";
#endif
char OBSERVERSTR[] = "*** Sorry, you are only an observer :-)\n";
char OBSERVERSTR2[] = "*** That command is not available to observers....\n";
char OBSERVERSTR3[] = "*** Observers are not permitted to send messages....\n";
char OBSERVERID[] = "Observer only";

static int maxfd = -1;

struct listeners listeners[256] = {
        { "*:3600", -1} ,
        { NULL, -1} ,
        { 0, -1}
};

struct langs languages[] = {
        /* { "filename-extension" } { "primary lang", ["aliases" [, ..],] 0 } */
        { "", { "reset", "none", 0 } },
        { "en", { "english", "british", "uk", 0 } },
        { "de", { "german", "deutsch", "dl", 0 } },
        { 0, { 0 } }
};


struct connection *connections;
struct permlink **permarray = NULLPERMARRAY;
struct destination *destinations;
struct channel *channels;

struct extendedcmds *ecmds = NULLEXTCMD;

struct stats daily[60];
struct stats hourly[60];
time_t starttime;
time_t nextdailystats;
time_t nexthourlystats;
long hoursonline = 0L;
long daysonline = 0L;

void process_input(struct connection * cp);

extern char *rip(char *str);
extern long seed;

struct cmdtable cmdtable[] =
{

	{"/quit",	bye_command,		CM_USER | CM_OBSERVER | CM_UNKNOWN},
	{"/echo",	echo_command,		CM_USER | CM_OBSERVER | CM_UNKNOWN},
	{"/comp",	comp_command,		CM_USER | CM_OBSERVER},
	{"?",		cmdsummary_command,	CM_USER | CM_OBSERVER},
	{"away",	away_command,		CM_USER},
	{"action",	me_command,		CM_USER},
	{"all",		all_command,		CM_USER},
	{"auth",	auth_command,		CM_UNKNOWN | CM_USER | CM_OBSERVER | CT_HOST},
	{"beep",	beep_command,		CM_USER | CM_OBSERVER},
	{"banlist",	banlist_command,	CM_USER},
	{"banparam",	banparam_command,	CM_USER},
	{"bell",	beep_command,		CM_USER | CM_OBSERVER},
	/*{"bjack",       bjack_command,          CM_USER}, no spam! */
	{"bye",		bye_command,		CM_USER | CM_OBSERVER | CM_UNKNOWN},
	{"channel",	channel_command,	CM_USER | CM_OBSERVER},
	{"charset",	charset_command,	CM_USER | CM_OBSERVER},
	{"chinfo",	chinfo_command,		CM_USER | CM_OBSERVER},
	{"cron",	cron_command,		CM_USER | CM_OBSERVER},
	{"cstat",	cstat_command,		CM_UNKNOWN | CM_USER | CM_OBSERVER},
	/*{"cut",         cut_command,            CM_USER }, no spam! */
	{"destinations", hosts_command,		CM_USER | CM_OBSERVER},
	{"echo",	echo_command,		CM_USER | CM_OBSERVER | CM_UNKNOWN},
	{"exclude",	imsg_command,		CM_USER},
	{"exit",	bye_command,		CM_USER | CM_OBSERVER | CM_UNKNOWN},
#ifdef	WANT_FILTER
	{"filterusers", filterusers_command,	CM_USER},
	{"filterwords", filterwords_command,	CM_USER | CM_OBSERVER },
	{"filtermsgs",  filtermsgs_command,	CM_USER | CM_OBSERVER },
#endif
	{"help",	help_command,		CM_USER | CM_OBSERVER},
	{"halt",	restart_command,	CM_USER},
	{"history",	history_command,	CM_USER | CM_OBSERVER},
	{"hosts",	hosts_command,		CM_USER | CM_OBSERVER},
	{"hushlogin",	hushlogin_command,	CM_USER | CM_OBSERVER | CM_UNKNOWN},
	{"invite",	invite_command,		CM_USER},
	{"idle",	idle_command,		CM_USER | CM_OBSERVER},
#ifdef	WANT_FILTER
	{"ignore",      filterusers_command,	CM_USER},
#endif
	{"imsg",	imsg_command,		CM_USER},
	{"irc",		irc_command,		CM_USER},
	{"join",	channel_command,	CM_USER | CM_OBSERVER},
	{"links",	links_command,		CM_USER | CM_OBSERVER},
	{"last",	last_command,		CM_USER | CM_OBSERVER},
	{"language",	language_command,	CM_USER | CM_OBSERVER},
	{"leave",	leave_command,		CM_USER | CM_OBSERVER},
	{"list",	list_command,		CM_USER | CM_OBSERVER},
	{"listsul",	listsul_command,	CM_USER },
	{"login",	name_command,		CM_UNKNOWN},
	{"msg",		msg_command,		CM_USER},
	{"map",		map_command,		CM_UNKNOWN | CM_USER | CM_OBSERVER},
	{"me",		me_command,		CM_USER},
	{"mkpass",	mkpass_command,		CM_USER | CM_HOST},
	{"mode",	mode_command,		CM_USER},
	{"name",	name_command,		CM_UNKNOWN},
	{"notify",	notify_command,		CM_USER},
	{"news",	news_command,		CM_USER | CM_OBSERVER},
	{"nickname",	nickname_command,	CM_USER},
	{"note",	personal_command,	CM_USER},
	{"nonickname",	nonickname_command,	CM_USER},
	{"notice",      notice_command,	        CM_USER },
	{"operator",	operator_command,	CM_USER},
	{"/priv",	operator_command,	CM_USER},
	{"observer",	observer_command,	CM_UNKNOWN},
	{"online",	who_command2,		CM_UNKNOWN | CM_USER | CM_OBSERVER},
	{"personal",	personal_command,	CM_USER},
	{"prompt",	prompt_command,		CM_USER | CM_OBSERVER},
	{"profile",	profile_command,	CM_USER | CM_OBSERVER},
	{"paclen",	paclen_command,		CM_UNKNOWN | CM_HOST | CM_USER | CM_OBSERVER},
	{"pass",        irc_pass_command,	CM_UNKNOWN },
	{"quit",	bye_command,		CM_USER | CM_OBSERVER | CM_UNKNOWN},
	{"query",	query_command,		CM_USER},
/*	{"quote",	quote_command,		CM_USER | CM_OBSERVER},*/
	{"realnames",	realname_command,	CM_USER | CM_OBSERVER},
	{"restart",	restart_command,	CM_USER},
	{"restricted",	restricted_command,	CM_UNKNOWN},
	/*{"roll",        roll_command,           CM_USER},*/
	{"rtt",         rtt_command,            CM_UNKNOWN | CM_USER | CM_OBSERVER},
	{"/rtt",        rtt_command,            CM_UNKNOWN | CM_USER | CM_OBSERVER},
	{"send",	msg_command,		CM_USER},
	{"shownicks",	shownicks_command,	CM_USER | CM_OBSERVER | CM_UNKNOWN},
	/*{"smiley",	smiley_command,		CM_USER}, no spam! */
	/*{"spam",	spam_command,		CM_USER}, no spam! */
	{"stats",	stats_command,		CM_USER | CM_OBSERVER},
	{"sysop",	operator_command,	CM_USER},
	{"sysinfo",	sysinfo_command,	CM_USER | CM_OBSERVER},
	{"topic",	topic_command,		CM_USER},
	{"time",	time_command,		CM_USER},
	{"timestamp",	timestamp_command,	CM_USER | CM_OBSERVER},
	{"trigger",	trigger_command,	CM_USER | CM_OBSERVER},
	{"users",	who_command,		CM_USER | CM_OBSERVER},
	{"uptime",	uptime_command,		CM_USER | CM_OBSERVER},
	{"verbose",	verbose_command,	CM_USER},
	{"version",	version_command,	CM_USER | CM_OBSERVER},
	{"who",		who_command,		CM_USER | CM_OBSERVER},
	{"whois",	whois_command,		CM_USER | CM_OBSERVER},
	{"wall",	wall_command,		CM_USER},
	{"width",	width_command,		CM_USER | CM_OBSERVER},
	{"wizard",	wizard_command,		CM_USER},
	{"write",	msg_command,		CM_USER},
	/*{"wx",		wx_command,		CM_USER }, */
	{"zap",		wizard_command,		CM_USER},

	{"\377\200away", h_away_command,	CM_HOST},
	{"\377\200ban",  h_ban_command,		CM_HOST},
	{"\377\200cmsg", h_cmsg_command,	CM_HOST},
	{"\377\200cdat", h_cdat_command,	CM_HOST},
	{"\377\200dest", h_dest_command,	CM_HOST},
	{"\377\200ecmd", h_ecmd_command,	CM_HOST},
#ifdef	WANT_FILTER
	{"\377\200filt", h_filt_command,	CM_HOST},
#endif
	{"\377\200help", h_help_command,	CM_HOST},
	{"\377\200host", h_host_command,	CM_UNKNOWN},
	{"\377\200invi", h_invi_command,	CM_HOST},
	{"\377\200info", h_info_command,	CM_HOST},
	{"\377\200link", h_link_command,	CM_HOST},
	{"\377\200loop", h_loop_command,	CM_HOST},
	{"\377\200nj", 	 h_netjoin_command,	CM_HOST},
	{"\377\200mode", mode_command,		CM_HOST},
	{"\377\200obsv", h_user_command,	CM_HOST},
	{"\377\200oper", h_oper_command,	CM_HOST},
	{"\377\200ping", h_ping_command,	CM_HOST},
	{"\377\200pong", h_pong_command,	CM_HOST},
	{"\377\200rout", h_rout_command,	CM_HOST},
	{"\377\200sysi", h_sysi_command,	CM_HOST},
	{"\377\200topi", h_topi_command,	CM_HOST},
	{"\377\200uadd", h_uadd_command,	CM_HOST},
	{"\377\200udat", h_udat_command,	CM_HOST},
	{"\377\200umsg", h_umsg_command,	CM_HOST},
	{"\377\200user", h_user_command,	CM_HOST},
	{"\377\200squit",h_squit_command,	CM_HOST},
	{"\377\200uquit",h_uquit_command,	CM_HOST},
	{"\377\200pass", auth_command,		CM_UNKNOWN},

	{NULL, 0, 0}
};

struct cmdtable cmdtable_irc[] =
{
	{"nick",        irc_nick_command,	CM_UNKNOWN | CM_USER | CM_OBSERVER | CM_HOST },
	{"pass",        irc_pass_command,	CM_UNKNOWN | CM_USER | CM_OBSERVER | CM_HOST },
	//{"server",       irc_server_command,	CM_UNKNOWN | CM_HOST },
	{"user",        irc_user_command,	CM_UNKNOWN | CM_USER | CM_OBSERVER | CM_HOST },
	{"users",	irc_users_command,	CM_USER | CM_OBSERVER},

	{"admin",       irc_admin_command,	CM_UNKNOWN | CM_USER | CM_OBSERVER | CM_HOST},
	{"connect",     irc_connect_command,	CM_USER | CM_OBSERVER},
	{"clink",       irc_clink_command,	CM_USER | CM_OBSERVER},
	{"cnotify",     irc_cnotify_command,	CM_USER | CM_OBSERVER},
	{"ison",        irc_ison_command,	CM_USER | CM_OBSERVER },
	{"links",       irc_links_command,	CM_USER | CM_OBSERVER },
	{"list",        irc_list_command,	CM_USER | CM_OBSERVER },
	{"lusers",      irc_lusers_command,	CM_USER | CM_OBSERVER },
	{"names",       irc_names_command,	CM_USER | CM_OBSERVER },
	{"notice",      irc_notice_command,	CM_USER | CM_HOST },
	{"operator",    irc_operator_command,	CM_USER | CM_OBSERVER },
	{"part",        irc_part_command,	CM_USER | CM_OBSERVER | CM_HOST },
	{"ping",        irc_ping_command,	CM_USER | CM_OBSERVER | CM_HOST },
	{"pong",        irc_pong_command,	CM_USER | CM_OBSERVER },
	{"privmsg",     irc_privmsg_command,	CM_USER | CM_HOST },
	{"squit",       irc_squit_command,	CM_USER | CM_OBSERVER },
	{"userhost",    irc_userhost_command,	CM_USER | CM_OBSERVER },
	{"who",         irc_who_command,	CM_USER | CM_OBSERVER },
	{"whois",	irc_whois_command,	CM_USER | CM_OBSERVER},
	{"whowas",	last_command,		CM_USER | CM_OBSERVER},

	{NULL, 0, 0}
};

static char *getargp;

void setargp(char *str)
{
	getargp = str;
}

char *showargp(void)
{
	return getargp;
}


/* -----------------------------------------------------------------------
 *	Log a link command
 */

void linklog(struct connection *p, int direction, const char *fmt, ...)
{
#if 0
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

	/* TODO: write to per-link file */
	fprintf(stderr, "%s %-10.10s %s", (direction == L_SENT) ? "=>" : "<=", p->name, s);
#endif
}

/*
 * irc notice header
 */

void send_notice(struct connection *cp, const char *fromname, int channel)
{
	char buffer[2048];

	if (!cp->ircmode)
		return;
	if (channel < 0)
		sprintf(buffer, ":%s NOTICE %s :", fromname, cp->nickname);
	else
		sprintf(buffer, ":%s NOTICE #%d :", fromname, channel);
	appendstring(cp, buffer);
}

/* -----------------------------------------------------------------------
 *	Write a string to a socket, with character conversion (for users)
 *	and extra buffering
 */

void appendstring(struct connection *cp, const char *string)
{
	struct mbuf *bp, *p;
	char buffer[4096];
	const char *p_string;
	int len;
	
	if (!cp || !string || !*string || cp->fd == -1)
		return;
	
	if ((len = strlen(string)) < 1)
		return;
	if (cp->type != CT_HOST && strncmp(string, "/\377\200", 3)) {
		char c;
		c = string[len-1];
		if (len > sizeof(buffer)-1)
			len = sizeof(buffer)-1;
		memcpy(buffer, string, len);
		buffer[len] = 0;
		convert(ISO, cp->charset_out, buffer);
		if ((len = strlen(buffer)) < 1) {
			len = 1;
			buffer[0] = c;
			buffer[1] = '0';
		}
		if (buffer[len-1] != '\n' || buffer[len-1] != '\r') {
			buffer[len-1] = c;
		}
		buffer[len] = 0;
		p_string = buffer;
        } else
		p_string = string;
	
	bp = (struct mbuf *) hmalloc(sizeof(struct mbuf) + len + 1);

	bp->next = 0;
	bp->data = memcpy(bp + 1, p_string, len + 1);
	
	if (cp->obuf) {
		for (p = cp->obuf; p->next; p = p->next);
		p->next = bp;
	} else {
		cp->time_write = currtime;
		cp->obuf = bp;
		FD_SET(cp->fd, &chkwrite);
	}
	
	if (cp->time_write + 120L < currtime && queuelength(cp->obuf) > ((cp->type == CT_USER) ? max_u_queue : max_h_queue)) {
		if (cp->type != CT_UNKNOWN)
			do_log(L_LINK, "Link failure with %s (outq full: %s)", cp->name, strerror(errno));
		if (cp->type == CT_HOST) {
       			sprintf(buffer, "%s<>%s broken: bad link", myhostname, cp->name);
       			bye_command2(cp, buffer);
       		} else {
			bye_command2(cp, "link failure (bad link)");
		}
	}
}

/*---------------------------------------------------------------------------*/

void append_general_notice(struct connection *cp, const char *text)
{
	append_chan_notice(cp, text, -1);
}

/*---------------------------------------------------------------------------*/

void append_chan_notice(struct connection *cp, const char *text, int channel)
{

	if (cp->ircmode) 
		send_notice(cp, myhostname, channel);
	if (text && *text)
		appendstring(cp, text);

}

/*---------------------------------------------------------------------------*/

void appendc(struct connection *cp, const int n, const int ast)
{
	static char x[2];
	char c;
	
	if (cp->type == CT_HOST || cp->ircmode)
		return;

	c = cp->prompt[n];
	if (c == '\0') {
		if (ast) {
			appendstring(cp, "***\n");
		}
	} else {
		x[0] = c;
		x[1] = '\0';
		appendstring(cp, x);
	}
}

/* -----------------------------------------------------------------------
 *	
 */

void appendprompt(struct connection *cp, const int ast)
{
	if (cp->ircmode)
		return;

	if (*cp->query)
		appendc(cp, 1, ast);
	else
		appendc(cp, 2, ast);
		
	fflush(NULL);
}

/* -----------------------------------------------------------------------
 *	Calculate total queue length
 */

int queuelength(const struct mbuf *bp)
{
	int len;
	
	for (len = 0; bp; bp = bp->next)
		len += strlen(bp->data);
		
	return len;
}

/* -----------------------------------------------------------------------
 *	Find a permlink, given it's name, or NULL if no named permlink
 *	wkt@cs.adfa.oz.au
 */
 
struct permlink *find_permlink(const char *name)
{
	int pl;
	struct permlink *p;
	
	if (permarray)
		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			p = permarray[pl];
			if (p && !strcasecmp(p->name, name))
				return (p);
		}
		
	return (NULL);
}

/* -------------------------------------------------------------------------*/

struct destination *find_destination(const char *name)
{

	struct destination *d;

	if (!name || !*name)
		return (NULLDESTINATION);

	for (d = destinations; d; d = d->next) {
		if (!strcasecmp(d->name, name))
			return d;
	}
	return (NULLDESTINATION);
}

/*---------------------------------------------------------------------------*/

void anti_idle_msg(struct connection *cp)
{
	char buffer[2048];

	if (cp->anti_idle_offset > 0) {
		sprintf(buffer, "*** (%s) Anti-Idle timer (every %s). ", ts2(currtime), ts5(cp->anti_idle_offset * 60L));
		sprintf(buffer+strlen(buffer), "Last message: you %s, ", ts5(currtime - cp->time_processed));
		sprintf(buffer+strlen(buffer), "me %s.\n", ts5(currtime - cp->time_write));
		append_general_notice(cp, buffer);
	} else {
		// user does not like nagging messages
		fast_write(cp, ((cp->ircmode || !cp->anti_idle_offset) ? (cp->ax25 ? "\r" : "\n") : " "), 0);
	}
}

/*---------------------------------------------------------------------------*/

void periodical_jobs(void)
{
	struct jobs *prev, *job;
	struct connection *cp;
	int state;
	time_t when;
	static time_t last_run = 0L;

#define	FORCE_TRIGGER	PERMLINK_MAXIDLE

#define	free_job() {			\
	if (job->command)		\
		hfree(job->command);	\
	hfree(job);			\
}

	// run every minute
	if (currtime - last_run < 60L)
		return;

	for (cp = connections; cp; cp = cp->next) {
		if (cp->type == CT_CLOSED)
			continue;
		if (cp->anti_idle_offset) {
			when = currtime - (abs(cp->anti_idle_offset) * 60L);
			if (cp->time_processed < when && cp->time_write < when) {
				// timers will be resetted automaticaly
				anti_idle_msg(cp);
			}
		}
		// check if the session is still alive (for e.g. irc users who
		// have joined no channel). if they have no anti-idle timer or
		// the timer is > 60min (considered for normal qso's), then we
		// periodicaly send a \r or \n (depending on the connection
		// type) in order to test if the link is dead.
		if (cp->type == CT_USER && !cp->chan_list && (!cp->anti_idle_offset || (abs(cp->anti_idle_offset) > 60))) {
			when = currtime - (FORCE_TRIGGER * 60L);
			if (cp->time_processed < when && cp->time_write < when) {
				int remember_aio = cp->anti_idle_offset;
				cp->anti_idle_offset = 0;
				anti_idle_msg(cp);
				cp->anti_idle_offset = remember_aio;
			}
		}
		if (cp->type == CT_UNKNOWN || (cp->type == CT_USER && is_banned(cp->name))) {
			// connection may be closed, or user is evil
			continue;
		}
		// characters in cp->ibuf? - don't overwrite
		if (cp->icnt)
			continue;
		state = 0;
outer_loop:
		for (prev = job = cp->jobs; job; job = job->next) {
inner_loop:
			// flood protection
			if (cp->sul && cp->sul->stop_process_until >= currtime + RateLimitMAX)
				break;
			if (job->command && job->when && job->when <= currtime) {

				if (!state) {
					char buffer[64];
					sprintf(buffer, "*** (%s) Running crontab\n", ts2(currtime));
					append_general_notice(cp, buffer);
					state++;
				}
				if (cp->ircmode)
					send_notice(cp, myhostname, -1);
				appendstring(cp, "*** Executing '");
				appendstring(cp, job->command);
				appendstring(cp, "'\n");

				strncpy(cp->ibuf, (cp->ircmode ? (job->command)+1 : job->command), MAX_MTU-2);
				cp->ibuf[MAX_MTU-1] = 0;
				process_input(cp);

				job->when = currtime + (job->interval * 60L);
				if (job->retries) {
					job->retries -= 1;
					if (!job->retries) {
						// mark to be deleted
						job->when = 0L;
					}
				}
			}

			// delete expired jobs
			if (!job->command || job->when == 0L) {
				if (prev == job) {
					cp->jobs = job->next;
					free_job();
					goto outer_loop;
				}
				prev->next = job->next;
				free_job();
				if (!(job = prev->next))
					break;
				goto inner_loop;
			}
		}
		if (state) {
			append_general_notice(cp, "*** crontab finished\n");
		}
	}
	last_run = currtime;
}

/*---------------------------------------------------------------------------*/

void expire_channel_history(void)
{
	struct channel *ch;
	static time_t last_run = 0L;

	// run every minute
	if (currtime - last_run < 60L)
		return;
	for (ch = channels; ch; ch = ch->next) {
		if (ch->chist)
			expire_chist(ch, 0);
	}

	last_run = currtime;
}

/* -------------------------------------------------------------------------
 *	Free a channel structure
 */


void expire_destroyed_channels(void)
{
	struct channel *ch, *ch_prev;
	static time_t last_run = 0L;

	// run every minute
	if (currtime - last_run < 60L)
		return;

again:
	ch_prev = 0;
	for (ch = channels; ch; ch = ch->next) {
		if (ch->expires != 0L) {
			if (ch->expires < currtime) {
				if (ch_prev) {
					ch_prev->next = ch->next;
				} else {
					channels = ch->next;
					ch_prev = 0;
				}
				expire_chist(ch, -2);
				hfree(ch);
				if (!(ch = ch_prev))
					goto again;
				continue;
			}
			if (ch->locked_until != 0L && ch->locked_until < currtime) {
				// expire lock
				ch->locked_until = 0;
				// reset data
				//ch->name[0] = '\0';
				ch->topic[0] = '\0';
				ch->time = 0;
				ch->tsetby[0] = '\0';
				ch->lastby[0] = 0;
				ch->ltime = 0;
				ch->flags = 0;
				ch->locked_until = 0L;
				expire_chist(ch, -2);
			}
		}
		ch_prev = ch;
	}

	last_run = currtime;
}

/*---------------------------------------------------------------------------*/

void destroy_channel(int number, int lock)
{
	struct channel *ch;

	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == number) {
			ch->expires = currtime + 60*60L;
			if (lock) {
				ch->locked_until = currtime + 15*60L;
			} else {
				ch->locked_until = 0;
				// reset data most channel data
				//ch->name[0] = '\0';
				ch->topic[0] = '\0';
				ch->time = 0;
				ch->tsetby[0] = '\0';
				ch->lastby[0] = 0;
				ch->ltime = 0;
				//ch->flags = 0; keep modes until lock timeouted or user rejoined
				ch->locked_until = 0L;
				expire_chist(ch, -2);
			}
			break;
		}
	}
}

/*---------------------------------------------------------------------------*/

void check_opless_channels(void)
{
	struct channel *ch;
	struct clist *cl;
	struct connection *p;
	static time_t last_run = 0L;

	// run every 15 minutes
	if (currtime - last_run < 15*60L)
		return;

	for (ch = channels; ch; ch = ch->next) {
		struct connection *p_prefered = 0;
		int anyone_from_local = 0;	// do this only when we've local users on this channel - will not irritate users on other hosts
		// we don't like the #0 concept? - then we'll not take care for
		// ops on this channel
		if (ch->chan == 0 && ban_zero)
			continue;
		for (p = connections; p; p = p->next) {
			int found = 0;
			if (p->type != CT_USER)
				continue;
			if (p->observer || !user_check(p, p->name))
				continue;
			if (p->channel == ch->chan) {
				if (p->channelop)
					goto chan_has_ops;
				found = 1;
			} else {
				for (cl = p->chan_list; cl; cl = cl->next) {
					if (cl->channel == ch->chan) {
						if (cl->channelop)
							goto chan_has_ops;
						found = 1;
						break;
					}
				}
			}
			if (!found)
				continue;
			if (!p->via)
				anyone_from_local = 1;
			// some condxs
			if (p_prefered) {
				if (p_prefered->isauth < p->isauth) 
					goto is_prefered;
				if (p_prefered->isauth > p->isauth) 
					continue;
				if (*p->away && (!*p_prefered->away || p->atime < p_prefered->atime))
					continue;
				if (p->time > p_prefered->time)
					continue;
			}
is_prefered:
			p_prefered = p;
		}
		if (!anyone_from_local)
			continue;
		if (p_prefered) {
			if (p_prefered->channel == ch->chan)
				p_prefered->channelop = 1;
			for (cl = p_prefered->chan_list; cl; cl = cl->next) {
				if (cl->channel == ch->chan) {
					cl->channelop = 1;
					break;
				}
			}
			clear_locks();
			send_opermsg(p_prefered->name, p_prefered->nickname, p_prefered->host, myhostname, myhostname, myhostname, ch->chan, 1);
		}
chan_has_ops:
		; // gcc warned - but i know what i do ;)
	}

	last_run = currtime;
}

/* ------------------------------------------------------------------------
 *	Check that the destination is not me, is defined, has a link,
 *	and via this connection.
 */
 
int dest_check(struct connection *cp, struct destination *dest)
{
	int retval = 0;
	/* bugfix: dest_check returns 0 if link exists in database and
	 * if dest->link is NULLPERMLINK. this may be legal, for e.g. because
	 * of a broken link.  return 1 to accept the new entry link entry for
	 * this * destination.
	 * this bug laid to the problem, that conversd learned only new
	 * destinations. thus, if a link got broken, the destination we
	 * learned from there were marked with d->link 0, d->rtt 0 in our table.
	 * if the link got up again, we received //DEST again, but dest->link
	 * failed the dest->link && dest->link->connection == cp check,
	 * so this dest information was completely ignored.
	 */
	if (dest == NULLDESTINATION || dest->link == NULLPERMLINK)
		retval = 1;
	else {
		if (strcasecmp(dest->name, myhostname) && dest->link)
			retval = ((dest->link->connection == cp) ? 1 : 0);
	}
	
	return retval;
}


/* ------------------------------------------------------------------------
 *	ancient: Check for loop on the user
 *	now: check for special users
 */

int user_check(struct connection *cp, char *user)
{
#ifdef	notdef // the concept bugs
	struct connection *p;
	for (p = connections; p; p = p->next) {
		if ((p->type == CT_USER) && (!strcasecmp(p->name, user) || !strcasecmp(p->nickname, user))) 
			return (p->via && cp != p->via) ? 0 : 1;
	}
	return 1;
#else
	static struct s_bad_users {
		char *name;
	} bad_users[] = {
		/* { "foobar" },	// testing - you may enter calls this way. CAVE: never enter "conversd" here! */
		{ 0 }			// trailling 0 - needed
	};
	struct s_bad_users *bup;

	// here we discard messages or actions from bogus users, like "0"
	if (strlen(user) < 3)
		return 0;
	// be more efficient: intialize user_matches()
	user_matches(user, 0);
	for (bup = bad_users; bup->name; bup++) {
		if (user_matches(0, bup->name))
			return 0;
	}
#endif
	return 1;
}

/* -----------------------------------------------------------------------
 *	Reset destination routes which are not connected, do not have
 *	a link defined, or are from this connection.
 */

void route_cleanup(struct connection *cp)
{
	static struct destination *d;
	
	for (d = destinations; d; d = d->next) {
		if ((!d->link) || (!d->connected) || (!d->link->connection) || (!d->link->connection->name) || (!cp->name)
		 || (!strcasecmp(d->link->connection->name, cp->name))) {
			d->link = NULLPERMLINK;
			d->rtt = 0;
			d->last_sent_rtt = 0;
			d->connected = 0;
		}
	}
}

/*---------------------------------------------------------------------------*/

void expire_dests(void)
{
	struct destination *d, *d_prev;
	static time_t last_run = 0L;

	// run every 10 minutes
	if (currtime - last_run < 600L)
		return;

again:
	for (d = d_prev = destinations; d; ) {
		if (d->auto_learned && count_user2(d->name))
			d->updated = currtime;
		if ((!d->rtt && !d->link && !d->connected) || (d->auto_learned && currtime - d->updated > 3*60*60L)) {
			// just 2b sure
			if (d->auto_learned && d->link)
				update_destinations(d->link, d->name, 0, "", 0, myhostname);
			if (d == destinations) {
				destinations = d->next;
				goto again;
			}
			d_prev->next = d->next;
			hfree(d);
			d = d_prev->next;
			continue;
		}
		d_prev = d;
		d = d->next;
	}
	last_run = currtime;
}

/*---------------------------------------------------------------------------*/

int ecmd_exists(char *cmdname)
{
	struct extendedcmds *echk;

	echk = ecmds;
	while (echk && echk->name) {
		if (!strcasecmp(echk->name, cmdname))
			return 1;
		echk = echk->next;
	}
	return 0;
}

/*---------------------------------------------------------------------------*/

void free_connection(struct connection *cp)
{
	struct mbuf *bp;
	struct permlink *p = NULLPERMLINK;
	struct jobs *job;
	int pl = -1;

	if (permarray)
		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			p = permarray[pl];
			if (p && (p->connection == cp)) {
					p->connection = NULLCONNECTION;
					p->statetime = currtime;
					break;
			}
		}
		
	if (cp->fd >= 0) {
		FD_CLR(cp->fd, &chkread);
		FD_CLR(cp->fd, &chkwrite);
		if (cp->fd == maxfd)
			while (--maxfd >= 0)
				if (FD_ISSET(maxfd, &chkread))
					break;
		close(cp->fd);
		cp->fd = -1;
	}
#ifdef	WANT_FILTER
	if (cp->filter)
		hfree(cp->filter);
	if (cp->filter)
		hfree(cp->filterwords);
#endif
	while ((bp = cp->obuf) != NULL) {
		cp->obuf = bp->next;
		hfree(bp);
	}
	if (cp->ibuf)
		hfree(cp->ibuf);
	if (cp->ibufq)
		hfree(cp->ibufq);
	while ((job = cp->jobs) != NULL) {
		cp->jobs = job->next;
		if (job->command)
			hfree(job->command);
		hfree(job);
	}

	free_sul_sp(cp);

	if (cp->addr)
		hfree(cp->addr);
#ifdef	notdef
	if ((pl > -1) && (pl < NR_PERMLINKS)) {
		if (p && !p->permanent)
			permarray[pl] = NULLPERMLINK;
	}
#endif
	hfree(cp);
}

/*---------------------------------------------------------------------------*/

void free_closed_connections()
{
	struct connection *cp, *p;
	struct permlink *l;
	int pl;

	for (p = NULLCONNECTION, cp = connections; cp;) {
		l = NULLPERMLINK;
		if (permarray)
			for (pl = 0; pl < NR_PERMLINKS; pl++) {
				if ((permarray[pl]) && (permarray[pl]->connection == cp)) {
					l = permarray[pl];
					break;
				}
			}
			
		if (l) {
			if (cp->type == CT_CLOSED ||
			    (cp->type == CT_UNKNOWN && cp->time + l->waittime - 5 < currtime)) {
				if (p) {
					p->next = cp->next;
					free_connection(cp);
					cp = p->next;
				} else {
					connections = cp->next;
					free_connection(cp);
					cp = connections;
				}
			} else {
				p = cp;
				cp = cp->next;
			}
		} else {
			if (cp->type == CT_CLOSED) {
				if (p) {
					p->next = cp->next;
					free_connection(cp);
					cp = p->next;
				} else {
					connections = cp->next;
					free_connection(cp);
					cp = connections;
				}
			} else {
				p = cp;
				cp = cp->next;
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

char *getarg(char *line, int all)
{
	char *arg;
	int c;

	if (line)
		getargp = line;
	while (isspace(uchar(*getargp)))
		getargp++;
	if (all)
		return getargp;
	arg = getargp;
	while (*getargp && !isspace(uchar(*getargp))) {
		c = tolower(uchar(*getargp));
		*getargp++ = c;
	}
	if (*getargp)
		*getargp++ = '\0';
	return arg;
}

/*---------------------------------------------------------------------------*/

char *getargcs(char *line, int all)
{
	char *arg;

	if (line)
		getargp = line;
	while (isspace(uchar(*getargp)))
		getargp++;
	if (all)
		return getargp;
	arg = getargp;
	while (*getargp && !isspace(uchar(*getargp))) {
		getargp++;
	}
	if (*getargp)
		*getargp++ = '\0';
	return arg;
}

/*---------------------------------------------------------------------------*/

void getTXname(struct connection *cp, char *buf)
{
	char *p = buf;

#ifdef	notdef	/* now in /pers, because the brandmarked cmsgs made problems with /filter in tnn/xnet conversd's */
	if (cp->isauth < 2 && BrandUser)
		*p++ = '~';
#endif
	if (!strcasecmp(cp->nickname, cp->name))
		strcpy(p, cp->name);
	else
		sprintf(p, "%s:%s", cp->name, cp->nickname);
}

/*---------------------------------------------------------------------------*/

char *get_nick_from_name(char *fromname)
{
	char *nick = fromname;

	if ((nick = strchr(nick, ':')) && *(++nick))
		return nick;
	return (*fromname == '~' ? (fromname + 1) : fromname);
}

/*---------------------------------------------------------------------------*/

char *get_user_from_name(char *fromname)
{
	static char user[NAMESIZE+1];
	char *q;

	if (*fromname == '~')
		fromname++;
	if ((q = strchr(fromname, ':'))) {
		int len = (q-fromname > NAMESIZE ? NAMESIZE : q-fromname);
		strncpy(user, fromname, len);
		user[len] = 0;
		return user;
	}
	return fromname;
}

/*---------------------------------------------------------------------------*/

struct connection *find_connection(char *fromname, char *hostname, uint8 ctype)
{
	struct connection *p;
	int look_for_me = (!hostname || !strcasecmp(hostname, myhostname));

	if (!fromname)
		return 0;

	for (p = connections; p; p = p->next) {
		if (p->type != ctype)
			continue;
		if (!p->via && !look_for_me)
			continue;
		if (hostname && strcasecmp(p->host, hostname))
			continue;
		if (!strcasecmp(p->name, fromname))
			break;
	}
	return p;
}

/*---------------------------------------------------------------------------*/

char *find_nickname(char *fromname, char *hostname)
{
	struct connection *p = find_connection(fromname, hostname, CT_USER);
	return (p ? p->nickname : fromname);
}

/*---------------------------------------------------------------------------*/

char *formatline(char *prefix, int offset, char *text, int linelen)
{
	char *f, *t, *x;
	int prefixlen, l, lw;
	static char buf[2048];

	// linelen 0 means no formated output
	if (!linelen) {
		prefixlen = strlen(prefix);
		if (prefixlen > sizeof(buf)-1)
			prefixlen = sizeof(buf)-1;
		strncpy(buf, prefix, prefixlen);
		if (prefixlen > 0 && buf[prefixlen-1] != ' ') {
			if (prefixlen == sizeof(buf)-1)
				prefixlen--;
			buf[prefixlen++] = ' ';
		}
		buf[prefixlen] = 0;
		l = strlen(text);
		if (prefixlen + l > sizeof(buf)-1)
			l = sizeof(buf)-1 - prefixlen;
		strncpy(buf+prefixlen, text, l);
		if (!(prefixlen+l)) {
			*buf = '\n';
			prefixlen = 1; l = 0;
		} else {
			if (buf[prefixlen+l-1] != '\n') {
				if (prefixlen + l == sizeof(buf)-1)
					l--;
				buf[prefixlen + l++] = '\n';
			}
		}
		buf[prefixlen+l] = 0;

		return buf;
	}

	linelen--;
	//prefixlen = strlen(prefix);
	prefixlen = 3; /* will result in "\n    this is the next line" */
	offset += prefixlen;
	for (f = prefix, t = buf; *f; *t++ = *f++);
	l = t - buf;
	f = text;

	for (;;) {
		while (isspace(uchar(*f)))
			f++;
		if (!*f) {
out:
			if (t == buf || *(t-1) != '\n')
				*t++ = '\n';
			*t = '\0';
			return buf;
		}
		for (x = f; *x && !isspace(uchar(*x)); x++);
		lw = x - f;
		if (l > offset && l + 1 + lw > linelen) {
			if (t-buf > sizeof(buf)-3)
				goto out;
			*t++ = '\n';
			l = 0;
		}
		do {
			*t++ = ' ';
			if (t-buf > sizeof(buf)-3)
				goto out;
			l++;
		} while (l < offset+1);
		while (lw--) {
			*t++ = *f++;
			if (t-buf > sizeof(buf)-3)
				goto out;
			l++;
		}
	}
}

/*---------------------------------------------------------------------------*/

char *ts(time_t gmt)
{
	static char buffer[80];
	static char monthnames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	struct tm *tm;

	tm = localtime(&gmt);
	if (gmt + 24 * 60 * 60 > currtime)
		sprintf(buffer, " %2d:%02d", tm->tm_hour, tm->tm_min);
	else
		sprintf(buffer, "%-3.3s %2d", monthnames + 3 * tm->tm_mon, tm->tm_mday);
	return buffer;
}

/*---------------------------------------------------------------------------*/

char *ts2(time_t gmt)
{
	static char buffer[80];
	static char monthnames[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
	struct tm *tm;

	tm = localtime(&gmt);
	if (tm) {
		if (gmt + 24 * 60 * 60 > currtime)
			sprintf(buffer, "%2d:%02d", tm->tm_hour, tm->tm_min);
		else
			sprintf(buffer, "%-3.3s %2d", monthnames + 3 * tm->tm_mon, tm->tm_mday);
	} else
		strcpy(buffer, "?????");
	return buffer;
}

/*---------------------------------------------------------------------------*/

char *ts3(time_t seconds, char *buffer)
{
	if (seconds < 100) {
		sprintf(buffer, "%lds", (long)seconds);
	} else if (seconds < 6000) {
		sprintf(buffer, "%ldm", (long)seconds / 60);
	} else if (seconds < 360000) {
		sprintf(buffer, "%ldh", (long)seconds / 3600);
	} else {
		sprintf(buffer, "%ldd", (long)seconds / 86400);
	}
	return buffer;
}

/*---------------------------------------------------------------------------*/

char *ts4(time_t seconds)
{
	int days, hours, minutes;
	static char buffer[2048];
	char buf[128];

	if (seconds < 0)
		seconds *= -1;

	days = seconds / 86400;
	hours = (seconds / 3600) % 24;
	minutes = (seconds / 60) % 60;
	seconds = seconds % 60;
	buffer[0] = '\0';
	if (days) {
		sprintf(buf, "%d day%s, ", days, (days == 1) ? "" : "s");
		strcat(buffer, buf);
	}
#ifdef	notdef
	if (days + hours) {
		sprintf(buf, "%d hour%s, ", hours, (hours == 1) ? "" : "s");
		strcat(buffer, buf);
	}
	if (days + hours + minutes) {
		sprintf(buf, "%2.2d%s, ", minutes, (minutes == 1) ? "" : "s");
		strcat(buffer, buf);
	}
	sprintf(buf, "%lu second%s.", (long)seconds, (seconds == 1) ? "" : "s");
	strcat(buffer, buf);
#else
	sprintf(buffer+strlen(buffer), "%2.2d:%2.2d:%2.2ld.", hours, minutes, seconds);
#endif
	return buffer;
}

/*---------------------------------------------------------------------------*/

char *ts5(time_t seconds)
{
	int days, hours, minutes;
	static char buffer[2048];

	if (seconds < 0)
		seconds *= -1;

	days = seconds / 86400;
	hours = (seconds / 3600) % 24;
	minutes = (seconds / 60) % 60;
	seconds = seconds % 60;

	buffer[0] = '\0';
	if (days)
		sprintf(buffer, "%dd", days);
	if (hours)
		sprintf(buffer+strlen(buffer), "%dh", hours);
	if (minutes)
		sprintf(buffer+strlen(buffer), "%dm", minutes);
	if (!*buffer || seconds)
		sprintf(buffer+strlen(buffer), "%lds", seconds);
	return buffer;
}

/*---------------------------------------------------------------------------*/

struct connection *alloc_connection(int fd)
{
	struct connection *cp;
	
	cp = (struct connection *) hcalloc(1, sizeof(struct connection));
	cp->ibuf = (char *) hcalloc(1, MAX_MTU+1);
	cp->ibufq = (char *) hcalloc(1, MAX_IBUFQ_LEN);
	cp->fd = fd;
	cp->time = currtime;
	cp->verbose = 1;	/* if set to 0, hushlogin is enabled */
	cp->lang = 0;
	cp->prompt[0] = 0;
	cp->prompt[1] = 0;
	cp->prompt[2] = 0;
	cp->prompt[3] = 0;
	cp->features = 0;
	*cp->rev = '\0';
	*cp->away = '\0';
	cp->chan_list = NULLCLIST;
	cp->amprnet = 1;
	cp->oldaway = 0;
	cp->atime = currtime;
	cp->mtime = 0;
	cp->idle_timeout = 0;
	cp->anti_idle_offset = 0;
	cp->time_recv = currtime;
	cp->time_write = currtime;
	cp->time_processed = currtime;
	*cp->pers = '\0';
	*cp->nickname = 0;
	cp->shownicks = 1;
	cp->width = DEFAULT_WIDTH;
#ifdef	WANT_FILTER
	cp->filter = NULLCHAR;
	cp->filterwords = NULLCHAR;
#endif
	cp->observer = 0;
	cp->restrictedmode = 0;
	cp->hostallowed = 1;
	cp->next = connections;
	connections = cp;
	cp->type = CT_UNKNOWN;
	cp->session_type = SESSION_UNKNOWN;
        cp->mtu = MAX_MTU;
	cp->compress = CAN_COMP;
	cp->addr = 0;
	cp->charset_in = cp->charset_out = ISO_STRIPED;
	cp->jobs = 0;
	cp->ircmode = 0;
	cp->needauth = 0;
	cp->isauth = 0;
	*cp->pass_got = *cp->pass_want = 0;

	// dummy
	generate_sul(cp, 0, 0);

	// try to get mut for fd
	if (maxfd < fd)
		maxfd = fd;
	FD_SET(fd, &chkread);
	return cp;
}

/*---------------------------------------------------------------------------*/

void accept_connect_request(int l)
{
	FILE *fdi;
	char buffer[2048];
	int addrlen;
	int fd, flags;
	struct sockaddr addr;
	struct connection *cp;
	int allow = 0;
	
	addrlen = sizeof(addr);
	if ((fd = accept(listeners[l].fd, &addr, &addrlen)) < 0) {
		do_log(L_CRIT, "listener[%d].%d (%s), died during accept(): %s\n", l, listeners[l].fd, listeners[l].name, strerror(errno));
		FD_CLR(listeners[l].fd, &chkread);
		FD_CLR(listeners[l].fd, &chkwrite);
		if (listeners[l].fd == maxfd)
                	maxfd--;
		close(listeners[l].fd);
		listeners[l].fd = -1;
		return;
	}
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		do_log(L_ERR, "accept_connect_request(): fcntl(F_GETFL) failed: %s", strerror(errno));
		goto fail;
	}
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1 ||
		fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		do_log(L_ERR, "accept_connect_request(): fcntl(F_SETFL, O_NONBLOCK) failed: %s", strerror(errno));
		goto fail;
	}

	allow = chk_access(&addr, listeners[l].name);
	if ((allow & ACC_DROP)) {
		// drop silently, for e.g. the regulary rmnc connects
		goto fail;
	}

	// protocol specific logging
	*buffer = 0;
#ifdef	HAVE_AF_INET
	if (addr.sa_family == AF_INET) {
		struct sockaddr_in * sin = (struct sockaddr_in *) &addr;
		sprintf(buffer, "%s", inet_ntoa(sin->sin_addr));
	}
#endif
#ifdef	HAVE_AF_UNIX
	if (addr.sa_family == AF_UNIX) {
		strncpy(buffer, listeners[l].name, sizeof(buffer)-1);
		buffer[sizeof(buffer)-1] = 0;
	}
#endif
#ifdef	HAVE_AF_AX25
	if (addr.sa_family == AF_AX25) {
		struct sockaddr_ax25 *sax = (struct sockaddr_ax25 *) &addr;
		sprintf(buffer, "%s", ax25_ntoa(&sax->sax25_call));
	}
#endif
	if (!*buffer) {
		sprintf(buffer, "proto %d, socket %s ", addr.sa_family, listeners[l].name);
	}
	do_log(L_SOCKET, "Incoming connection from %s to %s", buffer, listeners[l].name);

	if (!allow || (allow & ACC_BAN /* actually the same - but who knows */)) {
		if (addr.sa_family == AF_INET) {
			struct sockaddr_in *sin = (struct sockaddr_in *) &addr;
			do_log(L_AUTH, "Refused connection from [%s:%d] to %s ", inet_ntoa(sin->sin_addr), htons(sin->sin_port), listeners[l].name);
		} else {
			do_log(L_AUTH, "Refused connection from %s to %s", buffer, listeners[l].name);
		}
		sprintf(buffer, ":%s 463 * :Your host isn't among the privileged\n", myhostname);
		write(fd, buffer, strlen(buffer));
		goto fail;
	}
	cp = alloc_connection(fd);
	cp->session_type = SESSION_INBOUND;
	cp->addr = hcalloc(1, addrlen);
	memcpy(cp->addr, &addr, addrlen);
	cp->sockname = listeners[l].name;
	if (addr.sa_family != AF_INET) {
#ifdef HAVE_AF_AX25
		cp->ax25 = (addr.sa_family == AF_AX25);
#endif
		cp->mtu = COMP_MTU; /* 256-1, may be useful for "//comp" */
	}
	
	if (!(allow & ACC_HOST)) {
		cp->hostallowed = 0;
	}
	if (allow & ACC_OBSRV) {
		cp->observer = 1;
	}
	if (allow & ACC_RESTR) {
		/* cp->type = -1; -- no */
		cp->restrictedmode = 1;
	}
	if (!(allow & ACC_USER) && !cp->observer && !cp->restrictedmode) {
		cp->restrictedmode += 2;
	}
	if (allow & ACC_AUTH) {
		cp->needauth |= 1;
	}
	
	if (!access(issuefile, R_OK) && (fdi = fopen(issuefile, "r"))) {
		buffer[0] = '*';
		buffer[1] = ' ';
		while (!feof(fdi)) {
			fgets(buffer + 2, 2045, fdi);
			if (!feof(fdi)) {
				appendstring(cp, buffer);
			}
		}
		fclose(fdi);
	}
#ifdef HAVE_AF_AX25
	if (addr.sa_family == AF_AX25 && (restrictedmode || cp->restrictedmode) && !cp->hostallowed) {
		// default call from ax25, ask for channel
		strcpy(cp->ibuf, "/login -3");
		getarg(cp->ibuf, 0);
		name_command(cp);
	}
#endif

	return;

fail:
	close(fd);
}

/*---------------------------------------------------------------------------*/

int count_user(int channel)
{
	struct connection *p;
	struct clist *cl;
	int n = 0;

	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER) {
			if (p->via && (channel == -1 || p->channel == channel)) {
				n++;
			} else {
				for (cl = p->chan_list; cl; cl = cl->next) {
					if (channel == -1 || cl->channel == channel) {
						n++;
						break;
					}
				}
			}
		}
	}
	return n;
}

/*---------------------------------------------------------------------------*/

int count_user2(char *host)
{

	struct connection *p, *p_next;
	int n_users = 0;

	for (p = sort_connections(1); p; p = p_next) {
		// cave: do not use p->next, because sort_connections() is invariant
		p_next = sort_connections(0);
		if (p->type != CT_USER)
			continue;
		if (host && strcasecmp(p->host, host))
			continue;
		if (p_next && !strcasecmp(p->name, p_next->name) && !strcasecmp(p->host, p_next->host))
			continue;
		n_users++;
	}

	clear_locks();
	return n_users;
}

/*---------------------------------------------------------------------------*/

int count_topics(void)
{
	int n = 0;
	struct channel *ch;

	for (ch = channels; ch; ch = ch->next) {
		if (ch->expires != 0L)
			continue;
		if (!(ch->flags & (M_CHAN_S || M_CHAN_I)) && *ch->topic)
			n++;
	}
	return n;
}

/*---------------------------------------------------------------------------*/

int isonchannel(struct connection *cp, char *user, char *host)
{
	struct connection *p;
	struct clist *cl, *cl1;

	if (cp->channel == -1)
		return 0;
	for (p = connections; p; p = p->next) {
		if ((p->type == CT_USER) && p != cp && /* (!p->via) && */ (!strcasecmp(p->name, user) && !strcasecmp(p->host, host))) {
			if (p->channel == cp->channel)
            			return 1;
			for (cl1 = cp->chan_list; cl1; cl1 = cl1->next) {
				if (p->channel == cl1->channel)
            				return 1;
				for (cl = p->chan_list; cl; cl = cl->next) {
					if (cl->channel == cl1->channel)
						return 1;
				}
			}
		}
	}
	return 0;
}

/*---------------------------------------------------------------------------*/

void clear_locks()
{
	struct connection *p;

	for (p = connections; p; p = p->next)
		p->locked = 0;
}

/*---------------------------------------------------------------------------*/

void notify_destinations(struct destination *dest, 
	const char *host, const char *rev, int state)
{
	struct connection *p;
	char buffer[2048];
	char revstr[40];

	if (dest && ((state && dest->connected) || (!state && !dest->connected)))
		return;
	clear_locks();
	if (rev && *rev)
		sprintf(revstr, "(%.10s)", rev);
	else
		revstr[0] = 0;
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && !p->via && p->operator == 2) {
			if (p->ircmode && !p->verbose)
				continue;
			sprintf(buffer, "*** (%s) Server %sing the network: %s %s\n", ts2(currtime), (state) ? "join" : "leav", host, revstr);
			append_general_notice(p, buffer);
			appendprompt(p, 0);
		}
	}
	if (dest) {
		dest->connected = state;
		if (state == 0) {
			dest->link = NULLPERMLINK;
			dest->rtt = 0;
			dest->last_sent_rtt = 0;
		}
	}
}

/*---------------------------------------------------------------------------*/

void send_awaymsg(char *fromname, char *fromnickname, char *hostname, time_t time, char *text, int text_changed)
{
	char buffer[2048];
	struct connection *p;

	if (strlen(text) > AWAYSIZE)
		text[AWAYSIZE] = 0;
	for (p = connections; p; p = p->next) {
		if (p->ircmode) {
			if (!p->locked && (p->type == CT_HOST || (p->type == CT_USER && !p->via && (p->verbose || text_changed) && isonchannel(p, fromname, hostname)))) {
				sprintf(buffer, ":%s!%s@%s MODE %s %ca%s%s", (p->shownicks ? fromnickname : fromname), fromname, hostname, (p->shownicks ? fromnickname : fromname), *text ? '+' : '-', (*text ? " :" : ""), (*text ? text : ""));
				buffer[IRC_MAX_MSGSIZE] = 0;
				appendstring(p, buffer);
				appendstring(p, "\n"),
				p->locked = 1;
			}
			continue;
		}
		if (p->type == CT_HOST) {
			if (!p->locked) {
				if (p->oldaway) {
					sprintf(buffer, "/\377\200AWAY %s %s 69 %ld %s\n", fromname, hostname, (long)time, text);
				} else {
					sprintf(buffer, "/\377\200AWAY %s %s %ld %s\n", fromname, hostname, (long)time, text);
				}
				linklog(p, L_SENT, "%s", &buffer[3]);
				appendstring(p, buffer);
				p->locked = 1;
			}
		} else if (p->type == CT_USER) {
			if (!p->locked && !p->via && (p->verbose || text_changed)) {
#ifdef WANT_FILTER
				if (p->filter || p->filterwords) {
					if (is_filtered(0, fromname, p, text)) {
						p->locked = 1;
						continue;
					}
				}
#endif
				if (isonchannel(p, fromname, hostname)) {
					appendc(p, 0, 0);
					appendc(p, 3, 0);
					if (*text != '\0')
						sprintf(buffer, "*** (%s) %s@%s has gone away:\n    %s\n", ts2(currtime), fromname, hostname, text);
					else
						sprintf(buffer, "*** (%s) %s@%s is back again.\n", ts2(currtime), fromname, hostname);
					append_general_notice(p, buffer);
					appendprompt(p, 0);
					p->locked = 1;
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

char *get_mode_flags(int flags)
{
        static char mode[16];
        char *p = mode;
        p = mode;
        if (flags & M_CHAN_S) {
                *p++ = 's';
        }
        if (flags & M_CHAN_P) {
                *p++ = 'p';
        }
        if (flags & M_CHAN_T) {
                *p++ = 't';
        }
        if (flags & M_CHAN_I) {
                *p++ = 'i';
        }
        if (flags & M_CHAN_M) {
                *p++ = 'm';
        }
        if (flags & M_CHAN_L) {
                *p++ = 'l';
        }
        *p = 0;

        return mode;
}

/*---------------------------------------------------------------------------*/

void send_mode(struct connection *cp, struct channel *ch, int oldflags)
{
	struct connection *p;
	char buffer[2048];
	char flags[16];
	char flags_irc[16];
	char oldflags_irc[16];
	struct clist *cl2;
	char fromname[(NAMESIZE+1)*3+1];

	strncpy(flags, get_mode_flags(ch->flags), sizeof(flags)-1);
	flags[sizeof(flags)-1] = 0;
	strupr(flags);

	flags_irc[0] = '\0';
	oldflags_irc[0] = '\0';
	if (ch->flags & M_CHAN_S) {
		if (!(oldflags & M_CHAN_S))
			strcat(flags_irc, "p");
	}
	if (ch->flags & M_CHAN_P) {
		if (!(oldflags & M_CHAN_P))
			strcat(flags_irc, "i");
	}
	if (ch->flags & M_CHAN_T) {
		if (!(oldflags & M_CHAN_T))
			strcat(flags_irc, "t");
	}
	if (ch->flags & M_CHAN_I) {
		if (!(oldflags & M_CHAN_I))
			strcat(flags_irc, "s");
	}
	if (ch->flags & M_CHAN_M) {
		if (!(oldflags & M_CHAN_M))
			strcat(flags_irc, "m");
	}
	//if (ch->flags & M_CHAN_L) {
		//strcat(flags_irc, "l");
	//}
	if ((oldflags & M_CHAN_S) && !(ch->flags & M_CHAN_S))
		strcat(oldflags_irc, "p");
	if ((oldflags & M_CHAN_P) && !(ch->flags & M_CHAN_P))
		strcat(oldflags_irc, "i");
	if ((oldflags & M_CHAN_T) && !(ch->flags & M_CHAN_T))
		strcat(oldflags_irc, "t");
	if ((oldflags & M_CHAN_I) && !(ch->flags & M_CHAN_I))
		strcat(oldflags_irc, "s");
	if ((oldflags & M_CHAN_M) && !(ch->flags & M_CHAN_M))
		strcat(oldflags_irc, "m");

	if (*flags) {
		sprintf(buffer, "*** mode change: %s", flags);
	} else
		sprintf(buffer, "*** mode cleared.");
	add_to_history(ch, "conversd", buffer);

	if (cp) {
		if (cp->type == CT_USER) {
 			sprintf(fromname, "%s!%s@%s", (cp->shownicks ? cp->nickname : cp->name), cp->name, cp->host);
		} else {
 			strcpy(fromname, cp->name);
		}
	} else {
		strcpy(fromname, myhostname);
	}
	for (p = connections; p; p = p->next) {
		if (p->locked)
			continue;
		if (p->via) {
			p->locked = 1;
			continue;
		}
		if (!p->ircmode) {
			if (p->type == CT_HOST) {
				sprintf(buffer, "/\377\200MODE %d -sptiml+%s\n", ch->chan, flags);
				linklog(p, L_SENT, "%s", &buffer[3]);
				appendstring(p, buffer);
			} else if (cp && p == cp && p->type == CT_USER) {
				sprintf(buffer, "*** Flags: %s\n", flags);
				appendstring(p, buffer);
			}
		} else {
			if ((p->type == CT_HOST || p->type == CT_USER)) {
				if (p->type == CT_USER && !(p->operator == 2 && p->verbose)) {
					if (p->channel != ch->chan) {
						for (cl2 = p->chan_list; cl2; cl2 = cl2->next) {
							if (cl2->channel == ch->chan)
								break;
						}
						if (!cl2)
							continue;
					}
				}
				sprintf(buffer, ":%s MODE #%d %s%s%s%s\n", fromname, ch->chan, (*oldflags_irc) ? "-" : "", oldflags_irc, (*flags_irc) ? "+" : "", flags_irc);
				appendstring(p, buffer);
			}
		}
		p->locked = 1;
	}
}

/*---------------------------------------------------------------------------*/

void send_opermsg(char *toname, char *tonickname, char *hostname, char *fromname, char *fromnickname, char *fromhost, int channel, int status_really_changed)
{
	char buffer[2048];
	char chan[128];
	struct connection *p;
	struct clist *cl;
	struct channel *ch;
	int ison = 0;
	int flags = 0;
	char fromstring[(NAMESIZE+1)*3+1];

	if (channel >= 0) {
		*chan = 0;
        	for (ch = channels; ch; ch = ch->next)
                	if (ch->chan == channel)
                        	break;
		if (ch) {
			flags = ch->flags;
			if (flags & M_CHAN_S)
				strcpy(chan, "secret ");
			if (flags & M_CHAN_I)
				strcpy(chan, "invisible ");
			if (channel == debugchannel)
				strcpy(chan, "debug ");
		}
	}

	for (p = connections; p; p = p->next) {
		if (p->locked)
			continue;
		p->locked = 1;
		if (channel >= 0) {
			ison = 0;
			if (p->channel == channel)
				ison = 1;
			else {
				 for (cl = p->chan_list; cl; cl = cl->next) {
                                        if (cl->channel == channel) {
                                                ison = 1;
                                                break;
                                        }
                                }
			}
		}
		if (p->ircmode) {
			if (strcmp(fromname, fromhost))
					sprintf(fromstring, "%s!%s@%s", (p->type == CT_USER && !p->shownicks) ? fromname : fromnickname, fromname, fromhost);
			else
					strcpy(fromstring, fromname);
			if (channel == -1) {
				if (p->type == CT_HOST || (p->type == CT_USER && (p->verbose || !strcasecmp(toname, p->name)) && status_really_changed)) {
					sprintf(buffer, ":%s MODE %s +o\n", fromstring, ((p->type == CT_USER && p->shownicks) ? tonickname : toname));
					appendstring(p, buffer);
				}
			} else {
				if (p->type != CT_USER && p->type != CT_HOST)
					continue;
				if (p->type == CT_USER) {
					if (p->via || !status_really_changed)
						continue;
					if (!ison) {
						if (!p->verbose || (flags & M_CHAN_I) || (*chan && p->operator != 2))
							continue;
					}
#ifdef WANT_FILTER
					if (p->filter) {
						if (is_filtered(0, fromname, p, 0)) {
							p->locked = 1;
							continue;
						}
					}
					if (p->filter) {
						if (is_filtered(0, toname, p, 0)) {
							p->locked = 1;
							continue;
						}
					}
#endif
				}
				sprintf(buffer, ":%s MODE #%d +o %s\n", fromstring, channel, ((p->type == CT_USER && p->shownicks) ? tonickname : toname));
				appendstring(p, buffer);
			}
			continue;
		}
		if (p->type == CT_HOST) {
			// some wconversd's are bogous and send /oper every few seconds. block them if possible
			if (status_really_changed) {
				sprintf(buffer, "/\377\200OPER %s %d %s\n", fromname, channel, toname);
				appendstring(p, buffer);
				linklog(p, L_SENT, "%s", &buffer[3]);
			}
		} else if (p->type == CT_USER) {
			if (p->via || !status_really_changed)
				continue;
#ifdef WANT_FILTER
			if (p->filter) {
				if (is_filtered(0, fromname, p, 0)) {
					p->locked = 1;
					continue;
				}
			}
			if (p->filter) {
				if (is_filtered(0, toname, p, 0)) {
					p->locked = 1;
					continue;
				}
			}
#endif
			if (channel != -1) {
				if (!strcasecmp(p->name, toname) && p->channel == channel) {
					appendc(p, 0, 0);
					appendc(p, 3, 0);
					sprintf(buffer, "*** (%s) %s made you a channel operator for channel %d.\n",
						ts2(currtime), fromname, channel);
					append_chan_notice(p, buffer, channel);
					appendprompt(p, 0);
				} else {
					char chan_name[16];
					int hide_chan_name = (*chan && !ison && p->operator != 2);
					if (!ison) {
						if (!p->verbose || (flags & M_CHAN_I) || (p->operator != 2 && *chan))
								
							continue;
					}
					if (hide_chan_name)
						*chan_name = 0;
					else
						sprintf(chan_name, "%d", channel);

					appendc(p, 0, 0);
					appendc(p, 3, 0);
					sprintf(buffer, "*** (%s) %s@%s is now a channel operator for %schannel%s%s.\n",
						ts2(currtime), toname, hostname, (!ison ? chan : ""), (*chan_name ? " " : ""), chan_name);
					append_chan_notice(p, buffer, channel);
					appendprompt(p, 0);
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

#ifdef	notdef
// currently not in use
void send_persmsg(char *fromname, char *fromnickname, char *hostname, int channel, char *text, int text_changed, time_t time)
{
	char buffer[2048];
	struct connection *p;
	struct channel *ch;
	//char chan[128];
	int mychannel;
	struct clist *cl;

	if (strlen(text) > PERSSIZE)
		text[PERSSIZE] = 0;
	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == channel)
			break;
	}
#ifdef	notdef
	sprintf(chan, "channel %d", channel);
	if (ch) {
		if (ch->flags & M_CHAN_S)
			strcpy(chan, "secret channel");
		if (ch->flags & M_CHAN_I)
			strcpy(chan, "invisible channel");
	}
#endif
	for (p = connections; p; p = p->next) {
		if (p->type == CT_HOST) {
			if (!p->locked) {
				if (!p->ircmode) {
					if (p->amprnet)
						sprintf(buffer, "/\377\200USER %s %s %ld %d %d %s\n", fromname, hostname, (long)time, channel, channel, text);
					else
						sprintf(buffer, "/\377\200UDAT %s %s %s\n", fromname, hostname, (!*text || !strcmp(text, "@")) ? "" : text);
					appendstring(p, buffer);
					linklog(p, L_SENT, "%s", &buffer[3]);
				} else {
					sprintf(buffer, "NICK %s %d %s %s 0 + :%s", fromname, (strcasecmp(hostname, myhostname) ? 1 : 0), fromname, hostname, text);
					buffer[IRC_MAX_MSGSIZE] = 0;
					appendstring(p, buffer);
					appendstring(p, "\n");
				}
				p->locked = 1;
			}
		} else if (p->type == CT_USER) {
			if (p->locked || p->via || (!p->verbose && !text_changed))
				continue;
#ifdef WANT_FILTER
			if (p->filter || p->filterwords) {
				if (is_filtered(0, fromname, p, text)) {
					p->locked = 1;
					continue;
				}
			}
#endif
			mychannel = -1;
			for (cl = p->chan_list; cl; cl = cl->next) {
				if (cl->channel == channel) {
					mychannel = channel;
					break;
				}
			}
			if ((mychannel == channel) || (/* !(ch->flags & M_CHAN_I) && */ p->verbose)) {
				appendc(p, 0, 0);
				appendc(p, 3, 0);
				if (p->ircmode) {
					sprintf(buffer, ":%s 311 %s %s %s %s * :%s%s", myhostname, p->nickname, (p->shownicks ? fromnickname : fromname), fromname, hostname, (*text && *text != '@' && strcmp(text, "~")) ? (*text == '~' ? (text + 1) : text) : "No Info", (*text == '~' ? " [NonAuth]" : ""));
					buffer[IRC_MAX_MSGSIZE] = 0;
					appendstring(p, buffer);
					appendstring(p, "\n");
				} else {
					if ((*text != '\0') && strcmp(text, "@") && strcmp(text, "~")) {
						//sprintf(buffer, "*** (%s) %s@%s on %s set personal text:\n    %s\n", ts2(currtime), fromname, hostname, chan, text);
						sprintf(buffer, "*** (%s) %s@%s set personal text:\n    %s\n", ts2(currtime), fromname, hostname, text);
					} else {
						//sprintf(buffer, "*** (%s) %s@%s on %s removed personal text.\n", ts2(currtime), fromname, hostname, chan);
						sprintf(buffer, "*** (%s) %s@%s removed personal text.\n", ts2(currtime), fromname, hostname);
					}
					append_chan_notice(p, buffer, channel);
				}
				appendprompt(p, 0);
			}
			p->locked = 1;
		}
	}
}
#endif

/*---------------------------------------------------------------------------*/

void send_topic(char *fromname, char *fromnickname, char *hostname, time_t time, int channel, char *text)
{
	char buffer[2048];
	struct connection *p;
	struct channel *ch;
	char chan[128];
	char *chr;
	int mychannel, flags = 0;
	struct clist *cl;

        if (strlen(text) > TOPICSIZE)
		text[TOPICSIZE] = 0;

        for (ch = channels; ch; ch = ch->next)
                if (ch->chan == channel)
                        break;
	*chan = 0;
	if (ch && ch->expires == 0L) {
		flags = ch->flags;
		if (flags & M_CHAN_S)
			strcpy(chan, "secret ");
		if (flags & M_CHAN_I)
			strcpy(chan, "invisible ");
		if (time && time < ch->ctime)
			ch->ctime = time;
		if (ch->time < time) {

			// time correcture on channel creation time, because ctime is not transmitted on user change message
			for (chr = text; *chr != '\0'; chr++) {	/* Cut out bells */
				if (*chr == '\7')
					*chr = ' ';
			}

			// conversd change messages because a of a link reorg somewhere? or because of a user flood?
			if ((!*text && !*ch->topic) || !strcmp(ch->topic, text)) {
				if (!strcasecmp(fromname, ch->tsetby) || !strcmp(fromname, "conversd"))
					return;
			}
			ch->time = time;
			strncpy(ch->topic, text, TOPICSIZE);
			ch->topic[sizeof(ch->topic)-1] = 0;
			strncpy(ch->tsetby, fromname, NAMESIZE);
			ch->tsetby[sizeof(ch->tsetby)-1] = 0;
			if (strcmp(fromname, "conversd")) {
				// update channel topic on channel history (but not when updated after permlink reconnect..)
				if (*text != '\0')
					sprintf(buffer, "*** %s@%s set channel topic:\n    %s", fromname, hostname, text);
				else
					sprintf(buffer, "*** %s@%s removed channel topic.", fromname, hostname);
				add_to_history(ch, "conversd", buffer);
			}

			for (p = connections; p; p = p->next) {
				if (p->type == CT_HOST) {
					if (!p->locked) {
						if (!p->ircmode) {
							sprintf(buffer, "/\377\200TOPI %s %s %ld %d %s\n", fromname, hostname, (long)time, channel, text);
							appendstring(p, buffer);
							linklog(p, L_SENT, "%s", &buffer[3]);
						} else {
							sprintf(buffer, "%s!%s@%s TOPIC #%d :%s", (p->shownicks ? fromnickname : fromname), fromname, hostname, channel, text);
							buffer[IRC_MAX_MSGSIZE] = 0;
							appendstring(p, buffer);
							appendstring(p, "\n");
						}
						p->locked = 1;
					}
				} else if (p->type == CT_USER) {
					if (p->locked || p->via)
							continue;
#ifdef WANT_FILTER
					if (p->filter || p->filterwords) {
						if (is_filtered(0, fromname, p, text)) {
							p->locked = 1;
							continue;
						}
					}
#endif
					mychannel = -1;
					for (cl = p->chan_list; cl; cl = cl->next) {
						if (cl->channel == channel) {
							mychannel = channel;
							break;
						}
					}
					if ((!(ch->flags & M_CHAN_I) && p->verbose) || (mychannel == channel)) {
						int hide_chan_name = (*chan && mychannel != channel && p->operator != 2);
						char chan_name[16];
						if (!p->ircmode) {
							if (hide_chan_name)
								*chan_name = 0;
							else
								sprintf(chan_name, " %d", channel);
							appendc(p, 0, 0);
							appendc(p, 3, 0);
							sprintf(buffer, "*** (%s) %s@%s%s%s%s%s %s channel topic%s%s\n", ts2(currtime), fromname, hostname, (p->channel != channel ? " on " : ""), (p->channel != channel ? (mychannel != channel ? chan : "") : ""), (p->channel != channel ? "channel" : ""), (p->channel != channel ? chan_name : ""), *text ? "set" : "removed", (*text ? ":\n    " : "."), text);

							appendstring(p, buffer);
						} else {
							if (hide_chan_name)
								strcpy(chan_name, "*");
							else
								sprintf(chan_name, "#%d", channel);
							sprintf(buffer, ":%s!%s@%s TOPIC %s :%s", (p->shownicks ? fromnickname : fromname), fromname, hostname, chan_name, text);
							buffer[IRC_MAX_MSGSIZE] = 0;
							appendstring(p, buffer);
							appendstring(p, "\n");

						}
						appendprompt(p, 0);
					}
					p->locked = 1;
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

int get_hash(char *name)
{
	int i, hash;

	hash = 0;
	for (i = 0; i < strlen(name); i++)
		hash = ((hash << 1) + name[i]) & 0xff;
	return hash;
}


/*---------------------------------------------------------------------------*/

void fix_user_logs(void)
{
	DIR *dirp;
	FILE *fp;
	char fname[128];
	char *prefix, *q;
	struct dirent *dp;
	struct stat statbuf;
	
	if (!(prefix = strrchr(userfile, '/')) || !*(++prefix))
		return;

	sprintf(fname, "%s", userfile);
	if (!(q = strrchr(fname, '/')))
		return;
	*q = 0;
	if (!(dirp = opendir(fname)))
		return;

	for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
		if (strncmp(dp->d_name, prefix, strlen(prefix)))
			continue;
		q = strrchr(dp->d_name, '.');
		if (strlen(userfile) + strlen(q) > sizeof(fname)-1)
			continue;
		sprintf(fname, "%s%s", userfile, q);
		if (stat(fname, &statbuf))
			continue; 
		if (!S_ISREG(statbuf.st_mode) || S_ISLNK(statbuf.st_mode))
			continue;
		if (!(fp = fopen(fname, "r+")))
			continue;
		while (!feof(fp)) {
			char buffer[512];
			int len;
			long pos;
			pos = ftell(fp);
			if (fgets(buffer, sizeof(buffer), fp) < 0)
				break;
			if ((len = strlen(buffer)) > 2 && buffer[len-2] == '+') {
				long pos2 = ftell(fp);
				buffer[len-2] = '~';
				fseek(fp, pos, SEEK_SET);
				fputs(buffer, fp);
				fseek(fp, pos2, SEEK_SET);
			}
		}
		fclose(fp);
	}
	closedir(dirp);
}

/*---------------------------------------------------------------------------*/

void log_user_change(char *name, char *host, int oldchannel, int newchannel)
{
#if WANT_LOG
	if (oldchannel < 0 || newchannel < 0) {
		FILE *f;
		char fname[128];
		char xname[512];
		long found;
		int len;
		char *p_name = get_tidy_name(name);

		if (!*p_name)
			p_name = name;
		if (!*name)
			return;

		sprintf(fname, "%s.%02x", userfile, get_hash(p_name));
		f = fopen(fname, "r+");
		if (f == NULL) {
			f = fopen(fname, "w+");
			if (f == NULL)
				return;
		}
		strncpy(xname, p_name, NAMESIZE);
		xname[NAMESIZE] = 0;
		len = strlen(xname);
		xname[len++] = '@';
		xname[len] = 0;
		if (len < sizeof(xname)-1) {
		  strncpy(xname+len, host, sizeof(xname)-1 - len);
		  xname[sizeof(xname)-1] = 0;
		}

		/* zuerst die passende zeile suchen */
		found = -1L;
		while (!feof(f)) {
			char buffer[512];
			found = ftell(f);
			fgets(buffer, 512, f);
			if (!strncmp(buffer, xname, strlen(xname))	/*  && 
									   ((newchannel < 0 && strstr(buffer," +")) ||
									   (oldchannel < 0 && strstr(buffer," -")))       */ )
				break;
		}

		rewind(f);
		if (found >= 0L) {
			fseek(f, found, SEEK_SET);
		} else {
			fclose(f);
			f = fopen(userfile, "a");
		}

		if (f != NULL) {
			struct tm *tm;

			tm = localtime(&currtime);
			fprintf(f, "%-20s %2d.%02d.%04d %2d:%02d %c\n",
				xname, tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900, tm->tm_hour, tm->tm_min,
				(oldchannel < 0) ? '+' : '-');
			fclose(f);
		}
	}
#endif
}

/*---------------------------------------------------------------------------*/

int check_for_notify(char *notify, char *name, char *nick, char *host, int channel) {

	char buffer[2048];

	sprintf(buffer, " %s ", name);
	strlwr(buffer);
	if (strstr(notify, buffer))
		return 1;

	sprintf(buffer, " %s ", nick);
	strlwr(buffer);
	if (strstr(notify, buffer))
		return 1;

	sprintf(buffer, " @%s ", host);
	strlwr(buffer);
	if (strstr(notify, buffer))
		return 1;

	sprintf(buffer, " %s@%s ", name, host);
	strlwr(buffer);
	if (strstr(notify, buffer))
		return 1;

	sprintf(buffer, " #%d ", channel);
	if (strstr(notify, buffer))
		return 1;

	return 0;
}

/*---------------------------------------------------------------------------*/

void send_quits(struct connection *cp, char *name, char *host, char *reason)
{
	char buffer[2048];
	struct connection *p, *p2;
	int is_squit = (name ? 0 : 1);

	if (cp->type != CT_USER && cp->type != CT_HOST)
		return;

	// it's more efficient to announce the death of a permlink
	// (and his links) than transmitting a /..USER ... -1 reason
	// message for every user
	// ditto for local users leaving all their hundreds of channels

	clear_locks();
	cp->locked = 1;
	if (is_squit)
		sprintf(buffer, "/\377\200SQUIT %s %s\n", host, reason);
	else
		sprintf(buffer, "/\377\200UQUIT %s %s %s\n", name, host, reason);
	for (p = connections; p; p = p->next) {
		if (p->type == CT_HOST) {
			if (p != cp && (p->features & FEATURE_SAUPP_I)) {
				appendstring(p, buffer);
			}
		} else if (is_squit && p->via && p->via == cp && !strcasecmp(p->host, host)) {
			p->locked = 1;
		}
	}

	// if it's a local or remote UQUIT
	// or a remote-SQUIT (is_squit && cp->host != host)
	// then our job is done here
	if (!is_squit || strcasecmp(cp->name, host))
		return;

	// send SQUIT for every host coming via this broken local permlink link
	// to our neighbours.
	// in the meantime, mark the hosts for the broken link as announced
	for (p = connections; p; p = p->next) {
		if (p->locked)
			continue;
		if (!p->via || p->via != cp)
			continue;
		// found user via the dead link
		for (p2 = connections; p2; p2 = p2->next) {
			// announce
			if (p2->type == CT_HOST) {
				if (p2 != cp && (p2->features & FEATURE_SAUPP_I)) {
					sprintf(buffer, "/\377\200SQUIT %s %s\n", p->host, reason);
					appendstring(p2, buffer);
				}
			}
			if (p2->locked || !p2->via || p2->via != cp)
				continue;
			// else: a user on the same host? mark announced
			if (!strcasecmp(p->host, p2->host))
				p2->locked = 1;
		}
	}
}

/*---------------------------------------------------------------------------*/

void send_user_change_msg(char *name, char *nickname, char *host, int oldchannel, int newchannel, char *pers, int pers_changed, time_t time, int observer, int quits_sent, int announce_chanop_status, int nick_changed, char *oldnick)
{
	char buffer[2048];
	struct connection *p;
	char oldchan[128], newchan[128];
	struct channel *ch;
	int oldflags = 0, newflags = 0;
	int mychannel;
	struct clist *cl;

#define send_buffer(bing) { \
	if (bing) { \
		appendc(p, 0, 0); \
        	appendc(p, 3, 0); \
	} \
	appendstring(p, buffer); \
	appendprompt(p, 0); \
	p->locked = 1; \
}

	*newchan = 0;
	*oldchan = 0;
	if (strlen(pers) > PERSSIZE)
		pers[PERSSIZE] = 0;
	for (ch = channels; ch; ch = ch->next)
		if (ch->chan == newchannel)
			break;
	if (ch) {
		newflags = ch->flags;
		if (newflags & M_CHAN_S) {
			strcpy(newchan, "secret ");
		}
		if (newflags & M_CHAN_I) {
			strcpy(newchan, "invisible ");
		}
		if (ch->chan == debugchannel) {
			strcpy(newchan, "debug ");
		}
	}
	for (ch = channels; ch; ch = ch->next)
		if (ch->chan == oldchannel)
			break;
	if (ch) {
		oldflags = ch->flags;
		if (oldflags & M_CHAN_S) {
			strcpy(oldchan, "secret ");
		}
		if (oldflags & M_CHAN_I) {
			strcpy(oldchan, "invisible ");
		}
		if (ch->chan == debugchannel) {
			strcpy(oldchan, "debug ");
		}
	}
	log_user_change(name, host, oldchannel, newchannel);
	if (!*host) {
		do_log(L_ERR, "error: send_user_change_msg(): empty host for %s - should never happen!", name);
		return;
	}
	for (p = connections; p; p = p->next) {
		if (p->via || p->locked)
			continue;
		if (p->type == CT_USER) {
			mychannel = -1;
#ifdef WANT_FILTER
			if (p->filter || p->filterwords) {
				if (is_filtered(0, name, p, pers)) {
					p->locked = 1;
					continue;
				}
				if (p->filterwords) {
					if (is_filtered(0, 0, p, nickname)) {
						p->locked = 1;
						continue;
					}
				}
			}
#endif
			if (!(oldchannel > 0 && newchannel == -1) && ((newchannel == oldchannel && ((p->verbose && !(newflags & M_CHAN_I)) || isonchannel(p, name, host))))) {
				if (pers_changed) {
					if (p->ircmode) {
						sprintf(buffer, ":%s 311 %s %s %s %s * :%s%s", myhostname, p->nickname, (p->shownicks ? nickname : name), name, host, (*pers && *pers != '@' && strcmp(pers, "~")) ? (*pers == '~' ? (pers + 1) : pers) : "No Info", (*pers == '~' ? " [NonAuth]" : ""));
						buffer[IRC_MAX_MSGSIZE] = 0;
						strcat(buffer, "\n");
					} else {
						if ((*pers != '\0') && strcmp(pers, "@") && strcmp(pers, "~")) {
							//sprintf(buffer, "*** (%s) %s@%s on %s set personal text:\n    %s\n", ts2(currtime), name, host, newchan, (*pers == '~' ? (pers + 1) : pers));
							sprintf(buffer, "*** (%s) %s@%s set personal text:\n    %s\n", ts2(currtime), name, host, (*pers == '~' ? (pers + 1) : pers));
						} else {
							//sprintf(buffer, "*** (%s) %s@%s on %s removed personal text.\n", ts2(currtime), name, host, newchan);
							sprintf(buffer, "*** (%s) %s@%s removed personal text.\n", ts2(currtime), name, host);
						}
					}
					send_buffer(1);
				}
				if (nick_changed && p->shownicks) {
					if (p->ircmode)
						sprintf(buffer, ":%s!%s@%s NICK %s\n", oldnick, name, host, nickname);
					else {
						int nonick = !strcasecmp(name, oldnick);
						sprintf(buffer, "*** (%s) %s%s%s is now known as %s.\n", ts2(currtime), name, (nonick ? "" : ":"), (nonick ? "" : oldnick), nickname);
					}
					send_buffer(1);
				}
			} else {
				if (p->channel == oldchannel)
					mychannel = oldchannel;
				else {
					for (cl = p->chan_list; cl; cl = cl->next) {
						if (cl->channel == oldchannel) {
							mychannel = oldchannel;
							break;
						}
					}
				}
				if (oldchannel >= 0) {
					if ((mychannel == oldchannel) || (!(oldflags & M_CHAN_I) && (p->verbose || check_for_notify(p->notify, name, nickname, host, oldchannel)))) {
						
						int hide_chan_name = (*oldchan && mychannel != oldchannel && p->operator != 2);
						char chan_name[16];
						int special = 0;
						if (p->ircmode) {
							// hack (nicknames in irc are unique, so irc-clients may interprete mess. send as notice, with normal convers message layout. the part to the client is sent in the "leave" user command part, and his sruct p is locked anyway. so we must not care)
							if (mychannel == oldchannel && !strcasecmp(nickname, p->nickname)) {
								// prepend notice header
								send_notice(p, myhostname, hide_chan_name ? -1 : oldchannel);
								special = 1;
							}
						}
						if (!p->ircmode || special) {
							if (hide_chan_name)
								*chan_name = 0;
							else
								sprintf(chan_name, "%d", oldchannel);
							if (pers && (*pers == '\0' || strlen(pers) < 2)) {
								sprintf(buffer, "*** (%s) %s@%s left %schannel%s%s.\n", ts2(currtime), name, host, (mychannel != oldchannel ? oldchan : ""), (*chan_name ? " " : ""), chan_name);
							} else {
								sprintf(buffer, "*** (%s) %s@%s left %schannel%s%s (%s).\n", ts2(currtime), name, host, (mychannel != oldchannel ? oldchan : ""), (*chan_name ? " " : ""), chan_name, (*pers == '~' ? (pers + 1) : pers));
							}
							do_log(L_NUSER, "%s@%s left %i", name, host, oldchannel);
						} else {
							if (hide_chan_name)
								strcpy(chan_name, "*");
							else
								sprintf(chan_name, "#%d", oldchannel);
							sprintf(buffer, ":%s!%s@%s PART %s :%s", (p->shownicks ? nickname : name), name, host, chan_name, (*pers != '@' && strcmp(pers, "~") ? (*pers == '~' ? (pers + 1) : pers)  : ""));
							buffer[IRC_MAX_MSGSIZE] = 0;
							strcat(buffer, "\n");
						}
						send_buffer(1);
					} else {
						// very special
						if (p->operator == 2 && *oldchan == 'd') {
							if (p->ircmode)
								send_notice(p, myhostname, -1);
							if (pers && (*pers == '\0' || strlen(pers) < 2)) {
								sprintf(buffer, "*** (%s) %s@%s left %schannel.\n", ts2(currtime), name, host, oldchan);
							} else {
								sprintf(buffer, "*** (%s) %s@%s left %schannel (%s).\n", ts2(currtime), name, host, oldchan, (*pers == '~' ? (pers + 1) : pers));
							}
							send_buffer(1);
						}
					}
				}
				// find channel
				if (p->channel == newchannel)
					mychannel = newchannel;
				else {
					for (cl = p->chan_list; cl; cl = cl->next) {
						if (cl->channel == newchannel) {
							mychannel = newchannel;
							break;
						}
					}
				}
				if (newchannel >= 0) {
					int special = 0;
					if ((mychannel == newchannel) || (!(newflags & M_CHAN_I) && (p->verbose || (special = check_for_notify(p->notify, name, nickname, host, newchannel))))) {
						int hide_chan_name = (*newchan && mychannel != newchannel && p->operator != 2);
						char chan_name[16];
						int special2 = 0;
						if (p->ircmode) {
							// hack (nicknames in irc are unique, so irc-clients may interprete mess. send as notice, with normal convers message layout. the part to the client is sent in the "leave" user command part, and his sruct p is locked anyway. so we must not care)
							if (mychannel == newchannel && !strcasecmp(nickname, p->nickname)) {
								// prepend notice header
								send_notice(p, myhostname, hide_chan_name ? -1 : newchannel);
								special2 = 1;
							}
						}
						if (!p->ircmode || special2) {
							if (hide_chan_name)
								*chan_name = 0;
							else
								sprintf(chan_name, "%d", newchannel);
							if (pers && (*pers == '\0' || strlen(pers) < 2 || (p->filter && strstr(p->filter, name)))) {
								sprintf(buffer, "*** (%s) %s@%s joined %schannel%s%s.\n", ts2(currtime), name, host, (mychannel != newchannel ? newchan : ""), (*chan_name ? " " : ""), chan_name);
							} else {
								sprintf(buffer, "*** (%s) %s@%s joined %schannel%s%s:\n    %s\n", ts2(currtime), name, host, (mychannel != newchannel ? newchan : ""), (*chan_name ? " " : ""), chan_name, (*pers == '~' ? (pers + 1) : pers));
							}
						} else {
							if (hide_chan_name)
								strcpy(chan_name, "*");
							else
								sprintf(chan_name, "#%d", newchannel);
							sprintf(buffer, ":%s!%s@%s JOIN :%s\n", (p->shownicks ? nickname : name), name, host, chan_name);
							if (special) {
								appendstring(p, buffer);
								if (!strcasecmp(name, nickname) || !p->shownicks)
									sprintf(buffer, ":%s 303 %s :%s\n", myhostname, p->nickname, name);
								else
									sprintf(buffer, ":%s 303 %s :%s %s\n", myhostname, p->nickname, nickname, name);
							}
							if (observer) {
								appendstring(p, buffer);
								sprintf(buffer, ":%s MODE %s +r\n", host, (p->shownicks ? nickname : name));
							}
						}
						do_log(L_NUSER, "%s@%s joined %s", name, host, newchan);
						send_buffer(1);
					} else {
						// very special
						if (p->operator == 2 && *newchan == 'd') {
							if (p->ircmode)
								send_notice(p, myhostname, -1);
							if (pers && (*pers == '\0' || strlen(pers) < 2)) {
								sprintf(buffer, "*** (%s) %s@%s joined %schannel.\n", ts2(currtime), name, host, newchan);
							} else {
								sprintf(buffer, "*** (%s) %s@%s joined %schannel (%s).\n", ts2(currtime), name, host, newchan, (*pers == '~' ? (pers + 1) : pers));
							}
							send_buffer(1);
						}
					}
				}
			}
		} else if (p->type == CT_HOST) {
			if (time == currtime)
				time++;
			if (p->features & FEATURE_SAUPP_I) {
				// leave, with U/SQUIT?
				if (newchannel < 0 && quits_sent)
					continue;
				// netjoin?
				if ((oldchannel < 0 && newchannel >= 0) || (newchannel == oldchannel)) {
					int nick_reset = (observer || (nick_changed && !strcmp(name, nickname)));
					sprintf(buffer, "/\377\200NJ %s %s %s%s%s",
							name, host, (observer ? "r" : ""), (nick_changed ? (nick_reset ? "-" : "+") : "="), (nick_changed && !nick_reset) ? nickname : "");
					if (newchannel != oldchannel) {
						sprintf(buffer+strlen(buffer), " %d%s %ld",
							newchannel, ((!observer && announce_chanop_status) ? (announce_chanop_status == 2 ? ":@@" : ":@") : ""), time);
					}
					if (!observer && pers_changed)
						sprintf(buffer+strlen(buffer), " :%s", (!observer && pers_changed) ? (!strcmp(pers, "@") ? "" : pers) : "");
					appendstring(p, buffer);
					appendstring(p, "\n");
					
					if (announce_chanop_status)
						p->locked = 1;
					continue;
				}
				// fallback
			}
			if (newchannel != oldchannel || (p->amprnet && newchannel >= 0 && oldchannel >= 0)) {
				sprintf(buffer, "/\377\200%s %s %s %ld %d %d %s\n", (observer) ? "OBSV" : "USER", name, host, (long)time, oldchannel, newchannel, (p->amprnet || newchannel < 0) ? pers : "");
			//log(L_DEBUG, "user_change to %s: %s", p->name, &buffer[3]);
				linklog(p, L_SENT, "%s", &buffer[3]);
				appendstring(p, buffer);
			}
			if ((newchannel >= 0 && oldchannel < 0) || (oldchannel == newchannel)) {
				if ((p->features & FEATURE_NICK) && nick_changed) {
					sprintf(buffer, "/\377\200UADD %s %s %s", name, host, nickname);
					if (pers_changed && *pers && oldchannel == newchannel)
						sprintf(buffer + strlen(buffer), " -1 %s", pers);
					appendstring(p, buffer);
					appendstring(p, "\n");
					linklog(p, L_SENT, "%s", &buffer[3]);
				}
				if (!p->amprnet && !observer && pers_changed && *pers) {
					sprintf(buffer, "/\377\200UDAT %s %s %s\n", name, host, (!*pers || !strcmp(pers, "@")) ? "" : pers);
					appendstring(p, buffer);
					linklog(p, L_SENT, "%s", &buffer[3]);
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

void update_dests_behind(struct destination *d1)
{
	struct destination *d2;
	for (d2 = destinations; d2; d2 = d2->next) {
		if (d2 != d1 || !d2->link || d2->link != d1->link || !d2->rtt || strcasecmp(d2->hisneigh, d1->name))
			continue;
		update_destinations(d2->link, d2->name, d2->rtt, d2->rev, d1->hops /* will be incremented */, d1->name); 
		update_dests_behind(d2);
	}
}

/*---------------------------------------------------------------------------*/

void learn_destination_relation_from_route(char *buffer)
{
	struct destination *d1, *d2;
	char *host, *host_via;

	if (!permarray)
		return;

	host_via = getargcs(buffer+11, 0);
	host = getargcs(0, 0);
	if (!*host_via || !*host)
		return;
	if (!(*host == '-' || *host == '('))
		return;

	host = getargcs(0, 0);
	if (!*host || !isalnum(*host & 0xff))
			return;
	// security / misconfigured hosts
	if (!strcasecmp(host_via, myhostname) || !strcasecmp(host, myhostname) || !strcasecmp(host_via, host))
		return;

	if (!(d1 = find_destination(host_via)) || !d1->link || !d1->rtt)
		return;
	if (!(d2 = find_destination(host)) || !d2->link) {
		// hmm, unknown host. learn it
		update_destinations(d1->link, host, d1->rtt + 99, "-", d1->hops /* will be incremented */, d1->name /* learned neighbour */ ); 
		return;
	}
	if (d1->link != d2->link)
		return;
	// no changes?
	if (d2->hops == d1->hops + 1 && !strcasecmp(d2->hisneigh, d1->name))
		return;
	update_destinations(d2->link, d2->name, d2->rtt, d2->rev, d1->hops /* will be incremented */, d1->name /* learned neighbour */ ); 
	update_dests_behind(d2);
}

/*---------------------------------------------------------------------------*/

int get_user_host(char *s, char **fromname, char **hostname)
{
	static char user[NAMESIZE+1];
	static char host[NAMESIZE+1];

	*user = *host = 0;
	*fromname = 0;
	*hostname = 0;

	if (!s || !*s)
		return 0;

	while (*s && isspace(*s & 0xff))
		s++;
	if (!*s || *s == '@')
		return 0;
	*fromname = s;

	while (*s && !isspace(*s & 0xff) && *s != '@')
		s++;

	if (s - *fromname > NAMESIZE)
		return 0;

	strncpy(user, *fromname, s - *fromname);
	user[s - *fromname] = 0;
	*fromname = user;

	while (*s && isspace(*s & 0xff))
		s++;
	if (!*s || *s != '@')
		return 1;

	s++;
	while (*s && isspace(*s & 0xff))
		s++;

	*hostname = s;

	while (*s && !isspace(*s & 0xff))
		s++;
	if (s > *hostname && *(s-1) == '.')
		s--;

	if (s - *hostname < 1 || s - *hostname > NAMESIZE)
		return 1;

	strncpy(host, *hostname, s - *hostname);
	host[s - *hostname] = 0;
	*hostname = host;

	return 2;
}

/*---------------------------------------------------------------------------*/

int parse_notice_for_user(struct connection *cp, int channel, char *fromname, char *hostname, char *msg)
{

	char buffer[2048];
	int state;
	char *q;
	char *dcc_cmd;

	if (strstr(fromname, "conversd") && strstr(msg, "is up for "))
		return 0;
	
	state = 0;
	if (!strncasecmp(msg, "notice: ", 8)) {
		state = 3; // notice 
		msg += 8;
	}
	if (!strncasecmp(msg, "req :", 5)) {
		state = 1; // privmsg with dcc
		msg += 5;
	} else if (!strncasecmp(msg, "ack :", 5)) {
		state = 2; // notice with dcc
		msg += 5;
	}

	dcc_cmd = 0;
	if (state) {
		if (state < 3) {
			if ((q = strrchr(msg, ':')))
				*q = 0;
			dcc_cmd = "";
		} else {
			while (*msg && isspace(*msg & 0xff))
				msg++;
		}
	} else {
		/* normal /me message */
		dcc_cmd = "ACTION";
		state = 1; // privmsg
	}

	if (cp->ircmode) {
		if (strlen(msg) > IRC_MAX_MSGSIZE)
			msg[IRC_MAX_MSGSIZE] = 0;
		if (channel < 0) {
			sprintf(buffer, ":%s!%s@%s %s %s :", fromname, fromname, hostname, (state == 1 ? "PRIVMSG" : "NOTICE"), cp->name);
		} else {
			sprintf(buffer, ":%s!%s@%s %s #%d :", fromname, fromname, hostname, (state == 1 ? "PRIVMSG" : "NOTICE"), channel);
		}
		appendstring(cp, buffer);
		if (dcc_cmd) {
			appendstring(cp, "\001");
			if (*dcc_cmd) {
				appendstring(cp, dcc_cmd);
				appendstring(cp, " ");
			}
		}
		if (msg && *msg) {
			appendstring(cp, msg);
		}
		if (dcc_cmd)
			appendstring(cp, "\001\n");
		else
			appendstring(cp, "\n");
		return 1;
	} else {
		if (msg && *msg) {
			if (strlen(msg) > MAX_MSGSIZE)
				msg[MAX_MSGSIZE] = 0;
			if (state == 1) {
				if (!strncasecmp(msg, "PING ", 5)) {
					sprintf(buffer, "//ECHO /notice %s ack :", fromname);
					appendstring(cp, buffer);
					appendstring(cp, msg);
					appendstring(cp, ":\n");
					return 1;
				} else if (!strncasecmp(msg, "TALK ", 5)) {
					char *to = msg + 5;
					// skip blanks
					while (*to && isspace(*to & 0xff))
						to++;
					msg = to;
					while (*msg && !isspace(*msg & 0xff))
						msg++;
					if (!*msg || !*to)
						return 0;
					// terminate to
					*msg++ = 0;
					while (*msg && isspace(*msg & 0xff))
						msg++;
					// no message?
					if (!*msg)
						return 0;
					sprintf(buffer, "//TALK %s <%s> %s\n", to, fromname, msg);
					appendstring(cp, buffer);
					return 1;
				} else if (!strcasecmp(msg, "VERSION") || !strcasecmp(msg, "USERINFO") || !strcasecmp(msg, "INFO") || !strcasecmp(msg, "NEWS") || !strcasecmp(msg, "DATE") || !strcasecmp(msg, "ACTIVITY") || !strcasecmp(msg, "CS")) {
					// these have more text than a notice line. do a /query
					sprintf(buffer, "//ECHO /query %s\n", fromname);
					appendstring(cp, buffer);
					sprintf(buffer, "//%s\n", (!strcasecmp(msg, "USERINFO") ? "INFO" : msg));
					appendstring(cp, buffer);
					// reset query to old state
					sprintf(buffer, "//ECHO /query %s\n", cp->query);
					appendstring(cp, buffer);
					return 1;
				}
				return 0;
			} else
				return 0;
		}
		return 0;
	}
	return 0;
}

/*---------------------------------------------------------------------------*/

int interprete_conversd_messages(struct connection *cp, const char *text, int channel)
{
	char buffer[2048];
	char *fromname, *hostname;
	char *q, *r;
	int i;

	if (strncmp(text, "*** ", 4)) {
		// shoud never happen. furthermore, the functions down relay on it
		return 0;
	}

	if (strncmp(text, "*** (", 5) != 0) {
		// always work on a private copy
		strncpy(buffer, text, sizeof(buffer));
		buffer[sizeof(buffer)-1] = 0;
		if (!strncasecmp(buffer, "*** route: ", 11)) {
			learn_destination_relation_from_route(buffer);
			return 0;

		}
		if (cp && (r = strchr(buffer, '@')) && get_user_host(buffer+4, &fromname, &hostname) == 2 && ((q = strstr(r+1, hostname)) && strlen(q) >= (i = strlen(hostname)) /* should always succeed */ )) {
			// *** user@host xxxx notation
			q += i;
			// be gentle, skip leading blanks
			while (*q && isspace(*q & 0xff))
				q++;
			return parse_notice_for_user(cp, channel, fromname, hostname, q);
		}
		return 0;
	}

	// invi answer
	if (cp && !strncmp(text, "*** (", 5)) {
		strncpy(buffer, text, sizeof(buffer));
		buffer[sizeof(buffer)-1] = 0;
		strlwr(buffer);
		if ((q = strstr(buffer, " invitation sent to "))) {
			q += strlen(" invitation sent to ");
			if (get_user_host(q, &fromname, &hostname) == 2) {
				char txbuf[2048];
				if (cp->ircmode) {
					sprintf(txbuf, ":%s 341 %s %s *\n", hostname, cp->name, fromname);
					appendstring(cp, txbuf);
					return 1;
				}
				return 0;
			}
		}
		return 0;
	}

	return 0;
}

/*---------------------------------------------------------------------------*/

#ifdef WANT_FILTER
void send_filter_msg(char *fromname, char *hostname, time_t time, char *filter)
{
	char buffer[2048];
	struct connection *cp;

	for (cp = connections; cp; cp = cp->next)
		if (cp->type == CT_HOST && !cp->locked && (cp->features & FEATURE_FILTER)) {
			sprintf(buffer, "/\377\200FILT %ld %s %s %s\n", time, fromname, hostname, filter);
			appendstring(cp, buffer);
        		cp->locked = 1;
		}
}

/*---------------------------------------------------------------------------*/

int is_filtered(char *from_orig, char *fromuser, struct connection *cp, char *text)
{
	char buffer[2048];
	char *q;

	if (cp->filter && (from_orig || fromuser)) {

		if (from_orig && !strcmp(from_orig, "conversd")) {
			if (!text)
				return 0;
			*buffer = 0;
			if (sscanf(text, "*** %s ", buffer)) {
				if ((q = strchr(buffer, '@')))
					*q = 0;
			}
			if (!*buffer)
				goto badwords;
		} else {
			if (!fromuser)
				goto badwords;
			strncpy(buffer, fromuser, sizeof(buffer)-2);
		}
		buffer[sizeof(buffer)-2] = 0;
		strlwr(buffer);
		strcat(buffer, " ");

		if (strstr(cp->filter, buffer))
			return 1;

		if (strlen(buffer) > 6+1) {
			// max call length
			buffer[6] = 0;
			strcat(buffer, " ");
			if (strstr(cp->filter, buffer))
				return 1;
		}

		// another variant
		strncpy(buffer, get_tidy_name(fromuser), NAMESIZE);
		buffer[NAMESIZE] = 0;
		strlwr(buffer);
		strcat(buffer, " ");

		if (strstr(cp->filter, buffer))
			return 1;
	}

badwords:
	if (cp->filterwords && text) {
		char buffer2[2048];
		char *stored_argp = showargp();
		char *arg;

		strncpy(buffer, cp->filterwords, sizeof(buffer)-1);
		buffer[sizeof(buffer)-1] = 0;
		strlwr(buffer);
		strncpy(buffer2, text, sizeof(buffer2)-1);
		buffer2[sizeof(buffer2)-1] = 0;
		strlwr(buffer2);

		// check every substring of filterwords against text to send
		arg = getarg(buffer, 0);
		while (*arg) {
			if (strstr(buffer2, arg))
				break;
			arg = getarg(0, 0);
		}
		setargp(stored_argp);
		if (*arg)
			return 1;
	}
	return 0;
}
#endif

/*---------------------------------------------------------------------------*/

void send_msg_to_local_user(char *from_orig, char *fromuser, char *fromnick, char *toname, char *text)
{

	char buffer[2048];
	struct connection *p;
	char buf[(NAMESIZE * 2) + 3];

	for (p = connections; p; p = p->next)
		if (p->type == CT_USER && (!strcasecmp(p->name, toname) || !strcasecmp(p->nickname, toname)) && !p->locked)
			if (!p->via) {
#ifdef WANT_FILTER
				if (p->filter || p->filterwords) {
					if (is_filtered(from_orig, fromuser, p, text)) {
						p->locked = 1;
						continue;
					}
				}
#endif
				appendc(p, 0, 0);
				appendc(p, 3, 0);
				if (strcmp(from_orig, "conversd")) {
					if (!p->ircmode) {		
						sprintf(buffer, "<*%s*>:", (p->shownicks ? from_orig : fromuser));
						appendstring(p, formatline(buffer, 0, text, p->width));
					}
					else {
						sprintf(buffer,":%s%s PRIVMSG %s :%s", (*from_orig == '~' ? "~" : ""), (p->shownicks ? fromnick : fromuser), p->nickname, text);
						buffer[IRC_MAX_MSGSIZE] = 0;
						appendstring(p, buffer);
						appendstring(p, "\n");
					}

					if (!p->ircmode && p->away[0] != '\0' && text[0] != '*') {
						getTXname(p, buf);
						sprintf(buffer, "*** (%s) %s is away: %s", ts2(currtime), buf, p->away);
						send_msg_to_local_user("conversd", "conversd", "conversd", fromuser, buffer);
					}
				} else {
					append_general_notice(p, text);
					appendstring(p, "\n");
				}
				appendprompt(p, 0);
				p->locked = 1;
			}
}

/*---------------------------------------------------------------------------*/

void send_server_notice(char *host, char *text, int local_only, int priority)
{

	struct connection *p;
	char buffer[2048];

	for (p = connections; p; p = p->next) {
		if (p->locked || !(p->type == CT_USER || p->type == CT_HOST))
			continue;
		if (p->type == CT_HOST && local_only)
			continue;
		if (p->type == CT_USER) {
			if (p->via || (!priority && !p->verbose && (p->ircmode || p->operator != 2)))
				continue;
		}
		if (p->type == CT_HOST) {
			sprintf(buffer, "/\377\200INFO %s %s\n", host, text);
			appendstring(p, buffer);
			p->locked = 1;
			continue;
		}
		if (p->type == CT_USER) {
			if (p->ircmode)
				sprintf(buffer, ":%s NOTICE %s :%s\n", host, p->name, text);
			else
				sprintf(buffer, "*** (%s) server notice: %s %s\n", ts2(currtime), host, text);
			appendstring(p, buffer);
			p->locked = 1;
		}
	}
}

/*---------------------------------------------------------------------------*/

void add_time_stamp(struct connection *cp)
{
	static char buffer[128];

	// irc clients usually implement this on their own
	if (cp->ircmode)
		return;

	if (cp->timestamp != 0L) {
		// only on changes in minute
		time_t prev_time = cp->timestamp - (cp->timestamp % 60L);
		time_t act_time = currtime - (currtime % 60L);

		if (prev_time != act_time) {
			char s_days[64];
			time_t days = (currtime - cp->timestamp) / ONE_DAY;
			*s_days = 0;
			if (days > 0L)
				sprintf(s_days, " [last msg %ld day%s ago]", days, (days > 1) ? "s" : "");
			sprintf(buffer, "- %s%s -\n", ts2(currtime), s_days);
			appendstring(cp, buffer);
		}
		cp->timestamp = currtime;
	}
}

/*---------------------------------------------------------------------------*/

void send_msg_to_user(char *fromname, char *toname, char *text)
{
	char *fromuser, *fromnick;

	fromnick = get_nick_from_name(fromname);
	fromuser = get_user_from_name(fromname);
	send_msg_to_user2(fromname, fromuser, fromnick, toname, text, 1);
}

/*---------------------------------------------------------------------------*/

void send_msg_to_user2(char *from_orig, char *fromuser, char *fromnick, char *toname, char *text, int flood_checked)
{

	char buffer[2048];
	struct connection *p;
	char buf[(NAMESIZE * 2) + 3];
	int isop = 0;
	int filtered = 0;

	// avoid buffer overflows
	if (strlen(text) > MAX_MSGSIZE)
		text[MAX_MSGSIZE] = 0;

	if (!strcmp(from_orig, "conversd")) {
		interprete_conversd_messages(0, text, -1);
	}

	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && !strcasecmp(p->name, fromuser)) {
			if (p->operator == 2) {
				isop = 1;
				break;
			}
		}
	}

	if (!flood_checked && !isop && is_banned(fromuser))
		return;

	for (p = connections; p; p = p->next)
		if (p->type == CT_USER && (!strcasecmp(p->name, toname) || !strcasecmp(p->nickname, toname)) && !p->locked) {
			if (p->via) {
				if (!p->via->locked) {
					sprintf(buffer, "/\377\200UMSG %s %s %s\n", from_orig, toname, text);
					linklog(p, L_SENT, "%s", &buffer[3]);
					appendstring(p->via, buffer);
					p->via->locked = 1;
					p->locked = 1;
				}
			} else {
#ifdef WANT_FILTER
				if (!filtered && p->filtermsgs) {
					if (p->filtermsgs & MSG_PRIV_CONV) {
						if (!strcasecmp(fromuser, "conversd"))
							filtered |= p->filtermsgs;
					}
					if (p->filtermsgs & MSG_PRIV_USER)
						filtered |= p->filtermsgs;
				}
				if (!filtered && (p->filter || p->filterwords)) {
					if (is_filtered(from_orig, fromuser, p, text)) {
						p->locked = 1;
						filtered |= MSG_USER_FILT;
					}
				}
				if (filtered) {
					p->locked = 1;
					continue;
				}
#endif
				appendc(p, 0, 0);
				appendc(p, 3, 0);
				if (strcmp(from_orig, "conversd")) {
					if (!p->ircmode) {		
						add_time_stamp(p);
						sprintf(buffer, "<*%s*>:", (p->shownicks ? from_orig : fromuser));
						appendstring(p, formatline(buffer, 0, text, p->width));
					}
					else {
						sprintf(buffer,":%s%s PRIVMSG %s :%s", (*from_orig == '~' ? "~" : ""), (p->shownicks ? fromnick : fromuser), p->nickname, text);
						buffer[IRC_MAX_MSGSIZE] = 0;
						appendstring(p, buffer);
						appendstring(p, "\n");
					}

					if (!p->ircmode && p->away[0] != '\0' && text[0] != '*') {
						getTXname(p, buf);
						sprintf(buffer, "*** (%s) %s is away: %s", ts2(currtime), buf, p->away);
						send_msg_to_local_user("conversd", "conversd", "conversd", fromuser, buffer);
					}
				} else {
					if (!interprete_conversd_messages(p, text, -1)) {
						append_general_notice(p, text);
						appendstring(p, "\n");
					}
				}
				appendprompt(p, 0);
				p->locked = 1;
			}
		}

	if (filtered && strcasecmp(fromuser, "conversd")) {
		clear_locks();
		sprintf(buffer,"*** (%s) Your messages are ignored by %s%s.", ts2(currtime), toname, (filtered == MSG_PRIV_USER ? " (no privmsgs)" : ""));
		send_msg_to_user("conversd", fromuser, buffer);
	}

}

/*---------------------------------------------------------------------------*/

void expire_chist(struct channel *ch, int keep_lines)
{
	struct chist *chist;
	int i;

#define	free_chline()	{					\
	chist = ch->chist;					\
	ch->chist = chist->next;				\
	if (chist->name)	hfree(chist->name);	\
	if (chist->text)	hfree(chist->text);	\
	hfree(chist);					\
}

	if (!ch || !ch->chist) return;

	if (keep_lines < -1) {
		while (ch->chist)
			free_chline();
		return;
	}
	if (keep_lines <= 0 || keep_lines > history_lines) {
		keep_lines = history_lines;
		if (keep_lines < 0 || (!keep_lines && history_expires <= 0)) {
			// misconfigured? - expire by default CHIST_MAXLINES
			keep_lines = CHIST_MAXLINES;
		}
	}

	// expire ancient messages
	while (ch->chist && (currtime - ch->chist->time)/60L > history_expires)
		free_chline();
	
	if (!ch->chist || !keep_lines /* only expire by time */)
		return;
	
	// how many entries left?
	for (chist = ch->chist, i = 0; chist; chist = chist->next, i++) ;
	// free ancient head
	while (ch->chist && i-- > keep_lines)
		free_chline();
}

/*---------------------------------------------------------------------------*/

struct chist * alloc_chist(char *fromname, char *text)
{
	struct chist *chist;

	if (!(chist = (struct chist *) hmalloc(sizeof(struct chist))))
		return 0;

	chist->time = currtime;
	chist->name = fromname ? hstrdup(fromname) : 0;
	chist->text = text ? hstrdup(text) : 0;
	chist->next = 0;
	return chist;
}

/*---------------------------------------------------------------------------*/

void add_to_history(struct channel *ch, char *fromname, char *text)
{
	struct chist *chist;

	if (history_lines < 0 || !ch || !fromname || !text || (ch->flags & M_CHAN_I) || (ch->flags & M_CHAN_S))
		return;
	// some statistics
	if (strcmp(fromname, "conversd")) {
		ch->ltime = currtime;
		strncpy(ch->lastby, fromname, NAMESIZE);
		ch->lastby[sizeof(ch->lastby)-1] = 0;
	}
	
	// first, expire old entries. at least, remove oldest entry
	// if history exceeds history_lines (i.e. HIST_MAXLINES)
	expire_chist(ch, history_lines-1);

	// find last entry
	for (chist = ch->chist; chist && chist->next; chist = chist->next) ;

	if (!chist) {
		// head of the list
		ch->chist = alloc_chist(fromname, text);
	} else
		chist->next = alloc_chist(fromname, text);
}


/*---------------------------------------------------------------------------*/

void send_msg_to_channel(char *fromname, int channel, char *text)
{
	char *fromuser, *fromnick;

	fromuser = get_user_from_name(fromname);
	fromnick = get_nick_from_name(fromname);

	send_msg_to_channel2(fromname, fromuser, fromnick, channel, text, 1);
}

/*---------------------------------------------------------------------------*/

void send_msg_to_channel2(char *from_orig, char *fromuser, char *fromnick, int channel, char *text, int flood_checked)
{
	char buffer[2048];
	struct connection *p;
	struct channel *ch;
	char addchan[16];
	struct clist *cl;
	int printit;
	int isop = 0;

	// avoid buffer overflows
	if (strlen(text) > MAX_MSGSIZE)
		text[MAX_MSGSIZE] = 0;
	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == channel)
			break;
	}
	if (!ch)
		return;

	add_to_history(ch, from_orig, text);

	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && (!strcasecmp(p->name, fromuser) || !strcasecmp(p->nickname, fromnick))) {
			p->mtime = currtime;
			if (!isop && p->operator == 2)
				isop = 1;
		}
	}

	if (!flood_checked && !isop && is_banned(fromuser))
		return;

	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER) {
			printit = 0;
			if (p->channel == channel) {
				printit = 1;
				addchan[0] = '\0';
			} else {
				for (cl = p->chan_list; cl; cl = cl->next) {
					if (cl->channel == channel) {
						printit = 1;
						sprintf(addchan, "%d:", channel);
					}
				}
			}
			if (printit) {
				if (p->via) {
					if (!p->via->locked && !(ch->flags & M_CHAN_L)) {
						sprintf(buffer, "/\377\200CMSG %s %d %s\n", from_orig, channel, text);
						linklog(p, L_SENT, "%s", &buffer[3]);
						appendstring(p->via, buffer);
						p->via->locked = 1;
					}
				} else {
					if (!p->locked) {
#ifdef WANT_FILTER
						int filtered = 0;
						if (p->filtermsgs) {
							if (p->filtermsgs & MSG_CHAN_CONV) {
								if (!strcasecmp(fromuser, "conversd"))
									filtered |= p->filtermsgs;
							}
							if (p->filtermsgs & MSG_CHAN_USER)
								filtered |= p->filtermsgs;
						}
						if (!filtered && (p->filter || p->filterwords)) {
							if (is_filtered(from_orig, fromuser, p, text))
								filtered |= MSG_USER_FILT;
						}
						if (filtered) {
							p->locked = 1;
							continue;
						}
#endif
						appendc(p, 0, 0);
						appendc(p, 3, 0);
						if (strcmp(from_orig, "conversd")) {
							if (!p->ircmode) {
								add_time_stamp(p);
								sprintf(buffer, "<%s%s>:", addchan, (p->shownicks ? from_orig : fromuser));
								appendstring(p, formatline(buffer, 0, text, p->width));
							} else {
								sprintf(buffer, ":%s%s PRIVMSG #%d :%s", (*from_orig == '~' ? "~" : ""), (p->shownicks ? fromnick : fromuser), channel, text);
								buffer[IRC_MAX_MSGSIZE] = 0;
								appendstring(p, buffer);
								appendstring(p, "\n");
							}
						} else {
							if (!interprete_conversd_messages(p, text, channel)) {
								append_chan_notice(p, text, channel);
								appendstring(p, "\n");
							}
						}
						appendprompt(p, 0);
			
						p->locked = 1;
					}
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

void kill_hanging_child(int dummy)
{
	do_log(L_CRIT, "write(1) blocked!");
	kill(-1, SIGKILL);
	_exit(3);
}

/*---------------------------------------------------------------------------*/

void send_invi_response_443(char *fromname, char *toname, int channel)
{
	struct connection *p;
	char buffer[2048];

	// invi came from local user? - do answer only to local users, else this will mess up the net
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && !p->via) {
			if (strcasecmp(p->name, fromname))
				continue;
			if (p->ircmode) {
				sprintf(buffer, ":%s 443 %s %s #%d :is already on channel\n", myhostname, fromname, toname, channel);
			} else {
				sprintf(buffer, "*** (%s) User %s is already on this channel.\n", ts2(currtime), toname);
			}
			appendstring(p, buffer);
		}
	}
}

/*---------------------------------------------------------------------------*/

void send_invi_response_341(int sent, char *text, char *fromname, char *toname, int channel, char *is_away)
{
	struct connection *p, *p_real;
	char buffer[2048];

	clear_locks();
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER) {
			if (strcasecmp(p->name, fromname))
				continue;
			if (p->locked || (p->via && p->via->locked))
				continue;
			if (p->via) {
				// will not annonuce twice
				p->via->locked = 1;
				// be careful to write on the right descriptor
				p_real = p->via;
			} else {
				p->locked = 1;
				p_real = p;
			}
			if (p->ircmode) {
				if (sent) {
					sprintf(buffer, ":%s 341 %s %s #%d\n", myhostname, fromname, toname, channel);
					appendstring(p, buffer);
					if (is_away && *is_away) {
						sprintf(buffer, ":%s 301 %s %s :#%s", myhostname, fromname, toname, is_away);
						buffer[IRC_MAX_MSGSIZE] = 0;
						appendstring(p_real, buffer);
						appendstring(p_real, "\n");
					}
				}
			} else {
				if (p->via) {
					// print link layer header
					sprintf(buffer, "/\377\200UMSG conversd %s ", fromname);
					appendstring(p_real, buffer);
				}
				appendstring(p_real, text);
				appendstring(p_real, "\n");
				if (is_away && *is_away) {
					if (p->via) {
						// print link layer header
						sprintf(buffer, "/\377\200UMSG conversd %s ", fromname);
						appendstring(p_real, buffer);
					}
					sprintf(buffer, "*** (%s) %s is away: %s\n", ts2(currtime), fromname, is_away);
					appendstring(p_real, buffer);
				}
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

void send_invite_msg(char *fromname, char *orig_to, int channel)
{

	static char invitetext[] = "\n\007\007*** (%s) Message from %s...\nPlease join %s channel %d.\n\007\007\n";

	static char responsetext[] = "*** (%s) %s Invitation sent to %s @ %s.";
	static char responsetext_fail[] = "*** (%s) %s Invitation not sent to %s @ %s: %s.";

	char buffer[2048];
	char touser[NAMESIZE+1];
	char *tohost;
	int fdtty;
	int fdut;
	struct connection *p;
	struct stat stbuf;
	struct utmp utmpbuf;
	char *is_away = 0;
	int sent = 0;
	int found = 0;
	int state;
	int to_first, to_local, to_any;
	int filtered = 0;
	struct clist *cl;

	strncpy(touser, orig_to, sizeof(touser)-1);
	touser[sizeof(touser)-1] = 0;
	if ((tohost = strchr(touser, '@'))) {
	  *tohost = 0;
	  tohost++;
	  if (!isalnum(*tohost & 0xff))
	    tohost = 0;
	}

	to_first = (tohost) ? 0 : 1;
	if (tohost) {
		to_local = (!strcasecmp(tohost, myhostname) ? 1 : 0);
		to_any = (!strcasecmp(tohost, "all") ? 1 : 0);
	} else {
		to_local = to_any = 0;
	}

	if (!to_first && !to_local && !to_any)
		goto sendtonet;

	for (state = 0; state < 3; state++) {
		for (p = connections; p; p = p->next) {
			if (p->type == CT_USER && (!strcasecmp(p->name, touser) || !strcasecmp(p->nickname, touser))) {
				switch (state) {
				case 0:
					for (cl = p->chan_list; cl; cl = cl->next) {
						if (channel == cl->channel)
							break;
					}
					if (p->channel == channel || (cl && channel == cl->channel)) {
						// point of no return..
						send_invi_response_443(fromname, touser, channel);
						return;
					}
					break;
				case 1:
					if (to_first || to_local || to_any) {
						if (!p->via) {
#ifdef	WANT_FILTER
							if (!filtered && p->filter) {
								filtered = is_filtered(0, fromname, p, 0);
							}
#endif
							if (!filtered) {
								if (!p->ircmode) {
									sprintf(buffer, invitetext, ts2(currtime), fromname, convcmd, channel);
								} else {
									sprintf(buffer, ":%s!%s@%s INVITE %s :#%d\n", fromname, fromname, myhostname, touser, channel);
								}
								p->invitation_channel = channel;
								if (!is_away)
									is_away = (*p->away) ? p->away : 0;
								appendstring(p, buffer);
								sent = 1;
							}
						}
					}
					break;
				default:
					// user online, via another permlink. will ever reach?
					if (!to_local) {
						if (p->via && !p->via->locked) {
							sprintf(buffer, "/\377\200INVI %s %s %d\n", fromname, orig_to, channel);
							linklog(p, L_SENT, "%s", &buffer[3]);
							appendstring(p->via, buffer);
							p->via->locked = 1;
						}
					}
				}
			}
		}
		if (sent) {
 			// forced to invite on every host?
			if (to_first || to_local)
				goto final_response;
		}
	}

	if (filtered && (to_first || to_local))
		goto final_response;

	// ok, we've done what we could do. let's look on our local users
	if (!filtered && (to_first || to_local || to_any) && *UTMP__FILE && (fdut = open(UTMP__FILE, O_RDONLY, 0644)) >= 0) {
		int pid;
		int status;
#ifdef	HAVE_FORKPTY
		int amaster;
                char ptyslave[64];
		FILE *fp;
#endif


		/* dl9sau: we're not running under root, thus we may not be allowed
        	 * to write to the users pty: linux is 0620 for gid tty. let's try to
         	 * write direct. on failure, use the write command. but write needs a re
         	 * pty from the caller - ugly, hugh?
         	 */

		while (read(fdut, (char *) &utmpbuf, sizeof(utmpbuf)) == sizeof(utmpbuf)) {
			if (!strncmp(utmpbuf.ut_name, touser, sizeof(utmpbuf.ut_name))) {
				found++;
				strcpy(buffer, "/dev/");
				strncat(buffer, utmpbuf.ut_line, sizeof(utmpbuf.ut_line));
				if (stat(buffer, &stbuf))
					continue;
				if ((stbuf.st_mode & 002) || (stbuf.st_mode & 020)) {
					do_log(L_DEBUG, "trying direct write(2) for invitation");
					pid = fork();
					if (pid == 0) {
						// gain the needed privilleges for accessing the tty
						int new_uid = getuid();
                				setreuid(new_uid, new_uid);
						// write may block
						if ((fdtty = open(buffer, O_WRONLY | O_NOCTTY, 0644)) < 0)
							_exit(1);
						signal(SIGALRM, kill_hanging_child);
						alarm(2);
						sprintf(buffer, invitetext, ts2(currtime), fromname, convcmd, channel);
						if (write(fdtty, buffer, strlen(buffer)) > 0 && !close(fdtty)) {
							alarm(0);
							// mark success
							_exit(0);
						}
						alarm(0);
						_exit(1);

					}
					if (pid > 0) {
						while (waitpid(pid, &status, 0) != pid)
                                       			;
                               			if (WEXITSTATUS(status) == 0) {
                                       			sent++;
							// success
							continue;
                               			}
					}
				}
#ifdef	HAVE_FORKPTY
				// hmm, perhaps our permissions are not ok, or we are'nt suid. try write(1) instead
				// write(1) needs a pty. suse will fail anyway, because suse needs a logged in user
				// (utmp) for piped write. you may install a write(1) program from debian or redhat ;)
				pid = forkpty(&amaster, ptyslave, NULL, NULL);
        
                		if (pid == 0) {

                        		// the child process
                        		// drop all privileges
                        		if (!getuid()) {
                                		int new_uid = geteuid();
                                		if (setreuid(new_uid, new_uid)) {
                                        		do_log(L_CRIT, "after forkpty: could not drop privileges %d -> %d", getuid(), geteuid());
                                        		_exit(1);
						}
                                	}
                                	do_log(L_DEBUG, "using piped write(1) for invitation");
                                        sprintf(buffer, "/usr/bin/write %s /dev/%s", utmpbuf.ut_name, utmpbuf.ut_line);
                                        signal(SIGALRM, kill_hanging_child);
                                        alarm(2);
                                        if ((fp = popen(buffer, "w"))) {
                                        	fprintf(fp, "%s Invitation from %s:\nPlease join %s channel %d.\n\n", convcmd, fromname, convcmd, channel);
                                         	if (!pclose(fp)) {
                                                	alarm(0);
                                                        // mark success
                                                        _exit(0);
                                                }
					}
				        alarm(0);
                                        _exit(1);
				}

				if (pid > 0) {
                                	while (waitpid(pid, &status, 0) != pid)
                                                ;
                               		if (WEXITSTATUS(status) == 0) {
                                       		sent++;
						// success
						continue;
					}
                                }
#endif	/* HAVE_FORKPTY */
			}
		}
		close(fdut);

		/* do not say "not logged on" for "/invi dl9sau all" and "/invi dl9sau" requests */
		if (!found && (to_first || to_any))
			goto sendtonet;

		if ((to_first && sent /* unspecified invitation succeeded */) ||
				(to_local /* invi for user at us */))
			goto final_response;
	}

sendtonet:
	if (!to_local) {
		// well, not in convers, not on our unix. let's ask the net..
		for (p = connections; p; p = p->next) {
			if (p->type == CT_HOST && !p->locked) {
				sprintf(buffer, "/\377\200INVI %s %s %d\n", fromname, orig_to, channel);
				linklog(p, L_SENT, "%s", &buffer[3]);
				appendstring(p, buffer);
				p->locked = 1;
			}
		}
	}

	if (!sent && !found) { /* not: !filtered - because every conversd would report it */
		return;
	}

final_response:
	// at this point, we are ready to respond, because we do not need our
	// locks anymore
	if (sent)
		sprintf(buffer, responsetext, ts2(currtime), convcmd, touser, myhostname);
	else
		sprintf(buffer, responsetext_fail, ts2(currtime), convcmd, touser, myhostname, (filtered ? "filter active" : (found ? "permission denied" : "not logged in")));
	 send_invi_response_341(sent, buffer, fromname, touser, channel, is_away);
}

/*---------------------------------------------------------------------------*/

void update_destinations(struct permlink *p, char *name, long rtt, char *rev, int hops, char *hisneigh)
{
	struct permlink *l;
	struct destination *d, *d1;
	int pl;
	char buffer[2048];
	int force_send = 0;

	if (!permarray || !rev)
		return;

	for (d = destinations; d; d = d->next)
		if (!strcasecmp(d->name, name))
			break;

	hops++;

	if (!d) {
		if (!(d = (struct destination *)hcalloc(1, sizeof(struct destination))))
			return;
		d->last_sent_rtt = 0;
		d->rtt = 0;
		d->connected = 0;
		strncpy(d->name, name, HOSTNAMESIZE);
		d->name[sizeof(d->name)-1] = 0;
		strncpy(d->rev, rev, NAMESIZE);
		d->rev[sizeof(d->rev)-1] = 0;
		d->auto_learned = strcmp(rev, "-") ? 0 : 1;
		d->hops = hops;
		strncpy(d->hisneigh, hisneigh, HOSTNAMESIZE);
		d->hisneigh[sizeof(d->hisneigh)-1] = 0;
		if (!destinations || (!destinations->name) || (strcasecmp(destinations->name, name) > 0)) {
			d->next = destinations;
			destinations = d;
		} else {
			d1 = destinations;
			while (d1->next) {
				if (strcasecmp(d1->next->name, name) > 0) {
					d->next = d1->next;
					d1->next = d;
					break;
				}
				d1 = d1->next;
			}
			if (!d1->next) {
				d->next = d1->next;
				d1->next = d;
			}
		}
		force_send = 1;
	} else {
		if (d->link != NULLPERMLINK && d->link != p && d->rtt && (rtt >= d->rtt)) {
			do_log(L_R_ERR, "CMD ignored from %s: /..DEST %s %ld %s",
			      p->name, name, rtt, rev);
			return;
		}
		//if (strcmp(rev, "-") && (rtt || (!rtt && *rev))) {
			/* I. passive-learned new destinations hack.
			 * all known convers derivates will override rev
			 * when a new rev will occour, so we could send this
			 * information until we have received a real //DEST
			 * but do not overwrite if we have already a more
			 * useful information..
			 * II. //DEST 0 is usualy sent without rev-info - in
			 * this case, keep the old revision information.
			 */
			// test: leave the indicator as is, because then we're able to expire
			// these hosts after a while
			strncpy(d->rev, rev, NAMESIZE);
			d->rev[sizeof(d->rev)-1] = 0;
			d->auto_learned = strcmp(rev, "-") ? 0 : 1;
		//}
		if (d->link == NULLPERMLINK || p == NULLPERMLINK || d->link != p)
			force_send = 1;
	}
	if (strcasecmp(hisneigh, d->hisneigh)) {
		strncpy(d->hisneigh, hisneigh, HOSTNAMESIZE);
		d->hisneigh[sizeof(d->hisneigh)-1] = 0;
		force_send = 1;
	}
	if (d->hops != hops) {
		d->hops = hops;
		force_send = 1;
	}

	d->rtt = rtt;
	d->updated = currtime;

	if (d->rtt) {
		// notify_destinations is called from other routines too, if the node
		// is one of our direkt neighbours (permlink). special care must be taken.
		// especialy to set d->connected manually to 1 (because it's absolutely
		// needed) when notify_destinations() is not // involoed
		if (!d->link) {
			if (!p || strcasecmp(p->name, name)) {
				notify_destinations(d, name, rev, 1);
			} else {
				d->connected = 1;
			}
		}
		//d->connected = 1;
		d->link = p;
		force_send = 1;
	} else {
		if (d->link && strcasecmp(d->link->name, name)) {
			notify_destinations(d, name, "", 0);
		}
		d->link = 0;
		force_send = 1;
	}

	// learned hosts (rev == "-"): forward only when rtt = 0 (backward compatibility)
	if (d->auto_learned && d->rtt)
		goto behind_announce;

	// rtt change is in tolerance?
	if (abs(rtt - d->last_sent_rtt) > (d->last_sent_rtt / 8))
		force_send = 1;

	/* new rtt to our neigbour (computed by ping/pong)? - then recompute
	 * rtt's from known destinations we learned from other permlinks than
	 * him, and if > tolerance, sent actively send them to him
	 * (new algorithm) - so linktimes to nodes will be exacter, even if
	 * they never send new DESTs (because their own link is always
	 * in between the same tolerance)
	 * cave: note that these are only estimated rtt's, and are transmittet
	 * only in a wirder tolerance!
	 */
	if (rtt && d->last_sent_rtt && p != NULLPERMLINK && d->link && d->link == p && (p->connection) && (p->connection->type == CT_HOST) &&
			!strcasecmp(d->name, name) && !strcasecmp(d->link->name, name)) {
		//log(L_DEBUG, "debug: neighour: %s", name);
		for (d1 = destinations; d1; d1 = d1->next) {
			if (!d1->rtt || d1->link == NULLPERMLINK || 
					d1 == d || d1->link == p ||
					!*d1->name || !strcasecmp(d1->name, name) || !strcasecmp(d1->link->name, name))
				continue;
			if (abs((d1->rtt + d->rtt) - (d1->rtt + d->last_sent_rtt)) > ((d1->last_sent_rtt + d->last_sent_rtt) / 3)) {
				// higher tolerance: divisor 3
				//log(L_DEBUG, "(d1->rtt %d + d->rtt %d) - (d1->last_sent_rtt %d + d->last_sent_rtt %d) [%d] >  ((d1->last_sent_rtt + d->last_sent_rtt) / 3)) [%d]", d1->rtt, d->rtt, d1->last_sent_rtt, d->last_sent_rtt, abs((d1->rtt + d->rtt) - (d1->last_sent_rtt + d->last_sent_rtt)), (d1->last_sent_rtt + d->last_sent_rtt) / 3);
				sprintf(buffer, "/\377\200DEST %s %ld %s %d, %s\n", d1->name, d1->rtt + d->rtt, d1->rev, d1->hops, d1->hisneigh);
				linklog(p->connection, L_SENT, "%s", &buffer[3]);
				do_log(L_DEBUG, "debug: updated to %s: %s", p->name, &buffer[3]);
				appendstring(p->connection, buffer);
				d1->last_sent_rtt = d1->rtt;
			} //else do_log(L_DEBUG, "debug: in range: %s", d1->name);
		}
	}

	// rtt of a destination learned from p. should it be transmitted to all our neighbours?

	if (!force_send) {
		// rtt change is in tolerance
		return;
	}

	// send the new rtt to our neighbours
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if (!(l = permarray[pl]) || l == p)
			continue;
		if (!strcasecmp(d->name, l->name) || (d->link && !strcasecmp(d->link->name, l->name)))
			continue;
		if (l->connection && (l->connection->type == CT_HOST) && (d->link == NULLPERMLINK || d->link != p || strcasecmp(d->link->name, l->name))) {
			if (rtt) {
				sprintf(buffer, "/\377\200DEST %s %ld %s %d %s\n", name, rtt + (l->rxtime == -1 ? l->txtime : (l->txtime + l->rxtime) / 2L), d->rev, d->hops, d->hisneigh);
			} else {
				sprintf(buffer, "/\377\200DEST %s 0\n", name);
			}
			linklog(l->connection, L_SENT, "%s", &buffer[3]);
			do_log(L_DEBUG, "debug: sent to %s: %s", l->name, &buffer[3]);
			appendstring(l->connection, buffer);
		}
	}
behind_announce:
	d->last_sent_rtt = rtt;

	// mark the link dead
	if (!rtt) {
		d->link = NULLPERMLINK;
		d->hops = 0;
		d->connected = 0;
		*d->hisneigh = 0;
	}

}

/*---------------------------------------------------------------------------*/

struct permlink *update_permlinks(char *name, struct connection *cp, int isperm)
{
	struct destination *d;
	struct permlink *p = NULL;
	int pl = -1;
	
	/* Haenelt, 4.9.00 */
	//int pingpongintervall = 3600;
	//dl9sau: pp-conversd had 60. but tcp rtt is specified 120s, and ampr
	//is often more worse. well, it's honoured in host.c (pong_command)
	//where it's set to max(..., 120). but, let's start with 3*60s,
	//in order not to burst the new link with destination updates
	//int pingpongintervall = 60*3;
	int pingpongintervall = 60;
	
	if (!permarray)
		permarray = hcalloc(NR_PERMLINKS, sizeof(struct permlink *));
		
	if (cp != NULLCONNECTION)
		notify_destinations(NULLDESTINATION, name, cp->rev, 1);
		
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		p = permarray[pl];
		if (p && !strcasecmp(p->name, name)) {
			for (d = destinations; d; d = d->next) {
				/* dl9sau: bugfix. we had the problem, that on
				 * link failure //DEST 0 was only for the failed
				 * neighbour, but not for the other links we
				 * learned from him. this is, because the
				 * author of notify_destinations() deletes the
				 * d->link information. so update_destinations()
				 * sees "dead links", but does not know their
				 * reference to this link anymore.
				 * that's why we first have to do
				 * update_destinations(), and then
				 * notfify_destinations(), not vice versa.
				 */
				if (d->rtt && (d->link == p))
					update_destinations(p, d->name, 0, "", 0, d->hisneigh);
				if (!isperm && (d->link == p) && cp == NULLCONNECTION)
					notify_destinations(d, d->name, d->rev, 0);
			}
			if (cp) {
				p->connection = cp;
				p->statetime = currtime;
				p->tries = 0;
				p->waittime = 9;
				p->rxtime = 0;
				p->txtime = 0;
				p->testwaittime = currtime;
				p->testnexttime = currtime + pingpongintervall;
				p->retrytime = currtime + p->waittime;
				if (isperm)
					p->permanent = isperm;
			} else {
				p->statetime = currtime;
				p->curr_group = -1;
				if (!isperm && cp == NULLCONNECTION) {
					do_log(L_LINK, "Disconnected from %s", p->name);
					notify_destinations(NULLDESTINATION, p->name, p->connection->rev, 0);
				}
				if (!p->permanent) {
					p->connection = NULLCONNECTION;
#ifdef	notdef	// keep link info
					hfree(p);
					permarray[pl] = NULLPERMLINK;
#endif
				}
				else {
					if (p->retrytime < currtime + 30L) {
						p->retrytime = currtime + 30L;
						delay_permlink_connect(p, 2);
					}
				}
			}
			return p;
		}
	}
	
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if (!permarray[pl])
			break;
	}
	
	if ((pl > -1) && (pl < NR_PERMLINKS)) {
		p = (struct permlink *)hcalloc(1, sizeof(struct permlink));
		strncpy(p->name, name, HOSTNAMESIZE);
		p->curr_group = -1;
		p->connection = cp;
		p->statetime = currtime;
		p->tries = 0;
		p->waittime = 9;
		p->rxtime = 0;
		p->txtime = 0;
		p->testnexttime = currtime + pingpongintervall;
		p->testwaittime = currtime;
		p->retrytime = currtime + p->waittime;
		if (isperm)
			p->permanent = isperm;
		permarray[pl] = p;
	} else {
		//no: h_host (usally) should do this. bye_command2(cp, "too many permlinks"); /* no space left in array */
		return NULLPERMLINK;
	}
	
	return p;
}

/*---------------------------------------------------------------------------*/

char *hide_pw(char *pw)
{
	static char pass[PASSSIZE+1];
	int pwlen, ins_pos;

	pass[0] = 0;
	if (!pw)
		pw = "";

	if ((pwlen = strlen(pw)) >= PASSSIZE)
		return pw;

	ins_pos = conv_random(PASSSIZE - pwlen, 0);
	strcpy(pass, generate_rand_pw(ins_pos));
	strcpy(pass+ins_pos, pw);
	strcpy(pass+ins_pos+pwlen, generate_rand_pw(PASSSIZE - pwlen - ins_pos));

	return pass;
}

/*---------------------------------------------------------------------------*/

void delay_permlink_connect(struct permlink *pl, int delay)
{
	struct permlink * pl2;
	if (!pl || !pl->permanent)
		return;
	if (pl->retrytime <= currtime)
		pl->retrytime = currtime + 1L;
	delay *= 60;
	// make descending links wait a bit
	if (pl->backup) {
		// delay connect to backup link(s) a bit
		pl->backup->retrytime = pl->retrytime + ((long ) ((pl->primary ? 2 : 1) * delay));
		for (pl2 = pl->backup->backup; pl2 && pl2->primary; pl2 = pl2->backup) {
			pl2->retrytime = pl2->primary->retrytime + (long ) delay;
		}

	}
	// make upstream links wait a bit
	for (pl2 = pl->primary; pl2; pl2 = pl2->primary) {
		if (pl2->retrytime < currtime + delay)
			pl2->retrytime += ((long ) delay);
	}
}

/*---------------------------------------------------------------------------*/

void connect_permlinks(void)
{

#define MAX_WAITTIME   (60*60*3)

	char buffer[2048];
	int addrlen;
	int daddrlen;
	int fd;
	int pl;
	int flags;
	struct connection *cp;
	struct permlink *p, *p2;
	char *arg1, *arg2, *arg3;
	char *pw;
	struct sockaddr *addr, *daddr;
	struct destination *d;
	uid_t prev_uid;
	
	/* Update our existing connections */
	if (!permarray)
		return;
		
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if (!(p = permarray[pl]))
			continue;
		if (p->connection && p->connection->type == CT_HOST) {
			if (p->testnexttime < currtime) {
				if ((p->testwaittime + 7300) < currtime) {
					p->rxtime = 0;
					p->txtime = 0;
					for (d = destinations; d; d = d->next) {
						if (d->link == p) {
							update_destinations(p, d->name, 0, "", 0, d->hisneigh);
						}
					}
				}
				linklog(p->connection, L_SENT, "PING\n");
				appendstring(p->connection, "/\377\200PING\n");
				p->testwaittime = currtime;
				p->testnexttime = currtime + 7300;
			}

			// this is a test!
			if (currtime - p->connection->time_recv > (PERMLINK_MAXIDLE / 2)) {
				if (p->connection->idle_timeout) {
					if (currtime > p->connection->idle_timeout) {
						// bad link - give backup links a chance
						char buffer[512];
						char tmp[64];
						delay_permlink_connect(p, 2);
						// inform remote site what happened. helps debuging in case of malfunction
       						sprintf(buffer, "*** disconnecting: %s<>%s broken: locked up for %s%s", myhostname, p->connection->name, ts3(PERMLINK_MAXIDLE, tmp), p->connection->ax25 ? "\r" : "\n");
						fast_write(p->connection, buffer, 0);
       						sprintf(buffer, "%s<>%s broken: locked up for %s", myhostname, p->connection->name, ts3(PERMLINK_MAXIDLE, tmp));
						do_log(L_LINK, buffer);

						bye_command2(p->connection, buffer);
						continue;
					} /* else: not just now. wait */
				} else {
					// time to trigger (once)
					appendstring(p->connection, "/\377\200PING\n");
					p->testwaittime = currtime; // important, because the pong answer is our txtime measurement
					// set idle timeout to rtt + restidle + a bit bonous
					p->connection->idle_timeout = currtime + ((PERMLINK_MAXIDLE + p->rxtime + p->txtime) / 2) + 120;
				}
			} else if (p->connection->idle_timeout) {
				// reset idle timer. we received data
				p->connection->idle_timeout = 0L;
			}
		}

		// expire old dynamik link entries, remove loop-locks
		if (p->connection == NULLCONNECTION) {
			if (p->locked && p->locked_until < currtime)
				p->locked = 0;
			if (!p->permanent && !p->locked && (currtime - p->statetime) > 7300) {
				hfree(p);
				permarray[pl] = NULLPERMLINK;
			}
		}
	}

	/* Now attempt to connect our down connections */
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		p = permarray[pl];
		if (p && !p->connection && !p->locked) {
			if (!p->permanent)
				continue;

			// q&d fix:
			// retrytime sometimes goes up to 12hours. but shoud only currtime+MAX_WAITTIME
			if (p->retrytime > currtime + MAX_WAITTIME) {
				do_log(L_ERR, "debug: correcting retrytime [s]: p->retrytime %ld - currtime %ld = %ld && > %ld. p->waittime: %ld", p->retrytime, currtime, p->retrytime - currtime, MAX_WAITTIME, p->waittime);
				p->retrytime = currtime + MAX_WAITTIME;
			}
			if (p->retrytime < currtime) {
				int i, best;
				/* First see if there are any primary links. If there are and
				 * they are connected, don't connect this link.
				 */
				for (p2 = p->primary; p2; p2 = p2->primary) {
					if (p2->connection && (p2->connection->type == CT_HOST || p2->retrytime - p2->waittime + 120 > currtime))
						break;
				}
				if (p2 && p2->connection) {
					p->retrytime = currtime;
					p->waittime = 300;
					continue;
				}
				/* Ok, no primary connections, let's try and connect */

				// find best link for this host
				best = 0;
				for (i = 1; i < p->groups; i++) {
					if (p->s_groups[i].quality > p->s_groups[best].quality)
						best = i;
				}
				p->curr_group = best;
				p->s_groups[best].quality /= ((p->tries + 1) * 2);
				if (!p->s_groups[best].quality)
					p->curr_group = p->tries % p->groups;

				p->tries++;
				if (p->waittime == 9)
					p->waittime = 300;	/* *2 = 10 minutes */
				p->waittime <<= 1;	/* 10,20,40,80,160,180,180,... min */
				if (p->waittime > MAX_WAITTIME)
					p->waittime = MAX_WAITTIME;
				p->retrytime = currtime + p->waittime;

				// after updating the timers, we'll ignore hosts with no socket information or hosts with dyndns hack addresses
				if (p->permanent == 2)
					continue;
				if (!p->groups || !(p->s_groups[0].socket) || !*(p->s_groups[0].socket)) {
					// mark it as passive link
					p->permanent = 2;
					continue;
				}

				/* save current effective uid - neded for access
				 * to privileged tcp ports
				 */
				prev_uid = geteuid();
				
				strncpy(buffer, p->s_groups[p->curr_group].socket, sizeof(buffer)-1);
				buffer[sizeof(buffer)-1] = 0;
				arg1 = buffer;
				arg3 = 0;
				// host[:port[<host:port]] notation
				if ((arg2 = strchr(buffer, ':'))) {
					arg2++;
					if (*arg2 && *arg2 != '<') {
						if ((arg3 = strchr(buffer, '<'))) {
							*arg3++ = 0;
						}
					} else
						arg2 = 0;
				}
				if (!*arg1)
					goto unknown_protocol;

				if (!strncmp(buffer, "ax25:", 5)) {
#ifdef	HAVE_AF_AX25
				 	/* currently, you say as sysop:
				         *   "/link add te1st 1 1 ax25:te1st-12,te1st [optional commands]"
				         */
					if (!arg2) {
						do_log(L_ERR, "connect_permlinks(): need AX25 host to connect");
						continue;
					}
					// transform komma-seperated digi-list
					for (i = 0; arg2[i]; i++)
						if (arg2[i] == ',')
							arg2[i] = ' ';
					if (!(addr = build_sockaddr(buffer, &addrlen))) {
						do_log(L_ERR, "connect_permlinks(): bad syntax: %s", buffer);
						continue;
					}
#else
					goto unknown_protocol;
#endif /* HAVE_AF_AX25 */
				} else if (!strncmp(arg1, "unix:", 5) || !strncmp(arg1, "local:", 6)) {
#ifdef	HAVE_AF_UNIX
					if (!arg2) {
						do_log(L_ERR, "connect_permlinks(): need unix path to socket");
						continue;
					}
					if (!allowed_unix_paths(arg2)) {
						do_log(L_ERR, "connect_permlinks(): security violation: %s.\nhint: specify allowed unix paths to with a line like \"unix_sockets /tcp/.sockets/\" in %s.conf.", arg2, prgname);
						continue;
					}
					if (!(addr = build_sockaddr(buffer, &addrlen)))
						continue;
#else	/* HAVE_AF_UNIX */
					goto unknown_protocol;
#endif
				} else {
#ifdef	HAVE_AF_INET
					// dummy for dyndns hack: foo.bar.org:0
					if (arg2 && !strcmp(arg2, "0")) {
						// mark him as dyndns dummy host. will never connect again.
						p->permanent = 2;
						continue;
					}
					if (!arg2 || !atoi(arg2)) {
						char tmpbuf[128];
						sprintf(tmpbuf, "%.64s:%d", buffer, listen_port);
						if (!(addr = build_sockaddr(tmpbuf, &addrlen)))
							continue;
					} else {
						if (!(addr = build_sockaddr(buffer, &addrlen)))
							continue;
					}
#else
					goto unknown_protocol;
#endif	/* HAVE_AF_INET */
				}
				
#ifdef HAVE_AF_AX25
				if ((fd = socket(addr->sa_family, (addr->sa_family == AF_AX25 ? SOCK_SEQPACKET : SOCK_STREAM), 0)) < 0) {
#else
				if ((fd = socket(addr->sa_family, SOCK_STREAM, 0)) < 0) {
#endif
					do_log(L_ERR, "connect_permlinks(): socket() failed: %s", strerror(errno));
					continue;
				}
				if ((flags = fcntl(fd, F_GETFL, 0)) == -1 ||
					fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
					close(fd);
					seteuid(prev_uid);
					do_log(L_ERR, "connect_permlinks(): fcntl(O_NONBLOCK) failed: %s", strerror(errno));
					continue;
				}
				
				daddr = hcalloc(1, addrlen);
				memcpy(daddr, addr, addrlen);
                    		daddrlen = addrlen;

/*--------------- start cave -----------------------*/
				/* we must gein have root privileges for several
				 * reasons (prev_uid is set before): have the right to
				 * - bind to a secure source port (INET)
				 * - bind to a reserved ax25-call
				 * - access a protected unix socket file
				 * theres rights are also needed for connect()
				 * take care to set the uid back when we continue on
				 * errors.
				 */
				seteuid(0);
#ifdef	HAVE_AF_INET
				if (addr->sa_family == AF_INET && !geteuid()) {
					char tmpbuf[128];
					if (arg3) {
						if (!strchr(arg3, ':')) {
							sprintf(tmpbuf, "%.64s:%d", arg3, 0);
						} else {
							strncpy(tmpbuf, arg3, sizeof(tmpbuf)-1);
							tmpbuf[sizeof(tmpbuf)-1] = 0;
						}
						
						addr = build_sockaddr(tmpbuf, &addrlen);
					} else {
						addr = build_sockaddr("*:0", &addrlen);
					}

					if (addr) {
						struct sockaddr_in *sin = (struct sockaddr_in *) addr;
						if (sin->sin_port == htons(0)) {
							// default: try to bind to a secure port
							for (i = IPPORT_RESERVED-1; i; i--) {
									sin->sin_port = htons(i);
								if (!bind(fd, addr, addrlen))
									break;
							}
						} else {
							// source port requested - not a really good idea
							bind(fd, addr, addrlen);
						}
					}
				}
#endif
#ifdef	HAVE_AF_AX25
				if (addr->sa_family == AF_AX25 && arg3 && !geteuid()) {
					char tmp[128];
					// transform komma-seperated digi-list
					for (i = 0; arg3[i]; i++)
						if (arg3[i] == ',')
							arg3[i] = ' ';
					sprintf(tmp, "ax25:%.120s", arg3);
					if (!(addr = build_sockaddr(tmp, &addrlen)) ||
							bind(fd, addr, addrlen)) {
						close(fd);
						seteuid(prev_uid);
						do_log(L_ERR, "connect_permlinks(): could bind to %s %ld: %s", arg3, addr, strerror(errno));
						continue;
					}
				}
#endif
#ifdef	HAVE_AF_UNIX
				if (addr->sa_family == AF_UNIX) {
					struct stat statbuf;
					if (stat(addr->sa_data, &statbuf)) {
						close(fd);
						seteuid(prev_uid);
						do_log(L_ERR, "connect_permlinks(): could not stat %s: %s", addr->sa_data, strerror(errno));
						continue;
					}
					if (!S_ISSOCK(statbuf.st_mode)) {
						close(fd);
						seteuid(prev_uid);
						do_log(L_ERR, "connect_permlinks(): not a unix socket: %s.", addr->sa_data);
						continue;
					}
				}
#endif
				
				if (connect(fd, daddr, daddrlen)) {
					if (errno != EINPROGRESS) {
						close(fd);
						seteuid(prev_uid);
						do_log(L_ERR, "connect_permlinks(): connect to %s failed: %s", p->name, strerror(errno));
						continue;
					}
				}

				// at this poing we must drop privliges
				seteuid(prev_uid);
/*---------------- end cave ------------------------*/

				cp = alloc_connection(fd);
				if (!cp) {
					close(fd);
					do_log(L_ERR, "connect_permlinks(): cp == 0 on %s connect", p->name);
					continue;
				}
				cp->session_type = SESSION_OUTBOUND;
				cp->addr = daddr;
				cp->sockname = p->s_groups[p->curr_group].socket;
				if (daddr->sa_family != AF_INET) {
#ifdef HAVE_AF_AX25
					cp->ax25 = (daddr->sa_family == AF_AX25);
#endif
					cp->mtu = COMP_MTU; /* 256-1, may be useful for "//comp" */
				}
				p->connection = cp;

				do_log(L_LINK, "Connecting to %s [%s] (group %d/%d) ...", p->name, cp->sockname, p->curr_group + 1, p->groups);

#ifdef	notdef
				// dl9sau: now handled by h_host()
				/* We have made a connection. Now see if there are any backup
				 * links, and if so, close them down.
				 */
				for (p2 = p->backup; p2; p2 = p2->backup)
					if (p2->connection) {
						bye_command2(p2->connection, "backup link closed");
						free_closed_connections();
						//too evil.. sleep(10); /* EVIL, should not sleep, just send the HOST much later */
					}
#endif
				// optional: send command
				if ((arg1 = p->s_groups[p->curr_group].command)) {
					while (*arg1) {
						*buffer = 0;
						if ((arg2 = strstr(arg1, "\\n"))) {
							//strncat(buffer, arg1, ((arg2 - arg1) < sizeof(buffer)-2) ? (arg2 - arg1) : (sizeof(buffer)-2));
							strncpy(buffer, arg1, ((arg2 - arg1) < sizeof(buffer)-3) ? (arg2 - arg1) : (sizeof(buffer)-3));
							arg1 = arg2 + 2;
						} else {
							strncat(buffer, arg1, sizeof(buffer)-3);
							arg1 = "";
						}
						buffer[sizeof(buffer)-2] = 0;
						strcat(buffer, cp->ax25 ? "\r" : "\n");
						appendstring(cp, buffer);
					}
				}
				sprintf(buffer, "%s%s", convcmd, cp->ax25 ? "\r" : "\n");
				appendstring(cp, buffer);
				if ((pw = read_password(p->name))) {
					sprintf(buffer, "/\377\200PASS %s%s", hide_pw(pw), cp->ax25 ? "\r" : "\n");
					appendstring(cp, buffer);
				}
				// we currently assume, that our connection isn't spoofed
				cp->needauth = 0;
				cp->isauth = 2;
				sprintf(buffer, "/\377\200HOST %s %s %s%s", myhostname, SOFT_REL8, myfeatures, cp->ax25 ? "\r" : "\n");
				linklog(cp, L_SENT, "%s", &buffer[3]);
				appendstring(cp, buffer);
				p->testnexttime = currtime + 5;

				// default
				continue;
unknown_protocol:
				do_log(L_ERR, "connect_permlinks(): unsupported protocol: %s", arg1);

			}
		}
	}
}

/*---------------------------------------------------------------------------*/

void process_irc_input(struct connection *cp)
{
	
	char *cmd; 
	struct cmdtable *cmdp;
	int cmdlen, table;
	char buffer[2048];
	char *p;

	cp->ibuf[MAX_MSGSIZE] = 0;

	irc_sender = 0;

	if (*cp->ibuf == ':') {
		// answer or forwarded info message
		p = getarg(cp->ibuf, 0);
		if (cp->type == CT_HOST) {
			if (*(++p))
				irc_sender = p;
		}
		cmd = getarg(0, 0);
	} else
		cmd = getarg(cp->ibuf, 0);

	if ((cmdlen = strlen(cmd)) < 1)
		return;


	// process irc commands. if not found, conversd cmdtable
	for (table = 0; table < 2; table++) {
		switch (table) {
		case 0:
			cmdp = cmdtable_irc;
			break;
		case 1:
			cmdp = cmdtable;
			break;
		default:
			goto irc_unknown_command;
		}

		while (cmdp->name) {
			if (!strncmp(cmdp->name, cmd, cmdlen)) {
				if (cp->observer && !(cmdp->states & CM_OBSERVER)) {
					sprintf(buffer, OBSERVERSTR);
					append_general_notice(cp, buffer);
					sprintf(buffer, OBSERVERSTR2);
					append_general_notice(cp, buffer);
					appendprompt(cp, 0);
					//return;
					goto irc_unknown_command;
				}
				if (!cp->observer || cmdp->states & CM_OBSERVER) {
					if (cmdp->states & (1 << cp->type)) {
						(*cmdp->fnc) (cp);
						return;
					}
				}
			}
			cmdp++;
		}
	}

irc_unknown_command:
	// command failed
	if (cp->type == CT_USER) {
		sprintf(buffer, ":%s 421 %s %s :Unknown command\n", myhostname, (*cp->nickname ? cp->nickname : "*"), cmd);
		appendstring(cp, buffer);
	}
}

/*---------------------------------------------------------------------------*/

void process_input(struct connection *cp)
{
	char *arg;
	char buffer[2048];
	int arglen;
	struct cmdtable *cmdp;
	struct channel *ch;
	char buf[(NAMESIZE * 2) + 3];

	if (cp->type == CT_CLOSED)
		return;

	clear_locks();
	cp->locked = 1;

	// idle timeout check? - then do not update recv_time
	if ((cp->ircmode && strcasecmp(cp->ibuf, "ping")) || (strncasecmp(cp->ibuf, "/idle", 2) && strncasecmp(cp->ibuf, "/trigger", 2)))
		cp->time_processed = currtime;

	if (cp->type == CT_HOST) {
		if ((!strncmp(cp->ibuf, "*** ", 4)) &&
			cp->ibuf[4] /* htppu fix */ &&
			!strstr(cp->ibuf, " is up for ") /* temporary - bugfix */ ) {
			/* link failure messages on our link layer like
		 	 * "*** DB0TUD: Link failure with DB0DSD" 
		 	 * we only do it on conversd links, not on user connections, in order to
		 	 * allow them to talk about these messages ;)
		 	 */
			getarg(cp->ibuf, 0);
			arg = getargcs(0, 1);
			do_log(L_LINK, "Link failure with %s (%s)", cp->name, arg);
			sprintf(buffer, "%squit%s", cp->ax25 ? "\r" : "\n", (cp->ax25 ? "\r" : "\n"));
			fast_write(cp, buffer, 0);
			sprintf(buffer, "%s<>%s broken (%s)", myhostname, cp->name, buffer),
			bye_command2(cp, arg);
			return;
		}
	} else {
		if (!cp->ircmode && strncmp(cp->ibuf, "/\377\200", 3))
			convert(cp->charset_in, ISO, cp->ibuf);
		cp->ibuf[MAX_MSGSIZE] = 0;
		if (cp->type == CT_USER)
		  update_ratelimit(cp->sul, cp->name, cp->name_len, cp->nickname, cp->nickname_len, cp->ibuf);
	}


	if ((cp->type == CT_UNKNOWN && *cp->ibuf != '/')) {
		// special input in CT_UNKOWN state:
		// "/name" does login (not here),
		// "/auth md5 ..." does md5 authentication (not here)
		// foobar as plaintext password: handled by auth_command()
		// "NICK" and later "USER" does ircmode login. Also the client may
		// say "PASS" or "SERVER".
		// we could not pass all the mess we receive to proces_irc_input(), because
		// the commands are interpreted without /. we had the case when connecting
		// a permlink, he said" /foo: bla bla\n /QUIT  to log out". well, we
		// were CT_UNKNOWN, went to process input (where the leading blanks were
		// striped off) and found "/quit" (actually a //quit extension) in the
		// convers command table, and said goodbye :(
		// btw, if user said /name and has a password, and he responds with PASS xxx
		// (instead of just xxx or /auth xxx"), then his mode is ircmode. this is
		// intensionaly.
		if ( ( (!strncasecmp(cp->ibuf, "NICK", 4) ||
				!strncasecmp(cp->ibuf,"USER", 4) ||
				!strncasecmp(cp->ibuf,"PASS", 4) ||
				(cp->ircmode &&
					!strncasecmp(cp->ibuf, "QUIT", 4)) )
					&& (!cp->ibuf[4] || isspace(cp->ibuf[4]) )) ||
				(!strncasecmp(cp->ibuf, "SERVER", 6)
					&& (!cp->ibuf[6] || isspace(cp->ibuf[6]))) ) {
			process_irc_input(cp);
		} else if ((cp->needauth & 2)) {
			setargp(cp->ibuf);
			auth_command(cp);
		} // else: silently drop
		return;
	}

	// irc input is processed by the irc input handler
	if (cp->ircmode && cp->type != CT_UNKNOWN) {
		process_irc_input(cp);
		return;
	}

	if (*cp->ibuf == '/') {
		if (!strncmp(cp->ibuf, "/\377\200", 3))
			linklog(cp, L_RECV, "%s\n", &cp->ibuf[3]);
		arglen = strlen(arg = getarg(cp->ibuf + 1, 0));
		/* bugfig: "/", "/ " causes a segfault.
		 * and "//" is interpreted as "/quit"
		 */
		if (cp->type == CT_HOST && arglen < 4)
			return;
		if (!arglen || (cp->ibuf[1] == '/' && !cp->ibuf[2])) {
			if (cp->type == CT_HOST)
				return;
			goto unknown_command;
		}
		if (cp->type == CT_HOST) {
			// avoid buffer overflows
			if (strlen(getargp) > 2048 - 8 /* it's strlen("/..ABCD ") */ -1) {
				getargp[2048-8-1] = 0;
			}
		}
		for (cmdp = cmdtable; cmdp->name; cmdp++)
			if (!strncmp(cmdp->name, arg, arglen)) {
				if (cp->observer && !(cmdp->states & CM_OBSERVER)) {
					append_general_notice(cp, OBSERVERSTR);
					append_general_notice(cp, OBSERVERSTR2);
					appendprompt(cp, 0);
					return;
				}
				if (!cp->observer || cmdp->states & CM_OBSERVER)
					if (cmdp->states & (1 << cp->type))
						(*cmdp->fnc) (cp);
				return;
			}
		/* didn't find match in table - see if it is a host command */
		if (cp->type == CT_HOST && !strncmp(cp->ibuf, "/\377\200", 3)) {
			if (!*(--getargp))
				*getargp = ' ';
			h_unknown_command(cp);	/* yep, pass it on */
			return;
		}
		if (cp->type == CT_USER) {
			if (!ecmd_exists(arg)) {
unknown_command:
				sprintf(buffer, "*** Unknown command '/%s'. Type /HELP for help.\n", arg);
				append_general_notice(cp, buffer);
			} else {
				char *params;
				params = getarg(0, 1);
				sprintf(buffer, "/\377\200ECMD %s %s %s\n", cp->name, arg, params);
				strcpy(cp->ibuf, buffer);
				h_unknown_command(cp);
			}
			appendprompt(cp, 0);
		}
		return;
	}
	if (cp->observer)
		append_general_notice(cp, OBSERVERSTR);
	else if (cp->type == CT_USER) {
		getTXname(cp, buf);
		if (*cp->away)
			append_general_notice(cp, "*** You are away, aren't you ? :-)\n");
		if (cp->expected || (cp->needauth & 4)) {
			getarg(cp->ibuf, 1);	/* just fill that static variable */
			operator_command(cp);
			cp->expected = 0;
			cp->needauth &= ~4;
		} else {
			if (check_user_banned(cp, "PRIVMSG"))
				return;
			if (cp->query[0] == '\0') {
				for (ch = channels; ch; ch = ch->next) {
					if (ch->chan == cp->channel)
						break;
				}
				if (!ch)
					return;
				if (cp->operator == 2 || cp->channelop || !(ch->flags & M_CHAN_M))
					send_msg_to_channel(buf, cp->channel, cp->ibuf);
				else
					append_general_notice(cp, "*** This is a moderated channel. Only channel operators may write.\n");
			} else
				send_msg_to_user(buf, cp->query, cp->ibuf);
		}
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static int ours(const char *buf, const char *search)
{
	char buf2[82], search2[82];

	if (strchr(buf, '*'))
		return ((search == NULLCHAR) ? 1 : 0);
	if (search == NULLCHAR)
		return 1;
	strncpy(buf2, buf, sizeof(buf2));
	buf2[sizeof(buf2)-1] = 0;
	strncpy(&search2[1], search, sizeof(search2)-1);
	search2[sizeof(search2)-1] = 0;
	search2[0] = ':';
	strlwr(buf2);
	strlwr(search2);
	return (strstr(buf2, search2) != NULL);
}

/*---------------------------------------------------------------------------*/

int dohelper(struct connection *cp, char *name, const char *search)
{
	char buf[82], buf2[100];
	int count;
	FILE *fp;
	int foundit = 0;
	int didheader = 0;
	char help_buf[sizeof(helpfile)+10+1+1];
	struct stat statbuf;
	int lang = 0;

	if (!cp && (!name || !*name))
		return 0;

	if (cp) {
		// lang 0 is "default" - depending on the local configuration
		if ((lang = cp->lang) < 0) {
			lang = cp->lang = 0;
		}
	}
	if (lang) {
		sprintf(help_buf, "%s.%.10s", helpfile, languages[lang].ext);
		/* does the file for this language exist? - if not, fall
		 * back to the default language file
		 */
		if (stat(help_buf, &statbuf))
			lang = 0;
	}

	clear_locks();
	if ((fp = fopen(((lang) ? help_buf : helpfile), "r")) != (FILE *) 0) {
		if (search == NULLCHAR)
			didheader = 1;
		count = 0;
		do {
			if (fgets(buf, 80, fp)) {
				if (*buf == '#')
					continue;	/* skip comments */
				if (*buf != ':') {
					if (count) {
						count--;
						rip(buf);
						if (!didheader) {
							if (cp) {
								append_general_notice(cp, "*** Usage:\n");
							} else {
								send_msg_to_user("conversd", name, "*** Usage:");
								clear_locks();
							}
							didheader = 1;
						}
						sprintf(buf2, "%s%s%s", (*buf != '/') ? "  " : "", buf, (cp) ? "\n" : "");
						if (cp) {
							append_general_notice(cp, buf2);
						} else {
							send_msg_to_user("conversd", name, buf2);
							clear_locks();
						}
					}
				} else {	/* start of field or continuation */
					count = (ours(buf, search)) ? 32000 : 0;
					if (search == NULLCHAR && count)
						count = 1;
					if (buf[1] == ':' && count == 1)
						count--;
					if (count)
						foundit = 1;
				}
			}
		} while (!feof(fp));
		fclose(fp);
	}
	return (foundit);
}

/*---------------------------------------------------------------------------*/

void buildfnames(char *confdir, char *myself)
{
	sprintf(conffile, "%s/%s.conf", confdir, myself);
	sprintf(motdfile, "%s/%s.motd", confdir, myself);
	sprintf(restrictfile, "%s/%s.restrict", confdir, myself);
	sprintf(accessfile, "%s/%s.access", confdir, myself);
	sprintf(noaccessfile, "%s/%s.noaccess", confdir, myself);
	sprintf(helpfile, "%s/%s.help", confdir, /*myself*/ "convers");
	sprintf(observerfile, "%s/%s.observer", confdir, myself);
	sprintf(issuefile, "%s/%s.issue", confdir, myself);
	sprintf(userfile, "%s/log/%s.log", DATA_DIR, myself);
	sprintf(pidfile, "%s/%s.pid", DATA_DIR, myself);
	if (!strcmp(myself, "wconversd")) {
		listen_port = 3610;
		convcmd = "wconvers";
		sprintf(listener, "unix:%s/sockets/%s", DATA_DIR, "wconvers");
	} else if (!strcmp(myself, "lconversd")) {
		listen_port = 6810;
		convcmd = "lconvers";
		sprintf(listener, "unix:%s/sockets/%s", DATA_DIR, "lconvers");
	} else {
		listen_port = 3600; // default
		//convcmd = "convers";
		sprintf(listener, "unix:%s/sockets/%s", DATA_DIR, myself);
		// fooconversd -> fooconvers
		listener[strlen(listener)-1] = 0;
		convcmd = strrchr(listener, '/');
		convcmd++;
	}
#ifdef	HAVE_AF_UNIX
	listeners[0].name = listener;
#else
	listeners[0].name = "down";
#endif

#if 0
	printf("conffile: '%s'\n", conffile);
	printf("motdfile: '%s'\n", motdfile);
	printf("restrictfile: '%s'\n", restrictfile);
	printf("accessfile: '%s'\n", accessfile);
	printf("noaccessfile: '%s'\n", noaccessfile);
	printf("helpfile: '%s'\n", helpfile);
	printf("observerfile: '%s'\n", observerfile);
	printf("issuefile: '%s'\n", issuefile);
	printf("userfile: '%s'\n", userfile);
	fflush(stdout);
#endif
	return;
}


/*---------------------------------------------------------------------------*/

void update_stats(time_t now)
{
	int k;

	if (now >= nexthourlystats) {
		for (k = 59; k; k--) {
			hourly[k].tx = hourly[k - 1].tx;
			hourly[k].rx = hourly[k - 1].rx;
			hourly[k].start = hourly[k - 1].start;
		}
		hourly[0].start = nexthourlystats;
		nexthourlystats += ONE_HOUR;
		hoursonline++;
		hourly[0].rx = hourly[0].tx = 0;
	}
	if (now >= nextdailystats) {
		for (k = 59; k; k--) {
			daily[k].tx = daily[k - 1].tx;
			daily[k].rx = daily[k - 1].rx;
			daily[k].start = daily[k - 1].start;
		}
		daily[0].start = nextdailystats;
		nextdailystats += ONE_DAY;
		daysonline++;
		daily[0].rx = daily[0].tx = 0;
	}
}

/*---------------------------------------------------------------------------*/

void init_stats(void)
{
	struct tm *t;

	starttime = time((time_t *) 0);
	t = localtime(&starttime);
	t->tm_min = t->tm_sec = 0;
	hourly[0].start = mktime(t);
	nexthourlystats = hourly[0].start + ONE_HOUR;
	if (t->tm_hour)
		t->tm_hour = 0;
	daily[0].start = mktime(t);
	nextdailystats = daily[0].start + ONE_DAY;
	daily[0].rx = daily[0].tx = hourly[0].rx = hourly[0].tx = 0;
}

/*---------------------------------------------------------------------------*/

void mysighup(int sig)
{
	struct connection *p;

	/* bring down all the links, users and hosts */
	for (p = connections; p; p = p->next)
		bye_command2(p, "link shutdown");
	/* now actually do the disconnects */
	free_closed_connections();
	if (sig == SIGHUP)
		do_log(L_INFO, "SIGHUP received - restarting process");
	signal(SIGHUP, mysighup);
}

/* -----------------------------------------------------------------------
 *	Write PID to file (to aid external scripts kick our butt)
 */

void write_pid(void)
{
	int f;
	char s[10];

	sprintf(s, "%d\n", getpid());
	s[8] = '\0';

	if ((f = creat(pidfile, 0644)) < 0) {
		do_log(L_CRIT, "Could not open pid file %s for writing: %s", pidfile, strerror(errno));
		exit(1);
	}
	write(f, s, strlen(s));
	close(f);
}

/* ---------------------------------------------------------------------
 *	Drop to a non-priviledged UID if we happen to be root.
 */

void drop_privileges(void)
{
	uid_t olduid, uid;
	struct passwd *pwent;

	if ((olduid = geteuid()) == 0) {
		uid = atoi(OWNER);
		if (uid < 1) {
			if (!((pwent = getpwnam(OWNER))) || pwent->pw_uid == 0) {
				do_log(L_ERR, "\"%s\" is not a valid non-privileged UID or user name (being root is not preferred).", OWNER);
				return; 
			}
			uid = pwent->pw_uid;
		}
		if (seteuid(uid))
			do_log(L_ERR, "Could not set effective UID's from %d to %d: %s", olduid, uid, strerror(errno));
	}
}

/* ------------------------------------------------------------------------ */

int re_bind_listener(int i)
{
	int addrlen;
	struct sockaddr *addr;
	int arg ;

#ifdef	HAVE_AF_AX25
	if ((addr = build_sockaddr(listeners[i].name, &addrlen)) && (listeners[i].fd = socket(addr->sa_family, addr->sa_family == AF_AX25 ? SOCK_SEQPACKET : SOCK_STREAM, 0)) < 0) {
#else
	if ((addr = build_sockaddr(listeners[i].name, &addrlen)) && (listeners[i].fd = socket(addr->sa_family, SOCK_STREAM, 0)) < 0) {
#endif
		do_log(L_ERR, "%s for %s failed: %s", (addr) ? "listen" : "bind", listeners[i].name, strerror(errno));
		return -1;
	}
	switch (addr->sa_family) {
#ifdef	HAVE_AF_UNIX
        case AF_UNIX:
        	unlink(addr->sa_data);
		break;
#endif
#ifdef	HAVE_AF_INET
	case AF_INET:
		arg = 1;
		setsockopt(listeners[i].fd, SOL_SOCKET, SO_REUSEADDR, (char *)&arg, sizeof(arg));
		break;
#endif
        }
	/* bind socket to a name and listen for connects */
	if (bind(listeners[i].fd, addr, addrlen) || listen(listeners[i].fd, SOMAXCONN)) {
		do_log(L_ERR, "could not establish listener %s: %s", listeners[i].name , strerror(errno));
		close(listeners[i].fd);
		listeners[i].fd = -1;
		return -1;
	}

	do_log(L_INFO, "listening on %s", listeners[i].name);
#ifdef	HAVE_AF_UNIX
	if (addr->sa_family == AF_UNIX)
         	chmod(addr->sa_data, 0666);
#endif
	FD_SET(listeners[i].fd, &chkread);
	if (maxfd < listeners[i].fd)
		maxfd = listeners[i].fd;
	return listeners[i].fd;
}

/* ------------------------------------------------------------------------ */

void re_bind_listeners(void)
{
	static int last_run = 0L;
	uid_t prev_uid;
	int i;

	if (currtime - last_run < 60L)
		return;

	// we need root privilleges for binding to privilleged ports
	for (i = 0; i < 256 && listeners[i].name; i++) {
		if (!*listeners[i].name)
			continue;
		if (listeners[i].fd == -1 && *listeners[i].name) {
			// we're called the first time on startup. don't be too verbose there
			if (last_run != 0L)
				do_log(L_INFO, "listeners[%d]: %s is down. trying to bind() again", i, listeners[i].name);
			prev_uid = geteuid();
			seteuid(0);
			re_bind_listener(i);
			seteuid(prev_uid);
		}
	}
	if (last_run == 0) {
        	if (i < 256) {
			listeners[i].name = 0;
			listeners[i].fd = -1;
		}
	}

	last_run = currtime;
}

/* ------------------------------------------------------------------------ */

int fast_write(struct connection *cp, char *s, int force_comp)
{
	char buffer[MAX_MTU+1];
	char cbuf[260];
	int clen;
	int len;
	int slen;
	int size;
	char *spos;
	struct mbuf *bp;

	if (cp->type == CT_CLOSED || cp->fd == -1)
		return 0;

	cp->time_write = currtime;
	slen = strlen(s);

	if (force_comp >= 0 && (force_comp || (cp->compress & IS_COMP))) {
		spos = s;
		while (spos < s+slen) {
			strncpy(buffer, spos, COMP_MTU);
			buffer[COMP_MTU] = 0;
			len = strlen(buffer);
			if (encstathuf(buffer, len, cbuf, &clen)) {
//log(L_ERR, "debug: slen %d >%s<:%d -> >%s<:%d", slen, buffer, len, cbuf, clen);
			}
			size = write(cp->fd, cbuf, clen);
			if (size < clen) {
				// the comp protocol is frame orientated.
				// if only a half frame reached the channel,
				// it's an unrecoverable error
				goto linux_write_really_sucks;
			}
			/* if (size <= 0)
				goto failed; */
			cp->xmitted += len;
			cp->xmitted_comp += clen;
			spos += len;
		}
	} else {
		size = write(cp->fd, s, slen);
		if (size < slen)
			goto failed;
		if (size <= 0)
			goto failed;
		cp->xmitted += slen;
	}
	daily[0].tx += slen;
	hourly[0].tx += slen;
	if (!cp->obuf)
		FD_CLR(cp->fd, &chkwrite);

	return slen;

failed:
	// socked is blocked? EAGAIN is used more often used by linux 2.4.x 
	// (smaller buffers?)
        // why linux's write() sucks: it may write only n-m of n bytes.
	// and will not block now but the _next_ time with return -1 and
	// errno EAGAIN.
	// but this time, no errno is set (hugh!). that's why we had to check if
	// the returned size is exactly as expected.
	// strace said: 1. write(x, ".....", 2048) = 1890
	// strace said: 2. write(x, ".....", 2048) = -1 EAGAIN (Resource temporarily unavailable)
	if ((size == -1 && errno == EAGAIN) || (size >= 0 && size < slen)) {
		// kernel buffers are full (used often by linux 2.4.x)
		// re-add to queue
		if (size >= 0) {
			s += size;	// size bytes successfully written
			slen -= size;	// slen - size bytes remaining
		}
		if ((bp = (struct mbuf *) hmalloc(sizeof(struct mbuf) + slen + 1))) {
			bp->next = cp->obuf;
			bp->data = memcpy(bp + 1, s, slen + 1);
			if (!cp->obuf)
				FD_SET(cp->fd, &chkwrite);
			cp->obuf = bp;
			return 0;
		} /* else: fallthrough: no mem */
	}
linux_write_really_sucks:

	if (cp->type != CT_UNKNOWN)
		do_log(L_LINK, "Link failure with %s (write failed: %s)", cp->name, strerror(errno));
	if (cp->type == CT_HOST) {
       		sprintf(buffer, "%s<>%s broken (write)", myhostname, cp->name);
       		bye_command2(cp, buffer);
       	} else {
		bye_command2(cp, "link failure");
	}
	return 0;
}

/* ------------------------------------------------------------------------ */

int main(int argc, char **argv)
{
	fd_set actread;
	fd_set actwrite;
	char *sp, *sp2;
	char buffer[MAX_MTU+1];
	char cbuf[260];
	char timezone[64]; // needed, because buffer is to great for putenv()
	int i;
	int clen;
	int size;
	int status;
	struct connection *cp;
	struct mbuf *bp;
	struct timeval sel_timeout;
	struct stat statbuf;

	sp = strrchr(argv[0],'/');
	if (sp) {
		sp++;
	} else {
		sp = argv[0];
	}
	prgname = strdup(sp);

	/*
	   prgname = strdup(argv[0]);
	   tmp = rindex(prgname,47);
	   prgname = strdup(tmp++);
	 */

	time(&currtime);

	drop_privileges();
	parse_cmdl(argc, argv);

	if (demonize) {
		switch (fork()) {
			case 0:	/* child remains running */
				break;
			case -1:	/* error */
				fprintf(stderr, "conversd: fork failed: %s\n", strerror(errno));
				return 1;
				break;
			default:	/* parent - dies a quiet death */
				return 0;
		}
	}

	buildfnames(CONF_DIR,prgname);
	read_config();
	// close 0. depending on the configuration, open_log() may call
	// openlog(3) for syslog event logging (a unix socket connect()'ion
	// to unix:/dev/log, which then will choose this fd, because it's
	// the first one not in use). we need this, because we had strange
	// effects in some condxs: after open_log(), we closed this fd,
	// further down, but syslog(3) was not aware of the close.
	// some time later, it wrote to the fd (which was in use by a user
	// connection or unix/tcp lister, got a -1), and close()'ed this
	// filehandle. this may affected listeners or permlink connections,
	// and even could lead to a lockup of convers, because select()
	// checked this closed socket..
	close(0);
	open_log();

	if (tryonly) {
		do_log(LOG_INFO, "In try-only mode, exiting.");
		return 0;
	}

	time(&boottime);
	umask(002);		// bugfix: was 000 :(

	// fd's 0-2 are reserved (syslog, stderr logging, etc..)
	for (i = 3; i < FD_SETSIZE; i++)
		close(i);

	chdir(DATA_DIR);
	setsid();

	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, mysighup);
#ifdef	notdef
	if (!getenv("TZ"))
		putenv("TZ=localtime");
#else
	if (!mytimezone || !*mytimezone) {
		sp = getenv("TZ");
		mytimezone = hstrdup((sp) ? sp : "GMT");
	}
	sprintf(timezone, "TZ=%s", mytimezone);
        putenv(timezone);
#endif
	
	time(&currtime);
	
	connections = NULL;

	sp = ctime(&boottime);
	rip(sp);
	do_log(L_INFO, "%s started at %s", SOFT_RELEASE, sp);
	
	//strcpy(convtype, "conversd");
	strncpy(convtype, prgname, sizeof(convtype)-1);
	convtype[sizeof(convtype)-1] = 0;

	write_pid();
	init_stats();
	
	if (listen_port > 0) {
		sprintf(buffer, "*:%d", listen_port);
		listeners[1].name = hstrdup(buffer);
	} else {
		listeners[1].name = "";
	}

	charset_init();
	fix_user_logs();

	for (;;) {

		free_closed_connections();
		expire_dests();

		while (waitpid(-1, &status, WNOHANG) > 0);

		re_bind_listeners();
		connect_permlinks();

		actread = chkread;
		actwrite = chkwrite;
		//sel_timeout.tv_sec = 60;
		sel_timeout.tv_sec = 10;
		sel_timeout.tv_usec = 0;
		fflush(NULL);
		switch (select(maxfd + 1, &actread, &actwrite, (fd_set *) 0, &sel_timeout)) {
		case -1:
			// the problem has been resolved now. but it's not a bad idea to keep this check for a race condx:
			// hack: it seems to look like this:
			// syslog() writes on a connection fd (i.e. our unix socket listener! (a bug in conversd, or a libc6 problem??)).
			// after this, select() returns
			//   select(7, [0 3 4 6], [6], NULL, {10, 0}) = -1 EBADF (Bad file descriptor)
			//                  ^ was right before:
			//                  send(4, "<30>May 25 02:21:38 wconversd[12"..., 75, 0) = -1 EINVAL (Invalid argument)                                              
			//                  close(4)
			// now the listener is closed. but chkread is still set.
			// in such an error condx, select() _may_ return bad
			// values for actread and actwrite: man 2 select():
			//   the sets and timeout become undefined, so do not
			//   rely on their contents after an error.
			// also it was incorrect to FD_ZERO() actread and actwrite,
			// because even if the values may be not correct, after
			// FD_ZERO we will never read from our sockets or accept
			// new connections

			// let's find the bad filedescriptor
			do_log(L_CRIT, "select() returned -1: %s. maxfd+1 %s", strerror(errno), maxfd+1);
			for (i = 0; i < maxfd+1; i++) {
				// chkread or chkwrite matches filedescriptor?
				if (!FD_ISSET(i, &chkread) && !FD_ISSET(i, &chkwrite))
					continue;
				if (fstat(i, &statbuf)) {
					FD_CLR(i, &chkread);
					FD_CLR(i, &chkwrite);
					do_log(L_CRIT, "  may have found bad filedescriptor: %d (errno: %s)", i, strerror(errno));
				}
			}
			// now after correcting our filehandles, we could FD_ZERO,
			// and continue with at least the periodical jobs
			// -> fallthrough
		case 0:
			FD_ZERO(&actread);
			FD_ZERO(&actwrite);
			break;
		}
		if (maxfd < 0)
			sleep(1);

		time(&currtime);

		update_stats(currtime);
		expire_destroyed_channels();
		expire_channel_history();
		check_opless_channels();
		periodical_jobs();
		expire_banlist();
		expire_sul();

		for (i = 0; i < 256 && listeners[i].name; i++) {
			if (listeners[i].fd >= 0 && (FD_ISSET (listeners[i].fd, &actread)))
				accept_connect_request(i);
		}

		for (cp = connections; cp; cp = cp->next) {
			int last_was_cr;
			if (cp->fd < 0 || cp->via) {
				continue;
			}

			if ((cp->type == CT_USER && cp->idle_timeout && ((currtime - cp->time_processed) > cp->idle_timeout)) ||
					// incoming connection, with no state change to CT_USER or CT_HOST 15min after connect */
					(cp->type == CT_UNKNOWN && cp->session_type == SESSION_INBOUND && ((currtime - cp->time) > PERMLINK_MAXIDLE))) {
				if (!cp->ircmode)
					sprintf(buffer, "*** disconnecting: idle timeout%s", cp->ax25 ? "\r" : "\n");
				else
					sprintf(buffer, ":%s KILL %s :idle timeout\r\n", myhostname, cp->nickname);
				fast_write(cp, buffer, 0);
				bye_command2(cp, "idle timeout");
				// don't continue. do a read from the socket below and throw data away
			}
				

			if (FD_ISSET(cp->fd, &actread)) {

				if (cp->type == CT_CLOSED) {
					// be nice to the kernel and read the buffer
					read(cp->fd, buffer, sizeof(buffer)-1);
					// now throw away
					continue;
				}

				// cp->ibuf is MAX_MTU+1. it's a null-terminated string. read not more than MAX_MTU characters
				if ((cp->compress & IS_COMP)) {
					size = read(cp->fd, cbuf, COMP_MTU+1);
				} else {
					size = read(cp->fd, buffer, sizeof(buffer)-1);
				}

				if (size <= 0) {
					// socked is blocked (used often by linux 2.4.x)
					if (errno == EAGAIN)
						continue;
					// only log if not CT_UNKNOWN (for e.g., flexnet makes connects to potential link partners every few minutes
					if (cp->type != CT_UNKNOWN)
						do_log(L_LINK, "Link failure with %s (read failed: %s)", cp->name, strerror(errno));
					if (cp->type == CT_HOST) {
            					sprintf(buffer, "%s<>%s broken (read)", myhostname, cp->name);
            					bye_command2(cp, buffer);
          				} else {
						bye_command2(cp, "link failure");
					}
					continue;
				}
				// store when we read from socket
				cp->time_recv = currtime;
				if ((cp->compress & IS_COMP)) {
					clen = size;
					if (clen < 2 || !memcmp(cbuf, "*** ", 4) || decstathuf(cbuf, buffer, clen, &size)) {
						// compression error?
						memcpy(buffer, cbuf, clen);
						*(buffer+clen) = 0;
						size = clen;
//log(L_ERR, "is_comp bad: %d %d >%s<", clen, (unsigned char ) cbuf[0], buffer);
					} else {
						cp->received_comp += clen;
						*(buffer+size) = 0;
//log(L_ERR, "is_comp ok: %d/%d %d >%s<", clen, size, (unsigned char ) cbuf[0], buffer);
					}
				}
				cp->received += size;
				daily[0].rx += size;
				hourly[0].rx += size;
				if (cp->type == CT_USER) {
					// flood protection. reset timer
					if (cp->time_recv > cp->sul->stop_process_until)
							cp->sul->stop_process_until = cp->time_recv;

					/* server flood protection */
					if (size + cp->icnt + cp->ibufq_len > FLOODSIZE) {
						if (cp->type == CT_USER && (FeatureBAN & DO_BAN)) {
							ban_user(cp->name, myhostname, cp->sul, currtime,  BANTIME, "Local Flood");
						}
						if (!cp->ircmode)
							sprintf(buffer, "*** Closing Link: %s by %s (Excess Flood)%s", cp->name, myhostname, cp->ax25 ? "\r" : "\n");
						else
							sprintf(buffer, "ERROR :Closing Link: %s by %s (Excess Flood)\r\n", cp->nickname, myhostname);
						fast_write(cp, buffer, 0);
						bye_command2(cp, "Excess Flood");
						continue;
					}
				}
//appendstring(cp, "debug: read >");
//buffer[size] = 0;
//appendstring(cp, buffer);
//appendstring(cp, "<\n");
				if (cp->ibufq_len + size > MAX_IBUFQ_LEN) {
				   // CT_HOST and CT_UNKNOWN are always fully
				   // parsed; thus the queue will be empty.
				   // but a CT_USER with a local flood
				   // may overwrite his input buffer
				   size = MAX_IBUFQ_LEN - cp->ibufq_len;
				}
				// should always succeed
				memcpy(cp->ibufq + cp->ibufq_len, buffer, size);
				cp->ibufq_len += size;
			}

			// new data available?
//if (cp->ibufq_len) {
//sprintf(buffer, "debug: %ld ibufq_len %d icnt %d. will do: %d\n", currtime, cp->ibufq_len, cp->icnt, cp->stop_process_until - currtime);
//appendstring(cp, buffer);
//}
			last_was_cr = 0;
			for (sp = cp->ibufq; cp->ibufq_len; ) {

				// CT_USER: flood check
				// CT_CLOSED: connection closed (bye_command()) -> stop parsing input
				if ((cp->type == CT_USER && (FeatureFLOOD & NO_SERVER_FLOOD) && (cp->sul->stop_process_until > currtime + RateLimitMAX)) || cp->type == CT_CLOSED)
					break;

				cp->ibufq_len--;
				switch (*sp) {
					case '\n':
					case '\r':
						if (cp->icnt) {
							cp->ibuf[cp->icnt] = '\0';
//sprintf(buffer, "debug: %ld %d parsing >", currtime, cp->stop_process_until-currtime);
//appendstring(cp, buffer);
//appendstring(cp, cp->ibuf);
//appendstring(cp, "<\n");
							process_input(cp);
							cp->icnt = 0;
						} else {
							// user or conversd link holder triggered with enter
							if (!last_was_cr) {
								appendprompt(cp, 0);
								cp->time_processed = currtime;
							}
						}
						if (cp->ibufq_len) {
							// copy rest of bufq up to head
							sp++;
							for (i = 0; i < cp->ibufq_len; i++) {
								cp->ibufq[i] = sp[i];
							}
//appendstring(cp, "debug: copied restbuffer\n");
						}
						sp = cp->ibufq;
						last_was_cr = 1;
						break;
					case '\377':
						if (cp->icnt > 2 && cp->ibuf[cp->icnt -1] == '/') {	/* no <CR> or <LF> on prev line */
							cp->ibuf[cp->icnt - 1] = '\0';
							process_input(cp);
							cp->icnt = 1;
							cp->ibuf[0] = '/';
						}
						/* and fall through */
					default:
						if (cp->icnt < MAX_MTU - 5)
							cp->ibuf[cp->icnt++] = *sp++;
						if (last_was_cr)
							last_was_cr = 0;
				}
			}
		}
		send_sul_delayed_messages();

		for (cp = connections; cp; cp = cp->next) {
			if (cp->fd < 0 || cp->via || cp->type == CT_CLOSED) {
				continue;
			}
			if (FD_ISSET(cp->fd, &actwrite)) {
				if (cp->time_write == currtime && queuelength(cp->obuf) < cp->mtu) {
					// first packet in queue. wait at least 1s for more data

					// trick: go once again through select, but not 1000 times
					// until the second is over.
					cp->time_write -= 1;
					continue;
				}
				if (cp->mtu > sizeof(buffer)-1 || cp->mtu < 1 || ((cp->compress & IS_COMP) && cp->mtu > COMP_MTU)) {
#ifdef	HAVE_AF_AX25
					cp->mtu = ((cp->addr->sa_family == AF_AX25 || (cp->compress & IS_COMP)) ? COMP_MTU : sizeof(buffer)-1);
#else
					cp->mtu = ((cp->compress & IS_COMP) ? COMP_MTU : sizeof(buffer)-1);
#endif
				}

				// send buffer in multible packets when > MTU
				while (cp->obuf) {
					// collect data for a packet with whole MTU
					sp = buffer;
					for (bp = cp->obuf; bp; ) {
						for (sp2 = bp->data; *sp2; sp2++) {
							// skip trailing blanks, convert EOL on ax25 transport
							if (*sp2 == '\n') {
								while (sp > buffer && (*(sp-1) == ' ' || *(sp-1) == '\t'))
									sp--;
								if (cp->ircmode) {
									*sp++ = '\r';
									*sp++ = '\n';
								} else
									*sp++ = cp->ax25 ? '\r' : '\n';
							} else
								*sp++ = *sp2;
							if (sp - buffer >= cp->mtu - (cp->ircmode ? 1 : 0)) {
								// realloc buffer
								if (sp2[1]) {
									bp->data = sp2+1;
								} else {
									cp->obuf = bp->next;
									hfree(bp);
								}
								goto sendit;
							}
						}
						cp->obuf = bp->next;
						hfree(bp);
						bp = cp->obuf;
					}
sendit:
					*sp = 0;
					if (!*buffer)
						break;

					if (!fast_write(cp, buffer, 0))
						break;
				}
			}
		}
	}

	return 0;
}

