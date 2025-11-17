/*
 * This is Tampa Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@mdddhd.fc.hp.com>
 * Modifications by Fred Baumgarten <dc6iq@insu1.etec.uni-karlsruhe.de>
 *                  Matthias Welwarsky <dg2fef@uet.th-darmstadt.de>
 *
 * Modifications by Brian A. Lantz/KO4KS <brian@lantz.com>
 * Further mods by Thomas Osterried <dl9sau>, various bugs, raceconditions
 * and incompatibilities fixed; some extensions, some backpatches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "version.h"
#include "conversd.h"
#include "host.h"
#include "user.h"
#include "convert.h"
#include "log.h"
#include "config.h"
#include "hmalloc.h"
#include "ba_stub.h"
#include "access.h"
#include "tnos.h"
#include "noflood.h"

void h_away_command(struct connection *cp)
{
	char *fromname, *fromnickname, *hostname, *text, *char_p;
	time_t time = 0;
	struct connection *p;
	int text_changed = 0;

	fromname = getarg(0, 0);
	hostname = getargcs(0, 0);
	fromnickname = find_nickname(fromname, hostname);
	time = atol(getarg(0, 0));

	if (!user_check(cp, fromname))
		return;
	// AWAY user host [channel-dummy] time text
	if (time <= MAXCHANNEL) {
		if ((char_p = showargp())) {
			for (; *char_p && isspace(*char_p & 0xff); char_p++) ;
			if (isdigit(*char_p & 0xff))
				time = atol(getarg(0, 0));
		}
	}

	if (time == 0L || time > currtime)
		time = currtime;
	text = getarg(0, 1);
	cp->locked = 1;
	for (p = connections; p; p = p->next) {
		if (p->via && p->type == CT_USER) {
			if (!strcasecmp(p->name, fromname) && !strcasecmp(p->host, hostname)) {
				if (strcmp(p->away, text))
					text_changed = 1;
				strncpy(p->away, text, AWAYSIZE);
				p->away[sizeof(p->away)-1] = 0;
				p->atime = time;
				fromnickname = p->nickname;
			}
		}
	}

	if (check_cmd_flood(cp, fromname, hostname, SUL_AWAY, 0, text))
		return;
		
	send_awaymsg(fromname, fromnickname, hostname, time, text, text_changed);
}

/*---------------------------------------------------------------------------*/

void update_nickname_data(struct connection *cp, char *fromuser, char *fromnick)
{

	struct connection *p;
	//char buffer[2048];

	// cave: nick "deletion" works the same way: cp->nickname will contain
	// cp->name.

	// saupp handles nicknames correct. do not learn
	if ((cp->features & FEATURE_SAUPP_I))
		return;
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && p->via && p->via == cp &&
			!strcasecmp(p->name, fromuser) && strcasecmp(p->nickname, fromnick)) {
#ifdef	notdef
			// found user via this link. his nickname differs
			// it's bad, that we do not know his hostname;
			// h_uadd() will update data for all user@thathost connections
			sprintf(buffer, "/\377\200UADD %s %s %s", p->name, p->host, fromnick);
			getarg(buffer, 0);
			// avoid sending it back to our permlink neigbour
                	clear_locks();
			cp->locked = 1;
                	h_uadd_command(cp);
			
			// now continue, and look if user@anotherhost via this permlink is online
#else
			// avoid sending it back to our permlink neigbour
                	clear_locks();
			cp->locked = 1;
			change_pers_and_nick(cp, p->name, p->host, 0, -1, fromnick, 1, 1);
#endif
		}
	}

	// we need to clear our locks
	clear_locks();
	cp->locked = 1;
}

/*---------------------------------------------------------------------------*/

void h_cmsg_command(struct connection *cp)
{

	char *text;
	char *name;
	char *fromuser, *fromnick;
	int channel;

	name = getargcs(0, 0);
	channel = atoi(getarg(0, 0));
	text = getarg(0, 1);
	
	if (!user_check(cp, name))
		return;
	if (channel > MAXCHANNEL)
		return;
		
	fromuser = get_user_from_name(name);
	fromnick = get_nick_from_name(name);

	// restore leading blanks, kept by getarg(0, 1)
	while (*text && *(text -1) && isspace(*(text - 1) & 0xff))
		text--;

	if (check_msg_flood(cp, 0, channel, fromuser, fromnick, text))
		return;

	update_nickname_data(cp, fromuser, fromnick);

	if ((*text) && !((!strcmp(name, "conversd")) && strstr(text, "is calling CQ")))
		send_msg_to_channel2(name, fromuser, fromnick, channel, text, 0);
}

/*---------------------------------------------------------------------------*/

void h_ecmd_command(struct connection *cp)
{
	/* struct cmdtable *cmdp; */
	char *arg, *user, *params;
	struct connection *usercp;
	int arglen;
	/* struct extendedcmds *eadd; */
	//char buffer[64]; /* dl9sau: 64 is always too small! -> exploitable */
	char orig_arg[2048];
	char myarg[2048];
	char buffer[2048];

	arg = getarg(0, 1);
        strncpy(orig_arg, arg, sizeof(orig_arg)-1);
        orig_arg[sizeof(orig_arg)-1] = 0;
        strncpy(myarg, arg, sizeof(myarg)-1);
        myarg[sizeof(myarg)-1] = 0;

	user = getarg(myarg, 0);
	arglen = strlen(arg = getarg(0, 0));
	params = getarg(0, 1);

	if (!strcasecmp(user, "conversd")) {	/* adding a new command */
#ifdef	notdef
		/* dl9sau: we do not support this. at least currently, until we
		 *         understand what it is for.
		 *         for e.g., we never learn ecmd's from the remote site
		 *         during h_host. we have big problems when executing this
		 *         (see below). the cmd-list could get large (-> exploitable
		 *         DOS attack). etc..
		 */
		if (!ecmd_exists(arg)) {
			eadd = hmalloc(sizeof(struct extendedcmds));
			eadd->next = ecmds;
			eadd->name = hmalloc(strlen(arg) + 1);
			strcpy(eadd->name, arg);
			ecmds = eadd;
		}
#endif
		sprintf(cp->ibuf, "/\377\200ECMD %s\n", orig_arg);
		h_unknown_command(cp);
		return;
	}
	/* find the actual user who sent the command */
	for (usercp = connections; usercp; usercp = usercp->next) {
		/* dl9sau: why should we execute this command for a non-local user?
		 * where's the sense of this command? flooding the net?
		 */
		if (!strcasecmp(usercp->name, user))
			break;
	}
	if (usercp == NULLCONNECTION)
		return;

	/* now lookup the command locally */
#ifdef	notdef
	/* dl9sau: eh? why cmdtable, not ecmdtable? 
	 *         what's with security, if user is conversd operator?
	 *         usercp->ibuf is null if user is non-local. this could not work.
	 *         if user is local, the local user and not the remote user get's
	 *         the answer.
	 *         -> no no no!
	 */
	for (cmdp = cmdtable; cmdp->name; cmdp++)
		if (!strncmp(cmdp->name, arg, arglen)) {
			if (cmdp->states & (1 << usercp->type)) {
				sprintf(usercp->ibuf, "%s %s\n", arg, params);
				(void) getarg(usercp->ibuf, 0);
				(*cmdp->fnc) (usercp);
			}
			return;
		}
	
#else
	/* dl9sau: after any personal message, a link to a partner may be locked;
	 *         -> don't forget to release locks!
	 */
	sprintf(buffer, "*** ECMD feature prohibited at %s. This incident will be logged for further investigation.\n", myhostname);
	send_msg_to_user("conversd", user, buffer);
	do_log(L_INFO, "ECMD blocked (link %s): >%s %s<", cp->name, orig_arg);
	clear_locks();
	cp->locked = 1;
	/* dl9sau: at least be fair and let the others handle this trashy command */
#endif
	/* if not found locally, then send it on to others */
	sprintf(cp->ibuf, "/\377\200ECMD %s\n", orig_arg);
	h_unknown_command(cp);
}

/*---------------------------------------------------------------------------*/

void h_help_command(struct connection *cp)
{
	char *user, *arg;
	char buffer[2048];

	user = getarg(0, 0);
	arg = getarg(0, 0);

	/* now lookup the help locally */
	if (!dohelper(0, user, arg)) {
		/* if not found locally, then send it on to others */
		sprintf(cp->ibuf, "/\377\200HELP %s %s\n", user, arg);
		h_unknown_command(cp);
	} else {
		sprintf(buffer, "*** Extended Help Reply from host: %s\n", myhostname);
		send_msg_to_user("conversd", user, buffer);
		clear_locks();
		send_msg_to_user("conversd", user, "***");
	}
}

/*---------------------------------------------------------------------------*/

void h_dest_command(struct connection *cp)
{
	char *name, *rev, *hisneigh;
	int pl, rtt, hops;
	struct permlink *l = NULLPERMLINK;
	struct destination *dest;

	name = getargcs(0, 0);
	rtt = atoi(getarg(0, 0));
	rev = getarg(0, 0);
	if (!*rev)
		rev = "?";
	hops = atoi(getarg(0, 0));
	hisneigh = getargcs(0, 0);

	if (!permarray)
		return;
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		l = permarray[pl];
		if (l && l->connection && (l->connection == cp))
			break;
	}
	if (!l)
		return;
	if (!strcasecmp(name, myhostname)) {
		loop_handler(cp, l, CT_HOST, "", name, myhostname);
		return;
	}
	for (dest = destinations; dest; dest = dest->next) {
		if (!strcasecmp(dest->name, name))
			break;
	}
	// looop protection. already learned?
	if ((dest && dest->link && dest->link->connection != cp)) {
		loop_handler(cp, l, CT_HOST, "", name, dest->link->connection->name);
		return;
	}
	/* bugfix: dest_check returns 0 if link exists in database, but
	 * is dest->link is NULLPERMLINK. this is, because a link was
	 * broken. when this function is called, the link is up again
	 */
	if (rtt < 1) {
		*hisneigh = 0;
		hops = 0;
	} else {
		if (!*hisneigh) {
			hisneigh = (dest && dest->link && dest->rtt) ? dest->hisneigh : cp->name;
			if (hops < 1)
				hops = (dest && dest->hops > 1) ? (dest->hops -1) : 1;
		} else {
			// already learned more concrete map?
			if (dest && dest->link && dest->rtt) {
				// it's an active destinaton
				if (hops < 1 /* || dest->hops != hops+1 does not work, off by one */) {
					hops = dest->hops -1;
					hisneigh = dest->hisneigh;
				}
			}
		}
	}
				
	if (dest_check(cp, dest)) {
		update_destinations(l, name, rtt, rev, hops, hisneigh);
	}
}

