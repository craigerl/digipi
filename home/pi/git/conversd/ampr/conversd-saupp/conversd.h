/*
 * This is Tampa Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@mdddhd.fc.hp.com>
 * Modifications by Fred Baumgarten <dc6iq@insu1.etec.uni-karlsruhe.de>
 *
 * Modifications by Brian A. Lantz/KO4KS <brian@lantz.com>
 */

#ifndef CONVERSD_H
#define CONVERSD_H

#include <time.h>
#include <netinet/in.h>

#ifndef SOMAXCONN
#define SOMAXCONN       5
#endif

#ifndef WNOHANG
#define WNOHANG         1
#endif

#ifndef min
#define min(a,b) ((a) >= (b) ? (b) : (a))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

#define MINCHANNEL	0

#define MAXCHANNEL      32767
#define uchar(x) ((x) & 0xff)

#define WANT_FILTER
#ifdef	WANT_FILTER
#define MSG_PRIV_USER	1
#define MSG_CHAN_USER	2
#define MSG_PRIV_CONV	4
#define MSG_CHAN_CONV	6
#define MSG_USER_FILT	8
#endif

#define	CAN_COMP	1
#define	IS_COMP		2
#define	COMP_MTU	255

#define	LCK_PROFILE	1

typedef unsigned char uint8;	/* 8-bit unsigned integer */

struct mbuf {
	struct mbuf *next;
	char *data;
};

#define NULLCHAR ((char *) 0)

struct clist {
	int channel;		/* channel number */
	uint8 channelop;	/* channel operator status */
	time_t time;		/* join time */
	struct clist *next;	/* pointer to next entry */
};

#define NULLCLIST ((struct clist *) 0)

#define NAMESIZE (64 - 1)
#define PERSSIZE (512 - 1)
#define AWAYSIZE (512 - 1)
#define NOTIFYSIZE (512 - 1)


#define	MAX_MTU		1024
#define	MAX_IBUFQ_LEN	MAX_MTU*2
// worst case: /..COMMAND fromname:nick toser text""
//       2048  3 + 4     +1 +63  +1+63+1 +63 +1 +n+1  -> n = 1024-201 = 823
// in order to be compatible to furhter extensions, we limit it to 750
#define	MAX_MSGSIZE 750

// values derived from RFC:
// cave: NAMESIZE must be >= IRC_IRCNAMESIZE
#define IRC_NAMESIZE    63
// cave: MAX_MSGSIZE must be >= IRC_MAX_MSGSIZE
#define IRC_MAX_MSGSIZE	510     /* message length excl. \r\n on transport layer */

#define	PASSSIZE	80	/* for md5 passwords, at least 32 characters */

#define	PERMLINK_MAXIDLE	15*60

#define	FLOODSIZE	1785	/* flood protection: bytes max in input buffer: CAVE: should be greater than MAX_MSGSIZE and smaller than ibuf size  */

#define BANTIME 15              /* 15 minutes (for local users) */
#define SUL_LIFETIME    60*60   /* lifetime of stored information, if not referenced */
#define SUL_CMD_DELAY   30      /* every 30s a specific command could be executed */

#define	DEFAULT_WIDTH	80
#define	DEFAULT_WIDTH_IRC	 (512 - 9-1 - 9-1 - 63-1 - 1 - 1)

struct jobs {
	time_t when;		/* time when the job will be executed */
	uint8 interval;		/* at least every minute. scale: min */
	uint8 retries;		/* how often? - 0 is endless */
	char *command;		/* command to be executed */
	struct jobs *next;	/* next job in the list */
};

