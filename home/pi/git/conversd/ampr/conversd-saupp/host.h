/*
 * This is Tampa Ping-Pong convers/conversd derived from the wampes
 * convers package written by Dieter Deyke <deyke@mdddhd.fc.hp.com>
 * Modifications by Fred Baumgarten <dc6iq@insu1.etec.uni-karlsruhe.de>
 *
 * Modifications by Brian A. Lantz/KO4KS <brian@lantz.com>
 */

#ifndef HOST_H
#define HOST_H

extern void h_away_command(struct connection *cp);
extern void h_ban_command(struct connection *cp);
extern void h_cmsg_command(struct connection *cp);
extern void h_cdat_command(struct connection *cp);
extern void h_dest_command(struct connection *cp);
extern void h_ecmd_command(struct connection *cp);
#ifdef	WANT_FILTER
extern void h_filt_command (struct connection *cp);
#endif
extern void h_help_command(struct connection *cp);
extern void h_host_command(struct connection *cp);
extern void h_invi_command(struct connection *cp);
extern void h_info_command(struct connection *cp);
extern void h_link_command(struct connection *cp);
extern void h_netjoin_command(struct connection *cp);
extern void h_oper_command(struct connection *cp);
extern void h_ping_command(struct connection *cp);
extern void h_pong_command(struct connection *cp);
extern void h_rout_command(struct connection *cp);
extern void h_sysi_command(struct connection *cp);
extern void h_topi_command(struct connection *cp);
extern void h_udat_command(struct connection *cp);
extern void h_unknown_command(struct connection *cp);
extern void h_umsg_command(struct connection *cp);
extern void h_user_command(struct connection *cp);
extern void h_uquit_command(struct connection *cp);
extern void h_squit_command(struct connection *cp);

extern void loop_handler(struct connection *cp, struct permlink *pl, int reason, char *user, char *host, char *already_via);

#endif
