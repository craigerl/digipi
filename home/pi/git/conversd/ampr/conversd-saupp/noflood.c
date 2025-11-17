/*
 * Flood and Abuse Protection for saupp
 *
 * (c) 2002 Thomas Osterried <dl9sau>
 * License: GPL
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>

#include "conversd.h"
#include "log.h"
#include "hmalloc.h"
#include "config.h"
#include "cfgfile.h"
#include "noflood.h"
#include "access.h"
#include "user.h"

char *sul_cmd_names[] = {
  "unknown",
  "away",
  "invite",
  "join",
  "mode",
  "nick",
  "pers",
  "topic",
  "spam",
  "games",
  0
};

struct ban_list *banned_users = 0;
struct static_user_list *list_of_static_users = 0;

unsigned int RateLimitMAX = RATELIMIT_MAX;
unsigned int RateLimitLine = RATELIMIT_LINE;
unsigned int RateLimitContent = RATELIMIT_CONTENT;

unsigned int SulCmdDelay = SUL_CMD_DELAY;
unsigned int SulLifeTime = SUL_LIFETIME;

time_t BanTime = (time_t ) BANTIME;

char *ban_statistic_last_user = 0;
char *ban_statistic_last_by = 0;
char *ban_statistic_last_reason = 0;
int ban_statistic_bans = 0;
time_t ban_statistic_last_ban = 0;

int FeatureFLOOD = NO_SERVER_FLOOD + NO_LOCAL_COMMAND_FLOOD + NO_REMOTE_COMMAND_FLOOD + NO_REMOTE_MESSAGE_FLOOD + NO_LOCAL_JOIN_FLOOD + NO_REMOTE_JOIN_FLOOD;
int FeatureBAN = DO_BAN + ANNOUNCE_BAN + ACCEPT_BAN;

void send_ban(struct connection *cp, struct ban_list *bl)
{
        char buffer[2048];
	char tmp[64];
	long minutes;
	if (bl->until < currtime + 60L)
		return;
	minutes = (bl->until - currtime) / 60L;
	if (cp->type == CT_HOST) {
		if (!(FeatureBAN & ANNOUNCE_BAN))
			return;
        	sprintf(buffer, "/\377\200BAN %s %s %ld %ld %s\n", bl->name, bl->announced_from, bl->time_refreshed, minutes, bl->reason);
		appendstring(cp, buffer);
	} else {
		sprintf(buffer, "*** (%s) Banned %s for %s: %s\n", ts2(currtime), bl->name, ts3((bl->until - currtime), tmp), bl->reason);
       		append_general_notice(cp, buffer);
	}
}

/*--------------------------------------------------------------------------*/

void announce_banned_users(struct connection *cp)
{
	struct ban_list *bl;
	for (bl = banned_users; bl; bl = bl->next)
		send_ban(cp, bl);
}


/*--------------------------------------------------------------------------*/

void announce_ban(struct ban_list *bl)
{
        struct connection *p;
	
	if (bl->announce_next > currtime)
		return;
	bl->announce_next = currtime + (BanTime * 60L);
        for (p = connections; p; p = p->next) {
                if ((p->type == CT_HOST && !p->locked) || (p->type == CT_USER && p->operator && (!p->ircmode || p->verbose))) {
			send_ban(p, bl);
                }
        }
}

/*--------------------------------------------------------------------------*/

