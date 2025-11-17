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
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "conversd.h"
#include "user.h"
#include "irc.h"
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

extern long seed;
extern struct langs languages[];

extern struct cmdtable cmdtable[];
static int breakins;

/*---------------------------------------------------------------------------*/

int stringisnumber(char *p)
{
	if (!p)
		return 0;
	while(*p && isspace(*p & 0xff))
		p++;
	if (*p == '-')
		p++;
	if (!*p || *p < '0' || *p > '9')
		return 0;
	for (++p; *p && *p >= '0' && *p <= '9'; p++) ;
	if (!*p || isspace(*p & 0xff))
		return 1;
	return 0;
}

/*---------------------------------------------------------------------------*/

void all_command(struct connection *cp)
{
	struct channel *ch;
	char *s;
	char buf[(NAMESIZE * 2) + 3];

	s = getarg(0, 1);

	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == cp->channel)
			break;
	}
	if (!ch)
		return; // ircmode?, maybe /all in user's profile - silently discard
	if (cp->operator == 2 || cp->channelop || !(ch->flags & M_CHAN_M)) {
		if (*s) {
			if (check_user_banned(cp, "ALL"))
				return;
			getTXname(cp, buf);
			send_msg_to_channel(buf, cp->channel, s);
		}
	} else {
		if (!cp->ircmode) {
			append_general_notice(cp, "*** This is a moderated channel. Only channel operators may write.\n");
		}
		else {
			sprintf(buf, ":%s 404 %s #%d :Cannot send to channel\n",cp->host,cp->nickname ? cp->nickname : cp->name, cp->channel);
			appendstring(cp, buf);
		}
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void away_command(struct connection *cp)
{
	char *s;
	char buffer[2048];
	struct connection *p;
	int status_changed = 0;

	s = getarg(0, 1);
	if (cp->ircmode && *s == ':')
		s++;
	if (*s) {
		if (check_cmd_flood(cp, cp->name, myhostname, SUL_AWAY, 1, s))
			return;
		if (strlen(s) > AWAYSIZE)
			s[AWAYSIZE] = 0;
		for (p = connections; p; p = p->next) {
			if (p->type == CT_USER) {
				if (p->via)
					continue;
				if (!strcasecmp(p->name, cp->name) && !strcasecmp(p->host, cp->host)) {
					if (strcmp(p->away, s))
						status_changed = 1;
					strncpy(p->away, s, AWAYSIZE);
					p->away[sizeof(p->away)-1] = 0;
					p->atime = currtime;
					if (p->ircmode)
						sprintf(buffer, ":%s 306 %s :You have been marked as being away\n",cp->host,cp->nickname);
					else
						sprintf(buffer, "*** (%s) You are marked as being away.\n", ts2(currtime));
					appendstring(p, buffer);
					p->locked = 1;
				}
			}
		}
		// irc ircuser and not on any channel, do not announce. will be done later
		if (status_changed && (!cp->ircmode || cp->chan_list))
			send_awaymsg(cp->name, cp->nickname, myhostname, currtime, s, status_changed);
	} else {
		if (*cp->away) {
			if (check_user_banned(cp, "AWAY"))
				return;
			if (check_cmd_flood(cp, cp->name, myhostname, SUL_AWAY, 1, s))
				return;
			for (p = connections; p; p = p->next) {
				if (p->type == CT_USER) {
					if (!strcasecmp(p->name, cp->name) && !strcasecmp(p->host, cp->host)) {
						*p->away = '\0';
						p->atime = currtime;
						p->mtime = currtime;
						if (p->ircmode)
							sprintf(buffer, ":%s 305 %s :You are no longer marked as being away\n",cp->host,cp->nickname ? cp->nickname : cp->name);
						else
							sprintf(buffer, "*** (%s) You are no longer marked as being away.\n", ts2(currtime));
						appendstring(p, buffer);
						p->locked = 1;
					}
				}
			}
			if (!cp->ircmode || cp->chan_list)
				send_awaymsg(cp->name, cp->nickname, myhostname, currtime, s, 1);
		} else {
			if (!cp->ircmode) {
				sprintf(buffer, "*** (%s) Actually you were marked as being here :-)\n", ts2(currtime));
			}
			else {
				sprintf(buffer, ":%s 305 %s Actually you were marked as being here :-)\n", myhostname, cp->nickname);
			}
			appendstring(cp, buffer);
		}
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void beep_command(struct connection *cp)
{
	if (cp->prompt[3] == '\0')
		cp->prompt[3] = '\007';
	else
		cp->prompt[3] = '\0';
	if (cp->prompt[3] == '\007') {
		append_general_notice(cp, "*** Beep mode enabled.\n");
		sprintf(cp->ibuf, "/profile add beep");
	} else {
		append_general_notice(cp, "*** Beep mode disabled.\n");
		sprintf(cp->ibuf, "/profile del beep");
	}
	profile_change(cp);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void update_channel_history(struct connection *p_via, int channel, char *name, char *host)
{
	char buffer[2048];
	struct connection *p;
	struct clist *cl;
	struct channel *ch;
	int found = 0;
	int look_for_via_other_link = 0;

	for (ch = channels; ch; ch = ch->next)
		if (ch->chan == channel)
			break;
	if (!ch)
		return;
	for (p = connections; p; p = p->next) {
		if (p->type != CT_USER)
			continue;
		if (!p->via) {
			for (cl = p->chan_list; cl; cl = cl->next) {
				if (cl->channel == ch->chan) {
					found = 1;
					break;
				}
			}
		} else {
			if (p->channel == ch->chan) {
				if (!p_via) {
					// local user left. 
					// store 1. permlink with a user on this channel
					// for later check if another user comes via
					// another permlink
					p_via = p->via;
					look_for_via_other_link = 1;
				}
				if (look_for_via_other_link) {
					if (p_via != p->via) {
						found = 1;
					}
				} else {
					if (p_via == p->via) {
						found = 1;
					}
				}
			}
		}
		if (found)
			break;
	}
	if (!found) {
		add_to_history(ch, "conversd", "*** rx for channel messages will stop:");
		sprintf(buffer, "    %s@%s was last user on this channel in this subnet.", name, host);
		add_to_history(ch, "conversd", buffer);
	}
}

/*---------------------------------------------------------------------------*/

void bye_command2(struct connection *cp, char *reason)
{
	struct connection *p;
	struct static_user_list *sul;
	struct clist *cl = NULLCLIST;
	struct channel *ch;
	int channel;

	if (!cp) {
		do_log(L_ERR, "OUCH, bye_command2() called with cp == NULL");
		return;
	}
	switch (cp->type) {
	case CT_UNKNOWN:
		cp->type = CT_CLOSED;
		break;
	case CT_USER:
		// send quits only if user has not a second login with the same call
		for (p = connections; p; p = p->next) {
			if (p->type == CT_USER && p != cp && !cp->via && !strcasecmp(cp->name, p->name))
					break;
		}
		if (!p)
			send_quits(cp, cp->name, cp->host, reason);
		cp->type = CT_CLOSED;
		// old variant
		cl = cp->chan_list;
		while (cl) {
			channel = cl->channel;
			clear_locks();
			send_user_change_msg(cp->name, cp->nickname, cp->host, cl->channel, -1, reason, 1, currtime, 0, (p ? 0 : 1), 0, 0, cp->nickname);
			cp->chan_list = cl->next;
			hfree(cl);
			cl = cp->chan_list;
			if (!count_user(channel)) {
				destroy_channel(channel, 0);
			} else {
				update_channel_history(0, channel, cp->name, cp->host);
			}
		}
		break;
	case CT_HOST:
		send_quits(cp, 0, cp->name, reason);
		cp->type = CT_CLOSED;
		update_permlinks(cp->name, NULLCONNECTION, 0);

		// add netsplit info to channel history
		for (ch = channels; ch; ch = ch->next) {
			for (p = connections; p; p = p->next) {
				if (p->type == CT_USER && p->via == cp && (p->channel == ch->chan))
						ch->locked = 1;
			}
		}
		for (ch = channels; ch; ch = ch->next) {
			if (ch->locked) {
				char buffer[2048];
				sprintf(buffer, "*** Netsplit: link %s", cp->name);
				add_to_history(ch, "conversd", buffer);
				// don't forget to clear lock
				ch->locked = 0;
			}
		}

		// old variant
		for (p = connections; p; p = p->next) {
			if (p->via == cp) {
				p->type = CT_CLOSED;
				clear_locks();
				channel = p->channel;
				p->channel = -1;
				send_user_change_msg(p->name, p->nickname, p->host, channel, -1, reason, 1, currtime, 0, 1, 0, 0, cp->nickname);
				if (!count_user(channel))
					destroy_channel(channel, 1);
				else
					update_channel_history(cp, channel, p->name, p->host);
				free_sul_sp(p);
			}
		}
		for (sul = list_of_static_users; sul; sul = sul->next) {
			if (sul->via && sul->via == cp) {
				// mark session dead
				sul->via = 0;
			}
		}
		route_cleanup(cp);
		break;
	case CT_CLOSED:
		break;
	}
}

/*---------------------------------------------------------------------------*/

void bye_command(struct connection *cp)
{
	char *s;
	char buffer[2048];

	s = getargcs(0, 1);
	if ((cp->compress & IS_COMP)) {
		fast_write(cp, "\r//COMP 0\r", 1);
	}

        if (s && *s && (cp->ircmode && *s == ':')) {
		if (!*(++s))
			s = 0;
	}

	if (!s || !*s || cp->observer) {
		bye_command2(cp, "/quit");
	} else {
		sprintf(buffer, "\"%s\"", s);
		bye_command2(cp, buffer);
	}
}


/*---------------------------------------------------------------------------*/

struct channel *new_channel(int channel, char *fromuser)
{
	struct channel *ch;
	struct channel *chp;

	ch = (struct channel *)hmalloc(sizeof(struct channel));
	chp = channels;
	if (chp != NULLCHANNEL) {
		ch->next = NULLCHANNEL;
		while ((chp != NULLCHANNEL) && (chp->chan < channel)) {
			ch->next = chp;
			chp = chp->next;
		}
		if (ch->next == NULLCHANNEL) {
			ch->next = chp;
			channels = ch;
		} else {
			ch->next->next = ch;
			ch->next = chp;
		}
	} else {
		ch->next = NULLCHANNEL;
		channels = ch;
	}
	ch->next = chp;
	ch->chan = channel;
	ch->topic[0] = '\0';
	ch->tsetby[0] = '\0';
	ch->time = 0;
	//ch->name[0] = '\0';
	ch->flags = 0;
	ch->chist = 0;
	ch->ctime = currtime;
	ch->ltime = 0;
	ch->expires = 0L;
	ch->locked_until = 0L;
	strncpy(ch->createby, fromuser ? fromuser : "", NAMESIZE);
	ch->createby[sizeof(ch->createby)-1] = 0;
	*ch->lastby = 0;

	return ch;
}

/*---------------------------------------------------------------------------*/

void channel_command(struct connection *cp)
{
	char *arg, *s;
	char buffer[2048];
	char chan[64];
	int newchannel;
	struct channel *ch;
	int printit, first;
	struct clist *cl, *cl2;
	int announce_as_new_user = 0; // mostly irc-mode users

	arg = getarg(0, 0);
	if (*arg == ':')
		arg++;
	while ((s = strsep(&arg, ","))) {
		clear_locks();
		if (*s == '#') {
			// be irc user friendly
			s++;
		}
		if (!*s) {
			if (cp->ircmode)
				return;
			sprintf(buffer, "*** (%s) You are talking to channel %d. There are %d users.\n",
			    	ts2(currtime), cp->channel, count_user(cp->channel));
			appendstring(cp, buffer);
			for (ch = channels; ch; ch = ch->next) {
				if (ch->chan == cp->channel)
					break;
			}
			if (ch && *ch->topic) {
				sprintf(buffer, "*** Topic set by %s:\n    ", ch->tsetby);
				appendstring(cp, buffer);
				appendstring(cp, ch->topic);
				appendstring(cp, "\n");
			}
			sprintf(buffer, "*** Also attached:");
			printit = 0;
			first = 1;
			for (cl = cp->chan_list; cl; cl = cl->next) {
				if (cl->channel != cp->channel) {
					if (first) {
						first = 0;
					} else {
						strcat(buffer, ",");
					}
					printit = count_user(cl->channel);
					if (printit == 1) {
						sprintf(chan, " channel %d (alone)", cl->channel);
					} else {
						sprintf(chan, " channel %d (%d users)", cl->channel, printit);
						printit = 1;
					}
					strcat(buffer, chan);
				}
			}
			strcat(buffer, ".\n");
			if (printit)
				appendstring(cp, buffer);
			appendprompt(cp, 0);
			return;
		}

		newchannel = atoi(s);

		if (!stringisnumber(s) || (!newchannel && ban_zero && cp->operator != 2) || newchannel < MINCHANNEL || newchannel > MAXCHANNEL) {
			if (!cp->ircmode) {
				sprintf(buffer, "*** (%s) Channel numbers must be in the range %d..%d.\n", ts2(currtime), (MINCHANNEL == 0 && ban_zero) ? 1 : MINCHANNEL, MAXCHANNEL);
			} else {
				sprintf(buffer, ":%s 403 %s %s :No such channel\n", cp->host,cp->nickname ? cp->nickname : cp->name, s);
			}
			appendstring(cp, buffer);
			appendprompt(cp, 0);
			return;
		}
		if (newchannel == cp->channel) {
			if (!cp->ircmode) {
				sprintf(buffer, "*** (%s) Channel %d is already default.\n", ts2(currtime), cp->channel);
				appendstring(cp, buffer);
				appendprompt(cp, 0);
			}
			return;
		}
		for (ch = channels; ch; ch = ch->next) {
			if (ch->chan == newchannel)
				break;
		}
		for (cl = cp->chan_list; cl; cl = cl->next) {
			if (cl->channel == newchannel)
				break;
		}
		if (!cl && ch && (ch->flags & M_CHAN_P) && cp->invitation_channel != newchannel) {
			if (check_user_banned(cp, "JOIN"))
				return;
			if (check_join_flood(cp, cp->name, myhostname, newchannel))
				return;

			if (cp->operator == 2) {
				if (!cp->ircmode) {
					sprintf(buffer, "*** Channel %d is invite only. Try again if you really want to join.\n", newchannel);
				} else {
					sprintf(buffer, ":%s 473 %s #%d :Nick/channel is temporarily unavailable. Try again if you really want to join.\n",cp->host,cp->nickname ? cp->nickname : cp->name, newchannel);
				}
				cp->invitation_channel = newchannel;
			} else {
				if (!cp->ircmode) {
					sprintf(buffer, "*** (%s) You need an invitation to join the private channel %d.\n", ts2(currtime), newchannel);
				} else {
					sprintf(buffer, ":%s 473 %s #%d :Cannot join channel (+i)\n",cp->host,cp->nickname ? cp->nickname : cp->name, newchannel);
				}
			}
			appendstring(cp, buffer);
			appendprompt(cp, 0);
			sprintf(buffer, "*** (%s) %s@%s tried to join your private channel.", ts2(currtime), cp->name, myhostname);
    			send_msg_to_channel("conversd", newchannel, buffer);
			return;
		}
		if (cl && cp->ircmode) {
			// already on
			return;
		}

		if (!cl) {
			// channel delay mechanism (rfc2811)
			if (ch) {
				if (cp->operator != 2 && ch->locked_until != 0L && ch->locked_until > currtime && strcasecmp(ch->createby, cp->name)) {
					if (!cp->ircmode)
						sprintf(buffer, "*** (%s) Channel %d is currently unavailable: netsplit.\n", ts2(currtime), newchannel);
					else
						sprintf(buffer, ":%s 437 %s #%d :Channel is temporarily unavailable\n", myhostname, cp->nickname, newchannel);
					appendstring(cp, buffer);
					appendprompt(cp, 0);
					return;
				}
			}

			if (check_user_banned(cp, "JOIN"))
				return;
			if (check_join_flood(cp, cp->name, myhostname, newchannel))
				return;

			cp->locked = 1;
			if (!cp->chan_list)
				announce_as_new_user = 1;
			send_user_change_msg(cp->name, cp->nickname, cp->host,  -1, newchannel, cp->pers, (announce_as_new_user && strcmp(cp->pers, "@")) ? 1 : 0, cp->time, cp->observer, 0, ((!ch || ch->expires != 0L) ? 2 : 0), (announce_as_new_user && strcasecmp(cp->nickname, cp->name)) ? 1 : 0, cp->name);
			if (!ch || ch->expires != 0L) {
				cp->locked = 0;
				do_log(L_CHAN, "Channel %d created by %s@%s", newchannel, cp->name, cp->host);
			}
			cl = (struct clist *)hmalloc(sizeof(struct clist));
			cl->time = currtime;
			cp->mtime = currtime;
			if ((!cp->observer) && (!ch || ch->expires != 0L)) {
				cl->channelop = 1;
			} else {
				cl->channelop = 0;
			}
			cl->channel = newchannel;
			if (!cp->chan_list || cp->chan_list->channel > newchannel) {	/* only occurs with 1st entry */
				cl->next = cp->chan_list;
				cp->chan_list = cl;
			} else
				for (cl2 = cp->chan_list; cl2; cl2 = cl2->next) {
					if (cl2->next) {	/* if there IS a next entry */
						if (cl2->next->channel > newchannel) {
							cl->next = cl2->next;
							cl2->next = cl;
							break;
						}
					} else {
						cl2->next = cl;
						cl->next = NULLCLIST;
						break;
					}
				}
		}
		cp->channel = newchannel;
		if (!cp->ircmode) {
			sprintf(buffer, "*** (%s) You are now talking to channel %d. ", ts2(currtime), cp->channel);
			appendstring(cp, buffer);
		} else {
			sprintf(buffer,":%s!%s@%s JOIN :#%d\n",cp->nickname ? cp->nickname : cp->name, cp->name, cp->host, cp->channel);
			appendstring(cp, buffer);
			//sprintf(buffer, ":%s 329 %s #%d %ld\n",
			        //cp->host,
			        //cp->nickname ? cp->nickname : cp->name, cp->channel, (ch ? ch->ctime : currtime));
			//appendstring(cp, buffer);
			sprintf(buffer, ":%s MODE #%d +%s\n", myhostname, cp->channel, get_mode_flags2irc((ch ? ch->flags : 0)));
			appendstring(cp, buffer);
		}
		if (ch && ch->expires == 0L) {
			if (!cp->ircmode) {
				sprintf(buffer, "There are %d users.", count_user(ch->chan));
				appendstring(cp, buffer);
				if (ch->ltime) {
					char tmp[64];
					sprintf(buffer, "\n    Last message on channel %s ago.", ts3(currtime - ch->ltime, tmp));
					appendstring(cp, buffer);
				}
				appendstring(cp, "\n");
			}
			if (*ch->topic) {
				if (!cp->ircmode) {
					sprintf(buffer, "*** Topic set by %s:\n    ", ch->tsetby);
					appendstring(cp, buffer);
					appendstring(cp, ch->topic);
					appendstring(cp, "\n");
				} else {
					sprintf(buffer, ":%s 332 %s #%d :%s",cp->host,cp->nickname ? cp->nickname : cp->name, cp->channel, ch->topic);
					buffer[IRC_MAX_MSGSIZE] = 0;
					appendstring(cp, buffer);
					appendstring(cp, "\n");
					sprintf(buffer, ":%s 333 %s #%d %s %ld\n",cp->host,cp->nickname ? cp->nickname : cp->name, cp->channel, ch->tsetby, ch->time);
					appendstring(cp,buffer);
				}
			}
			cp->channelop = cl->channelop;
		} else {
			if (!cp->ircmode) {
				sprintf(buffer, "You're alone.\n");
				appendstring(cp, buffer);
			}
			if (!cp->observer)
				cp->channelop = 1;
			if (ch) {
				strncpy(ch->createby, cp->name, NAMESIZE);
				ch->createby[sizeof(ch->createby)-1] = 0;
				ch->ctime = currtime;
				ch->locked_until = 0L;
				ch->expires = 0L;
			} else {
				new_channel(cp->channel, cp->name);
			}
		}
		if (cp->ircmode) {
			sprintf(cp->ibuf, "NAMES #%d", cp->channel);
			getarg(cp->ibuf, 0);
			irc_names_command(cp);
		}
		appendprompt(cp, 0);
	}
	if (announce_as_new_user) {
		clear_locks();
		if (cp->operator) {
			// may be not really (operator == 1 is learned. but /..OPER works
			// on "name" basis, not name@host
			clear_locks();
			cp->locked = 1;
			send_opermsg(cp->name, cp->nickname, myhostname, cp->name, cp->nickname, cp->host, -1, 1);
		}
		if (*cp->away) {
			clear_locks();
			cp->locked = 1;
			send_awaymsg(cp->name, cp->nickname, myhostname, cp->atime, cp->away, 1);
		}
#ifdef WANT_FILTER
		if (cp->filter && !cp->observer) {
			clear_locks();
			cp->locked = 1;
			send_filter_msg(cp->name, cp->host, cp->filter_time, cp->filter);
		}
#endif
	}
}
	
/*---------------------------------------------------------------------------*/

void charset_command(struct connection *cp)
{

	char *s1, *s2;
	char buffer[2048];
	int charset_in, charset_out;

	if (cp->ircmode) {
		// on ircmode /charset does not make any sense
		// because it may run out of /profile after login,
		// we silently discard any complains or status info
		// on charset. irc is iso. basta ;)
		return;
	}

	s1 = getarg(0, 0);


	if (!*s1) {
		sprintf(buffer, "*** Charset in/out is %s/%s.\n",
			get_charset_by_ind(cp->charset_in),
			get_charset_by_ind(cp->charset_out));
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}
	charset_in = get_charset_by_name(s1);

	s2 = getarg(0, 0);
	if (*s2)
		charset_out = get_charset_by_name(s2);
	else
		charset_out = charset_in;

	if (charset_in < 0 || charset_out < 0) {
		sprintf(buffer, "Unknown charset: '%s'.  You may use one of them:\n",
			(charset_in < 0) ? s1 : s2);
		append_general_notice(cp, buffer);
		append_general_notice(cp, list_charsets());
		append_general_notice(cp, "***\n");
		appendprompt(cp, 0);
		return;
	}
	cp->charset_in = charset_in;
	cp->charset_out = charset_out;

	sprintf(buffer, "*** Charset in/out set to %s/%s.\n",
		get_charset_by_ind(cp->charset_in),
		get_charset_by_ind(cp->charset_out));
	append_general_notice(cp, buffer);
	if (cp->charset_in == ISO && cp->charset_out == ISO) // default - iso iso
		sprintf(cp->ibuf, "/profile del charset");
	else
		sprintf(cp->ibuf, "/profile add charset %s %s", get_charset_by_ind(cp->charset_in), get_charset_by_ind(cp->charset_out));
	profile_change(cp);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void cstat_command(struct connection *cp)
{
	display_links_command(cp, NULLCHAR, "");
	hosts_command(cp);
}

/*---------------------------------------------------------------------------*/

void help_command(struct connection *cp)
{
	char buf[255];
	char *search, *p;
	int prompttype = 1;

	p = search = hstrdup(getarg(0, 0));
	if (*search == '/')
		search++;
	if (!*search) {
		sprintf(buf, "Conversd commands for %s at %s%s\n", SOFT_RELEASE, myhostname, cp->ircmode ? "" : "\n");
		append_general_notice(cp, buf);
	}
	if (!dohelper(cp, 0, (*search) ? search : NULLCHAR)) {
		if (ecmd_exists(search)) {
			sprintf(cp->ibuf, "/\377\200HELP %s %s\n", cp->name, search);
			h_unknown_command(cp);
			sprintf(buf, "*** Extended help request for command '%s' sent...\n", search);
			append_general_notice(cp, buf);
			prompttype = 0;
		} else {
			append_general_notice(cp, "No help found for '");
			appendstring(cp, search);
			appendstring(cp, "'.\n");
		}
	}
	appendprompt(cp, prompttype);
	hfree(p);
}

/*---------------------------------------------------------------------------*/
// print either path or count hops

char *compute_dest_path(struct destination *d)
{
	static char buffer[2048];
	struct destination *d2;
	int len;

	if (!d)
		goto end;
	strcpy(buffer, d->name);

	for (d2 = destinations; d2; d2 = d2->next) {
		d2->locked = 0;
	}
again:
	for (d2 = destinations; d2; d2 = d2->next) {
		if (d2->locked || !d2->link || !d2->rtt)
			continue;
		if (strcasecmp(d->hisneigh, d2->name))
			continue;
		// found
		len = strlen(buffer);
		sprintf(buffer+len, "!%s", (len + 1 + strlen(d2->name) + 1 + 3 /* ... */ + 1 + strlen(myhostname) + 1 < sizeof(buffer)) ? d2->name : "...");
		d = d2;
		d2->locked = 1;
		goto again;
	}
end:
	sprintf(buffer+strlen(buffer), "!%s", myhostname);
	return buffer;
}

/*---------------------------------------------------------------------------*/

void hosts_command(struct connection *cp)
{
	char buffer[2048], tmp[64];
	struct destination *d;
	int i = 1;
	char *dest = 0;
	int prompttype = 1;
	int goodlink;

	if (cp->type == CT_USER && !cp->observer)
		dest = getargcs(0, 0);
	else
		dest = "";

	if (*dest == '\0') {
		for (d = destinations; d; d = d->next) {
			if (d->rtt || cp->operator) {
				if (!cp->operator) {
					// operator could see the whole list - debuging purposes
					sprintf(buffer, "%-9.9s (%8.8s) %3s", d->name, d->rev, ts3(d->rtt, tmp));
					} else {
					sprintf(buffer, "%-9.9s via %-9.9s (%8.8s) %3s", d->name, ((d->link) ? d->link->name : "<dead>"), d->rev, ts3(d->rtt, tmp));
				}
					append_general_notice(cp, buffer);
				if (cp->ircmode)
					appendstring(cp, "\n");
				else {
					if ((i == (cp->operator ? 2 : 3)) || !(d->next)) {
							appendstring(cp, "\n");
						i = 0;
					} else {
						appendstring(cp, "   ");
					}
					i++;
				}
			}
		}
		if ((i != 1) && !cp->ircmode) {
			appendstring(cp, "\n");
		}
	} else {
		if (check_user_banned(cp, "HOST"))
			return;

		for (d = destinations; d; d = d->next) {
			// check d->link - just 2b sure..
			goodlink = (d->link && d->link->name && *(d->link->name));
			if (d->rtt && !strcasecmp(d->name, dest)) {
				clear_locks();
				sprintf(buffer, "*** Host : %s (%s) T=%lds H=%d [P:", d->name, d->rev, d->rtt, d->hops);
        			append_general_notice(cp, buffer);
				appendstring(cp, compute_dest_path(d));
				appendstring(cp, "]\n");
				sprintf(buffer, "*** route: %s -> %s -> %s (%ld)\n", myhostname,
						(goodlink) ? d->link->name : "(dead link)", dest, d->rtt);
				append_general_notice(cp, buffer);
				prompttype = 0;
				if (goodlink && strcasecmp(dest, d->link->name)) {
					sprintf(buffer, "/\377\200ROUT %s %s 99\n", dest, cp->name);
					linklog(d->link->connection, L_SENT, "%s", &buffer[3]);
					appendstring(d->link->connection, buffer);
				}
				break;
			}
		}
		if (d == NULLDESTINATION) {
			sprintf(buffer, "*** No route to %s.\n", dest);
			append_general_notice(cp, buffer);
			return;
		}
	}
	appendprompt(cp, prompttype);
}

/*---------------------------------------------------------------------------*/

void imsg_command(struct connection *cp)
{
	char *toname, *text;
	struct connection *p, *q;
	char buf[(NAMESIZE * 2) + 3];

	if (check_user_banned(cp, "IMSG"))
		return;

	toname = getarg(0, 0);
	if (*toname == '~')
		toname++;
	text = getarg(0, 1);
	if (!*text) {
		appendprompt(cp, 0);
		return;
	}
	for (p = connections; p; p = p->next) {
		getTXname(cp, buf);
		if (p->type == CT_USER && p->channel == cp->channel && strcasecmp(p->name, toname) != 0 && strcasecmp(p->nickname, toname) != 0) {
			getTXname(cp, buf);
			send_msg_to_user(buf, p->name, text);
			q = p->next;
			while (q) {
				if (!strcasecmp(p->name, q->name)) {
					q->locked = 1;
				}
				q = q->next;
			}
		}
		if (p->via)
			p->via->locked = 0;
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void invite_command(struct connection *cp)
{
	char *toname;
	char *arg;
	struct clist *cl;
	struct channel *ch;
	int chan;
	char buffer[2048];

	if (check_user_banned(cp, "IMSG"))
		return;

	toname = getarg(0, 0);
	if (*toname == '~')
		toname++;
	arg = getarg(0, 0);

	if (*arg == ':')
		arg++;
	if (*arg == '#')
		arg++;
	if (cp->ircmode) {
		if (!*arg || !*toname) {
			sprintf(buffer, ":%s 461 %s INVITE :Not enough parameters\n", myhostname, cp->nickname);
			appendstring(cp, buffer);
			appendprompt(cp, 0);
			return;
		}
	} else {
		if (!*toname) {
			append_general_notice(cp, "*** Who do you want to invite?\n");
			appendprompt(cp, 0);
			return;
		}
	}

	if (!*arg)
		chan = cp->channel;
	else
		chan = atoi(arg);

	if (*arg && (chan < 0 || !stringisnumber(arg))) {
		if (cp->ircmode) {
			sprintf(buffer, ":%s 403 %s #%s :No such channel\n", myhostname, cp->nickname, arg);
			appendstring(cp, buffer);
		} else {
			append_general_notice(cp, "*** Not a valid channel\n");
		}
		appendprompt(cp, 0);
		return;
	}

	for (ch = channels; ch; ch = ch->next) {
		if (ch->expires != 0L)
			continue;
		if (ch->chan == chan)
			break;
	}
	if (!ch) {
		if (cp->ircmode) {
			sprintf(buffer, ":%s 403 %s #%d :No such channel\n", myhostname, cp->nickname, chan);
			appendstring(cp, buffer);
		} else {
			append_general_notice(cp, "*** Channel does not exist\n");
		}
		appendprompt(cp, 0);
		return;
	}
	cl = 0;
	if (chan != cp->channel) {
		for (cl = cp->chan_list; cl; cl = cl->next) {
			if (cl->channel == chan)
				break;
		}
		if (!cl) {
			// don't be too concrete (don't announce sected channels
			if (ch->flags & M_CHAN_I || ch->flags & M_CHAN_S) {
				if (cp->ircmode) {
					sprintf(buffer, ":%s 403 %s #%d :No such channel\n", myhostname, cp->nickname, chan);
					appendstring(cp, buffer);
				} else {
					append_general_notice(cp, "*** Channel does not exist\n");
				}
			} else {
				if (cp->ircmode) {
					sprintf(buffer, ":%s 442 %s #%d :You're not on that channel\n", myhostname, cp->nickname, chan);
					appendstring(cp, buffer);
				} else {
					append_general_notice(cp, "*** You're not on that channel\n");
				}
			}
			appendprompt(cp, 0);
			return;
		}
	}
	// irc rfc conform checking..
	if (ch->flags & M_CHAN_P && !(cp->operator == 2 || (cp->channel == chan && cp->channelop) || (cl && cl->channelop))) {
		if (cp->ircmode) {
			sprintf(buffer, ":%s 482 %s #%d :You're not channel operator\n", myhostname, cp->nickname, chan);
			appendstring(cp, buffer);
		} else {
			append_general_notice(cp, "*** You need to be operator to invite to invite-only channels\n");
		}
		appendprompt(cp, 0);
		return;
	}

	if (check_cmd_flood(cp, cp->name, myhostname, SUL_INVI, 1, toname))
		return;

	// checking done
	cp->locked = 0; // need to be cleared in order to get a response
	send_invite_msg(cp->name, toname, chan);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void language_command(struct connection *cp)
{
	char *lang;
	char buffer[1024];
	int i, j;

	lang = getarg(0, 0);

	if (!*lang) {
		sprintf(buffer, "*** Your current language is %s.\n", languages[cp->lang].lang[0]);
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}
	if (!strcasecmp(lang, "help"))
		strcpy(lang,  "?");
	if (*lang == '?') {
		sprintf(buffer, "*** Available languages are:\n");
		append_general_notice(cp, buffer);
	}

	for (i = 0; languages[i].ext; i++) {
		if (*lang == '?') {
			sprintf(buffer, "    %-7s: ", (i == 0) ? "default" : languages[i].ext);
			append_general_notice(cp, buffer);
		}
		for (j = 0; languages[i].lang[j]; j++) {
			if (*lang == '?') {
				sprintf(buffer, "%s%s", ((j < 1) ? "" : ", "), languages[i].lang[j]);
				append_general_notice(cp, buffer);
			}
			else if (!strncasecmp(languages[i].lang[j], lang, strlen(lang))) {
				cp->lang = i;
				sprintf(buffer, "*** Changed your language to %s\n", languages[i].lang[0]);
				append_general_notice(cp, buffer);
				appendprompt(cp, 0);
				if (i == 0) {
					sprintf(cp->ibuf, "/profile del language");
				} else {
					sprintf(cp->ibuf, "/profile add language %s", languages[i].lang[0]);
				}
				profile_change(cp);
				return;

			}
		}
		if (*lang == '?')
			appendstring(cp, "\n");
	}
	if (*lang != '?') {
		sprintf(buffer, "Sorry, language %s is not supported.\n", lang);
		append_general_notice(cp, buffer);
		sprintf(buffer, "Type /lang ? for a listing of available languages.\n");
		append_general_notice(cp, buffer);
	}
	appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

void last_command(struct connection *cp)
{
#define	MAX_ENTRIES	64
	struct s_found_entries {
		char *userhost;
		char *data;
		long qtr;
		uint8 what;
	};
	struct s_found_entries *found_entries = 0;

	FILE *f;
	char buffer[2048];
	char buffer2[128];
	char myarg[2048];
	long qtr;
	int i, i_oldest, what, day, month, year, hour, mins;
	char *toname, *host, *orig_arg, *arg, *p, *q;
	int num_entries = 1; /* default: only the newest entry */

	orig_arg = getarg(0, 1);
	strncpy(myarg, orig_arg, sizeof(myarg)-1);
	myarg[sizeof(myarg)-1] = 0;

	arg = getarg(myarg, 0);
	if (*arg == '-' && (i = atoi(arg)) <= 0) {
		num_entries = abs(i);
		// silently avoid abuse
		if (num_entries > MAX_ENTRIES || num_entries <= 0) {
			sprintf(buffer, "*** List is limited to %d entries.\n", MAX_ENTRIES);
			append_general_notice(cp, buffer);
			num_entries = MAX_ENTRIES;
		}
		while (*orig_arg && !isspace(*orig_arg & 0xff))
			orig_arg++;
		while (*orig_arg && isspace(*orig_arg & 0xff))
			orig_arg++;
		arg = getarg(0, 0);
	}
	if (!(found_entries = (struct s_found_entries *) hmalloc(sizeof(struct s_found_entries) * num_entries)))
		return; // should never happen
again:
	while ((toname = strsep(&arg, ","))) {
		if (*toname == '~')
			toname++;
		if (!*toname) {
			if (!cp->ircmode) {
				append_general_notice(cp, "*** Need username.\n");
			} else {
				sprintf(buffer, ":%s 431 %s :No nickname given\n",cp->host,cp->nickname ? cp->nickname : cp->name);
				appendstring(cp,buffer);
			}
			appendprompt(cp, 0);
			if (found_entries)
				hfree(found_entries);
			return;
		}
		if (strlen(toname) > NAMESIZE-1)
			toname[NAMESIZE] = 0;
		if ((host = strchr(toname, '@')))
			*host++ = 0;
		sprintf(buffer, "%s.%02x", userfile, get_hash(toname));
		f = fopen(buffer, "r+");
		if (f == NULL) {
			if (!cp->ircmode) {
				sprintf(buffer, "%s: never seen here.\n", toname);
				append_general_notice(cp, buffer);
			} else {
				sprintf(buffer, ":%s 406 %s %s :There was no such nickname\n",cp->host,cp->nickname ? cp->nickname : cp->name,toname);
				appendstring(cp,buffer);
			}
			goto next_user;
		}
		for (i = 0; i < num_entries; i++) {
			found_entries[i].userhost = 0;
			found_entries[i].data = 0;
			found_entries[i].qtr = 0;
			found_entries[i].what = 0;
		}
		while (fgets(buffer2, 512, f)) {
			if (strlen(buffer2) < 3 || strncmp(buffer2, toname, strlen(toname)))
				continue;
			switch (buffer2[strlen(buffer2)-2]) {
			case '-':
				what = 0;
				break;
			case '+':
				what = 1;
				break;
			case '~':
				what = 2;
				break;
			default:
				continue;
			}
			buffer2[strlen(buffer2)-3] = 0;
			for (p = buffer2; *p != ' '; p++);
			*p++ = '\0';
			if (host && *host && (q = strchr(buffer2, '@')) && strcmp(q+1, host))
				continue;
			for (; *p == ' '; p++);
			strncpy(buffer, p, sizeof(buffer));
			buffer[sizeof(buffer)-1] = 0;
			if (sscanf(buffer, "%d.%d.%d %d:%d", &day, &month, &year, &hour, &mins) != 5)
				continue;
			// approx
			qtr = mins + 24 * hour + 60*24 * day + 60*24*31 * month + 60*24*31*12* year;
			for (i = 0, i_oldest = 0; i < num_entries; i++) {
				if (!found_entries[i].userhost) {
					i_oldest = i;
					break;
				}
				if (i > 0 && found_entries[i].qtr < found_entries[i_oldest].qtr)
					i_oldest = i;
			}
			if (qtr < found_entries[i_oldest].qtr)
				continue;
			if (found_entries[i_oldest].userhost)
				hfree(found_entries[i_oldest].userhost);
			found_entries[i_oldest].userhost = hstrdup(buffer2);
			if (found_entries[i_oldest].data)
				hfree(found_entries[i_oldest].data);
			found_entries[i_oldest].data = hstrdup(buffer);
			found_entries[i_oldest].qtr = qtr;
			found_entries[i_oldest].what = what;
		}
		fclose(f);

		if (found_entries[0].userhost) {
			int num_entries_curr = 0;
			for (;;) {
				int i_newest;
				// newest entries first
				for (i = 0, i_newest = 0; i < num_entries; i++) {
					if (!found_entries[i].userhost)
						continue;
					if (i > 0 && found_entries[i].qtr > found_entries[i_newest].qtr)
						i_newest = i;
				}
				if (!found_entries[i_newest].userhost)
					break;
				i = i_newest;
				if (!cp->ircmode) {
					sprintf(buffer, "%s %s: %s\n", found_entries[i].userhost, (found_entries[i].what ? (found_entries[i].what > 1 ? "[shutdown]" : "last sign on") : "last sign off"), found_entries[i].data);
				}
				else {
					if ((q = strchr(found_entries[i].userhost,'@')))
						*q++ = 0;
					sprintf(buffer, ":%s 312 %s %s %s :%s%s\n", myhostname, cp->nickname ? cp->nickname : cp->name, toname, (q ? q : myhostname), found_entries[i].data, (found_entries[i].what ? (found_entries[i].what > 1 ? " [shutdown]" : " (sign on)") : ""));
				}
				appendstring(cp, buffer);
				hfree(found_entries[i].userhost);
				found_entries[i].userhost = 0;
				hfree(found_entries[i].data);
				found_entries[i].data = 0;
				found_entries[i].qtr = 0;
				found_entries[i].what = 0;
				if (++num_entries_curr == num_entries)
					break;
			}
		} else {
			if ( !cp->ircmode) {
				sprintf(buffer, "*** %s was never logged on.\n", toname);
			}
			else {
				sprintf(buffer, ":%s 406 %s %s :There was no such nickname\n",cp->host,cp->nickname ? cp->nickname : cp->name,toname);
			}
			appendstring(cp, buffer);
		}
	}

next_user:
	arg = getarg(0, 0);
	if (*arg)
		goto again;

	if (found_entries)
		hfree(found_entries);

	if (cp->ircmode) {
		sprintf(buffer, ":%s 369 %s %s :End of WHOWAS\n",cp->host,cp->nickname ? cp->nickname : cp->name, orig_arg);
		appendstring(cp, buffer);
	}
	appendprompt(cp, 1);
}
	
/*---------------------------------------------------------------------------*/

void leave_command(struct connection *cp)
{
	int chan;
	struct clist *cl, *cl2;
	char *arg, *arg2;
	char *s;
	char buffer[2048];
	char reason[2048];

	arg = getarg(0, 0);
	if (*arg == ':')
		arg++;
	while ((arg2 = strsep(&arg, ","))) {
		clear_locks();
		if (arg2 && *arg2 == '#') {
			// be irc user friendly
			arg2++;
		}
		if (*arg2) {
			if (*arg2 == '*') {
				// user means this channel
                		chan = cp->channel;
			} else
				chan = atoi(arg2);
        	} else {
                	chan = cp->channel;
        	}
		if (chan < 0)
			goto not_on_chan;

	
		s = getargcs(0, 1);
        	if (s && *s && (cp->ircmode && *s == ':')) {
			if (!*(++s))
				s = 0;
		}
	
		if (!s || !*s || cp->observer) {
			*reason = 0;
		} else {
			sprintf(reason, "\"%s\"", s);
		}

		if ((chan == cp->channel) && (cp->chan_list->next == NULLCLIST)) {
			if (!cp->ircmode) {
				bye_command2(cp, reason);
				return;
			}
		}
	
		cl2 = cp->chan_list;
		if (cl2 && cl2->channel == chan) {
			cp->chan_list = cl2->next;
			hfree(cl2);
			cp->locked = 1;
			send_user_change_msg(cp->name, cp->nickname, cp->host, chan, -1, (*reason ? reason : "/leave"), 1, cp->time, cp->observer, 0, 0, 0, cp->nickname);
			cp->locked = 0;
			if (chan == cp->channel) {
				cp->channel = (cp->chan_list) ? cp->chan_list->channel : -1;
				cp->channelop = (cp->chan_list) ? cp->chan_list->channelop : 0;
				if (!cp->ircmode) {
					sprintf(buffer, "*** (%s) Default channel is now %d.\n", ts2(currtime), cp->channel);
				}
				else {
					sprintf(buffer,":%s!%s@%s PART #%d\n",
				        	cp->nickname ? cp->nickname : cp->name, cp->name, cp->host, chan);
				}
				appendstring(cp, buffer);
				appendprompt(cp, 0);
			} else {
				if (!cp->ircmode) {
					sprintf(buffer, "*** (%s) Left channel %d.\n", ts2(currtime), chan);
				}
				else {
					sprintf(buffer,":%s!%s@%s PART #%d\n",
				        	cp->nickname ? cp->nickname : cp->name, cp->name, cp->host, chan);
				}
				appendstring(cp, buffer);
				appendprompt(cp, 0);
			}
			if (!count_user(chan))
				destroy_channel(chan, 0);
			else
				update_channel_history(0, chan, cp->name, cp->host);
			return;
		}
		cl = cp->chan_list;
		while (cl) {
			if (cl->channel == chan) {
				cl2->next = cl->next;
				hfree(cl);
				cl = cl2;
				cp->locked = 1;
				send_user_change_msg(cp->name, cp->nickname, cp->host, chan, -1, (*reason ? reason : "@"), 1, cp->time, cp->observer, 0, 0, 0, cp->nickname);
				cp->locked = 0;
				if (chan == cp->channel) {
					cp->channel = cp->chan_list->channel;
					cp->channelop = cp->chan_list->channelop;
					if (!cp->ircmode) {
						sprintf(buffer, "*** (%s) Default channel is now %d.\n", ts2(currtime), cp->channel);
					}
					else {
						sprintf(buffer,":%s!%s@%s PART #%d\n",
					        	cp->nickname ? cp->nickname : cp->name, cp->name, cp->host, chan);
					}
					appendstring(cp, buffer);
					appendprompt(cp, 0);
				} else {
					if (!cp->ircmode) {
						sprintf(buffer, "*** (%s) Left channel %d.\n", ts2(currtime), chan);
					}
					else {
						sprintf(buffer,":%s!%s@%s PART #%d\n",
					        	cp->nickname ? cp->nickname : cp->name, cp->name, cp->host, chan);
					}
					appendstring(cp, buffer);
					appendprompt(cp, 0);
				}
				if (!count_user(chan))
					destroy_channel(chan, 0);
				else
					update_channel_history(0, chan, cp->name, cp->host);
				return;
			} else {
				cl2 = cl;
			}
			cl = cl->next;
		}
not_on_chan:
		if (!cp->ircmode) {
			sprintf(buffer, "*** (%s) You were not on channel %d.\n", ts2(currtime), chan);
		}
		else {
			sprintf(buffer, ":%s 442 %s #%d :You're not on that channel\n",
					  	cp->host,
					  	cp->nickname ? cp->nickname : cp->name, chan);
		}
		appendstring(cp, buffer);
		appendprompt(cp, 0);
	}
}
	
/*---------------------------------------------------------------------------*/

void display_links_command(struct connection *cp, char *user, char *lookup)
{
	char buffer[2048], tmp[64], tmp2[64], tmp3[64], tmp4[64];
	char *cptr;
	int pl;
	struct permlink *pp;
	long rxdivisor, txdivisor;
	int i;
	int state;

	if (!cp && (!user || !*user))
		return;

	if (user) {
		clear_locks();
		send_msg_to_user("conversd", user, "Host       State        Quality Software  Since NextTry Tries Queue    RX    TX");
	} else
		append_general_notice(cp, "Host       State        Quality Software  Since NextTry Tries Queue    RX    TX\n");

	if (!permarray)
		goto behind_permarray_null_problem;

	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if ((pp = permarray[pl]))
			pp->locked_func = 0;
	}

      for (state = 0; state < 4; state++) {
        // beautified output on /links:
        // - static links first
        //   - first those without backup link
        //   - then primaries with their correspondent backup links         
        // - learned links
        //   - in connected state
        //   - in disconnected state

	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		char more_info[256];
		if (!(pp = permarray[pl]))
			continue;
		if (pp->locked_func)
			continue;
		switch (state) {
		case 0:
			// static links
			// show only primary links without backup links
			if (!pp->permanent || pp->primary || !pp->backup)
				continue;
			break;
		case 1:
			// static links
			// show primary links first
			// backup links are shown due to the goto jump below
			// if no backup links configured, they have been showed already
			if (!pp->permanent || pp->primary || pp->backup)
				continue;
			break;
		case 2:
			// learned links
			// show learned links in connected state
			if (pp->permanent || !pp->connection)
				continue;
			break;
		case 3:
			// learned links
			// show links in disconnected state
			if (pp->permanent || pp->connection)
				continue;
			break;
		}
		pp->locked_func = 1;

again:
		if (*lookup && strcasecmp(pp->name, lookup) && strcasecmp(lookup, "long"))
			continue;
		sprintf(buffer, "%-10.10s ", pp->name);
		cptr = &buffer[strlen(buffer)];
		if (*lookup) {
			more_info[0] = 0;
		} else {
			more_info[0] = (pp->permanent ? (pp->primary ? ((pp->permanent != 2) ? 'B' : 'b') : ((pp->permanent != 2) ? 'P' : 'p')) : 'l');
			more_info[1] = 0;
			if (pp->permanent == 2) {
				// dummy link (passive) for dyndns hack. will be never actively connected
				strlwr(more_info);

			}
			if (pp->permanent) {
				if (pp->primary) {
					strcat(more_info, ",");
					strcpy(more_info+strlen(more_info), (pp->permanent != 2 ? "P:" : "p:"));
					strcpy(more_info+strlen(more_info), pp->primary->name);
					more_info[18] = 0;
				}
				if (pp->backup) {
					strcat(more_info, ",");
					strcpy(more_info+strlen(more_info), (pp->permanent != 2 ? "B:" : "b:"));
					strcpy(more_info+strlen(more_info), pp->backup->name);
					more_info[36] = 0;
				}
			} else
				strcpy(more_info, "l");
		}
		if (!pp->connection) {
			if (pp->locked) {
				strcpy(tmp, ts(pp->locked_until));
				sprintf(cptr, "Disc./locked%s  ---            %s %s        %s", (pp->locked > 1 ? "!" : " "), ts(pp->statetime), tmp, more_info);
			} else if (pp->permanent) {
				strcpy(tmp, ts(pp->retrytime));
				sprintf(cptr, "%s   ---            %s %s  %5d %s", (pp->primary ? "Backup link " : "Disconnected"), ts(pp->statetime), tmp, pp->tries, more_info);
			} else {
				sprintf(cptr, "Disconnected   ---            %s    -%s",  ts(pp->statetime), (*lookup ? "" : "          l"));
			}
		} else {
			if (pp->connection->type == CT_HOST) {
				char connect_opts[3];
				*connect_opts = 0;
				if (!pp->permanent) {
					strcat(connect_opts, "l");
				} else if (pp->permanent == 2) {
					strcat(connect_opts, (pp->primary ? "b" : "p"));
				}
				if (pp->connection->compress & IS_COMP)
					strcat(connect_opts, "c");
				if (pp->txtime || pp->rxtime) {
					if (pp->rxtime == -1)
						sprintf(tmp2, "  %3s  ", ts3(pp->txtime, tmp3));
					else
						sprintf(tmp2, "%3s/%-3s", ts3(pp->txtime, tmp3), ts3(pp->rxtime, tmp4));
				} else
					strcpy(tmp2, "  ---  ");
				rxdivisor = (pp->connection->received > (ONE_MEG * 9)) ? ONE_MEG : ONE_K;
				txdivisor = (pp->connection->xmitted > (ONE_MEG * 9)) ? ONE_MEG : ONE_K;
				sprintf(cptr, "Connected %-2s %s %8.8s %s               %5d %4d%c %4d%c",
					connect_opts,
					tmp2, pp->connection->rev, ts(pp->connection->time),
					queuelength(pp->connection->obuf), (int) (pp->connection->received / rxdivisor),
					(rxdivisor == ONE_K) ? 'K' : 'M', (int) (pp->connection->xmitted / txdivisor),
					(txdivisor == ONE_K) ? 'K' : 'M');
			} else {
				strcpy(tmp, ts(pp->retrytime));
				sprintf(cptr, "Connecting     ---            %s %s  %5d %s",
					ts(pp->statetime), tmp, pp->tries, more_info);
			}
		}
		if (user) {
			clear_locks();
			send_msg_to_user("conversd", user, buffer);
		} else {
			append_general_notice(cp, buffer);
			appendstring(cp, "\n");
		}
		if (*lookup) {
			// requested lookup, so he get's a long list
			if (pp->connection && pp->connection->type == CT_HOST && (pp->connection->compress & IS_COMP)) {
				sprintf(buffer, "  Compression ratio: RX %2.2f (%ld/%ld)  TX %2.2f (%ld/%ld)",
					pp->connection->xmitted_comp ? (pp->connection->xmitted / (float ) pp->connection->xmitted_comp) : 0.0F,
					pp->connection->xmitted,
					pp->connection->xmitted_comp,
					pp->connection->received_comp ? (pp->connection->received / (float ) pp->connection->received_comp) : 0.0F,
					pp->connection->received,
					pp->connection->received_comp);
				if (user) {
					clear_locks();
					send_msg_to_user("conversd", user, buffer);
				} else {
					append_general_notice(cp, buffer);
					appendstring(cp, "\n");
				}
			}
			sprintf(buffer, "  Link %s: %s. Groups %d.",
					pp->name,
					((pp->permanent) ? "static" : "learned"),
					pp->groups);
			if ((pp->primary)) {
					strcat(buffer, " Primary: ");
					strcat(buffer, pp->primary->name);
					strcat(buffer, ".");
			}
			if ((pp->backup)) {
					strcat(buffer, " Backup link: ");
					strcat(buffer, pp->backup->name);
					strcat(buffer, ".");
			}
			if (user) {
				clear_locks();
				send_msg_to_user("conversd", user, buffer);
			} else {
				append_general_notice(cp, buffer);
				appendstring(cp, "\n");
			}
			for (i = 0; i < pp->groups; i++) {
				if (pp->permanent && pp->s_groups[i].socket && *pp->s_groups[i].socket) {
					sprintf(buffer, "   %squal=%3.3u socket: %s%s%.127s%s",
						(pp->curr_group == i ? "+" : " "),
						pp->s_groups[i].quality,
						pp->s_groups[i].socket,
						(pp->s_groups[i].command ? "; command: \"" : ""),
						(pp->s_groups[i].command ? pp->s_groups[i].command : ""),
						(pp->s_groups[i].command ? "\"." : ""));
					if (user) {
						clear_locks();
						send_msg_to_user("conversd", user, buffer);
					} else {
						append_general_notice(cp, buffer);
						appendstring(cp, "\n");
					}
				}
			}
		}
		if (pp->backup && !pp->backup->locked_func) {
			pp = pp->backup;
			goto again;
		}
	}
      }
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if ((pp = permarray[pl]))
			pp->locked_func = 0;
	}

behind_permarray_null_problem:
	if (user) {
		clear_locks();
		send_msg_to_user("conversd", user, "***");
	} else
		appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

void links_command(struct connection *cp)
{
	char buffer[2048];
	char buffer2[2048];
	char *myargv[16];
	char *hostname, *sock_name, *cmd;
	int quality, group = 0;
	int pl;
	int len;
	struct permlink *pp, *p;
	struct connection *cpp;
	char *reason;
	int found;
	int do_lock = 0;
	time_t lock_time = 0L;

	hostname = getargcs(0, 0);
	// more comfort: /link add ....

	if (!(len = strlen(hostname))) { ; /* noop - exclude side effects */ }
	else if (!strncasecmp("add", hostname, len))
		hostname = "+";
	else if (!strncasecmp("del", hostname, len) || !strncasecmp("drop", hostname, len))
		hostname = "-";
	else if (!strncasecmp("list", hostname, len))
		hostname = "long";
	else if (!strncasecmp("lookup", hostname, len))
		hostname = "?";
	else if (!strncasecmp("reset", hostname, len) || !strncasecmp("kill", hostname, len))
		hostname = "~";
	else if (!strncasecmp("acl", hostname, len))
		hostname = "<";
	else if (!strcmp(hostname, "!") || !strncasecmp("lock", hostname, len)) {
		hostname = "~";
		do_lock = 1;
	}
	cmd = getargcs(0, 0);

	if (strlen(hostname) > NAMESIZE) {
		append_general_notice(cp, "The host name is too long.\n");
		appendprompt(cp, 0);
		return;
	}
	if (!hostname || !*hostname) {
		display_links_command(cp, NULLCHAR, "");
		return;
	}
	if (!strcmp(hostname, "?") || !strcasecmp(hostname, "long") || (!*cmd && strcmp(hostname, "-") && strcmp(hostname, "~") && strcmp(hostname, "+") && strcmp(hostname, "<"))) {
		if (!strcasecmp(hostname, "long")) {
		  cmd = "long";
		  hostname = myhostname;
		}
		if (!strcasecmp((hostname[0] == '@' ? (hostname+1) : hostname), myhostname) || !strcmp(hostname, "?")) {
			display_links_command(cp, NULLCHAR, cmd);
			return;
		}
		if (!strcasecmp((hostname[0] == '@' ? (hostname+1) : hostname), "all")) {
			sprintf(buffer, "*** Links at %s\n", myhostname);
			append_general_notice(cp, buffer);
			display_links_command(cp, NULLCHAR, cmd);
		}

		for (cpp = connections; cpp; cpp = cpp->next) {
			if (cpp->type == CT_HOST && !cpp->locked) {
				sprintf(buffer, "/\377\200LINK %s %s\n", cp->name, hostname);
				buffer[80] = '\0';
				linklog(cpp, L_SENT, "%s", &buffer[3]);
				appendstring(cpp, buffer);
				cpp->locked = 1;
			}
		}
		return;
	}
	if (cp->operator != 2) {
		append_general_notice(cp, "You must be an operator to set up new links.\n");
		goto prompt;
	}

	// link reset?
	if (!strcmp(hostname, "~")) {
		if (!permarray) {
			appendprompt(cp, 0);
			return; // nothing to do..
		}
		hostname = cmd;
		if (!*hostname) {
			append_general_notice(cp, "You must specify a hostname or \"all\" to reset each link.\n");
			goto prompt;
		}
		if (*hostname == '-' || !strncasecmp("all", hostname, strlen(hostname))) {
			// every link
			*hostname = 0;
		}
		do_log(L_INFO, "Link: link %s by operator %s: %s", (do_lock ? "lock" : "reset"), cp->name, ((*hostname)) ? hostname : "all");
		if (do_lock) {
			char *char_p;
			if ((char_p = showargp()))
                        	for (; *char_p && isspace(*char_p & 0xff); char_p++) ;
                        lock_time = currtime + (((char_p && isdigit(*char_p & 0xff)) ? atol(getarg(0, 0)) : 30L)  * 60L);
		}
		reason = getarg(0, 1);
		sprintf(buffer2, "link killed%s%s%s", (*reason) ? " (" : "", reason, (*reason) ? ")" : "");
		found = 0;
		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			if (!(pp = permarray[pl]))
				continue;
			if (!strcasecmp(pp->name, hostname) || !*hostname) {
				if (!found)
					found = 1;
				if (do_lock)
					sprintf(buffer, "*** Link: lock %s until %s\n", pp->name, ts2(lock_time));
				else 
					sprintf(buffer, "*** Link: reset %s\n", pp->name);
				append_general_notice(cp, buffer);
				if (pp->connection)
					bye_command2(pp->connection, buffer2);
				pp->statetime = currtime;
				pp->tries = 0;
				if (do_lock) {
					pp->locked = 2; // mark manual lock
					pp->locked_until = lock_time;
				} else {
					pp->locked = 0;
				}
				pp->retrytime = currtime+1;
				pp->waittime = 9;
				delay_permlink_connect(pp, 2);
				break;
			}
		}
		if (!found) {
			if (!cp->ircmode)
				sprintf(buffer, "Not in table: %s\n", hostname);
			else
				sprintf(buffer, ":%s 402 %s %s :No such server\n", myhostname, cp->nickname, hostname);
			appendstring(cp, buffer);
		}
		goto prompt;
	}
	// acl?
	if (!strcmp(hostname, "<")) {
		int argc = 1;
		char *argv[16];
		argv[0] = "Access";
		argv[1] = cmd;
		if (*cmd) {
			argc++;
			for (;;) {
				char *q = getarg(0, 0);
				if (!*q)
					break;
				argv[argc++] = q;
			}
		}
		modify_acl(cp, argc, argv);
		// does it's own prompt, if necessary
		return;
	}
	// link add
	if (strcmp(hostname, "-")) {
		if (!strcmp(hostname, "+")) {
			hostname = cmd;
			cmd = getarg(0, 0);
		}
		if (!*hostname) {
			append_general_notice(cp, "You must at least specify a hostname.\n");
			goto prompt;
		}
		do_log(L_INFO, "Link: new link set by operator %s: %s", cp->name, hostname);
		if (!permarray)
			goto behind_permarray_null_problem; 
		// "/li host ...." command
		/* bufgix: first, whe have to walk through the
		 * whole list. it's a linked list.. delete all
		 * references to this old link
		 */
		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			if (!(pp = permarray[pl]))
				continue;
			// delete old primary, if any
			if (pp->primary && !strcasecmp(pp->primary->name, hostname))
				pp->primary = NULL;
			// delete old backup link, if any
			if (pp->backup && !strcasecmp(pp->backup->name, hostname))
				pp->backup = NULL;
		}
		group = atoi(cmd);
		quality = atoi(getarg(0, 0));
		// free old entry for host, if any
		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			if (!(pp = permarray[pl]))
				continue;
			if (!strcasecmp(pp->name, hostname)) {
				if (pp->connection && pp->connection->type == CT_HOST)
					bye_command2(pp->connection, "link killed (administrative)");
				permarray[pl] = NULLPERMLINK;
				while (pp->groups) {
					if (pp->s_groups[pp->groups - 1].socket)
						hfree(pp->s_groups[pp->groups - 1].socket);
					if (pp->s_groups[pp->groups - 1].command)
						hfree(pp->s_groups[pp->groups - 1].command);
					pp->groups--;
				}
				hfree(pp);
				break;
			}
		}
behind_permarray_null_problem:
		// now add the new link
#ifdef	notdef
		p = update_permlinks(hostname, NULLCONNECTION, 0);
		if (p) {
			group = atoi(cmd);
			quality = atoi(getarg(0, 0));
			sock_name = getarg(0, 0);
			cmd = getarg(0, 1);
			strncpy(p->name, hostname, HOSTNAMESIZE);
			p->name[sizeof(p->name)-1] = 0;
			p->name_len = strlen(p->name);
			p->host = hstrdup(sock_name);
			p->groups = group;
			p->quality = quality;
			p->permanent = 1;
			permarray[pl] = p;
		} else {
			append_general_notice(cp, "*** Link table full!\n");
			goto prompt;
		}
#else
		p = 0; // make compiler happy
		len = 0;
		myargv[len++] = "add";
		myargv[len++] = hostname;	  /* host - case-sensitive */
		myargv[len++] = sock_name = getarg(0, 0);     /* s_group[n].socket (host:port) */
		myargv[len++] = getargcs(0, 0);   /* primary - case-sensitive.. */
		myargv[len++] = getargcs(0, 1);   /* s_group[n].command (optional) */
		/* do_link is in config.c */
		do_link(0, len, myargv);
#endif
		/* give sysop feedback */
		display_links_command(cp, NULLCHAR, hostname);
	} else {
		// link delete
		if (!permarray)
			goto behind_permarray_null_problem2; 
		hostname = cmd;
		// no host specified - delete whole table. *hostname == 0 is the marker
		if (!*hostname) {
			append_general_notice(cp, "You must specify a hostname or \"all\" to remove all links.\n");
			goto prompt;
		}
		if (!strcmp(hostname, "-") || !strncmp("all", hostname, strlen(hostname)))
			*hostname = 0;
		do_log(L_INFO, "Link: link removed by operator %s: %s", cp->name, ((*hostname) ? hostname : "whole link list; list is empty now."));
		/* bufgix: first, whe have to walk through the
		 * whole list. it's a linked list.. delete all
		 * references to this old link.
		 * if we cware said to clear the whole list,
		 * we need to do this, because every
		 * reference will go..
		 */
		reason = getarg(0, 1);
		sprintf(buffer2, "link killed%s%s%s", (*reason) ? " (" : "", reason, (*reason) ? ")" : "");
		found = 0;
		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			if (!(pp = permarray[pl]))
				continue;
			if ((!*hostname || !strcasecmp(pp->name, hostname))) {
				if (!found)
					found = 1;
				sprintf(buffer, "*** Link: deleted %s\n", pp->name);
				append_general_notice(cp, buffer);
				if (pp->connection && pp->connection->type == CT_HOST)
					bye_command2(pp->connection, buffer2);
				if (*hostname) {
					// relink primary / backup link list
					if (pp->primary)
						pp->primary->backup = pp->backup;
					if (pp->backup)
						pp->backup->primary = pp->primary;
				}
				permarray[pl] = NULLPERMLINK;
				while (pp->groups) {
					if (pp->s_groups[pp->groups - 1].socket)
						hfree(pp->s_groups[pp->groups - 1].socket);
					if (pp->s_groups[pp->groups - 1].command)
						hfree(pp->s_groups[pp->groups - 1].command);
					pp->groups--;
				}
				hfree(pp);
				/* should delete only this host? */
				if (*hostname)
					break;
				/* else: continue - delete all permlinks */
			}
		}
		if (!found) {
			if (!cp->ircmode)
				sprintf(buffer, "*** Not in table: %s\n", hostname);
			else
				sprintf(buffer, ":%s 402 %s %s :No such server\n", myhostname, cp->nickname, hostname);
			appendstring(cp, buffer);
		} else {
			/* give sysop feedback */
			display_links_command(cp, NULLCHAR, "long");
		}
	}
	return; /* at end of both condition, display_links_command does the prompt */
behind_permarray_null_problem2:
prompt:
	if (cp->type == CT_USER)
		appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

struct connection *sort_connections(int start) {
	static struct connection *p_last = 0;
	struct connection *p_best, *p;
	int i;

	if (start) {
		// we use locks as indicator that we have already announced an entry
		clear_locks();
		p_best = connections;
	} else {
		p_best = p_last;
	}
	if (!p_best) {
		p_last = 0;
		return 0;
	}
	for (p = connections; p; p = p->next) {
		// do not check ourself
		if (p_best == p)
			continue;
		// already announced? (i < 0)
		if (p->locked)
			continue;
		if (p_best->locked) {
			// loop for a new p_best
			p_best = p;
			continue;
		}
		i = strcmp(p->name, p_best->name);
		// announce later?
		if (i > 0)
			continue;
		if (i == 0) {
			// announce later
			if (strcmp(p->host, p_best->host) > 0)
				continue;
		}
		p_best = p;
	}
	if (p_best->locked) {
		// no new entry found
		p_last = 0;
		return 0;
	}

	// store last announced entry
	p_last = p_best;
	// and lock
	p_last->locked = 1;

	return p_last;
}

/*---------------------------------------------------------------------------*/

void list_command(struct connection *cp)
{
	char buffer[2048];
	char flags[16];
	struct channel *ch;
	int invisible;
	int secret;
	int special;
	int showit;
	int isonchan;
	struct connection *p;
	struct clist *cl;
	int n;
	char buf[(NAMESIZE * 2) + 3];
	char *arg;
	int show_topic = 1;
	int state = 0;
	int width = (cp->width < 1) ? 80 : cp->width;

	if ((arg = getarg(0, 0)) && *arg == 's')
		show_topic = 0;

	if (show_topic)
		appendstring(cp, "Channel Flags  Topic\n        Users\n");
	else
		appendstring(cp, "Channel Flags  Users\n");

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
					break;
				}
			}
			if (ch->flags & M_CHAN_S) {
				// secret channels later -> make them unpredictable
				if ((state == 0 && !(ch->chan == cp->channel || isonchan || cp->operator == 2)) ||
					(state == 1 && (ch->chan == cp->channel || isonchan || cp->operator == 2)))
					continue;
				strcat(flags, "S");
				special++;
				secret++;
			} else {
				if (state == 1)
					continue;
			}
			if (ch->flags & M_CHAN_P) {
				strcat(flags, "P");
			}
			if (ch->flags & M_CHAN_T) {
				strcat(flags, "T");
			}
			if (ch->flags & M_CHAN_I) {
				strcat(flags, "I");
				invisible++;
				special++;
			}
			if (ch->flags & M_CHAN_M) {
				strcat(flags, "M");
			}
			if (ch->flags & M_CHAN_L) {
				strcat(flags, "L");
			}
			showit = 0;
			if ((ch->chan == cp->channel) || isonchan || cp->operator == 2 || !special) {
				if (!ch->flags && (!show_topic || (ch->topic[0] == '\0'))) {
					sprintf(buffer, "%7d", ch->chan);
				} else {
					sprintf(buffer, "%7d %-6.6s %s\n       ", ch->chan, flags, ((show_topic) ? ch->topic : ""));
				}
				showit++;
			} else {
				if (secret && !invisible) {
					sprintf(buffer, "------- %-6.6s %s\n       ", flags, ((show_topic) ? ch->topic : ""));
					showit++;
				}
			}
			n = 9;
			if (showit) {
				for (p = sort_connections(1); p; p = sort_connections(0)) {
					if (p->type != CT_USER)
						continue;
					if (n > width - 12) {
						n = 9;
						strcat(buffer, "\n       ");
					}
					if (p->via) {
						if (p->channel == ch->chan) {
							strcat(buffer, " ");
							getTXname(p, buf);
							strcat(buffer, buf);
							strcat(buffer, "(");
							n += 2 + strlen(buf);
							if (!p->observer && p->operator) {
								strcat(buffer, "!");
								n++;
							} else if (!p->observer && p->channelop) {
								strcat(buffer, "@");
								n++;
							}
							if (*p->away) {
								strcat(buffer, "G");
								n++;
							}
							if (p->observer) {
								strcat(buffer, "O");
								n++;
							}
							if (buffer[strlen(buffer) - 1] == '(') {
								buffer[strlen(buffer) - 1] = '\0';
								n--;
							} else {
								strcat(buffer, ")");
								n++;
							}
						}
					} else {
						for (cl = p->chan_list; cl; cl = cl->next) {
							if (cl->channel == ch->chan) {
								strcat(buffer, " ");
								getTXname(p, buf);
								strcat(buffer, buf);
								strcat(buffer, "(");
								n += 2 + strlen(buf);
								if (!p->observer && p->operator) {
									strcat(buffer, "!");
									n++;
								} else if (!p->observer && cl->channelop) {
									strcat(buffer, "@");
									n++;
								}
								if (*p->away) {
									strcat(buffer, "G");
									n++;
								}
								if (p->observer) {
									strcat(buffer, "O");
									n++;
								}
								if (buffer[strlen(buffer) - 1] == '(') {
									buffer[strlen(buffer) - 1] = '\0';
									n--;
								} else {
									strcat(buffer, ")");
									n++;
								}
							}
						}
					}
				}
				strcat(buffer, "\n");
				appendstring(cp, buffer);
			}
		}
	}
	appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

void me_command(struct connection *cp)
{
	char *text;
	char buffer[2048];

	if (check_user_banned(cp, "ME"))
		return;

	text = getarg(0, 1);
	if (*text) {
		cp->locked = 1;
		sprintf(buffer, "*** %s@%s %s", cp->name, cp->host, text);
		send_msg_to_channel("conversd", cp->channel, buffer);
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void notice_command(struct connection *cp)
{
	char buffer[2048];
	int channel;
	char *toname, *text;
	struct channel *ch;
	struct clist *cl;
	char *q;

	if (check_user_banned(cp, "NOTICE"))
		return;

	*buffer = 0;

	toname = getarg(0, 0);
	if (*toname && (*toname == '#' || *toname == '~'))
		toname++;
	// notice commands should not be answered
	if (!*toname) return;

	text = getargcs(0, 1);

	cp->locked = 1;

	if (cp->ircmode) {
		if (*text == ':')
			text++;
		// dcc mess
		if ((q = strchr(text, '\001'))) {
			text = q+1;
			if ((q = strchr(text, '\001')))
				*q = 0;
			if (!strncasecmp(text, "AWAY ", 5)) {
				text = text+5;
				while (*text && isspace(*text & 0xff))
					text++;
				if (strcmp(cp->away, text))
					send_awaymsg(cp->name, cp->nickname, myhostname, currtime, text, 1);
				return;
			}
			if (!strncasecmp(text, "ACTION ", 7)) {
				text = text+7;
				while (*text && isspace(*text & 0xff))
					text++;
				if (strlen(text) > 384)
					text[384] = 0;
			} else {
				while (*text && isspace(*text & 0xff))
					text++;
				if (strlen(text) > 384)
					text[384] = 0;
				sprintf(buffer, "*** %s@%s ack :%s:", cp->name, cp->host, text);
			}
		}
	}

	if (!*buffer)
		sprintf(buffer, "*** %s@%s notice: %s", cp->name, cp->host, text);

	if (stringisnumber(toname) && (channel = atoi(toname)) >= 0) {
		// to channel. special care must be taken
		for (ch = channels; ch; ch = ch->next) {
			if (ch->expires != 0L)
				continue;
			if (ch->chan == channel)
				break;
		}
		if (!ch)
			return;
		// only channels we are on (and have permission to talk)
		if (cp->operator == 2) {
			;
		} else if  (cp->channel == ch->chan) {
			if ((ch->flags & M_CHAN_M) && !cp->channelop)
				return;
		} else {
			for (cl = cp->chan_list; cl; cl = cl->next)
				if (cl->channel == ch->chan)
					break;
			if (!cl)
				return;
			if ((ch->flags & M_CHAN_M) && !cl->channelop)
				return;
		}
		send_msg_to_channel("conversd", channel, buffer);
	} else {
		send_msg_to_user("conversd", toname, buffer);
	}
}

/*---------------------------------------------------------------------------*/

#ifdef WANT_FILTER
#define	FILTER_USERS	0
#define	FILTER_WORDS	1

void filter_command_handler(struct connection *cp, int what) {

	char *arg;
	char buffer[512];
	char call[256];
	char *filter;

	arg = getarg(0,1);
	strncpy(call, arg, sizeof(call)-2);
	call[sizeof(call)-2] = 0;
	strcat(call, " ");

	if (cp->observer && what == FILTER_USERS) {
		// FILTER_USERS will be sent out as /..FILTER
		sprintf(buffer, "*** Filter could not be set for observers.\n");
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}

	filter = (what == FILTER_USERS ? cp->filter : cp->filterwords);
	if (*arg) {
		if (!strcmp(arg, "-") || !strcmp(arg, "@") || !strncasecmp("off ", call, strlen(call))) {
			if (filter) {
				hfree(filter);
				if (what == FILTER_USERS)
					cp->filter = NULL;
				else
					cp->filterwords = NULL;
				sprintf(buffer, "*** %s-Filter switched off.\n", (what == FILTER_USERS ? "User" : "Word"));
				sprintf(cp->ibuf, "/profile del filter%s", (what == FILTER_USERS ? "users" : "words"));
				profile_change(cp);
				if (what == FILTER_USERS) {
					send_filter_msg(cp->name, cp->host, currtime, "@");
					cp->filter_time = currtime;
				}
			} else {
				sprintf(buffer, "*** %s-Filter was inactive.\n", (what == FILTER_USERS ? "User" : "Word"));
			}
		} else {
			if (filter) {
				hfree(filter);
				sprintf(buffer, "*** %s-Filter updated, now filtering %s\n", (what == FILTER_USERS ? "User" : "Word"), call);
			} else {
				sprintf(buffer, "*** %s-Filter active: %s\n", (what == FILTER_USERS ? "User" : "Word"), call);
			}
			if ((filter = (char *)hmalloc(strlen(call)+1)) != NULL) {
				strcpy(filter, call);
				if (what == FILTER_USERS) {
					send_filter_msg(cp->name, cp->host, currtime, call);
					cp->filter_time = currtime;
				}
				sprintf(cp->ibuf, "/profile add filter%s ", (what == FILTER_USERS ? "users" : "words"));
				strncpy(&cp->ibuf[strlen(cp->ibuf)], filter, MAX_MTU-strlen(cp->ibuf)-1);
				cp->ibuf[MAX_MTU-1] = 0;
				profile_change(cp);
			} else {
				sprintf(buffer, "*** Cannot set %s-filter: out of memory.\n", (what == FILTER_USERS ? "User" : "Word"));
			}
			if (what == FILTER_USERS)
				cp->filter = filter;
			else
				cp->filterwords = filter;
			
		}
	} else {
		if (filter) {
			sprintf(buffer, "*** %s-Filter active: %s\n", (what == FILTER_USERS ? "User" : "Word"), filter);
		} else {
			sprintf(buffer, "*** %s-Filter inactive.\n", (what == FILTER_USERS ? "User" : "Word"));
		}
	}

	append_general_notice(cp, buffer);
	appendprompt(cp, 0);
}
#endif

/*---------------------------------------------------------------------------*/

void filterusers_command(struct connection *cp)
{
	filter_command_handler(cp, FILTER_USERS);
}

/*---------------------------------------------------------------------------*/

void filterwords_command(struct connection *cp)
{
	filter_command_handler(cp, FILTER_WORDS);
}

/*---------------------------------------------------------------------------*/

void msg_command(struct connection *cp)
{
	char *toname, *tonickname, *text;
	char buffer[2048];
	struct connection *p;
	int chan;
	struct channel *ch;
	struct clist *cl;
	char buf[(NAMESIZE * 2) + 3];
	char *q;
	int as_conversd = 0;

	if (check_user_banned(cp, "PRIVMSG"))
		return;

	toname = getarg(0, 0);
	text = getarg(0, 1);

	if (cp->ircmode) {
		if (*text == ':')
			text++;
		// dcc mess
		if ((q = strchr(text, '\001'))) {
			text = q+1;
			if ((q = strchr(text, '\001')))
				*q = 0;
			if (!strncasecmp(text, "AWAY ", 5)) {
				text = text+5;
				while (*text && isspace(*text & 0xff))
					text++;
				if (strcmp(cp->away, text))
					send_awaymsg(cp->name, cp->nickname, myhostname, currtime, text, 1);
				return;
			}
			if (!strncasecmp(text, "ACTION ", 7)) {
				text = text+7;
				while (*text && isspace(*text & 0xff))
					text++;
				if (strlen(text) > 384)
					text[384] = 0;
				sprintf(buffer, "*** %s@%s %s", cp->name, cp->host, text);
				text = buffer;
			} else {
				while (*text && isspace(*text & 0xff))
					text++;
				if (strlen(text) > 384)
					text[384] = 0;
				sprintf(buffer, "*** %s@%s req :%s:", cp->name, cp->host, text);
				text = buffer;
			}
			as_conversd = 1; // authenticated
		}

	}

	if (!*toname || !*text) {
		if (cp->ircmode) {
			if (!*toname) {
				sprintf(buffer, ":%s 411 %s :No recipient given (PRIVMSG)\n", myhostname, cp->nickname);
			} else if (!*text) {
				sprintf(buffer, ":%s 412 %s :No text to send\n", myhostname, cp->nickname);
			}
		} else {
			sprintf(buffer, "*** (%s) Not sent: %s.\n", ts2(currtime), (*toname ? "No Text" : "Receipient missing"));

		}
		appendstring(cp, buffer);
		appendprompt(cp, 0);
		return;
	}
	if (toname[0] == '#') {
		chan = atoi(toname + 1);
		for (cl = cp->chan_list; cl; cl = cl->next) {
			if (cl->channel == chan)
				break;
		}
		for (ch = channels; ch; ch = ch->next) {
			if (ch->chan == cp->channel)
				break;
		}
		if (!cl | !ch) {
			if (!cp->ircmode) {
				sprintf(buffer, "*** (%s) You have not joined channel %d.\n", ts2(currtime), chan);
			}
			else {
				sprintf(buffer, ":%s 442 %s #%d :You're not on that channel\n",
							  cp->host,
							  cp->nickname ? cp->nickname : cp->name, chan);
			}
			appendstring(cp, buffer);
		} else {
			if (cl->channelop || !(ch->flags & M_CHAN_M)) {
				if (!as_conversd) {
					getTXname(cp, buf);
					send_msg_to_channel(buf, cl->channel, text);
				} else {
					send_msg_to_channel("conversd", cl->channel, text);
				}
			} else {
				if (!cp->ircmode) {
					append_general_notice(cp, "*** This is a moderated channel. Only channel operators may write.\n");
				}
				else {
					sprintf(buffer, ":%s 404 %s #%d :Cannot send to channel\n",
								  cp->host,
								  cp->nickname ? cp->nickname : cp->name, chan);
				}
			}
		}
	} else {
		if ((tonickname = strchr(toname, ':'))) {
			*tonickname++ = 0;
			if (!*tonickname)
				tonickname = toname;
		} else {
			tonickname = toname;
		}
		if (*toname == '~')
			toname++;
		p = 0;
		if (*toname) {
			for (p = connections; p; p = p->next)
				if (p->type == CT_USER && (!strcasecmp(p->name, toname) || !strcasecmp(p->nickname, tonickname)))
					break;
		}
		if (!p) {
			if (!cp->ircmode) {
				sprintf(buffer, "*** (%s) No such user: %s.\n", ts2(currtime), toname);
			}
			else {
				sprintf(buffer, ":%s 401 %s %s :No such nick\n",
							  cp->host,
							  cp->nickname ? cp->nickname : cp->name, toname);
			}
			appendstring(cp, buffer);
		} else {
			if (!as_conversd) {
				getTXname(cp, buf);
				send_msg_to_user(buf, toname, text);
			} else {
				send_msg_to_user("conversd", toname, text);
			}
			if (*p->away) {
				if (!cp->ircmode) {
					sprintf(buffer, "*** (%s) %s is away: %s\n", ts2(currtime), toname, p->away);
				}
				else {
					sprintf(buffer, ":%s 301 %s %s :%s",
								  cp->host,
								  cp->nickname ? cp->nickname : cp->name, toname, p->away);
					buffer[IRC_MAX_MSGSIZE] = 0;
					strcat(buffer, "\n");
				}
				appendstring(cp, buffer);
			}
		}
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void mode_command(struct connection *cp)
{
	char *arg, *c;
	int remove = 0;
	int channel = 0;
	struct connection *p;
	struct channel *ch;
	int oldflags = 0;
	struct clist *cl = 0;
	int op = 0;
	char buffer[2048];

	arg = getarg(0, 0);

	if (*arg == ':')
		arg++;
	if (*arg == '#') {
		// be irc user friendly
		arg++;
	} else if (cp->ircmode) {
		if (!strcmp(arg, "*")) {
			/* convers syntax, refers to this channel
			 * silently ignore
			 */
			return;
		}
		// irc USER MODE change (+o, +a, etc..
		if (cp->type == CT_USER) {
			char *arg2 = getarg(0, 0);
			if (!*arg) {
				sprintf(buffer, ":%s 461 %s MODE :Not enough parameters\n",
				  	cp->host,
				  	cp->nickname ? cp->nickname : cp->name);
				appendstring(cp, buffer);
				return;
			}
			if (strcasecmp(arg, cp->nickname)) {
				sprintf(buffer, ":%s 403 %s %s :No such channel\n",
			       		cp->host,
			       		cp->nickname ? cp->nickname : cp->name, arg);
				appendstring(cp, buffer);
				return;
			}
			*buffer = 0;
			if (!*arg2) {
				sprintf(buffer, ":%s 211 %s %s\n", myhostname, cp->nickname, (cp->verbose) ? "+sw" : "+");
			} 
			else if (strstr(arg2, "+s") || strstr(arg2, "+w"))
				cp->verbose = 1;
			else if (strstr(arg2, "-s") || strstr(arg2, "-w"))

				cp->verbose = 0;
			else {
				sprintf(buffer, ":%s 501 %s :Unknown MODE flag: %s\n",
			        	cp->host, cp->nickname ? cp->nickname : cp->name, arg2);
			}
			if (!*buffer)
				sprintf(buffer, ":%s MODE %s %csw\n", myhostname, cp->nickname, cp->verbose ? '+' : '-');
			appendstring(cp, buffer);
		} // else CT_HOST: MODE foo +o (aka /..OPER)  - see send_oper_msg..
		return;
	}

	/* no channel, or '*' like in /who * syntax? - she means her current */
	channel = atoi(arg);
	if (*arg && !strcmp(arg, "*")) {
		channel = cp->channel;
	} else if (!*arg || !stringisnumber(arg)) {
		if (cp->type != CT_HOST) {
			if (cp->ircmode) {
				if (*arg) {
					sprintf(buffer, ":%s 403 %s #%s :No such channel\n",
			        		cp->host,
			        		cp->nickname ? cp->nickname : cp->name, arg);
				} else {
					sprintf(buffer, ":%s 461 %s MODE :Not enough parameters\n",
					  	cp->host,
					  	cp->nickname ? cp->nickname : cp->name);
				}
				appendstring(cp, buffer);
			} else {
				if (*arg)
					append_general_notice(cp, "*** Channel does not exist.\n");
				else
					append_general_notice(cp, "*** Not enough parameters.\n");
			}
			appendprompt(cp, 0);
		}
		return;
	}

	arg = getarg(0, 1);

	if (!*arg && cp->type == CT_USER) {
		// hmm, wants to see modes for current channel
		// perhaps he wants to see all channels,
		// or the specified. is it secret. who wants to do this?
		// let's say /who q
		if (!cp->ircmode) {
			appendstring(cp, "Usage: /mode <channel> <args>\n");
			appendstring(cp, "For viewing the modes of a channel type '/who q'.\n");
			appendprompt(cp, 1);
			return;
		} else {
			for (ch = channels; ch; ch = ch->next) {
				if (ch->chan == channel) {
					if (ch->expires != 0L) {
						ch = 0;
						break;
					}
					sprintf(buffer, ":%s 324 %s #%d +%s\n",
					        cp->host,
					        cp->nickname ? cp->nickname : cp->name, channel, get_mode_flags2irc(ch->flags));
					appendstring(cp, buffer);

					sprintf(buffer, ":%s 329 %s #%d %ld\n",
					        cp->host,
					        cp->nickname ? cp->nickname : cp->name, channel, ch->ctime);
					appendstring(cp, buffer);
					appendprompt(cp, 0);
					break;
				}
			}
			if (!ch) {
				sprintf(buffer, ":%s 403 %s #%d :No such channel\n",
			        cp->host,
			        cp->nickname ? cp->nickname : cp->name, channel);
				appendstring(cp, buffer);
				appendprompt(cp, 0);
			}
		}
		return;
	}

	do_log(L_CHAN, "Mode command on channel %d: %s", channel, arg);
	if (cp->type == CT_USER) {
		for (cl = cp->chan_list; cl; cl = cl->next) {
			if (cl->channel == channel) {
				if (cl->channelop) {
					op++;
				}
				break;
			}
		}
		if (cp->operator == 2)
			op++;
		else {
			if (!cl) {
				if (!cp->ircmode) {
					sprintf(buffer, "*** (%s) You have not joined channel %d.\n", ts2(currtime), channel);
					append_general_notice(cp, buffer);
				}
				else {
					sprintf(buffer, ":%s 442 %s #%d :You're not on that channel\n",
							  	cp->host,
							  	cp->nickname ? cp->nickname : cp->name, channel);
					appendstring(cp, buffer);
				}
				appendprompt(cp, 0);
				return;
			}
		}
	}
	for (ch = channels; ch; ch = ch->next) {
		if (channel == ch->chan)
			break;
	}
	if ((ch && op) || cp->type == CT_HOST) {
		if (cp->type == CT_USER) {
			if (channel == 0 && cp->operator != 2) {
				if (!cp->ircmode) {
					append_general_notice(cp, "*** No modes on channel 0 allowed.\n");
				}
				else {
					sprintf(buffer, ":%s 477 %s #%d :Channel doesn't support modes\n",
					        cp->host,
					        cp->nickname ? cp->nickname : cp->name, channel);
					appendstring(cp, buffer);
				}
				appendprompt(cp, 0);
				return;
			}
		}
		if (cp->type == CT_USER && (check_user_banned(cp, "MODE") || check_cmd_flood(cp, cp->name, myhostname, SUL_MODE, 1, arg)))
			return;

			oldflags = ch->flags;
			while (*arg) {
				// ignore modes on #0. except +t and +o.
				if (ch->chan == 0 && !remove && isalpha(*arg & 0xff) && *arg != 't' && *arg != 'T' && *arg != 'o' && *arg != 'O') {
					arg++;
					continue;
				}
				switch (*arg) {
				case '+':{
						remove = 0;
						arg++;
						break;
				}
				case '-':{
						remove = 1;
						arg++;
						break;
					}
				case 'S':
				case 's':{
						if (remove && ch) {
							if (!cp->ircmode) {
								ch->flags -= (ch->flags & M_CHAN_S);
							} else  {
								ch->flags -= (ch->flags & M_CHAN_I);
							}
						} else {
							if (!cp->ircmode) {
								ch->flags |= M_CHAN_S;
							} else {
								ch->flags |= M_CHAN_I;
							}
						}
						arg++;
						break;
					}
				case 'P':
				case 'p':{
						if (remove && ch) {
							if (!cp->ircmode) {
								ch->flags -= (ch->flags & M_CHAN_P);
							} else {
								ch->flags -= (ch->flags & M_CHAN_S);
							}
						} else {
							if (!cp->ircmode) {
								ch->flags |= M_CHAN_P;
							} else {
								ch->flags |= M_CHAN_S;
							}
						}
						arg++;
						break;
					}
				case 'T':
				case 't':{
						if (remove && ch) {
							ch->flags -= (ch->flags & M_CHAN_T);
						} else {
							ch->flags |= M_CHAN_T;
						}
						arg++;
						break;
					}
				case 'I':
				case 'i':{
						if (remove && ch) {
							if (!cp->ircmode) {
								ch->flags -= (ch->flags & M_CHAN_I);
							} else {
								ch->flags -= (ch->flags & M_CHAN_P);
							}
						} else {
							if (!cp->ircmode) {
								ch->flags |= M_CHAN_I;
							} else {
								ch->flags |= M_CHAN_P;
							}
						}
						arg++;
						break;
					}
				case 'L':
				case 'l':{
						if (!cp->ircmode) {
							if (remove && ch) {
								ch->flags -= (ch->flags & M_CHAN_L);
							} else {
								ch->flags |= M_CHAN_L;
							}
						}
						else {
							sprintf(buffer, ":%s 472 %s %c :is unknown mode char to me\n",
							        cp->host,
							        cp->nickname ? cp->nickname : cp->name, *arg);
							appendstring(cp, buffer);
						}
						arg++;
						break;
					}
				case 'M':
				case 'm':{
						if (remove && ch) {
							ch->flags -= (ch->flags & M_CHAN_M);
						} else {
							ch->flags |= M_CHAN_M;
						}
						arg++;
						break;
					}
				case 'O':
				case 'o':{
						arg++;
						if (remove) {
							while (*arg && *arg == ' ')
								arg++;
							if (cp->type == CT_USER) {
								if (cp->ircmode) {
									sprintf(buffer, ":%s 481 %s %s :Permission denied (conversd protocol does not support deop).\n", cp->host, cp->nickname, (*arg ? arg : "[empty]"));

									appendstring(cp, buffer);
								} else {
									sprintf(buffer, "*** Conversd does not allow deop (%s).\n", arg);
									append_general_notice(cp, buffer);
								}
							}
							*arg = 0;
							break;
						}
						while (*arg) {
							int status_really_changed = 0;
							struct connection *p_found = 0;
							char *fromname = cp->name;
							char *fromnickname = (cp->type == CT_HOST ? cp->name : cp->nickname);
							char *fromhost = (cp->type == CT_HOST ? cp->name : cp->host);
							int user_found = 0;
							if (cp->type == CT_HOST && !user_check(cp, fromname))
								return;

							while (*arg == ' ')
								arg++;
							if (*arg == ':')
								arg++;

							if (*arg && !strncmp(arg, "+o", 2)) {
								// irssi hack: "sais +o+o nick1 nick2"
								arg += 2;
								continue;
							}

							c = arg;
							while (*c && *c != ' ')
								c++;
							if (*c == ' ') {
								*c++ = '\0';
							}
							if (cp->type != CT_HOST && channel < 0 )
								continue;
							clear_locks();
							if (cp->type == CT_HOST) {
								cp->locked = 1;
							}
							for (p = connections; p; p = p->next) {
								if (p->type != CT_USER)
									continue;
								if (!(!strcasecmp(p->name, arg) || (cp->type == CT_USER && !strcasecmp(p->nickname, arg))))
									continue;
								if (p->observer || !user_check(cp, p->name)) {
									p_found = p;
									user_found = -1; // mark bad, do not op, do not send_opermsg()
								}
								if (user_found < 0)
									continue;
								user_found = 1;
								if (channel == -1) {
									if (!p_found || !p->operator)
										p_found = p;
									if (!status_really_changed) {
										status_really_changed = (p->operator == 0);
									}
									p->operator = ((secretnumber < 0 && !p->via) ? 2 : 1);
								} else {
									if (p->channel == channel) {
										if (!p_found || !p->channelop)
											p_found = p;
										if (!status_really_changed)
											status_really_changed = (p->channelop == 0);
										p->channelop = 1;
									}
									for (cl = p->chan_list; cl; cl = cl->next) {
										if (cl->channel == channel) {
											if (!p_found || !p->channelop)
												p_found = p;
											if (!status_really_changed)
												status_really_changed = (cl->channelop == 0);
											cl->channelop = 1;
										}
									}
								}
							}
							if (p_found) {
								if (user_found > 0)
									send_opermsg(p_found->name, p_found->nickname, p_found->host, fromname, fromnickname, fromhost, channel, status_really_changed);
								else {
									if (cp->type == CT_USER) {
										if (cp->ircmode) {
											sprintf(buffer, ":%s 481 %s %s :Permission denied (user is %s)\n", cp->host, cp->nickname, arg, (p_found->observer ? "observer" : "unwanted"));

											appendstring(cp, buffer);
										} else {
											sprintf(buffer, "*** User is %s: %s\n", (p_found->observer ? "observer" : "unwanted"), arg);
											append_general_notice(cp, buffer);
										}
									}
								}
							} else {
								if (cp->type == CT_USER) {
									if (cp->ircmode) {
										if (user_found > 0)
											sprintf(buffer, ":%s 441 %s %s #%d :They aren't on that channel\n",
							        				cp->host,
							        				cp->nickname, arg, channel);
										else
											sprintf(buffer, ":%s 401 %s %s :No such nick\n",
							        				cp->host,
							        				cp->nickname, arg);
										appendstring(cp, buffer);
									} else {
										if (user_found > 0)
											sprintf(buffer, "*** User not in channel: %s.\n", arg);
										else 
											sprintf(buffer, "*** No such user: %s.\n", arg);
										append_general_notice(cp, buffer);
									}
								}
							}
							cp->locked = 1;
							arg = c;
						}
						break;
					}
				case 'N':
				case 'B':
				case 'b':
				case 'V':
				case 'v':
				case 'K':
				case 'k':
				case 'W':
				case 'w': {
						if (cp->ircmode) {
							sprintf(buffer, ":%s 472 %s :Unknown MODE flag (%c).\n",
							        cp->host,
							        cp->nickname ? cp->nickname : cp->name, *arg);
							appendstring(cp, buffer);
						}
						arg++;
						break;
					}

				case 'n':
				default:{
						arg++;
						break;
					}
				}
			}
			if (ch && (ch->flags != oldflags)) {
				clear_locks(); // send_opermsg may have locked it..
				// if cp is a user, send back the result. else: lock
				if (cp->type != CT_USER)
					cp->locked = 1;
				send_mode(cp, ch, oldflags);
			}
	} else {
		if (cp->type != CT_HOST) {
			if (!ch || ((ch->flags & M_CHAN_S || ch->flags & M_CHAN_I) && !(cp->operator == 2 || cl))) {
				if (!cp->ircmode) {
					append_general_notice(cp, "*** Channel does not exist.\n");
				} else {
					sprintf(buffer, ":%s 403 %s #%d :No such channel\n",
				        	cp->host,
				        	cp->nickname ? cp->nickname : cp->name, channel);
					appendstring(cp, buffer);
				}
			} else {
				if (!cp->ircmode) {
					append_general_notice(cp, "*** You are not an operator.\n");
				}
				else {
					sprintf(buffer, ":%s 482 %s #%d :You're not channel operator\n",
			        		cp->host,
			        		cp->nickname ? cp->nickname : cp->name, channel);
					appendstring(cp, buffer);
				}
			}
		}
	}
	if (cp->type == CT_USER) {
		appendprompt(cp, 0);
	}
}

/*---------------------------------------------------------------------------*/

void observer_command(struct connection *cp)
{
	cp->observer = 1;
	name_command(cp);
}

/*---------------------------------------------------------------------------*/

void restricted_command(struct connection *cp)
{
	cp->restrictedmode = 1;
	name_command(cp);
}

/*---------------------------------------------------------------------------*/

void fail_access(struct connection *cp, char *reason, int cause, int read_restrictfile, int kill)
{
	char buffer[2048];
	FILE *fd;
	char *sp;

	// protocol specific logging on refuse
	*buffer = 0;
#ifdef	HAVE_AF_INET
	if (cp->addr->sa_family == AF_INET) {
		struct sockaddr_in * sin = (struct sockaddr_in *) cp->addr;
		sprintf(buffer, "%s", inet_ntoa(sin->sin_addr));
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

	do_log(L_AUTH, "%s login as %s from [%s] to %s (%s)",  (kill ? "Refused" : "Restricted"), cp->name, buffer, cp->sockname, reason);


	if (cause) {
		if (!cp->ircmode) {	
			sprintf(buffer, "%s*** Sorry, access restricted (%s).%s", cp->ax25 ? "\r" : "\n", reason, cp->ax25 ? "\r" : "\n");
		} else {
			switch (cause) {
			case 2:
				if (cp->ircmode) {
					sprintf(buffer, ":%s 463 %s :Your host isn't among the privileged\r\n", myhostname, cp->nickname);
				}
				break;
			case 3:
				if (cp->ircmode) {
					sprintf(buffer, ":%s 484 %s :Your connection is restricted\r\n", myhostname, cp->nickname);
				}
			default:
				if (cp->ircmode) {
					sprintf(buffer, ":%s 465 %s :You are banned from this server\r\n",
		        			myhostname,
		        			(*cp->nickname) ? cp->nickname : "*");
				}
			}
		}
		fast_write(cp, buffer, 0);
	}
	
	if (read_restrictfile && !access(restrictfile, R_OK)) {
		if (cp->ircmode) {
			sprintf(buffer, ":%s 375 %s :- %s Message of the Day -\n", myhostname, cp->nickname, myhostname);
			fast_write(cp, buffer, 0);
		}
		fd = fopen(restrictfile, "r");
		if (fd != (FILE *) 0) {
			while (!feof(fd)) {
				if (cp->ircmode) {
					sprintf(buffer, ":%s 372 %s :\n", myhostname, cp->nickname);
					fast_write(cp, buffer, 0);
				}
				buffer[0] = '*';
				buffer[1] = ' ';
				fgets(buffer + 2, 2045, fd);
				if (!feof(fd)) {
					if (cp->ax25 && (sp = strchr(buffer, '\n')))
						*sp = '\r';
					fast_write(cp, buffer, 0);
				}
				
			}
			fclose(fd);
		}
		if (cp->ircmode) {
			sprintf(buffer, ":%s 376 %s :End of /MOTD command.\n", myhostname, cp->nickname);
			fast_write(cp, buffer, 0);
		}
	}

	if (kill) {
		if (cp->ircmode) {
			sprintf(buffer, ":%s KILL %s :Access restriced (%s)\r\n", myhostname, cp->nickname, reason);
			fast_write(cp, buffer, 0);
		}
	}
}

/*---------------------------------------------------------------------------*/

void irc_command(struct connection *cp)
{

	char buffer[2048];
	struct channel *ch;
	struct clist *cl;

	cp->ircmode = 1 - cp->ircmode;

	if (cp->ircmode) {
		// old characterset may not valid anymore: irc standard is iso
		cp->charset_in = cp->charset_out = ISO_STRIPED;
		append_general_notice(cp, "*** IRC mode enabled\n");
		sprintf(buffer, "USERHOST %s", cp->nickname);
		getarg(buffer, 0);
		irc_userhost_command(cp);
		sprintf(buffer, "LUSERS");
		getarg(buffer, 0);
		irc_lusers_command(cp);
		for (cl = cp->chan_list; cl; cl = cl->next) {
			sprintf(buffer,":%s!%s@%s JOIN :#%d\n",cp->nickname ? cp->nickname : cp->name, cp->name, cp->host, cl->channel);
			appendstring(cp, buffer);
			for (ch = channels; ch; ch = ch->next) {
				if (ch->chan == cl->channel)
					break;
			}
			sprintf(buffer, ":%s MODE #%d +%s\n", myhostname, cp->channel, get_mode_flags2irc((ch ? ch->flags : 0)));
			appendstring(cp, buffer);
			if (!ch)
				continue;
			//sprintf(buffer, ":%s 329 %s #%d %ld\n",
			        //cp->host,
			        //cp->nickname ? cp->nickname : cp->name, ch->chan, ch->ctime);
			//appendstring(cp, buffer);
			if (*ch->topic) {
				sprintf(buffer, ":%s 332 %s #%d :%s",cp->host,cp->nickname ? cp->nickname : cp->name, ch->chan, ch->topic);
				buffer[IRC_MAX_MSGSIZE] = 0;
				appendstring(cp, buffer);
				appendstring(cp, "\n");
				sprintf(buffer, ":%s 333 %s #%d %s %ld\n",cp->host,cp->nickname ? cp->nickname : cp->name, ch->chan, ch->tsetby, ch->time);
				appendstring(cp,buffer);
			}
			sprintf(buffer, "NAMES #%d", ch->chan);
			getarg(buffer, 0);
			irc_names_command(cp);
			sprintf(buffer, "WHO #%d", ch->chan);
			getarg(buffer, 0);
			irc_who_command(cp);
		}
	} else
		append_general_notice(cp, "*** IRC mode disabled\n");
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

char *default_authtype(struct connection *cp, char *newauthtype)
{
	FILE * fp;
	static char authtype[16];
	char conversrcfileAuth[PATH_MAX];
	char *name;

	*authtype = 0;

	if (!cp || cp->ircmode)
		return authtype;

	name = get_tidy_name(cp->name);
	if (!*name)
		return authtype;

	strcpy(conversrcfileAuth, DATA_DIR);
	strcat(conversrcfileAuth, "/conversrc/");
	strcat(conversrcfileAuth, name);
	strcat(conversrcfileAuth, ".auth");

	if (newauthtype) {
		if (strcasecmp(newauthtype, "text") && strcasecmp(newauthtype, "plain")) {
			strncpy(authtype, newauthtype, sizeof(authtype));
			authtype[sizeof(authtype)-1] = 0;
			rip(authtype);
		}
		if (!*authtype) {
			unlink(conversrcfileAuth);
		}
	} else {
		if ((fp = fopen(conversrcfileAuth, (newauthtype ? "w" : "r")))) {
			if (newauthtype)
				fputs(authtype, fp);
			else {
				fgets(authtype, sizeof(authtype), fp);
				rip(authtype);
			}
			fclose(fp);
		}
	}
	return authtype;
}

/*---------------------------------------------------------------------------*/

void user_login_common_partA(struct connection *cp)
{

	FILE * fd;
	char buffer[2048];
	int unf;
	char *p;
	char *reason = "unknown";
	struct ban_list *isbanned;
	struct connection *cp2;
	struct clist *cl;
	int cause;
	int is_via = 0;
	int is_local = 0;
	int on_channels = 0;
	int is_local_op = 0;

	/* convert / to _ */  /* unix security */
	for (p = cp->name; *p != '\0'; p++)
		if (*p == '/' || *p == '\\' || *p == '.' || *p == '@' || *p == ':')
			*p = '_';
	unf = 0;
	cause = 1;
	if (!*cp->name) {
		fast_write(cp, buffer, 0);
		unf = 2;
		reason = "bad_nick";
	} else if ((isbanned = is_banned(cp->name))) {
		reason = isbanned->reason;
		unf = 2;
	} else if (cp->restrictedmode > 1) {
		/* restrictedmode == 2? -> user came via reserved source port */
		unf = 1;
		reason = "reserved_socked";
		cause = 2;
	} else {
		/* noaccess - check for specificly denied access */
		fd = fopen(noaccessfile, "r");
		if (fd != (FILE *) 0) {
			while (!feof(fd)) {
				fgets(buffer, 2047, fd);
				for (p = buffer; *p && isspace(*p & 0xff); p++) ;
				rip(p);
				if (!*p || *p == '#')
					continue;
				if (user_matches(cp->name, p)) {
					reason = "convers.noaccess";
					unf = 1;
					break;
				}
			}
			fclose(fd);
		}
	}
	if (unf) {
		fail_access(cp, reason, cause, (unf == 1 ? 1 : 0), 1);
		bye_command(cp);
		return;
	}

	cause = 0;
	
	/* otherwise, if restricted mode or restricted or oberserver command, check */
#ifdef	HAVE_AF_AX25
	if ((restrictedmode || cp->restrictedmode) && cp->addr->sa_family != AF_AX25) {
		/* until here, ax25 calls have already been verified */
#else
	if (restrictedmode || cp->restrictedmode) {	/* restricted access - check for permitted */
#endif
		fd = fopen(accessfile, "r");
		unf = 0;
		if (fd != (FILE *) 0) {
			while (!feof(fd)) {
				fgets(buffer, 2047, fd);
				for (p = buffer; *p && isspace(*p & 0xff); p++) ;
				rip(p);
				if (!*p || *p == '#')
					continue;
				if (user_matches(cp->name, p)) {
					unf = 1;
					break;
				}
			}
			fclose(fd);
		}
		if (!unf) {
			if (restrictedmode == 1 || (cp->restrictedmode == 1)) {		/* only valid continue */
				fail_access(cp, "convers.access", cause, 1, 1);
				bye_command(cp);
				return;
			}
			/* else, let them go as observers */
			fail_access(cp, "restricted_socket", 3, 0, 0);
			cp->observer = 1;
		}
	}
	
	/* If someone thinks they are going to play games as conversd, rudely awaken them */
	if (!cp->ircmode) {
		if (!strcasecmp(cp->name, "conversd")) {
			sprintf(buffer, "%s*** You are a cheater -- goodbye!%s", cp->ax25 ? "\r" : "\n", cp->ax25 ? "\r" : "\n");
			fast_write(cp, buffer, 0);
			bye_command(cp);
			return;
		}
		if (callvalidate && !callvalid(cp->name))  {
			sprintf(buffer, "%s*** '%s' is not a valid call -- goodbye!%s", cp->ax25 ? "\r" : "\n", cp->name, cp->ax25 ? "\r" : "\n");
			fast_write(cp, buffer, 0);
			bye_command(cp);
			return;
		}
	} else {
		if (!strcasecmp(cp->name, "conversd") || (callvalidate && !callvalid(cp->name)))  {
			sprintf(buffer, ":%s 465 %s :You are banned from this server\r\n",
		        	myhostname,
		        	cp->nickname);
			fast_write(cp, buffer, 0);
			sprintf(buffer, ":%s KILL %s :Bad nickname (%s)\r\n", myhostname, cp->nickname, cp->nickname);
			fast_write(cp, buffer, 0);
			bye_command(cp);
			return;
		}
	}
	
	// be more efficient: intialize user_matches()
	user_matches(cp->name, 0);
	for (cp2 = connections; cp2; cp2 = cp2->next) {
		if (cp2->type == CT_USER && user_matches(0, cp2->name)) { 
			// for ForceAuthWhenOn
			if (cp != cp2) {
				if (cp2->via)
					is_via = 1;
				else
					is_local = 1; // already local
			}
			// for channel join abuse protection
			if (!cp2->via) {
				for (cl = cp2->chan_list; cl; cl = cl->next)
					on_channels++;
				if (!is_local_op && cp2->operator == 2)
					is_local_op = 1;
			}
		}
	}
	if ((FeatureFLOOD & NO_LOCAL_JOIN_FLOOD) && on_channels >= MAX_CHAN_PER_USER_AND_HOST && !is_local_op) {
		if (!cp->ircmode)
			sprintf(buffer, "*** %sYou have joined too many channels (%d) -- goodbye!%s\n", cp->ax25 ? "\r" : "\n", on_channels, cp->ax25 ? "\r" : "\n");
		else {
			sprintf(buffer, ":%s 465 %s :You are banned from this server\r\n",
        			myhostname,
        			cp->nickname);
			fast_write(cp, buffer, 0);
			sprintf(buffer, ":%s KILL %s :You have joined too many channels (%d)\r\n", myhostname, cp->nickname, on_channels);
		}
		fast_write(cp, buffer, 0);
		bye_command(cp);
		return;
	}

	if (!(cp->needauth & 1)) {
		if (strstr(cp->name, myhostname)) {
			// abuse?
			cp->needauth |= 1;
		} else if (ForceAuthWhenOn) {
			// check if there's already a user with this name in convers (not via this host)
			// if true, then we force password authentication
			if (((ForceAuthWhenOn < 2) ? (is_via && !is_local) : (is_via || is_local))) {
				cp->needauth |= 1;
				if (!cp->ircmode)
					appendstring(cp, "\n");
				append_general_notice(cp, "FYI: Login Passwort required (already signed on)\n");
			}
		}
	}
	if (!cp->ircmode) {
		strncpy(cp->nickname, cp->name, NAMESIZE);
		cp->nickname[sizeof(cp->nickname)-1] = 0;
		cp->nickname_len = strlen(cp->nickname);
	}
	cp->charset_in = cp->charset_out = (cp->ircmode) ? ISO_STRIPED : ISO;

	strncpy(cp->host, myhostname, NAMESIZE);
	cp->host[sizeof(cp->host)-1] = 0;

	sprintf(buffer, "/auth %s %s", default_authtype(cp, 0), cp->pass_got);
	getarg(buffer, 0);
	auth_command(cp);

}

/*---------------------------------------------------------------------------*/
	
void user_login_common_partB(struct connection *cp)
{

	FILE * fd;
	char usernotefile[PATH_MAX];
	char buffer[2048];
	int unf;
	int cnt;

	strcpy(usernotefile, DATA_DIR);
	strcat(usernotefile, "/personals/");
	strcat(usernotefile, cp->name);
	if (cp->observer) {
		if (!access(observerfile, R_OK) && (fd = fopen(observerfile, "r"))) {
			buffer[0] = '*';
			buffer[1] = ' ';
			while (!feof(fd)) {
				fgets(buffer + 2, 2045, fd);
				if (!feof(fd))
					appendstring(cp, buffer);
			}
			fclose(fd);
		} else {
			if (cp->ircmode) {
				sprintf(buffer, "%s: 484 %s :Your connection is restricted!\n", myhostname, cp->nickname);
				appendstring(cp, buffer);
				sprintf(buffer, "%s: MODE %s +r\n", myhostname, cp->nickname);
				appendstring(cp, buffer);
			} else {
				append_general_notice(cp, "*** You are an observer, with limited commands and no messages permitted\n");
			}
		}
		strncpy(cp->pers, OBSERVERID, PERSSIZE);
		cp->pers[sizeof(cp->pers)-1] = 0;
#ifdef	notdef /* why this? */
		if ((unf = creat(usernotefile, 0644)) != -1) {
			write(unf, cp->pers, strlen(cp->pers));
			close(unf);
		}
#endif
	} else {
		if (!cp->ircmode || !*cp->pers || !strcasecmp(cp->pers, "@")) {
			if (!access(usernotefile, R_OK)) {
				cnt = 0;
				if ((unf = open(usernotefile, O_RDONLY)) >= 0) {
					cnt = read(unf, buffer, PERSSIZE);
					close(unf);
				}
				if (cnt != 0 && cnt < sizeof(cp->pers)) {
					strncpy(cp->pers, buffer, cnt);
					cp->pers[cnt] = 0;
					sprintf(buffer, "*** Personal data set from file: %s\n", cp->pers);
				} else {
					sprintf(buffer, "*** Failed to read personal data file.\n");
				}
				if (cp->verbose)
					append_general_notice(cp, buffer);
			}
		}
	}
	//
	// BOFH-only option - be aware that ampr links are very SLO0oo-__w..
	// you have to uncomment the following line, if you _really_ need this
	// (set it directly, don't write it as profile)
	//cp->idle_timeout = 180*60L;	/* 3h */

}

/*---------------------------------------------------------------------------*/

void name_command(struct connection *cp)
{
	char buffer[2048];
	int newchannel = -1; // default channel
	int oldtype;
	char *p;

	oldtype = cp->type;
	/* cp->type  = CT_USER */; // dl9sau: oh no, do not assign CT_USER here:
			// if user said "/name" without arg, he'd be a user now,
			// could join channels and the program may segfault
			// anywhere in the code...

#ifdef	HAVE_AF_AX25
	if (cp->addr->sa_family == AF_AX25) {
		struct sockaddr_ax25 *sax = (struct sockaddr_ax25 *) cp->addr;

		// copy the call from transport layer
		sprintf(buffer, "%s", ax25_ntoa(&sax->sax25_call));
		strlwr(buffer);
		// strip ssid
		if ((p = strchr(buffer, '-')))
			*p = 0;
		// for ax25 users, "/name <channel>" may be sufficient
		for (p = cp->ibuf; *p && isspace(*p & 0xff); p++) ;
		p = getarg(0, 0);
		// a channel may begin with '#' - skip
		if (*p == '#')
			p++;
		// calls may begin with 4u...
		if (*p && !stringisnumber(p)) {
			// was a call - honour options like dl9sau-p
			if (restrictedmode || cp->restrictedmode == 1) {
				if (strncmp(buffer, p, strlen(buffer)) || isalnum(p[strlen(buffer)])) {
					// it's a fake
					sprintf(buffer, "*** (%s) Disconnecting: \"%s\" is not your own call (%s).\n", ts2(currtime), p, ax25_ntoa(&sax->sax25_call));
					fast_write(cp, buffer, 0);
					do_log(L_AUTH, "Refused login as %s from [%s] to %s (restricted)", p, ax25_ntoa(&sax->sax25_call), cp->sockname);
					bye_command(cp);
					return;
				}
			}
			strncpy(cp->name, p, NAMESIZE);
			cp->name[sizeof(cp->name)-1] = 0;
			cp->name_len = strlen(cp->name);
			p = getarg(0, 0);
			if (*p == '#')
				p++;
		} else {
			// was a call
			strncpy(cp->name, buffer, NAMESIZE);
			cp->name[sizeof(cp->name)-1] = 0;
			cp->name_len = strlen(cp->name);
		}
		if (stringisnumber(p))
			newchannel = atoi(p);
	} else
#endif
        {
		p = getarg(0, 0);
		if (*p == '#')
			p++;
		// calls may begin with 4u...
		if (!stringisnumber(p)) {
			strncpy(cp->name, p, NAMESIZE);
			cp->name[sizeof(cp->name)-1] = 0;
			cp->name_len = strlen(cp->name);
			p = getarg(0, 0);
			if (*p == '#')
				p++;
		}
		if (stringisnumber(p))
			newchannel = atoi(p);
	}
	if (*cp->name == '~') {
		// names starting with ~ are not valid. it's an indicator for unauthenticated users
		for (p = cp->name; *p; p++)
			*p = *(p+1);
	}
	if (!*cp->name || !validate_nickname(cp->name)) {
		sprintf(buffer, "*** '%s' is not a valid call.\n", cp->name);
		appendstring(cp, buffer);
		appendprompt(cp, 0);
		return;
	}

	if (newchannel == -3) {
		// ax25 login on restricted socket, or pre-defined name
		// /name is restricted to his call. ask for channel
		char hint[64];
		if (defaultchannel == 0 && ban_zero)
			strcpy(hint, "random");
		else
			sprintf(hint, "%d, -2 is random", defaultchannel);
		sprintf(buffer, "\n* Type /lo [channel] (default is %s)\n", hint);
		appendstring(cp, buffer);
		return;
	}

	cp->ircmode = 0;
	cp->channel = newchannel;

	user_login_common_partA(cp);
}

/*---------------------------------------------------------------------------*/

void user_login_convers_partA(struct connection *cp)
{

	FILE * fd;
	char buffer[2048];
	char usernotefile[PATH_MAX];
	struct channel *ch;
	struct clist *cl;
	int newchannel;
	int n_channels;
	int cnt;
	int unf;

	cp->locked = 1;

	if (cp->verbose)
		cp->verbose = is_hushlogin(cp) ? 0 : 1;

	newchannel = cp->channel;
//craiger	sprintf(buffer, "%s @ %s %18.18s %s-%s\n", convtype, myhostname, SOFT_LONGNAME, SOFT_TREE, SOFT_VERSION);
//craiger		appendstring(cp, buffer);
	if (!cp->verbose)
		goto skiped1;
//craiger
//craiger	appendstring(cp, "* Type /HELP for help.\n");

	/* print motd */
	if (!access(motdfile, R_OK) && (fd = fopen(motdfile, "r"))) {
		buffer[0] = '*';
		buffer[1] = ' ';
		while (!feof(fd)) {
			fgets(buffer + 2, 2045, fd);
			if (!feof(fd)) {
				appendstring(cp, buffer);
			}
		}
		fclose(fd);
	}
	
	cnt = count_user2(0);
	for (n_channels = 0, ch = channels; ch; ch = ch->next) {
		if (ch->expires != 0L)
			continue;
		if (!(ch->flags & M_CHAN_I))
                       	n_channels++;
       	}
	sprintf(buffer, "*** There %s %d user%s on %d channel%s online.\n", (cnt == 1) ? "is" : "are", cnt, (cnt != 1) ? "s" : "", n_channels, (n_channels == 1) ? "" : "s");
	appendstring(cp, buffer);
#ifdef	notdef	/* this is really not of interest */
	cnt = count_topics();
	if (cnt) {
		sprintf(buffer, "*** There %s %d topic%s available\n", (cnt == 1) ? "is" : "are", cnt, (cnt != 1) ? "s" : "");
		appendstring(cp, buffer);
	}
#endif

skiped1:

	ch = 0;
	// /name <call> 0, or without channel argument?
	if ((ban_zero && !newchannel) || (newchannel < MINCHANNEL && newchannel != -2) || newchannel > MAXCHANNEL) {
		if (newchannel > 0) {
			sprintf(buffer, "*** (%s) Channel numbers must be in the range %d..%d.\n",
				ts2(currtime), (MINCHANNEL == 0 && ban_zero) ? 1 : MINCHANNEL, MAXCHANNEL);
			appendstring(cp, buffer);
		}
		if (defaultchannel) {
			sprintf(buffer, "*** Will try local default channel %d.\n", defaultchannel);
			appendstring(cp, buffer);
		}
		newchannel = defaultchannel ? defaultchannel : -2; // use default channel or random channel
	}
	// find channel
	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == newchannel)
			break;
	}
	// channel delay mechanism (rfc2811)
	if (ch) {
		if (ch->locked_until != 0L && ch->locked_until > currtime && strcasecmp(ch->createby, cp->name)) {
			sprintf(buffer, "*** Channel %d is currently unavailable: netsplit.\n", newchannel);
			appendstring(cp, buffer);
			newchannel = -2;
		}
	}
	if (newchannel != -2) {
		if (ch && (ch->flags & M_CHAN_P)) {
			sprintf(buffer, "*** (%s) You need an invitation to join channel %d.\n", ts2(currtime), newchannel);
			appendstring(cp, buffer);
			sprintf(buffer, "*** (%s) %s@%s tried to join your private channel.\n", ts2(currtime), cp->name, myhostname);
    			send_msg_to_channel("conversd", newchannel, buffer);
			clear_locks();
			cp->locked = 1;
			newchannel = -2; // new channel (or default channel) mode P? - then fallback to random channel
		}
	} 
	if (newchannel < 0) {
		long tries = 0L;
#define	MAXTRIES	2*MAXCHANNEL
		append_general_notice(cp, "*** Will search for an empty channel.\n");
		// random channels
		if (seed == 1L)
			conv_randomize();
		for (;;) {
			if (tries++ > MAXTRIES)
				newchannel = MAXCHANNEL; // dos attack?
			if ((newchannel = conv_random(MAXCHANNEL-1, 1)) < /*0*/ MAXCHANNEL/2 || newchannel > MAXCHANNEL)
				continue;
			for (ch = channels; ch; ch = ch->next) {
				if (ch->chan == newchannel)
					break;
			}
			if (!ch || (ch->locked_until != 0L && ch->locked_until <= currtime && strcasecmp(ch->createby, cp->name)) || tries > MAXTRIES)
				break;
		}
	}
	cp->channel = newchannel;
	do_log(L_LUSER, "New local user %s entering channel %d", cp->name, cp->channel);
	if (ch && ch->expires == 0L) {
		sprintf(buffer, "*** (%s) You are now talking to channel %d. ", ts2(currtime), newchannel);
		appendstring(cp, buffer);
		if (cp->verbose) {
			cnt = count_user(newchannel);
			if (!cnt) {
				appendstring(cp, "You're alone.");
			} else {
				sprintf(buffer, "There are %d users.", cnt+1);
				appendstring(cp, buffer);
				if (ch->ltime) {
					char tmp[64];
					sprintf(buffer, "\n    Last message on channel %s ago.", ts3(currtime - ch->ltime, tmp));
					appendstring(cp, buffer);
				}
			}
		}
		appendstring(cp, "\n");
		if (cp->verbose && *ch->topic) {
			sprintf(buffer, "*** Topic set by %s:\n    ", ch->tsetby);
			appendstring(cp, buffer);
			appendstring(cp, ch->topic);
			appendstring(cp, "\n");
		}
		cp->channelop = 0;
	} else {
		sprintf(buffer, "*** You created a new channel %d.\n", cp->channel);
		appendstring(cp, buffer);
		if (!cp->observer)
			cp->channelop = 1;
		if (ch /* implicits: ch->expires == 0L */) {
			strncpy(ch->createby, cp->name, NAMESIZE);
			ch->createby[sizeof(ch->createby)-1] = 0;
			ch->ctime = currtime;
			ch->locked_until = 0L;
			ch->expires = 0L;
		} else {
			new_channel(cp->channel, cp->name);
		}
		do_log(L_CHAN, "Channel %d created by %s@%s", newchannel, cp->name, cp->host);
	}
	cl = (struct clist *)hmalloc(sizeof(struct clist));
	cl->next = NULLCLIST;
	cl->channelop = cp->channelop;
	cl->channel = cp->channel;
	cl->time = currtime;
	cp->mtime = currtime;
	cp->chan_list = cl;
	strcpy(cp->pers, "@");

	user_login_common_partB(cp);

	if (cp->isauth < 2 && BrandUser) {
		char pers[PERSSIZE+1];
		strcpy(pers, "~");
		if (strcmp(cp->pers, "@")) {
			strncpy(pers+1, cp->pers, PERSSIZE-1);
			pers[PERSSIZE] = 0;
		}
		strncpy(cp->pers, pers, PERSSIZE);
		cp->pers[sizeof(cp->pers)-1] = 0;
	}

	cp->locked = 1;

	strcpy(usernotefile, DATA_DIR);
	strcat(usernotefile, "/nicknames/");
	strcat(usernotefile, cp->name);
	if (!access(usernotefile, R_OK)) {
		cnt = 0;
		if ((unf = open(usernotefile, O_RDONLY)) >= 0) {
			cnt = read(unf, buffer, 512);
			close(unf);
		}
		if (cnt != 0 && cnt < sizeof(cp->nickname)) {
			char oldnickname[NAMESIZE+1];
			strncpy(oldnickname, cp->name, NAMESIZE);
			oldnickname[NAMESIZE] = 0;
			strncpy(cp->nickname, buffer, cnt);
			cp->nickname[cnt] = 0;
			cp->nickname_len = cnt;
			sprintf(buffer, "*** Nickname set from file: %s\n", cp->nickname);
		} else {
			sprintf(buffer, "*** Failed to read nickname file.\n");
		}
		if (cp->verbose)
			appendstring(cp, buffer);

	}
	send_user_change_msg(cp->name, cp->nickname, cp->host, -1, cp->channel, cp->pers, strcmp(cp->pers, "@") ? 1 : 0, currtime, cp->observer, 0, cp->channelop ? 2 : 0, strcasecmp(cp->nickname, cp->name) ? 1 : 0, cp->name);

	update_hushlogin(cp, !cp->verbose);
	cp->verbose = 0;

	// run profile
	strcpy(cp->ibuf, "/profile run");
	profile_change(cp);

}

/*---------------------------------------------------------------------------*/

void notify_command(struct connection *cp)
{
	char *p, *q, *toname;
	struct connection *pc;
	char buffer[512], t[512];
	int action;

	toname = getarg(0, 0);
	strncpy(t, toname, sizeof(t));
	t[sizeof(t)-1] = 0;
	toname = t;

	if (*toname == '\0') {
		if (*cp->notify && strlen(cp->notify) > 2) {
			sprintf(buffer, "*** (%s) You are notified if one of the following users sign on/off:\n",
			ts2(currtime));
			append_general_notice(cp, buffer);
			append_general_notice(cp, cp->notify);
		} else {
			sprintf(buffer, "*** (%s) Your notify list is empty", ts2(currtime));
			append_general_notice(cp, buffer);
		}
		appendstring(cp, "\n");
	}
	action = 0;
	while (*toname) {
		while ((*toname == '+') || (*toname == '-')) {
			while (*toname == '+') {
				action = 0;
				toname++;
				if (*toname == '\0') {
					toname = getarg(0, 0);
					strncpy(t, toname, sizeof(t));
					t[sizeof(t) - 1] = 0;
					toname = t;
					if (*toname == '\0') {
						break;
					}
				}
			}
			while (*toname == '-') {
				action = 1;
				toname++;
				if (*toname == '\0') {
					toname = getarg(0, 0);
					strncpy(t, toname, sizeof(t));
					t[sizeof(t) - 1] = 0;
					toname = t;
					if (*toname == '\0') {
						break;
					}
				}
			}
		}
		strcat(toname, " ");
		p = cp->notify;
		p = strstr(p, toname);
		while (p) {
			p--;
			if (*p != ' ') {
				p++;
				p++;
			} else {
				q = ++p;
				while (*q != ' ')
					q++;
				while (*q == ' ')
					q++;
				while (*q != '\0')
					*p++ = *q++;
				*p = *q;
				p = cp->notify;
			}
			p = strstr(p, toname);
		}
		if (action == 0) {
			if (cp->notify[0] == '\0') {
				strcpy(cp->notify, " ");
			}
			strcat(cp->notify, toname);
			for (pc = connections; pc; pc = pc->next) {
				sprintf(buffer, "%s ", pc->name);
				if ((pc->type == CT_USER) && !strncmp(t, buffer, strlen(buffer))) {
					if (!cp->ircmode) {
						sprintf(buffer, "*** %s is online.\n", pc->name);
					} else {
						if (strcasecmp(pc->name, pc->nickname))
							sprintf(buffer, ":%s 303 %s :%s %s\n", myhostname, cp->nickname, pc->nickname, pc->name);
						else
							sprintf(buffer, ":%s 303 %s :%s\n", myhostname, cp->nickname, pc->name);
					}
					appendstring(cp, buffer);
					break;
				}
			}
		}
		toname = getarg(0, 0);
		strncpy(t, toname, sizeof(t));
		t[sizeof(t) - 1] = 0;
		toname = t;
	}
	if (*cp->notify && strlen(cp->notify) > 2) {
		sprintf(cp->ibuf, "/profile add notify ");
		strncpy(&cp->ibuf[strlen(cp->ibuf)], cp->notify, MAX_MTU-strlen(cp->ibuf)-1);
		cp->ibuf[MAX_MTU-1] = 0;
	}
	else
		sprintf(cp->ibuf, "/profile del notify");
	profile_change(cp);
	appendprompt(cp, 0);
}


/*---------------------------------------------------------------------------*/

void operator_command(struct connection *cp)
{
	int number, answer, k;
	char buffer[2048];
	char *arg;

	arg = getargcs(0, 1);
	answer = atoi(arg);

	cp->needauth &= ~4;	// clear /operator input forcing

	if (*arg) {
		char *pw_got;
		int authtype = 0;

		pw_got = arg;
		if (*pw_got) {
			if (!strncasecmp(arg, "sys ", strlen(arg) > 3 ? 4 : 3)) {
				authtype = 1;
				pw_got = arg + 3;
			} else if (!strncasecmp(arg, "md5 ", strlen(arg) > 3 ? 4 : 3)) {
				authtype = 2;
				pw_got = arg + 3;
			}

			while (*pw_got && isspace(*pw_got & 0xff))
				pw_got++;
		}

		if (!*pw_got) {
			// secret password configured? if not use secret number
			if (!secretpass || !*secretpass) {
				// fallback to secretnumber
				if (!secretnumber)
					goto nolocalsysops;
				sprintf(buffer, "%5.5d", secretnumber > 0 ? secretnumber : (secretnumber * -1));
			} else {
				strncpy(buffer, secretpass, sizeof(buffer)-1);
				buffer[sizeof(buffer)-1] = 0;
			}
		
			if (authtype == 1) {
				ask_pw_sys(cp, buffer);
			} else if (authtype == 2) {
				ask_pw_md5(cp, buffer);
			}
			cp->needauth |= 4; // read answer from stdin
			return;

		}
		// a "standard" numeric answer? (believe it or not: max len 3)
		if (strlen(arg) < 4)
			goto standard_sys;

		if (!*cp->pass_want)
			goto fail;

		if (strstr(pw_got, cp->pass_want)) {
			*cp->pass_want = 0;	// reset state
			goto success;
		}
		*cp->pass_want = 0;	// reset state
		goto fail;
	}

standard_sys:
	if (!answer) {
		if (!secretnumber) {
nolocalsysops:
			// security: local administrative access denied
			do_log(L_AUTH, "Operator breakin by %s", cp->name);
			if (!cp->ircmode)
				sprintf(buffer, "*** Sorry, operator access is disabled at %s.\n", myhostname);
			else
				sprintf(buffer, ":%s 491 %s :No Operator access at %s\n", myhostname, cp->nickname, myhostname);
			breakins++;
		} else {
			if (seed == 1L)
				conv_randomize();
			number = 0;
			for (k = 0; k < 5; k++) {
				number += conv_random(9, 0);
				if (k != 4)
					number *= 10;
			}
			cp->expected = 0;
			if (cp->ircmode) {
				sprintf(buffer, ":%s 461 %s :Please respond to ", myhostname, cp->nickname);
				appendstring(cp, buffer);
			}
			sprintf(buffer, "(%d) %5.5d>\n", breakins, number);
			answer = secretnumber;
			if (answer < 0) {
				// negative secret number: remote administration by remote ops possible
				answer *= -1;
			}
			while (answer) {
				cp->expected += (number % 10) * (answer % 10);
				number /= 10;
				answer /= 10;
			}
		}
	} else {
		if (answer == cp->expected) {
success:
			do_log(L_AUTH, "Operator success by %s", cp->name);
			cp->operator = 2;
			cp->locked = 0;
			send_opermsg(cp->name, cp->nickname, myhostname, cp->name, cp->nickname, cp->host, -1, 1);
			if (!cp->ircmode)
				sprintf(buffer, "*** (%s) You are now an operator.\n", ts2(currtime));
			else
				sprintf(buffer, ":%s 381 %s :You are now an IRC operator\n", myhostname, cp->nickname);
		} else {
fail:
			do_log(L_AUTH, "Operator breakin by %s", cp->name);
			if (!cp->ircmode)
				sprintf(buffer, "*** (%s) Operator breakin attempt notified.\n", ts2(currtime));
			else
				sprintf(buffer, ":%s 464 %s :Password incorrect\n", myhostname, cp->nickname);
			breakins++;
		}
		/* after /oper, process_input() reads the next answer
		 * packet directly from the user input. but if user
		 * has answered with "/oper <num>" (we don't know..),
		 * it's on us to clear the flag now.
		 */
		cp->expected = 0;
	}
	appendstring(cp, buffer);
	if (answer) {
		appendprompt(cp, 0);
	}
#ifdef POSIX
	fflush(NULL);
#endif
}

/*---------------------------------------------------------------------------*/

void personal_command(struct connection *cp)
{

	char *s;
	char buffer[2048];
	char usernotefile[PATH_MAX];
	char pers[PERSSIZE+1];
	int unf;
	//struct connection *p;
	int pers_changed = 0;

	if (check_user_banned(cp, "PERS"))
		return;

	strcpy(usernotefile, DATA_DIR);
	strcat(usernotefile, "/personals/");
	strcat(usernotefile, cp->name);

	s = getarg(0, 1);
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_PERS, 1, s))
		return;


	if (*s == '~')
		s++;	// reserverd for brandmarking nonauth'ed users
	if (!*s || (!strcmp(s, "@"))) {
		s = "@";
		sprintf(buffer, "*** (%s) Personal text deleted.\n", ts2(currtime));
		unlink(usernotefile);
	} else {
		if (strlen(s) > PERSSIZE)
			s[PERSSIZE] = 0;
		sprintf(buffer, "*** (%s) Personal text set.\n", ts2(currtime));
		if ((unf = creat(usernotefile, 0644)) != -1) {
			write(unf, s, strlen(s));
			close(unf);
		}
	}
	cp->mtime = currtime;

	if (cp->isauth < 2 && BrandUser) {
		strcpy(pers, "~");
		if (strcmp(s, "@")) {
			strncpy(pers+1, s, PERSSIZE-1);
			pers[PERSSIZE] = 0;
		}
		s = pers;
	}
#ifdef	notdef
	for (p = connections; p; p = p->next) {
		if (p->type == CT_USER) {
			if (p->via)
				continue;
			if (!strcasecmp(p->name, cp->name) && !strcasecmp(p->host, cp->host)) {
				if (!strcmp(p->pers, s)) {
					append_general_notice(cp, "*** Personal not changed.\n");
					continue;
				}
				strncpy(p->pers, s, PERSSIZE);
				p->pers[sizeof(p->pers)-1] = 0;
				append_general_notice(p, buffer);
				p->locked = 1;
				pers_changed = 1;
			}
		}
	}
#endif
	//update_user_data(cp, 1, 0, pers_changed);
	// irc ircuser and not on any channel, do not announce. will be done later
#ifdef	notdef
	if (pers_changed && (!cp->ircmode || cp->chan_list)) {
		//send_persmsg(cp->name, cp->nickname, myhostname, cp->channel, s, pers_changed, currtime);
		send_user_change_msg(cp->name, cp->nickname, myhostname, -1, -1, cp->pers, pers_changed, currtime, cp->observer, 0, 0, 0, cp->nickname);
	}
#else
	pers_changed = strcmp(s, cp->pers) ? 1 : 0;
	if (pers_changed && (!cp->ircmode || cp->chan_list) && change_pers_and_nick(cp, cp->name, myhostname, s, cp->channel, 0, 1, 1)) {
		appendstring(cp, buffer);
	} else {
		append_general_notice(cp, "*** Personal not changed.\n");
	}
#endif
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void prompt_command(struct connection *cp)
{
	char *args;

	if (cp->ircmode) {
		char buffer[2048];
		sprintf(buffer, ":%s 421 %s PROMPT :Unknown command\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	args = getarg(0, 1);

	if (*args && (!strcmp(args, "-") || !strcmp(args, "off")))
		*args = 0;
	cp->prompt[2] = '\0';
	cp->prompt[3] = '\0';
	cp->prompt[0] = '\0';
	if ((cp->prompt[1] = args[0]) != '\0')
		if ((cp->prompt[2] = args[1]) != '\0')
			if ((cp->prompt[3] = args[2]) != '\0')
				cp->prompt[0] = args[3];
	if (cp->prompt[3] == '\b' && cp->prompt[0] == '\0') {
		cp->prompt[0] = '\b';
		cp->prompt[3] = '\0';
	}
	if (*args) {
		char buf[5];
		append_general_notice(cp, "*** Prompting mode enabled.\n");
		strncpy(buf, args, sizeof(buf));
		buf[sizeof(buf)-1] = 0;
		sprintf(cp->ibuf, "/profile add prompt %s", buf);
	} else {
		append_general_notice(cp, "*** Prompting mode disabled.\n");
		sprintf(cp->ibuf, "/profile del prompt");
	}
	profile_change(cp);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void query_command(struct connection *cp)
{

	char *toname;
	char buffer[2048];
	struct connection *p;

	if (cp->ircmode) {
		sprintf(buffer, ":%s 421 %s QUERY :Unknown command\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	toname = getarg(0, 0);
	if (*toname == '~')
		toname++;

	if (*toname) {
		for (p = connections; p; p = p->next)
			if (p->type == CT_USER && (!strcasecmp(p->name, toname) || !strcasecmp(p->nickname, toname)))
				break;
		if (!p) {
			sprintf(buffer, "*** (%s) No such user: %s.\n", ts2(currtime), toname);
			append_general_notice(cp, buffer);
		} else {
			strncpy(cp->query, toname, NAMESIZE);
			cp->query[sizeof(cp->query)-1] = 0;
			sprintf(buffer, "*** (%s) Starting private conversation with %s.\n", ts2(currtime), cp->query);
			append_general_notice(cp, buffer);
		}
	} else if (cp->query[0] != '\0') {
		sprintf(buffer, "*** (%s) Ending private conversation with %s.\n", ts2(currtime), cp->query);
		append_general_notice(cp, buffer);
		cp->query[0] = '\0';
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void restart_command(struct connection *cp)
{
	int halting;
	char buffer[512];

	if (cp->operator != 2) {
		if (!cp->ircmode)
			appendstring(cp, "You must be an operator to restart/halt the system.\n");
		else {
			sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
			appendstring(cp, buffer);
		}
		appendprompt(cp, 1);
		return;
	}
	/* this same routine is used by both the /restart and /halt
	   commands. we now look to see if we are to halt */
	halting = (tolower(cp->ibuf[1]) == 'h');

	mysighup(0);

	if (halting) {
		/* yep, halting, so now loop indefinitely */
		//for (;;);
		exit(0);
	}
}

/*---------------------------------------------------------------------------*/

void stats_command(struct connection *cp)
{
	char *how, *howmany;
	int mode;
	long count;
	struct stats *ptr;
	int k;
	char buffer[80];
	char *tmfmtstr;
	char strbuf[20];
	int offset;
	char *fmtstr;

	how = getargcs(0, 0);
	if (cp->ircmode) {
		char buffer2[2048];
		if (!*how || strchr(how, 'u')) {
			sprintf(buffer, ":%s 242 %s :Server Up %s\n", myhostname, cp->name, ts4(currtime - boottime));
			appendstring(cp, buffer);
		}
		sprintf(buffer2, ":%s 219 %s %s :End of /STATS report\n", myhostname, cp->name, getarg(0, 0));
		appendstring(cp, buffer2);
		return;
	}
	how = getargcs(0, 0);
	howmany = getarg(0, 0);
	if (*how == 'd') {
		mode = 1;
		count = daysonline;
		ptr = daily;
		tmfmtstr = "%b %d: ";
		fmtstr = " %s(%9ld/%-9ld)";
	} else {		/* default to hourly */
		mode = 0;
		count = hoursonline;
		if (hoursonline > 59)
			count = 59;
		ptr = hourly;
		tmfmtstr = "%b %d, %H:00: ";
		fmtstr = " %s(%7ld/%-7ld)";
	}

	if (*howmany && (k = atoi(howmany))) {
		k--;
		if (k < count)
			count = k;
	}
	offset = (count / 2) + 1;
	sprintf(buffer, "*** %sly statistics profile of last %ld %s%s (rx/tx) at %s:\n",
		(mode) ? "Dai" : "Hour", count + 1, (mode) ? "day" : "hour", (count) ? "s" : "", myhostname);
	appendstring(cp, buffer);
	sprintf(buffer, "*** Online since %s ***\n", rip(ctime(&starttime)));
	append_general_notice(cp, buffer);
	for (k = count; k >= offset; k--) {
		strftime(strbuf, 20, tmfmtstr, localtime(&ptr[k].start));
		append_general_notice(cp, "***  ");
		sprintf(buffer, fmtstr, strbuf, ptr[k].rx, ptr[k].tx);
		appendstring(cp, buffer);
		strftime(strbuf, 20, tmfmtstr, localtime(&ptr[k - offset].start));
		appendstring(cp, "       ");
		sprintf(buffer, fmtstr, strbuf, ptr[k - offset].rx, ptr[k - offset].tx);
		appendstring(cp, buffer);
		appendstring(cp, "\n");
	}
	if (!(count % 2)) {	/* one remaining listing */
		strftime(strbuf, 20, tmfmtstr, localtime(&ptr[k].start));
		append_general_notice(cp, "***  ");
		sprintf(buffer, fmtstr, strbuf, ptr[k].rx, ptr[k].tx);
		appendstring(cp, buffer);
		appendstring(cp, "\n");
	}
	append_general_notice(cp, "***\n");
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void sysinfo_command(struct connection *cp)
{
	char *host;
	char buffer[2048];

	host = getargcs(0, 0);
	if (host && *host == '@') {
		// be irc user friendly, blank allowed (bbs user friendly ;)
		host++;
		if (!*host)
			host = getargcs(0, 0);
	}
	if (!*host || !strcasecmp(host, myhostname) || !strcasecmp(host, "all")) {
		sprintf(buffer, "*** %s: System information [%s] - email to %s\n", myhostname, SOFT_TREE, (myemailaddr) ? myemailaddr : "*** unknown ***");
		append_general_notice(cp, buffer);
		if (mysysinfo && *mysysinfo) {
			sprintf(buffer, "*** %s: %s\n", myhostname, mysysinfo);
			append_general_notice(cp, buffer);
		}
		sprintf(buffer, "*** %s: %s version %s\n", myhostname, SOFT_TREE, SOFT_RELEASE);
		append_general_notice(cp, buffer);
		uptime_command(cp);
		if (strcasecmp(host, "all")) {
			appendprompt(cp, 0);
			return;
		}
	}
	if (cp->observer)
		return;
	if (check_user_banned(cp, "SYSINFO"))
		return;

	sprintf(buffer, "/\377\200SYSI %s %s\n", cp->name, host);
	strcpy(cp->ibuf, buffer);
	h_unknown_command(cp);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void topic_command(struct connection *cp)
{
	struct channel *ch;
	char *topic;
	char buffer[2048];
	long time;
	int channel;
	struct clist *cl;
	int state = 0;
	int prompt_stars = 0;

	if (check_user_banned(cp, "TOPIC"))
		return;

	channel = cp->channel;
	topic = getarg(0, 1);
	while (isspace(*topic & 0xff))
		topic++;
	if (*topic == '#') {
		topic++;
		while (isspace(*topic & 0xff))
			topic++;
		if (!strcasecmp(topic, "all")) {
			// list all topics
			channel = -1;
		} else
			channel = atoi(topic);
		while (*topic && !isspace(*topic & 0xff))
			topic++;
		while (*topic && isspace(*topic & 0xff))
			topic++;
	}

	if (cp->ircmode) {
		if (channel < 0) {
			sprintf(buffer, ":%s 442 %s * :No channel joined. Try /join #<channel>\n", myhostname, cp->nickname);
			appendstring(cp, buffer);
			return;
		}
		if (*topic == ':') {
			char *p = topic+1;
			while (*p && isspace(*p & 0xff))
				p++;
			if (*p)
				topic = p;
			else
				*topic = '@';	// mark deleted
		}
	}

	if (channel == -1 && *topic) {
		if (!cp->ircmode) {
			append_general_notice(cp, "*** Refusing to set topic on all channels.\n");
		}
		else {
			sprintf(buffer,":%s 482 %s #all :You're not channel operator\n",
			        myhostname, cp->nickname);
			appendstring(cp,buffer);
		}
		goto out;
	}

	time = currtime;
	if (channel == -1) {
		for (state = 0; state < 2; state++) {
			for (ch = channels; ch; ch = ch->next) {
				if (ch->expires != 0L)
					continue;
				/* print only channels with topics.. */
				cl = 0;
				if (cp->operator != 2 && cp->channel != ch->chan) {
					for (cl = cp->chan_list; cl; cl = cl->next)
						if (cl->channel == ch->chan)
							break;
				}
				if (!(ch->chan == cp->channel || cl || cp->operator == 2)) {
					if (ch->flags & M_CHAN_I)
						 continue;
					if (state == 0 && ch->flags & M_CHAN_S)
						continue;
					else if (state == 1 && !(ch->flags & M_CHAN_S))
						continue;
				} else {
					if (state == 1)
						continue;
				}

				if (*ch->topic) {
					if (ch->flags & M_CHAN_S && !(ch->chan == cp->channel || cl || cp->operator == 2))
						sprintf(buffer, "*** Topic on a secret channel, set by %s (%s):\n    ", ch->tsetby, ts2(ch->time));
					else
						sprintf(buffer, "*** Topic on channel %d, set by %s (%s):\n    ", ch->chan, ch->tsetby, ts2(ch->time));
					append_general_notice(cp, buffer);
					appendstring(cp, ch->topic);
					appendstring(cp, "\n");
				}
			}
		}
		prompt_stars = 1;
		goto out;
	}

	for (ch = channels; ch; ch = ch->next)
		if (ch->chan == channel)
			break;
	if (ch) {
		// are we on that channel?
		cl = 0;
		if (cp->operator != 2 && cp->channel != ch->chan) {
			for (cl = cp->chan_list; cl; cl = cl->next)
				if (cl->channel == ch->chan)
					break;
		}
		if ((ch->flags & M_CHAN_I || ch->flags & M_CHAN_S) && !(cp->channel == ch->chan || cl || cp->operator == 2))
			goto does_not_exist;
		if (ch->time > time)
			time = ch->time + 1L;
		if (*topic) {
			if (cp->operator == 2 || ((ch->flags & M_CHAN_T) ? ((cp->channel == ch->chan && cp->channelop) || (cl && cl->channelop)) : (cp->channel == ch->chan || cl))) {
				int is_already = 0;
				if (check_cmd_flood(cp, cp->name, myhostname, SUL_TOPIC, 1, topic))
					return;
				if ((*topic == '@' && !*ch->topic) || (*topic && !strcmp(ch->topic, topic))) {
					is_already = 1;
				}
				if (*topic == '@') {
					*topic = '\0';
					if (!cp->ircmode)
						sprintf(buffer, "*** (%s) Channel topic on channel %d %sremoved.\n", ts2(currtime), channel, (is_already ? "is already " : ""));
				} else {
					if (!cp->ircmode)
						sprintf(buffer, "*** (%s) Channel topic set on channel %d%s.\n", ts2(currtime), channel, (is_already ? " is already the same" : ""));
				}
				if (strlen(topic) > TOPICSIZE)
					topic[TOPICSIZE] = 0;
				if (cp->ircmode) {
					sprintf(buffer, ":%s TOPIC #%d :%s\n", cp->nickname, channel, topic);
				}
				if (!is_already)
					send_topic(cp->name, cp->nickname, cp->host, time, channel, topic);
			} else {
				if (!cp->ircmode)
					sprintf(buffer, "*** (%s) You are not an operator.\n", ts2(currtime));
				else
					sprintf(buffer, ":%s 482 %s #%d :You're not channel operator\n", myhostname, cp->name, channel);
			}
		} else {
			if (*ch->topic) {
				if (!cp->ircmode) {
					sprintf(buffer, "*** (%s) Current channel topic on channel %d, set by %s (%s):\n    %s\n", ts2(currtime), channel, ch->tsetby, ts2(ch->time), ch->topic);
				} else {
					sprintf(buffer, ":%s 332 %s #%d :%s",cp->host,cp->nickname ? cp->nickname : cp->name, cp->channel, ch->topic);
					buffer[IRC_MAX_MSGSIZE] = 0;
					appendstring(cp, buffer);
					appendstring(cp, "\n");
					sprintf(buffer, ":%s 333 %s #%d %s %ld\n",cp->host,cp->nickname ? cp->nickname : cp->name, channel, ch->tsetby, ch->time);
				}
				prompt_stars = 1;
			} else {
				if (!cp->ircmode)
					sprintf(buffer, "*** (%s) No current channel topic on channel %d.\n", ts2(currtime), channel);
				else
					sprintf(buffer, ":%s 331 %s #%d :There isn't a topic.\n", myhostname, cp->nickname, channel);
			}
		}
	} else {
does_not_exist:
		if (!cp->ircmode)
			sprintf(buffer, "*** (%s) Channel %d does non exist.\n", ts2(currtime), channel);
		else
			sprintf(buffer, ":%s 442 %s #%d :You're not on that channel\n", myhostname, cp->nickname, channel);
	}
	appendstring(cp, buffer);
out:
	appendprompt(cp, prompt_stars);
}

/*---------------------------------------------------------------------------*/

void uptime_command(struct connection *cp)
{
	char buffer[2048];
#if defined(linux)
	char lx[128];
	char *chp;
	long uptime;
	FILE *fd;
#endif

	if (!cp->ircmode) {
		sprintf(buffer, "*** %s: %s is up for %s QTR here is %s %s.\n", myhostname, convtype, ts4(currtime - boottime), ts2(currtime), mytimezone);
	} else {
		sprintf(buffer, ":%s 242 %s :Server Up %s\n", myhostname, cp->name, ts4(currtime - boottime));
		appendstring(cp, buffer);
		sprintf(buffer, ":%s 219 %s u :End of /STATS report\n", myhostname, cp->name);
	}
	appendstring(cp, buffer);
#if defined(linux)
	if (!access("/proc/uptime", R_OK) && (fd = fopen("/proc/uptime", "r"))) {
		if (cp->ircmode)
			send_notice(cp, myhostname, -1);
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
		sprintf(buffer, "*** %s: %s is up for %s\n", myhostname, lx, ts4(uptime));
		appendstring(cp, buffer);
	}
#endif
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void verbose_command(struct connection *cp)
{
	char buffer[2048];
	cp->verbose = 1 - cp->verbose;
	if (!cp->ircmode) {
		if (cp->verbose)
			append_general_notice(cp, "*** Verbose mode enabled.\n");
		else
			append_general_notice(cp, "*** Verbose mode disabled.\n");
	} else {
		sprintf(buffer, ":%s MODE %s %cws\n", myhostname, cp->name, cp->verbose ? '+' : '-');
		appendstring(cp, buffer);
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void version_command(struct connection *cp)
{
	char buffer[2048];

	sprintf(buffer, "*** conversd %s %s\n", SOFT_LONGNAME, SOFT_RELEASE);
	append_general_notice(cp, buffer);
	append_general_notice(cp, "    This conversd implementation was originally written by Dieter Deyke\n");
	append_general_notice(cp, "  (DK5SG) <deyke@mdddhd.fc.hp.com> and modified by Fred Baumgarten (DC6IQ)\n");
	append_general_notice(cp, "  <dc6iq@insu1.etec.uni-karlsruhe.de>.\n");

	if (!cp->ircmode)
		appendstring(cp, "\n");
	append_general_notice(cp, "    This is a version with TNOS Conference Bridge extensions.\n");
	append_general_notice(cp, "  This variant (TPP) extended and maintained by Brian A. Lantz (KO4KS).\n");

	if (!cp->ircmode)
		appendstring(cp, "\n");
	append_general_notice(cp, "    This version has also been modified by Heikki Hannikainen (OH7LZB),\n");
	append_general_notice(cp, "  <hessu@pspt.fi>, who removed some of Brian's features (which he\n");
	append_general_notice(cp, "  considered more harmful than useful) and added default channel setting\n");
	append_general_notice(cp, "  & optional channel 0 banning.\n");
	if (!cp->ircmode)
		appendstring(cp, "\n");
	sprintf(buffer, "    This version %s (%s) has bugfixes, loop-avoidance\n", SOFT_TREE, SOFT_VERSION);
	append_general_notice(cp, buffer);
	append_general_notice(cp, "  and some backports from its ancestors pp-3.13, tpp, htpp1.23, htppu1.5\n");
	append_general_notice(cp, "  made by Thomas Osterried <dl9sau>.\n");
	if (!cp->ircmode)
		appendstring(cp, "\n");
	append_general_notice(cp, "    News: Version 1.62 comes with irc-client protocol and flood protection.\n");
	if (!cp->ircmode)
		appendstring(cp, "***\n");
	if (*getarg(0, 0)) {
		if (!cp->ircmode)
			appendstring(cp, "\n");
		append_general_notice(cp, "Compiler switches:\n");
		if (!cp->ircmode)
			appendstring(cp, "\n");
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void who_command2(struct connection *cp)
{
	strcpy(cp->ibuf, "/who n");
	getarg(cp->ibuf, 0);
	who_command(cp);
}

/*---------------------------------------------------------------------------*/

void who_command(struct connection *cp)
{
	char buffer[4094];	// state of the art..
	char tmp[64];
	int flags;
	int thischan = -1;
	int full = 0;
	int quick = 1;
	int aways = 0;
	int user = 0;
	char *athost = (char *) 0;
	char channelstr[16];
	char *options;
	char *cptr;
	struct connection *p;
	struct channel *ch;
	struct clist *cl;
	int showit;
	int opstat;
	int isop;
	int isonchan;
	int realname = 0;
	int usercount = 0;
	int state = 0;
	int width = (cp->width < 1) ? 80 : cp->width;

	options = getarg(0, 0);
	switch (*options) {
	case '*':
	case '#':	/* irc compatibility */
	case 'c':	/* pp3.13 compatibility */
		quick = 0;
		thischan = cp->channel;
		if (isalnum(options[1] & 0xff))
			cptr = options+1;
		else
			cptr = getarg(0, 0);
		if (*cptr)
			thischan = atoi(cptr);
		break;
	case 'a':
		quick = 0;
		aways = 1;
		break;
	case 'l':
		quick = 0;
		full = 1;
		break;
	case 'q':
		quick = 1;
		break;
	case 'n':
		quick = 0;
		break;
	case '@':
		// allow "/who @db0tud" syntax too
		if (isalnum(options[1] & 0xff))
			athost = options+1;
		else
			athost = getargcs(0, 0);
		if (!*athost)
			athost = myhostname;
		quick = 0;
		break;
	case 'r':
		realname = 1;
		quick = 0;
		options = getargcs(0, 0);
		break;
	case 'u':
		whois_command(cp);
		return;
	case 's':
		strcpy(cp->ibuf, "/list short");
		getarg(cp->ibuf, 0);
		list_command(cp);
		return;
	case 't':
		strcpy(cp->ibuf, "/topic #all");
		getarg(cp->ibuf, 0);
		topic_command(cp);
		return;
	}

	if (quick) {
		list_command(cp);
		return;
	}
	if (realname) {
		sprintf(buffer, "User     Nickname Channel Personal\n");
		append_general_notice(cp, buffer);
		cp->locked = 0;
		for (state = 0; state < 2; state++) {
			for (ch = channels; ch; ch = ch->next) {
				if (ch->expires != 0L)
					continue;
				for (p = sort_connections(1); p; p = sort_connections(0)) {
					if (p->type != CT_USER)
						continue;
					if (p->channel != ch->chan) {
						for (cl = p->chan_list; cl; cl = cl->next)
							if (cl->channel == ch->chan)
								break;
						if (!cl)
							continue;
					}
					cl = 0;
					if (cp->operator != 2 && cp->channel != ch->chan) {
						for (cl = cp->chan_list; cl; cl = cl->next)
							if (cl->channel == ch->chan)
								break;
					}
					if (ch->flags & M_CHAN_I && !(cp->operator == 2 || cp->channel == ch->chan || cl))
						continue;
					if (ch->flags & M_CHAN_S) {
						if ((state == 0 && !(cp->operator == 2 || cp->channel == ch->chan || cl)) ||
							(state == 1 && (cp->operator == 2 || cp->channel == ch->chan || cl)))
							continue;
							
					} else {
						if (state == 1)
							continue;
					}
	
					if (!*options || !strncasecmp(options, p->nickname, strlen(options)) || !strncasecmp(options, p->name, strlen(options))) {
						if (!(ch->flags & M_CHAN_S) || cp->operator == 2 || cp->channel == ch->chan || cl) {
							sprintf(buffer, "%-8.8s %-8.8s %7d %s",
								p->name, (!strcasecmp(p->name, p->nickname)) ? "" : p->nickname, ch->chan, (p->pers && strcmp(p->pers, "@")) ? (*p->pers == '~' ? (p->pers + 1) : p->pers) : "");
						} else {
							sprintf(buffer, "%-8.8s %-8.8s  secret %s",
								p->name, (!strcasecmp(p->name, p->nickname)) ? "" : p->nickname, (p->pers && strcmp(p->pers, "@")) ? (*p->pers == '~' ? (p->pers + 1) : p->pers) : "");
						}
						if (cp->width < 1)
							buffer[80-1] = 0;
						else if (cp->width > 0 && cp->width < strlen(buffer))
							buffer[cp->width -1] = 0;
						append_general_notice(cp, buffer);
						appendstring(cp, "\n");
					}
				}
			}
		}
		appendprompt(cp, 1);
		return;
	}
	append_general_notice(cp, "User      Host       Via      Channel  ");
	if (thischan != -1) {
		if (full) {
			appendstring(cp, " Idle Queue RX      TX\n");
		} else {
			appendstring(cp, " Idle\n");
		}
	} else {
		if (full) {
			appendstring(cp, "Login Queue RX      TX\n");
		} else {
			appendstring(cp, "Login\n");
		}
	}
  for (state = 0; state < 2; state++) {
	for (ch = channels; ch; ch = ch->next) {
		if (ch->expires != 0L)
			continue;
		flags = ch->flags;
		for (p = sort_connections(1); p; p = sort_connections(0)) {
			if (p->type != CT_USER)
				continue;
			showit = 0;
			opstat = 0;
			isop = 0;
			isonchan = 0;
			if (p->channel == ch->chan) {
				opstat = p->channelop;
				showit = 1;
			}
			isop = cp->operator == 2;
			if (ch->chan == cp->channel || (!strcasecmp(p->name, cp->name) && !strcasecmp(p->host, cp->host)))
				isonchan = 1;
			else {
				for (cl = cp->chan_list; cl; cl = cl->next) {
					if (ch->chan == cl->channel) {
						isop |= cl->channelop;
						isonchan = 1;
						break;
					}
				}
			}
			for (cl = p->chan_list; cl; cl = cl->next) {
				if (ch->chan == cl->channel) {
					opstat = cl->channelop;
					showit = 1;
					break;
				}
			}
			if (thischan != -1 && (thischan != ch->chan))
				showit = 0;
			if (athost && showit && strncasecmp(athost, p->host, strlen(athost)))
				showit = 0;
			if (thischan != -1 && flags & M_CHAN_S && !(isop || ch->chan == cp->channel || isonchan))
				showit = 0;
			if (flags & M_CHAN_S) {
				if (state == 0 && !(isop || ch->chan == cp->channel || isonchan) )
					continue;
				if (state == 1 && (isop || ch->chan == cp->channel || isonchan))
					continue;
			} else {
				if (state == 1)
					continue;
			}
					
			if (showit) {
				if (!(flags & M_CHAN_I) || (isop) || (ch->chan == cp->channel) || isonchan) {
					if (!(flags & M_CHAN_S) || (isop) || (ch->chan == cp->channel) || isonchan) {
						sprintf(channelstr, "%5d", ch->chan);
					} else {
						strcpy(channelstr, "-----");
					}
					if (p->type == CT_USER) {
						sprintf(buffer, full ?
							"%-8.8s%c %-10.10s %-10.10s %5s %6s %5d %7ld %7ld" :
							"%-8.8s%c %-10.10s %-10.10s %5s %6s",
							p->name, p->observer ? 'O' : p->operator ? '!' : opstat ? '@' : ' ', p->host,
							p->via ? p->via->name : "", channelstr,
							(thischan != -1) ? (p->mtime > p->time ? ts3(currtime - p->mtime, tmp) : "-") : ts(p->time),
							queuelength(p->obuf), p->received, p->xmitted);
						if (strcasecmp(p->name, p->nickname))
							if (full) {
								strcat(buffer, "\n          Nickname: ");
								strcat(buffer, p->nickname);
							}
						usercount++;
						if ((*p->pers && strcmp(p->pers, "@") && strcmp(p->pers, "~")) || aways) {
							if (full) {
								int n;

								n = width - 21;
								strcat(buffer, "\n          Personal: ");
								strncat(buffer, (*p->pers == '~' ? (p->pers + 1) : p->pers), n);
							} else {
								strcat(buffer, " ");
								if (aways) {
									if (*p->away) {
										int n;

										n = width - 61;
										strncat(buffer, p->away, n);

										strcat(buffer, " (since ");
										strcat(buffer, ts2(p->atime));
										strcat(buffer, ")");
									} else {
										strcat(buffer, "(here)");
									}
								} else {
									int n;

									if (*p->away) {
										n = width - 53;
									} else {
										n = width - 46;
									}
									strncat(buffer, (*p->pers == '~' ? (p->pers + 1) : p->pers), n);

								}
							}
						}
						if (!aways) {
							if (*p->away) {
								if (full) {
									int n;

									n = width - 32;
									strcat(buffer, "\n          Away: ");
									strncat(buffer, p->away, n);
									strcat(buffer, " (since ");
									strcat(buffer, ts2(p->atime));
									strcat(buffer, ")");
								} else {
									strcat(buffer, " (AWAY)");
								}
							} else {
								if (full) {
									if (p->mtime && p->mtime > p->time) {
										strcat(buffer, "\n          Last Activity: ");
										strcat(buffer, ts2(p->mtime));
									}
								}
							}
#ifdef WANT_FILTER
							if (full && p->filter) {
								strcat(buffer, "\n          Filter on: ");
								strcat(buffer, p->filter);
							}
#endif

						}
						strcat(buffer, "\n");
						if (!user || (strstr(options, p->name))) {
							append_general_notice(cp, buffer);
						}
					}
				}
			}
		}
	}
  }
	if (usercount) {
		sprintf(buffer, "*** Total of %d users. ***\n", usercount);
		append_general_notice(cp, buffer);
	} else
		appendprompt(cp, 1);
}



/*---------------------------------------------------------------------------*/

void whois_command(struct connection *cp)
{
	char buffer[2048];
	int flags;
	char channelstr[32];
	struct connection *p;
	struct channel *ch;
	struct clist *cl, *cl2 = 0;
	char *name, *nextname;
	int found;
	int substring;
	char *q, *r, *s;
	int sent = 0;
	int firstrun = 1;
	int isonchan;
	int state = 0;

	name = getarg(0, 0);
	if (!name || !*name) {
		appendstring(cp, "Need username.\n");
		appendprompt(cp, 1);
		return;
	}
loop:
	nextname = getarg(0, 0);
	substring = 0;
	sent = 0;

	/* "/whois *sau" */
	if (*name == '*') {
		substring = 1;
		name++;
	}
	if (*name && (q = strchr(name, '*'))) {
		if (!substring) {
			/* "/whois dl9sau*" */
			substring = 2;
		}
		else {
			/* "/whois *sau*" */
			substring = 3;
		}
	        *q = 0;
	}
	if (!*name)
		substring = 4;

	for (p = sort_connections(1); p; p = sort_connections(0)) {
		found = 0;
		if (p->type != CT_USER)
			continue;
		switch (substring) {
		case 0:
			if (strcasecmp(p->name, name) && strcasecmp(p->nickname, name))
				continue;
			break;
		case 1:
			if (!*p->name && !*p->nickname) continue;
			s = p->name;
			for (q = s+strlen(s)-1, r = name+strlen(name)-1; q > s && r > name; q--, r--)
				if (tolower(*q & 0xff) != tolower(*r & 0xff))
					break;
			if (r == name && tolower(*q & 0xff) == tolower(*r & 0xff)) {
				// trailing substring found
				break;
			}
			s = p->nickname;
			for (q = s+strlen(s)-1, r = name+strlen(name)-1; q > s && r > name; q--, r--)
				if (tolower(*q & 0xff) != tolower(*r & 0xff)) {
					break;
				}
			if (r == name && tolower(*q & 0xff) == tolower(*r & 0xff)) {
				// trailing substring found
				break;
			}
			// no match
			continue;
		case 2:
			if (strncasecmp(p->name, name, strlen(name)) && strncasecmp(p->nickname, name, strlen(name)))
				continue;
			break;
		case 3:
			if (!strstr(p->name, name) && !strstr(p->nickname, name))
				continue;
			break;
		case 4:
			/* show */
			break;
		default:
			continue;
		}
		if (firstrun) {
			appendstring(cp, "User            Host            Via              Login Queue Receivd Xmitted\n");
			firstrun = 0;
		} else
			appendstring(cp, "\n");

		sprintf(buffer, "%-15.15s %-15.15s %-15.15s %6s %5d %7ld %7ld",
			p->name, p->host,
			p->via ? p->via->name : "", ts(p->time),
		        queuelength(p->obuf), p->received, p->xmitted);
		if (strcasecmp(p->name, p->nickname))
			sprintf(&buffer[strlen(buffer)], "\n      Nickname: %s", p->nickname);
		if (*p->pers && strcmp(p->pers, "@") && strcmp(p->pers, "~"))
			sprintf(&buffer[strlen(buffer)], "\n      Personal: %s", (*p->pers == '~' ? (p->pers + 1) : p->pers));
		if (*p->pers == '~' || p->isauth < 2)
			sprintf(&buffer[strlen(buffer)], "\n          Auth: no");
		if (*p->away)
			sprintf(&buffer[strlen(buffer)], "\n          Away: %s (since %s)", p->away, ts2(p->atime));
		else if (p->mtime && p->mtime > p->time)
			sprintf(&buffer[strlen(buffer)], "\n  Last Message: %s", ts2(p->mtime));
		if (p->idle_timeout)
			sprintf(&buffer[strlen(buffer)], "\n  Idle Timeout: %s", ts5(p->time_processed - currtime + p->idle_timeout));


		/* now print generated header */
		appendstring(cp, buffer);
		sent = 1;
		/* bugfix: non-local users fail with p->chan_list, because
		 * they have for each channel an own connection pointer(!)
		 * this lead to the bug, that channel has been showed, even
		 * if it was marked as secret :(
		 */
		if (!p->chan_list) {
			// non-local user
			flags = 0;
			isonchan = 0;
			for (ch = channels; ch; ch = ch->next) {
				if (ch->chan == p->channel) {
					flags = ch->flags;
					break;
				}
			}
			if (!ch)  {
				// channel not found?! - should never happen
				continue;
			}
			// me on channel?
			if (ch->chan == cp->channel || (!strcasecmp(p->name, cp->name) && !strcasecmp(p->host, cp->host)))
				isonchan = 1;
			else {
				for (cl2 = cp->chan_list; cl2; cl2 = cl2->next) {
					if (cl2->channel == ch->chan) {
						isonchan = 1;
						break;
					}
				}
			}
			if (cp->operator != 2 && (flags & M_CHAN_I) && !isonchan)
				continue;
			if (cp->operator == 2 || !(flags & M_CHAN_S) || isonchan)
				sprintf(channelstr, "%-7d", ch->chan);
			else
				strcpy(channelstr, "secret ");
			if (!found) {
				sprintf(buffer, "\n       Channel: %s", channelstr);
				found = 1;
			} else
				sprintf(buffer, "\n                %s", channelstr);
			if (*ch->topic && strcmp(ch->topic, "@")) {
				if (strlen(ch->topic) < 54)
					sprintf(&buffer[strlen(buffer)], " - %.53s", ch->topic);
				else if (cp->operator == 2 || !(flags & M_CHAN_S) || isonchan)
					sprintf(&buffer[strlen(buffer)], " - [Type /topic #%d for full topic]", ch->chan);
				else
					sprintf(&buffer[strlen(buffer)], " - [Type /topic <channel> for full topic]");
			}
			appendstring(cp, buffer);
		} else {
			for (state = 0; state < 2; state++) {
				for (cl = p->chan_list; cl; cl = cl->next) {
					flags = 0;
					isonchan = 0;
					for (ch = channels; ch; ch = ch->next) {
						if (ch->chan == cl->channel) {
							flags = ch->flags;
							break;
						}
					}
					if (!ch)  {
						// channel not found?! - should never happen
						continue;
					}
					// me on channel?
					if (ch->chan == cp->channel || (!strcasecmp(p->name, cp->name) && !strcasecmp(p->host, cp->host)))
						isonchan = 1;
					else {
						for (cl2 = cp->chan_list; cl2; cl2 = cl2->next) {
							if (cl2->channel == ch->chan) {
								isonchan = 1;
								break;
							}
						}
					}
					if (cp->operator != 2 && (flags & M_CHAN_I) && !isonchan)
						continue;
					if (flags & M_CHAN_S) {
						if (state == 0 && !(cp->operator == 2 || isonchan))
							continue;
						if (state == 1 && (cp->operator == 2 || isonchan))
							continue;
					} else {
						if (state == 1)
							continue;
					}
					if (cp->operator == 2 || !(flags & M_CHAN_S) || isonchan)
						sprintf(channelstr, "%-7d", ch->chan);
					else
						strcpy(channelstr, "secret ");
					if (!found) {
						/* now print generated header */
						sprintf(buffer, "\n       Channel: %s", channelstr);
						found = 1;
					} else
						sprintf(buffer, "\n                %s", channelstr);
					if (*ch->topic && strcmp(ch->topic, "@")) {
						if (strlen(ch->topic) < 54)
							sprintf(&buffer[strlen(buffer)], " - %.53s", ch->topic);
						else if (cp->operator == 2 || !(flags & M_CHAN_S) || isonchan)
							sprintf(&buffer[strlen(buffer)], " - [Type /topic #%d for full topic]", ch->chan);
						else
							sprintf(&buffer[strlen(buffer)], " - [Type /topic <channel> for full topic]");
					}
					appendstring(cp, buffer);
				}
			}
		}
#ifdef	notdef /* a user may come from more than one host or is network user and on multible channels */
		if (!substring)
			break;
#endif
	}			/* for connections */
	if (!sent) {
		sprintf(buffer, "%s%s: User not found.\n", (firstrun ? "" : "\n"), name);
		appendstring(cp, buffer);
	} else {
		appendstring(cp, "\n");
	}
	if (nextname && *nextname) {
		name = nextname;
		goto loop;
	}
	appendprompt(cp, 1);
}



/*---------------------------------------------------------------------------*/

void width_command(struct connection *cp)
{
	int neww;
	char buffer[128];
	char *arg;

	if (cp->ircmode) {
		//be quiet due to profile run
		//sprintf(buffer, ":%s 421 %s WIDTH :Unknown command\n", myhostname, cp->nickname);
		//appendstring(cp, buffer);
		return;
	}

	arg = getarg(0, 0);

	if (!*arg) {
		sprintf(buffer, "*** Current screen width is %d.\n", cp->width);
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}

	neww = atoi(arg);

	if (neww == 0) {
		if (*arg == '0') {
			cp->width = 0;
			sprintf(buffer, "*** Screen width set to 0 (unlimited linelen, unformated output).\n");
		} else {
			cp->width = (cp->ircmode ? DEFAULT_WIDTH_IRC : DEFAULT_WIDTH);
			sprintf(buffer, "*** Resetting width to %d (default).\n", cp->width);
		}
	} else {
		if (neww < 0 || neww > MAX_MTU) {
			cp->width = (cp->ircmode ? DEFAULT_WIDTH_IRC : DEFAULT_WIDTH);
			sprintf(buffer, "*** Refusing %s width %d. Resetting width to %d (default).\n", (neww > MAX_MTU ? "too large" : "negative"), neww, cp->width);
		} else {
			cp->width = neww;
			sprintf(buffer, "*** Screen width set to %d.\n", cp->width);
		}
	}
	append_general_notice(cp, buffer);
	if (!cp->ircmode) {
		if (cp->width == DEFAULT_WIDTH) // default
			sprintf(cp->ibuf, "/profile del width");
		else
			sprintf(cp->ibuf, "/profile add width %d", cp->width);
		profile_change(cp);
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void wall_command(struct connection *cp) {

	char buffer[2048];
	struct connection *p;
	char *text;

	if (cp->operator != 2) {
		if (!cp->ircmode)
			append_general_notice(cp, "*** You are not an operator.\n");
		else {
			sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
			appendstring(cp, buffer);
		}
		appendprompt(cp, 0);
		return;
	}

	text = getargcs(0, 1);
	for(p=connections; p; p=p->next) {
		if (p->type == CT_USER)
			if (!p->via)
				if (p != cp) {
					sprintf(buffer, "*** Urgent message from operator (%s):\n", cp->name);
					append_general_notice(p, buffer);
					append_general_notice(p, "    ");
					appendstring(p, text);
					appendstring(p, "\n");
					appendprompt(p, 0);
				}
		}
}

/*---------------------------------------------------------------------------*/

void wizard_command(struct connection *cp)
{
	struct connection *p;
	char *cmd;
	char buffer[1024];

	cmd = getargcs(0, 1);
	if (cp->operator != 2)  {
		if (!cp->ircmode)
			append_general_notice(cp, "*** You are not an operator.\n");
		else {
			sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
			appendstring(cp, buffer);
		}
		appendprompt(cp, 0);
		return;
	}
	else if (!cmd || !*cmd) {
		if (!cp->ircmode)
			append_general_notice(cp, "*** Not enough parameters.\n");
		else {
			sprintf(buffer, ":%s 461 %s WIZARD :Not enough parameters\n", myhostname, cp->nickname);
			appendstring(cp, buffer);
		}
	}
	else {
		sprintf(buffer, "/\377\200%s\n", cmd);
		for (p = connections; p; p = p->next) {
			if (p->type == CT_HOST)
				appendstring(p, buffer);
		}

	}
	appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

void paclen_command(struct connection *cp)
{
	char *cmd;
	char buffer[1024];

	cmd = getarg(0, 0);
	*buffer = 0;

	
	if (!*cmd) {
		sprintf(buffer, "*** Your current paclen is %d.\n", cp->mtu);
	} else {
		if ((cp->mtu = atoi(cmd)) < 64 || cp->mtu > MAX_MTU) {
			if (cp->mtu > 1) {
				sprintf(buffer, "*** A paclen %s than %d does not make sense.\n", (cp->mtu > MAX_MTU ? "greater" : "smaller"), (cp->mtu > MAX_MTU ? MAX_MTU : 64));
				append_general_notice(cp, buffer);
			}
#ifdef	HAVE_AF_AX25
			cp->mtu = ((cp->addr->sa_family == AF_AX25) ? COMP_MTU : MAX_MTU);
#else
			cp->mtu = MAX_MTU;
#endif
			sprintf(buffer, "*** Your paclen has been reset to %d.\n", cp->mtu);
			sprintf(cp->ibuf, "/profile del paclen");
		} else {
			sprintf(buffer, "*** Your paclen has been set to %d.\n", cp->mtu);
			sprintf(cp->ibuf, "/profile add paclen %d", cp->mtu);
		}
		profile_change(cp);
	}
	append_general_notice(cp, buffer);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void idle_command(struct connection *cp)
{
	char *cmd;
	char buffer[1024];

	cmd = getarg(0, 0);
	*buffer = 0;

	if (!*cmd) {
		if (!cp->idle_timeout) {
			sprintf(buffer, "*** Your idle timer is off.\n");
		}
	} else {
		if ((cp->idle_timeout = (atoi(cmd) * 60L)) < 60L) {
			cp->idle_timeout = 0;
			sprintf(buffer, "*** (%s) Your idle timeout has been switched off.\n", ts2(currtime));
			sprintf(cp->ibuf, "/profile del idle");
		} else {
			// avoid immediate kick..
			cp->time_processed = currtime;
			sprintf(buffer, "*** (%s) Your idle timeout is set to %s.\n", ts2(currtime), ts5(cp->idle_timeout));
			sprintf(cp->ibuf, "/profile add idle %d", (int ) cp->idle_timeout / 60);
		}
		profile_change(cp);
	}
	if (!*buffer)
		sprintf(buffer, "*** Your idle timeout is %s. Time left: %s\n", ts5(cp->idle_timeout), ts5(cp->time_processed - currtime + cp->idle_timeout));
	append_general_notice(cp, buffer);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void trigger_command(struct connection *cp)
{
	char buffer[2048];
	char *arg;
	int i;

	arg = getarg(0, 0);

	if (!*arg) {
		if (!cp->anti_idle_offset) {
			append_general_notice(cp, "*** Your anti-idle timer is off.\n");
		} else
			anti_idle_msg(cp);
	} else  {
		if (!strcmp(arg, "-") || !strncasecmp(arg, "off", strlen(arg)) || !(i = atoi(arg))) {
			sprintf(buffer, "*** (%s) Your anti-idle timer has been switched off.\n", ts2(currtime));
			cp->anti_idle_offset = 0;
			sprintf(cp->ibuf, "/profile del trigger");
		} else {
			cp->anti_idle_offset = i;
			sprintf(buffer, "*** (%s) Your anti-idle timer is set to %s.\n", ts2(currtime), ts5(abs(cp->anti_idle_offset) * 60L));
			sprintf(cp->ibuf, "/profile add trigger %d", cp->anti_idle_offset);
		}
		append_general_notice(cp, buffer);
		profile_change(cp);
	}
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void echo_command(struct connection *cp)
{
	char buffer[2048];
	char *line;

	line = getargcs(0, 1);

	// hmm, did the user not read carefully the //rtt instructions?
	if (*line && *line == '/' &&
		(!strncasecmp(line, "/rtt ", 5) || !strncasecmp(line+1, "/rtt ", 5) || !strcasecmp(line, "/rtt") || !strcasecmp(line+1, "/rtt"))) {
		strcpy(buffer, line);
		strcpy(cp->ibuf, buffer);
		getarg(cp->ibuf, 0);
		rtt_command(cp);
		return;
	}

	sprintf(buffer, "*** (%s) ", ts2(currtime));
	append_general_notice(cp, buffer);
	if (*line) {
		appendstring(cp, line);
		appendstring(cp, "\n");
	} else {
		appendstring(cp, "echo response\n");
	}

	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void rtt_command(struct connection *cp)
{
	char buffer[1024];
	char *line;
	time_t response_time = 0L;

	line = getarg(0, 0);

	if (cp->ircmode && *line == ':')
		line++;

	if (!line || !*line) {
		// send in upcase
		if (!cp->ircmode) {
			sprintf(buffer, "//ECHO /rtt %lx\n", currtime);
			append_general_notice(cp, buffer);
		} else {
			sprintf(buffer, "PING :%ld\n", currtime);
			appendstring(cp, buffer);
		}
		return;
	}

	// getarg will always respond with a locase line
	if (!cp->ircmode)
		sscanf(line, "%lx", &response_time);
	else
		sscanf(line, "%ld", &response_time);

	sprintf(buffer, "*** (%s) rtt", ts2(currtime));
	append_general_notice(cp, buffer);

	if (response_time < 1L) {
		sprintf(buffer, ": bad answer, sorry\n");
		appendstring(cp, buffer);
	} else {
		sprintf(buffer, " %s <-> %s is %s", myhostname, (*cp->name) ? cp->name : "you", ts5(currtime - response_time));
		if (currtime - response_time > 60) {
			appendstring(cp, buffer);
			sprintf(buffer, " (%lds)", currtime - response_time);
		}
		strcat(buffer, ".\n");
		appendstring(cp, buffer);
	}

	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

static int update_conversrc(struct connection *cp, int verbose)
{
	FILE * fi;
	FILE * fo = 0;
	char buffer[2048];
	char conversrcfile[PATH_MAX];
	char conversrcfileTmp[PATH_MAX];
	char *action, *command, *data;
	char *p, *q;
	char *for_user = 0;
	char *name;
	int act_id = 1;
	int wrote = 0;
	int found = 0;

	// locking: when profile execute's a command, this command may
	// auto-update the profile, which is _really_ not a good idea..
	// -> introduced new function_locking
	if ((cp->locked_func & LCK_PROFILE)) {
		return 0;
	}

	for_user = cp->name;
start:
	// this could happen for e.g. on /paclen (which is a CM_UNKNOWN
	// command), when the user did not yet signed on with /name:
	if (!*for_user)
		return 1;

	// add, del, show
	action = getarg(0, 0);
	switch (*action) {
	case 's':
	case 'l':
	case '=':
		// show, list
		act_id = 1;
		break;
	case 'd':
	case '-':
		// "del"
		act_id = 2;
		break;
	case 'a':
	case '+':
		// "add"
		act_id = 3;
		break;
	case 'r':
	case '!':
		act_id = 0;
		break;
	case '~':
		if (cp->operator != 2) {
			if (verbose) {
				if (!cp->ircmode)
					append_general_notice(cp, "*** You must be an operator for modifying an other user's profile.\n");
				else {
					sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
					appendstring(cp, buffer);
				}
			}
			return 1;
		}
		if (action[1])
			for_user = &action[1];
		else
			for_user = getarg(0, 0);
		goto start;
		break;
	case '?':
	case 'h':
		if (verbose)
			append_general_notice(cp, "*** usage: /profile [~ user] [add|del|list|run] [command] [args]\n");
		return 1;
		break;
	default:
		// show
		act_id = 1;
	}

	command = getarg(0, 0);
	while (*command == '/')
		command++;

	data = getargcs(0, 1);
	// rip CR/LF and trailing blanks, if any
	for (q = data; *q; q++) ;
	while (--q >= data && isspace(*q & 0xff))
		*q = 0;

	// security. and: strip ssid, find real call for his profile
	name = get_tidy_name(for_user);
	if (!name || !*name)
		return 1;
	strcpy(conversrcfile, DATA_DIR);
	strcat(conversrcfile, "/conversrc/");
	strcat(conversrcfile, name);

	strcpy(conversrcfileTmp, conversrcfile);
	strcat(conversrcfileTmp, ".tmp");

	// more sanity checks

	if (act_id == 2 && !*command) {
		// remove profile
		if (verbose)
			append_general_notice(cp, "*** Removing your profile.\n");
		unlink(conversrcfile);
		return 0;
	}
	if (act_id == 3) {
		if (!*command) {
			// nothing to add??
			if (verbose)
				append_general_notice(cp, "*** You must at least give a command.\n");
			return 1;
		}
		if (!strncasecmp("profile", command, strlen(command))) {
			// you like recursion, eh?
			if (verbose)
				append_general_notice(cp, "*** Nice try -- got you!\n");
			return 1;
		}
	}


	// rc file present?
	if ((fi = fopen(conversrcfile, "r"))) {
		// tmp file -- needed for del and add
		if (act_id > 1) {
			unlink(conversrcfileTmp); // just 2b sure
		 	if (!(fo = fopen(conversrcfileTmp, "w"))) {
				fclose(fi);
				return -1;
			}
		}
		while (fgets(buffer, sizeof(buffer)-1 /* enough space for / */, fi)) {
			// ignore empty lines
			for (p = buffer; *p && (isspace(*p & 0xff) || *p == '/'); p++);
			// rip CR/LF
			if ((q = strpbrk(p, "\r\n")))
				*q = 0;
			if (!*p)
				continue;
			switch (act_id) {
			case 0:
				// lock automatic call of /profile during profile execution
				cp->locked_func |= LCK_PROFILE;
				if (!cp->ircmode) {
					*cp->ibuf = '/';
					strcpy(cp->ibuf+1, p);
				} else
					strcpy(cp->ibuf, p);
				// execute
				process_input(cp);
				// unlock
				cp->locked_func &= ~LCK_PROFILE;
				break;
			case 1:
				if (!verbose)	// list command while non-verbose
					continue;
				if (!found) {
					// print header
					append_general_notice(cp, "*** Current profile:\n");
					found = 1;
				}
				if (cp->ircmode)
					send_notice(cp, myhostname, -1);
				appendstring(cp, "  ");
				appendstring(cp, p);
				appendstring(cp, "\n");
				break;
			default:
				if (!strncasecmp(buffer, command, strlen(command))) {
					if (found || act_id < 3) {
						// delete command, or: already found (hmm, non unique abbreviations? - delete..)
						if (verbose) {
							append_general_notice(cp, "*** Deleting entry '");
							appendstring(cp, buffer);
							appendstring(cp, "'.\n");
						}
						continue;
					}
					// change to new command
					if (*data) {
						sprintf(buffer, "%s %s", command, data);
						p = buffer;
					} else
						p = command;
					found = 1;
				}
				fprintf(fo, "%s\n", p);
				wrote++;
			}
		}
		fclose(fi);
	} else {
		// rc file not present. command < add?
		if (act_id < 3) {
			if (verbose) {
				sprintf(buffer, "*** Your profile is%s empty.\n", (act_id > 1) ? " already" : "");
				append_general_notice(cp, buffer);
			}
			// no conversrc and nothing to execute
			return 0;
		}
		// tmp file -- needed for requested add
		unlink(conversrcfileTmp); // just 2b sure
		if (!(fo = fopen(conversrcfileTmp, "w")))
			return -1;
	}

	if (act_id < 2) {
		if (act_id == 1 && found && verbose)
			append_general_notice(cp, "***\n");
		return 0;
	}

	if (act_id == 3) {
		if (!found) {
			// new entry
			if (*data) {
				sprintf(buffer, "%s %s", command, data);
				p = buffer;
			} else
				p = command;
			fprintf(fo, "%s\n", p);
			wrote++;
		}
		if (verbose)
			append_general_notice(cp, "*** Profile updated.\n");
	}
	fclose(fo);
	unlink(conversrcfile);
	if (wrote) {
		// tmp file still has data - ok, it's our new conversrc
		rename(conversrcfileTmp, conversrcfile);
	} else {
		unlink(conversrcfileTmp);
		if (verbose)
			append_general_notice(cp, "*** Your profile is now empty.\n");
	}
	return 0;
}

/*---------------------------------------------------------------------------*/

void profile_command(struct connection *cp)
{
	update_conversrc(cp, 1);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void profile_change(struct connection *cp)
{
	// skip dummy
	getarg(cp->ibuf, 0);
	update_conversrc(cp, 0);
}

/*---------------------------------------------------------------------------*/

void comp_command(struct connection *cp)
{
	char *arg;
	arg = getarg(0, 0);

	if (!strcmp(arg, "on")) {
		if ((cp->compress & CAN_COMP)) {
			fast_write(cp, "\r//COMP 1\r", -1);
			cp->compress |= IS_COMP;
		} else {
			fast_write(cp, "\r//COMP 0\r", -1);
		}
	} else if (!strcmp(arg, "off")) {
		fast_write(cp, "\r//COMP 0\r", 1);
		cp->compress &= ~IS_COMP;
	} else if (!strcmp(arg, "1")) {
		cp->compress |= IS_COMP;
	} else if (!strcmp(arg, "0")) {
		cp->compress &= ~IS_COMP;
	} else {
		char buffer[512];
		sprintf(buffer, "*** Compression is %sabled. Your socket is %sconsidered to be 8-bit clean.\n", (cp->compress & IS_COMP) ? "en" : "dis", (cp->compress & CAN_COMP) ? "" : "not ");
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
	}
}

/*---------------------------------------------------------------------------*/

void history_command(struct connection *cp)
{

	char buffer[2048];
	char header[128];
	char tmp[64];
	char tmp2[64];
	int chan = -1;
	int delete = 0;
	int sent = 0;
	int found = 0;
	int left_lines;
	int head_lines = 0;
	int tail_lines = 16;	// per default, only show the last 16 lines
	int curr_head_lines, curr_tail_lines;
	int changed;
	struct channel *ch;
	struct clist *cl;
	struct chist *chist;
	char *arg;

	arg = getarg(0, 0);

	if (*arg && ((*arg == '?') || !strncasecmp("help", arg, strlen(arg)))) {
		append_general_notice(cp, "*** usage: /history [-|delete] [[#]channel|*|all] [[+|-][lines|[0|all]]\n");
		appendprompt(cp, 0);
		return;
	}

	if (*arg && ((*arg == '-' && arg[1] == 0) || !strncasecmp("delete", arg, strlen(arg)))) {
		delete = 1;
		arg = getarg(0, 0);
	} else {
		if (*arg == '+' || *arg == '-') {
			chan = cp->channel;
			goto lines_arg;
		}
	}

	if (!*arg || *arg == '*') {
		chan = cp->channel;
	} else {
		if (arg && *arg == '#') {
			// be irc user friendly
			arg++;
		}
		if ((chan = atoi(arg)) < 0) {
			sprintf(buffer, "*** '%d' is not a valid channel number.\n", chan);
			append_general_notice(cp, buffer);
			appendprompt(cp, 0);
			return;
		}
		if (!strncasecmp("all", arg, strlen(arg))) {
			if (cp->operator != 2) {
				if (!cp->ircmode)
					append_general_notice(cp, "*** You are not an operator.\n");
				else {
					sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
					appendstring(cp, buffer);
				}
				appendprompt(cp, 0);
				return;
			}
			chan = -1;
		}
	}
	arg = getarg(0, 0);
lines_arg:
	if (*arg) {
		if (*arg == '+') {
			// print only first n lines
			head_lines = atoi(arg);
			if (head_lines < 0)
				head_lines *= -1;
			// from start
			tail_lines = 0;
		} else {
			// print n lines
			tail_lines = atoi(arg);
			if (tail_lines < 0)
				tail_lines *= -1;
		} /* else: print every line */
	}

	// he's on the channel (or operator)?
	if (cp->operator != 2) {
		for (cl = cp->chan_list; cl; cl = cl->next) {
			if (cl->channel == chan)
				break;
		}
		if (!cl) {
			if (!cp->ircmode) {
				sprintf(buffer, "*** Sorry, you are not on channel %d.\n", chan);
			} else {
				if (chan < 0)
					sprintf(buffer, ":%s 442 %s * :No channel joined. Try /join #<channel>\n", cp->host, cp->nickname ? cp->nickname : cp->name);
				else
					sprintf(buffer, ":%s 442 %s #%d :You're not on that channel\n", myhostname, cp->nickname, chan);
			}
				
			appendstring(cp, buffer);
			appendprompt(cp, 0);
			return;
		}
		if (delete && !cl->channelop) {
			if (!cp->ircmode)
				sprintf(buffer, "*** Sorry, you are not an operator on channel %d.\n", chan);
			else
				sprintf(buffer, ":%s 482 %s #%d :You're not channel operator\n", myhostname, cp->nickname, chan);
			appendstring(cp, buffer);
			appendprompt(cp, 0);
			return;
		}
	}

	// find channel
	for (ch = channels; ch; ch = ch->next) {
		if (ch->chan == chan || chan < 0) {

			found = 1;

			if (delete && ch->chist)
				expire_chist(ch, -2);

			if (!ch->chist) {
				if (chan >= 0) {
					break;
				}
				continue;
			}

			left_lines = 0;
			changed = 0;
			curr_head_lines = head_lines;
			curr_tail_lines = tail_lines;	// tail_lines == 0 -> whole list

			sent = 1;
			// how many lines?
			for (chist = ch->chist; chist; chist = chist->next)
				left_lines++;

			sprintf(buffer, "*** (%s) History for channel %d (%d lines):\n", ts2(currtime), ch->chan, left_lines);
			append_general_notice(cp, buffer);

			*tmp = 0;
			for (chist = ch->chist; chist; chist = chist->next, left_lines--) {
				if (head_lines) {
					if (!curr_head_lines) {
						// at least, still check time_last..
						continue;
					}
					curr_head_lines--;
				}
				if (curr_tail_lines) {
					if (left_lines - tail_lines > 0)
						continue;
					// tail reached, do not check again
					curr_tail_lines = 0;
				}
				sprintf(tmp2, "[%s]", ts2(chist->time));
				if (strcmp(tmp, tmp2)) {
					strcpy(tmp, tmp2);
					changed = 1;
				} else {
					changed = 0;
				}
				if (!chist->name || !*chist->name || !strcmp(chist->name, "conversd")) {
					sprintf(header, "%s ", (changed) ? tmp : "       ");
					append_general_notice(cp, header);
					if (cp->ircmode) {
						int len;
						char *p, *q;
						len = IRC_MAX_MSGSIZE - strlen(header);
						for (q = buffer, p = chist->text; *p && len; len--, p++, q++) {
							*q = (*p == '\n' ? ' ' : *p);
						}
						*q = 0;
						if (*buffer)
							appendstring(cp, buffer);
					} else {
						appendstring(cp, chist->text);
					}
					appendstring(cp, "\n");
				} else {
					sprintf(header, "%s <%s>:", (changed) ? tmp : "       ", chist->name);
					append_general_notice(cp, formatline(header, strlen(tmp)+1, chist->text, cp->width));
				}
			}
			if (ch->ltime) {
				sprintf(buffer, "*** Last message on channel %s ago.\n", ts3(currtime - ch->ltime, tmp));
				append_general_notice(cp, buffer);
			}

			if (chan >= 0)
				break;
		}
	}

	if (!found) {
		if (!cp->ircmode)
			sprintf(buffer, "*** Channel %d does not exist.\n", chan);
		else
			sprintf(buffer, ":%s 403 %s #%d :No such channel\n", myhostname, cp->nickname, chan);
		appendprompt(cp, 0);
		return;
	}

	if (!sent) {
		if (chan < 0) {
			if (delete)
				sprintf(buffer, "*** (%s) Channel history for all channels deleted.\n", ts2(currtime));
			else 
				sprintf(buffer, "*** No channel histories available.\n");

		} else {
			if (delete)
				sprintf(buffer, "*** (%s) Channel history for channel %d deleted.\n", ts2(currtime), chan);
			else
				sprintf(buffer, "*** No history for channel %d.\n", chan);
		}
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}

	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void chinfo_command(struct connection *cp)
{

	char buffer[2048];
	char tmp[64];
	struct clist *cl;
	struct channel *ch;
	char *arg;
	char *p_special;
	int chan = -1;
	int sent = 0;

	arg = getarg(0, 0);

	if (*arg == '#') {
		arg++;
	}
	if (*arg) {
		if (*arg == '*' || !strcmp(arg, "all")) {
			chan = -1;
		} else {
			if (!stringisnumber(arg) || (chan = atoi(arg)) < 0)
				chan = cp->channel;
		}
	} else {
		chan = cp->channel;
	}

	for (ch = channels; ch; ch = ch->next) {
		int isop = 1;
		if (ch->chan != chan && chan >= 0)
			continue;
		if (cp->operator != 2 && ch->expires != 0L && !(ch->locked_until != 0L || !strcasecmp(cp->name, ch->createby)))
			continue;
		if (((ch->flags & M_CHAN_I) || (ch->flags & M_CHAN_S)) && cp->operator != 2) {
			if (cp->channel != ch->chan) {
				for (cl = cp->chan_list; cl; cl = cl->next)
					if (cl->channel == ch->chan)
						break;
				if (!cl) {
					isop = 0;
					if ((ch->flags & M_CHAN_I)) {
						// not operator, but asked for a special channel
						if (chan == ch->chan)
							goto not_found;
						continue;
					}
				}
			}
		}

		if (ch->flags & M_CHAN_I)
			p_special = "invisible ";
		else if (ch->flags & M_CHAN_S)
			p_special = "secret ";
		else
			p_special = "";
		if (!isop && (ch->flags & M_CHAN_S))
			sprintf(buffer, "*** (%s) Channel statistics for secret channel:\n", ts2(currtime));
		else
			sprintf(buffer, "*** (%s) Channel statistics for %schannel #%d:\n", ts2(currtime), p_special, ch->chan);
		append_general_notice(cp, buffer);
		sprintf(buffer, "    Created by %s, %s.\n", ch->createby, ts2(ch->ctime));
		append_general_notice(cp, buffer);
		if (ch->time) {
			sprintf(buffer, "    Topic %s by %s, %s.\n", (*ch->topic) ? "set" : "removed", ch->tsetby, ts2(ch->time));
			append_general_notice(cp, buffer);
		}
		if (ch->ltime && *ch->lastby && !(!isop && (ch->flags & M_CHAN_S))) {
			sprintf(buffer, "    Last message by %s, %s ago.\n",  ch->lastby, ts3(currtime - ch->ltime, tmp));
			append_general_notice(cp, buffer);
		}
		if (ch->expires != 0L) {
			char tmp[64];
			sprintf(buffer, "    Channel is empty. Will be destroyed in %s.%s\n", ts3(ch->expires - currtime, tmp), (ch->locked_until != 0L ? " Netsplit!" : ""));
		} else {
			sprintf(buffer, "    Channel has %d users.\n", count_user(ch->chan));
		}
		append_general_notice(cp, buffer);
		if (!sent)
			sent = 1;
	}
	if (!sent) {
not_found:
		sprintf(buffer, "*** Channel %d does not exist.\n", chan);
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}
	appendprompt(cp, 1);

}

/*---------------------------------------------------------------------------*/

void cron_command(struct connection *cp)
{
	char buffer[2048];
	char tmp[64];
	char tmp2[64];
	int job_num = 0;
	int interval;
	int retries;
	int what;
	int i;
	int wrote_header;
	char *arg;
	char *q;
	struct jobs *job, *tmp_job;
	struct connection *cp2 = cp;

#define	LIST_JOBS	0
#define	DELETE_JOB	1
#define	ADD_JOB		2

	arg = getarg(0, 0);
	if (*arg == '~') {
		if (!cp->operator) {
			if (!cp->ircmode)
				append_general_notice(cp, "*** You are not an operator.\n");
			else {
				sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
				appendstring(cp, buffer);
			}
			appendprompt(cp, 0);
			return;
		}
		if (!*(++arg))
			arg = getarg(0, 0);
		if ((q = strchr(arg, '/'))) {
			*q++ = 0;
			i = atoi(q);
		} else
			i = 1;
		if (!*arg) {
			append_general_notice(cp, "*** Username needed.\n");
			appendprompt(cp, 0);
			return;
		}
		for (cp2 = connections; cp2; cp2 = cp2->next) {
			if (!strcmp(cp2->name, arg) && !strcmp(cp2->host, myhostname)) {
				if (!--i)
					break;
			}
		}
		if (!cp2) {
			sprintf(buffer, "*** User %s not found.\n", arg);
			append_general_notice(cp, buffer);
			appendprompt(cp, 0);
			return;
		}
		arg = getarg(0, 0);
	} else
		cp2 = cp;

	if (!*arg || !strncasecmp(arg, "list", strlen(arg))) {
		what = LIST_JOBS;
	} else  {
		if (*arg == '?' || !strncasecmp(arg, "help", strlen(arg))) {
			append_general_notice(cp, "*** usage: /cron [~ user[/n]] [add interval retries command|delete #job|[all]>|list]\n");
			appendprompt(cp, 0);
			return;
		}
		if (*arg == '-' || !strncasecmp(arg, "delete", strlen(arg))) {
			what = DELETE_JOB;
			arg = getarg(0, 0);
			if (*arg == '#')
				arg++;
			if ((job_num = atoi(arg)) < 0)
				job_num = 0;
		}
		else if (*arg == '+' || !strncasecmp(arg, "add", strlen(arg))) {
			what = ADD_JOB;
			arg = getarg(0, 0);
		} else
			what = ADD_JOB;
		if (what == ADD_JOB && cp2 != cp) {
			append_general_notice(cp, "*** Refusing to add a cronjob for another user.\n");
			appendprompt(cp, 0);
			return;
		}
	}

	wrote_header = 0;
	if (what == LIST_JOBS || what == DELETE_JOB) {
		for (i = 1, job = cp2->jobs; job; job = job->next, i++) {
			if (job_num && i != job_num)
				continue;
			if (what == DELETE_JOB) {
				// mark to be removed
				job->when = 0L;
			} else { /* LIST_JOBS */
				if (!wrote_header) {
					if (cp == cp2) {
						sprintf(buffer, "*** (%s) Current cron table:\n", ts2(currtime));
					} else {
						sprintf(buffer, "*** (%s) Cron table for %s:\n", ts2(currtime), cp2->name);
					}
					append_general_notice(cp, buffer);
					wrote_header++;
				}
				if (job->when && job->command) {
					sprintf(buffer, "  #%2.2d: every %s, ", i, ts3(job->interval * 60, tmp));
					append_general_notice(cp, buffer);
					if (job->retries) {
						sprintf(buffer, "%d retr%s, ", job->retries, (job->retries == 1) ? "y" : "ies");
						appendstring(cp, buffer);
					} else {
						appendstring(cp, "infinitive, ");
					}
					sprintf(buffer, "time left %s:\n", ts3(job->when - currtime, tmp2));
					appendstring(cp, buffer);
					append_general_notice(cp, "       ");
					appendstring(cp, job->command);
					appendstring(cp, "\n");
				} else {
					sprintf(buffer, " [#%2.2d: deleted]\n", i);
					append_general_notice(cp, buffer);
				}
			}
			// found
			if (job_num)
				break;
		}
		if (job_num && !job) {
			sprintf(buffer, "*** Job #%2.2d not found.\n", job_num);
			append_general_notice(cp, buffer);
		} else {
			if (what == DELETE_JOB) {
				if (job_num) {
					sprintf(buffer, "*** (%s) Crontab: job #%2.2d deleted.\n", ts2(currtime), job_num);
				} else {
					sprintf(buffer, "*** (%s) Crontab removed.\n", ts2(currtime));
				}
				append_general_notice(cp, buffer);
			} else { /* LIST_JOBS */
				if (!wrote_header) {
					append_general_notice(cp, "*** Crontab is empty.\n");
				} else {
					appendprompt(cp, 1);
					return;
				}
			}
		}
	} else {
		if (!*arg) {
			append_general_notice(cp, "*** Need an interval.\n");
			return;
		}
		interval = atoi(arg);
		if (interval < 0)
			interval = 1; /* every minute */
		arg = getarg(0, 0);
		if (!*arg) {
			append_general_notice(cp, "*** How many retries?\n");
			return;
		}
		retries = atoi(arg);
		if (retries < 0)
			retries = 0; /* infinitive */
		arg = getargcs(0, 1);
		if (*arg == '/')
			arg++;
		if (!*arg) {
			append_general_notice(cp, "*** What do you want me to do?\n");
			return;
		}
		if (!(job = (struct jobs *) hmalloc(sizeof(struct jobs))))
			return;
		job->next = 0;
		job->interval = interval;
		job->when = currtime + (interval * 60L);
		job->retries = retries;
		*buffer = '/';
		strncpy(buffer+1, arg, sizeof(buffer)-2);
		buffer[sizeof(buffer)-1] = 0;
		job->command = hstrdup(buffer);
		i = 1;
		if (cp2->jobs) {
			for (i++, tmp_job = cp2->jobs; tmp_job->next; tmp_job = tmp_job->next, i++) ;
			tmp_job->next = job;
		} else
			cp2->jobs = job;
		sprintf(buffer, "*** (%s) Added new job. You have %d job%s running.\n", ts2(currtime), i, (i == 1) ? "" : "s");
		append_general_notice(cp, buffer);
	}
	// currently, cron jobs are not added to the profile
	//update_profile.........
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void auth_command(struct connection *cp)
{

	char *arg;
	char *pw_got;
	int authtype = 0; // 0: cleartext, 1: sys, 2: md5
	char buffer[2048];
	int ret;
	char *pass = 0;

	arg = getargcs(0, 1);

	cp->needauth &= ~2;	// clear /auth input forcing

	if (*arg == ':') {
		arg++;
		while (*arg && isspace(*arg & 0xff))
			arg++;
	}
	if (!*arg) {
		if (cp->ircmode && (toupper(*cp->ibuf & 0xff) == 'P')) {
			sprintf(buffer, ":%s 461 %s PASS :Not enough parameters\n", myhostname, *cp->nickname ? cp->nickname : "*");
			appendstring(cp, buffer);
			return;
		}
	}


	if (cp->type != CT_UNKNOWN) {
		if (cp->ircmode) {
			sprintf(buffer, ":%s 462 %s :Unauthorized command (already registered)\n", myhostname, cp->nickname);
			appendstring(cp, buffer);

		} else {
			append_general_notice(cp, "*** You are already authenticated\n");
		}
		appendprompt(cp, 0);
		return;
	}

again:
	pw_got = arg;
	if (*pw_got) {
		if (!strncasecmp(arg, "sys ", strlen(arg) > 3 ? 4 : 3)) {
			authtype = 1;
			pw_got = arg + 3;
		} else if (!strncasecmp(arg, "md5 ", strlen(arg) > 3 ? 4 : 3)) {
			authtype = 2;
			pw_got = arg + 3;
		} else if (!strncasecmp(arg, "plain ", strlen(arg) > 5 ? 6 : 5)) {
			authtype = 0;
			pw_got = arg + 5;
		}

		while (*pw_got && isspace(*pw_got & 0xff))
			pw_got++;
	}

	if (!*pw_got) {
		if (!*cp->name) {
			// may be irc client connection state 1. password stored.
			appendstring(cp, "You need to say /name first\n");
			return;
		}
		
		if (authtype == 1) {
			default_authtype(cp, "sys");
			ask_pw_sys(cp, pass);
		} else if (authtype == 2) {
			default_authtype(cp, "md5");
			ask_pw_md5(cp, pass);
		} else {
			default_authtype(cp, "plain");
			ask_pw_plain(cp, pass);
		}
		cp->needauth |= 2; // read answer from stdin
	} else {

		strncpy(cp->pass_got, pw_got, sizeof(cp->pass_got));
		cp->pass_got[sizeof(cp->pass_got)-1] = 0;
	
		if (!*cp->name) {
			// may be irc client connection state 1. password stored.
			return;
		}
	}

	// no loop
	if ((cp->needauth & 8) && pass)
		return;

	if ((ret = check_password(cp)) < 0) {
		// if on auth socket, no password was found and acceptAPRSpass is set:
		if (ret == -2 && AcceptAPRSpass && !pass && (pass = compute_aprs_pass(cp->name))) {
			cp->needauth |= 8;
			goto again;
		}

		if (!cp->ircmode) {
			if (ret == -1) {
				if (*pw_got) {
					appendstring(cp, "Permission denied.\n");
					appendstring(cp, "Type /auth [sys|md5|plain] and try again.\n");
				} // else: ask state in ask_pw_* routines
			} else {
				appendstring(cp, "You are not allowed to sign on. Ask your sysop for a password.\n");
			}
		} else {
			if (ret == -1)
				sprintf(buffer, ":%s 464 %s :Password incorrect\n", myhostname, cp->nickname);
			else
				sprintf(buffer, ":%s 465 %s :You are banned from this server\n", myhostname, cp->nickname);
			appendstring(cp, buffer);
		}
		*cp->pass_got = 0;	// reset state
		appendprompt(cp, 0);
		return;
	}

	// for APRSpass
	if ((cp->needauth & 8)) {
		ret = 2;
	}

	if (ret > 0) {
		if (!cp->ircmode)
			appendstring(cp, "\n");
		if (ret == 2)
			append_general_notice(cp, "FYI: aprs-password accepted. Please set a better password.\n");
		else
//craiger		append_general_notice(cp, "FYI: No password found.\n");
//		append_general_notice(cp, "     Try /help mkpass.\n");
		if (!cp->ircmode)
			appendstring(cp, "\n");
		// mark user as unauthenticated
		cp->isauth = 1;
	} else {
		cp->isauth = 2;
	}

	if (cp->ircmode && !*cp->host) {
		// wrong order. sait NICK foo\nPASS bar
		// silently (rfc) wait for USER 
		return;
	}

	// now he has gained valid user rights
	cp->type = CT_USER;
	cp->sul = generate_sul(cp, 0, cp->sul);

	if (cp->ircmode) {
		user_login_irc_partA(cp);
	} else {
		user_login_convers_partA(cp);
	}

}

/*---------------------------------------------------------------------------*/

void mkpass_command(struct connection *cp)
{
	
	char buffer[2048];
	char *what, *user, *pw, *arg, *arg2;
	char *q;
	int todo = 0;

	what = getarg(0, 0);
	user = cp->name;
	arg = getargcs(0, 0);
	arg2 = getargcs(0, 0);


	if (*what) {
		if (!strncasecmp(what, "list", strlen(what)))
			todo = 1;	// show
		else if (*what == '-' || !strcasecmp(what, "delete") || !strcasecmp(what, "remove"))
			todo = 2;
		else if (*what == '+' || !strcasecmp(what, "change") || !strcasecmp(what, "new"))
			todo = 3;
	}

	if (todo == 3) {
		if (*arg2) {
			pw = arg2;
			user = arg;
		} else
			pw = arg;
	} else {
		if (*arg)
			user = arg;
		pw = arg2;
	}
	

	if (!*what || (!*pw && todo == 3) || (*pw && todo != 3))
		todo = 0;

	if (strcmp(user, cp->name) && cp->operator != 2) {
		if (!cp->ircmode) {
			append_general_notice(cp, "*** You are not an operator.\n");
		} else {
			sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
			appendstring(cp, buffer);
		}
		return;
	}

	if (todo == 3) {
		if (!strcasecmp(pw, "empty")) {
			pw = "";
		} else if (!strcasecmp(pw, "random")) {
			int len = atoi(getarg(0, 0));
			if (len < 1)
				len = 16;
			else if (len > PASSSIZE)
				len = PASSSIZE;
			pw = generate_rand_pw(len);
			sprintf(buffer, "*** (%s) Generated password is: %s\n", ts2(currtime), pw);
			append_general_notice(cp, buffer);

		} else if (strlen(pw) > PASSSIZE) {
			sprintf(buffer, "*** Your new password exceeds maximum length of %d and will be truncated.\n", PASSSIZE);
			append_general_notice(cp, buffer);
			pw[PASSSIZE] = 0;
		}
		for (q = pw; *q; q++) {
			if (*q == ':' || *q == '#' || *q == '/' || !isalnum(*q & 0xff)) {
				sprintf(buffer, "*** Sorry, '%c' is not a valid character. Action aborted.\n", *q);
				append_general_notice(cp, buffer);
				return;
			}
		}
	}


	switch (todo) {
	case 1:
		if ((pw = read_password(user))) {
			if (*pw)
				sprintf(buffer, "*** Current password is '%s'\n", pw);
			else
				sprintf(buffer, "*** Empty password found.\n");
		} else
			sprintf(buffer, "*** No password found.\n");
		break;
	case 2:
		if (update_password(user, 0) >= 0)
			sprintf(buffer, "*** Password removed.\n");
		else
			sprintf(buffer, "*** Errors have occured. No changes made.\n**** Pease report this to your sysop.\n");
		break;
	case 3:
		if (update_password(user, pw) >= 0)
			sprintf(buffer, "*** Password updated.\n");
		else
			sprintf(buffer, "*** Errors have occured. No changes made.\n*** Pease report this to your sysop.\n");
		break;
	default:
		sprintf(buffer, "*** Usage: mkpass <new|change|list|delete> [user] [yournewpass|random|empty]\n");
	}
	append_general_notice(cp, buffer);
	appendprompt(cp, 0);

}

/*---------------------------------------------------------------------------*/

void print_his_destinations(struct connection *cp, struct destination *d, int n_users, char *offset)
{
	struct destination *d2, *d3;
	char buffer[512];
	int i;

	for (d2 = destinations; d2; d2 = d2->next) {
		// has next?
		if (d2->locked || !d2->link || !d2->rtt)
			continue;
		if (strcasecmp(d2->hisneigh, d->name))
			continue;
		for (d3 = d2->next; d3; d3 = d3->next) {
			if (d3->locked || !d3->link || !d3->rtt || strcasecmp(d3->hisneigh, d->name))
				continue;
			break;
		}
		if (cp->ircmode) {
			sprintf(buffer, ":%s 015 %s :", myhostname, cp->nickname);
			appendstring(cp, buffer);
		}
		appendstring(cp, offset);
		i = count_user2(d2->name);
		// actualize hop count (hack)
		//if (d->hops <= d2->hops)
			//d2->hops = d->hops+1;
		sprintf(buffer, " %c- %s (%lds) #%d [%d | %0.1f%c | %s]\n", (d3) ? '|' : '`', d2->name, d2->rtt, d2->hops, i, (100 * (i / (float ) n_users)), '%', d2->rev);
		appendstring(cp, buffer);
		if (strlen(offset) + 6 + HOSTNAMESIZE + 2 + 16 + 3 + 1 + 1 < 256)
			sprintf(buffer, "%s %s ", offset, (d3) ? "|" : "  ");
		else
			strcpy(buffer, "..");
		d2->locked = 1;
		print_his_destinations(cp, d2, n_users, buffer);
	}
}

/*---------------------------------------------------------------------------*/

void map_command(struct connection *cp)
{

	char buffer[2048];
	struct permlink *pp;
	struct destination *d;
	int pl;
	int n_users;
	int n_dest;
	int n_permlinks;
	int i;

	n_users = count_user2(0);

	for (n_dest = 1 /* we */, d = destinations; d; d = d->next) {
                d->locked = 0;	// need to unlock
		if (d->rtt && d->link)
			n_dest++;
	}

	if (cp->ircmode) {
		sprintf(buffer, ":%s 018 %s :", myhostname, cp->nickname);
		appendstring(cp, buffer);
	}
	sprintf(buffer, "*** (%s) %d Server (rtt/s) #hops [users | %c (of %d total) | revision]\n", ts2(currtime), n_dest, '%', n_users);
	appendstring(cp, buffer);

	if (n_users < 1) // avoid div by zero
		n_users = 1;

	if (!cp->ircmode) {
		i = strlen(buffer)-1;
		while (i) {
			buffer[--i] = '-';
		}
		appendstring(cp, buffer);
	}

	// me
	if (cp->ircmode) {
		sprintf(buffer, ":%s 015 %s :", myhostname, cp->nickname);
		appendstring(cp, buffer);
	}
	if ((i = count_user2(myhostname)) < 1)
		i = 0;
	sprintf(buffer, "%s (it's me) #0 [%d | %0.1f%c | %s]\n", myhostname, i, (100 * (i / (float ) n_users)), '%', SOFT_RELEASE);
	appendstring(cp, buffer);

	if (!permarray) {
		appendprompt(cp, 1);
		return;
	}

        for (n_permlinks = 0, pl = 0; pl < NR_PERMLINKS; pl++) {
		if (!(pp = permarray[pl]))
			continue;
		pp->locked_func = 0;
		n_permlinks++;
	}
		
	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if (!(pp = permarray[pl]))
			continue;
		if (pp->locked_func)
			continue;
		if (pp->permanent && pp->primary)
			continue;
		pp->locked_func = 1;

again:
		n_permlinks--;

		if (cp->ircmode) {
			sprintf(buffer, ":%s 015 %s :", myhostname, cp->nickname);
			appendstring(cp, buffer);
		}
		sprintf(buffer, " %c- %s:%s ", n_permlinks ? '|' : '`', pp->permanent ? (pp->primary ? (pp->permanent != 2 ? "B" : "b") : (pp->permanent != 2 ? "P" : "p")) : "l", pp->name);
		if (!pp->connection || !(d = find_destination(pp->connection->name))) {
			strcat(buffer, "<dead>\n");
			appendstring(cp, buffer);
			goto end;
		}

		i = (count_user2(pp->name));
		sprintf(buffer+strlen(buffer), "(%lds) #1 [%d | %0.1f%c | %s]\n", d->rtt, i, (100 * (i / (float ) n_users)), '%', d->rev);
		appendstring(cp, buffer);

		d->locked = 1;
		sprintf(buffer, " %s  ", n_permlinks ? "|" : " ");
		print_his_destinations(cp, d, n_users, buffer);
end:
		if (pp->backup && !pp->backup->locked_func) {
			pp = pp->backup;
			goto again;
		}
	}

	if (cp->ircmode) {
		sprintf(buffer, ":%s 017 %s :End of /MAP\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
	}
	appendprompt(cp, 1);

	for (pl = 0; pl < NR_PERMLINKS; pl++) {
		if ((pp = permarray[pl]))
			pp->locked_func = 0;
	}
}

/*---------------------------------------------------------------------------*/

void banlist_command(struct connection *cp)
{

	char buffer[2048];
	char tmp [16];
	struct ban_list *bl;
	char *arg;

	arg = getarg(0, 0);

	if (*arg) {
		if (!strcmp(arg, "-") || !strncasecmp(arg, "delete", strlen(arg))) {
			if (cp->operator != 2) {
				if (!cp->ircmode) {
					append_general_notice(cp, "*** You are not an operator.\n");
				} else {
					sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
					appendstring(cp, buffer);
				}
				return;
			}
			arg = getarg(0, 0);
			if (!*arg) {
				if (!cp->ircmode) {
					append_general_notice(cp, "*** Not enough parameters.\n");
				} else {
					sprintf(buffer, ":%s 461 %s BAN :Not enough parameters\n", myhostname, *cp->nickname ? cp->nickname : "*");
					appendstring(cp, buffer);
				}
				return;
			}
			while (*arg) {
				sprintf(buffer, "Deleted localy by %s", cp->name);
				for (bl = banned_users; bl; bl = bl->next) {
					if (!strcasecmp(bl->name, arg)) {
						bl->until = currtime + 30;
						bl->announced_from[0] = '-';
						bl->announced_from[1] = 0;
						strncpy(bl->reason, buffer, sizeof(bl->reason)-1);
						bl->reason[sizeof(bl->reason)-1] = 0;
						break;
					}
					
				}
				if (bl)
					sprintf(buffer, "*** (%s) Marked %s for deletion.\n", ts2(currtime), arg);
				else
					sprintf(buffer, "*** (%s) User not found: %s.\n", ts2(currtime), arg);
				append_general_notice(cp, buffer);
				arg = getarg(0, 0);
			}
			appendprompt(cp, 1);
			return;
		}

		sprintf(buffer, "*** Sorry, the banlist could not be manually modified.\n");
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}

	if (ban_statistic_bans && ban_statistic_last_ban && ban_statistic_last_user && ban_statistic_last_by && ban_statistic_last_reason) {
		sprintf(buffer, "*** (%s) Ban statistic: %d ban%s, last by %s: %s (%s ago).\n", ts2(currtime), ban_statistic_bans, (ban_statistic_bans == 1 ? "" : "s"), ban_statistic_last_by, ban_statistic_last_user, ts3(currtime - ban_statistic_last_ban, tmp));
		append_general_notice(cp, buffer);
		sprintf(buffer, "    Reason: %s\n", ban_statistic_last_reason);
		append_general_notice(cp, buffer);
	}

	if (!banned_users) {
		sprintf(buffer, "*** (%s) Currently, the banlist is empty.\n", ts2(currtime));
		append_general_notice(cp, buffer);
		appendprompt(cp, 0);
		return;
	}


	sprintf(buffer, "*** (%s) The following users are currently banned:\n", ts2(currtime));
	append_general_notice(cp, buffer);
	sprintf(buffer, "    User              From      Since   Until  Last  Reason\n");
	append_general_notice(cp, buffer);

	for (bl = banned_users; bl; bl = bl->next) {
		sprintf(buffer, "    %-16s  %-8s  %-6s  ", bl->name, bl->announced_from, ts2(bl->time));
		append_general_notice(cp, buffer);
		sprintf(buffer, "%-6s %4.4s  %s\n", (bl->until ? ts2((bl->until > currtime ? bl->until : currtime)) : "  -   "), ts3(currtime - bl->time_refreshed, tmp), bl->reason);
		appendstring(cp, buffer);
	}
	if (cp->ircmode)
		append_general_notice(cp, "***\n");
	appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

void banparam_command(struct connection *cp)
{
	char buffer[2048];
	char tmp[64];
	char *arg;

	arg = getarg(0, 0);

	if (*arg) {
		if (cp->operator != 2) {
			if (!cp->ircmode) {
				append_general_notice(cp, "*** You are not an operator.\n");
			} else {
				sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
				appendstring(cp, buffer);
			}
			return;
		}
		// more: 2b implemented
		//...
		//
		//return;
	}

	sprintf(buffer, "*** (%s) Current Flood Protection settings at %s:\n", ts2(currtime), myhostname);
	append_general_notice(cp, buffer);

	append_general_notice(cp, "    Ban: ");
	if (!BanTime) {
		sprintf(buffer, "Expires never");
	} else {
		sprintf(buffer, "Expires %s", ts3((BanTime * 60L), tmp));
	}
	appendstring(cp, buffer);
	appendstring(cp, "\n");
	sprintf(buffer, "         DoBan %s\n", ((FeatureBAN & DO_BAN) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Announce %s\n", ((FeatureBAN & ANNOUNCE_BAN) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Accept %s\n", ((FeatureBAN & ACCEPT_BAN) ? "on" : "off"));
	append_general_notice(cp, buffer);
	append_general_notice(cp, "    Flood protection:\n");
	sprintf(buffer, "         Local Server Floods:  %s\n", ((FeatureFLOOD & NO_SERVER_FLOOD) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Local Command Floods: %s\n", ((FeatureFLOOD & NO_LOCAL_COMMAND_FLOOD) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Local Join Flood: %s\n", ((FeatureFLOOD & NO_LOCAL_JOIN_FLOOD) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Remote Message Floods: %s\n", ((FeatureFLOOD & NO_REMOTE_MESSAGE_FLOOD) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Remote Command Floods: %s\n", ((FeatureFLOOD & NO_REMOTE_COMMAND_FLOOD) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Remote Join Flood: %s\n", ((FeatureFLOOD & NO_REMOTE_JOIN_FLOOD) ? "on" : "off"));
	append_general_notice(cp, buffer);
	sprintf(buffer, "         RateLimitContent: %d\n", RateLimitContent);
	append_general_notice(cp, buffer);
	sprintf(buffer, "         RateLimitLine: %d\n", RateLimitLine);
	append_general_notice(cp, buffer);
	sprintf(buffer, "         RateLimitMAX: %d\n", RateLimitMAX);
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Sul Command Delay: %d\n", SulCmdDelay);
	append_general_notice(cp, buffer);
	sprintf(buffer, "         Sul Lifetime: %s\n", ts3(SulLifeTime, tmp));
	append_general_notice(cp, buffer);


	if (cp->ircmode)
		append_general_notice(cp, "***\n");
	appendprompt(cp, 1);

}

/*---------------------------------------------------------------------------*/

void listsul_command(struct connection *cp)
{

	struct static_user_list *sul;
	char buffer[2048];
	char tmp[64];
	char *arg;
	struct ban_list *isbanned;
	int i;
	int all_suls = 0;

	arg = getarg(0, 0);
	if (*arg) {
		if (cp->operator != 2) {
			if (!cp->ircmode) {
				append_general_notice(cp, "*** You are not an operator.\n");
			} else {
				sprintf(buffer, ":%s 481 %s :Permission Denied - You're not an IRC operator\n", myhostname, cp->name);
				appendstring(cp, buffer);
			}
			return;
		}
		all_suls = 1;
	}
	

	if (!list_of_static_users) {
		sprintf(buffer, "*** (%s) Currently, the sul list is empty.\n", ts2(currtime));
		append_general_notice(cp, buffer);
		return;
	}
	sprintf(buffer, "*** (%s) List of static users:\n", ts2(currtime));
	append_general_notice(cp, buffer);
	for (sul = list_of_static_users; sul; sul = sul->next) {
		struct sul_sp *sp;
		int sul_header_sent = 0;
#define	prepend_sul_header() { \
		if (!sul_header_sent) { \
			sprintf(buffer, "    Sul %lx Name %s Via %s%s%s%s - Last referenced %s\n", (long ) sul, sul->name, sul->host, (sul->via ? " (" : ""), (sul->via ? sul->via->name : ""), (sul->via ? ")" : ""), ts2(sul->last_updated)); \
			append_general_notice(cp, buffer); \
			sul_header_sent = 1; \
		} \
}
		if (all_suls)
			prepend_sul_header();
		isbanned = is_banned(sul->name);
		if (isbanned) {
			prepend_sul_header();
			sprintf(buffer, "      Banned: %s\n", isbanned->reason);
			append_general_notice(cp, buffer);
		}

		for (i = 0; i < SUL_CMD_MAX; i++) {
			if (sul->time_nextcmd[i] > currtime || sul->num_floods[i]) {
				prepend_sul_header();
				sprintf(buffer, "      %s floods %d, blocked until %s\n", sul_cmd_names[i], sul->num_floods[i], ts2(sul->time_nextcmd[i]));
				append_general_notice(cp, buffer);
			}
		}
		if (sul->stop_process_until > currtime) {
			prepend_sul_header();
			sprintf(buffer, "      Process Delay: %s\n", ts3(sul->stop_process_until - currtime, tmp));
			append_general_notice(cp, buffer);
		}

		if (all_suls && (sp = sul->sp)) {
			append_general_notice(cp, "     ");
			for (sp = sul->sp, i = 0; sp; sp = sp->next) {
				struct connection *p = sp->connection;
				if (!*p->name) { 
					sprintf(buffer, " %s-%d", p->type != CT_UNKNOWN ? "unknown" : "signon", ++i);
					appendstring(cp, buffer);
				} else if (p->type == CT_HOST) {
					sprintf(buffer, " %s", p->name);
					appendstring(cp, buffer);
				} else {
					struct clist *cl;
					sprintf(buffer, " %s@%s:", p->name, p->host);
					appendstring(cp, buffer);
					if (!p->chan_list) {
						sprintf(buffer, "%d:", p->channel);
						appendstring(cp, buffer);
					} else {
						for (cl = p->chan_list; cl; cl = cl->next) {
							sprintf(buffer, "%d:", cl->channel);
							appendstring(cp, buffer);
						}
					}
				}
			}
			appendstring(cp, "\n");
		}
		if (sul->delayqueue || sul->delayqueue_len) {
			struct sul_qp *qp;
			for (qp = sul->delayqueue, i = 0; qp; qp = qp->next, i++) ;
			prepend_sul_header();
			append_general_notice(cp, "      ");
			sprintf(buffer, "Delayqueue: Size %d, Messages %d\n", sul->delayqueue_len, i);
			append_general_notice(cp, buffer);
		 }
	}
	if (cp->ircmode)
		append_general_notice(cp, "***\n");
	appendprompt(cp, 1);
}

/*---------------------------------------------------------------------------*/

int is_hushlogin(struct connection *cp)
{
	return handle_hushlogin(cp, 0);	
}

/*---------------------------------------------------------------------------*/

int update_hushlogin(struct connection *cp, int what)
{
	return handle_hushlogin(cp, (what ? 1 : 2));	
}

/*---------------------------------------------------------------------------*/

int handle_hushlogin(struct connection *cp, int what)
{

	char conversrcfileHush[PATH_MAX];
	struct stat statbuf;
	int fd;
	char *name;

	if (!cp || !*cp->name)
		return 0;

	name = get_tidy_name(cp->name);
	if (!*name)
		return 0;

	strcpy(conversrcfileHush, DATA_DIR);
	strcat(conversrcfileHush, "/conversrc/");
	strcat(conversrcfileHush, name);
	strcat(conversrcfileHush, ".hush");

	if (!what)
		return (stat(conversrcfileHush, &statbuf) ? 0 : 1);

	if (what == 1) {
		if ((fd = open(conversrcfileHush, O_WRONLY | O_CREAT, 0664)) >= 0) {
			close(fd);
			return 0;
		}
		return -1;
	}

	return unlink(conversrcfileHush);
}

/*---------------------------------------------------------------------------*/

void hushlogin_command(struct connection *cp)
{
	int status;
	char buffer[2048];

	if (!*cp->name || cp->type != CT_USER) {
		cp->verbose = 0;
		return;
	}

	status = is_hushlogin(cp);
	update_hushlogin(cp, !status);

	sprintf(buffer, "*** (%s) Switched hushlogin %s.\n", ts2(currtime), status ? "off" : "on");
	append_general_notice(cp, buffer);
	appendprompt(cp, 0);
}

/*---------------------------------------------------------------------------*/

void shownicks_command(struct connection *cp)
{
	char buffer[2048];
	char *arg;

	if (cp->ircmode) {
		sprintf(buffer, ":%s 421 %s SHOWNICKS :Unknown command (currently not available for ircmode; always on)\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	arg = getarg(0, 0);
	if (*arg != 's')
		cp->shownicks = !cp->shownicks;
	sprintf(buffer, "*** (%s) Nicknames are %sshown.\n", ts2(currtime), cp->shownicks ? "" : "not ");
	append_general_notice(cp, buffer);
	appendprompt(cp, 0);
	if (*arg != 's') {
		// shownicks is default
		sprintf(cp->ibuf, "/profile %s shownicks toggle", cp->shownicks ? "del" : "add");
		profile_change(cp);
	}
}

/*---------------------------------------------------------------------------*/

#ifdef WANT_FILTER
void filtermsgs_command(struct connection *cp)
{
	char buffer[2048];
	char *arg;

	arg = getarg(0, 0);
	if (*arg && !isdigit(*arg & 0xff)) {
		append_general_notice(cp, "*** Filtermsgs does finetuning on private and channel messages to you.\n");
				
		append_general_notice(cp, "    Values are bitwise ORed, for ignoring:\n");
		append_general_notice(cp, "      private msgs from  1: users  4: conversd\n");
		append_general_notice(cp, "      channel            2: users  8: conversd\n");
		append_general_notice(cp, "    A value of 0 means no filtering (default).\n");
		append_general_notice(cp, "    Do not filter conversd messages if there is not *really* a reason for.\n");
		if (cp->ircmode)
			append_general_notice(cp, "***\n");
		appendprompt(cp, 1);
		return;
	}
	if (*arg)
		cp->filtermsgs = atoi(arg);
	sprintf(buffer, "*** (%s) Filtermsgs is %s%d.\n", ts2(currtime), (*arg ? "set to " : ""), cp->filtermsgs);
	append_general_notice(cp, buffer);
	appendprompt(cp, 0);
	if (!*arg)
		return;
	sprintf(cp->ibuf, "/profile %s filtermsgs %d", cp->filtermsgs ? "add" : "del", cp->filtermsgs);
	profile_change(cp);
	return;
}
#endif

/*---------------------------------------------------------------------------*/

void timestamp_command(struct connection *cp)
{
	char buffer[2048];
	char extra_info[64];

	if (cp->ircmode) {
		sprintf(buffer, ":%s 421 %s TIMESTAMP :Unknown command\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}
	
	*extra_info = 0;
	if (cp->timestamp == 0L) {
		cp->timestamp = currtime - 60L;
	} else {
		time_t days = (currtime - cp->timestamp) / ONE_DAY;
		if (days > 0L) {
			sprintf(extra_info, " [last msg %ld day%s ago]", days, (days > 1) ? "s" : "");
		}
		cp->timestamp = 0L;
	}
	sprintf(buffer, "*** (%s) Timestamping switched %s%s.\n", ts2(currtime), (cp->timestamp == 0L ? "off" : "on"), extra_info);
	append_general_notice(cp, buffer);
	appendprompt(cp, 0);
	sprintf(cp->ibuf, "/profile %s timestamp", (cp->timestamp == 0L ? "del" : "add"));
	profile_change(cp);
}
