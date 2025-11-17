/* $LastChangedDate: 2024-04-08 00:08:57 +0200 (Mon, 08 Apr 2024) $ */
/*
 common.cpp : Miscellaneous stuff.

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

// including fim.h poses incompatibilities with <fstream>
//#include "fim.h"

#include <iostream>
#include "fim.h"
#include "fim_string.h"
#include "common.h"
#include <string>
#include <fstream>
#include <sys/time.h>	/* gettimeofday */
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h> /* getrusage */
#endif /* HAVE_SYS_RESOURCE_H */

#ifdef HAVE_GETLINE
#include <stdio.h>	/* getline : _GNU_SOURCE  */
#endif /* HAVE_GETLINE */
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif /* HAVE_WCHAR_H */
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif /* HAVE_LIBGEN_H */

#define FIM_WANT_ZLIB 0
/* #include <zlib.h> */ /* useless as long as FIM_WANT_ZLIB is 0 */

/*
void fim_tolowers(fim_char_t *s)
{
	if(!s)
		return;
	for(;*s;++s)
		*s=tolower(*s);
}

void fim_touppers(fim_char_t *s)
{
	if(!s)
		return;
	for(;*s;++s)
		*s=toupper(*s);
}
*/

static fim::string fim_my_dirname(const fim::string& arg)
{
	size_t lds = arg.rfind(FIM_CNS_DIRSEP_CHAR);
	if (lds != std::string::npos)
	{
		while (lds > 0 && arg[lds-1] == FIM_CNS_DIRSEP_CHAR)
			--lds;
		fim::string dir(arg);
		dir.erase(lds);
		return dir;
	}
	return arg;
}

fim::string fim_dirname(const fim::string& arg)
{
#ifdef HAVE_LIBGEN_H
	fim_char_t buf[FIM_PATH_MAX];
	strncpy(buf,arg.c_str(),FIM_PATH_MAX-1);
	buf[FIM_PATH_MAX-1]='\0';
	return dirname(buf);
#else /* HAVE_LIBGEN_H */
	return fim_my_dirname(arg);
#endif /* HAVE_LIBGEN_H */
}

fim::string fim_auto_quote(const fim::string& arg, int quoted)
{
	char qc = 0;
	fim::string ear=arg;
	fim::string res=FIM_CNS_EMPTY_STRING;

	if (quoted == 0)
	{
		quoted = 2;
		if (arg.size() == 1)
			if ( arg[0] == '\"' )
				quoted = 1;
		// ...
	}
	if (quoted == 1)
		qc = '\'';
	if (quoted == 2)
		qc = '"';
	if (quoted == 3)
	{
		const char * as = arg.c_str();
		const auto sqi = strchr(as,'\'');
		const auto dqi = strchr(as,'"');
		
		if (!sqi || ( sqi && dqi && sqi< dqi) )
			qc = '\'';
		if (!dqi || ( sqi && dqi && sqi>=dqi) )
			qc = '"';
		res = qc + ear + qc;
		return res;
	}
	if(qc)
		res+=qc;
	if(qc == '\'')
		ear.substitute("'","\\'");
	if(qc == '\"')
		ear.substitute("\"","\\\"");
	res+=ear;
	if(qc)
		res+=qc;
	return res;
}

fim::string fim_shell_arg_escape(const fim::string& arg, bool quoted)
{
	fim::string ear=arg;
	fim::string res=FIM_CNS_EMPTY_STRING;

	if(quoted)
		res+="'";
	ear.substitute("'","\\'");
	res+=ear;
	if(quoted)
		res+="'";
	return res;
}

fim::string fim_key_escape(const fim::string uk)
{
	// escape for display
	std::ostringstream oss;
	oss << "[" << uk << "]";
	return oss.str();
}

fim::string fim_text_to_man(const fim::string ts)
{
	// this is *not* the reverse of fim_man_to_text
	auto ms {ts};
#if FIM_USE_CXX_REGEX
	ms.substitute("\\\\'", "\\e'", 0); // without this, literal \' would get translated as ´ (acute accent) 
#else /* FIM_USE_CXX_REGEX */
	ms.substitute("\\'", "\\e'", 0); // without this, literal \' would get translated as ´ (acute accent) 
#endif /* FIM_USE_CXX_REGEX */
	return ms;
}

fim::string fim_man_to_text(const fim::string ms, bool keep_nl)
{
	fim::string ts = ms;
#if FIM_USE_CXX_REGEX
	ts.substitute("(\\\\fB)(.*?)(\\\\fP)", "$2", 0);
	ts.substitute("(\n\\.B)(.*?)(\n)", "$2 ", 0);
	ts.substitute("(\\\\fR\\\\fI)(.*?)(\\\\fR)", "$2", 0);
	if(!keep_nl)
		ts.substitute("(\n)", " ", 0);
#else /* FIM_USE_CXX_REGEX */
	/* workaround */
	ts.substitute("\\\\fB", "", 0);
	ts.substitute("\\\\fP", "", 0);
	ts.substitute("\\\\fR", "", 0);
	ts.substitute("\\\\fI", "", 0);
	ts.substitute("\\.B", " ", 0);
	if(!keep_nl)
		ts.substitute("(\n)", " ");
#endif /* FIM_USE_CXX_REGEX */
	ts.substitute("\\\\:", "");
	return ts;
}