/*---------------------------------------------------------------------------*/

#ifdef WANT_FILTER
void h_filt_command(struct connection *cp)
{
	char *name;
	char *host;
	char *time;
	char *filter;
	time_t filter_time;
	struct connection *p;
	int len;

	time = getarg(0,0);
	name = getarg(0,0);
	host = getargcs(0,0);
	filter = getarg(0,1);

	if ((len = strlen(filter)) > 512-2) {
		filter[len] = 0;
	}
	strcat(filter, " ");

	filter_time = (time_t)atol(time);
	if (filter_time > currtime)
		filter_time = currtime;

	cp->locked = 1;
	send_filter_msg(name, host, filter_time, filter);

	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && p->via) {
			if (!strcasecmp(p->host, host)
					&& !strcasecmp(p->name, name)) {

				if (p->filter) {
					if (filter_time > p->filter_time) {
						hfree(p->filter);
						p->filter = NULL;
						p->filter_time = filter_time;

						if (*filter != '@') {
							if ((p->filter = (char *)hmalloc(strlen(filter)+1)) != NULL) {
								strcpy(p->filter, filter);
							}
						}
					}
				} else {
					if (filter_time > p->filter_time) {
						p->filter_time = filter_time;

						if (*filter != '@') {
							if ((p->filter = (char *)hmalloc(strlen(filter)+1)) != NULL) {
								strcpy(p->filter, filter);
							}
						}
					}
				}
			}
		}
	}
}
#endif

/*---------------------------------------------------------------------------*/

int check_permlink_by_permlink_entry(struct connection *cp, char *name)
{

	char buffer[2048];
	struct permlink *pp = 0;
	struct sockaddr *addr;
	struct sockaddr_in *sin, *sin_act;
	int pl;
	int addrlen;
	int group;
	char *q;

	// hack for dyndns hosts: if ACCESS fails because we could not allow
	// connections from a whole provider, this function looks if the
	// permlink has been configured, and which host will be connected.
	// we could not resolve IN PTR (because it's the name of the provider
	// network), but we could resolve the IN A of the dyndns addr.
	// if this temporary address matches the address of the host who has
	// connected us, then he's probaly permitted to connect.

	if (!permarray)
		return 0;	// no permlinks at all

	if (cp->addr->sa_family != AF_INET)
		return 0;	// currently, we only support AF_INET sockets

	if (!(sin = (struct sockaddr_in *) cp->addr))
		return 0;

	// find entry
	if (!permarray)
		return 0;
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if ((pp = permarray[pl]) && !strcasecmp(pp->name, name))
			break;
	}
	if (pl == NR_PERMLINKS || !pp || pp->permanent != 2)
		return 0;	// not found

	for (group = 0; group < pp->groups; group++) {
		strncpy(buffer, pp->s_groups[group].socket, sizeof(buffer)-1);
		buffer[sizeof(buffer)-1] = 0;
		if (!*buffer)
			return 0;
		// ax25:db0foo-1 or unix:/foo/bar or host[:port]
		if ((q = strchr(buffer, ':'))) {
			q++;
			if (!strncasecmp(buffer, "unix:", 5) || !strncasecmp(buffer, "local:", 6) || !strncasecmp(buffer, "ax25:", 5))
				continue;
		}
		if (!q || !atoi(q)) {
        		char tmpbuf[128];
                	sprintf(tmpbuf, "%.64s:%d", buffer, listen_port);
                	if (!(addr = build_sockaddr(tmpbuf, &addrlen)))
			continue;
        	} else {
        		if (!(addr = build_sockaddr(buffer, &addrlen)))
                		continue;
		}
		if (!(sin_act = (struct sockaddr_in *) addr))
				continue;
		if (sin->sin_addr.s_addr == sin_act->sin_addr.s_addr)
			return 1;	// puh. found.
	}
	return 0;
}

/*---------------------------------------------------------------------------*/

void h_cdat_command(struct connection *cp)
{
	struct channel *ch;
	char *arg;
	time_t time;
	int channel;
	int i;

	arg = getarg(0, 0);
	if (!*arg)
		return;
	channel = atoi(arg);

	for (ch = channels; ch; ch = ch->next)
		if (ch->chan == channel)
			break;
	if (!ch)
		return;
	for (i = 0; ; i++) {
		arg = getarg(0, 0);
		if (!*arg || (time = atol(getarg(0, 0))) == 0L)
			return;

		switch(i) {
		case 0:
			// first touple: creator and creation time
			if (ch->ctime > time || !*ch->createby) {
				strncpy(ch->createby, arg, NAMESIZE);
				ch->createby[sizeof(ch->createby)-1] = 0;
				ch->ctime = time;
			}
			break;
		case 1:
			// second touple: last message and time
			if (ch->ltime < time || ch->ltime == 0L || !*ch->lastby) {
				strncpy(ch->lastby, arg, NAMESIZE);
				ch->lastby[sizeof(ch->lastby)-1] = 0;
				ch->ltime = time;
			}
			break;
		}
	}
}

/*---------------------------------------------------------------------------*/

struct channel *find_active_channel(int channel)
{
	struct channel *ch;

	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == channel)
			break;
	}
	if (!ch || ch->expires != 0L)
		return 0;
	return ch;
}

/*---------------------------------------------------------------------------*/

char *chan_plus_mode(int channel, int chanop, char *is_creator)
{
	static char chinfo[64];
	struct channel * ch;

	sprintf(chinfo, "%d", channel);
	if (!(ch = find_active_channel(channel)))
		return chinfo;
	if (chanop) {
		sprintf(chinfo + strlen(chinfo), ":@%s", (!strcasecmp(is_creator, ch->createby) ? "@" : ""));
	}
	if (!ch->locked && ch->flags)
		sprintf(chinfo + strlen(chinfo), "%s%s", (chanop ? "" : ":"), get_mode_flags(ch->flags));
	ch->locked = 1;
	return chinfo;
}

/*---------------------------------------------------------------------------*/

void announce_channel_mode(struct connection *cp, int channel)
{
	char buffer[2048];
	struct channel *ch;
	char *flags;

	if (!(ch = find_active_channel(channel)))
		return;
	if (ch->locked)
		return;
	flags = get_mode_flags(ch->flags);
	if (!*flags)
		return;
	sprintf(buffer, "/\377\200MODE %d +%s\n", ch->chan, get_mode_flags(ch->flags));
	linklog(cp, L_SENT, "%s", &buffer[3]);
	appendstring(cp, buffer);
	ch->locked = 1;
}

/*---------------------------------------------------------------------------*/

void clear_chan_locks(void)
{
	struct channel *ch;
	for (ch = channels; ch; ch = ch->next)
		ch->locked = 0;
}

/*---------------------------------------------------------------------------*/

