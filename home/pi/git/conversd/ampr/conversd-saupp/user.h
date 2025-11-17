/*
 * This is Tampa Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@mdddhd.fc.hp.com>
 * Modifications by Fred Baumgarten <dc6iq@insu1.etec.uni-karlsruhe.de>
 *
 * Modifications by Brian A. Lantz/KO4KS <brian@lantz.com>
 */

#ifndef USER_H
#define USER_H

extern void all_command(struct connection *cp);
extern void away_command(struct connection *cp);
extern void beep_command(struct connection *cp);
extern void bye_command2(struct connection *cp, char *reason);
extern void bye_command(struct connection *cp);
extern void channel_command(struct connection *cp);
extern void charset_command(struct connection *cp);
extern void chinfo_command(struct connection *cp);
extern void cq_command(struct connection *cp);
extern void cstat_command(struct connection *cp);
extern void display_links_command(struct connection *cp, char *user, char *lookup);
extern void filterusers_command(struct connection *cp);
extern void filterwords_command(struct connection *cp);
extern void filtermsgs_command(struct connection *cp);
extern void help_command(struct connection *cp);
extern void hosts_command(struct connection *cp);
extern void invite_command(struct connection *cp);
extern void imsg_command(struct connection *cp);
extern void language_command(struct connection *cp);
extern void leave_command(struct connection *cp);
extern void links_command(struct connection *cp);
extern void list_command(struct connection *cp);
extern void last_command(struct connection *cp);
extern void msg_command(struct connection *cp);
extern void me_command(struct connection *cp);
extern void mode_command(struct connection *cp);
extern void name_command(struct connection *cp);
extern void notify_command(struct connection *cp);
extern void observer_command(struct connection *cp);
extern void operator_command(struct connection *cp);
extern void priv_command(struct connection *cp);
extern void personal_command(struct connection *cp);
extern void prompt_command(struct connection *cp);
extern void query_command(struct connection *cp);
extern void restart_command(struct connection *cp);
extern void restricted_command(struct connection *cp);
extern void stats_command(struct connection *cp);
extern void sysinfo_command(struct connection *cp);
extern void topic_command(struct connection *cp);
extern void uptime_command(struct connection *cp);
extern void verbose_command(struct connection *cp);
extern void version_command(struct connection *cp);
extern void who_command(struct connection *cp);
extern void who_command2(struct connection *cp);
extern void whois_command(struct connection *cp);
extern void wall_command(struct connection *cp);
extern void width_command(struct connection *cp);
extern void wizard_command(struct connection *cp);
extern void paclen_command(struct connection *cp);
extern void idle_command(struct connection *cp);
extern void trigger_command(struct connection *cp);
extern void echo_command(struct connection *cp);
extern void rtt_command(struct connection *cp);
extern void profile_command(struct connection *cp);
extern void profile_change(struct connection *cp);
extern void comp_command(struct connection *cp);
extern void history_command(struct connection *cp);
extern void cron_command(struct connection *cp);
extern void irc_command(struct connection *cp);
extern void user_login_common_partA(struct connection *cp);
extern void user_login_convers_partA(struct connection *cp);
extern void user_login_common_partB(struct connection *cp);
extern void notice_command(struct connection *cp);
extern void auth_command(struct connection *cp);
extern void mkpass_command(struct connection *cp);
extern void map_command(struct connection *cp);
extern void banlist_command(struct connection *cp);
extern void banparam_command(struct connection *cp);
extern void listsul_command(struct connection *cp);
extern void hushlogin_command(struct connection *cp);
extern void shownicks_command(struct connection *cp);
extern void timestamp_command(struct connection *cp);

extern void ecmdtest_command(struct connection *cp);

extern int stringisnumber(char *s);
extern int do_link(struct permlink ***pa, int argc, char **argv);
extern int allowed_unix_paths(char *s);
extern int is_hushlogin(struct connection *cp);
extern int update_hushlogin(struct connection *cp, int what);
extern int handle_hushlogin(struct connection *cp, int what);
extern void update_channel_history(struct connection *p_via, int channel, char *name, char *host);

extern struct connection *sort_connections(int init);
extern struct channel *new_channel(int channel, char *fromname);

extern char help_all[];
extern char help_away[];
extern char help_beep[];
extern char help_char[];
extern char help_cq[];
extern char help_dest[];
extern char help_excl[];
extern char help_filt[];
extern char help_halt[];
extern char help_help[];
extern char help_invi[];
extern char help_join[];
extern char help_last[];
extern char help_leav[];
extern char help_link[];
extern char help_list[];
extern char help_me[];
extern char help_mode[];
extern char help_msg[];
extern char help_noti[];
extern char help_oper[];
extern char help_pers[];
extern char help_prom[];
extern char help_quer[];
extern char help_quit[];
extern char help_rest[];
extern char help_sysi[];
extern char help_topi[];
extern char help_upti[];
extern char help_verb[];
extern char help_vers[];
extern char help_who[];
extern char help_whoi[];
extern char help_widt[];

#endif