void fim_perror(const fim_char_t *s)
{
#if 1
	if(errno)
	{
		if(s)
			perror(s);
		errno=0; // shall reset the error status
	}
#endif
}

size_t fim_strlen(const fim_char_t *str)
{
	return strlen(str);
}

#if 0
void trhex(fim_char_t *str)
{
	/*	
	 * 	translates C-like hexcodes (e.g.: \xFF) to chars, in place.
	 * 	if \x is not followed by two hexadecimal values, it is ignored and silently copied.
	 * 
	 *
	 *	this function could be optimized.
	 *
	 * 	FIXME : UNUSED
	 */
	const fim_char_t *fp;//fast pointer
	fim_char_t *sp;//slow pointer
	fim_char_t hb[3];

	if(!str)
		goto ret;

	hb[2]=0;
	fp=sp=str;
	while(*fp)
	{
			if(
				    fp[0] =='\\'
				 && fp[1] && fp[1]=='x'
				 && fp[2] && isxdigit(toupper(fp[2])) 
				 && fp[3] && isxdigit(toupper(fp[3]))  )
			{
				unsigned int hc;
				hb[0]=toupper(fp[2]);
				hb[1]=toupper(fp[3]);
				hc=(fim_byte_t)strtol(hb,FIM_NULL,FIM_PRINTINUM_BUFSIZE);
				*sp=hc;
				fp+=3;
			}
			else
				*sp=*fp;
			++fp;
			++sp;
	}
	*sp=0;
ret:
	return;
}
#endif

void trec(fim_char_t *str,const fim_char_t *f,const fim_char_t*t)
{
	/*	this function translates escaped characters at index i in 
	 *	f into the characters at index i in t.
	 *
	 *	order is not important for the final effect.
	 * 
	 *	this function could be optimized.
	 *	20090520 hex translation in
	 *	20221204 not sure about whether hex translation is useful
	 */
	int tl;
	fim_char_t*_p=FIM_NULL;
	const fim_char_t *fp;
	const fim_char_t *tp;

	if(!str || !f || !t || strlen(f)-strlen(t))
		goto ret;

	tl = strlen(f);//table length
	_p=str;

	while(*_p && _p[1])//redundant ?
	{
		fp=f;
		tp=t;

#if 1
		if(
			    _p[0] =='\\'
			 && _p[1] && _p[1]=='x'
			 && _p[2] && isxdigit(toupper(_p[2])) 
			 && _p[3] && isxdigit(toupper(_p[3]))  )
		{
			unsigned int hc = (toupper(_p[2])-'0')*16 + (toupper(_p[3])-'0');
			*_p=hc;
			/*	
				\xFF
				^-_p^-_p+4
			*/
			fim_char_t *pp=_p+4;
			while(*pp){pp[-3]=*pp;++pp;}
			pp[-3]='\0';
		}
		else
#endif
		while(*fp)
		{
			//  if the following character is backslash-escaped and is in our from-list ..
			if( *_p == '\\' && *(_p+1) == *fp )
			{
				fim_char_t*pp;
				*_p = *tp;//translation	
				++_p;  //new focus
				pp=_p+1;
				while(*pp)
				{
					pp[-1]=*pp;++pp;
				}//!*pp means we are done :)
				pp[-1]='\0';
				//if(*_p=='\\')++_p;//we want a single pass
//				if(*_p)++_p;//we want a single pass // ! BUG
				fp=f+tl;// in this way  *(fp) == '\0' (single translation pass) as soon as we continue
				if(!*_p)
					goto ret;
				--_p;//note that the outermost loop will increment this anyway
				continue;//we jump straight to while(NUL)
			}
			++fp;++tp;
		}
		++_p;
	} 
ret:
	return;
}

#if FIM_SHALL_BUFFER_STDIN
	fim_byte_t* slurp_binary_FD(FILE* fd, size_t  *rs)
	{
			/*
			 * ripped off quickly from slurp_binary_fd
			 * FIXME : it is not throughly tested
			 * */
			fim_byte_t	*buf=FIM_NULL;
			int	inc=FIM_FILE_BUF_SIZE,rb=0,nrb=0;
			buf=(fim_byte_t*)fim_calloc(inc,1);
			if(!buf) 
				goto ret;
			while((nrb=fim_fread(buf+rb,1,inc,fd))>0)
			{
				fim_byte_t *tb;
				// if(nrb==inc) a full read. let's try again
				// else we assume this is the last read (could not be true, of course)
				tb=(fim_byte_t*)fim_realloc(buf,rb+=nrb);
				if(tb!=FIM_NULL)
					buf=tb;
				else
					{rb-=nrb;continue;}
			}
			if(rs)
			{
				*rs=rb;
			}
ret:
			return buf;
	}