void h_host_command(struct connection *cp)
{
	char buffer[2048];
	int pl;
	int ret;
	int hops, hops_next;
	int state;
	int i;
	struct connection *p, *p_next;
	struct destination *d;
	struct permlink *pp;
	struct permlink *me_in_permlist;
	struct channel *ch;
	struct clist *cl;
	char *name;
	char *rev;
	char *features;

	name = getargcs(0, 0);
	rev = getarg(0, 0);
	if (!*rev)
		rev = "?";
	features = getargcs(0, 0);

	if (!*name)
		return;
	
	// protocol specific logging on refuse
	*buffer = 0;
#ifdef	HAVE_AF_INET
	if (cp->addr->sa_family == AF_INET) {
		struct sockaddr_in * sin = (struct sockaddr_in *) cp->addr;
		sprintf(buffer, "%s", inet_ntoa(sin->sin_addr));
		if (!cp->hostallowed) {
			if ((cp->hostallowed = check_permlink_by_permlink_entry(cp, name)))
				do_log(L_AUTH, "Acceptet host %s by his known permlink address\n", name);
		}
	}
#endif
#ifdef	HAVE_AF_UNIX
	if (cp->addr->sa_family == AF_UNIX) {
		sprintf(buffer, "%s", cp->sockname);
	}
#endif
#ifdef	HAVE_AF_AX25
	if (cp->addr->sa_family == AF_AX25) {
		struct sockaddr_ax25 *sax = (struct sockaddr_ax25 *) cp->addr;
		sprintf(buffer, "%s", ax25_ntoa(&sax->sax25_call));
	}
#endif
	if (!*buffer)
		sprintf(buffer, "proto %d, socket %s", cp->addr->sa_family, cp->sockname);

	if (!cp->hostallowed) {
		do_log(L_AUTH, "Refused HOST connection from %s [%s] to %s", name, buffer, cp->sockname);
		return;
	}
	if (!strcasecmp(name, myhostname)) {
		do_log(L_AUTH, "Refused HOST connection %s [%s]: claimed to be our host(!). unix echo?", name, buffer);
		appendstring(cp, "you are not me, are you?\n");
		return;
	}
	if (cp->type != CT_UNKNOWN)
		return;

	// authentication. we need his name at this point. and further down for the
	// loop handler
	strncpy(cp->name, name, NAMESIZE);
	cp->name[sizeof(cp->name)-1] = 0;
	cp->name_len = strlen(cp->name);
	if (!cp->isauth) {
		if ((ret = check_password(cp)) < 0) {
			do_log(L_AUTH, "Refused HOST connection from %s [%s] to %s: Wrong password. ret = %d. Got: >%s<", name, buffer, cp->sockname, ret, cp->pass_got);
			appendstring(cp, "Sorry, password incorrect.\n");
			return;
		}
		cp->isauth = (ret) ? 1 : 2;
	}	

	if (!permarray)
		goto behind_permarray_null_problem;

	do_log(L_DEBUG, "checking new host %s", name);
	me_in_permlist = 0;

	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if (!(pp = permarray[pl]))
			continue;
		if (pp->connection != NULLCONNECTION && pp->connection != cp && pp->connection->type == CT_HOST && !strcasecmp(pp->name, name)) {
			do_log(L_LINK, "h_host - disconnecting old connection to %s: reconnect", pp->name);
			sprintf(buffer, "/\377\200SQUIT %s %s<>%s link reorg (reconnect)%s", myhostname, myhostname, name, cp->ax25 ? "\r" : "\n");
			fast_write(pp->connection, buffer, 0);
			sprintf(buffer, "*** link reorg: new connection from %s%s", name, cp->ax25 ? "\r" : "\n");
			fast_write(pp->connection, buffer, 0);
			sprintf(buffer, "%s<>%s link reorg (reconnect)", myhostname, name);
			bye_command2(pp->connection, buffer);
			continue;
		}
		if (pp->connection == cp) {
			if (strcasecmp(pp->name, name)) {
				do_log(L_ERR, "h_host: configuration error: connected to %s but host claimed to be %s. correcting permlist.", pp->name, name);
				strncpy(pp->name, name, HOSTNAMESIZE);
				pp->name[HOSTNAMESIZE] = 0;
			}
		}
		if (!strcasecmp(pp->name, name)) {
			if (!pp->connection || pp->connection != cp)
				pp->curr_group = -1;
			me_in_permlist = pp;
			do_log(L_DEBUG, "h_host - found %s (%s) in permlist. %d primary, %d backup link", pp->name, name, ((pp->primary) ? 1 : 0), ((pp->backup) ? 1: 0));
		}
	}

	/* dl9sau:
	 * new: - loop avoidance instead of just loop detection in the handling
	 *        of connecting primary / secondary links
	 *      - recognize if the conversd name of a permlink has changed
	 */
	if (me_in_permlist) {
		if (me_in_permlist->locked) {
			char tmp[64];
			ts3(me_in_permlist->locked_until - currtime, tmp);
			/* check if link is locked. This is part of the loop detector */
			do_log(L_LINK, "h_host - disconnecting %s: host is locked until %s", name, tmp);
			sprintf(buffer, "*** sorry, loop prevention. try again in %s.%s", tmp, cp->ax25 ? "\r" : "\n");
			fast_write(cp, buffer, 0);
			// rip \n
			buffer[strlen(buffer)-1] = 0;
			bye_command2(cp, buffer);
			return;
		}

		// precaution: our prioritive connections are in a flat list
		// check for ascending connections
		for (pp = me_in_permlist->primary; pp; pp = pp->primary) {
			do_log(L_DEBUG, "checked up %s", pp->name);
			if (pp->connection && pp->connection->type == CT_HOST) {
				if (pp->txtime || currtime - pp->connection->time < 120) {
					// our primary is already connected and functional, or in progress to be..
					do_log(L_LINK, "h_host - disconnecting %s: host with higher priority (%s) already connected", name, pp->name);
					sprintf(buffer, "*** link reorg: prio %s > %s (old>new); disc new%s", pp->name, name, cp->ax25 ? "\r" : "\n");
					fast_write(cp, buffer, 0);
					// rip \n
					buffer[strlen(buffer)-1] = 0;
					bye_command2(cp, buffer);
					return;
				} else {
					do_log(L_LINK, "h_host - disconnecting %s: host with lower priority (%s) has connected, but old link does not seem to be functional: txtime %d, idle %ld", pp->name, name, pp->txtime, (currtime - pp->connection->time));
					// don't waste bandwith
					sprintf(buffer, "/\377\200SQUIT %s %s<>%s link reorg%s", myhostname, myhostname, pp->name, cp->ax25 ? "\r" : "\n");
					fast_write(pp->connection, buffer, 0);
					sprintf(buffer, "%s<>%s: link reorg", myhostname, cp->name);
					bye_command2(pp->connection, buffer);
					// testing:
					// loop protection - connect again, lock old host
					pp->locked = 1;
					pp->locked_until = currtime + 1800;
					sprintf(buffer, "*** disconnected old fallback link. please connect again, %s", cp->ax25 ? "\r" : "\n");
					fast_write(cp, buffer, 0);
					// rip \n
					buffer[strlen(buffer)-1] = 0;
					bye_command2(cp, buffer);
					me_in_permlist->retrytime = currtime + 5*60;
					me_in_permlist->tries = 0;
					return;
				}
			}
		}
		// check for descending connections
		for (pp = me_in_permlist->backup; pp; pp = pp->backup) {
			do_log(L_DEBUG, "checked down %s", pp->name);
			if (pp->connection && pp->connection->type == CT_HOST) {
				// disconnect - txtime is irrelevat: high prio for high prio servers
				do_log(L_LINK, "h_host - disconnecting %s: host with higher priority (%s) has connected", pp->name, name);
				sprintf(buffer, "/\377\200SQUIT %s %s<>%s link reorg: prio < %s%s", myhostname, myhostname, pp->name, name, cp->ax25 ? "\r" : "\n");
				fast_write(pp->connection, buffer, 0);
				sprintf(buffer, "%s<>%s link reorg: prio < %s", myhostname, pp->name, name);
				bye_command2(pp->connection, buffer);
				// we could break, now - but just 2b sure..
				// testing:
				// loop protection - connect again, lock old host
				pp->locked = 1;
				pp->locked_until = currtime + 1800;
				sprintf(buffer, "*** disconnected old permlink. please connect again%s", cp->ax25 ? "\r" : "\n");
				fast_write(cp, buffer, 0);
				// rip \n
				buffer[strlen(buffer)-1] = 0;
				bye_command2(cp, buffer);
				me_in_permlist->retrytime = currtime + 5*60;
				me_in_permlist->tries = 0;
				return;
			} else {
				do_log(L_DEBUG, "h_host - did not found active connection for %s in permlist", name);
			}
		}
	}

	// is this host already marked found an active destination? (me_in_perlist may be nullpointer)
	if (((d = find_destination(name)) && d->link && d->link->connection)) {
		// fake /..HOST. this is, that the remote site knows that the connection
		// to our host may established. we may be his primary, and then he
		// could disconnect his old fallback link. the connecting host may
		// be not configured as primary or fallback on our site, so the code
		// above did not apply.
		sprintf(buffer, "/\377\200HOST %s %s %s%s", myhostname, SOFT_REL8, myfeatures, cp->ax25 ? "\r" : "\n");
		fast_write(cp, buffer, 0);
		do_log(L_LINK, "h_host - disconnecting connecting host %s: DEST already via %s", name, d->link->connection->name);
		// thus don't send /..LOOP, because it would lock him.
		//loop_handler(cp, me_in_permlist, CT_HOST, "", name, d->link->connection->name);
	        sprintf(buffer, "/\377\200INFO %s detected loop with connecting host %s: HOST %s. DEST already via %s%s", myhostname, name, name, d->link->name, cp->ax25 ? "\r" : "\n");
		fast_write(cp, buffer, 0);
		rip(buffer);
		do_log(L_LINK, buffer + 3);
		getarg(buffer, 0);
		h_info_command(cp);
		sprintf(buffer, "*** disconnecting: your host is already known via %s. please disconnect your fallback link, wait a while and try again.%s", d->link->connection->name, cp->ax25 ? "\r" : "\n");
		fast_write(cp, buffer, 0);
		bye_command2(cp, buffer);
		if (me_in_permlist) {
			me_in_permlist->locked = 1;
			me_in_permlist->locked_until = currtime + 1800;
		}
		return;
	}

	do_log(L_LINK, "accepted new host %s", name);
#ifdef	notdef
	/* dl9sau: do we really need this? - inform link parners about the
	 * change - * it's already done by the bye_command2(), and
	 * update_destinations() will inform every permlink anyway..
	 * */
	if (permarray) {
 		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			if ((pp = permarray[pl]) && pp->connection && strcasecmp(pp->name, name))
				update_destinations(pp, name, 0, rev, name, 0, myhostname);
		}
	}
#endif

behind_permarray_null_problem:

	cp->type = CT_HOST;

	strncpy(cp->rev, rev, NAMESIZE);
	cp->rev[sizeof(cp->rev)-1] = 0;

	generate_sul(cp, 0, cp->sul);

	route_cleanup(cp);
	//cp->charset_in = cp->charset_out = dumb; -- no, if, then ISO
	cp->charset_in = cp->charset_out = ISO; // but it's ignored on inter-permlink traffic anyway..
	if (strchr(features, 'a'))
		cp->features |= FEATURE_AWAY;
	if (strchr(features, 'A'))
		cp->features |= FEATURE_AWAY;
	if (strchr(features, 'd'))
		cp->features |= FEATURE_FWD;
	if (strchr(features, 'm'))
		cp->features |= FEATURE_MODES;
	if (strchr(features, 'p'))
		cp->features |= FEATURE_LINK;
	if (strchr(features, 'u'))
		cp->features |= FEATURE_UDAT;
	if (strchr(features, 'n'))
		cp->features |= FEATURE_NICK;
	if (strchr(features,'f'))
		cp->features |= FEATURE_FILTER;
	if (strchr(features,'i'))
		cp->features |= FEATURE_SAUPP_I;
/* #define DEBUG */
#ifdef DEBUG
	printf("Features '%s': %s%s%s%s%s%s\n", name,
	       (cp->features & FEATURE_AWAY) ? "a" : "",
	       (cp->features & FEATURE_FWD) ? "d" : "",
	       (cp->features & FEATURE_MODES) ? "m" : "",
	       (cp->features & FEATURE_LINK) ? "p" : "",
	       (cp->features & FEATURE_UDAT) ? "u" : "",
	       (cp->features & FEATURE_NICK) ? "n" : "",
	       (cp->features & FEATURE_FILTER) ? "f" : "");