void ban_user(char *user, char *announced_from, struct static_user_list *sul, time_t when, time_t minutes, char *reason)
{

	struct ban_list *bl;
	char *name;
	
	name = get_tidy_name(user);

	// someone's fooling us?
	if (!*name || !strcasecmp(name, "conversd"))
		return;

	if (minutes < 1)
		minutes = (minutes < 0 ? 0 : BanTime);

	for (bl = banned_users; bl; bl = bl->next) {
		if (!strcasecmp(bl->name, name))
			break;
		// store ancestor
	}
	if (!bl) {
		if (!(bl = (struct ban_list *) hmalloc(sizeof(struct ban_list))))
			return; // no mem
		strncpy(bl->name, name, sizeof(bl->name)-1);
		bl->name[sizeof(bl->name)-1] = 0;
		bl->next = banned_users;
		banned_users = bl;
		bl->time = when;
		bl->announce_next = currtime;
		ban_statistic_bans++;
		ban_statistic_last_ban = currtime;
		if (ban_statistic_last_user)
			hfree(ban_statistic_last_user);
		ban_statistic_last_user = hstrdup(name);
		if (ban_statistic_last_by)
			hfree(ban_statistic_last_by);
		ban_statistic_last_by = hstrdup(announced_from);
		if (ban_statistic_last_reason)
			hfree(ban_statistic_last_reason);
		ban_statistic_last_reason = hstrdup(reason);
		do_log(L_INFO, "BAN %s by %s: %s", name, announced_from, reason);
	}
	strncpy(bl->announced_from, announced_from, sizeof(bl->announced_from)-1);
	bl->announced_from[sizeof(bl->announced_from)-1] = 0;
	strncpy(bl->reason, reason, sizeof(bl->reason)-1);
	bl->reason[sizeof(bl->reason)-1] = 0;
	bl->time_refreshed = when;
	// time update
	bl->until = (minutes ? (currtime + minutes * 60L) : 0L);

	if (sul)
		free_sul_delayqueue(sul);
	announce_ban(bl);
}

/*--------------------------------------------------------------------------*/

struct ban_list *is_banned(char *n)
{
	struct ban_list *bl;
	char *name;
	
	if (!(FeatureBAN))
		return 0;

	name = get_tidy_name(n);

	for (bl = banned_users; bl; bl = bl->next) {
		if (!strcasecmp(bl->name, name))
			return bl;
	}
		
	return 0;
}

/*--------------------------------------------------------------------------*/

void expire_banlist(void)
{
	struct ban_list *bl, *bl_prev, *bl_next;
	static time_t last_run = 0L;

        if (currtime - last_run < 60L)
		return;

	for (bl = bl_prev = banned_users; bl; ) {
		if (!bl->until || bl->until > currtime) {
			announce_ban(bl);
			bl_prev = bl;
			bl = bl->next;
			continue;
		}
		if (bl == banned_users) {
			banned_users = bl_prev = bl->next;
		} else if (bl_prev) {
			bl_prev->next = bl->next;
		}
		bl_next = bl->next;
		hfree(bl);
		bl = bl_next;
	}
		
	last_run = currtime;
}

/*--------------------------------------------------------------------------*/

struct static_user_list *get_sul(char *user, struct connection *via_p)
{
	struct static_user_list *sul;
	char name[NAMESIZE+1];

	strncpy(name, get_tidy_name(user), NAMESIZE);
	name[NAMESIZE] = 0;

	for (sul = list_of_static_users; sul; sul = sul->next) {
		if (sul->via != via_p)
			continue;
		if (!strcasecmp(sul->name, name))
			return sul;
	}
	// not found
	return 0;
}

/*--------------------------------------------------------------------------*/

static void relink_sul(struct connection *cp)
{
	struct static_user_list *sul;
	for (sul = list_of_static_users; sul; sul = sul->next) {
		if (!strcasecmp(sul->host, cp->name))
			sul->via = cp;
	}
}

/*--------------------------------------------------------------------------*/

void free_sul_sp(struct connection *cp)
{
	struct sul_sp *sp, *sp_prev;
	for (sp = sp_prev = cp->sul->sp; sp; sp = sp->next) {
                if (sp->connection == cp) {
                        if (sp_prev == sp) {
                                cp->sul->sp = cp->sul->sp->next;
                        } else {
                                sp_prev->next = sp->next;
                        }
                        hfree(sp);
                        break;
                }
                sp_prev = sp;
        }
}


/*--------------------------------------------------------------------------*/

struct static_user_list *generate_sul(struct connection *cp, struct connection *via_p, struct static_user_list *oldsul)
{
	struct static_user_list *sul;
	struct sul_sp *sp;
	char name[NAMESIZE+1];
	int i;