#endif /* FIM_SHALL_BUFFER_STDIN */

	fim_char_t* slurp_binary_fd(int fd,int *rs)
	{
			/*
			 * slurps a string.
			 * puts length of non-NUL bytes in *rs.
			 * TODO: rename to slurp_string().
			 * */
			fim_char_t	*buf=FIM_NULL;
			int	inc=FIM_FILE_BUF_SIZE,rb=0,nrb=0;
			buf = fim_stralloc(inc);
			if(!buf)
			       	goto ret;
			while((nrb=read(fd,buf+rb,inc))>0)
			{
				fim_char_t *tb;
				// if(nrb==inc) a full read. let's try again
				// else we assume this is the last read (could not be true, of course)
				tb=(fim_char_t*)fim_realloc(buf,(rb+=nrb)+1);
				if(tb!=FIM_NULL)
					buf=tb;
				else
					{rb-=nrb;buf[rb]=FIM_SYM_CHAR_NUL;continue;}
			}
			if(rs)
				*rs=rb;
ret:
			return buf;
	}

	fim::string slurp_file(fim::string filename)
	{
		std::string file;
		std::ifstream fs;
		fs.open(filename.c_str(),std::ios::binary);
		if(fs.is_open())
		{
			std::string tmp;
			/* FIXME : this is not efficient */
			while(!fs.eof())
			{
				getline(fs,tmp);
				tmp+="\n" ;
				file+=tmp;
			}
			//	printf("%s\n",file.c_str());
		}
		fs.close();
		return fim::string(file.c_str());
	}

	bool write_to_file(fim::string filename, fim::string lines, bool append)
	{
		std::ofstream fs;

		fs.open(filename.c_str(), std::ios::out | ( append ? std::ios::app : std::ios::binary ));

		if(fs.rdstate()!=std::ios_base::goodbit)
		{
			cout << "Error opening file " << filename << " for writing !\n";
			goto err;
		}

		if(fs.is_open())
		{
			fs << lines.c_str();

			if(fs.rdstate()!=std::ios_base::goodbit)
			{
				cout << "Error writing to file " << filename << " !\n";
				goto err;
			}

			fs.close();

			if(fs.rdstate()!=std::ios_base::goodbit)
			{
				cout << "Error closing file " << filename << " !\n";
				goto err;
			}
		}
		else 
		{
			cout << "Error after opening file " << filename << " !\n";
			goto err;
		}
#if HAVE_SYNC
		sync();
#endif /* HAVE_SYNC */
		return true;
err:
		return false;
	}

/*
 * Turns newline characters in NULs.
 * Does stop on the first NUL encountered.
 */
void chomp(fim_char_t *s)
{
	for(;*s;++s)
		if(*s=='\n')
			*s='\0';
}

/*
 * cleans the input string terminating it when some non printable character is encountered
 * (except newline)
 * */
void sanitize_string_from_nongraph_except_newline(fim_char_t *s, int c)
{	
	const int n=c;
	if(s)
		while(*s && (c--||!n))
		{
			if(!isgraph(*s)&&*s!='\n')
				*s=' ',
				++s;
			else
			       	++s;
		}
	return;
}

/*
 * cleans the input string terminating it when some non printable character is encountered
 * */

#if FIM_WANT_OBSOLETE
void sanitize_string_from_nongraph(fim_char_t *s, int c)
{
	const int n=c;
	if(s)
		while(*s && (c--||!n))
		{
			if(!isgraph(*s)||*s=='\n')
				*s=' ',
				++s;
			else
			       	++s;
		}
	return;
}
#endif /* FIM_WANT_OBSOLETE */

/*
 *	Allocation of a small string for storing the 
 *	representation of a double.
 */
fim_char_t * dupnstr (float n, const fim_char_t c)
{
	//allocation of a single string
	fim_char_t *r = (fim_char_t*) fim_malloc (32);
	if(!r){/*assert(r);*/throw FIM_E_NO_MEM;}
	sprintf(r,"%f%c",n,c);
	return (r);
}

fim_char_t * dupnstr (const fim_char_t c1, double n, const fim_char_t c2)
{
	//allocation of a single string
	fim_char_t *r = (fim_char_t*) fim_malloc (32);
	if(!r){/*assert(r);*/throw FIM_E_NO_MEM;}
	sprintf(r,"%c%f%c",c1,n,c2);
	return (r);
}

/*
 *	Allocation of a small string for storing the *	representation of an integer.
 */
