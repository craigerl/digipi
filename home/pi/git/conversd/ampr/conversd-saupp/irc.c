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
 * and incompatibilities fixed; some extensions, some backpatches.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/inet.h>

#ifdef	__GNUC__
#define _GNU_SOURCE
#else
#define	FNM_CASEFOLD	0
#endif
#include <fnmatch.h>

#include "conversd.h"
#include "irc.h"
#include "user.h"
#include "host.h"
#include "tnos.h"
#include "convert.h"
#include "version.h"
#include "log.h"
#include "config.h"
#include "hmalloc.h"
#include "ba_stub.h"
#include "cfgfile.h"
#include "access.h"
#include "noflood.h"

char *irc_sender = 0;	// original sender of an irc command

/*---------------------------------------------------------------------------*/

void irc_nick_command(struct connection *cp)
{
	char *arg;
	char buffer[2048];
	//char oldnickname[NAMESIZE+1];
	int ret;

	arg = getargcs(0, 0);
	if (cp->type == CT_HOST) {
		// not implemented yet
		return;
	}

	// force charset
	cp->charset_in = cp->charset_out = ISO_STRIPED;

	if (*arg == ':')
		arg++;
	if (!*arg) {
		sprintf(buffer, ":%s 431 * :No nickname given\n", myhostname);
		appendstring(cp, buffer);
		return;
	}

	if (!validate_nickname(arg) || !strcasecmp(arg, "conversd"))
		goto invalid_nick;

	if (cp->type != CT_HOST && check_nick_dupes(cp, arg)) {
		sprintf(buffer, ":%s 433 %s %s :Nickname already in use\n", myhostname, (*cp->nickname ? cp->nickname : "*"), arg);
		appendstring(cp, buffer);
		return;
	}

	if (cp->type == CT_UNKNOWN) {
		if (*cp->nickname) {
			// already have a nick. where does it come from? forced
			// by ax25 login? or does he try hacking accounts?
			// -> silently ignore this command (don't irritate him)
			return;
		}
	} else if (cp->type == CT_USER) {

		if (cp->observer)
			goto announce;
		if (check_user_banned(cp, "NICK"))
			return;

		if ((ret = check_cmd_flood(cp, cp->name, myhostname, SUL_NICK, 0, arg))) {
			if (ret > 0) {
announce:
				sprintf(buffer, ":%s 437 %s %s :Nick is temporarily unavailable\n", myhostname, cp->nickname, arg);
				appendstring(cp, buffer);
			}
			return;
		}

#ifdef	notdef	// old variant
		// store old nick for later reference (update_user_data())
		strncpy(oldnickname, cp->nickname, NAMESIZE);
		oldnickname[NAMESIZE] = 0;
#endif
		sprintf(buffer, ":%s!%s@%s NICK :%s\n", cp->nickname, cp->name, myhostname, arg);
		appendstring(cp, buffer);
		// announce to other users / servers;
		// will not send  NICK change message back to us.
		if (cp->chan_list)
			change_pers_and_nick(cp, cp->name, cp->host, 0, -1, arg, 1, 1);

	}

	strncpy(cp->nickname, arg, IRC_NAMESIZE);
	cp->nickname[IRC_NAMESIZE] = 0;
	cp->nickname_len = strlen(cp->nickname);

	// not signed on? -> no /nick change. learn nick as name
	if (cp->type == CT_UNKNOWN) {
		// wrong order NICK and USER message?
		int user_sent_before_nick = (*cp->name ? 1 : 0);
		cp->ircmode = 1;
		cp->width = DEFAULT_WIDTH_IRC;
		strncpy(cp->name, arg, IRC_NAMESIZE);
		cp->name[IRC_NAMESIZE] = 0;
		cp->name_len = strlen(cp->name);
//craiger		strlwr(cp->name);
//craiger		strlwr(cp->nickname);
		if (user_sent_before_nick) {
			// broken client: says USER x\nNICK y.
			// we've everything we need now, and do partA as normaly irc_user_command() would do
			user_login_common_partA(cp);
			// definitely point of no return
			return;
		}
	}
	return;

invalid_nick:
	sprintf(buffer, ":%s 432 %s %s :Erroneous nickname\n", myhostname, (*cp->nickname ? cp->nickname : "*"), arg);
	appendstring(cp, buffer);
}

/*---------------------------------------------------------------------------*/

void irc_pass_command(struct connection *cp)
{
	// force charset
	cp->charset_in = cp->charset_out = ISO_STRIPED;

	cp->ircmode = 1;
	auth_command(cp);
}

/*---------------------------------------------------------------------------*/

