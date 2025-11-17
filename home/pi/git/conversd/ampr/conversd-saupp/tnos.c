/*
 * This is Tampa Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@mdddhd.fc.hp.com>
 *
 * By Brian A. Lantz/KO4KS <brian@lantz.com>
 * Further mods by Tony Jenkins G8TBF, Jan 2000. Changed to V1.23 
 * /nickname & /nonickname commands replaced, various bugs fixed.
 * Mods by Brian Rogers/N1URO <n1uro@n1uro.com>
 */

#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "conversd.h"
#include "version.h"
#include "user.h"
#include "irc.h"
#include "host.h"
#include "log.h"
#include "config.h"
#include "noflood.h"

void h_uadd_command(struct connection *cp);

#define SMILEYFile "/smileys.txt"
#define SPAMFile "/spam.txt"
#define ConfNews "/convers.news"
/* #define Quotes "/usr/lib/quotes" */
#define READ_TEXT "r"
#define NULLFILE ((FILE *) 0)
static char cutstr[] = "%s%s has cut the deck and selected the %s of %s!! %s";
static char rollstr[] = "%s%s has rolled a %d and a %d for a total of %d! %s";
static char bjackstr[] = "%s%s has drawn %d and %d in 21-stud for a total of %d! %s";
static char summary[] = "%sconversd %s Command Summary:\n";
static char noopenstr[] = "Can't open '%s'\n";
static char nicknmstr[] = "Nickname ";
static char nicknamestr[] = "%s%s%sset to '%s'.\n";
//static char uaddstr[] = "/\377\200UADD %s %s %s %s %s";
static char systemstr[] = "SYSTEM";
/* static char quotebanner[] = " *** Quote of the day: \n"; */
static char tmstr[] = "Time";
char stars[] = "*** ";
static char conversd[] = "conversd";
//static char *oldnick = 0;

char *Days[7] =
{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char *Months[12] =
{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};



long seed = 1L;
#define CONV_RAND_MAX 0x7fffffff

int conv_rand(void)
{
	seed = (1103515245L * seed + 12345) & CONV_RAND_MAX;
	return ((int) (seed & 077777));
}

void conv_randomize(void)
{
	seed = (time(0) & CONV_RAND_MAX);
}

int conv_random(int num, int base)
{
	return (((long) (conv_rand() * time(0)) & CONV_RAND_MAX) % num + base);
}

static char *suits[] = {"Hearts", "Clubs", "Diamonds", "Spades"};
static char *cards[] = {"Ace", "Two", "Three", "Four", "Five", "Six", "Seven", "Eight", "Nine", "Ten", "Jack", "Queen", "King", "Joker"};

char *rip(char *str)
{
	if (str && *str) {	// make sure strlen() will be >= 1 .....
		if ((str[strlen(str) - 1] == '\n') || (str[strlen(str) - 1] == '\r'))
			str[strlen(str) - 1] = '\0';
	}
	return str;
}


#ifdef	notdef
char *skipwhite(char *cp)
{
	while (*cp && (*cp == ' ' || *cp == '\t'))
		cp++;
	return (cp);
}


char *skipnonwhite(char *cp)
{
	while (*cp && *cp != ' ' && *cp != '\t')
		cp++;
	return (cp);
}
#endif

void smiley_command(struct connection *cp)
{
	int offset, k;
	char buff[200];
	FILE *fp;

        if (check_user_banned(cp, "SMILEY"))
		return;
	strcpy(buff, CONF_DIR);
	strcat(buff, SMILEYFile);
	
	conv_randomize();
	offset = conv_random (7, 1);
	if (!(fp = fopen (buff, "r")))
		return;
	sprintf (buff, "%sSmiley: ", stars);
	for (k = 0; k < offset; k++)
		fgets (&buff[12], 188, fp);
	fclose (fp);
	rip (buff);
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_SPAM, 1, buff))
												return;
	clear_locks ();
	send_msg_to_channel (cp->name, cp->channel,buff);
}