struct connection {
	uint8 type;			/* Connection type */
#define CT_UNKNOWN      0
#define CT_USER         1
#define CT_HOST         2
#define CT_CLOSED       3
#define CT_OBSERVER	4
	uint8 session_type;		/* Inbound, Outbound, Circuit */
#define	SESSION_UNKNOWN	0
#define	SESSION_INBOUND		1
#define	SESSION_OUTBOUND	2
#define	SESSION_CIRCUIT		3
	char name[NAMESIZE + 1];	/* Name of user or host */
	uint8 name_len;			/* Length of the name (used for flood protection */
	char host[NAMESIZE + 1];	/* Name of host where user is logged on */
	char rev[NAMESIZE + 1];		/* revision of software (CT_HOST) */
	char nickname[NAMESIZE + 1];	/* Nickname of user */
	uint8 nickname_len;             /* Length of the nickname (used for flood protection */
	uint8 shownicks;		/* show nicknames */
	struct connection *via;		/* Pointer to neighbor host */
	time_t time;			/* Connect time */
	uint8 locked;			/* Set if mesg already sent */
	uint8 locked_func;		/* function is locked - do not call again until unlocked */
	
	/* These for local connections only: */
	int fd;				/* Socket descriptor */
	time_t time_write;              /* time when data put to an empty txqueue */
	time_t time_recv;               /* time when data last red from socket */
	time_t time_processed;		/* time when the last command or message was processed */
	struct sockaddr *addr;		/* Socket address */
	//char ibuf[2048];		/* Input buffer which is processed */
	//char ibufq[2048];		/* Input buffer queue from last read() */
	char *ibuf;			/* Input buffer which is processed */
	int icnt;			/* Number of bytes in input buffer */
	char *ibufq;			/* Input buffer queue from last read() */
	int ibufq_len;			/* Length of ibufq */
	struct mbuf *obuf;		/* Output queue */
	long received;			/* Number of bytes received */
	long xmitted;			/* Number of bytes transmitted */
	long received_comp;		/* Number of compressed frames on rx */
	long xmitted_comp;		/* Number of compressed frames on tx */
	uint8 hostallowed;		/* connectee is allowed to become a HOST */
	char *sockname;			/* name of the local socket */
	int mtu;			/* mtu on link layer */
	uint8 compress;			/* compression on link layer */
	
	/* For users: */
	int channel;			/* current channel number */
	struct clist *chan_list;	/* linked list of joined channels */
	struct jobs *jobs;		/* linked list of periodical jobs */
	char away[AWAYSIZE + 1];	/* Away string */
	time_t atime;			/* time of last "away" state change */
	time_t mtime;			/* time of last message receive */
	time_t idle_timeout;		/* user idle timeout for disconnect */
	int anti_idle_offset;           /* time in minutes for an action requested when the user is idle */
	char pers[PERSSIZE + 1];	/* Personal string */
	uint8 verbose;			/* verbose mode */
	char prompt[4];			/* prompt mode */
	char query[NAMESIZE + 1];	/* name of queried user */
	char notify[NOTIFYSIZE + 1];	/* List of calls you like to be notified */
	char pass_want[PASSSIZE+1];	/* authentication */
	char pass_got[PASSSIZE+1];	/* authentication */
	uint8 needauth;			/* authentication forced? */
	uint8 isauth;			/* authentication succeded. level? */
#ifdef	WANT_FILTER
	char *filter;			/* filtered calls */
	char *filterwords;		/* filtered words */
	time_t filter_time;             /* time filter was set */
	uint8 filtermsgs;		/* filter for channel or user msgs */
#endif
	uint8 lang;			/* user's language for the helpfile */
	uint8 charset_in;			/* charset - default ISO (ansi) */
	uint8 charset_out;		/* charset - default ISO (ansi) */
	uint8 channelop;			/* channel operator of current channel ? */
	uint8 operator;			/* convers operator */
	int expected;			/* expected answer */
	int invitation_channel;		/* last invitation was from this channel */
	int width;			/* user screen width */
	uint8 observer;			/* is the user an observer only */
	uint8 restrictedmode;		/* is the user link a resticted? */
	time_t timestamp;		/* for the /timestamp command */
	struct static_user_list *sul;   /* common timers against flooders */
	
