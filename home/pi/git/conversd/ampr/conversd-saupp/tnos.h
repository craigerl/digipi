/*
 * This is Tampa Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@mdddhd.fc.hp.com>
 *
 * By Brian A. Lantz <brian@lantz.com>
 */

#ifndef TNOS_H

extern int conv_rand(void);
extern void conv_randomize(void);
extern int conv_random(int num, int base);

extern void smiley_command(struct connection *cp);
extern void spam_command(struct connection *cp);
extern void time_command(struct connection *cp);
extern void roll_command(struct connection *cp);
extern void cut_command(struct connection *cp);
extern void bjack_command(struct connection *cp);
extern void tnoshelp_command(struct connection *cp);
extern void news_command(struct connection *cp);
extern void cmdsummary_command(struct connection *cp);
/* extern void quote_command(struct connection *cp); */
extern void nickname_command(struct connection *cp);
extern void nonickname_command(struct connection *cp);
extern void realname_command(struct connection *cp);
extern void h_uadd_command(struct connection *cp);
extern void update_user_data(struct connection *cp, int personal, char *oldnick);
extern int change_pers_and_nick(struct connection *cp, char *name, char *host, char *pers, int suggested_channel, char *nick, int do_announce, int is_flood_checked);
extern void h_loop_command(struct connection *cp);

extern void setargp(char *str);
extern char *rip(char *str);
#ifdef	notdef	// nowhere uses anymore
extern char *skipwhite(char *cp);
extern char *skipnonwhite(char *cp);
#endif

extern char *make_pers_consistent(char *pers);

extern char help_bjack[];
extern char help_cut[];
extern char help_news[];
extern char help_nick[];
extern char help_nonick[];
/* extern char help_quote[]; */
extern char help_real[];
extern char help_roll[];
extern char help_smile[];
extern char help_spam[];
extern char help_summ[];
extern char help_time[];

extern void wx_command(struct connection *cp);

#endif
