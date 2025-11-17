/* $LastChangedDate: 2024-03-22 00:40:40 +0100 (Fri, 22 Mar 2024) $ */
/*
 Var.h : Var class header file

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

#ifndef FIM_VAR_H
#define FIM_VAR_H

#include "string.h"
#include "fim_types.h"

#define FIM_FFL_PRT printf("In %s located in %20s:%d :\n",__func__,__FILE__,__LINE__)
#if 0
#define DBG(X) FIM_FFL_PRT;std::cout<<X;
#else
#define DBG(X) 
#endif

namespace fim
{
	enum fim_vtc { FimTypeInt='i', FimTypeDefault='i', FimTypeFloat='f', FimTypeString='s', FimTypeWrong='!' };
class Var FIM_FINAL
{
	typedef int fim_var_t;
	fim_var_t type;
	union
	{
		fim_float_t f;
		fim_int i;
	};
	fim::string s_;
	public:
	Var(const Var& v):
		type(FimTypeDefault),i(0)
	{
		this->set(v);
	}

	int set(const Var& v)
	{
		DBG("(v:?)\n");
		this->type=v.type;
		if(type==FimTypeInt)
			this->i=v.i;
		if(type==FimTypeFloat)
			this->f=v.f;
		if(type==FimTypeString)
			this->s_=v.s_;
		return getInt();
	}

	Var(fim_float_t v):
		type(FimTypeFloat),i(0)
	{
		DBG("(f:"<<v<<")\n");
		f=v;
	}

	Var(bool v):
		type(FimTypeDefault),i(0)
	{
		DBG("(i:"<<v<<")\n");
		type=FimTypeInt;
		i=v?1:0;
	}

	Var(fim_int v)
	:type(FimTypeDefault),i(0)
	{
		DBG("(i:"<<v<<")\n");
		type=FimTypeInt;
		i=v;
	}

	Var(void)
	:type(FimTypeDefault),i(0)
	{
		const fim_char_t *s="0";
		DBG("(v())\n");
		type=FimTypeInt;
		if(type==FimTypeInt)i=fim_atoi(s);
		else
		       	if(type==FimTypeFloat)
				f=fim_atof(s);
		else
		       	if(type==FimTypeString)
				this->s_=s;
		else
		       	i=0;
	}

	Var(const fim::string s):
		type(FimTypeDefault),i(0)
	{
		type=FimTypeString;
		this->s_=s.c_str();
	}

	Var(const fim_char_t*s):
		type(FimTypeDefault),i(0)
	{
		type=FimTypeString;
		this->s_=s;
	}
	const Var& operator= (int   i){DBG("2i:"<<i<<"\n";type=FimTypeInt);this->i=i;return *this;}
	const Var& operator= (fim_float_t f){setFloat(f);return *this;}
	const Var& operator= (fim::string& s){setString(s);return *this;}
	const Var& operator= (const Var& rhs){set(rhs);return *this;}
	//Var concat(const Var& v)const{return this->getString()+v.getString();}

	fim_float_t setFloat(fim_float_t f){type=FimTypeFloat;return this->f=f;}
	fim_int   setInt(fim_int i){type=FimTypeInt;return this->i=i;}
	fim::string setString(const fim::string& s){type=FimTypeString;this->s_=s;return this->s_;}
	int getType(void)const{return type;}
	fim_int getInt(void)const{return(type==FimTypeInt)?i:
		(type==FimTypeFloat?(static_cast<fim_int>(f)):
		 (type==FimTypeString?(fim_atoi(s_.c_str())):0)
		 );
		}

	fim_float_t getFloat(void)const{
	
	return(type==FimTypeFloat)?f:
		(type==FimTypeInt?
		 	((fim_float_t)i):
			((type==FimTypeString)?fim_atof(s_.c_str()):0.0f)
			);
			}

	fim::string getString(void)const
	{
		DBG("t:"<<(char)type <<"\n");
		if(type==FimTypeString)
			return this->s_;
		else
		{
			fim_char_t buf[FIM_CHARS_FOR_INT];
			if(type==FimTypeInt)
				fim_snprintf_fim_int(buf,i);
			else
			       	if(type==FimTypeFloat)
					sprintf(buf,"%f",f);
			DBG(" v:"<<buf <<"\n");
			return buf;
		}
		
	}

	operator fim_int(void)const{return getInt();}
//	operator int(void)const{return getInt();}
///	operator fim_float_t(void)const{return getFloat();}
//	operator string(void)const{return getString();}

	Var  operator<=(const Var& v)const { return getFloat()<=v.getFloat(); }
	Var  operator>=(const Var& v)const { return getFloat()>=v.getFloat(); }
	Var  operator< (const Var& v)const { return getFloat()< v.getFloat(); }
	Var  operator> (const Var& v)const { return getFloat()> v.getFloat(); }
	private:
	bool both_typed(fim_var_t t, const Var& v)const {return getType() == t && ( ((getType()==t) && (v.getType()==t))); }
	//#define _types() DBG("t1:"<<(getType())<<",t2:"<<(v.getType())<<"\n");
	public:
	Var operator!=(const Var& v)const {
		//_types() 
		if(both_typed(FimTypeInt,v))
			return getInt() !=v.getInt(); 
		if(both_typed(FimTypeFloat,v))
			return getFloat() !=v.getFloat();
		if(both_typed(FimTypeString,v))
			return getString()!=v.getString(); 
		return getFloat()!=v.getFloat();
	}
	Var operator==(const Var& rhs)const {DBG("EQV\n"); return 1-(*this != rhs).getInt(); }
	Var operator/ (const Var& v)const
	{
		if(both_typed(FimTypeInt,v))
			return getInt()/(v.getInt()!=0?v.getInt():1); 
		return getFloat()/(v.getFloat()!=0.0?v.getFloat():1); 
	}
	Var operator* (const Var& v)const
	{
		if(both_typed(FimTypeInt,v))
			return getInt()*v.getInt(); 
		if(both_typed(FimTypeFloat,v))
			return getFloat()*v.getFloat();
		if(both_typed(FimTypeString,v))
			return getFloat()*v.getFloat();
		return getFloat()*v.getFloat(); 
	}
	Var operator| (const Var& v)const
	{
		return getInt()|v.getInt(); 
	}
	Var operator& (const Var& v)const
	{
		return getInt()&v.getInt(); 
	}
	Var operator+ (const Var& v)const
	{
		if(both_typed(FimTypeInt,v))
			return getInt()+v.getInt(); 
		if(both_typed(FimTypeFloat,v))
			return getFloat()+v.getFloat();
		if(both_typed(FimTypeString,v))
			return getString()+v.getString();
		return getFloat()+v.getFloat(); 
	}
	Var operator- (const Var& v)const
	{
		DBG("SUB"<<getType()<<"-"<<v.getType()<<"\n"); 
		DBG("SUB"<<getFloat()<<"-"<<v.getFloat()<<"\n"); 
		DBG("SUB"<<getString()<<"-"<<v.getString()<<"\n"); 
		//std::cout<<"t1:"<<(getType())<<",t2:"<<(v.getType())<<"\n";
		if(both_typed(FimTypeInt,v))
			return getInt()-v.getInt(); 
		if(both_typed(FimTypeFloat,v))
			return getFloat()-v.getFloat();
		if(v.type==FimTypeString)
		{
			if(type!=FimTypeString)
			{
				if((type==FimTypeFloat && f==0) || (type==FimTypeInt && i==0))
					return Var{""};
				return *this;
			}
#if FIM_WANT_STRINGS_SUBTRACT
			if (!isdigit(s_[0]) && !isdigit(v.s_[0]))
			{
				fim::string ts = getString();
				ts.substitute(v.getString().c_str(), "");
				return ts;
			}
#endif /* FIM_WANT_STRINGS_SUBTRACT */
			return getFloat()-v.getFloat();
		}
		return getFloat()-v.getFloat(); 
	}
	int eq (const char*s)const
	{
		int aeq = 1;
		if(s)
			aeq = strcmp(s,this->getString().c_str());
		return aeq == 0;
	}
	int re (const Var& v)const
	{
		return re_match(v).getInt();
	}
	Var operator- (void)const {
		if(getType()==FimTypeFloat)return - getFloat(); 
		if(getType()==FimTypeString)return - getFloat(); 
		/*if(getType()==FimTypeInt)*/return - getInt(); 
	}
	Var operator% (const Var& v)const { return getInt()%v.getInt(); }