#endif

	cp->amprnet = (strchr(features, 'u') ? 1 : 0);
	if (cp->features & FEATURE_AWAY)
		cp->oldaway = (strchr(features, 'A') ? 0 : 1);

	if ((me_in_permlist = update_permlinks(name, cp, 0)) == NULLPERMLINK) {
		sprintf(buffer, "*** Link table full!%s", cp->ax25 ? "\r" : "\n");
		do_log(L_LINK, "link table full: %s", name);
		fast_write(cp, buffer, 0);
		// rip \n
		buffer[strlen(buffer)-1] = 0;
		bye_command2(cp, buffer+4);
		return;
	}

	sprintf(buffer, "/\377\200HOST %s %s %s\n", myhostname, SOFT_REL8, myfeatures);
	linklog(cp, L_SENT, "%s", &buffer[3]);
	appendstring(cp, buffer);

	// first, print destinations. announce in priority of hopcount
	// doing it here, will help to prevent loop before user's have been announced
	for (hops = hops_next = 0; ; hops = hops_next) {
		for (d = destinations; d; d = d->next) {
			if (d->hops != hops) {
				if (d->hops > hops && (d->hops < hops_next || hops >= hops_next)) {
					hops_next = d->hops;
					}
				continue;
		}
			if (d->rtt && d->link && d->link->connection != cp && !d->auto_learned) {
				sprintf(buffer, "/\377\200DEST %s %ld %s %d %s\n", d->name, d->rtt + 99/99, (*d->rev ? d->rev : "-"), d->hops, d->hisneigh);
				do_log(L_DEBUG, "sent to %s: %s", name, &buffer[3]);
				linklog(cp, L_SENT, "%s", &buffer[3]);
				appendstring(cp, buffer);
			}
		}
		if (hops == hops_next) {
			break;
		}
	}

  for (state = 0; state < 3; state++) {
	int announce_user_host = 1;
	int announce_user = 1;
	// announce users
    	if (state == 2) {
		for (ch = channels; ch; ch = ch->next) {
			if (ch->expires != 0L)
				continue;
			if (*ch->topic) {
				sprintf(buffer, "/\377\200TOPI %s %s %ld %d %s\n", (*ch->tsetby ? ch->tsetby : "conversd"), myhostname, (long)ch->time, ch->chan, ch->topic);
				linklog(cp, L_SENT, "%s", &buffer[3]);
				appendstring(cp, buffer);
			}
#ifdef	notdef	/* nice try - but i'll do it in another way */
			if (cp->features & FEATURE_SAUPP_I) {
				sprintf(buffer, "/\377\200CDAT %d %s %ld", ch->chan, ch->createby, ch->ctime);
				if (*ch->lastby)
					sprintf(buffer + strlen(buffer), " %s %ld", get_user_from_name(ch->lastby), ch->ltime);
				strcat(buffer, "\n");
				appendstring(cp, buffer);
			}
#endif
		}
		continue;

	}
	for (p = sort_connections(1), i = 0 ; p; p = p_next, i++) {
		p_next = sort_connections(0);
		if (p->type == CT_USER) {
			int next_is_same_user_host = 0;
			int next_is_same_user = 0;
			if (!*p->host) {
				do_log(L_ERR, "error: h_host_command(): empty host for %s - should never happen!", p->name);
				continue;
			}
			if (p_next && p_next->type == CT_USER && !strcasecmp(p->name, p_next->name)) {
				next_is_same_user = 1;
				if (!strcasecmp(p->host, p_next->host))
					next_is_same_user_host = 1;
			}
			if (!state) {
				if (cp->features & FEATURE_SAUPP_I) {
					int has_nick = (announce_user_host && !p->observer && strcmp(p->name, p->nickname)) ? 1 : 0;
					int chanop, channel;
					time_t time;
					// PROVE OF CONCEPT. WILL CHANGE!!!
					// NetJoin:
					// example: NJ user host [o|r|a]<+|-|=>[nick] [[channel[:[mode][@[@]]]] jointime [...]] [:pers]"
					// o = op, a away (text follows later), r restricted (obs)
					// +/-/=nick: if any, and not announced earlier. on a position that cannot be spoofed.
					//   + is set, - is "reset" (or "not set") to p->name, = is "leave untouched"
					// channel
					// mode: channel modes (if any)
					// jointime: if lowest, channel creation time.
					// @: 1. operator 2. creator
					// pers: if any, and not announced earlier. if "empty", mark with ":". if announced, leave away.
					if (announce_user_host) {
						// print header
						sprintf(buffer, "/\377\200NJ %s %s %s%s%s%s%s", p->name, p->host, (!p->observer && (announce_user && p->operator)) ? "o" : "", (p->observer ? "r" : ""), (!p->observer && announce_user_host && *p->away) ? "a" : "", (announce_user_host ? (has_nick ? "+" : "-") : "="), has_nick ? p->nickname : "");
						appendstring(cp, buffer);
					}
					if (!p->chan_list && p->channel >= 0) {
						chanop = p->channelop;
						channel = p->channel;
						time = p->time;
						cl = 0;
						goto nerd;
					}
					for (cl = p->chan_list; cl; cl = cl->next) {
						chanop = cl->channelop;
						channel = cl->channel;
						time = cl->time;
nerd:
						sprintf(buffer, " %s %ld", chan_plus_mode(channel, chanop, p->name), time);
						appendstring(cp, buffer);
						if (!cl)
							break;
					}
					if (!next_is_same_user_host || !p_next || (next_is_same_user_host && p_next && ((p->observer && !p_next->observer) || (!p->observer && p_next->observer))) || (i >= MAX_CHAN_PER_USER_AND_HOST) /* avoid buffer overflows on rx site */) {
						if (!p->observer) {
							sprintf(buffer, " :%s", !strcmp(p->pers, "@") ?  "" : p->pers);
							appendstring(cp, buffer);
						}
						appendstring(cp, "\n");

						i = 0;
						if (next_is_same_user_host) {
							announce_user_host = announce_user = 1;
							continue;
						}
					}
					goto end_p;
				}
				if (!p->via) {
					for (cl = p->chan_list; cl; cl = cl->next) {
						sprintf(buffer, "/\377\200%s %s %s %ld -1 %d%s%s\n", (p->observer) ? "OBSV" : "USER", p->name, p->host, (long)cl->time, cl->channel, (cp->amprnet ? " " : ""), (cp->amprnet ? p->pers : ""));
						//log(L_DEBUG, "sent to %s: %s", name, &buffer[3]);
						linklog(cp, L_SENT, "%s", &buffer[3]);
						appendstring(cp, buffer);
						announce_channel_mode(cp, cl->channel);
						if (cl->channelop) {
							sprintf(buffer, "/\377\200OPER conversd %d %s\n", cl->channel, p->name);
							linklog(cp, L_SENT, "%s", &buffer[3]);
							appendstring(cp, buffer);
						}
					}
				} else {
					sprintf(buffer, "/\377\200%s %s %s %ld -1 %d%s%s\n", (p->observer) ? "OBSV" : "USER", p->name, p->host, (long)p->time, p->channel, (cp->amprnet ? " " : ""), (cp->amprnet ? p->pers : ""));
					//log(L_DEBUG, "sent to %s: %s", name, &buffer[3]);
					linklog(cp, L_SENT, "%s", &buffer[3]);
					appendstring(cp, buffer);
					announce_channel_mode(cp, p->channel);
					if (p->channelop) {
						sprintf(buffer, "/\377\200OPER conversd %d %s\n", p->channel, p->name);
						linklog(cp, L_SENT, "%s", &buffer[3]);
						appendstring(cp, buffer);
					}
				}
				if (!next_is_same_user && p->operator) {
					sprintf(buffer, "/\377\200OPER conversd -1 %s\n", p->name);
					linklog(cp, L_SENT, "%s", &buffer[3]);
					appendstring(cp, buffer);
				}
				goto end_p;
			}
			if (next_is_same_user_host)
				goto end_p;
			if (cp->features & FEATURE_SAUPP_I)
				goto skiped_umsgs;
			if (cp->features & FEATURE_NICK) {
				if (*p->nickname && strcasecmp(p->name, p->nickname)) {
					sprintf(buffer, "/\377\200UADD %s %s %s", p->name, p->host, p->nickname);
					if (strcmp(p->pers, "@"))
						sprintf(buffer + strlen(buffer), " -1 %s", p->pers);
					linklog(cp, L_SENT, "%s", &buffer[3]);
					appendstring(cp, buffer);
					appendstring(cp, "\n");
					// other conversd's have a bad UADD handling and need UDAT.
				}
			}
			if (!cp->amprnet && !p->observer && strcmp(p->pers, "@")) {
				sprintf(buffer, "/\377\200UDAT %s %s %s\n", p->name, p->host, p->pers);
				linklog(cp, L_SENT, "%s", &buffer[3]);
				appendstring(cp, buffer);
			}
skiped_umsgs:
			if ((cp->features & FEATURE_AWAY) && *p->away) {
				if (cp->oldaway)
					sprintf(buffer, "/\377\200AWAY %s %s 69 %ld %s\n", p->name, p->host, (long)p->atime, p->away);
				else
					sprintf(buffer, "/\377\200AWAY %s %s %ld %s\n", p->name, p->host, (long)p->atime, p->away);
				linklog(cp, L_SENT, "%s", &buffer[3]);
				appendstring(cp, buffer);
			}
#ifdef WANT_FILTER
			if ((cp->features & FEATURE_FILTER) && p->filter && !p->observer) {
				sprintf(buffer, "/\377\200FILT %ld %s %s %s\n", p->filter_time, p->name, p->host, p->filter);
				appendstring(cp, buffer);
			}
#endif
end_p:
			announce_user_host = (next_is_same_user_host ? 0 : 1);
			announce_user = (next_is_same_user ? 0 : 1);
		}
	}

  }
  	clear_chan_locks();
	announce_banned_users(cp);
	/* give a bonus as long ping/pong is not computed,
	 * so the link has a chance to stay when another host does h_host()
	 * and for having enough time to exchange user information.
	 * furthermore, txtime is viewable as "Quality" in the link-list,
	 * and update_destinations() sets the dest-list properly
	 */
	//me_in_permlist->txtime = 99;
	me_in_permlist->txtime = 3;
	me_in_permlist->rxtime = -1;
	me_in_permlist->waittime = 9; // reset waittime state after a successful connect
	me_in_permlist->retrytime = currtime; // reset retrytime state
	update_destinations(me_in_permlist, cp->name, 99/99, cp->rev, 0, myhostname);
}

/*---------------------------------------------------------------------------*/

/* this command simply passes on any host requests that we don't understand,
   since our neighbors MIGHT understand them */

void h_unknown_command(struct connection *cp)
{
	struct connection *p;
	int len, has_lf;
	
	if ((len = strlen(cp->ibuf) < 1))
		return;
	has_lf = (cp->ibuf[len-1] == '\n' ? 1 : 0);

	for (p = connections; p; p = p->next) {
		if (p->type == CT_HOST && p != cp && !p->ircmode) {
			linklog(p, L_SENT, "%s", &cp->ibuf[3]);
			appendstring(p, cp->ibuf);
			if (!has_lf)
				appendstring(p, "\n");
		}
	}
}


/*---------------------------------------------------------------------------*/

void h_invi_command(struct connection *cp)
{

	char *fromname, *toname;
	int channel;

	fromname = getarg(0, 0);
	toname = getargcs(0, 0);
	channel = atoi(getarg(0, 0));
	if (!user_check(cp, fromname))
		return;
	cp->locked = 1;

	if (check_cmd_flood(cp, fromname, 0, SUL_INVI, 0, toname))
		return;

	send_invite_msg(fromname, toname, channel);
}

/*---------------------------------------------------------------------------*/