	/* For links: */
	uint8 amprnet;			/* is it /..UDAT (1) or /..USER (0) ? */
	uint8 oldaway;			/* is it the pre3.02 Version of away ? */
	uint8 ax25;			/* AX.25 Connection ? */
	uint8 features;
#define FEATURE_AWAY	1		/* a - "away feature" */
#define FEATURE_FWD	2		/* d - "destination forwarding" */
#define FEATURE_MODES	4		/* m - "channel modes" */
#define FEATURE_LINK	8		/* p - "ping pong link measurement" */
#define FEATURE_UDAT	16		/* u - "udat command extension and user command understood both" */
#define FEATURE_NICK	32		/* n - "TNOS Nickname extensions" */
#define FEATURE_FILTER	64		/* f - "call filter (/ignore)" */
#define FEATURE_SAUPP_I	128		/* j - "saupp netjoin and server squit and user uquit messages" */
	uint8 ircmode;			/* client is an irc-client (for future use */
	struct connection *next;	/* Linked list pointer */
};

#define CM_UNKNOWN   (1 << CT_UNKNOWN)
#define CM_USER      (1 << CT_USER)
#define CM_HOST      (1 << CT_HOST)
#define CM_CLOSED    (1 << CT_CLOSED)
#define CM_OBSERVER  (1 << CT_OBSERVER)

#define NULLCONNECTION  ((struct connection *) 0)

#define DEF_NR_PERMLINKS 16	/* was 40. you may configure with maxlinks in conversd.conf */
#define	NR_GROUPS	4	/* number of alternative links to a permlink */

#define HOSTNAMESIZE (80 - 1)
// cave: HOSTNAMESIZE must be >= IRC_HOSTNAMESIZE
#define IRC_HOSTNAMESIZE     63

struct s_group {
	char *socket;		/* Name of socket to connect to */
	char *command;		/* Optional connect command */
	uint8 quality;		/* quality of this link */
};

struct permlink {
	char name[HOSTNAMESIZE + 1];	/* Name of host */
	struct s_group s_groups[NR_GROUPS];	/* a permlink may have multible alternative connections (for e.g. ax25 and tcp) */
	struct connection *connection;	/* Pointer to associated connection */
	struct permlink *primary;	/* If !NULL, points to link we backup for */
	struct permlink *backup;	/* If !NULL, points to our backup link */
	time_t statetime;		/* Time of last (dis)connect */
	int tries;		/* Number of connect tries */
	long waittime;		/* Time between connect tries */
	time_t retrytime;	/* Time of next connect try */
	long testwaittime;	/* Time between tries */
	time_t testnexttime;	/* Time of next test */
	time_t locked_until;          /* Time to release lock */
	long rxtime;		/* rtt by other side */
	long txtime;		/* rtt found out by me */
	uint8 groups;		/* link list group */
	signed char curr_group;	/* current used group */
	uint8 permanent;		/* this is a permanent entered link */
	uint8 locked;             /* Link is locked (i.e. unconnectable because caused loops)  */
	uint8 locked_func;	/* function is locked - do not call again until unlocked */
};

#define NULLPERMLINK  ((struct permlink *) 0)
#define NULLPERMARRAY  ((struct permlink **) 0)


extern struct permlink **permarray;

struct destination {
	char name[HOSTNAMESIZE + 1];	/* destination name */
	char hisneigh[HOSTNAMESIZE + 1];/* destination's direct neighbour */
	char rev[NAMESIZE + 1];	/* revision of software (CT_HOST) */
	struct permlink *link;	/* link to this destination */
	long rtt;		/* round trip time to this host */
	int hops;		/* hops */
	long last_sent_rtt;	/* last donwnstream sent rtt */
	time_t updated;		/* how old is this information? */
	uint8 auto_learned;	/* host autolearned? - mark it, so we could expire him later */
	uint8 connected;	/* is it connected now? (used by notify_destinations) */
	//int downstream[DEF_NR_PERMLINKS];	/* all up/downstream times */
	struct destination *next;	/* a one dimansional list is ok for now :-) */
	uint8 locked;
};