fim_char_t * dupnstr (fim_int n)
{
	//allocation of a single string
	fim_char_t *r = (fim_char_t*) fim_malloc (FIM_PRINTINUM_BUFSIZE);
	if(!r){/*assert(r);*/throw FIM_E_NO_MEM;}
	fim_snprintf_fim_int(r,n);
	return (r);
}

/*
 *	Allocation and duplication of a single string
 */
fim_char_t * dupstr (const fim_char_t* s)
{
	fim_char_t *r = (fim_char_t*) fim_malloc (strlen (s) + 1);
	if(!r){/*assert(r);*/throw FIM_E_NO_MEM;}
	strcpy (r, s);
	return (r);
}

/*
 *	Allocation and duplication of a single string, slash-quoted
 */
fim_char_t * dupsqstr (const fim_char_t* s)
{
	int l=0;
	fim_char_t *r = (fim_char_t*) fim_malloc ((l=strlen (s)) + 3);
	if(!r){/*assert(r);*/throw FIM_E_NO_MEM;}
	else
	{
		r[0]='/';
		strcpy (r+1  , s);
		strcat (r+1+l,"/");
	}
	return (r);
}

/*
 *	Allocation and duplication of a single string (not necessarily terminating)
 */
#ifdef HAVE_FGETLN
#if FIM_USE_CXX14
auto                dupstrn (const fim_char_t* s, size_t l) /* Return type inference could be useful in a few places; this is just an example reminder. */
#else /* FIM_USE_CXX14 */
static fim_char_t * dupstrn (const fim_char_t* s, size_t l)
#endif /* FIM_USE_CXX14 */
{
	fim_char_t *r = (fim_char_t*) fim_malloc (l + 1);
	if(!r){/*assert(r);*/throw FIM_E_NO_MEM;}
	strncpy(r,s,l);
	r[l]='\0';
	return (r);
}
#endif /* HAVE_FGETLN */

template<typename T>
static int pick_word(const fim_char_t *f, T*wp)
{
	int fd = open(f,O_RDONLY);

	if(fd==-1)
	       	goto ret;

	if(read(fd,wp,sizeof(T))==sizeof(T))
		wp=FIM_NULL; // success
	close(fd);
	if(wp==FIM_NULL)
		fd=0; // success
	else
		fd=-1;
ret:
	return fd;
}

fim_int fim_rand(void)
{
	/*
	 * Please don't use Fim random numbers for cryptographical purposes ;)
	 * Note that we use /dev/urandom because it will never block on reading.
	 * Reading from     /dev/random may block.
	 * */
	fim_int w;
	unsigned int u;

	if(pick_word(FIM_LINUX_RAND_FILE,&u)==0)
	       	w = (u%RAND_MAX);
	else
	{
		srand(clock());
		w = rand();
	}
	return w;
}

	bool regexp_match(const fim_char_t*s, const fim_char_t*r, int ignorecase, int ignorenewlines, int globexception)
	{
		/*
		 *	given a string s, and a Posix regular expression r, this
		 *	member function returns true if there is match. false otherwise.
		 */
		bool match=false;
		match=true;
#if FIM_USE_CXX_REGEX
		/*
		 * we allow for the default match, in case of null regexp
		 */
		if(!r || !*r || !strlen(r))
			goto ret;

		/* fixup code for a mysterious bug (TODO: check if still relevant) */
		if(globexception && *r=='*')
		{
			match = false;
			goto ret;
		}

		try {
			match = std::regex_search(s, std::regex(r, ignorecase ? ( std::regex_constants::extended | std::regex_constants::icase) : std::regex_constants::extended ));
		} catch (const std::exception &) { match = false; }
#else /* FIM_USE_CXX_REGEX */
#if HAVE_REGEX_H
		regex_t regex;		//should be static!!!
		FIM_CONSTEXPR int nmatch=1;	// we are satisfied with the first match, aren't we ?
		regmatch_t pmatch[nmatch];

		/*
		 * we allow for the default match, in case of null regexp
		 */
		if(!r || !*r || !strlen(r))
			goto ret;

		/* fixup code for a mysterious bug (TODO: check if still relevant) */
		if(globexception && *r=='*')
		{
			match = false;
			goto ret;
		}

		if(ignorenewlines)
		{
			/* TODO: unfinished, maybe obsolete */
			fim::string aux;
			aux=s;
		}

		//if(regcomp(&regex,"^ \\+$", 0 | REG_EXTENDED | REG_ICASE )!=0)
		if(regcomp(&regex,r, 0 | REG_EXTENDED | (ignorecase==0?0:REG_ICASE) )!=0)
		{
			/* error calling regcomp (invalid regexp?)! (should we warn the user ?) */
			//cout << "error calling regcomp (invalid regexp?)!" << "\n";
			return false;
		}
		else
		{
//			cout << "done calling regcomp!" << "\n";
		}
		//if(regexec(&regex,s+0,nmatch,pmatch,0)==0)
		if(regexec(&regex,s+0,nmatch,pmatch,0)!=REG_NOMATCH)
		{
//			cout << "'"<< s << "' matches with '" << r << "'\n";
/*			cout << "match : " << "\n";
			cout << "\"";
			for(int m=pmatch[0].rm_so;m<pmatch[0].rm_eo;++m)
				cout << s[0+m];
			cout << "\"\n";*/
			regfree(&regex);
			goto ret;
		}
		else
		{
			/*	no match	*/
		};
		regfree(&regex);
		match = false;
#endif /* HAVE_REGEX_H */
#endif /* FIM_USE_CXX_REGEX */
ret:
		return match;
	}