//	Var operator, (const Var& v)const { return (getString()+v.getString()); }
	Var operator&&(const Var& v)const { return getInt()&&v.getInt(); }
	Var operator||(const Var& v)const { return getInt()||v.getInt(); }
	Var re_match(const Var& v)const { return getString().re_match(v.getString().c_str()); }
	//#undef _types
	std::ostream& print(std::ostream& os)const;
	size_t size(void) const { return sizeof(Var) + this->s_.capacity(); }
	void shrink_to_fit(void) {
	       	this->s_.shrink_to_fit();
       	}
	int find(const fim_char_t c)const{const char*p=FIM_NULL,*r=FIM_NULL;if(getType()==FimTypeString && (p=s_.c_str()) && (r=strchr(s_.c_str(),c)))return r-p; return -1; }
	int find(const char * const s)const{const char*p=FIM_NULL,*r=FIM_NULL;if(getType()==FimTypeString && (p=s_.c_str()) && (r=strstr(s_.c_str(),s)))return r-p; return -1; }
};
	fim::string fim_var_help_db_query(const fim::string& id);
	void fim_var_help_db_init(void);
	fim::string fim_get_variables_reference(FimDocRefMode refmode);
	std::ostream& operator<<(std::ostream& os, const Var& var);
}

#endif /* FIM_VAR_H */
