
/*
 *	Access control list for htpp
 *	
 *	by Tomi Manninen, OH2BNS
 *
 *	Password authentication for saupp
 *	by Thomas Osterried <dl9sau>
 *	(c) 2002 Thomas Osterried <dl9sau>
 *      License: GPL
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
#include "access.h"
#include "log.h"
#include "hmalloc.h"
#include "config.h"
#include "ba_stub.h"
#include "cfgfile.h"
#include "user.h"
#include "tnos.h"
#include "md5_ampr.h"

extern struct sockaddr *build_sockaddr(const char *name, int *addrlen);
extern long seed;

typedef struct access_s {
	unsigned int	network;
	unsigned int	netmask;
	int		mode;
#if defined (HAVE_AF_UNIX) || defined (HAVE_AF_AX25)
#ifdef	__linux__
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,0)
	short		proto;
#else
	sa_family_t	proto;
#endif
#else
	sa_family_t	proto;
#endif
#endif
	char		*source_addr;
	time_t		last_resolved;
	struct access_s	*next;
} acclist_t;

static struct bitlist_t modelist[] = {
	{ "BAN",	ACC_BAN   },
	{ "USER",	ACC_USER  },
	{ "HOST",	ACC_HOST  },
	{ "RESTRICTED",	ACC_RESTR },
	{ "OBSERVER",	ACC_OBSRV },
	{ "HOST_SECURE",ACC_HOST_SECURE },
	{ "AUTH",	ACC_AUTH },
	{ "DROP",	ACC_DROP  },
	{ NULL,		-1        }
};

static acclist_t *accesslist = NULL;


/*
 *	Convert bits to bitmask
 */

static unsigned long bits_to_mask(int bits)
{
	/* GCC seems to have problems shifting with 32 */
	if (bits == 0)
		return 0L;

        return htonl(0xffffffffL << (32 - bits));
}

/*
 *	Configuration file entry "Access"
 */

int do_access(void *list, int argc, char **argv)
{
	acclist_t *ap;
	char *cp, buf[256];
	
	if (argc < 3)
		return -1;
	
	if ((ap = hcalloc(1, sizeof(acclist_t))) == NULL) {
		/* no memory */
		return -1;
	}
	
	if ((!strncmp(argv[1], "unix:", 5) || !strncmp(argv[1], "local:", 6)) && (cp = strchr(argv[1], ':'))) {
#ifdef	HAVE_AF_UNIX
		ap->proto = AF_UNIX;
		ap->source_addr = hstrdup(cp+1);
		sprintf(buf, "unix socket: %s", ap->source_addr);
#else
		do_log(L_CONFIG, "unix sockets not supported on this architecture");
		hfree(ap);
		return -1;
#endif
	} else if (!strncmp(argv[1], "ax25:", 5)) {
#ifdef	HAVE_AF_AX25
		ap->proto = AF_AX25;
		ap->source_addr = hstrdup(&argv[1][5]);
		strupr(ap->source_addr);
		sprintf(buf, "ax25: %s", ap->source_addr);
#else
		do_log(L_CONFIG, "protocol ax25 not supported on this architecture");
		hfree(ap);
		return -1;
#endif
	} else {
		struct in_addr inaddr;
		struct hostent *hp;

		ap->proto= AF_INET;
		ap->netmask = 0xffffffffL;
		
		if ((cp = strchr(argv[1], '/')) != NULL) {
			*cp = 0;
			ap->netmask = bits_to_mask(atoi(++cp));
		}
		
		if ((hp = gethostbyname(argv[1])) != NULL) {
			ap->network = ((struct in_addr *)(hp->h_addr))->s_addr;
		} else {
			/* unknown host/network */
			if (ap->netmask != 0xffffffffL) {
				hfree(ap);
				return -1;
			}
			// resolve host later
			ap->network = 0L;
		}

		if (ap->netmask == 0xffffffffL) {
			char *p;
			int matches;
			inaddr.s_addr = ap->network;

			p = (hp ? inet_ntoa(inaddr) : "unresolved");
			matches = (hp && !strcmp(p, argv[1]));

			sprintf(buf, "host: %s", argv[1]);
			if (!matches) {
				// host name does not match. store arg[1]
				// for later resolving
				ap->source_addr = hstrdup(argv[1]);
				ap->last_resolved = currtime;
				sprintf(buf+strlen(buf), " [%s] (forward resolve)", p);
			}
		} else {
			inaddr.s_addr = ap->network;
			sprintf(buf, "network: %-15s ", inet_ntoa(inaddr));
			inaddr.s_addr = ap->netmask;
			sprintf(buf + 25, "netmask: %-15s", inet_ntoa(inaddr));
		}
	
#ifdef BITCHING_MODE
		if (ap->network & ~(ap->netmask)) {
			/* network address and netmask don't match */
			hfree(ap);
			return -1;
		}
#else
		ap->network &= ap->netmask;
#endif
	}
	
	if ((ap->mode = bitcfg(modelist, argc - 2, &argv[2])) == -1)
		return -1;
		
	ap->next = accesslist;
	accesslist = ap;
	
	if (ap->mode)
		printbits(ap->mode, modelist, buf, 256);
	else
		strcat(buf, " BAN");
	
	do_log(L_CONFIG, "Access: %s", buf);
	
	return 0;
}

