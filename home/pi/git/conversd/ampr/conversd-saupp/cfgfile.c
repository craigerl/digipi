
/*
 *	cfgfile.c
 *
 *	Generic config file parsing routines
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "cfgfile.h"
#include "log.h"
#include "hmalloc.h"

/*
 *	String to upper case
 */

char *strupr(char *s)
{
	char *c = s;
	
	while (*c)
		*c = toupper(*c & 0xff), c++;
		
	return s;
}

/*
 *	String to lower case
 */

char *strlwr(char *s)
{
	char *c = s;
	
	while (*c)
		*c = tolower(*c & 0xff), c++;
		
	return s;
}

 /* ***************************************************************** */

int do_string(char **dest, int argc, char **argv)
{
	if (argc < 2)
		return -1;
	if (*dest)
		hfree(*dest);
	*dest = hstrdup(argv[1]);
	do_log(L_CONFIG, "%s: %s", argv[0], *dest);
	return 0;
}

int do_int(int *dest, int argc, char **argv)
{
	if (argc < 2)
		return -1;
	*dest = atoi(argv[1]);
	do_log(L_CONFIG, "%s: %d", argv[0], *dest);
	return 0;
}

 /* ***************************************************************** */

/*
 *	Parse c-style escapes (neat to have!)
 */
 
static char *parse_string(char *str)
{
	char *cp = str;
	unsigned long num;

	while (*str != '\0' && *str != '\"') {
		if (*str == '\\') {
			str++;
			switch (*str++) {
			case 'n':
				*cp++ = '\n';
				break;
			case 't':
				*cp++ = '\t';
				break;
			case 'v':
				*cp++ = '\v';
				break;
			case 'b':
				*cp++ = '\b';
				break;
			case 'r':
				*cp++ = '\r';
				break;
			case 'f':
				*cp++ = '\f';
				break;
			case 'a':
				*cp++ = '\007';
				break;
			case '\\':
				*cp++ = '\\';
				break;
			case '\"':
				*cp++ = '\"';
				break;
			case 'x':
				num = strtoul(str-1, (&str)-1, 16); str--;
				*cp++ = (char) num;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				num = strtoul(str-1, (&str)-1, 8); str--;
				*cp++ = (char) num;
				break;
			case '\0':
				return NULL;
			default:
				*cp++ = *(str - 1);
				break;
			};
		} else {
			*cp++ = *str++;
		}
	}
	if (*str == '\"')
		str++;		/* skip final quote */
	*cp = '\0';		/* terminate string */
	return str;
}

/*
 *	Parse command line to argv, honoring quotes and such
 */
 
int parse_args(char *argv[], char *cmd)
{
	int ct = 0;
	int quoted;
	
	while (ct < 255)
	{
		quoted = 0;
		while (*cmd && isspace(*cmd))
			cmd++;
		if (*cmd == 0)
			break;
		if (*cmd == '"') {
			quoted++;
			cmd++;
		}
		argv[ct++] = cmd;
		if (quoted) {
			if ((cmd = parse_string(cmd)) == NULL)
				return 0;
		} else {
			while (*cmd && !isspace(*cmd))
				cmd++;
		}
		if (*cmd)
			*cmd++ = 0;
	}
	argv[ct] = NULL;
	return ct;
}

/*
 *	Combine arguments back to a string
 */
 
char *argstr(int arg, int argc, char **argv)
{
	static char s[CFGLINE_LEN];
	int i;
	
	s[0] = '\0';
	
	for (i = arg; i < argc; i++) {
		strncat(s, argv[i], CFGLINE_LEN);
		strncat(s, " ", CFGLINE_LEN);
	}
	
	if ((i = strlen(s)) > 0)
		s[i-1] = '\0';
	
	return s;
}

/*
 *	Find the command from the command table and execute it.
 */
 
int cmdparse(struct cfgcmd *cmds, char *cmdline)
{
	struct cfgcmd *cmdp;
	int argc;
	char *argv[256];

	if ((argc = parse_args(argv, cmdline)) == 0 || *argv[0] == '#')
		return 0;
	strlwr(argv[0]);
	for (cmdp = cmds; cmdp->function != NULL; cmdp++)
		if (strncasecmp(cmdp->name, argv[0], strlen(argv[0])) == 0)
			break;
	if (cmdp->function == NULL)
		return -1;
	return (*cmdp->function)(cmdp->dest, argc, argv);
}

 /* ***************************************************************** */

/*
 *	Read configuration
 */
 
int read_cfgfile(char *f, struct cfgcmd *cmds)
{
	FILE *fp;
	char line[CFGLINE_LEN];
	int ret, n = 0;
	
	if ((fp = fopen(f, "r")) == NULL) {
		do_log(L_CRIT, "Cannot open %s: %s", f, strerror(errno));
		return -1;
	}
	
	while (fgets(line, CFGLINE_LEN, fp) != NULL) {
		n++;
		ret = cmdparse(cmds, line);
		if (ret < 0) {
			do_log(L_CRIT, "Problem in %s at line %d: %s", f, n, line);
			return -2;
		}
	}
	fclose(fp);
	
	return 0;
}