#define NULLDESTINATION  ((struct destination *) 0)

#define	CHIST_MAXLINES	64 	/* 64 lines */
#define	CHIST_EXPIRES	8*60	/* steps: minutes. default: 8h */

struct chist {
	time_t time;
	char *name;
	char *text;
	struct chist *next;
};

struct channel {
	char name[80];		/* ascii name of channel */
#define TOPICSIZE (512 - 1)
	char topic[TOPICSIZE + 1];	/* topic of channel */
	time_t time;			/* when was the topic set ? */
	char tsetby[NAMESIZE + 1];	/* who set the topic? */
	char createby[NAMESIZE + 1];	/* who created the channel? */
	time_t ctime;			/* when? */
	char lastby[NAMESIZE + 1];	/* who sent last msg to the channel? */
	time_t ltime;			/* when? */
	time_t locked_until;            /* time when local rejoin is possible (after a netsplit */
	time_t expires;     		/* time when channel expires */
	int chan;		/* integer value of channel (<32768 !) */
	uint8 locked;		/* Set if mesg already sent */
	uint8 flags;		/* channel flags */
#define M_CHAN_S 0x01		/* secret channel - number invisible */
#define M_CHAN_P 0x02		/* private channel - only join by invitation */
#define M_CHAN_T 0x04		/* topic settable by operator only */
#define M_CHAN_I 0x08		/* invisible channel - only sysops can see */
#define M_CHAN_M 0x10		/* moderated channel - operators may talk */
#define M_CHAN_L 0x20		/* local channel - no forwarding */
	struct channel *next;	/* linked list again */
	struct chist *chist;	/* channel message history */
};

#define NULLCHANNEL  ((struct channel *) 0)

struct cmdtable {
	char *name;
	void (*fnc)(struct connection *cp);
	int states;
};

extern struct cmdtable cmdtable[];
extern struct cmdtable cmdtable_irc[];


struct extendedcmds {
	char *name;
	struct extendedcmds *next;
};

#define NULLEXTCMD ((struct extendedcmds *)0)
extern struct extendedcmds *ecmds;


struct stats {
	long rx;
	long tx;
	time_t start;
};

struct langs {
	char *ext;
	char *lang[5];
};

struct listeners {
	char *name;
	int fd;
};

#define ONE_HOUR (60*60)
#define ONE_DAY (60*60*24)
#define ONE_K (1024L)
#define ONE_MEG (1024L * 1024L)

#define L_SENT 1
#define L_RECV 2

extern void linklog(struct connection *p, int direction, const char *fmt, ...);