#if FIM_WANT_OBSOLETE
int strchr_count(const fim_char_t*s, int c)
{
	int n=0;
	if(!s)
		return 0;
	while((s=strchr(s,c))!=FIM_NULL && *s)
		++n,++s;
	return n;
}

int newlines_count(const fim_char_t*s)
{
	/*
	 * "" 0
	 * "aab" 0
	 * "aaaaba\nemk" 1
	 * "aaaaba\nemk\n" 2
	 * */
	int c=strchr_count(s,'\n');
	if(s[strlen(s)-1]=='\n')
		++c;
	return c;
}
#endif /* FIM_WANT_OBSOLETE */

const fim_char_t* next_row(const fim_char_t*s, int cols)
{
	/*
	 * returns a pointer to the first character *after*
	 * the newline or the last one of the string.
	 *
	 * for cols=3:
	 * next_row("123\n")  -> \0
	 * next_row("123\n4") ->  4
	 * next_row("12")     -> \0
	 * next_row("1234")   ->  4
	 * */
	const fim_char_t *b=s;
	int l=strlen(s);
	if(!s)
		return FIM_NULL;
	if((s=strchr(s,'\n'))!=FIM_NULL)
	{
		// we have a newline marking the end of line:
		// with newline-column merge (*s==\n and s+1 is after)
		if((s-b)<=cols)
		       	return s+1;
		// ... or without merge (b[cols]!=\n and belongs to the next line)
		else
		       	return b+=cols;
	}
	return b+(l>=cols?cols:l);// no newlines in this string; we return the cols'th character or the NUL
}

int lines_count(const fim_char_t*s, int cols)
{
	/* for cols=6
	 *
	 * "" 0
	 * "aab" 0
	 * "aaaaba\nemk" 1
	 * "aaaaba\nemk\n" 2
	 * "aaaabaa\nemk\n" 3
	 * */
	if(cols<=0)
		return -1;
#if FIM_WANT_OBSOLETE
	if(cols==0)
		return newlines_count(s);
#endif /* FIM_WANT_OBSOLETE */

	int n=0;
	const fim_char_t*b;
	if(!s)
		return 0;
	b=s;
	while((s=strchr(s,'\n'))!=FIM_NULL && *s)
	{
		/*
		 * we want a cols long sequence followed by \n
		 * to be counted as one line, just as cols chars alone.
		 *
		 * moreover, we want to be able to enter in this body
		 * even if *++s is NUL, just to perform this computation.
		 */
		n+=s>b?(s-1-b)/cols:0;	/* extra lines due to the excessive line width (if s==b we can't expect any wrapping, of course )	*/
		++n;	// the \n is counted as a new line
		b=++s;	// if now *s==NUL, strchr simply will fail
	}
	//printf("n:%d\n",n);
	s=b;//*b==NUL or *b points to the last substring non newline terminated
	n+=(strlen(s))/cols;	// we count the last substring chars (with no wrapping exceptions)
	return n;
}

int swap_bytes_in_int(int in)
{
	// to Most Significant Byte First
	// FIXME : this function should be optimized
	const int b=sizeof(int);
	int out=0;
	int i=-1;
	while(i++<b/2)
	{
		((fim_byte_t*)&out)[i]=((fim_byte_t*)&in)[b-i-1];
		((fim_byte_t*)&out)[b-i-1]=((fim_byte_t*)&in)[i];
	}
	return out;
}

int fim_common_test(void)
{	
	/*
	 * this function should test the correctness of the functions in this file.
	 * it should be used for debug purposes, for Fim maintainance.
	 * */
	int score = 0;
	score += ( 0==lines_count("" ,6));
	score += ( 0==lines_count("aab" ,6));
	score += ( 1==lines_count("aaaaba\nemk" ,6));
	score += ( 2==lines_count("aaaaba\nemk\n" ,6));
	score += ( 3==lines_count("aaaabaa\nemk\n" ,6));
	score += ( *next_row("123\n",3)=='\0');
	score += ( *next_row("123\n4",3)=='4');
	score += ( *next_row("12",3)=='\0');
	score += ( *next_row("1234",3)=='4');
	score += ( fim_my_dirname("media/file")  == "media" );
	score += ( fim_my_dirname("media//file") == "media" );
	score += ( fim_dirname("media/file")  == "media" );
	score += ( fim_dirname("media//file") == "media" );
	score += ( swap_bytes_in_int(0x01020304) == 0x04030201 );
	return score - 14;
}