void spam_command(struct connection *cp)
{
int offset, k;
char buff[200];
FILE *fp;
	
	strcpy(buff, CONF_DIR);
	strcat(buff, SPAMFile);
	
        if (check_user_banned(cp, "SPAM"))
		return;
	conv_randomize();
	offset = conv_random (10, 1);
	if (!(fp = fopen (buff, "r")))
		return;
	sprintf (buff, "%sdaemon: ", stars);
	for (k = 0; k < offset; k++)
		fgets (&buff[12], 188, fp);
	fclose (fp);
	rip (buff);
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_SPAM, 1, buff))
												return;
	clear_locks ();
	send_msg_to_channel (cp->name, cp->channel,buff);
}


void time_command(struct connection *cp)
{
	char buff[256];

        if (check_user_banned(cp, "TIME"))
		return;
	sprintf(buff, "%s%s at %s is %s %s", stars, tmstr, myhostname, ts2(currtime), mytimezone);
#ifdef	notdef	/* no reason to send this to channel */
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_SPAM, 1, buff))
												return;

	clear_locks();
	send_msg_to_channel(cp->name, cp->channel, buff);
#else
	append_general_notice(cp, buff);
	appendstring(cp, "\n");
#endif
}

static int DisplayFile(char *filename, struct connection *cp)
{
	FILE *fdi;
	char buffer[2048];
	int retval = 0;

	if (!access(filename, R_OK) && (fdi = fopen(filename, "r"))) {
		buffer[0] = '*';
		buffer[1] = ' ';
		while (!feof(fdi)) {
			fgets(buffer + 2, 2045, fdi);
			if (!feof(fdi))
				append_general_notice(cp, buffer);
		}
		fclose(fdi);
		retval = 1;
	}
	return (retval);
}



void tnoshelp_command(struct connection *cp)
{
	int k;
	char *cptr;
	char file2open[200];

	cptr = getarg(0, 0);

	strcpy(file2open, CONF_DIR);
	strcat(file2open, ConfNews);
	k = DisplayFile(file2open, cp);
	if (!k) {
		char buf[300];
		sprintf(buf, noopenstr, file2open);
		append_general_notice(cp, buf);
	}
	appendprompt(cp, 1);
}

void
roll_command(struct connection *cp)
{
	int die1, die2;
	char buff[2048];
	char buf[(NAMESIZE * 2) + 2];

        if (check_user_banned(cp, "ROLL"))
		return;

	conv_randomize();
	die1 = conv_random (6, 1);
	die2 = conv_random (6, 1);
 	clear_locks ();
	getTXname (cp, buf);
	sprintf (buff, rollstr, stars, buf, die1, die2, die1+die2, stars);
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_GAME, 1, buff))
		return;
	send_msg_to_channel(cp->name,cp->channel,buff);
}

void bjack_command(struct connection *cp)
{
	int die1, die2;
	char buff[2048];
	char buf[(NAMESIZE * 2) + 2];

        if (check_user_banned(cp, "BJACK"))
		return;

	conv_randomize();
	die1 = conv_random (11,1);
	die2 = conv_random (11,1);
	clear_locks ();
	getTXname (cp, buf);
	sprintf (buff, bjackstr, stars, buf, die1, die2, die1+die2, stars);
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_GAME, 1, buff))
		return;
	send_msg_to_channel(cp->name,cp->channel,buff);
}

void cut_command(struct connection *cp)
{
	int die1, die2;
	char buff[2048];
	char buf[(NAMESIZE * 2) + 2];
 
        if (check_user_banned(cp, "CUT"))
		return;

 	conv_randomize();
 	die1 = conv_random (4, 0);
 	die2 = conv_random (14, 0);
 	clear_locks ();
 	getTXname (cp, buf);
 	sprintf (buff, cutstr, stars, buf, cards[die2], suits[die1], stars);
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_GAME, 1, buff))
		return;
 	send_msg_to_channel(cp->name,cp->channel,buff);
}
 	
void news_command(struct connection *cp)
{
	char buf[8];

	strcpy(buf, "news");
	setargp(buf);
	tnoshelp_command(cp);
}


