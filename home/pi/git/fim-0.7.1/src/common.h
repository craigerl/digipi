/* $LastChangedDate: 2024-04-08 00:08:57 +0200 (Mon, 08 Apr 2024) $ */
/*
 common.h : Miscellaneous stuff header file

 (c) 2007-2024 Michele Martone

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#ifndef FIM_COMMON_H
#define FIM_COMMON_H
#include "fim_types.h"
#if HAVE_FLEXLEXER_H
#undef yyFlexLexer
#include <FlexLexer.h>
#endif /* HAVE_FLEXLEXER_H */

#define FIM_WANT_PIPE_IN_LEXER 0 /* note that we are still checking for HAVE_PIPE */
#if FIM_WANT_PIPE_IN_LEXER
extern int fim_pipedesc[2];
#else /* FIM_WANT_PIPE_IN_LEXER */
#include <deque>
#include <queue>
typedef std::deque<fim_char_t> fim_cmd_deque_t;
typedef std::queue<fim_char_t, fim_cmd_deque_t> fim_cmd_queue_t;
#endif /* FIM_WANT_PIPE_IN_LEXER */

namespace fim
{
	class CommandConsole;
	class string;
}

int fim_isspace(int c);
int fim_isquote(int c);

fim::string fim_dirname(const fim::string& arg);
fim::string fim_auto_quote(const fim::string& arg, int quoted=0);
fim::string fim_shell_arg_escape(const fim::string& arg, bool quoted=true);
fim::string fim_key_escape(const fim::string uk);
fim::string fim_man_to_text(const fim::string ms, bool keep_nl=false);
fim::string fim_text_to_man(const fim::string ts);
void fim_perror(const fim_char_t *s);
//void fim_tolowers(fim_char_t *s);
//void fim_touppers(fim_char_t *s);
size_t fim_strlen(const fim_char_t *str);
void trec(fim_char_t *str,const fim_char_t *f,const fim_char_t*t);
//void trhex(fim_char_t *str);
void chomp(fim_char_t *s);
#if FIM_WANT_OBSOLETE
void sanitize_string_from_nongraph(fim_char_t *s, int c=0);
#endif /* FIM_WANT_OBSOLETE */
void sanitize_string_from_nongraph_except_newline(fim_char_t *s, int c=0);

using namespace fim;

int int2msbf(int in);
#if FIM_WANT_OBSOLETE
int int2lsbf(int in);
#endif /* FIM_WANT_OBSOLETE */
fim::string slurp_file(fim::string filename);
fim_char_t* slurp_binary_fd(int fd,int *rs);
#if FIM_SHALL_BUFFER_STDIN
fim_byte_t* slurp_binary_FD(FILE* fd, size_t  *rs);
#endif /* FIM_SHALL_BUFFER_STDIN */
bool write_to_file(fim::string filename, fim::string lines, bool append = false);

fim_char_t * dupstr (const fim_char_t* s);
fim_char_t * dupnstr (float n, const fim_char_t c='\0');
fim_char_t * dupnstr (const fim_char_t c1, double n, const fim_char_t c2='\0');
fim_char_t * dupnstr (fim_int n);
fim_char_t * dupsqstr (const fim_char_t* s);
fim_int fim_rand(void);

bool regexp_match(const fim_char_t*s, const fim_char_t*r, int ignorecase=1, int ignorenewlines=0, int globexception=1);

int lines_count(const fim_char_t*s, int cols);
#if FIM_WANT_OBSOLETE
int strchr_count(const fim_char_t*s, int c);
int newlines_count(const fim_char_t*s);
#endif /* FIM_WANT_OBSOLETE */
const fim_char_t* next_row(const fim_char_t*s, int cols);
int fim_common_test(void);

double getmicroseconds(void);
double getmilliseconds(void);
const bool fim_getenv_is_nonempty(const fim_char_t * name);
const fim_char_t * fim_getenv(const fim_char_t * name);
FILE * fim_fread_tmpfile(FILE * fp);
double fim_atof(const fim_char_t *nptr);
ssize_t fim_getline(fim_char_t **lineptr, size_t *n, FILE *stream, int delim);

bool is_dir(const fim::string nf);
bool is_file(const fim::string nf);
bool is_file_nonempty(const fim::string nf);
FILE *fim_fopen(const char *path, const char *mode);
int fim_fclose(FILE*fp);
size_t fim_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fim_rewind(FILE *stream);
int fim_fseek(FILE *stream, long offset, int whence);
int fim_fgetc(FILE *stream);
int fim_snprintf_fim_int(char *r, fim_int n);
int fim_snprintf_XB(char *str, size_t size, size_t q);
fim_byte_t * fim_pm_alloc(unsigned int width, unsigned int height, bool want_calloc = false);
const fim_char_t * fim_basename_of(const fim_char_t * s);
fim::string fim_getcwd(void);
fim_int fim_util_atoi_km2(const fim_char_t *nptr);
fim_int fim_atoi(const char*s);
size_t fim_maxrss(void);
fim_bool_t fim_is_id(const char*s);
fim_bool_t fim_is_id_char(const char c);
std::string fim_get_int_as_hex(const int i);

/* exceptions */
typedef int FimException;
#define FIM_E_NO_IMAGE 1
#define FIM_E_NO_VIEWPORT 2
#define FIM_E_WINDOW_ERROR 3
#define FIM_E_TRAGIC -1	/* no hope */
#define FIM_E_NO_MEM 4	/* also a return code */
/* ... */

#define FIM_CHAR_BIT 8 /* FIXME */
#define FIM_IS_SIGNED(T)   (((T)0) > (((T)-1)))
#define FIM_MAX_UNSIGNED(T) ((T)-1)
#define FIM_HALF_MAX_SIGNED(T) ((T)1 << (sizeof(T)*FIM_CHAR_BIT-2))
#define FIM_MAX_SIGNED(T) (FIM_HALF_MAX_SIGNED(T) - 1 + FIM_HALF_MAX_SIGNED(T))
#define FIM_MAX_VALUE_FOR_TYPE(T) (FIM_IS_SIGNED(T)?FIM_MAX_SIGNED(T):FIM_MAX_UNSIGNED(T))
#define FIM_MAX_INT FIM_MAX_VALUE_FOR_TYPE(fim_int) 

#endif /* FIM_COMMON_H */