void h_link_command(struct connection *cp)
{
	char *fromname, *host;
	char buffer[2048];

	fromname = getargcs(0, 0);
	host = getargcs(0, 0);
	if (!user_check(cp, fromname))
		return;
	if ((strlen(fromname) > NAMESIZE) || (strlen(host) > NAMESIZE))
		return;
	if (!strcasecmp(((host[0] == '@') ? (host+1) : host), myhostname) ||
		 !strcasecmp(((host[0] == '@') ? (host+1) : host), "all")) {
		clear_locks();
		sprintf(buffer, "*** Links at %s", myhostname);
		send_msg_to_user("conversd", fromname, buffer);
		display_links_command(NULLCONNECTION, fromname, "");
		if (strcasecmp((host[0] == '@' ? (host+1) : host), "all"))
			return;
		cp->locked = 1;
	}
	/* else, pass it on */
	sprintf(buffer, "/\377\200LINK %s %s\n", fromname, host);
	strcpy(cp->ibuf, buffer);
	h_unknown_command(cp);
}

/*---------------------------------------------------------------------------*/

void h_oper_command(struct connection *cp)
{
	char *toname, *tonickname, *tohost, *fromname, *fromnickname, *fromhost;
	int channel;
	struct connection *p;
	struct clist *cl;
	int status_changed = 0;
	int found = 0;
	int search_nickname = 1;
	
	fromname = getarg(0, 0);
	search_nickname = strcasecmp(fromname, "conversd");
	channel = atoi(getarg(0, 0));
	toname = getarg(0, 0);
	cp->locked = 1;
	fromnickname = fromname;
	fromhost = fromname;
	tonickname = 0;
	tohost = 0;

	for (p = connections; p; p = p->next) {
		if (p->type != CT_USER)
			continue;
		if (p->observer)
			continue;
		// store possible sender's nick
		if (search_nickname && p->via && !strcasecmp(p->name, fromname)) {
			fromnickname = p->nickname;
			fromhost = p->host;
		}
		if (strcasecmp(p->name, toname))
			continue;
		if (channel == -1) {
			if (!tonickname || !p->operator) {
				// store possible receipient's nick
				tonickname = p->nickname;
				tohost = p->host;
			}
			if (!status_changed) {
				status_changed = (p->operator == 0); 
			}
			// secretnumber < 0: remote administration possible
			// then user must be logged in localy, too to gain this right
			if (p->operator < 2)
				p->operator = ((secretnumber < 0 && !p->via) ? 2 : 1);
			if (!found)
				found = 1;
		} else {
			if (p->channel == channel) {
				if (!tonickname || !p->channelop) {
					// store possible receipient's nick
					tonickname = p->nickname;
					tohost = p->host;
				}
				if (!status_changed) {
					status_changed = (p->channelop == 0);
				}
				p->channelop = 1;
				if (!found)
					found = 1;
			}
			// must go through this list
			for (cl = p->chan_list; cl; cl = cl->next) {
				if (cl->channel == p->channel) {
					if (!tonickname || !cl->channelop) {
						// store possible receipient's nick
						tonickname = p->nickname;
						tohost = p->host;
					}
					if (!status_changed) {
						status_changed = (cl->channelop == 0);
					}
					cl->channelop = 1;
					if (!found)
						found = 1;
				}
			}
		}
			
	}
	if (!tonickname) {
		tonickname = toname;
		tohost = cp->name;
	}
	if (!found)
		status_changed = 1; // be transparent
	send_opermsg(toname, tonickname, tohost, fromname, fromnickname, fromhost, channel, status_changed);
}

/*---------------------------------------------------------------------------*/

void h_ping_command(struct connection *cp)
{
	char buffer[2048];
	int pl;

	if (!permarray)
		return;

	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if (permarray[pl] && (permarray[pl]->connection == cp)) {
			/* dl9sau: ping from our link-partner
			 *   just 2b sure..: rtt < 1 is a really bad
			 *   idea, because other parts of the protocol 
			 *   interprets it as "dead link".
			 */
			sprintf(buffer, "/\377\200PONG %ld\n", (permarray[pl]->txtime > 0) ? permarray[pl]->txtime : 1L);
			linklog(cp, L_SENT, "%s", &buffer[3]);
			appendstring(cp, buffer);
		}
	}
}

/*---------------------------------------------------------------------------*/

void h_pong_command(struct connection *cp)
{
#define SRTT(last, current) ((5L * last + current) / 6L)
//#define SRTT(last, current) ((last + current * 2L) / 3L)
	struct permlink *l;
	int pl;
	long rxtime, txtime;
	long rtt;

	l = NULLPERMLINK;

	if (!permarray)
		return;

	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if (permarray[pl] && (permarray[pl]->connection == cp)) {
			l = permarray[pl];
			rxtime = atol(getarg(0, 0));
			txtime = max(currtime - l->testwaittime, 1);
			if (rxtime < 1)
				rxtime = txtime;
			if (l->txtime)
				l->txtime = SRTT(l->txtime, txtime);
			else
				l->txtime = txtime;

			l->rxtime = rxtime;

			rtt = (l->txtime + l->rxtime) / 2L;
			//l->testnexttime = l->testwaittime + max(min(60 * (l->txtime), 7200), 120);
			//l->testnexttime = currtime + max(min(rtt * 15, 300), 60);
			l->testnexttime = currtime + max(min(rtt * 15, 300), 120);
			update_destinations(l, l->name, rtt, cp->rev, 0, myhostname);
			if (l->curr_group >= 0)
				l->s_groups[l->curr_group].quality = 255.0 - (255.0 * (double) min((rtt - 1) * 10, 6000) / 6000.0);
			break;
		}
	}
}

/*---------------------------------------------------------------------------*/

void h_rout_command(struct connection *cp)
{
	char *dest, *user;
	struct destination *d;
	char buffer[2048];
	int ttl;
	int goodlink;

	dest = getargcs(0, 0);
	user = getarg(0, 0);
	ttl = atoi(getarg(0, 0));
	if (!user_check(cp, user))
		return;
	clear_locks();
	for (d = destinations; d; d = d->next) {
		goodlink = (d->link && d->link->name && *d->link->name);
		if (!strcasecmp(d->name, dest)) {
			sprintf(buffer, "*** route: %s -> %s -> %s (%ld)", myhostname,
					(goodlink) ? d->link->name : "(dead link)", dest, d->rtt);
			send_msg_to_user("conversd", user, buffer);
			if (ttl && goodlink && strcasecmp(d->link->name, dest)) {
				ttl--;
				sprintf(buffer, "/\377\200ROUT %s %s %d\n", dest, user, ttl);
				linklog(cp, L_SENT, "%s", &buffer[3]);
				appendstring(d->link->connection, buffer);
			}
		}
	}
}

/*---------------------------------------------------------------------------*/

void h_sysi_command(struct connection *cp)
{
	char *user, *host;
	char buffer[2048];
#if defined(linux)
	char lx[128];
	char *chp;
	long uptime;
	FILE *fd;
#endif

	user = getargcs(0, 0);
	host = getargcs(0, 0);
	if (host && *host == '@') {
		// be irc user friendly
		host++;
	}
	cp->locked = 0;
	if (!user_check(cp, user))
		return;

	if (!strcasecmp(host, myhostname) || !strcasecmp(host, "all")) {	/* if for us */
		sprintf(buffer, "*** %s: System Information [%s] - email to %s", myhostname, SOFT_TREE, (myemailaddr) ? myemailaddr : "*** unknown ***");
		send_msg_to_user("conversd", user, buffer);
		clear_locks();
		if (mysysinfo && *mysysinfo) {
			sprintf(buffer, "*** %s: %s", myhostname, (mysysinfo) ? mysysinfo : "No additional information available");
			send_msg_to_user("conversd", user, buffer);
			clear_locks();
		}
		sprintf(buffer, "*** %s: %s version %s", myhostname, SOFT_TREE, SOFT_RELEASE);
		send_msg_to_user("conversd", user, buffer);
		clear_locks();
		sprintf(buffer, "*** %s: %s is up for %s QTR here is %s %s.", myhostname, convtype, ts4(currtime - boottime), ts2(currtime), mytimezone);
		send_msg_to_user("conversd", user, buffer);
#if defined(linux)
		clear_locks();
		if (!access("/proc/uptime", R_OK) && (fd = fopen("/proc/uptime", "r"))) {
			fscanf(fd, "%ld", &uptime);
			fclose(fd);

			if (!access("/proc/version", R_OK) && (fd = fopen("/proc/version", "r"))) {
				fgets(lx, 128, fd);
				fclose(fd);
				chp = lx + 15;
				while (*chp != ' ') {
					chp++;
				}
					*chp = '\0';
			} else {
				sprintf(lx, "Linux");
			}
			sprintf(buffer, "*** %s: %s is up for %s", myhostname, lx, ts4(uptime));
			send_msg_to_user("conversd", user, buffer);
		}
#endif
		if (strcasecmp(host, "all"))
			return;
	}
	/* else, pass it on */
	sprintf(buffer, "/\377\200SYSI %s %s\n", user, host);
	strcpy(cp->ibuf, buffer);
	h_unknown_command(cp);
}

/*---------------------------------------------------------------------------*/

void h_topi_command(struct connection *cp)
{
	char *fromname, *fromnickname, *hostname, *text;
	int channel;
	time_t time;

	fromname = getarg(0, 0);
	hostname = getargcs(0, 0);
	fromnickname = find_nickname(fromname, hostname);
	time = atol(getarg(0, 0));
	if (time > currtime)
		time = currtime;
	channel = atoi(getarg(0, 0));
	text = getarg(0, 1);

	if (!strcasecmp(fromname, "zeus")) {
		do_log(L_ERR, "Ancient gods detected in the direction of %s, ignoring their madness.", cp->name);
		return;
	}

	if ((!strcasecmp(fromname, "conversd")) && (strstr(text, "may not be longer than"))) {
		do_log(L_ERR, "Echoes of ancient gods heard in the direction of %s, ignoring it's foolishness.", cp->name);
		return;
	}

	if (!user_check(cp, fromname))
		return;
	cp->locked = 1;

	if (check_cmd_flood(cp, fromname, hostname, SUL_TOPIC, 0, text))
		return;

	send_topic(fromname, fromnickname, hostname, time, channel, text);
}

/*---------------------------------------------------------------------------*/

void h_udat_command(struct connection *cp)
{
	char *fromname, *hostname, *text;
	//struct connection *p;
	//struct connection *p_found = 0;

	fromname = getarg(0, 0);
	hostname = getargcs(0, 0);
	text = getarg(0, 1);

	text = make_pers_consistent(text);

	if (!*text)
		text = "@";

	if (!user_check(cp, fromname))
		return;
	cp->locked = 1;
#ifdef	notdef
	for (p = connections; p; p = p->next) {
		if (p->via && p->type == CT_USER && !p->observer && !strcasecmp(p->name, fromname) &&
				!strcasecmp(p->host, hostname) && strcmp(p->pers, text)) {
			p->isauth = (*text == '~' ? 1 : 2);
			strncpy(p->pers, text, PERSSIZE);
			p->pers[sizeof(p->pers)-1] = 0;
			p_found = p;
			//send_persmsg(fromname, p->nickname, hostname, p->channel, text, text_changed, currtime);
		}
	}
	if (p_found)
		send_user_change_msg(p_found->name, p_found->nickname, p_found->host, -1, -1, p_found->pers, 1, currtime, p_found->observer, 0, 0, 0, p_found->nickname);
#else
	change_pers_and_nick(cp, fromname, hostname, text, -1, 0, 1, 0);
#endif
}