void cmdsummary_command(struct connection *cp)
{
	int i, n;
	char buf[82];
	char buf2[100];
	struct cmdtable *cmdp = cmdtable;
	int width = 13, count = 6;
	struct extendedcmds *ecmdp;

	sprintf(buf2, summary, stars, SOFT_RELEASE);
	append_general_notice(cp, buf2);
	for (n = 0; n < (cp->ircmode ? 2 : 1); n++) {
		if (cp->ircmode) {
			append_general_notice(cp, (n) ? "*** 2. Classic convers commands ***\n" : "*** 1. IRC-Client commands ***\n");
			cmdp = ((n) ? cmdtable : cmdtable_irc);
		}
		memset(buf, ' ', sizeof(buf));
		buf[78] = '\n';
		buf[79] = '\0';
		for (i = 0; cmdp->name; cmdp++, i = ((i + 1) % count)) {
			if ((unsigned char) *cmdp->name == 255)
				break;	/* skip hosts commands */
			if (cmdp->states == CM_UNKNOWN) {
				i--;
				continue;	/* skip non-user commands */
			}
			if (*cmdp->name == '/') {
				i--;
				continue;	/* skip invisible commands */
			}
			strncpy(&buf[1+(i * width)], cmdp->name, strlen(cmdp->name));
			if (i == (count - 1)) {
				append_general_notice(cp, buf);
				memset(buf, ' ', 78);
			}
		}
		if (i) {
			append_general_notice(cp, buf);
		} else {
			if (!cp->ircmode)
				appendstring(cp, "\n");
		}
	}
	if (ecmds != NULLEXTCMD && ecmds->name) {
		ecmdp = ecmds;
		memset(buf, ' ', sizeof(buf));
		buf[79] = '\n';
		buf[80] = '\0';
		sprintf(buf2, "\n%sExtended Remote Commands:\n ", stars);
		appendstring(cp, buf2);
		for (i = 0; ecmdp && ecmdp->name; ecmdp = ecmdp->next, i = ((i + 1) % count)) {
			strncpy(&buf[1+(i * width)], ecmdp->name, strlen(ecmdp->name));
			if (i == (count - 1)) {
				append_general_notice(cp, buf);
				memset(buf, ' ', 78);
			}
		}
		if (i)
			append_general_notice(cp, buf);
		else
			appendstring(cp, "\n");
	}
	appendprompt(cp, 1);
}
/* Quote server for future reference...

void quote_command(struct connection *cp)
{
	char buf2[512];
	char searchstr[5];
	char *cptr;
	FILE *fp;
	int dt;
	time_t tm;

        if (check_user_banned(cp, "QUOTE"))
		return;
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_SPAM, 1, "quote"))
		return;

	if ((fp = fopen(Quotes,READ_TEXT)) == NULLFILE)
		return;
	tm = time((time_t *)0);
	dt = atoi (&(ctime(&tm))[8]);
	sprintf (searchstr, "[%-d]", dt);
	while(fgets(buf2,512,fp) ! = NULLCHAR)	{
		if (!strncmp (searchstr, buf2, strlen(searchstr)))
			break;
	}
	if ((cptr = strchr (buf2, '%')) ! = NULLCHAR) 	{
		rip (buf2);
		fclose (fp);
		if ((fp = fopen (++cptr,READ_TEXT)) == NULLFILE)	{
			printf ("Couldn't open '%s'\n", &buf2[1]);	
			return;
		}
	}
	clear_locks ();
	send_msg_to_channel(cp->name,cp->channel,quotebanner);
	clear_locks ();
	while(fgets(buf2,512,fp) ! = NULLCHAR)	{
		if (*buf2 == '[')
			break;
		rip(buf2);
		send_msg_to_channel(cp->name,cp->channel,buf2);
		clear_locks ();
	}
	send_msg_to_channel(cp->name,cp->channel,stars);
	fclose (fp);
}
*/	