	if (!*cp->name)
		strcpy(name, "[login]");
	else if (!strcasecmp(cp->name, "conversd"))
		strcpy(name, "[unknown]");
	else strncpy(name, get_tidy_name(cp->name), NAMESIZE);
	name[NAMESIZE] = 0;

	if (!(sul = get_sul(name, via_p))) {
		// a new sul
		if (!(sul = (struct static_user_list *) hcalloc(1, sizeof(struct static_user_list)))) {
			return 0;
		}
		sul->delayqueue = 0;	// just 2b sure
		sul->delayqueue_len = 0;
		// link
		sul->next = list_of_static_users;
		list_of_static_users = sul;

		// copy user data
		strncpy(sul->name, name, sizeof(sul->name)-1);
		sul->name[sizeof(sul->name)-1] = 0;
		if (via_p) {
			// via permlink
			sul->via = via_p; // but permlink may break. remember permlinks name
			strncpy(sul->host, via_p->name, sizeof(sul->host)-1);
		} else {
			// local user
			sul->via = 0;
			strncpy(sul->host, myhostname, sizeof(sul->host)-1);
		}
		sul->host[sizeof(sul->host)-1] = 0;

		sul->stop_process_until = currtime;
		// initialize timers
		for (i = 0; i < SUL_CMD_MAX; i++) {
			sul->time_nextcmd[i] = currtime - SulCmdDelay;
			sul->num_floods[i] = 0;
		}
	} else {
		if (!sul->via && via_p) {
			// not local. permlink back again
			sul->via = via_p;
			strncpy(sul->host, via_p->name, sizeof(sul->host)-1);
			sul->host[sizeof(sul->host)-1] = 0;
		}
		// accept /away /pers /nick etc.. update once
		for (i = 0; i < SUL_CMD_MAX; i++) {
			if (sul->num_floods[i])
				sul->num_floods[i] -= 1;
			if (sul->time_nextcmd[i] <= currtime + SulCmdDelay)
				sul->time_nextcmd[i] = currtime - SulCmdDelay;
		}
	}
	if (!sul->via) {
		// permlink connection re-established
		if (cp->type == CT_HOST)
			relink_sul(cp);
	}

	if (oldsul) {
		if (sul->stop_process_until > oldsul->stop_process_until)
			oldsul->stop_process_until = sul->stop_process_until;
		else
			sul->stop_process_until = oldsul->stop_process_until;
		// free our old connection pointer
		free_sul_sp(cp);
	}
	// store session pointer
	sp = (struct sul_sp *) hmalloc(sizeof(struct sul_sp));
	sp->connection = cp;
	sp->next = sul->sp;
	sul->sp = sp;
	sul->last_updated = currtime;
	// relink
	cp->sul = sul;

	return sul;
}

/*--------------------------------------------------------------------------*/

void expire_sul(void)
{
	struct static_user_list *sul, *sul_prev, *sul_next;
	int keep;
	int i;
	static time_t last_run = 0L;

        if (currtime - last_run < 60L)
		return;

	for (sul = sul_prev = list_of_static_users; sul; ) {
		keep = 0;
	
		if (sul->sp) {
			// sul still in use
			keep = 1;
		} else {
			// user left convers
			if (sul->delayqueue) {
				free_sul_delayqueue(sul);
			}
		}

		if (*sul->name) {
			if (is_banned(sul->name)) {
				sul->last_updated = currtime;
				keep = 1;
			} else if ((sul->last_updated + SulLifeTime) > currtime) {
				keep = 1;
			}
		}


		for (i = 0; i < SUL_CMD_MAX; i++) {
			if (sul->num_floods[i]) {
				if ((sul->time_nextcmd[i] + SulLifeTime) < currtime) {
					sul->num_floods[i] = 0;
					sul->last_updated = currtime;
					keep = 1;
				} else {
				}
			} else {
				if ((sul->time_nextcmd[i] + SulLifeTime) < currtime) {
					sul->time_nextcmd[i] = currtime;
				}
			}
		}

		if (keep) {
			sul_prev = sul;
			sul = sul->next;
			continue;
		}

		if (sul == list_of_static_users) {
			list_of_static_users = sul_prev = sul->next;
		} else if (sul_prev) {
			sul_prev->next = sul->next;
		}
		sul_next = sul->next;
		hfree(sul);
		sul = sul_next;
	}
	last_run = currtime;
}