/*---------------------------------------------------------------------------*/

void h_umsg_command(struct connection *cp)
{
	char *name, *fromuser, *fromnick, *toname, *text;

	name = getargcs(0, 0);
	toname = getarg(0, 0);
	text = getarg(0, 1);

	if (!user_check(cp, name))
		return;

	fromuser = get_user_from_name(name);
	fromnick = get_nick_from_name(name);

	// restore leading blanks, kept by getarg(0, 1)
	while (*text && *(text -1) && isspace(*(text - 1) & 0xff))
		text--;

	if (check_msg_flood(cp, toname, -1, fromuser, fromnick, text))
		return;

	update_nickname_data(cp, fromuser, fromnick);

	if ((!strcmp(name, "conversd")) && (strstr(text, "calling CQ")))
		return;

	if (*text)
		send_msg_to_user2(name, fromuser, fromnick, toname, text, 0);
}

/*---------------------------------------------------------------------------*/

void h_ban_command(struct connection *cp)
{
	char *name, *fromhost, *reason;
	int mins;
	time_t when;

	if (!FeatureBAN)
		return;
	if (!(FeatureBAN & ACCEPT_BAN)) {
		int len = strlen(cp->ibuf);
		if (len < MAX_MSGSIZE) {
			cp->ibuf[len] = ' ';
			cp->ibuf[MAX_MSGSIZE] = 0;
		}
		h_unknown_command(cp);
		return;
	}

	name = getarg(0, 0);
	fromhost = getargcs(0, 0);

	if (!*name || !*fromhost)
		return;

	when = atol(getarg(0, 0));
	mins = atoi(getarg(0, 0));
	reason = getargcs(0, 1);

	if (!*reason)
		reason = "[unknown]";

	if (when > currtime)
		when = currtime;

	// adds local ban, and will announce to permlinks
	ban_user(name, fromhost, 0, when, mins, reason);
}

/*---------------------------------------------------------------------------*/

void h_info_command(struct connection *cp)
{
	char *arg;
	char *host;

	host = getargcs(0, 0);
	arg = getarg(0, 1);
	if (!*host || !*arg)
		return;
	if (strlen(host) > NAMESIZE)
		host[NAMESIZE] = 0;
	if (strlen(arg) > 255)
		arg[255] = 0;
	do_log(L_INFO, "info (link %s): %s %s", cp->name, host, arg);
	cp->locked = 1;
	send_server_notice(host, arg, 0, 0);
}

/*---------------------------------------------------------------------------*/

void loop_handler(struct connection *cp, struct permlink *pl, int reason, char *user, char *host, char *already_via)
{
	char buffer[2048];

	if (!cp)
		return;
	if (pl) {
		pl->locked = 1;
		pl->locked_until = currtime + 1800;
		delay_permlink_connect(pl, 5);
	}
	if (!already_via)
	  already_via = myhostname;
	do_log(L_R_ERR, "conversd tx: LOOP %s %s %s", cp->name, host, user, already_via);
	// we generate an INFO message. Could not send LOOP, because the
	// h_loop_command would cause the remote permlink to close his
	// connection to us
	if (reason == CT_USER) {
	  sprintf(buffer, "/\377\200INFO %s detected loop with %s: USER %s@%s. HOST already via %s%s", myhostname, cp->name, user, host, already_via, cp->ax25 ? "\r" : "\n");
	} else {
	  if (*user) {
	  	sprintf(buffer, "/\377\200INFO %s detected loop with %s: USER %s@%s. DEST already via %s%s", myhostname, cp->name, user, host, already_via, cp->ax25 ? "\r" : "\n");
	  } else {
	    sprintf(buffer, "/\377\200INFO %s detected loop with %s: DEST %s. DEST already via %s%s", myhostname, cp->name, host, already_via, cp->ax25 ? "\r" : "\n");
	  }
	}
	fast_write(cp, buffer, 0);
	rip(buffer);
	do_log(L_LINK, buffer + 3);
	getarg(buffer, 0);
	cp->locked = 1;
	h_info_command(cp);

	sprintf(buffer, "/\377\200LOOP %s %s %s %s %s%s", myhostname, cp->name, host, already_via, ((reason == CT_USER || *user) ? "HOST" : "DEST"), cp->ax25 ? "\r" : "\n");
	fast_write(cp, buffer, 0);
	linklog(cp, L_SENT, "%s", &buffer[3]);

	sprintf(buffer, "%s<>%s broken (loop detected)", myhostname, cp->name);
	bye_command2(cp, buffer);
	return;
}

/*---------------------------------------------------------------------------*/

void handle_h_quit_commands(struct connection *cp, int is_squit)
{
	char buffer[2048];
	struct connection *p;
	struct connection *p_tohost = 0;
	struct destination *d;
	int len;
	char *user;
	char *host;
	char *reason;

	if (!is_squit)
		user = getarg(0, 0);
	else
		user = 0;
	host = getargcs(0, 0);
	reason = getargcs(0, 1);

	if ((!is_squit && !*user) || !*host) {
		if (is_squit)
			do_log(L_CRIT, "squit: protocol error. got host >%s<", host);
		else
			do_log(L_CRIT, "uquit: protocol error. got user >%s< host >%s<", user, host);
		return;
	}

	if ((len = strlen(reason)) > 255)
		reason[255] = 0;
	if (!*reason)
		reason = (is_squit ? "squit" : "quit");

	if (is_squit && !strcasecmp(host, cp->name)) {
		// our neighbour said SQUIT to us
		// example from irc: db0tud has ping timeout with db0lpz-svr
		// -> sends:
		// to db0lpz-svr: db0tud.ampr.org SQUIT db0tud.ampr.org 0 :Ping timeout
		// to rest of the net: db0tud.ampr.org SQUIT db0lpz-svr.ampr.org 1021243494 :Ping timeout
	    	sprintf(buffer, "/\377\200INFO %s received%s squit %s from %s. reason: %s.%s", myhostname, (AcceptSquit ? "" : "(but rejecting)"), host, cp->name, reason, cp->ax25 ? "\r" : "\n");
		fast_write(cp, buffer, 0);
		rip(buffer);
		do_log(L_LINK, buffer + 3);
		getarg(buffer, 0);
		cp->locked = 1;
		h_info_command(cp);
		if (!AcceptSquit)
			return;
		// give feedback
		sprintf(buffer, "/\377\200SQUIT %s %s<>%s disc (%s)%s", cp->name, myhostname, cp->name, reason, cp->ax25 ? "\r" : "\n");
		fast_write(cp, buffer, 0);
		// bye_command2 will send squit downstream
		bye_command2(cp, reason);
		return;
	}
		
	if (!strcasecmp(host, myhostname)) {
		if (!is_squit)
			return;
		// SQUIT for our host has reached us.
		// point of no return. do not propagate to our neighbours.
	    	sprintf(buffer, "/\377\200INFO %s received%s squit %s from %s. reason: %s.%s", myhostname, (AcceptSquit ? "" : "(but rejecting)"), host, cp->name, reason, cp->ax25 ? "\r" : "\n");
		fast_write(cp, buffer, 0);
		rip(buffer);
		do_log(L_LINK, buffer + 3);
		getarg(buffer, 0);
		cp->locked = 1;
		h_info_command(cp);
		if (!AcceptSquit)
			return;
		// give feedback
		sprintf(buffer, "/\377\200SQUIT %s %s<>%s disc (%s)%s", myhostname, myhostname, cp->name, reason, cp->ax25 ? "\r" : "\n");
		fast_write(cp, buffer, 0);
		bye_command2(cp, reason);
		return;
	}

	// find direction of the SQUIT or UQUIT message
	// first search the destination lists
	for (d = destinations; d; d = d->next) {
		if (!strcasecmp(d->name, host))
			break;
	}
	if (d && d->link && d->link->connection) {
		if (d->link->connection == cp) {
			goto send_upstream;
		} else {
			p_tohost = d->link->connection;
			goto pass_downstream;
		}
	}
	// fallback
	for (p = connections; p; p = p->next) {
		if (p->via && !strcasecmp(p->host, host))
			break;
	}
	if (p && p->via != cp) {
		p_tohost = p->via;
		goto pass_downstream;
	}
	// announced host was part of cp's subnet
	// or: messages of unkown source go upstream
	
send_upstream:
	send_quits(cp, user, host, reason);
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && p->via && p->via == cp && (is_squit || !strcasecmp(user, p->name)) && !strcasecmp(host, p->host)) {
			int channel = p->channel;
			cp->locked = 1;
			// set before count_user()
			// send before destroy_channel()
			p->type = CT_CLOSED;
			p->channel = -1;
			send_user_change_msg(p->name, p->nickname, p->host, channel, -1, reason, 1, currtime, 0, 1, 0, 0, p->nickname);
			clear_locks();
			if (count_user(channel) < 1) {
				// rfc2811 channel delay when squit message
				destroy_channel(channel, is_squit);
			} else {
				update_channel_history(cp, channel, p->name, p->host);
			}
		}
	}
	return;

pass_downstream:
	// SQUIT only
	// if QUIT and coming from the outer net, it's an error
	// (would have the effekt like KILL in irc). silently drop
	if (!is_squit || !p_tohost || p_tohost == cp)
		return;
	sprintf(buffer, "/\377\200SQUIT %s %s\n", host, reason);
	appendstring(p_tohost, buffer);
}

/*---------------------------------------------------------------------------*/

void h_squit_command(struct connection *cp)
{
	handle_h_quit_commands(cp, 1);
}

/*---------------------------------------------------------------------------*/

void h_uquit_command(struct connection *cp)
{
	handle_h_quit_commands(cp, 0);
}

/*---------------------------------------------------------------------------*/