void h_loop_command(struct connection *cp)
{
	char buffer[2048];
	char *arg;
	int pl;

	arg = getarg(0, 1);
	if (strlen(arg) > 255)
		arg[255] = 0;
	do_log(L_R_ERR, "conversd rx: LOOP %s", arg);

	// lock host
	if (permarray) {
		for (pl = 0; pl < NR_PERMLINKS; pl++) {
			if (permarray[pl] && (permarray[pl]->connection == cp)) {
				permarray[pl]->locked = 1;
				permarray[pl]->locked_until = currtime + 1800;
				delay_permlink_connect(permarray[pl], 5);
				break;
			}
		}
	}

	sprintf(buffer, "*** live long and prosper%s", cp->ax25 ? "\r" : "\n");
	fast_write(cp, buffer, 0);
	sprintf(buffer, "%s<>%s broken (remote detected loop)", myhostname, cp->name);
	bye_command2(cp, buffer);

	// after having said goodbye, we generate an informational message
	sprintf(buffer, "/\377\200INFO %s got loop info from %s: %s", myhostname, cp->name, arg);
	do_log(L_LINK, buffer+3);
	getarg(buffer, 0);
	cp->locked = 1;
	h_info_command(cp);

}



#ifdef	notdef
/* Send updated personal data, nickname, and password */
void update_user_data(struct connection *cp, int personal, char *oldnickname)
{
	char buffer[2048];

	cp->locked = 0;
	        cp->locked = 0;
        if (personal) {
                send_user_change_msg(cp->name, cp->nickname, cp->host, cp->chann
el, cp->channel, cp->pers, 1, cp->time, cp->observer, 0, 0, 0);
                clear_locks();
        }
        oldnick = oldnickname;

        sprintf(buffer, uaddstr,
                cp->name, cp->host, (*cp->nickname && strcasecmp(cp->nickname, c
p->name)) ? cp->nickname : cp->name, "-1", (personal ? ((cp->pers && *cp->pers) 
? cp->pers : "@") : ""));

        strncpy(cp->ibuf, buffer, MAX_MTU);
        cp->ibuf[MAX_MTU-1] = 0;
        getarg(cp->ibuf, 0);
        h_uadd_command(cp);
}
#endif


int change_pers_and_nick(struct connection *cp, char *name, char *host, char *pers, int suggested_channel, char *nick, int do_announce, int flood_checked)
{
	struct connection *p;
	struct connection *p_found = 0;
	char oldnickname[NAMESIZE+1];
	int pers_changed = 0;
	int nick_changed = 0;

	if (cp->type == CT_USER && cp->observer)
		return 0;
	cp->locked = 1;
	if (pers && !*pers)
		pers = 0;

	// dummy
	*oldnickname = 0;

	for (p = connections; p; p = p->next) {
		// observers may not change their observer id
		if (p->observer)
			continue;
		if (p->type == CT_USER && (!p->via || p->via == cp) && !strcasecmp(p->name, name) && !strcasecmp(p->host, host)) {
			if (!p_found)
				p_found = p;
			if (pers && strcmp(p->pers, pers)) {
				if (!flood_checked) {
					if (check_cmd_flood(cp, name, host, SUL_PERS, 0, pers))
						return 0;
					flood_checked = 1;
				}
				if (suggested_channel < 0)
					suggested_channel = p->channel;
				strncpy(p->pers, pers, PERSSIZE);
				p->pers[sizeof(p->pers)-1] = 0;
				if (cp->type == CT_HOST) {
					p->isauth = (*pers == '~' ? 1 : 2);
				}
				pers_changed = 1;
			}
			if (nick && strcmp(p->nickname, nick)) {
				if (!flood_checked) {
					if (nick_changed && check_cmd_flood(cp, name, host, SUL_NICK, 0, nick))
						return 0;
					flood_checked = 1;
				}
				// backup nickname
				strncpy(oldnickname, p->nickname, sizeof(oldnickname));
				oldnickname[sizeof(oldnickname)-1] = 0;
				// actualize
				strncpy(p->nickname, (*nick ? nick : p->nickname), sizeof(p->nickname));
				p->nickname[sizeof(p->nickname)-1] = 0;
				p->nickname_len = strlen(p->nickname);
				nick_changed = 1;
			}
		}
	}
	if (p_found && do_announce && (pers_changed || nick_changed)) {
		send_user_change_msg(p_found->name, p_found->nickname, p_found->host, suggested_channel, suggested_channel, p_found->pers, pers_changed, currtime, 0, 0, 0, nick_changed, (*oldnickname ? oldnickname : p_found->name));
		return 2;
	}
	return 1;

}


