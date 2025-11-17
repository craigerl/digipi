
#ifndef CFGFILE_H
#define CFGFILE_H

#define CFGLINE_LEN	1024

struct cfgcmd {
	char	*name;
	int	(*function)	(void *dest, int argc, char **argv);
	void	*dest;
};

extern char *strupr(char *s);
extern char *strlwr(char *s);

extern int parse_args(char *argv[], char *cmd);
extern char *argstr(int arg, int argc, char **argv);

extern int read_cfgfile(char *f, struct cfgcmd *cmds);

extern int do_string(char **dest, int argc, char **argv);
extern int do_int(int *dest, int argc, char **argv);

#endif