int checkhost(struct connection *cp, char *name, char *host, int learn_new_nodes)
{
	struct permlink *pl;
	struct destination *d;
	struct connection *p;
	/* dl9sau: this is a good chance to learn new destinations.
	 * it may be a good point too to see if we have a loop
	 * but do not learn our own host..
	 * this is the loop detector. we check if the host this user 
	 * logged is downstream from us. Else we disconnect the according link
	 * and lock it for half of an hour. We cannot check every message,
	 * because the "host" field is not part of CMSG.
	 * 
	 * There is another problem: the loop detector will not work
	 * reliable until every host supports this feature, because during the
	 * initial handshake between two hosts, the user information is sent
	 * *before* the destination information (for historical reasons). 
	 * So, to make it work properly, we would have to scan the whole userlist
	 * for bogus logins on *every* new destination information. Hm.
	 */

	pl = find_permlink(cp->name);
	if (!strcasecmp(host, myhostname)) {
		loop_handler(cp, pl, CT_HOST, "", name, myhostname);
		return 0;
	}
	d = find_destination(host);
	if (d && pl && pl->connection && pl->connection == cp && d->link && d->link->connection && d->link->connection != cp) {
		loop_handler(cp, pl, CT_HOST, name, host, d->link->connection->name);
		return 0;
	}
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER) {
			/* new 920705 dl9sau */
			/* If Neighbour2 registers a user on HostX, while someone has already
			 * been registered for HostX via Neighbour1, then we definitely have
			 * a loop !  We send a loop detect message and then close the link:
			 * /..LOOP <Chostname> <myneighbour> <host>
			 *
			 * The LOOP PREVENTION CODE detects ONLY a loop if it starts at this
			 * host. That's, why I suggest this code to be implemented in every
			 * conversd implementation.
			 */
			if (p->via != cp && !strcasecmp(p->host, host)) {
				loop_handler(cp, pl, CT_USER, name, host, (p->via ? p->via->name : myhostname));
				return 0;
			}
		}
	}
	if (pl) {
		if (!d || d->link == NULLPERMLINK || !d->rtt) {
			// no destination found. learn it now
			//update_destinations(pl, host, 99/99, "");
			if (learn_new_nodes)
				update_destinations(pl, host, ((pl->rxtime == -1) ? pl->txtime : (pl->rxtime + pl->txtime) / 2L) + 99/99, "-", 1, cp->name);
		}
	}
	return 1;
}

/*---------------------------------------------------------------------------*/

struct connection *make_new_user(struct connection *cp, char *name, char *host, int newchannel, time_t time, char *force_mode, int force_chanop, int force_operator, int force_observer, int force_away, int force_creator, char *pers, int force_pers_changed, char *nick, int force_nick_changed)
{
	int remember_flags;
	int is_operator = force_operator;
	struct connection *p, *p2, *p_found;
	struct channel *ch;
	int pers_changed = force_pers_changed;
	int nick_changed = force_nick_changed;
	char oldnickname[NAMESIZE+1];

	p_found = 0;
	*oldnickname = 0;
	// find nickname and away info from another connection via this host
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER && !strcasecmp(p->name, name)) {
			// observers do not have interesting data (since they can't transmit
			if (p->observer)
				continue;
			// generaly: is operator?
			if (p->operator && !force_observer)
				is_operator = 1;
			if (force_operator && !p->operator && !p->observer)
				p->operator = 1;
			// match?
			if (p->via != cp || strcasecmp(p->host, host))
				continue;
			if (force_nick_changed) {
				if (!*oldnickname) {
					strncpy(oldnickname, p->nickname, NAMESIZE);
					oldnickname[NAMESIZE] = 0;
				}
				strncpy(p->nickname, nick, NAMESIZE);
				p->nickname[sizeof(p->nickname)-1] = 0;
				p->nickname_len = strlen(p->nickname);
			}
			if (force_pers_changed) {
				strncpy(p->pers, pers, PERSSIZE);
				p->pers[sizeof(p->pers)-1] = 0;
			}
			if (force_away)
				p->atime = time;
			// walk through the whole list, do not break:
			// user already announced (bug occured in ppconversd)?
			// if dest-channel time and no pers update,
			// or time differs put /pers equals (pers change?)
			// -> then drop
			if (p->channel == newchannel) {
				if (!*pers && p->time == time)
					return 0;
				if (!strcmp(p->pers, pers) && p->time == time) {
					p->time = time;
					if (p->mtime < time)
						p->mtime = time;
					return 0;
				}
			}
			// better match
			if (!p_found)
				p_found = p;
		}
	}

	p = (struct connection *)hcalloc(1, sizeof(struct connection));
	if (!p) {
		bye_command2(cp, "memory exhausted");
		return 0;
	}
	p->type = CT_USER;
	p->session_type = SESSION_CIRCUIT;
	strncpy(p->name, name, NAMESIZE);
	p->name[sizeof(p->name) - 1] = 0;
	p->name_len = strlen(p->name);
	strncpy(p->nickname, (p->observer ? name : nick), NAMESIZE);
	p->nickname[sizeof(p->nickname) -1] = 0;
	p->nickname_len = strlen(p->nickname);
	strncpy(p->host, host, NAMESIZE);
	p->host[sizeof(p->host) -1] = 0;
	p->via = cp;
	p->fd = -1;
	p->query[0] = '\0';
	p->notify[0] = '\0';
#ifdef	HAVE_FILTER
	p->filter = NULLCHAR;
	p->filterwords = NULLCHAR;
#endif
	p->time = time;
	p->mtime = time;
		
	p->next = connections;
	connections = p;

	generate_sul(p, cp, 0);

	p->channel = newchannel;

	do_log(L_NUSER, "New user %s@%s", name, host);

	if (force_observer) {
		pers = OBSERVERID;
		p->observer = 1;
	} else {
		if (force_away)
			p->atime = time;
		if (force_chanop)
			p->channelop = 1;
		if (*pers && !strcasecmp(pers, OBSERVERID)) {
			force_observer = p->observer = 1;
		}
	}
	if (!force_observer)
		p->operator = is_operator;

	if (*pers || force_observer) {
		strncpy(p->pers, pers, PERSSIZE);
		p->pers[sizeof(p->pers)-1] = 0;
		p->isauth = (*pers == '~' ? 1 : 2);
		pers_changed = 1;
	} 
	if (p_found && !force_observer) {
		if (!*pers) {
			if (*p_found->pers) {
				strncpy(p->pers, p_found->pers, PERSSIZE);
				p->pers[sizeof(p->pers)-1] = 0;
				p->isauth = (*pers == '~' ? 1 : 2);
				// pers_changed = 0; - found
			} /* else: no pers found */
		} else {
			/* new pers */
			if (*p_found->pers && !strcmp(p_found->pers, p->pers)) {
					if (!force_pers_changed)
						pers_changed = 0;
			} else {
				/* we have to actualize the pers data for all user @ host sessions */
				for (p2 = connections; p2; p2 = p2->next) {
					if (p2 != p && p2->type == CT_USER && p2->via == cp && !p2->observer && !strcasecmp(p2->name, p->name) && !strcasecmp(p2->host, p->host)) {
						strncpy(p2->pers, p->pers, PERSSIZE);
						p2->pers[sizeof(p2->pers)-1] = 0;
						p2->isauth = (*p2->pers == '~' ? 1 : 2);
					}
				}
				// we have to announce the change
				pers_changed = 1;
			}
		}
		if (strcmp(p->nickname, p_found->nickname)) {
			strncpy(p->nickname,p_found->nickname, NAMESIZE);
			p->nickname[sizeof(p->nickname)-1] = 0;
			nick_changed = 1;
		}
		// actualize away info for the new connection
		if (p_found->atime && *p_found->away) {
			p->atime = p_found->atime;
			strncpy(p->away, p_found->away, AWAYSIZE);
			p->away[sizeof(p->away)-1] = 0;
		}
#ifdef	WANT_FILTER
		if (p_found->filter) {
			if ((p->filter = hmalloc(strlen(p_found->filter) +1))) {
				strcpy(p->filter, p_found->filter);
				p->filter_time = p_found->filter_time;
			}
		}
#endif
	}

	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == newchannel)
			break;
	}
	if (ch) {
		if (!p->observer && (ch->expires != 0L || ch->ctime > time) && (force_creator || !*ch->createby)) {
			strncpy(ch->createby, name, NAMESIZE);
			ch->createby[sizeof(ch->createby)-1] = 0;
			ch->ctime = time;
			force_chanop = 2; /* announce as chanop and creator */
		} 
		ch->expires = 0L;
		ch->locked_until = 0L;
	} else {
		ch = new_channel(newchannel, name);
		// adjust channel creation time
		ch->ctime = time;
	}
	remember_flags = ch->flags;
	if (!(ch->flags & (M_CHAN_L | M_CHAN_I | M_CHAN_S | M_CHAN_P)) && strpbrk(force_mode, "lisp")) {
		// fake flags (do not announce private / secret / invisible channels)
		ch->flags |= ((strchr(force_mode, 'l') ? M_CHAN_L : 0) | (strchr(force_mode, 'i') ? M_CHAN_I : 0) | (strchr(force_mode, 's') ? M_CHAN_S : 0) | (strchr(force_mode, 'p') ? M_CHAN_P : 0));
	}

	cp->locked = 1;

	send_user_change_msg(name, p->nickname, host, -1, newchannel, p->pers, pers_changed, time, p->observer, 0, force_chanop, nick_changed, (*oldnickname ? oldnickname : p->nickname));

	//log(L_DEBUG, "got from %s: USER %s %s %d %d %ld", cp->name, name, host, oldchannel, newchannel, time);

	// reestablish flags
	ch->flags = remember_flags;
	return p;
}

/*---------------------------------------------------------------------------*/

