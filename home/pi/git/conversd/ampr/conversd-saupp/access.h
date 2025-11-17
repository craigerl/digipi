
#ifndef ACCESS_H
#define ACCESS_H

#define	ACC_BAN		0
#define	ACC_USER	1
#define	ACC_HOST	2
#define	ACC_RESTR	4
#define	ACC_OBSRV	8
#define	ACC_HOST_SECURE	16
#define	ACC_AUTH	32
#define	ACC_DROP	64

extern int do_access(void *list, int argc, char **argv);
extern int modify_acl(struct connection *cp, int arc, char **argv);
extern int chk_access(const struct sockaddr *addr, char *local_name);
extern int check_password(struct connection *cp);
extern int update_password(char *foruser, char *password);
extern char *read_password(char *foruser);

extern void ask_pw_plain(struct connection *cp, char *pw);
extern void ask_pw_sys(struct connection *cp, char *pw);
extern void ask_pw_md5(struct connection *cp, char *pw);
extern char *generate_rand_pw(int len);

extern int user_matches(char *name1, char *name2);
extern char *get_tidy_name(char *name);

extern char *compute_aprs_pass(char *call);

#endif