/*---------------------------------------------------------------------------*/

void update_ratelimit(struct static_user_list *sul, char *name, int name_len, char *nickname, int nickname_len, char *text)
{
#define	STD_CALL	9
        // allow only 1 msg every 2s. up to 5 msg bursts 
        int ratelimit = 0;

        if (!sul)
                return;

	// name up to 9 (like ax25 call "dl9sau-15"
	// nickname: counts. if not present (means "same as name") it does
	// not cound if > 9, it counts double.

	if (RateLimitLine > 0)
		ratelimit += RateLimitLine;
	if (RateLimitContent > 0)
        	ratelimit += (strlen(text) + name_len > STD_CALL ? name_len : 0 + (strcasecmp(name, nickname) ? (nickname_len > STD_CALL ? (2 * nickname_len) : nickname_len) : 0)) / RateLimitContent;

        sul->stop_process_until += ratelimit;
}

/*--------------------------------------------------------------------------*/

int check_user_banned(struct connection *cp, char *cmd)
{
        struct ban_list *isbanned;

        if (cp->operator)
                return 0;

        if ((isbanned = is_banned(cp->name))) {
                char buffer[2048];
                if (!cp->ircmode) {
                        sprintf(buffer, "*** (%s) You are banned (%s). Please wait a while and try again.\n", ts2(currtime), isbanned->reason);
                } else {
                        sprintf(buffer, ":%s 263 %s %s :You are banned (%s). Please wait and try a gain.\n", myhostname, cmd, (*cp->nickname ? cp->nickname : "*"
), isbanned->reason);
                }
                appendstring(cp, buffer);
                return 1;
        }
        return 0;
}

/*--------------------------------------------------------------------------*/

int check_msg_flood(struct connection *cp, char *toname, int channel, char *name, char *nickname, char *text)
{
	struct static_user_list *sul;

	if ((cp->type == CT_HOST && !(FeatureFLOOD & NO_REMOTE_MESSAGE_FLOOD)) ||
			!(FeatureFLOOD & NO_SERVER_FLOOD))
		return 0;

	if (!strcasecmp(name, "conversd"))
			return 0;

	if (is_banned(name))
		return 1;

	if (!(sul = get_sul(name, cp))) {
		// we know nothing abt him. pass it along. maybe droped
		// later. it's not on us here and now
		return 0;
	}

	sul->last_updated = currtime;

	if (currtime > sul->stop_process_until)
        	sul->stop_process_until = currtime;
	if (sul->delayqueue || sul->stop_process_until > currtime + RateLimitMAX) {
		add_to_sul_delayqueue(cp, sul, toname, channel, name, text);
		return 1;
	}
	update_ratelimit(sul, name, strlen(name), nickname, strlen(nickname), text);
	// send it
	return 0;
}

/*--------------------------------------------------------------------------*/

