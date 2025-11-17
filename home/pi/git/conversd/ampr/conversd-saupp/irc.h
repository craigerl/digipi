#ifndef IRC_H
#define IRC_H

extern char *irc_sender;

extern void irc_admin_command(struct connection *cp);
extern void irc_clink_command(struct connection *cp);
extern void irc_cnotify_command(struct connection *cp);
extern void irc_connect_command(struct connection *cp);
extern void irc_ison_command(struct connection *cp);
extern void irc_list_command(struct connection *cp);
extern void irc_links_command(struct connection *cp);
extern void irc_lusers_command(struct connection *cp);
extern void irc_names_command(struct connection *cp);
extern void irc_nick_command(struct connection *cp);
extern void irc_notice_command(struct connection *cp);
extern void irc_operator_command(struct connection *cp);
extern void irc_pass_command(struct connection *cp);
extern void irc_part_command(struct connection *cp);
extern void irc_ping_command(struct connection *cp);
extern void irc_pong_command(struct connection *cp);
extern void irc_privmsg_command(struct connection *cp);
extern void irc_squit_command(struct connection *cp);
extern void irc_user_command(struct connection *cp);
extern void irc_userhost_command(struct connection *cp);
extern void irc_users_command(struct connection *cp);
extern void irc_who_command(struct connection *cp);
extern void irc_whois_command(struct connection *cp);

extern int validate_nickname(char *s);
extern void user_login_irc_partA(struct connection *cp);
extern int check_nick_dupes(struct connection *cp, char *nick);
char *get_mode_flags2irc(int flags);

#endif
