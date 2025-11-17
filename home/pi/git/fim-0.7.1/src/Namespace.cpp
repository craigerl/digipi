/* $LastChangedDate: 2024-03-18 15:17:10 +0100 (Mon, 18 Mar 2024) $ */
/*
 Namespace.cpp : a class for local variables storage

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

#include "fim.h"

#ifndef FIM_INDEPENDENT_NAMESPACE
#define FIM_NS_SV(VN,VL) if(rnsp_) rnsp_->setVariable(VN,VL); /* FIXME: need a better solution here ! */
#else
#define FIM_NS_SV(VN,VL)
#endif /* FIM_INDEPENDENT_NAMESPACE */

namespace fim
{
		fim_int Namespace::setVariable(const fim_var_id& varname,fim_int value)
		{
			variables_[varname].setInt(value);
			return value;
		}

#if FIM_WANT_LONG_INT
		int Namespace::setVariable(const fim_var_id& varname,int value)
		{
			setVariable(varname,static_cast<fim_int>(value));
			return value;
		}

		unsigned int Namespace::setVariable(const fim_var_id& varname,unsigned int value)
		{
			setVariable(varname,static_cast<fim_int>(value));
			return value;
		}
#endif /* FIM_WANT_LONG_INT */

		fim_float_t Namespace::setVariable(const fim_var_id& varname,fim_float_t value)
		{
			variables_[varname].setFloat(value);
			return value;
		}

		const Var& Namespace::setVariable(const fim_var_id& varname,const Var&value)
		{
			variables_[varname].set(value);
			return value;
		}

		const fim::string& Namespace::setVariable(const fim_var_id& varname,const fim::string&value)
		{
			variables_[varname].set(value);
			return value;
		}

		const fim_char_t* Namespace::setVariable(const fim_var_id& varname,const fim_char_t*value)
		{
			variables_[varname].setString(string(value));
			return value;
		}
	
		fim_bool_t Namespace::isSetVar(const fim_var_id& varname)const
		{
			fim_bool_t isv = ( variables_.find(varname) != variables_.end() );
			assert(! (varname.size()>2 && varname[1] == FIM_SYM_NAMESPACE_SEP ) );
			return isv;
		}

		void Namespace::unsetVariable(const fim_var_id& varname)
		{
			variables_t ::iterator vi=variables_.find(varname);
			if( vi!= variables_.end() )
				variables_.erase(varname);
		}

		fim_int Namespace::getIntVariable(const fim_var_id& varname)const
		{
			variables_t::const_iterator vi=variables_.find(varname);
			fim_int retval = FIM_CNS_EMPTY_INT_VAL;

			if(vi!=variables_.end())
				retval = vi->second.getInt();
			return retval;
		}

		const Var Namespace::getVariable(const fim_var_id& varname)const
		{
			if(varname == "*")
			{
				return Var(get_variables_list(true));
			}
			else
			{
				variables_t::const_iterator vi=variables_.find(varname);

				if(vi!=variables_.end())
				{
					return vi->second;
				}
				else
				{
			       		return Var();
				}
			}
		}

		fim_float_t Namespace::getFloatVariable(const fim_var_id& varname)const
		{
			variables_t::const_iterator vi=variables_.find(varname);
			fim_float_t retval = FIM_CNS_EMPTY_FP_VAL;

			if(vi!=variables_.end())
			       	retval = vi->second.getString();
			return retval;
		}

		fim::string Namespace::getStringVariable(const fim_var_id& varname)const
		{
			fim::string retval = FIM_CNS_EMPTY_RESULT;
			variables_t::const_iterator vi=variables_.find(varname);

			if(vi!=variables_.end())
				retval = vi->second.getString();
			return retval;
		}

	        fim_float_t Namespace::setGlobalVariable(const fim_var_id& varname,fim_float_t value)
		{
			FIM_NS_SV(varname,value)
			return value;
		}

		fim_int Namespace::setGlobalVariable(const fim_var_id& varname,fim_int value)
		{
			FIM_NS_SV(varname,value)
			return value;
		}

#if FIM_WANT_LONG_INT
		int Namespace::setGlobalVariable(const fim_var_id& varname,int value)
		{
			return setGlobalVariable(varname,static_cast<fim_int>(value));
		}

		unsigned int Namespace::setGlobalVariable(const fim_var_id& varname,unsigned int value)
		{
			return setGlobalVariable(varname,static_cast<fim_int>(value));
		}
#endif /* FIM_WANT_LONG_INT */

		const fim_char_t* Namespace::setGlobalVariable(const fim_var_id& varname,const fim_char_t*value)
		{
			FIM_NS_SV(varname,value)
			return value;
		}