void nonickname_command(struct connection *cp)
{
	char buf[PATH_MAX];
#ifdef	notdef
	char oldnickname[NAMESIZE+1];
#endif

	if (cp->ircmode) {
		char buffer[2048];
		sprintf(buffer, ":%s 421 %s NONICKNAME :Unknown command\n", myhostname, cp->nickname);
		appendstring(cp, buffer);
		return;
	}

	if (!strcasecmp(cp->name, cp->nickname)) {
		append_general_notice(cp, "*** There's no nickname to be changed.\n");
		return;
	}
	if (check_user_banned(cp, "NICK"))
		return;
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_NICK, 1, ""))
		return;

#ifdef	notdef
	strncpy(oldnickname, cp->nickname, NAMESIZE);
	oldnickname[NAMESIZE] = 0;
	strncpy(cp->nickname, cp->name, sizeof(cp->nickname));
	cp->nickname[sizeof(cp->nickname)-1] = 0;
	cp->nickname_len = strlen(cp->nickname);
	sprintf(buf, nicknamestr, stars, nicknmstr, "re", cp->name);
	append_general_notice(cp, buf);
	strcpy(buf, DATA_DIR);
	strcat(buf, "/nicknames/");
	strcat(buf, cp->name);
	unlink(buf);
	update_user_data(cp, 0, oldnickname);
#else
	change_pers_and_nick(cp, cp->name, cp->host, 0, -1, cp->name, 1, 1);
	sprintf(buf, nicknamestr, stars, nicknmstr, "re", cp->nickname);
	append_general_notice(cp, buf);
	strcpy(buf, DATA_DIR);
	strcat(buf, "/nicknames/");
	strcat(buf, cp->name);
	unlink(buf);
#endif
}


void nickname_command(struct connection *cp)
{
	char usernotefile[PATH_MAX];
#ifdef	notdef
	char oldnickname[NAMESIZE+1];
#endif
	char buf[80];
	char *new;
	int unf;

	strcpy(usernotefile, DATA_DIR);
	strcat(usernotefile, "/nicknames/");
	strcat(usernotefile, cp->name);
	new = getargcs(0, 0);
	if (!new || !*new || !strcasecmp(new, cp->nickname)) {
		sprintf(buf, nicknamestr, stars, nicknmstr, "", cp->nickname);
		append_general_notice(cp, buf);
		appendprompt(cp, 0);
		return;
	}
	if (*new == '@' || !strcasecmp(new, cp->name)) {
		nonickname_command(cp);
		return;
	}
	if (!validate_nickname(new)) {
		sprintf(buf, "*** Not a valid nickname\n");
		appendstring(cp, buf);
		appendprompt(cp, 0);
		return;
	}
	if (check_nick_dupes(cp, new)) {
		sprintf(buf, "*** Nickname is already in use\n");
		appendstring(cp, buf);
		appendprompt(cp, 0);
		return;
	}
	if (check_cmd_flood(cp, cp->name, myhostname, SUL_NICK, 1, new))
		return;


	if (!strcasecmp(new, systemstr) || !strcasecmp(new, conversd)) {
		sprintf(buf, nicknamestr, stars, nicknmstr, "can't be ", new);
		append_general_notice(cp, buf);
		return;
	}

	//strncpy(oldnickname, cp->nickname, NAMESIZE);
	//oldnickname[NAMESIZE] = 0;
	new[16] = 0;
#ifdef	notdef
	if (strlen(new) > NAMESIZE)
		new[NAMESIZE] = 0;
	strncpy(cp->nickname, new, NAMESIZE);
	cp->nickname[sizeof(cp->nickname)-1] = 0;
	cp->nickname_len = strlen(cp->nickname);
	if ((unf = creat(usernotefile, 0644)) != -1) {
		write(unf, new, strlen(new));
		close(unf);
	}
	sprintf(buf, nicknamestr, stars, nicknmstr, "", cp->nickname);
	append_general_notice(cp, buf);
	appendprompt(cp, 0);
	update_user_data(cp, 0, oldnickname);
#else
	if ((unf = creat(usernotefile, 0644)) != -1) {
		write(unf, new, strlen(new));
		close(unf);
	}
	change_pers_and_nick(cp, cp->name, cp->host, 0, -1, new, 1, 1);
	sprintf(buf, nicknamestr, stars, nicknmstr, "", cp->nickname);
	append_general_notice(cp, buf);
	appendprompt(cp, 0);
#endif
}





