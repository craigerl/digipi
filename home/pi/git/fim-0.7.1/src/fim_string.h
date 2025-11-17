/* $LastChangedDate: 2022-11-14 09:47:33 +0100 (Mon, 14 Nov 2022) $ */
#ifndef FIM_STRING_H
#define FIM_STRING_H
/*
 string.h : Fim's string type

 (c) 2007-2022 Michele Martone

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

/*
 * the rest of the program waits still for a proper adaptation of dynamic strings.
 */
#define FIM_CHARS_FOR_INT 32 /* should fit a pointer address printout */

#include <string>
#include "fim_types.h"

#define FIM_REG_EXTENDED 0x1
#define FIM_REG_ICASE 0x2
namespace fim
{
	class string /*FIM_FINAL*/ :public std::string
	{
		public:
		string();
		~string(){}

		string(const std::string&rhs):std::string(rhs){}
		string(const fim_char_t*s):std::string(s?s:""){}

		string(fim_char_t c);
#if FIM_WANT_LONG_INT
		string(int i);
#endif /* FIM_WANT_LONG_INT */
		string(fim_int i);
		string(float i);
		string(int * i);
		string(size_t i);

 		operator fim_int  (void)const;
#if FIM_WANT_LONG_INT
 		operator int  (void)const;
#endif /* FIM_WANT_LONG_INT */
		operator float(void)const;
		operator const char*(void)const{return c_str();}
		operator bool (void)const{return c_str()!=NULL;}

		string operator+(const char * s)const;
		string operator+(const string rhs)const;
		fim_char_t operator[](int i)const {return this->std::string::operator[](i);} // to avoid ambiguity with operator[] of std::string and char*
		/* copy constructor */
		string(const string& rhs);
		bool re_match(const fim_char_t*r)const;
		void substitute(const fim_char_t*r, const fim_char_t* s, int flags=FIM_REG_EXTENDED | FIM_REG_ICASE);
#if FIM_WANT_OBSOLETE
		fim::string line(int ln)const;
		size_t lines(void)const;
#endif /* FIM_WANT_OBSOLETE */
//		int find_re(const fim_char_t*r,int *mbuf=FIM_NULL)const;
	}; /* fim::string */

	class Browser;
	class CommandConsole;
	std::ostream& operator<<(std::ostream& os,const string& s);
	std::ostream& operator<<(std::ostream& os, const Browser& b);
} /* namespace fim */
#endif /* FIM_STRING_H */
