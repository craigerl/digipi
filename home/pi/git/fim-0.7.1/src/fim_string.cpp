/* $LastChangedDate: 2023-10-12 23:28:06 +0200 (Thu, 12 Oct 2023) $ */
/*
 string.h : Fim's string type

 (c) 2007-2023 Michele Martone

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
#include "fim.h"
#include "fim_string.h"
#include <string>

#define FIM_WANT_DEBUG_REGEXP 0

namespace fim
{
	std::ostream& operator<<(std::ostream& os,const string& s)
	{
		return os << s.c_str();
	}

	std::ostream& operator<<(std::ostream& os, const Browser& b)
	{
		return os;
	}

	string::string(const string& rhs):std::string(rhs)
	{
	}

	string::string(fim_char_t c)
	{
		fim_char_t buf[2];
		buf[0]=c;
		buf[1]='\0';
		append(buf);
	}

#if FIM_WANT_LONG_INT
	string::string(int i):string(std::to_string(i)) { }
#endif /* FIM_WANT_LONG_INT */

	string::string(fim_int i):string(std::to_string(i)) { }
	string::string(size_t i):string(std::to_string(i)) { }
	string::string(float i):string(std::to_string(i)) { }

	string::string(int * i)
	{
		fim_char_t buf[FIM_CHARS_FOR_INT];
		snprintf(buf,FIM_CHARS_FOR_INT-1,"%p",(void*)i);
		buf[FIM_CHARS_FOR_INT-1]='\0';
		append(buf);
	}

	string::string(void):std::string(){}

	string string::operator+(const char * s)const
	{
		string res(*this);
		if(s)
			res+=s;
		return string(res);
	}

	string string::operator+(const string s)const
	{
		return (*this)+s.c_str();
	}

	bool string::re_match(const fim_char_t*r)const
	{
		if( !r || !*r )
			return false;

		return ::regexp_match(c_str(), r, true, true /* obsolete second arg */, false);
	}

#if FIM_WANT_LONG_INT
 	string::operator int  (void)const{return atoi(this->c_str());}
#endif /* FIM_WANT_LONG_INT */
 	string::operator fim_int  (void)const{return fim_atoi(this->c_str());}
	string::operator float(void)const{return fim_atof(this->c_str());}

#if 0
	int string::find_re(const fim_char_t*r, int *mbuf)const
	{
		/* TODO: replace using <regex> */
		/*
		 * each occurrence of regular expression r will be substituted with t
		 *
		 * FIXME : return values could be more informative
		 * NOTE: mbuf is int, but pmatch->rm_so and pmatch->rm_eo are regoff_t from regex.h
		 * */
#if HAVE_REGEX_H
		regex_t regex;
		const size_t nmatch=1;
		regmatch_t pmatch[nmatch];

		if( !r || !*r )
			return -1;

		if( regcomp(&regex,r, 0 | REG_EXTENDED | REG_ICASE ) != 0 )
			return -1;

		if(regexec(&regex,c_str(),nmatch,pmatch,0)==0)
		{
			regfree(&regex);
			if(mbuf)
			{
				mbuf[0]=pmatch->rm_so;
				mbuf[1]=pmatch->rm_eo;
			}
			return pmatch->rm_so;
		}
		regfree(&regex);
#endif /* HAVE_REGEX_H */
		return -1;
	}
#endif

	void string::substitute(const fim_char_t*r, const fim_char_t* s, int flags)
	{
		/* each occurrence of regular expression r will be substituted with s */
		if( !r || !*r || !s )
			return;

#if FIM_USE_CXX_REGEX
		std::string os;
		try {
			std::regex_constants::syntax_option_type mf{};
			if (flags & FIM_REG_EXTENDED)
				mf |= std::regex_constants::extended;
			if (flags & FIM_REG_ICASE)
				mf |= std::regex_constants::icase;
			//mf |= std::regex_constants::format_sed;
			os = std::regex_replace( *this, std::regex(r, mf), s);
		} catch (const std::exception &) { }
		if(os!=*this)
			*this=os.c_str();
#else /* FIM_USE_CXX_REGEX */
#if HAVE_REGEX_H
		regex_t regex;
		FIM_CONSTEXPR int nmatch=1;
		regmatch_t pmatch[nmatch];
		int off=0;//,sl=0;
		std::string rs =FIM_CNS_EMPTY_STRING;
		int ts=this->size();
		int mf = 0;
		if (flags & FIM_REG_EXTENDED)
			mf |= REG_EXTENDED;
		if (flags & FIM_REG_ICASE)
			mf |= REG_ICASE;
		if( regcomp(&regex,r, 0 | mf ) != 0 )
		{
			std::cerr << "Warning: " << r << "  made regcomp fail!\n";
			return;
		}

		//sl=strlen(s);
		//const int s_len=strlen(s);
		while(regexec(&regex,off+c_str(),nmatch,pmatch,REG_NOTBOL)==0)
		{
			/*
			 * please note that this is not an efficient substitution implementation.
			 * */
			if(FIM_WANT_DEBUG_REGEXP)std::cerr << "rm_so :"<<pmatch->rm_so<< " rm_eo:"<<pmatch->rm_eo<<"\n";
			if(FIM_WANT_DEBUG_REGEXP)std::cerr << "pasting "<<off<< ":"<<off+pmatch->rm_so<<"\n";
			if(pmatch->rm_so>0)
			rs+=substr(off,pmatch->rm_so);
			if(FIM_WANT_DEBUG_REGEXP)std::cerr << "forward to "<<rs<<"\n";
			rs+=s;
			//rs+=substr(pmatch->rm_eo,ts);
			if(FIM_WANT_DEBUG_REGEXP)std::cerr << "match at "<<c_str() << " from " << off+pmatch->rm_so << " to " << off+pmatch->rm_eo<< ", off="<<off<<"\n";
			off+=pmatch->rm_eo;
			if(FIM_WANT_DEBUG_REGEXP)std::cerr << "forward to "<<rs<<"\n";
		}
		if(FIM_WANT_DEBUG_REGEXP)std::cerr << "forward no more\n";
		if(ts>off)
			rs+=substr(off,ts);
		if(FIM_WANT_DEBUG_REGEXP)std::cerr << "res is "<<rs<<", off= "<<off<<"\n";
		if(rs!=*this)
			*this=rs.c_str();
		regfree(&regex);
#endif /* HAVE_REGEX_H */
#endif /* FIM_USE_CXX_REGEX */
		return;
	}

#if FIM_WANT_OBSOLETE
	size_t string::lines(void)const
	{
		/*
		 * each empty line will be counted unless it is the last and not only.
		 * */
		const fim_char_t*s=c_str(),*f=s;
		size_t c=0;
		if(!s)return 0;
		while((s=strchr(s,'\n'))!=FIM_NULL){++c;f=++s;}
		return c+(strlen(f)>0);
	}

	fim::string string::line(int ln)const
	{
		/*
		 * returns the ln'th line of the string, if found, or ""
		 * */
		const fim_char_t*s,*f;
		s=this->c_str();
		f=s;
		if(ln< 0 || !s)return "";
		while( ln && ((f=strchr(s,'\n'))!=FIM_NULL ) )
		{
			--ln;
			s=f+1;
		}
		if(!ln)
		{
			const fim_char_t *se=s;
			if(!*s)se=s+strlen(s);
			else se=strchr(s,'\n');
			fim::string rs;
			rs=substr(s-this->c_str(),se-s).c_str();
			return rs;
		}
		return "";
	}
#endif /* FIM_WANT_OBSOLETE */
}

