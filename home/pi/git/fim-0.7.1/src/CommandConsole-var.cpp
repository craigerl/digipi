/* $LastChangedDate: 2024-03-18 15:17:10 +0100 (Mon, 18 Mar 2024) $ */
/*
 CommandConsole-var.h : CommandConsole variables store

 (c) 2013-2024 Michele Martone

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

	fim_var_id CommandConsole::rnid(const fim_var_id& varname)const
	{
		fim::string id;
#ifdef FIM_NAMESPACES
		if( varname.length() > 2 && varname.at(1) == FIM_SYM_NAMESPACE_SEP )
		{
			if( varname.length() > 2 )
				id = varname.c_str()+2;
		}
		else
#endif /* FIM_NAMESPACES */
			id = varname;
		return id;
	}

	Namespace * CommandConsole::rns(const fim_var_id varname)
	{
		Namespace * nsp = FIM_NULL;
#ifdef FIM_NAMESPACES
		if( varname.length() > 2 && varname.at(1) == FIM_SYM_NAMESPACE_SEP )
		{
			try
			{
			//a specific namespace was selected!
			const fim_char_t ns = varname.at(0);
			const fim_var_id id = varname.c_str()+2;

			if( ns == FIM_SYM_NAMESPACE_WINDOW_CHAR )
#ifdef FIM_WINDOWS
			{
				//window variable
				nsp = window_;
				goto err;
			}
			else
			if( ns == FIM_SYM_NAMESPACE_VIEWPORT_CHAR )
			{
				//viewport variable
				if(window_)
					nsp = window_->current_viewportp();
				goto err;
			}
			else
#endif /* FIM_WINDOWS */
			if( ns == FIM_SYM_NAMESPACE_IMAGE_CHAR )
			{
				//image variable
#if FIM_WINDOWS
				if(window_ &&
				   window_->current_viewportp() && 
				   window_->current_viewportp()->getImage())
					nsp = window_->current_viewportp()->getImage();
#else /* FIM_WINDOWS */
				if ( viewport_->getImage() )
					nsp = viewport_->getImage();
#endif /* FIM_WINDOWS */
				goto err;
			}
			else
			if( ns == FIM_SYM_NAMESPACE_BROWSER_CHAR )
			{
				//browser variable
				nsp = & browser_;
				goto err;
			}
			else
			if( ns == FIM_SYM_NAMESPACE_GLOBAL_CHAR )
			{
				nsp = (Namespace*) this;
				goto err;
			}
			else
			if( ns != FIM_SYM_NAMESPACE_GLOBAL_CHAR )
			{
				//invalid namespace
				goto err;
			}
			}
			catch(FimException){}
		}
#endif /* FIM_NAMESPACES */
		nsp = this;
err:
		return nsp;
	}

	const Namespace * CommandConsole::c_rns(const fim_var_id varname)const
	{
		const Namespace * nsp = FIM_NULL;
#ifdef FIM_NAMESPACES
		if( varname.length() > 2 && varname.at(1) == FIM_SYM_NAMESPACE_SEP )
		{
			try
			{
			//a specific namespace was selected!
			const fim_char_t ns = varname.at(0);
			const fim_var_id id = varname.c_str()+2;

			if( ns == FIM_SYM_NAMESPACE_WINDOW_CHAR )
#ifdef FIM_WINDOWS
			{
				//window variable
				nsp = window_;
				goto err;
			}
			else
			if( ns == FIM_SYM_NAMESPACE_VIEWPORT_CHAR )
			{
				//viewport variable
				if(window_)
					nsp = window_->current_viewportp();
				goto err;
			}
			else
#endif /* FIM_WINDOWS */
			if( ns == FIM_SYM_NAMESPACE_IMAGE_CHAR )
			{
				//image variable
				nsp = browser_.c_getImage();
				goto err;
			}
			else
			if( ns == FIM_SYM_NAMESPACE_BROWSER_CHAR )
			{
				//browser variable
				nsp = & browser_;
				goto err;
			}
			else
			if( ns == FIM_SYM_NAMESPACE_GLOBAL_CHAR )
			{
				nsp = (Namespace*) this;
				goto err;
			}
			else
			if( ns != FIM_SYM_NAMESPACE_GLOBAL_CHAR )
			{
				//invalid namespace
				goto err;
			}
			}
			catch(FimException e){}
		}
#endif /* FIM_NAMESPACES */
		nsp = this;
err:
		return nsp;
	}

	fim_int CommandConsole::setVariable(const fim_var_id& varname,fim_int value)
	{
		if( Namespace *nsp = rns(varname) )
			nsp->setVariable(rnid(varname),value);
		return value;
	}

#if FIM_WANT_LONG_INT
	int CommandConsole::setVariable(const fim_var_id& varname,int value)
	{
		return setVariable(varname,static_cast<fim_int>(value));
	}

	unsigned int CommandConsole::setVariable(const fim_var_id& varname,unsigned int value)
	{
		return setVariable(varname,static_cast<fim_int>(value));
	}
#endif /* FIM_WANT_LONG_INT */

	fim_float_t CommandConsole::setVariable(const fim_var_id& varname,fim_float_t value)
	{
		if( Namespace *nsp = rns(varname) )
			nsp->setVariable(rnid(varname),value);
		return value;
	}

	const fim_char_t* CommandConsole::setVariable(const fim_var_id& varname,const fim_char_t*value)
	{
		if( Namespace *nsp = rns(varname) )
			nsp->setVariable(rnid(varname),value);
		return value;
	}

	const Var & CommandConsole::setVariable(const fim_var_id varname,const Var&value)
	{
		if( Namespace *nsp = rns(varname) )
			nsp->setVariable(rnid(varname),value);
		return value;
	}

	const fim::string& CommandConsole::setVariable(const fim_var_id varname,const fim::string&value)
	{
		if( Namespace *nsp = rns(varname) )
			nsp->setVariable(rnid(varname),value);
		return value;
	}

	fim_bool_t CommandConsole::isSetVar(const fim_var_id& varname)const
	{
		if( const Namespace *nsp = c_rns(varname) )
			return nsp->isSetVar(rnid(varname));
		return false;
	}

	fim_int CommandConsole::getIntVariable(const fim_var_id& varname)const
	{
		fim_int retval = 0;
		if( const Namespace *nsp = c_rns(varname) )
			retval = nsp->getIntVariable(rnid(varname));
		return retval;
	}

	fim_float_t CommandConsole::getFloatVariable(const fim_var_id& varname)const
	{
		fim_float_t retval = FIM_CNS_EMPTY_FP_VAL;
		if( const Namespace *nsp = c_rns(varname) )
			retval = nsp->getFloatVariable(rnid(varname));
		return retval;
	}

	fim::string CommandConsole::getStringVariable(const fim_var_id& varname)const
	{
		fim::string retval = FIM_CNS_EMPTY_RESULT;
		if( const Namespace *nsp = c_rns(varname) )
			retval = nsp->getStringVariable(rnid(varname));
		return retval;
	}

	const Var CommandConsole::getVariable(const fim_var_id& varname)const
	{
		if( const Namespace *nsp = c_rns(varname) )
			return nsp->getVariable(rnid(varname));
		else
			return {};
	}
} /* namespace fim */