extern void send_notice(struct connection *cp, const char *fromname, int where);
extern void appendstring(struct connection *cp, const char *string);
extern void append_general_notice(struct connection *cp, const char *string);
extern void append_chan_notice(struct connection *cp, const char *string, int channel);
extern void appendc(struct connection *cp, const int n, const int ast);
extern void appendprompt(struct connection *cp, const int ast);
extern void destroy_channel(int number, int lock);
extern void free_connection(struct connection *cp);
extern int queuelength(const struct mbuf *bp);
extern struct permlink *find_permlink(const char *name);
extern struct destination *find_destination(const char *name);
extern void free_closed_connections(void);
extern char *showargp(void);
extern char *getarg(char *line, int all);
extern char *getargcs(char *line, int all);
extern void getTXname(struct connection *cp, char *buf);
extern char *formatline(char *prefix, int offset, char *text, int linelen);
extern void notify_destinations(struct destination *dest, const char *host, const char *rev, int state);
extern char *ts(time_t gmt);
extern char *ts2(time_t gmt);
extern char *ts3(time_t seconds, char *buffer);
extern char *ts4(time_t seconds);
extern char *ts5(time_t seconds);
extern char *get_nick_from_name(char *fromname);
extern char *get_user_from_name(char *fromname);
extern struct connection *find_connection(char *fromname, char *hostname, uint8 ctype);
extern char *find_nickname(char *fromname, char *hostname);
extern struct connection *alloc_connection(int fd);
extern void accept_connect_request(int flisten);
extern void clear_locks(void);
extern int count_user(int channel);
extern int count_user2(char *host);
extern int count_topics(void);
extern void send_awaymsg(char *name, char *nick, char *host, time_t time, char *text, int text_changed);
#ifdef	WANT_FILTER
extern void send_filter_msg (char *name, char *host, time_t time, char *filter);
extern int is_filtered(char *from_orig, char *fromuser, struct connection *cp, char *text);
#endif
extern char *get_mode_flags(int flags);
extern void send_mode(struct connection *cp, struct channel *chan, int oldflags);
extern void send_opermsg(char *name, char *tonick, char *host, char *from, char *fromnick, char *fromhost, int channel, int status_changed);
extern void send_persmsg(char *name, char *nick, char *host, int channel, char *text, int text_changed, time_t time);
extern void send_topic(char *name, char *nick, char *host, time_t time, int channel, char *text);
extern void send_user_change_msg(char *name, char *nick, char *host, int oldchannel, int newchannel, char *pers, int pers_changed, time_t time, int observer, int sent_quits, int announce_chanop_status, int nick_changed, char *oldnick);
extern void send_msg_to_user(char *fromname, char *toname, char *text);
extern void send_msg_to_user2(char *fromname, char *fromuser, char *fromnick, char *toname, char *text, int flood_checked);
extern void send_msg_to_channel(char *fromname, int channel, char *text);
extern void send_msg_to_channel2(char *fromname, char *fromuser, char *fromnick, int channel, char *text, int flood_checked);
extern void send_server_notice(char *host, char *text, int local_only, int priority);
extern void send_invite_msg(char *fromname, char *toname, int channel);
extern void log_user_change(char *name, char *host, int oldchannel, int newchannel);
extern void update_destinations(struct permlink *p, char *name, long rtt, char *rev, int hops, char *histneigh);
extern struct permlink *update_permlinks(char *name, struct connection *cp, int isperm);
extern void connect_permlinks(void);
extern int get_hash(char *name);
extern int ecmd_exists(char *cmdname);
extern void update_ecmds(void);
extern void update_stats(time_t now);
extern void init_stats(void);
extern void mysighup(int sig);
extern int dohelper(struct connection *cp, char *name, const char *search);

extern void route_cleanup(struct connection *cp);
extern int dest_check(struct connection *cp, struct destination *dest);
extern int user_check(struct connection *cp, char *user);

extern void process_input(struct connection * cp);
extern void expire_chist(struct channel *ch, int keep_lines);
extern int fast_write(struct connection *cp, char *s, int force_comp);
extern void anti_idle_msg(struct connection *cp);
extern void add_to_history(struct channel *ch, char *fromname, char *text);

extern struct connection *connections;
extern struct permlink *permlinks;
extern struct permlink *notpermlinks;
extern struct destination *destinations;
extern struct channel *channels;
extern struct stats daily[];
extern struct stats hourly[];
extern time_t starttime;
extern time_t nextdailystats;
extern time_t nexthourlystats;
extern long hoursonline;
extern long daysonline;

extern time_t boottime;
extern time_t currtime;
extern char userfile[];
extern char motdfile[];
extern char restrictfile[];
extern char accessfile[];
extern char noaccessfile[];
extern char helpfile[];
extern char observerfile[];
extern char convtype[];
extern char myrev[];
extern char myfeatures[];
extern int restrictedmode;
extern char OBSERVERID[];
extern int NR_PERMLINKS;

extern char *convcmd;

extern void setargp(char *str);

extern int encstathuf(char *src, int srclen, char *dest, int *destlen);
extern int decstathuf(char *src, char *dest, int srclen, int *destlen);

extern int interprete_conversd_messages(struct connection *cp, const char *text, int channel);
extern void delay_permlink_connect(struct permlink *pl, int delay);
extern void send_quits(struct connection *cp, char *name, char *host, char *reason);

#endif