		const fim::string& Namespace::setGlobalVariable(const fim_var_id& varname, const fim::string& value)
		{
			FIM_NS_SV(varname,value)
			return value;
		}

		fim_bool_t Namespace::isSetGlobalVar(const fim_var_id& varname)const
		{
			fim_bool_t isv = false;
#ifndef FIM_INDEPENDENT_NAMESPACE
			if(rnsp_)
				isv = ( rnsp_->variables_.find(varname) != rnsp_->variables_.end() );
#endif /* FIM_INDEPENDENT_NAMESPACE */
			return isv;
		}

		fim_int Namespace::getGlobalIntVariable(const fim_var_id& varname)const
		{
#ifndef FIM_INDEPENDENT_NAMESPACE
			if(rnsp_)
				return rnsp_->getIntVariable(varname);
#endif /* FIM_INDEPENDENT_NAMESPACE */
			return FIM_CNS_EMPTY_INT_VAL;
		}

		fim_float_t Namespace::getGlobalFloatVariable(const fim_var_id& varname)const
		{
#ifndef FIM_INDEPENDENT_NAMESPACE
			if(rnsp_)
				return rnsp_->getFloatVariable(varname);
#endif /* FIM_INDEPENDENT_NAMESPACE */
			return FIM_CNS_EMPTY_FP_VAL;
		}

		fim::string Namespace::getGlobalStringVariable(const fim_var_id& varname)const
		{
#ifndef FIM_INDEPENDENT_NAMESPACE
			if(rnsp_)
				return rnsp_->getStringVariable(varname);
#endif /* FIM_INDEPENDENT_NAMESPACE */
			return FIM_CNS_EMPTY_RESULT;
		}

		const Var Namespace::getGlobalVariable(const fim_var_id& varname)const
		{
#ifndef FIM_INDEPENDENT_NAMESPACE
			if(rnsp_)
				return rnsp_->getVariable(varname);
#endif /* FIM_INDEPENDENT_NAMESPACE */
			return FIM_CNS_EMPTY_RESULT;
		}

		fim::string Namespace::autocmd_exec(const fim::string&event, const fim_fn_t& fname)
		{
#ifdef FIM_AUTOCMDS
#ifndef FIM_INDEPENDENT_NAMESPACE
			if(rnsp_)
				return rnsp_->autocmd_exec(event,fname);
#endif /* FIM_INDEPENDENT_NAMESPACE */
			return FIM_CNS_EMPTY_RESULT;
#else /* FIM_AUTOCMDS */
			return FIM_CNS_EMPTY_RESULT;
#endif /* FIM_AUTOCMDS */
		}

		fim::string Namespace::get_variables_list(bool with_values, bool fordesc)const
		{
			/* A list of var=val like lines. */
			std::ostringstream oss;
			variables_t::const_iterator vi;

			for( vi=variables_.begin();vi!=variables_.end();++vi)
			{
				if(fordesc)
					oss << "#!fim:";
				if(ns_char_!=FIM_SYM_NULL_NAMESPACE_CHAR)
				{
					oss << ns_char_ << FIM_SYM_NAMESPACE_SEP;
				}
				oss << ((*vi).first);

				if(fordesc)
				{
					oss << "=";
					if(with_values)
						oss << ((*vi).second.getString());
			       		oss << "\n"; // newline always
				}
				else
				{
					oss << " ";
					if(with_values)
						oss << " = " << ((*vi).second.getString()) << "\n"; // newline only on with_values
				}
			}
			return oss.str();
		}

		fim_err_t Namespace::find_matching_list(fim_cmd_id cmd, args_t& completions, bool prepend_ns)const
		{
			for(variables_t::const_iterator vi=variables_.begin();vi!=variables_.end();++vi)
			{
				if((vi->first).find(cmd)==0)
				{
					fim::string res;
					if(prepend_ns)
						res+=ns_char_,res+=FIM_SYM_NAMESPACE_SEP;
					res+=(*vi).first;
					completions.push_back(res);
				}
			}
			return FIM_ERR_NO_ERROR;
		}
		
		fim_err_t Namespace::assign_ns(const Namespace& ns)
		{
			for(variables_t::const_iterator fit=ns.variables_.begin();fit!=ns.variables_.end();++fit)
				setVariable((fit->first),Var(fit->second));
			return FIM_ERR_NO_ERROR;
		}

	std::ostream& Namespace::print(std::ostream&os)const
	{
		return os << this->get_variables_list(true);
	}

	std::ostream& operator<<(std::ostream &os, const Namespace& ns)
	{
		return ns.print(os);
	}
}

