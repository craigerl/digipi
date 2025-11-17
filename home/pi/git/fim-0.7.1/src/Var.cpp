/* $LastChangedDate: 2022-11-15 09:29:35 +0100 (Tue, 15 Nov 2022) $ */
/*
 Var.cpp : 

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
#include "fim.h"
namespace fim
{
	typedef std::map<fim::string, fim::string> fim_var_help_t;//variable id -> variable help
	static fim_var_help_t fim_var_help_db;	/* this is the global help db for fim variables */

	void fim_var_help_db_init(void)
	{
		#define FIM_WANT_INLINE_HELP 1
		#include "help.cpp"
		#undef  FIM_WANT_INLINE_HELP
		;/* freebsd 7.2 cc dies without */
	}

	fim::string fim_var_help_db_query(const fim::string& id)
	{
		fim_var_help_t::const_iterator hi = fim_var_help_db.find(id);
		fim::string hs;

		if(hi != fim_var_help_db.end())
			hs = hi->second;
		return hs;
	}

	fim::string fim_get_variables_reference(FimDocRefMode refmode)
	{
		std::ostringstream oss;
		fim_var_help_t::const_iterator vi;
		if(refmode==Man)
			goto manmode;
		for( vi=fim_var_help_db.begin();vi!=fim_var_help_db.end();++vi)
			oss << vi->first << " : " << fim_var_help_db_query(vi->first) << "\n";
		return oss.str();
manmode:
		for( vi=fim_var_help_db.begin();vi!=fim_var_help_db.end();++vi)
			oss << ".na\n" <<  /* No output-line adjusting; unelegant way to avoid man --html=cat's: cannot adjust line */
				".B\n" << vi->first << "\n" << fim_var_help_db_query(vi->first) << "\n" << ".fi\n";
		fim::string s(oss.str());
	       	s.substitute("\\$","$\\:"); /* Zero-width break point on $ (that is, on long hardcoded regexps). */
		return s;
	}

	std::ostream& Var::print(std::ostream& os)const
	{
		return os << this->getString();
	}

	std::ostream& operator<<(std::ostream& os, const Var& var)
	{
		return var.print(os);
	}
}