/*
 *	Check address against ACL
 */

int chk_access(const struct sockaddr *addr, char *local_name)
{
	acclist_t *ap = accesslist;
	int ret = ACC_BAN;
	struct sockaddr_in *sin = (struct sockaddr_in *) addr;
#ifdef	HAVE_AF_AX25
	struct sockaddr_ax25 *sax = (struct sockaddr_ax25 *) addr;
	char tmpbuf[256];
	char *to_local_call, *want_to_local_call;
	char *from_src_call, *want_from_src_call;
#endif
	char *p = 0;

	if (!addr)
		return ret;

	if (ap == NULL)
		return ACC_USER | ACC_HOST | ACC_HOST_SECURE;
	
	/* Walk the list. Last match counts (actually first in the file) */
	for ( ; ap != NULL; ap = ap->next) {
		if (ap->proto != addr->sa_family) {
			continue;
		}
		switch (ap->proto) {
		case AF_INET:
			if (ap->source_addr && currtime - ap->last_resolved > 3*60L) {
				// active resolving
				struct hostent *hp;
				if ((hp = gethostbyname(ap->source_addr)) != NULL) {
					ap->network = ((struct in_addr *)(hp->h_addr))->s_addr;
				} else {
					ap->network = 0L;
				}
				ap->last_resolved = currtime;
			}
			if ((sin->sin_addr.s_addr & ap->netmask) == ap->network) {
				// match
				ret = ap->mode;
				if (ap->source_addr) {
					// dynamic address seems still to be valid
					// update resolve time for better performance if he connects again really soon
					ap->last_resolved = currtime;
				}
			}
			break;
#ifdef	HAVE_AF_UNIX
		case AF_UNIX:
			if (ap->source_addr &&
					(!strcmp(ap->source_addr, "*") ||
					(local_name && (p = strchr(local_name, ':')) && !strcmp(ap->source_addr, p+1))))
				ret = ap->mode;
			break;
#endif
#ifdef	HAVE_AF_AX25
		case AF_AX25:
			if (!ap->source_addr)
				continue;
			from_src_call = ax25_ntoa(&sax->sax25_call);
			if (callvalidate && !callvalid(from_src_call))
					continue;
			if (local_name && !strncmp(local_name, "ax25:", 5) && strlen(local_name) > 5) {
				to_local_call = &local_name[5];
			} else {
				to_local_call = "*";
			}
			strcpy(tmpbuf, ap->source_addr);
			want_from_src_call = tmpbuf;
			if ((want_to_local_call  = strchr(tmpbuf, '>'))) {
				*want_to_local_call++ = 0;
				if (!*want_from_src_call)
					want_from_src_call = "*";
			} else
				want_to_local_call = "*";

			// currently, we check only against calls without digipeaters.
			if ((!strcmp(want_from_src_call, "*") || !strcasecmp(want_from_src_call, from_src_call)) &&
			      (!strcmp(want_to_local_call, "*") || !strcasecmp(want_to_local_call, to_local_call)))
				ret = ap->mode;
			break;
#endif
		}
	}

	/* ACC_HOST_SECURE implies ACC_HOST - this is what the sysop expects ;) */
	if ((ret & ACC_HOST_SECURE) && !(ret & ACC_HOST))
		ret |= ACC_HOST;

	if (addr->sa_family == AF_INET) {
		if ((ret & ACC_HOST_SECURE) && htons(sin->sin_port) >= IPPORT_RESERVED) {
			if ((ret & ~(ACC_HOST_SECURE | ACC_HOST)))
				ret &= ~(ACC_HOST_SECURE | ACC_HOST);  /* user connects may be allowed */
			else
				ret = ACC_BAN;
		}
	}
	
	
	return ret;
}