void h_netjoin_command(struct connection *cp)
{
	char buffer[2048];
	time_t time;
	int channel;
	int nick_changed = 0;
	int pers_changed = 0;
	int is_away = 0;
	int is_observer = 0;
	int is_operator = 0;
	int is_chanop = 0;
	int is_creator = 0;
	struct connection *p;
	char *name;
	char *host;
	char *nick;
	char *pers;
	char *chan;
	char *mode;
	char *arg;

#define lock_j_unlock_other(s) { \
	for (p = connections; p; p = p->next) { \
		if (cp != p && !p->via && p->type == CT_HOST && (p->features & FEATURE_SAUPP_I)) { \
			if (s) \
				appendstring(p, s); \
			p->locked = 1; \
		} else if (cp == p) \
			p->locked = 1; \
		else \
			p->locked = 0; \
	} \
}

	// save input buffer before it's fragmented
	sprintf(buffer, "/\377\200NJ %s\n", showargp());

	name = getarg(0, 0);
	host = getargcs(0, 0);
	if (!*name || !*host)
		return;

	// loop check
	if (!checkhost(cp, name, host, 1))
		return;

	// some bad users
	if (!user_check(cp, name))
		is_observer = 1;

	// umode:nick
	// first check the nick
	mode = getargcs(0, 0);

	/* temporary: there is a bug around, in the form
	 * "/377\200NJ te1st  +Nick 1 1056729926". happens very seldomly,
	 * and after months of waiting i still have not found why this
	 * occurs.
	 */
	if (!*mode)
		goto err;
	switch (*host) {
	case '+':
	case '-':
	case '=':
err:
		do_log(L_ERR, "error: h_netjoin_command(): empty host for %s (%s) from neighbor %s - should never happen!", name, host, cp->name);
		do_log(L_ERR, "error: parsed mode >%s< host >%s< line >%s<", mode, host, buffer);
		return;
	}

	nick = name;
	if ((arg = strpbrk(mode, "+-="))) {
		switch (*arg) {
		case '-':
			// nick reset to name
			/* fall through */
		case '+':
			// new nick
			if (!is_observer) {
				if (*(arg+1))
					nick = arg+1;
				nick_changed = 1;
			}
			break;
		// default: // no change
		}
		*arg = 0;
	}

	// user modes
	while (*mode) {
		switch (*mode) {
		case 'a':
			is_away = 1;
			break;
		case 'o':
			is_operator = 1;
			break;
		case 'r':
			is_observer = 1;
			break;
		}
		mode++;
	}

	// "...channelMess... :this is a pers" set?
	arg = getargcs(0, 1);
	while (*arg && isspace(*arg & 0xff))
		arg++;
	pers = arg;
	if (*pers == ':' || (pers = strstr(pers, " :"))) {
		pers_changed = 1;
		if (*pers != ':')
			*pers++ = 0;
		// skip ':'
		*pers++ = 0;
		// skip blanks (if any)
		while (*pers && isspace(*pers & 0xff))
			pers++;
		// empty pers and pers changed -> "reset"
		if (!*pers)
			pers = "@";
	} else {
		pers = "";
	}

	chan = getarg(arg, 0);

	// check for a join flood befor announcing the new user
	if (*chan) {
		if (check_join_flood(cp, name, host, atoi(chan))) {
			return;
		}
	} else {
		if (!nick_changed && !pers_changed && !is_operator) {
			// oops - nothing to do..
			return;
		}
	}

	// first send it all permlinks which support this feature.
	// cave: do not send it to other hosts later, because the h_unknown command
	// will forward it as it is. thus, duplicated messages may occur
	lock_j_unlock_other(buffer);


	do {
		if (*chan) {
			// reset state
			is_chanop = is_creator = 0;
			// 2 tokens: channel[:modes] time
			if ((mode = strchr(chan, ':'))) {
				*mode++ = 0;
				if (*mode == '@') {
					is_chanop = 1;
					mode++;
					if (*mode == '@') {
						is_creator = 1;
						mode++;
					}
				}
			} else
				mode = "";
			if (!stringisnumber(chan) || (channel = atoi(chan)) < 0) {
				// protocol violation;
				do_log(L_CRIT, "debug: NJ_PROTO_VIOLATION from %s: name %s host %s channel %s mode %s nick %s pers %s rest >%s< %d %d \n", cp->name, name, host, chan, mode, nick, pers, getargcs(0, 1));
				return;
			}
			arg = getarg(0, 0);
			if ((time = atol(arg)) == 0L)
				time = currtime;

			// save arg vector (need this for mode_command())
			arg = showargp();
			if (check_join_flood(cp, name, host, channel))
				return;
			if (!(make_new_user(cp, name, host, channel, time, mode, is_chanop, is_operator, is_observer, is_away, is_creator, pers, pers_changed, nick, nick_changed)))
				return;
			// forced channel mode?
			if (*mode) {
				lock_j_unlock_other(0);
				sprintf(buffer, "/\377\200MODE %d +%s", channel, mode);
				getarg(buffer, 0);
				mode_command(cp);
			}
			if (is_chanop && !is_observer) {
				lock_j_unlock_other(0);
				send_opermsg(name, nick, host, host, host, host, channel, 1);
			}
		} else {
			// if no channels given (where user and nick have been already announced), just announce pers and nick (if changed)
			// flood check here (by change_pers_and_nick()) but not on channel join, because a bad link may break often (also due to the loop protoector)
			change_pers_and_nick(cp, name, host, (pers_changed ? pers : 0), -1, (nick_changed ? nick : 0), 1, 0);
			// will leave the loop
			*arg = 0;
		}
		if (is_operator) {
			lock_j_unlock_other(0);
			send_opermsg(name, nick, host, host, host, host, -1, 1);
			sprintf(buffer, "/\377\200OPER conversd -1 %s", name);
			getarg(buffer, 0);
			h_oper_command(cp);
		}
		// do not announce operator status again
		is_operator = 0;
		// do not announce pers and nick again
		pers_changed = nick_changed = 0;
		// lock FEATURE_SAUPP_I hosts (they've been served already)
		lock_j_unlock_other(0);
		// next tuple
		chan = getarg(arg, 0);
	} while (*chan);
}

/*---------------------------------------------------------------------------*/

void h_user_command(struct connection *cp)
{
	char *host;
	char *name;
	char *pers;
	int newchannel = 0;
	int oldchannel;
	struct connection *p, *p_found;
	time_t time;
	int pers_changed = 0;
	int is_observer = 0;

	pers = name = getarg(0, 0);
	if (!*pers)
		return;		/* invalid user name */
	while (*pers) {
		if (!isgraph(*pers))
			return;	/* invalid user name */
		pers++;
	}
	pers = host = getargcs(0, 0);
	if (!*pers)
		return;		/* invalid host name */

	if (!strcmp(pers, "-1"))
		return;		/* invalid host name */

	while (*pers) {
		if (!isgraph(*pers))
			return;	/* invalid host name */
		if (!isdigit(*pers))
			newchannel++;
		pers++;
	}
	if (!newchannel)
		return;		/* invalid host name - all digits */
	time = atol(getarg(0, 0));
	if (time /*== 0*/ > currtime)
                time = currtime;
	oldchannel = atoi(getarg(0, 0));
	newchannel = atoi(getarg(0, 0));

	pers = getarg(0, 1);

	pers = make_pers_consistent(pers);

	if (oldchannel > MAXCHANNEL)
		oldchannel = -1;
	if (newchannel > MAXCHANNEL)
		newchannel = -1;

	// loop check (only for signon. cave: on signoff, a DEST 0 may have preceeded. so do not update his node again)
	if (!checkhost(cp, name, host, (oldchannel == -1 && newchannel >= 0)))
		return;

	if (oldchannel == -1 && newchannel == -1)
		return;
	is_observer = (tolower(cp->ibuf[3]) == 'o' || !user_check(cp, name));


	p_found = 0;
	if (newchannel >= 0 && oldchannel >= 0) {
		int pers_to_change = (newchannel == oldchannel ? 1 : 0);
		int do_announce = 0;
		p_found = 0;
		if (pers_to_change) {
			if (is_observer || check_cmd_flood(cp, name, host, SUL_PERS, 0, pers))
				return;
			if (!*pers)
				pers = "@";
		}

		// find all occurences of user @ host and change his personal information
		do_announce = 0;
		for (p = connections; p; p = p->next) {
			if (p->type == CT_USER && p->via == cp && !strcasecmp(p->name, name) && !strcasecmp(p->host, host)) {
				// observers may not change their observer id
				if (p->observer && pers_to_change)
					continue;
				if (!p_found)
					p_found = p;
				if (!pers_to_change && p->channel == oldchannel) {
					p->channel = newchannel;
					p->time = time;
					p->mtime = time;
					// one switch per message. now check pers
					pers_to_change = 1; // will never go to this part again
					do_announce = 1;
					p_found = p;
					if (count_user(oldchannel) < 1) {
						// rfc2811 channel delay when squit message
						destroy_channel(oldchannel, 0);
					} else {
						update_channel_history(cp, oldchannel, name, host);
					}
				}
				if ((!*pers && newchannel != oldchannel) || !strcmp(p->pers, pers) || p->observer)
					continue;
				strncpy(p->pers, p->observer ? OBSERVERID : pers, PERSSIZE);
				p->pers[sizeof(p->pers)-1] = 0;
				p->isauth = (*pers == '~' ? 1 : 2);
				do_announce = pers_changed = 1;
			}
		}
		if (p_found && do_announce) {
			send_user_change_msg(name, p_found->nickname, host, oldchannel, newchannel, pers, pers_changed, time, is_observer, 0, 0, 0, p_found->nickname);
		}
		return;
	} else if (newchannel < 0 && oldchannel >= 0) {
		p_found = 0;
		// channel part (or netsplit?)
		// on netsplits, the bye command is neither \"73 es gn\"
		// nor /quit or /leave. we have something like
		// "link1 <> link2 broken" as message, that's how we could
		// decide if a channel should be marked as unavailable,
		// and start a squit autodetection
		for (p = connections; p; p = p->next) {
			if (p->type == CT_USER && p->via == cp && !strcasecmp(p->name, name) && !strcasecmp(p->host, host) && p->channel == oldchannel) {
				if (!p_found) {
					p_found = p;
					if (p->time == time) {
						// better match
						break;
					}
				}
			}
		}
		if (*pers && *pers != '\"' && strstr(pers, "<>")) {
			// already announced in the last run?
			if (p_found) {
				char buffer[2048];
				sprintf(buffer, "/\377\200SQUIT %s %s", host, pers);
				getarg(buffer, 0);
				handle_h_quit_commands(cp, 1);
			}
			return;
		}
		// no. ok. will go the normal procedure.
		// find one (best) match
		if (p_found) {
			p_found->type = CT_CLOSED;
			p_found->channel = -1;
		}
		if (!*pers)
			pers = "/leave";

		// else: pass along
		send_user_change_msg(name, (p_found ? p_found->nickname : name), host, oldchannel, -1, pers, 1, time, is_observer, 0, 0, 0, (p_found ? p_found->nickname : name));

		if (count_user(oldchannel) < 1) {
			// rfc2811 channel delay when squit message
			destroy_channel(oldchannel, 0);
		} else {
			update_channel_history(cp, oldchannel, name, host);
		}
		return;
	} 


	/* last condition: newchannel >= 0 && oldchannel < 0 -> channel join */
	if (check_join_flood(cp, name, host, newchannel))
		return;
	if (!(make_new_user(cp, name, host, newchannel, time, "", 0, 0, is_observer, 0, 0, pers, pers_changed, name, 0)))
		return;


}