void add_to_sul_delayqueue(struct connection *cp, struct static_user_list *sul, char *toname, int channel, char *name, char *text)
{

	struct sul_qp *new_qp, *qp;
	int text_len;

	if (!(text_len = strlen(text)))
		return;
	if (toname && (strlen(toname) > NAMESIZE))
		toname[NAMESIZE] = 0;
	if (strlen(name) > NAMESIZE)
		name[NAMESIZE] = 0;

	if (sul->delayqueue_len + text_len > FLOODSIZE && (FeatureBAN & DO_BAN)) {
		char reason[64+NAMESIZE+1];
		char tmp[64];
		char buffer[2048];
		// avoid spoofing ("foo" sends message to real-"foo"). then do not ban.
		// we leave the rate-limit as it is and drop the packets, because we want
		// to protect our permlinks
		if (!strcasecmp(sul->name, get_tidy_name(toname)))
			return;
		sprintf(reason, "%s Message Flood to ", (cp->type == CT_HOST ? "Remote" : "Local"));
		if (toname)
			sprintf(reason+strlen(reason), "<%s>", toname);
		else
			sprintf(reason+strlen(reason), "#%d", channel);
		sprintf(buffer, "*** (%s) conversd@%s Banned %s for %s: %s", ts2(currtime), myhostname, name, ts3(BanTime * 60, tmp), reason);
		if (toname) {
			send_msg_to_user("conversd", toname, buffer);
		} else {
			send_msg_to_channel("conversd", channel, buffer);
		}
		sprintf(buffer, "%s (%s%s%s)", reason, name, (cp->type == CT_HOST ? " via " : "@"), (cp->type == CT_HOST ? cp->name : myhostname));
		clear_locks();
		cp->locked = 1;
		ban_user(name, myhostname, sul, currtime, BanTime, buffer);
		return;
	}
	sul->delayqueue_len += text_len;

	new_qp = (struct sul_qp *) hmalloc(sizeof(struct sul_qp));
	// there's a common sul for call and call-foo. store name
	new_qp->name = hstrdup(name);
	new_qp->text = hstrdup(text);
	new_qp->text_len = text_len;
	new_qp->channel = channel;
	new_qp->toname = (toname) ? hstrdup(toname) : 0;
	new_qp->next = 0;

	for (qp = sul->delayqueue; qp && qp->next; qp = qp->next) ;

	if (qp)
		qp->next = new_qp;
	else
		sul->delayqueue = new_qp;
}

/*--------------------------------------------------------------------------*/

void free_qp(struct sul_qp *qp)
{
	if (qp->name)
		hfree(qp->name);
	if (qp->text)
		hfree(qp->text);
	if (qp->toname)
		hfree(qp->toname);
	hfree(qp);
}

/*--------------------------------------------------------------------------*/

void free_sul_delayqueue(struct static_user_list *sul)
{
	struct sul_qp *qp, *qp_next;
	for (qp = sul->delayqueue; qp; ) {
		qp_next = qp->next;
		free_qp(qp);
		qp = qp_next;
	}
	sul->delayqueue = 0;		// mark empty
	sul->delayqueue_len = 0;	// len is also zero
}

/*--------------------------------------------------------------------------*/

void send_sul_delayed_messages(void)
{
	struct static_user_list *sul;
	struct sul_sp *sp;
	struct sul_qp *qp, *qp_next;
	struct connection *p;
	struct clist *cl;
	static time_t last_run = 0;

	if (currtime - last_run < 60L)
		return;

	for (sul = list_of_static_users; sul; sul = sul->next) {
		if (!sul->delayqueue_len)
			continue;
		if (!sul->via || sul->via->type == CT_CLOSED || !sul->sp) {
			free_sul_delayqueue(sul);
			continue;
		}
		for (qp = sul->delayqueue; qp; ) {
			if (currtime > sul->stop_process_until)
        			sul->stop_process_until = currtime;
			if (sul->stop_process_until > currtime + RateLimitMAX)
				break;
			p = 0;
			for (sp = sul->sp; sp; sp = sp->next) {
				if ((p = sp->connection) && p->type == CT_USER /* != CT_CLOSED */ &&
						!strcasecmp(p->name, qp->name)) {
					// user still online
					// stored message a privmsg?
					if (qp->toname)
						break;
					// if it's a channel message, check if user is still on this channel
					if (p->channel == qp->channel)
						break;	
					for (cl = p->chan_list; cl; cl = cl->next)
						if (cl->channel == qp->channel)
							break;
					if (cl)
						break;
				}
				// not found
				p = 0;
			}
			if (p) {
				clear_locks();
				// need to lock the host the message initially came from
				sul->via->locked = 1;
				if (qp->toname)
					send_msg_to_user2(p->name, p->name, p->nickname, qp->toname, qp->text, 1);
				else
					send_msg_to_channel2(p->name, p->name, p->nickname, qp->channel, qp->text, 1);
				update_ratelimit(sul, p->name, p->name_len, p->nickname, p->nickname_len, qp->text);
			}
			sul->delayqueue_len -= qp->text_len;
			qp_next = qp->next;
			free_qp(qp);
			qp = qp_next;
		}
		sul->delayqueue = qp;	// will mark 0 if empty
	}

	last_run = currtime;
}