/*--------------------------------------------------------------------------*/

int modify_acl(struct connection *cp, int argc, char *argv[])
{
	char buffer[1024];
	int ret = -1;
	acclist_t *ap;

	if (!cp || !argv)
		return -2;

	switch (argc) {
	case 0:
	case 2:
		sprintf(buffer, "*** (%s) ACL modify: wrong parameters.\n", ts2(currtime));
		append_general_notice(cp, buffer);
		break;
	case 1:
		// show acl list

		// current accesslist empty?
		if (!accesslist) {
			sprintf(buffer, "*** (%s) ACL is empty.\n", ts2(currtime));
			append_general_notice(cp, buffer);
			ret = 0;
			break;
		}
		sprintf(buffer, "*** (%s) ACL entries\n", ts2(currtime));
		append_general_notice(cp, buffer);
		for (ap = accesslist; ap; ap = ap->next) {
			struct in_addr inaddr;
			switch (ap->proto) {
			case AF_INET:
				inaddr.s_addr = ap->network;
				if (ap->netmask == 0xffffffffL) {
					sprintf(buffer, "host: %s", inet_ntoa(inaddr));
					if (ap->source_addr)
						sprintf(buffer+strlen(buffer), " [%s] (forward resolve)", ap->source_addr);
				} else {
					sprintf(buffer, "net: %s/", inet_ntoa(inaddr));
					inaddr.s_addr = ap->netmask;
					strcat(buffer+strlen(buffer), inet_ntoa(inaddr));
				}
				break;
#ifdef	HAVE_AF_UNIX
			case AF_UNIX:
				sprintf(buffer, "unix socket: %s", ap->source_addr);
				break;
#endif
#ifdef	HAVE_AF_AX25
			case AF_AX25:
				sprintf(buffer, "ax25: %s", ap->source_addr);
				break;
#endif
			default:
				continue;
			}
			if (strlen(buffer) > 64)
				buffer[64] = 0;
			if (ap->mode)
				printbits(ap->mode, modelist, buffer, sizeof(buffer));
			else
				strcat(buffer, " BAN");
			append_general_notice(cp, buffer);
			appendstring(cp, "\n");
		}
		appendprompt(cp, 1);
		break;
	default:
		ret = do_access(0, argc, argv);
		if (ret < 0)
			sprintf(buffer, "*** (%s) ACL modify failed (%d)\n", ts2(currtime), ret);
		else {
			// do_access puts it to wrong position
			if (accesslist->next) {
				for (ap = accesslist->next; ap->next; ap = ap->next) ;
				ap->next = accesslist;
				accesslist = accesslist->next;
				ap->next->next = 0;
			}
			sprintf(buffer, "*** (%s) ACL insert: success.\n", ts2(currtime));
		}
		append_general_notice(cp, buffer);
		break;
	}

	return ret;
}