#if FIM_WANT_OBSOLETE
int int2lsbf(int in)
{
	const int one=0x01;
	if( 0x01 & (*(fim_byte_t*)(&one)) )/*true on msbf (like ppc), false on lsbf (like x86)*/
		return swap_bytes_in_int(in);
	return in;
}
#endif /* FIM_WANT_OBSOLETE */

int int2msbf(int in)
{
	const int one=0x01;
	if( 0x01 & (*(fim_byte_t*)(&one)) )/*true on msbf (like ppc), false on lsbf (like x86)*/
		return in;
	return swap_bytes_in_int(in);
}

double getmicroseconds(void)
{
	double dt=0.0;
        struct timeval tv;
        /*err=*/gettimeofday(&tv, FIM_NULL);
	dt+=tv.tv_usec;
	dt+=tv.tv_sec *1000 * 1000;
	// note : we ignore err!
	return dt;
}

double getmilliseconds(void)
{
	/*
         * For internal usage: returns with milliseconds precision the current clock time.
         * NOTE : this function is NOT essential.
         */
	//int err;//t,pt in ms; d in us
	return getmicroseconds() / 1000;
}

#if 0
struct fim_bench_struct { void *data; };

typedef fim_err_t (*fim_bench_ft)(struct fim_bench_struct*);
static fim_err_t fim_bench_video(struct fim_bench_struct*)
{
	//cc.clear_rect(0, width()-1, 0,height()/2);
	return FIM_ERR_NO_ERROR;
}
#endif

const fim_char_t * fim_getenv(const fim_char_t * name)
{
	/*
	*  A getenv() wrapper function.
	*/
#ifdef HAVE_GETENV
	return getenv(name);
#else /* HAVE_GETENV */
	return FIM_NULL;
#endif /* HAVE_GETENV */
}

const bool fim_getenv_is_nonempty(const fim_char_t * name)
{
	const fim_char_t * ev = fim_getenv(name);
	return ev && (*ev != FIM_SYM_CHAR_NUL);
}

FILE * fim_fread_tmpfile(FILE * fp)
{
	/*
	*  We transfer a stream contents in a tmpfile(void)
	*/
	FILE *tfd=FIM_NULL;

	errno=0; // shield form previous errors'
	if( ( tfd=tmpfile() )!=FIM_NULL )
	{
		/* todo : read errno in case of error and print some report.. */
		const size_t buf_size=FIM_STREAM_BUFSIZE;
		fim_char_t buf[buf_size];
		size_t rc=0,wc=0;/* on some systems fwrite has attribute warn_unused_result */
		while( (rc=fim_fread(buf,1,buf_size,fp))>0 )
		{
			wc=fwrite(buf,1,rc,tfd);
			if(wc!=rc)
			{
				/* FIXME : this error condition should be handled, as this mechanism is very brittle */
				// std::cerr << "fwrite error writing to tmpfile()!\n";
			}
		}
		rewind(tfd);
		if(errno)
		{
			// FIXME: need to handle errors properly.
			std::cerr << "Error while reading temporary file: errno has now value "<<errno<<":"<<
#if HAVE_STRERROR
				strerror(errno)<<"\n";
#else
				/*sys_errlist[errno]<< */"\n"; /* sys_errlist is deprecated in favour of strerror */
#endif
		}
		/*
		 * Note that it would be much nicer to do this in another way,
		 * but it would require to rewrite much of the file loading stuff
		 * (which is quite fbi's untouched stuff right now)
		 * */
		return tfd;
	}
	else
	{
		// std::cerr << "tmpfile() error !\n";
	}
	return FIM_NULL;
}

double fim_atof(const fim_char_t *nptr)
{
	/* the original atof suffers from locale 'problems', like non dotted radix representations */
	/* although, atof can be used if one calls setlocale(LC_ALL,"C");  */
	double n=0.0;
	double d=1.0;
	bool sign=false;
	while( *nptr == '-' ){++nptr;sign=!sign;}
	if(!nptr)
		return n;
	while( isdigit(*nptr) )
	{
		n+=.1*((double)(*nptr-'0'));
		n*=10.0;
		++nptr;
	}
	if(*nptr!='.')
		return sign?-n:n;
	++nptr;
	while( isdigit(*nptr) )
	{
		d/=10.0;
		n+=d*((double)(*nptr-'0'));
		++nptr;
	}
	return sign?-n:n;
}