void realname_command(struct connection *cp)
{
	char *name;
	char buf[80];

	name = getarg(0, 0);
	sprintf(buf, "r %s", name);
	setargp(buf);
	who_command(cp);
}


char *make_pers_consistent(char *pers) {
        static char nopers[2]; 

        if (!pers || !*pers) {  
		*nopers = 0;
		return nopers; // no pers _change_
	}
	while (*pers == '@')
		pers++;

	if (!*pers) {
		// pers is "empty"
                strcpy(nopers, "@");
                return nopers;
        }

        // this came from tnos.c u_add():
        // it seems to be a compatibility fix. no idea, why they send
        // "~~" sometimes (i think they made up a third alternative for
	// transmitting "empty" perstext)
        // "~" in saupp brands user nonauth. because above "~~" is easy to
        // fake (and via a tnos compliant host it will become an empty pers
	// text), we've to translate this to "~" in order not to break
	// our auth model
        while (*pers == '~' && pers[1] == '~') {
		pers++;
        }
        return pers;
}

/* Command to take user's personal data across a link */
void h_uadd_command(struct connection *cp)
{
	char *name, *host, *data, *nickname, *channel;
#ifdef	notdef
	struct connection *p, *p2;
	struct clist *cl, *cl2;
	char buffer[2048];
	char buffer2[2048];
	char oldnickname[NAMESIZE+1];
	int pers_changed, nick_changed;
	int chan = -1;
#endif

	if (cp->type == CT_USER && !cp->via && cp->ircmode) {
		// do not announce as long no channel joined
		// announce will be done later on channel_command()
		if (!cp->chan_list)
			return;
	}

	name = getarg(0, 0);
	host = getargcs(0, 0);
	nickname = getargcs(0, 0);
	channel = getarg(0, 0);
	data = getargcs(0, 1);
	data = make_pers_consistent(data);
	if (!user_check(cp, name))
		return;
	cp->locked = 1;
	change_pers_and_nick(cp, name, host, (*data ? data : 0), -1, nickname, 1, 0);

#ifdef	notdef	/* now handled by send_user_change_msg() */
	sprintf(buffer, uaddstr,
		name, host, nickname, *data ? "-1" : "", data);
	strcat(buffer, "\n");
	pers_changed = nick_changed = 0;
	for (p = connections; p; p = p->next) {
		if (!strcasecmp(p->name, name) && !strcasecmp(p->host, host)) {
			if (*data && strcmp(p->pers, data)) {
				if (cp->type == CT_HOST) {
					if (*data == '~')
						p->isauth = 1;
					pers_changed = 1;
				}
				strncpy(p->pers, data, PERSSIZE);
				p->pers[sizeof(p->pers)-1] = 0;
				// else: already announced by update_user_data()
			}

			// if not stored? i's a permlink notification
			if (!oldnick) {
				// cave: oldnick is an external modified variable.
				strncpy(oldnickname, p->nickname, NAMESIZE);
				oldnickname[NAMESIZE] = 0;
				oldnick = oldnickname;
			}

			// nick change?
			if (strcmp(oldnick, nickname))
				nick_changed = 1;
			if (!nick_changed && !pers_changed)
				continue;

			strncpy(p->nickname, nickname, NAMESIZE);
			p->nickname[sizeof(p->nickname)-1] = 0;
			p->nickname_len = strlen(p->nickname);

			// announce to local user or permlink
			for (p2 = connections; p2; p2 = p2->next) {
				int announce = 0;
				if (p2->locked)
					continue;
				if (p2->type == CT_USER) {
					if (p2->via)
						continue;
					if (p2 == cp)
						continue;
#ifdef WANT_FILTER
                        		if (p2->filter) {
                                		if (is_filtered(0, name, p2->filter, 0)) {
                                        		p2->locked = 1;
                                        		continue;
                                		}
                        		}
#endif
					if (p2->channel == p->channel) {
						announce = 1;
						chan = p->channel;
					} else {
						for (cl2 = p2->chan_list; !announce && cl2; cl2 = cl2->next) {
							if (cl2->channel == p->channel)  {
								announce = 1;
								chan = p->channel;
								break;
							} else {
								for (cl = p->chan_list; cl; cl = cl->next) {
									if (cl->channel == cl2->channel) {
										announce = 1;
										chan = cl2->channel;
										break;
									}
								}
							}
						}
					}
					if (announce) {
						if (nick_changed && p2->shownicks) {
							appendc(p2, 0, 0);
							appendc(p2, 3, 0);
							if (p2->ircmode)
								sprintf(buffer2, ":%s!%s@%s NICK %s\n", oldnick, p->name, p->host, p->nickname);
							else {
								int nonick = !strcasecmp(p->name, oldnick);
								sprintf(buffer2, "*** (%s) %s%s%s is now known as %s.\n", ts2(currtime), p->name, (nonick ? "" : ":"), (nonick ? "" : oldnick), p->nickname);
							}
							appendstring(p2, buffer2);
							appendprompt(p2, 0);
						}
						if (pers_changed) {
							appendc(p2, 0, 0);
							appendc(p2, 3, 0);
							if (p2->ircmode) {
                                        			sprintf(buffer2, ":%s 311 %s %s %s %s * :%s", myhostname, p2->nickname, (p2->shownicks ? p->nickname : p->name), p->name, p->host, (*data && *data != '@' && strcmp(data, "~")) ? (*p->pers == '~' ? (p->pers + 1) : p->pers) : "");
                                        			buffer2[IRC_MAX_MSGSIZE] = 0;
                                        			appendstring(p2, buffer2);
                                        			appendstring(p2, "\n");
                                			} else {
								if ((*data != '\0') && (strcmp(data, "@") && strcmp(data, "~"))) {
                                                			sprintf(buffer2, "*** (%s) %s@%s set personal text:\n    %s\n", ts2(currtime), p->name, p->host, (*p->pers == '~' ? (p->pers + 1) : p->pers));
                                        			} else {
                                                			sprintf(buffer2, "*** (%s) %s@%s removed personal text.\n", ts2(currtime), p->name, p->host);
                                        			}
                                        			append_chan_notice(p2, buffer2, chan);
							}
							appendprompt(p2, 0);
						}
					}
				}
				else if (p2->type == CT_HOST) {
					if (p2->features & FEATURE_NICK) {
						linklog(p2, L_SENT, "%s", &buffer[3]);
						appendstring(p2, buffer);
					}
				}
				p2->locked = 1;
			}
		}
	}
	oldnick = 0;
#endif
}

/*------------------------------------------------------------------------*/

#define WxProg "/usr/local/bin/wx"

void wx_command(struct connection *cp)
{
	char *cptr;
	char str[256];
	FILE *fp;

	if (check_user_banned(cp, "WX"))
		return;
	cptr = getarg(0, 0);
	
	if ((fp = popen(WxProg, "r")) == NULL) {
		sprintf(str, "*** popen: %s\n", strerror(errno));
		append_general_notice(cp, str);
		appendprompt(cp, 0);
	} else {
		strcpy(str, "*** ");
		if (fgets(str + 4, 251, fp) != NULL) {
			rip(str);
			if (!check_cmd_flood(cp, cp->name, myhostname, SUL_CHAN_SPAM, 1, str)) {
				clear_locks();
				send_msg_to_channel(conversd, cp->channel, str);
				/* appendstring(cp, str); */
				/* appendprompt(cp, 0); */
			}
		}
		pclose(fp);
	}
	return;
}