/*--------------------------------------------------------------------------*/

int verify_password(char *got, char *want)
{
	if (!*got)
		return -1;
#ifdef	notdef
	if (!strcmp(got, want))
		return 0;
#else
	if (strstr(got, want))
		return 0;	// baycomm standard: hide password in long answer
#endif
	return -1;
}

/*--------------------------------------------------------------------------*/

int lock_fd(int fd, int exclusive, int dont_block)
{ 
  struct flock flk;

  flk.l_type = exclusive ? F_WRLCK : F_RDLCK;
  flk.l_whence = SEEK_SET;
  flk.l_start = 0;
  flk.l_len = 0;
  return fcntl(fd, dont_block ? F_SETLK : F_SETLKW, &flk);
}

/*--------------------------------------------------------------------------*/

int lock_file(const char *filename, int exclusive, int dont_block)
{
  int fd;

  if ((fd = open(filename, O_RDWR | O_CREAT, 0644)) < 0)
    return -1;
  if (!lock_fd(fd, exclusive, dont_block))
    return fd;
  close(fd);
  return -1;
}

/*--------------------------------------------------------------------------*/

char *get_tidy_name(char *name)
{
	static char nn[NAMESIZE+1];
	char *n = nn;
	char *q;
	if (name) {
		strncpy(n, name, NAMESIZE);
		n[NAMESIZE] = 0;
		while (*n && (q = strpbrk(n, "-/_\\.:"))) {
			if (q != n) {
				*q = 0;
				break;
			}
			n++;
		}
	} else {
		*n = 0;
	}
	return n;
}

/*--------------------------------------------------------------------------*/

int user_matches(char *name1, char *name2)
{
	static char n1[NAMESIZE+1];
	static char n2[NAMESIZE+1];

	if (name1) {
		strncpy(n1, get_tidy_name(name1), NAMESIZE);
		n1[NAMESIZE] = 0;
	}
	if (name2) {
		strncpy(n2, get_tidy_name(name2), NAMESIZE);
		n2[NAMESIZE] = 0;
	} else
		return 0;
	return (strcasecmp(n1, n2) ? 0 : 1);
}

/*--------------------------------------------------------------------------*/

char *change_password_file(char *foruser, char *hispass, int do_write)
{
#define	PASSWDFILE	DATA_DIR "/passwd"

	FILE *fi;
	static char password[PASSSIZE+1];
	char buffer[2048];
	char name[NAMESIZE+1];
	char compare_with[NAMESIZE+1+1];
	char *q, *pw;
	int fd = -1;
	int fdlock = -1;
	int i;
	int changed = 0;

	if (!foruser || !*foruser || (*foruser == '~' && !*(foruser+1)))
		return 0;

	if (*foruser == '~')
		foruser++;
	strncpy(name, get_tidy_name(foruser), NAMESIZE);
	name[NAMESIZE] = 0;
	if (!*name)
		return 0;
	
	if (do_write == 0)  // read
		pw = 0;
	else {		    // update
		pw = hispass;

		// lock passwd file against passwd file for our sisters like wconversd, etc..
    		for (i = 0; ((fdlock = lock_file(PASSWDFILE ".LCK", 1, 0)) < 0) && i < 3; i++) {
      			sleep(1);
    		}
    		if (fdlock < 0)
			goto log_err;
		// delete ancient tmp file (just 2b sure)
		unlink(PASSWDFILE ".tmp");
		if ((fd = open(PASSWDFILE ".tmp", O_WRONLY | O_CREAT | O_EXCL, 0660)) < 0) {
			goto log_err;
		}
		sprintf(compare_with, "%s:", name);

	}


	if ((fi = fopen(PASSWDFILE, "r"))) {
		while (fgets(buffer, sizeof(buffer), fi)) {
			if (!*buffer || *buffer == '\n')
				continue;
			if (do_write) {
				if (!strncmp(buffer, compare_with, strlen(compare_with))) {
					// found user.
					if (!pw) // pw 2b deleted -> skip
						continue;
					// change buffer
					sprintf(buffer, "%s%s\n", compare_with, pw);
					// and write it..
					changed = 1;
				}
				if (write(fd, buffer, strlen(buffer)) < 0) {
					fclose(fi);
					goto log_err;
				}
				continue;
			}
			if ((q = strchr(buffer, '#')))
				*q = 0;
			if (!*buffer)
				continue;
			// user:pass notation
			if (!(q = strchr(buffer, ':')))
				continue;
			*q++ = 0;
			rip(buffer);
			if (!*buffer)
				continue;
			if (strcmp(name, buffer))
				continue;
			pw = q;
			while (*pw && isspace(*pw & 0xff))
				pw++;
			rip(pw);
			*password = 0;
			if (*pw) {
				// null password -> ok
				strncpy(password, pw, sizeof(password));
				password[sizeof(password)-1] = 0;
			}
			pw = password;
			break;
		}
		fclose(fi);
	}
	
	if (do_write) {
		if (!changed && pw) {
			// add new password for new user
			sprintf(buffer, "%s:%s\n", name, pw);
			if (write(fd, buffer, strlen(buffer)) < 0) {
				fclose(fi);
				goto log_err;
			}
		}
		close(fd);
		close(fdlock);
		unlink(PASSWDFILE);
		if (rename(PASSWDFILE ".tmp", PASSWDFILE))
			goto log_err;
		// return a useful value
		return foruser;
	}
	return pw;

log_err:
	if (do_write) {
		if (fd > -1)
			close(fd);
		if (fdlock > -1)
			close(fdlock);
	}
	do_log(L_ERR, "password changes %s failed while updating %s: %s", foruser, PASSWDFILE, strerror(errno));
	return 0;
}

