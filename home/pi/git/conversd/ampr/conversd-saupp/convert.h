/* convert() - converts machine dependend character standards */
/* This is convert.h */

#ifndef CONVERT_H
#define CONVERT_H

#define	BIN		255

#define	ISO		0
#define	ISO_STRIPED	1
#define	CP1252		2
#define dumb		3
#define TeX		4
#define IBM7		5
#define ROMAN8		6
#define IBMPC		7
#define ATARI		8
#define HTML		9
#define MAC		10		/* This one by HB9JNX (sailer@ips.id.ethz.ch, hb9jnx@hb9w.che.eu) */
#define POSTSCRIPT	11		/* This one by HB9JNX (sailer@ips.id.ethz.ch, hb9jnx@hb9w.che.eu) */

#define  CHARSETS 12		/* # of the lines above  defined charsets */
#define  CHARS    96		/* 96 ISO char's are defined */

struct charsets {
	char *name;
	int ind;
};

void charset_init(void);
int get_charset_by_name(char *buf);
char *get_charset_by_ind(int ind);
char *list_charsets(void);
char *convert(int in, int out, char *string);

#endif