/*--------------------------------------------------------------------------*/

void complain_flood(struct connection *cp, char *cmd_name, int floods, time_t time_next)
{
	char buffer[2048];
	char tmp[64];

	if (floods < 3 || !(FeatureBAN & DO_BAN)) {
		if (!cp->ircmode)
			sprintf(buffer, "*** (%s) Flood protection for '/%s'. Please wait %s and try again.\n", ts2(currtime), cmd_name, ts3(time_next - currtime, tmp));
		else {
			strupr(cmd_name);
			sprintf(buffer, ":%s 263 %s %s :Please wait %s and try again.\n", myhostname, (*cp->nickname ? cp->nickname : "*"), cmd_name, ts3(time_next - currtime, tmp));
		}
	} else {
		if (!cp->ircmode) {
			sprintf(buffer, "*** (%s) Please do not flood. You will be banned%s soon.\n", ts2(currtime), (floods > 4 ? " really" : ""));
		} else {
			sprintf(buffer, ":%s 466 %s :You will be banned.%s\n", myhostname, (*cp->nickname ? cp->nickname : "*"), (floods > 4 ? " Don't say you have not been warned." : ""));
		}
	}
	appendstring(cp, buffer);
}

/*--------------------------------------------------------------------------*/

int check_cmd_flood(struct connection *cp, char *name, char *host, int SUL_CMD, int warn, char *cmd)
{
	struct static_user_list *sul;
	int is_local;
	int time_next;
	int floods;

	if ((cp->type == CT_HOST && !(FeatureFLOOD & NO_REMOTE_COMMAND_FLOOD)) ||
			!(FeatureFLOOD & NO_LOCAL_COMMAND_FLOOD))
		return 0;
	if (SUL_CMD < 0 || SUL_CMD >= SUL_CMD_MAX)
		return 0;
	if (cp->type == CT_USER && cp->operator)
		return 0;

	if (cp->type == CT_HOST)
		sul = get_sul(name, cp);
	else
		sul = cp->sul;
	if (!sul)
		return 0;
	
	is_local = (cp->type != CT_HOST && !cp->via);

	time_next = sul->time_nextcmd[SUL_CMD];
	sul->last_updated = currtime;

	// commands like /topic, .. are traffic relevant. update rate limit (used
	// for normal message queue). if user is local, then rate limit has already
	// been updated.
	if (cp->type == CT_HOST) {
		int len = strlen(name);
		update_ratelimit(sul, name, len, name, len, cmd);
	}

	if (time_next > currtime) {

		time_next += (SulCmdDelay * 2);
		sul->time_nextcmd[SUL_CMD] = time_next;

		if (sul->num_floods[SUL_CMD] >= 5 && (FeatureBAN & DO_BAN)) {
			char reason[2048];
			char cmd_name[5];
			if (strlen(name) > NAMESIZE)
				name[NAMESIZE] = 0;
			if (!host) 
				host = "";
			else if (strlen(host) > NAMESIZE)
				host[NAMESIZE] = 0;

			strncpy(cmd_name, sul_cmd_names[SUL_CMD], sizeof(cmd_name)-1);
			cmd_name[sizeof(cmd_name)-1] = 0;
			strupr(cmd_name);
			sprintf(reason, "%s Command Flood %s (%s%s%s%s%s)", (is_local) ? "Local" : "Remote", cmd_name, name, *host ? "@" : "", host, (is_local ? "" : " via "), (is_local ? "" : cp->name));
			if (is_local) {
				char buffer[2048];
				clear_locks();
				ban_user(name, myhostname, sul, currtime, BanTime, reason);
				if (!cp->ircmode)
					sprintf(buffer, "*** Closing Link: %s by %s (Command Flood)%s", cp->name, myhostname, cp->ax25 ? "\r" : "\n");
				else
					sprintf(buffer, "ERROR :Closing Link: %s by %s (Command Flood)\r\n", cp->nickname, myhostname);
				fast_write(cp, buffer, 0);
				bye_command2(cp, "Command Flood");
			} else {
				clear_locks();
				cp->locked = 1;
				ban_user(name, myhostname, sul, currtime, BanTime, reason);
			}
			return -1;
		}

		floods = ++sul->num_floods[SUL_CMD];

		if (is_local && warn) {
			char cmd_name[16];
			strncpy(cmd_name, sul_cmd_names[SUL_CMD], sizeof(cmd_name)-1);
			cmd_name[sizeof(cmd_name)-1] = 0;
			complain_flood(cp, cmd_name, floods, time_next);

		}
		return floods;

	} else {
		time_next += SulCmdDelay;
		sul->time_nextcmd[SUL_CMD] = time_next;
		if (sul->num_floods[SUL_CMD]) {
			if (time_next + (SulLifeTime / 4 /* remember for 15 minutes */ ) < currtime)
				sul->num_floods[SUL_CMD] = 0;
			else
				sul->num_floods[SUL_CMD] -= 1;
		}
		return 0;
	}

	return 0;
}