ssize_t fim_getline(fim_char_t **lineptr, size_t *n, FILE *stream, int delim)
{
#ifdef HAVE_GETDELIM
	if (!n)
		free(*lineptr), *lineptr = FIM_NULL;
	else
		return getdelim(lineptr,n,delim,stream);
	return EINVAL;
#else /* HAVE_GETDELIM */
	/* the man page depends on this */
#endif /* HAVE_GETDELIM */
#ifdef HAVE_GETLINE
	if (!n)
		free(*lineptr), *lineptr = FIM_NULL;
	else
		return getline(lineptr,n,stream);
	return EINVAL;
#endif /* HAVE_GETLINE */
#ifdef HAVE_FGETLN
	if (!n)
		fim_free(*lineptr), *lineptr = FIM_NULL;
	else
	{	
		/* for BSD (in cstdlib) */
		fim_char_t *s,*ns;
		size_t len=0;
		s=fgetln(stream,&len);
		if(!s)
			return EINVAL;
		*lineptr=dupstrn(s,len);
		*n=len;
		return len;
	}
#endif /* HAVE_FGETLN */
	return EINVAL;
}

	bool is_dir(const fim::string nf)
	{
#if HAVE_SYS_STAT_H
		struct stat stat_s;
		/*	if the directory doesn't exist, return */
		if(-1==stat(nf.c_str(),&stat_s))
			return false;
		if( ! S_ISDIR(stat_s.st_mode))
			return false;
		return true;
#else /* HAVE_SYS_STAT_H */
		return false;
#endif /* HAVE_SYS_STAT_H */
	}

	bool is_file(const fim::string nf)
	{
		/* FIXME */
#if 0
		return !is_dir(nf);
#else
#if HAVE_SYS_STAT_H
		struct stat stat_s;
		/*	if the file (it can be a device, but not a directory) doesn't exist, return */
		if(-1==stat(nf.c_str(),&stat_s))
			return false;
		if( S_ISDIR(stat_s.st_mode))
			return false;
		/*if(!S_IFREG(stat_s.st_mode))return false;*/
#endif /* HAVE_SYS_STAT_H */
		return true;
#endif
	}

	bool is_file_nonempty(const fim::string nf)
	{
		/* FIXME: merge the stat-using functions into one, with arguments! */
#if 0
		return !is_dir(nf);
#else
#if HAVE_SYS_STAT_H
		struct stat stat_s;
		/*	if the file (it can be a device, but not a directory) doesn't exist, return */
		if(-1==stat(nf.c_str(),&stat_s))
			return false;
		if( S_ISDIR(stat_s.st_mode))
			return false;
		/*if(!S_IFREG(stat_s.st_mode))return false;*/
		if( stat_s.st_size == 0 )
			return false;
#endif /* HAVE_SYS_STAT_H */
		return true;
#endif
	}

int fim_isspace(int c){return isspace(c);}
int fim_isquote(int c){return c=='\'' || c=='\"';}

FILE *fim_fopen(const char *path, const char *mode)
{
#if FIM_WANT_ZLIB
	/* cast necessary; in v.1.2.3.4 declared as void* */
	return (FILE*)gzopen(path,mode);
#else /* FIM_WANT_ZLIB */
	return fopen(path,mode);
#endif /* FIM_WANT_ZLIB */
}

int fim_fclose(FILE*fp)
{
#if FIM_WANT_ZLIB
	return gzclose(fp);
#else /* FIM_WANT_ZLIB */
	return fclose(fp);
#endif /* FIM_WANT_ZLIB */
}

size_t fim_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
#if FIM_WANT_ZLIB
	return gzread(stream,ptr,size*nmemb);
#else /* FIM_WANT_ZLIB */
	return fread(ptr,size,nmemb,stream);
#endif /* FIM_WANT_ZLIB */
}

int fim_rewind(FILE *stream)
{
#if FIM_WANT_ZLIB
	gzrewind(stream);
	return 0;
#else /* FIM_WANT_ZLIB */
	rewind(stream);
	return 0;
#endif /* FIM_WANT_ZLIB */
}

int fim_fseek(FILE *stream, long offset, int whence)
{
#if FIM_WANT_ZLIB
	return gzseek(stream,offset,whence);
	0;
#else /* FIM_WANT_ZLIB */
	return fseek(stream,offset,whence);
#endif /* FIM_WANT_ZLIB */
}

int fim_fgetc(FILE *stream)
{
#if FIM_WANT_ZLIB
	return gzgetc(stream);
#else /* FIM_WANT_ZLIB */
	return fgetc(stream);
#endif /* FIM_WANT_ZLIB */
}

int fim_snprintf_fim_int(char *r, fim_int n)
{
	// this does not pass g++ -pedantic-errors
	if(FIM_LARGE_FIM_INT)
#if FIM_USE_OLDCXX
		return sprintf(r,"%ld",(long int)n);
#else
		return sprintf(r,"%lld",(long long int)n);
#endif
		//return sprintf(r,"%jd",(intmax_t)n);
		//return sprintf(r,"%zd",(signed size_t)n);
		//return sprintf(r,"%lld",(long long int)n);
	else
		return sprintf(r,"%d",(int)n);
}