/*--------------------------------------------------------------------------*/

char *read_password(char *foruser) {
	return change_password_file(foruser, 0, 0);
}

/*--------------------------------------------------------------------------*/

int update_password(char *foruser, char *password) {
	return (change_password_file(foruser, password, 1) ? 0 : -1);
}

/*--------------------------------------------------------------------------*/

int check_password(struct connection *cp)
{
	char *want;

	if (!*cp->pass_want) {
		// init
		want = read_password(cp->name);
	} else {
		want = cp->pass_want;
	}

	// no password found: pw = 0. if user is always authenticated: pw = "".

	// authentication on his socket needed?
	if (!want)
		return ((cp->needauth & 1) ? -2 : 1);

	// user found but has no password and it's ok.
	if (!*want)
		return 0;

	return verify_password(cp->pass_got, want);
}

/*--------------------------------------------------------------------------*/

char *compute_aprs_pass(char *call)
{
#define kKey 0x73e2 // This is the seed for the key
	static char pass[6];
	char rootCall[10]; // need to copy call to remove ssid from parse
	short hash;
	short i,len;
	char *ptr = rootCall;

	strncpy(rootCall, get_tidy_name(call), sizeof(rootCall)-1);
	rootCall[sizeof(rootCall)-1] = 0;
	if (!*rootCall)
		return 0;
	strupr(rootCall);

    	hash = kKey; // Initialize with the key value
    	i = 0;
    	len = (short)strlen(rootCall);

    	while (i<len) {// Loop through the string two bytes at a time
        	hash ^= (unsigned char)(*ptr++)<<8; // xor high byte with accumulated hash
        	hash ^= (*ptr++); // xor low byte with accumulated hash
        	i += 2;
    	}
	sprintf(pass, "%5.5d", (hash & 0x7fff)); // mask off the high bit so number is always positive

	return pass;
}

/*--------------------------------------------------------------------------*/

void ask_pw_plain(struct connection *cp, char *pw)
{

	char *pass;

	*cp->pass_want = 0;

	pass = (pw ? pw : read_password(cp->name));
	if (!pass || !*pass)
		return;

	if (!cp->ircmode)
		append_general_notice(cp, "Enter your password (or type /auth [sys|md5|plain]):\n");
	strncpy(cp->pass_want, pass, sizeof(cp->pass_want));
	cp->pass_want[sizeof(cp->pass_want)-1] = 0;

	return;
}