void irc_user_command(struct connection *cp)
{
	char buffer[2048];
	char *user, *mode, *pers;
	char *q;

	if (cp->type == CT_HOST) {
		// not implemented yet
		return;
	}

	// force charset
	cp->charset_in = cp->charset_out = ISO_STRIPED;

	// user - normaly ignore
	user = getarg(0, 0);
	// mode (rfc) / user's hostname (xchat)
	mode = getarg(0, 0);
	// skip (reserved field)
	getarg(0, 0);
	// get /personal info
	pers = getargcs(0, 1);

	// rfc: all 4 args must be present
	if (!*pers) {
		sprintf(buffer, ":%s 461 %s USER :Not enough parameters\n", myhostname, (*cp->nickname ? cp->nickname : "*"));
		appendstring(cp, buffer);
		return;
	}

	if (cp->type == CT_USER) {
		// already signed on
		sprintf(buffer, ":%s 462 %s :Unauthorized command (already registered)\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	if (*cp->nickname) {
		strncpy(cp->name, cp->nickname, IRC_NAMESIZE);
	} else {
		// broken client: there are irc-clients which send 1. USER 2. NICK instead
		// of normal order 1. NICK 2. USER
		strncpy(cp->name, user, IRC_NAMESIZE);
	}
	cp->name[IRC_NAMESIZE] = 0;
	cp->name_len = strlen(cp->name);

	if (isdigit(*mode & 0xff)) {
		int i = atoi(mode);
		if (i == 0) {
			// does not need an ack
		}
		if (i & 2) {
			cp->verbose = 1;
			sprintf(buffer, ":%s MODE %s +ws\n", myhostname, cp->name);
			appendstring(cp, buffer);

		}
		if (i & 3) {
			// +i not supporded be non-verbose. just don't ack
		}
		// rest: do'nt care
	}
	// some irc clients are broken; do not only check *pers == ':', but strchr(pers, ':')
	if ((q = strchr(pers, ':')))
		pers = q;
	if (*pers == ':')
		pers++;
	if (*pers) {
		strncpy(cp->pers, pers, PERSSIZE);
		cp->pers[sizeof(cp->pers)-1] = 0;
	} else {
		strcpy(cp->pers, "@");
	}

	cp->ircmode = 1;

	if (!*cp->nickname) {
		// not fully registered yet
		// it's rfc conform to be verbose. but most irc-servers
		// accept also USER / NICK instead of standard NICK / USER
		//sprintf(buffer, ":%s 451 * :You have not registered\n", myhostname);
		//appendstring(cp, buffer);
		return;
	}

	user_login_common_partA(cp);
}

/*---------------------------------------------------------------------------*/

void user_login_irc_partA(struct connection *cp) {

	char buffer[2048];
	char buffer2[2048];
	FILE * fd;

	cp->channel = -1;
	// force charset
	cp->charset_in = cp->charset_out = ISO_STRIPED;

	if (cp->verbose)
		cp->verbose = is_hushlogin(cp) ? 0 : 1;
	sprintf(buffer, ":%s 001 %s :Welcome to the %s Network %s\n", myhostname, cp->nickname, convcmd, cp->nickname);
	appendstring(cp, buffer);
	sprintf(buffer, ":%s 002 %s :Your host is %s, running version %s\n", myhostname, cp->nickname, myhostname, SOFT_RELEASE);
	appendstring(cp, buffer);
	if (!cp->verbose)
		goto skiped;
	// ctime sucks
	sprintf(buffer, ":%s 003 %s :This server was created %s %s\n", myhostname, cp->nickname, rip(ctime(&boottime)), mytimezone);
	appendstring(cp, buffer);

	irc_lusers_command(cp);

	/* print motd */
	if (!access(motdfile, R_OK) && (fd = fopen(motdfile, "r"))) {
		sprintf(buffer, ":%s 375 %s :- %s Message of the Day -\n", myhostname, cp->nickname, myhostname);
		appendstring(cp, buffer);

		sprintf(buffer2, ":%s 372 %s:- ", myhostname, cp->nickname);
		while (fgets(buffer, sizeof(buffer), fd)) {
			appendstring(cp, buffer2);
			rip(buffer);
			appendstring(cp, buffer);
			appendstring(cp, "\n");
		}
		fclose(fd);
		sprintf(buffer, ":%s 376 %s :End of /MOTD command.\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
	}
skiped:

	cp->mtime = currtime;

	user_login_common_partB(cp);

	update_hushlogin(cp, !cp->verbose);
	cp->verbose = 0;

	strcpy(cp->ibuf, "/profile run");
	profile_change(cp);

	// overwrite cp->width in any case
	cp->width = DEFAULT_WIDTH_IRC;
	// force charset
	cp->charset_in = cp->charset_out = ISO_STRIPED;
}

/*---------------------------------------------------------------------------*/

void irc_privmsg_command(struct connection *cp)
{
	char buffer[2048];

	if (cp->type == CT_HOST) {
		// not implemented yet
		return;
	}

	if (cp->type == CT_OBSERVER || cp->observer) {
		sprintf(buffer, ":%s 404 %s * :Cannot send to channel / nick. You are observer only.\n", myhostname, cp->name);
		appendstring(cp, buffer);
		return;
	}

	if (cp->away && *cp->away) {
		sprintf(buffer, ":%s 301 %s %s :You are away, aren't you ? :-)\n", myhostname, cp->nickname, cp->nickname);
		appendstring(cp, buffer);
	}

	msg_command(cp);
	return;
}

/*---------------------------------------------------------------------------*/

void irc_part_command(struct connection *cp)
{
	leave_command(cp);
	return;
}

/*---------------------------------------------------------------------------*/

void irc_list_command(struct connection *cp)
{
	char buffer[2048];
	struct channel *ch;
	int invisible;
	int secret;
	int special;
	int showit;
	int isonchan;
	int selchannel;
	struct connection *p;
	struct clist *cl;
	int n;
	char *arg, *arg2;
	int min;
	int max;
	int chan;
	int first_run = 1;
	int state;

	arg = getarg(0, 0);
	while ((arg2 =strsep(&arg,","))) {
		selchannel = 0;
		min = 0;
		max = 1024;
		chan = 0;
		if (*arg2 == '#') {
			arg2++;
			if (*arg2) {
				if (!stringisnumber(arg2) || (chan = atoi(arg2)) < 0)
					continue;
				selchannel = 1;
			}
		}
		else if (*arg2 == '<') {
			arg2++;
			if (*arg2) {
				max = atoi(arg2);
			}
		}
		else if (*arg2 == '>') {
			arg2++;
			if (*arg2) {
				min = atoi(arg2);
			}
		}
	
		if (first_run) {
			sprintf(buffer, ":%s 321 %s Channel :Users  Name\n",
				  	cp->host,
				  	cp->nickname ? cp->nickname : cp->name);
			appendstring(cp, buffer);
			first_run = 0;
		}
	
		for (state = 0; state < 2; state++) {
		for (ch = channels; ch; ch = ch->next) {
			if (ch->expires != 0L)
				continue;
			isonchan = 0;
			secret = 0;
			special = 0;
			invisible = 0;
			for (cl = cp->chan_list; cl; cl = cl->next) {
				if (cl->channel == ch->chan) {
					isonchan++;
				}
			}
			if (ch->flags & M_CHAN_S) {
				special++;
				secret++;
			}
			if (ch->flags & M_CHAN_I) {
				invisible++;
				special++;
			}
			showit = 0;
			if ((ch->chan == cp->channel) || isonchan || cp->operator == 2 || (!special || !invisible)) {
				if (secret && !((ch->chan == cp->channel) || isonchan || cp->operator == 2))
					showit = (state ? 1 : 0);
				else
					showit = state ? 0 : 1;
			} 

			if (selchannel && ch->chan != chan) {
				showit=0;
			}
			n = 0;
			if (showit) {
				for (p = connections; p; p = p->next) {
					if (p->type == CT_USER) {
						if (p->channel == ch->chan)
							n++;
						else {
							for (cl = p->chan_list; cl; cl = cl->next)
								if (cl->channel == ch->chan)
									break;
							if (cl)
								n++;
						}
					}
				}
				
				if (n > min && n < max) {
					if (special && !isonchan && cp->operator != 2)
						sprintf(buffer, ":%s 322 %s * %d :%s",
							  	cp->host,
							  	cp->nickname ? cp->nickname : cp->name, n, 
									ch->topic);
					else
						sprintf(buffer, ":%s 322 %s #%d %d :%s",
							  	cp->host,
							  	cp->nickname ? cp->nickname : cp->name, ch->chan, n, 
									ch->topic);
					buffer[IRC_MAX_MSGSIZE] = 0;
					appendstring(cp, buffer);
					appendstring(cp, "\n");
				}
			}
		}
		}
	}
	sprintf(buffer, ":%s 323 %s :End of /LIST\n",
		  	cp->host,
		  	cp->nickname ? cp->nickname : cp->name);
	appendstring(cp,buffer);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void irc_names_command(struct connection *cp)
{
	char buffer[2048];
	char flags[16];
	struct channel *ch;
	int invisible;
	int secret;
	int special;
	int showit;
	int isonchan;
	int selchannel;
	struct connection *p, *p_next;
	struct clist *cl, *cl2;
	char *arg, *arg2;
	int chan;
	int state;

	arg = getarg(0, 0);
	while ((arg2=strsep(&arg,","))) {
		selchannel = 0;
		chan = 0;
		if (*arg2 == '#') {
			arg2++;
			if (*arg2) {
				chan = atoi(arg2);
				if (!stringisnumber(arg2) || (chan = atoi(arg2)) < 0)
					goto end;
				selchannel = 1;
			}
		}
	
		for (state = 0; state < 2; state++) {
		for (ch = channels; ch; ch = ch->next) {
			if (ch->expires != 0L)
				continue;
			flags[0] = '\0';
			isonchan = 0;
			secret = 0;
			special = 0;
			invisible = 0;
			for (cl = cp->chan_list; cl; cl = cl->next) {
				if (cl->channel == ch->chan) {
					isonchan++;
				}
			}
			// on irc, secret and private channel are exclusive
			if (ch->flags & M_CHAN_I) {
				strcpy(flags, "@");
				invisible++;
				special++;
			}
			if (ch->flags & M_CHAN_S) {
				if (!*flags)
					strcpy(flags, "*");
				special++;
				secret++;
			}
			if (!*flags)
				strcpy(flags, "=");
			showit = 0;
			if ((ch->chan == cp->channel) || isonchan || cp->operator == 2|| (!special || !invisible)) {
				if (secret && !((ch->chan == cp->channel) || isonchan || cp->operator == 2))
					showit = (state ? 1 : 0);
				else
					showit = state ? 0 : 1;
			} 
			if (selchannel && ch->chan != chan) {
				showit=0;
			}
			if (showit) {
				if (special && !isonchan && cp->operator != 2)
					sprintf(buffer,":%s 353 %s %s * :",
						  	myhostname,
						  	cp->nickname, flags); 
				else
					sprintf(buffer,":%s 353 %s %s #%d :",
						  myhostname,
						  cp->nickname, flags, ch->chan); 
				appendstring(cp,buffer);
				for (p = sort_connections(1); p; p = sort_connections(0)) {
					if (p->type == CT_USER) {
						int chop = 0;
						if (p->channel == ch->chan) {
							chop = p->channelop;
						} else if (p->channel != ch->chan) {
							for (cl2 = p->chan_list; cl2; cl2 = cl2->next) {
								if (cl2->channel == ch->chan) {
									chop = cl2->channelop;
									break;
								}
							}
							// found?
							if (!cl2)
								continue;
						}
						sprintf(buffer, "%s%s ", (chop ? "@" : ""), p->nickname);
						appendstring(cp,buffer);
					}
				}
				appendstring(cp,"\n");
			}
		}
		}
end:
		if (selchannel && *arg2) {
			sprintf(buffer, ":%s 366 %s #%s :End of /NAMES list.\n",
					  	myhostname,
					  	cp->nickname, arg2);
		}
		else {
			int found = 0;
			// users not on any channel (local irc users only)
			for (p = sort_connections(1); p; p = p_next) {
				p_next = sort_connections(0);
				if (p->type != CT_USER || p->channel >= 0)
					continue;
				if (p_next && !strcasecmp(p->nickname, p_next->nickname))
					continue;
				if (!found) {
					sprintf(buffer,":%s 353 %s * * :",
						myhostname,
						cp->nickname); 
					appendstring(cp, buffer);
					found = 1;
				}
				sprintf(buffer, "%s ", p->nickname);
				appendstring(cp, buffer);
			}
			if (found)
				appendstring(cp, "\n");
			sprintf(buffer, ":%s 366 %s * :End of /NAMES list.\n",
					  	myhostname,
					  	cp->nickname);
		}
	
		appendstring(cp,buffer);
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void irc_who_command(struct connection *cp)
{
	char buffer[2048];
	char flags[16];
	struct channel *ch;
	int invisible;
	int secret;
	int special;
	int showit;
	int isonchan;
	int selchannel = 0;
	struct connection *p;
	struct destination *d;
	struct clist *cl, *cl2;
	char *arg;
	char *q;
	int chan = 0;
	int only_operators = 0;
	int match;
	char arg_orig[512];
	int hops;
	int state;

	arg = getarg(0, 0);
	strncpy(arg_orig, arg, sizeof(arg_orig));
	arg_orig[sizeof(arg_orig)-1] = 0;

	if (*arg == '#') {
		arg++;
		if (*arg) {
			chan = atoi(arg);
			if (chan < 0)
				goto end;
			selchannel = 1;
		}
	}  else if (*arg == '~')
		arg++;
	if (*arg) {
		q = getarg(0, 0);
		if (*q && !strcmp(q, "o"))
			only_operators = 1;
	}
	// simple pattern matching:
	match = 1; // strcasecmp() - default
	if (*arg && (q = strrchr(arg, '*')) && q > arg) {
		*q = 0;
		match = 2; // strncasecmp()
	}
	if (!*arg || !strcmp(arg, "*"))
		match = 0; // any
	else if (*arg == '*') {
		match = 3; // strstr()
		arg++;
	}

	for (state = 0; state < 2; state++) {
	for (ch = channels; ch; ch = ch->next) {
		if (ch->expires != 0)
			continue;
		flags[0] = '\0';
		isonchan = 0;
		secret = 0;
		special = 0;
		invisible = 0;

		if (selchannel && (ch->chan != chan))
			continue;

		for (cl = cp->chan_list; cl; cl = cl->next) {
			if (cl->channel == ch->chan) {
				isonchan++;
			}
		}
		// on irc, secret and invisible (private) channel are exclusive
		if (ch->flags & M_CHAN_S) {
			special++;
			secret++;
		}
		if (ch->flags & M_CHAN_I) {
			invisible++;
			special++;
		}
		showit = 0;
		if ((ch->chan == cp->channel) || isonchan || cp->operator == 2 || (!special || !invisible)) {
			if (secret && !((ch->chan == cp->channel) || isonchan || cp->operator == 2))
				showit = (state ? 1 : 0);
			else
				showit = state ? 0 : 1;
		}
		if (selchannel && ch->chan != chan) {
			showit=0;
		}
		if (showit) {
			for (p = sort_connections(1); p; p = sort_connections(0)) {
				if (p->type == CT_USER) {
					int chop = 0;
					if (only_operators && !p->operator)
						continue;
					if (!selchannel) {
						switch (match) {
						case 1:
							if (strcasecmp(p->host, arg) && strcasecmp(p->name, arg) && strcasecmp(p->nickname, arg) && strcasecmp((*p->pers == '~' ? (p->pers + 1) : p->pers), arg))
								continue;
							break;
						case 2:
							if (strncasecmp(p->host, arg, strlen(arg)) && strncasecmp(p->name, arg, strlen(arg)) && strncasecmp(p->nickname, arg, strlen(arg)) && strncasecmp((*p->pers == '~' ? (p->pers + 1) : p->pers), arg, strlen(arg)))
								continue;
							break;
						case 3:
							if (!strstr(p->host, arg) && !strstr(p->name, arg) && !strstr(p->nickname, arg) && !strstr((*p->pers == '~' ? (p->pers + 1) : p->pers), arg))
								continue;
							break;
						// default:	//show
						}
					}
					// find status
					if (p->channel == ch->chan)
						chop = p->channelop;
					else if (p->channel != ch->chan) {
						for (cl2 = p->chan_list; cl2; cl2 = cl2->next) {
							if (cl2->channel == ch->chan) {
								chop = cl2->channelop;
								break;
							}
						}
						// found?
						if (!cl2)
							continue;
					}
					if (!p->via) {
						// local user
						hops = 0;
					} else {
						if ((d = find_destination(p->host))) {
							hops = d->hops;
						} else {
							hops = 2; // at least beyond our permlink
						}
					}
					if (special && !isonchan && cp->operator != 2)
						sprintf(buffer,":%s 352 %s * %s%s %s %s %s %s%s%s :%d %s%s", 
					        myhostname, cp->nickname, /* (p->isauth > 1) ? "" : "~" */ "" , p->name, p->host, p->host, p->nickname, (p->away && *p->away ? "G" : "H"), (p->operator ? "*" : ""), (chop ? "@" : ""),  hops, ((*p->pers != '@' && strcmp(p->pers, "~")) ? (*p->pers == '~' ? (p->pers + 1) : p->pers) : "No Info"), (*p->pers == '~' ? " [NonAuth]" : ""));
					else
						sprintf(buffer,":%s 352 %s #%d %s%s %s %s %s %s%s%s :%d %s%s", 
					        myhostname, cp->nickname, ch->chan, /* (p->isauth > 1) ? "" : "~" */ "" , p->name, p->host, p->host, p->nickname, (p->away && *p->away ? "G" : "H"), (p->operator ? "*" : ""), (chop ? "@" : ""),  hops, ((*p->pers != '@' && strcmp(p->pers, "~")) ? (*p->pers == '~' ? (p->pers + 1) : p->pers) : "No Info"), (*p->pers == '~' ? " [NonAuth]" : ""));
					buffer[IRC_MAX_MSGSIZE] = 0;
					appendstring(cp, buffer);
					appendstring(cp, "\n");
				}
			}
		}
	}
	}
end:
	sprintf(buffer, ":%s 315 %s %s :End of /WHO list.\n",
		  myhostname,
		  cp->nickname,
		  arg_orig);

	appendstring(cp,buffer);

	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void irc_links_command(struct connection *cp)
{
	char buffer[2048];
	struct destination *d;
	char *dest = 0;

	if (cp->type == CT_USER)
		dest = getargcs(0, 0);
	else
		dest = "";

	if (*dest == '\0') {
		for (d = destinations; d; d = d->next) {
			if (d->rtt) {
				sprintf(buffer, ":%s 364 %s %s %s :%d %s\n", 
				        cp->host,
				        cp->nickname ? cp->nickname : cp->name,
				        d->name,
				        d->link->name ? d->link->name : myhostname, 
								d->hops, d->rev);
				appendstring(cp, buffer);
			}
		}
	}
	sprintf(buffer, ":%s 364 %s %s %s :0 %s\n", 
				  cp->host,
				  cp->nickname ? cp->nickname : cp->name,
				  cp->host,
				  cp->host,
				  SOFT_RELEASE);
	appendstring(cp, buffer);
	sprintf(buffer, ":%s 365 %s * :End of /LINKS list.\n",
				  cp->host,
				  cp->nickname ? cp->nickname : cp->name);
	appendstring(cp, buffer);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void irc_notice_command(struct connection *cp)
{
	// client -> server notices are silently discarded
	// client notice messages to a channel or user should not be
	// responded by error messages
	if (cp->type == CT_OBSERVER || cp->observer)
		return;
	notice_command(cp);
	return;
}

/*---------------------------------------------------------------------------*/

void irc_ping_command(struct connection *cp)
{
	char *arg;
	char buffer[2048];

	arg = getargcs(0, 0);
	arg[cp->width] = 0;

	sprintf(buffer, ":%s PONG %s :%s\n", myhostname, myhostname, arg);
	appendstring(cp, buffer);

}

/*---------------------------------------------------------------------------*/

void irc_pong_command(struct connection *cp)
{
	rtt_command(cp);
}

/*---------------------------------------------------------------------------*/

void irc_operator_command(struct connection *cp)
{

	char buffer[2048];

	sprintf(buffer, ":%s 421 %s OPER :Unknown command. Try /sysop\n",
		myhostname, cp->nickname);
	appendstring(cp,buffer);
}

/*---------------------------------------------------------------------------*/

void irc_ison_command(struct connection *cp)
{

	struct connection *p, *p_next;
	char *user;
	char buffer[2048];
	struct clist *cl;

	user = getarg(0, 0);
	if (!*user) {
		sprintf(buffer, ":%s 461 %s ISON :Not enough parameters\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
	} else {
		sprintf(buffer, ":%s 303 %s :", myhostname, cp->nickname);
		appendstring(cp, buffer);
		do {
			for (p = sort_connections(1); p; p = p_next) {
				p_next = sort_connections(0);
				if (p_next && !strcasecmp(p->name, p_next->name)) // name, not nickname, because it's the indicator in ampr convers
					continue;
				if (p->type == CT_USER) {
					int chan;
					switch (*user) {
					case '#':
						if (!stringisnumber(user+1) || (chan = atoi(user+1)) < 0)
							continue;
						if (p->channel != chan) {
							for (cl = p->chan_list; cl; cl = cl->next) {
								if (cl->channel == chan)
									break;
							}
							if (!cl)
								continue;
						}
						sprintf(buffer, "%s ", p->nickname);
						appendstring(cp, buffer);
						break;
					case '@':
						if (!*(user+1))
							continue;
						if (!strcasecmp(p->host, user+1)) {
							sprintf(buffer, "%s ", p->nickname);
							appendstring(cp, buffer);
						}
						break;
					default:
						if (!strcasecmp(p->name, user) || !strcasecmp(p->nickname, user)) {
							if (!strcasecmp(p->name, p->nickname))
								sprintf(buffer, "%s ", p->nickname);
							else
								sprintf(buffer, "%s %s ", p->nickname, p->name);
							appendstring(cp, buffer);
						}
					}
				}
			}
			user = getarg(0, 0);
		} while (*user);
		appendstring(cp, "\n");
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void irc_lusers_command(struct connection *cp)
{
	char buffer[2048];
	struct destination *d;
	struct channel *ch;
	struct connection *p, *p_next;
	int n_users = 0;
	int n_dest, n_channels, n_clients, n_servers, n_typeunknown, n_operators;

	for (n_dest = 0, d = destinations; d; d = d->next) {
		if (d->rtt && d->link)
			n_dest++;
	}
	for (n_channels = 0, ch = channels; ch; ch = ch->next) {
		if (ch->expires != 0L)
			continue;
		if (!(ch->flags & M_CHAN_I))
			n_channels++; 
	}
	for (n_clients = n_servers = n_typeunknown = n_operators = 0, p = sort_connections(1); p; p = p_next) {
		p_next = sort_connections(0);
		switch (p->type) {
			case CT_HOST:
			n_servers++;
			break;
		case CT_USER:
			if (p_next && !strcasecmp(p_next->name, p->name) && !strcasecmp(p_next->host, p->host))
				break;
			n_users++;
			if (!p->via)
				n_clients++;
			if (p->operator)
				n_operators++;
			break;
		case CT_UNKNOWN:
		default:
			n_typeunknown++;
		}
	}

	sprintf(buffer, ":%s 251 %s :There %s %d user%s on %d server%s\n", myhostname, cp->nickname, (n_users == 1) ? "is" : "are", n_users, (n_users != 1) ? "s" : "", (n_dest) ? n_dest+1 : 1, (n_dest) ? "s" : "");
	appendstring(cp, buffer);
	
	if (n_operators) {
		sprintf(buffer, ":%s 252 %s %d :operator%s online\n", myhostname, cp->nickname, n_operators, (n_operators != 1) ? "s" : "");
		appendstring(cp, buffer);
	}
	if (n_typeunknown) {
		sprintf(buffer, ":%s 253 %s %d :unknown connection%s\n", myhostname, cp->nickname, n_typeunknown, (n_typeunknown != 1) ? "s" : "");
		appendstring(cp, buffer);
	}
	sprintf(buffer, ":%s 254 %s %d :channel%s formed\n", myhostname, cp->nickname, n_channels, n_channels != 1 ? "s" : "");
	appendstring(cp, buffer);
	sprintf(buffer, ":%s 255 %s :I have %d client%s and %d server%s\n", myhostname, cp->nickname, n_clients, (n_clients != 1) ? "s" : "", n_servers, n_servers != 1 ? "s" : "");

	appendstring(cp, buffer);
}

/*---------------------------------------------------------------------------*/

void irc_whois_command(struct connection *cp)
{
	char *server, *users;
	char buffer[2048];
	char pat[512];
	char *pattern;
	char *d_revision;
	struct connection *p, *c, *last_p;
  	struct clist *cl, *mycl;
	struct channel *ch;
	struct destination *d;
	int showit;
	int found;
	int channel_found = 0;

  	server = getarg(0, 0);
  	found = 0;

	if (*server == ':')
		server++;
	if (*server == '~')
		server++;
	users = getarg(0, 1);
	if (*users == '~')
		users++;
	if (!server || !*server) {
		sprintf(buffer, ":%s 431 %s WHOIS :No nickname given\n",
			myhostname, cp->nickname);
		appendstring(cp,buffer);
		return;
	}

	if (users || !*users) {
		users=server;
	}
	strncpy(pat, users, sizeof(pat));
	pat[sizeof(pat)-1] = 0;
	
	while ((pattern=strsep(&users,","))) {
		if (*pattern == '~')
			pattern++;
		if (!*pattern)
			continue;
		last_p = 0;
		for (p=sort_connections(1); p; p = sort_connections(0)) {
			if (last_p && !strcasecmp(p->name, last_p->name) && !strcmp(p->host, last_p->host)) {
				// already announced
				continue;
			}
			if (p->type == CT_USER) {
				if (!fnmatch(pattern, p->name, FNM_CASEFOLD) || !fnmatch(pattern, p->nickname, FNM_CASEFOLD)) {
					found = 1;
					sprintf(buffer, ":%s 311 %s %s %s%s %s * :%s%s",
					        myhostname, cp->nickname,
						p->nickname, (p->isauth > 1) ? "" : "~", p->name, p->host, 
					        (*p->pers && *p->pers != '@' && strcmp(p->pers, "~")) ? (*p->pers == '~' ? (p->pers + 1) : p->pers) : "No Info", (*p->pers == '~' ? " [NonAuth]" : ""));
					buffer[IRC_MAX_MSGSIZE] = 0;
					appendstring(cp,buffer);
					appendstring(cp, "\n");
					if (p->operator) {
						sprintf(buffer, ":%s 313 %s %s :is an IRC operator\n",
					        	myhostname, cp->nickname, p->nickname);
						appendstring(cp,buffer);
					}

					channel_found = 0;
					for (c=connections; c; c = c->next) {
						// Testen der channel-modes und connection-type (!Host)
						if (c->type != CT_USER)
							continue;
						if (strcmp(p->name,c->name) || strcmp(p->host, c->host))
							continue;
						
						if (c->via) {
							// check his main channel
							for (ch = channels; ch; ch = ch->next)
								if (ch->chan == c->channel)
									break;
							// found. how are the modes?
							showit = 0;
							if (ch) {
								if (cp->operator != 2 && ((ch->flags & M_CHAN_S) || (ch->flags & M_CHAN_I))) {
									// am i on?
									if (cp->channel == ch->chan)
										showit = 1;
									else {
										for (mycl = cp->chan_list; mycl; mycl = mycl->next) {
											if (mycl->channel == c->channel) {
												showit = 1;
												break;
											}
										}
									}
								} else {
									showit = 1;
								}
							}
							if (showit) {
								if (!channel_found) {
									sprintf(buffer, ":%s 319 %s %s :", myhostname, cp->nickname, p->nickname);
									appendstring(cp, buffer);
									channel_found++;
								}
								// ok, that was quite easy..
								sprintf(buffer,"%s#%d ", c->channelop ? "@" : "", c->channel);
								appendstring(cp, buffer);
							}
						} else {
							// now check his other channels (against all channels i'm on..)
							for (cl = c->chan_list; cl; cl = cl->next) {
								showit = 0;
								// check his main channel
								for (ch = channels; ch; ch = ch->next)
									if (ch->chan == cl->channel)
										break;
								if (!ch)
									continue; // should never happen
								// found. how are the modes?
								if (cp->operator != 2 && ((ch->flags & M_CHAN_S) || (ch->flags & M_CHAN_I))) {
									if (cl->channel == cp->channel)
										showit = 1;
									else {
										for (mycl = cp->chan_list; mycl; mycl = mycl->next) {
											if (mycl->channel == cl->channel) {
												showit = 1;
												break;
											}
										}
									}
								} else {
									showit = 1;
								}
								if (showit) {
									if (!channel_found) {
										sprintf(buffer, ":%s 319 %s %s :", myhostname, cp->nickname, p->nickname);
										appendstring(cp, buffer);
										channel_found++;
									}
									sprintf(buffer,"%s#%d ", cl->channelop ? "@" : "", cl->channel);
									appendstring(cp, buffer);
								}
							}
							// puh.. this was a bit more complex ;)
						}
					}
					if (channel_found)
						appendstring(cp, "\n");

					d_revision = "";
					if (!strcasecmp(p->host, myhostname)) {
						d_revision = SOFT_RELEASE;
					} else {
						for (d = destinations; d; d = d->next)
							if (!strcasecmp(d->name, p->host))
								break;
						d_revision = (d) ? d->rev :  "";
					}
					if (*d_revision) {
						sprintf(buffer, ":%s 312 %s %s %s :%s\n",
				        		myhostname, cp->nickname,
							p->nickname, p->host, d_revision);
						appendstring(cp,buffer);
					}
					sprintf(buffer, ":%s 317 %s %s %ld %ld :seconds idle, signon time\n",
					        myhostname, cp->nickname,
						p->nickname, (currtime - p->mtime), p->time);
					appendstring(cp,buffer);
				}
				last_p = p;
			}
		}
	}
	if (!found) {
		sprintf(buffer, ":%s 401 %s %s :No such nick\n",
		        myhostname, cp->nickname, pat);
		appendstring(cp,buffer);
	}
	sprintf(buffer, ":%s 318 %s %s :End of /WHOIS list.\n",
	        myhostname, cp->nickname, pat);

	appendstring(cp,buffer);
	return;
}

/*---------------------------------------------------------------------------*/

void irc_userhost_command(struct connection *cp)
{
	char buffer[2048];
	char buffer2[2048];
	int found = 0;
	struct connection *p;
	char *arg;


	arg = getarg(0, 0);

	if (!*arg) {
		sprintf(buffer, ":%s 461 %s USERHOST :Not enough parameters\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	sprintf(buffer2, ":%s 302 %s :", myhostname, cp->nickname);
	while (*arg) {
		// first, find ourself if requested (ampr nicknames are non-unique)
		p = 0;
		if (!strcasecmp(cp->nickname, arg))
			p = cp;
		else {
			for (p = connections; p; p = p->next) {
				if (!strcasecmp(p->nickname, arg))
					break;
			}
		}
		if (!p) {
			sprintf(buffer, ":%s 401 %s %s :No such nick\n",
		        	myhostname, cp->nickname, arg);
			appendstring(cp,buffer);
		} else {
			sprintf(buffer, "%s%s%s=%c%s%s@%s", (found) ? " " : "", p->nickname, (p->operator ? "*" : ""), (*p->away ? '-' : '+'), (p->isauth > 1) ? "" : "~", p->name, p->host);
			if (strlen(buffer) + strlen(buffer2) > IRC_MAX_MSGSIZE /* sizeof(buffer2) */ )
				break;
			strcat(buffer2, buffer);
			if (!found)
				found++;
		}

		arg = getarg(0, 0);
	}

	if (found) {
		appendstring(cp, buffer2);
		appendstring(cp, "\n");
	}
}

/*---------------------------------------------------------------------------*/

int validate_nickname(char *s)
{

	if (!s || !*s)
		return 0;
	for (; *s; s++) {
		int c = *s & 0xff;
		if (c >= 'A' && c <= 'Z')
			continue;
		if (c >= 'a' && c <= 'z')
			continue;
		if (isdigit(c))
			continue;
		switch (c) {
		case '[':
		case ']':
		case '\\':
		case '`':
		case '_':
		case '^':
		case '{':
		case '|':
		case '}':
		case '-':
			continue;
		default:
			return 0;
		}
	}
	return 1;
}

/*---------------------------------------------------------------------------*/

void irc_users_command(struct connection *cp)
{
	char buffer[2048];

	sprintf(buffer, ":%s 421 %s USERS :Unknown command.\n", myhostname, cp->nickname);
	appendstring(cp,buffer);
}

/*---------------------------------------------------------------------------*/

void irc_connect_command(struct connection *cp)
{
	char buffer[2048];
	char *arg;

	arg = getargcs(0, 1);

	if (!*arg) {
		sprintf(buffer, ":%s 461 %s CONNECT :Not enough parameters\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	if (cp->operator != 2) {
		sprintf(buffer, ":%s 481 %s :Permission Denied- You're not an IRC operator\n", myhostname, cp->name);
		appendstring(cp, buffer);
		return;
	}

	sprintf(buffer, "/link add %s", arg);
	getarg(buffer, 0);

	links_command(cp);
}

/*---------------------------------------------------------------------------*/

void irc_admin_command(struct connection *cp)
{

	char buffer[2048];

	sprintf(buffer, ":%s 256 %s %s :Administrative info\n", myhostname, cp->nickname, myhostname);
	appendstring(cp, buffer);
	if (mysysinfo && *mysysinfo) {
		sprintf(buffer, ":%s 257 %s :%s\n", myhostname, cp->nickname, mysysinfo);
		appendstring(cp, buffer);
	}
	sprintf(buffer, ":%s 259 %s :%s\n", myhostname, cp->nickname, (myemailaddr && *myemailaddr) ? myemailaddr : "[email not configured]");
	appendstring(cp, buffer);
}

/*---------------------------------------------------------------------------*/

void irc_squit_command(struct connection *cp)
{
	char buffer[2048];
	char *arg;

	arg = getargcs(0, 1);

	if (!*arg) {
		sprintf(buffer, ":%s 461 %s SQUIT :Not enough parameters\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	if (cp->operator != 2) {
		sprintf(buffer, ":%s 481 %s :Permission Denied- You're not an IRC operator\n", myhostname, cp->name);
		appendstring(cp, buffer);
		return;
	}

	sprintf(buffer, "/link reset %s", arg);
	getarg(buffer, 0);

	links_command(cp);
}

/*---------------------------------------------------------------------------*/

void irc_clink_command(struct connection *cp)
{
	// links command for irc has a complete other meaning. that's why
	// i choose the clink command name
	links_command(cp);
}

/*---------------------------------------------------------------------------*/

void irc_cnotify_command(struct connection *cp)
{
	// notify is a reserved command, in the irc client program
	// so we need cnotify command if the user likes to change the list
	notify_command(cp);
}

/*---------------------------------------------------------------------------*/

int check_nick_dupes(struct connection *cp, char *nick)
{

	struct connection *p;

	if (!strcasecmp(nick, cp->name))
		return 0;

	for (p = connections; p; p = p->next) {
		if (p->type != CT_USER || p == cp)
			continue;
		if (!strcasecmp(p->name, cp->name))
			continue;
		if (!strcasecmp(p->nickname, nick) || !strcasecmp(p->name, nick))
			return 1;
	}
	return 0;
}

/*---------------------------------------------------------------------------*/

char *get_mode_flags2irc(int flags)
{
	static char mode[16];
	char *p = mode;
	p = mode;
	if (flags & M_CHAN_I) {
		*p++ = 's';
	}
	if (flags & M_CHAN_M) {
		*p++ = 'm';
	}
	if (flags & M_CHAN_T) {
		*p++ = 't';
	}
	if (flags & M_CHAN_P) {
		*p++ = 'i';
	}
	if (flags & M_CHAN_S) {
		*p++ = 'p';
	}
	// always +n
	*p++ = 'n';
	//if (flags & M_CHAN_L) {
		//*p++ = 'l';
		//*p++ = ' ';
	//}
	*p = 0;

	return mode;
}