int fim_snprintf_XB(char *str, size_t size, size_t q)
{
	/* result fits in 5 bytes */
	char u='B',b=' ';
	size_t d=1;
	int src;
	if(q/d>1024)
		d*=FIM_CNS_K,u='K',b='B';
	if(q/d>1024)
		d*=FIM_CNS_K,u='M';
	if(q/d>1024)
		d*=FIM_CNS_K,u='G';
#if (SIZEOF_SIZE_T > 4)
	if(q/d>1024)
		d*=FIM_CNS_K,u='T';
	if(q/d>1024)
		d*=FIM_CNS_K,u='P';
#endif
	if(q/d<10)
		src = snprintf(str, size, "%1.1f%c%c",((float)q)/((float)d),u,b);
	else
#if FIM_USE_OLDCXX
		src = snprintf(str, size, "%d%c%c",(int)(q/d),u,b);
#else
		src = snprintf(str, size, "%zd%c%c",q/d,u,b);
#endif
	return src;
}

fim_byte_t * fim_pm_alloc(unsigned int width, unsigned int height, bool want_calloc)
{
	size_t nmemb=1, size=1;
	nmemb *= width;
	nmemb *= height;
	nmemb *= 3;
	/* FIXME: shall implement overflow checks here */
	if(want_calloc)
		return (fim_byte_t*)fim_calloc(nmemb, size);
	else
		return (fim_byte_t*)fim_malloc(nmemb);
}

const fim_char_t * fim_basename_of(const fim_char_t * s)
{
	if(s && *s)
	{
#if 0
		size_t sl = strlen(s);

		while(sl > 0 )
		{
			sl--;
			if(s[sl]=='/')
				return s+sl+1;
		}
#else
		const fim_char_t * const bn = strrchr(s,'/');
		if(bn)
			s=bn+1;
#endif
	}
	return s;
}

fim::string fim_getcwd(void)
{
		fim::string cwd;
#if HAVE_GET_CURRENT_DIR_NAME
		/* default */
		if( fim_char_t *p = get_current_dir_name() )
		{
			cwd=p;
			free(p);
		}
#else /* HAVE_GET_CURRENT_DIR_NAME */
#if _BSD_SOURCE || _XOPEN_SOURCE >= 500
		{
			/* untested */
			fim_char_t *buf[PATH_MAX];
			getcwd(buf,PATH_MAX-1): 
			buf[PATH_MAX-1]=FIM_SYM_CHAR_NUL;
			cwd=buf;
		}
#endif /* _BSD_SOURCE || _XOPEN_SOURCE >= 500 */
#endif /* HAVE_GET_CURRENT_DIR_NAME */
		return cwd;
}

static fim_int fim_util_atoi_kmX(const fim_char_t *nptr, int base)
{
	// from rsb__util_atoi_kmX
	fim_int v = fim_atoi(nptr);

	if(*nptr=='-')
		++nptr;
	while(isdigit(*nptr))
		++nptr;
	if(*nptr && tolower(*nptr)=='g')
		v *= base * base * base;
	if(*nptr && tolower(*nptr)=='m')
		v *= base * base;
	if(*nptr && tolower(*nptr)=='k')
		v *= base;

	return v;
}

fim_int fim_util_atoi_km2(const fim_char_t *nptr)
{
	// from rsb__util_atoi_km2
	return fim_util_atoi_kmX(nptr, 1024);
}

fim_int fim_atoi(const char*s)
{
	if(FIM_LARGE_FIM_INT)
		return atoll(s);
	else
		return atoi(s);
}

size_t fim_maxrss(void)
{
#if HAVE_SYS_RESOURCE_H
	struct rusage usage;
	/* int gru = */getrusage(RUSAGE_SELF,&usage);// TODO: shall report in case of error.
	// printf("ru_maxrss: %ld (maximum resident set size -- MB)\n",usage.ru_maxrss / 1024);
	// return quantity in B
	return usage.ru_maxrss * 1024;
#else /* HAVE_SYS_RESOURCE_H */
	return 0;
#endif /* HAVE_SYS_RESOURCE_H */
}

fim_bool_t fim_is_id(const char*s)
{
	/* is fim variable identifier ? */
	if(s)
	{
		if(*s != '_' && ! isalpha(*s) )
			goto no;
		for(; *s == '_' || isalnum(*s); ++s )
			;
		if(!*s)
			return true;
	}
no:
	return false;
}

fim_bool_t fim_is_id_char(const char c)
{
	// valid only within desc file ids
	return isalpha(c) || c == '_';
}

std::string fim_get_int_as_hex(const int i)
{
	fim_char_t buf[2*FIM_CHARS_FOR_INT];
	snprintf(buf,FIM_CHARS_FOR_INT-1,"0x%x", i);
	buf[FIM_CHARS_FOR_INT-1]='\0';
	return buf;
}