/*--------------------------------------------------------------------------*/

int check_join_flood(struct connection *cp, char *name, char *host, int channel)
{

	struct connection *p, *p2;
	struct clist *cl;
	int on_channels = 0;
	char buffer[2048];

	if (!FeatureFLOOD)
		return 0;

	if (cp->type == CT_USER) {
		// first check SUL_JOIN
		if (cp->ircmode) {
			int ret;
			if ((ret = check_cmd_flood(cp, cp->name, myhostname, SUL_JOIN, 0, cp->pers))) {
				if (ret > 0) {
					sprintf(buffer, ":%s 437 %s #%d :Nick/channel is temporarily unavailable\n", myhostname, cp->nickname, channel);
					appendstring(cp, buffer);
				}
				return 1;
			}
		} else {
			if (check_cmd_flood(cp, cp->name, myhostname, SUL_JOIN, 1, cp->pers))
				return 1;
		}
		if (!(FeatureFLOOD & NO_LOCAL_JOIN_FLOOD) || cp->operator)
			return 0;
	} else if (cp->type == CT_HOST) {
		if (!(FeatureFLOOD & NO_REMOTE_JOIN_FLOOD))
			return 0;
	} else { /* not user, not host? */
		return -1;
	}


	// flood protection against excessive remote channel joins. this is user@host based,
	// and could only work on channel joins (never on channel leaves, because it
	// would generate inconsistencies). it's not a check_cmd_flood detection,
	// because when links are bad, a user may join and leave quite often..
	user_matches(name, 0);
	p2 = 0;
	for (p = connections; p; p = p->next) {
		if (cp->type == CT_HOST) {
			if (p->type == CT_USER && p->via && p->via == cp && !strcasecmp(p->host, host) && user_matches(0, p->name)) {
				on_channels++;
				p2 = p;
			}
		} else if (cp->type == CT_USER) {
			if (!p->via && p->type == CT_USER && user_matches(0, p->name)) {
				for (cl = p->chan_list; cl; cl = cl->next)
					on_channels++;
			}
		}
	}

	if (on_channels < MAX_CHAN_PER_USER_AND_HOST)
		return 0;

	// block
	if (cp->type == CT_HOST) {
		if (p2 && p2->operator)
			return 0;
		if ((FeatureBAN & DO_BAN)) {
			if (strlen(name) > NAMESIZE)
				name[NAMESIZE] = 0;
			if (strlen(host) > NAMESIZE)
				host[NAMESIZE] = 0;
			sprintf(buffer, "Remote Join Flood to #%d (%s@%s via %s)", channel, name, host, cp->name);
			ban_user(name, myhostname, p2->sul, currtime, BanTime, buffer);
		}
	} else if (cp->type == CT_USER) {
		// we do not BAN this kind of request, because this funktion
		// prevents from abuse
		if (!cp->ircmode)
			sprintf(buffer, "*** (%s) Cannot join #%d: You have joined too many channels.\n", ts2(currtime), channel);
		else
			sprintf(buffer, ":%s 405 %s #%d :You have joined too many channels\n", myhostname, cp->nickname, channel);
		appendstring(cp, buffer);
	}
	return 1;
}
