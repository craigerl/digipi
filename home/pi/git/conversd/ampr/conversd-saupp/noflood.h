/*
 * Flood and Abuse Protection for saupp
 *
 * (c) 2002 Thomas Osterried <dl9sau>
 * License: GPL
 *
 */

#ifndef NOFLOOD_H
#define NOFLOOD_H

#define	RATELIMIT_MAX		10
#define	RATELIMIT_LINE		2
#define	RATELIMIT_CONTENT	64

extern unsigned int RateLimitMAX;
extern unsigned int RateLimitLine;
extern unsigned int RateLimitContent;

/* masks */
#define NO_SERVER_FLOOD		1
#define NO_LOCAL_COMMAND_FLOOD	2
#define	NO_LOCAL_JOIN_FLOOD	4
#define NO_REMOTE_COMMAND_FLOOD	8
#define	NO_REMOTE_MESSAGE_FLOOD	16
#define	NO_REMOTE_JOIN_FLOOD	32

extern int FeatureFLOOD;

/* masks */
#define	DO_BAN		1
#define	ANNOUNCE_BAN	2
#define	ACCEPT_BAN	4

extern time_t BanTime;
extern int FeatureBAN;

extern char *ban_statistic_last_user;
extern char *ban_statistic_last_by;
extern char *ban_statistic_last_reason;
extern int ban_statistic_bans;
extern time_t ban_statistic_last_ban;

struct ban_list {
        char name[NAMESIZE+1];          /* who is banned */
        char announced_from[NAMESIZE+1];/* who has banned */
        char reason[64];                /* why */
        time_t time;                    /* when */
        time_t time_refreshed;          /* still.. */
        time_t announce_next;           /* update */
        time_t until;                   /* how long? - 0 means for ever */
        struct ban_list *next;          /* pointer to next entry */
};

extern struct ban_list *banned_users;


extern unsigned int SulCmdDelay;
extern unsigned int SulLifeTime;

extern struct static_user_list *list_of_static_users;

#define SUL_UNKNOWN     0
#define SUL_AWAY        1
#define SUL_INVI        2
#define SUL_JOIN        3
#define SUL_MODE        4
#define SUL_NICK        5
#define SUL_PERS        6
#define SUL_TOPIC       7
#define SUL_CHAN_SPAM   8
#define SUL_CHAN_GAME   9
#define SUL_CMD_MAX    10       /* this is always the latest */

extern char *sul_cmd_names[];

/* here, all active sessions are stored */
struct sul_sp {
	struct connection *connection;
	struct sul_sp *next;
};

/* here, all delayed messages are stored */
struct sul_qp {
	char *name;
	char *text;
	int text_len;
	int channel;
	char *toname;
	struct sul_qp *next;
};

struct static_user_list {
        char name[NAMESIZE+1];
        char host[NAMESIZE+1];
        time_t last_updated;
        time_t stop_process_until;
        time_t time_recv;
        time_t time_nextcmd[SUL_CMD_MAX];
        int    num_floods[SUL_CMD_MAX];
	struct connection *via;
	struct sul_sp *sp;
	struct sul_qp *delayqueue;
	int delayqueue_len;
        struct static_user_list *next;
};

/* this is needed by check_join_flood(), and is currently hardcoded */
#define MAX_CHAN_PER_USER_AND_HOST      15

extern struct static_user_list *generate_sul(struct connection *cp, struct connection *p_via, struct static_user_list *oldsul);
extern void expire_sul(void);

extern void update_ratelimit(struct static_user_list *sul, char *name, int name_len, char *nickname, int nickname_len, char *text);

extern void ban_user(char *name, char *fromhost, struct static_user_list *sul, time_t when, time_t minutes, char *reason);
extern struct ban_list *is_banned(char *name);
extern void announce_banned_users(struct connection *cp);
extern void expire_banlist(void);

extern int check_user_banned(struct connection *cp, char *name_cmd);
extern int check_cmd_flood(struct connection *cp, char *name, char *host, int pos_cmd, int verbose, char *cmd);
extern int check_msg_flood(struct connection *cp, char *toname, int channel, char *name, char *nickname, char *text);
extern int check_join_flood(struct connection *cp, char *name, char *host, int channel);

extern void free_sul_sp(struct connection *cp);
extern void free_sul_delayqueue(struct static_user_list *sul);
extern void add_to_sul_delayqueue(struct connection *cp, struct static_user_list *sul, char *toname, int channel, char *name, char *text);
extern void send_sul_delayed_messages(void);
#endif