/*--------------------------------------------------------------------------*/

char *get_my_prompt_name(void)
{
	static char myname[16];
	char *p, *q;

	if (*myname)
		return myname;

	for (p = myhostname, q = myname; *p && q - myname < sizeof(myname)-1; ) {
		if (!isalnum(*p & 0xff))
			break;
		*q++ = toupper((*p++) & 0xff);
	}
	*q = 0;
	if (!callvalid(myname))
		strcpy(myname, "C0NV");
	return myname;
}

/*--------------------------------------------------------------------------*/

void ask_pw_sys(struct connection *cp, char *pw)
{
	char buffer[2048];
	int five_digits[5];
	int pwlen;
	int i, j;
	char *pass;

	*cp->pass_want = 0;

	pass = (pw ? pw : read_password(cp->name));
	if (!pass || !*pass)
		return;

	pwlen = strlen(pass);

	if (seed == 1L)
		conv_randomize();

	for (i = 0; i < 5; i++) {
		int k;
again:
		j = conv_random(pwlen, 0);
		// store generated request-numbers
		five_digits[i] = j+1; // pos0 refers as 1
		// same number again?
                for (k = 0; k < i; k++) {
                        if (five_digits[k] == five_digits[i])
                                goto again;
                }
		// store expected string in cp->passwd
		cp->pass_want[i] = pass[j];
	}
	// and terminate the string
	cp->pass_want[i] = 0;

	sprintf(buffer, "%s>  %d %d %d %d %d\n", get_my_prompt_name(), five_digits[0], five_digits[1], five_digits[2], five_digits[3], five_digits[4]);
	append_general_notice(cp, "\n");
	append_general_notice(cp, buffer);
}

/*--------------------------------------------------------------------------*/

static void char_to_hex(char *c, char *h, int n)
{

	int  i;
	static char  *hextable = "0123456789abcdef";

	for (i = 0; i < n; i++) {
		*h++ = hextable[(*c>>4)&0xf];
		*h++ = hextable[*c++ &0xf];
	}
	*h = '\0';
}

/*--------------------------------------------------------------------------*/

char *generate_rand_pw(int len)
{
	static char pass[PASSSIZE+1];
	int i, j;

	pass[0] = 0;

	if (seed == 1L)
		conv_randomize();

	if (len < 1)
		return pass;

	if (len > PASSSIZE)
		len = PASSSIZE;

        for (i = 0; i < len; i++) {
                j = conv_random(10+26*2, 0);
                if (j < 10) {
                        pass[i] = j + '0';
                        continue;
                }
                j -= 10;
                if (j < 26) {
                        pass[i] = j + 'A';
			continue;
                }
                j -= 26;
                pass[i] = j + 'a';
        }
        pass[len] = 0;

        return pass;
}

/*--------------------------------------------------------------------------*/

void ask_pw_md5(struct connection *cp, char *pw)
{
#define	SALT_LEN	10
	char buffer[2048];
	char cipher[16];
	char key[256];
	char *pass;
	char *challenge;

	pass = (pw ? pw : read_password(cp->name));
	*cp->pass_want = 0;

	if (!pass || !*pass)
		return;

	if (seed == 1L)
		conv_randomize();

	
	// copy password
	strncpy(key, pass, sizeof(key));
	key[sizeof(key)-1] = 0;

	// compute random salt
	challenge = generate_rand_pw(SALT_LEN);
	
	// ask for proper response to this challenge:
	sprintf(buffer, "%s> [%s]\n", get_my_prompt_name(), challenge);
	append_general_notice(cp, "\n");
	append_general_notice(cp, buffer);

	// compute md5 challenge
	calc_md5_pw(challenge, key, cipher);
	// store expected answer
	char_to_hex(cipher, cp->pass_want, 16);
}

