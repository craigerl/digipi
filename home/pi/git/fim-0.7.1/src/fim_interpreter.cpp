/* $LastChangedDate: 2024-03-20 23:39:08 +0100 (Wed, 20 Mar 2024) $ */
/*
 interpreter.cpp : Fim language interpreter

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

#include <cstdio>
#include <map>
#include "lex.h"
#include "yacc.tab.hpp"
#include "fim.h"
#include "common.h"
#include "fim_interpreter.h"
#include <cstdarg>	/* va_list, va_arg, va_start, va_end */

#define FIM_INTERPRETER_ERROR "Unexpected !\n"

#ifdef DBG
#undef DBG
#endif /* DBG */

#if 0
#define DBG(X) if(dv)std::cout<<__LINE__<<":"<<X;
#else
#define DBG(X) if(dv)std::cout<<"# "<<indent(sd)<<X;
//#define DBG(X) 
#endif

fim::string static indent(const int sd)
{
	fim::string is;
	for(int i=0;i<sd;++i)
		is+=" ";
	return is;
}

#if FIM_INDEPENDENT_NAMESPACE
#define FIM_NO_BREAK /* fim::cc.catchLoopBreakingCommand(0)==0 */ 1
#define FIM_OPRND(P,N) ((P)->opr.op[(N)])
#define FIM_FACC(O)  (O)->fid.f
#define FIM_SACC(O)  (O)->scon.s
#define FIM_IACC(O)  (O)->con.value
#define FIM_NOPS(O)  (O)->opr.nops
#define FIM_FOPRND(P,N) FIM_FACC(FIM_OPRND(P,N))
#define FIM_IOPRND(P,N) FIM_IACC(FIM_OPRND(P,N))
#define FIM_OPRNDO(P) (P)->opr.oper
#define FIM_OPRNDT(P) (P)->type
#define FIM_OPRNDH(P) (P)->typeHint
#define FIM_GV(V) Var((fim_int)1) /* fim::cc.getVariable(V) */
#define FIM_SV(I,V) /*fim::cc.setVariable(I,V)*/
#define FIM_GVT(V) /*fim::cc.getVariableType(V)*/FIM_SYM_TYPE_FLOAT
#define FIM_EC(CMD,ARGS) /* fim::cc.execute(CMD,ARGS) */ "result" /*Var((fim_int)1)*/ /* FIXME: shall return Arg or Var or Val */
typedef nodeType * NodeType;
#else /* FIM_INDEPENDENT_NAMESPACE */
#define FIM_NO_BREAK ( abs(fim::cc.show_must_go_on()) == 1 && ( fim::cc.catchLoopBreakingCommand(0)==0) && (cc.getIntVariable(FIM_VID_IN_SLIDESHOW)!=1) )
#define FIM_OPRND(P,N) ((P)->opr.op[(N)])
#define FIM_FACC(O)  (O)->fid.f
#define FIM_SACC(O)  (O)->scon.s
#define FIM_IACC(O)  (O)->con.value
#define FIM_NOPS(O)  (O)->opr.nops
#define FIM_FOPRND(P,N) FIM_FACC(FIM_OPRND(P,N))
#define FIM_IOPRND(P,N) FIM_IACC(FIM_OPRND(P,N))
#define FIM_OPRNDO(P) (P)->opr.oper
#define FIM_OPRNDT(P) (P)->type
#define FIM_OPRNDH(P) (P)->typeHint
#define FIM_GV(V) fim::cc.getVariable(V)
#define FIM_SV_BARE(I,V) fim::cc.setVariable(I,V)
#ifdef FIM_WITH_LIBGTK
#define FIM_SV(I,V) \
			if ( ! (strlen(I)>2 && I[1] == FIM_SYM_NAMESPACE_SEP ) ) /* we're only concerned with globals */ \
			if ( /* isalpha(I[1]) && */ !fim::cc.isSetVar(I) ) /* and we only care if still unset; allow a one-char identifier */ \
				fim::cc.setVariable(FIM_INTERNAL_STATE_CHANGED,1); \
			FIM_SV_BARE(I,V)
#else /* FIM_WITH_LIBGTK */
#define FIM_SV(I,V) FIM_SV_BARE(I,V)
#endif /* FIM_WITH_LIBGTK */
#define FIM_GVT(V) fim::cc.getVariableType(V)
#define FIM_EC(CMD,ARGS) fim::cc.execute(CMD,ARGS)
typedef nodeType * NodeType;
#endif /* FIM_INDEPENDENT_NAMESPACE */
#define FIM_INTERPRETER_PARANOIA 0

namespace fim
{
	extern CommandConsole cc;
}

std::ostream& operator<<(std::ostream& os, const nodeType& p)
{
	os<< "type " << p.type << FIM_SYM_ENDL;
	return os;
}

static Var cvar(NodeType p, const bool dv, const int sd)
{
	/* evaluate a single 'arg' entry */
	NodeType np=p;
  	fim::string arg;

	if(FIM_OPRNDT(p) == typeOpr && FIM_OPRNDO(p)==FIM_SYM_STRING_CONCAT)
	{
		DBG("Concat (.)"<<FIM_SYM_ENDL)
		for(int i=0;i<FIM_NOPS(p);++i)
		{
			np=(FIM_OPRND(p,i));
			arg+=cvar(np,dv,sd).getString();
		}
		goto ret;
	}
	else
	if(FIM_OPRNDT(p) == stringCon )
	{
		DBG("stringCon"<<FIM_SYM_ENDL)
		arg=(FIM_SACC(p));
		return arg;
	}
	else
	if(FIM_OPRNDT(p) == vId )
	{	
		DBG("cvId"<<FIM_SYM_ENDL)
		return FIM_GV(FIM_SACC(p));
	}
	else if(FIM_OPRNDT(p) == intCon )
	{
		DBG("cvar:intCon: "<<FIM_IACC(p)<<FIM_SYM_ENDL)
		return Var(FIM_IACC(p));
	}
	else if(FIM_OPRNDT(p) == floatCon)
	{
		DBG("cvar:floatCon: "<<FIM_FACC(p)<<FIM_SYM_ENDL)
		return FIM_FACC(p);
	}
	else
	{
		//DBG("nest:"<<FIM_OPRNDT(p)<<"\n")
		return ex(p);
	}
ret:
	return arg;
}

static args_t var(NodeType p, const bool dv, const int sd)
{
	/* evaluate a whole chain of arg entries */
	NodeType np=p;
  	args_t args;

	if( FIM_OPRNDT(p) == typeOpr && FIM_OPRNDO(np)=='a' )
	for(int i=0;i<FIM_NOPS(p);++i)
	{
		np=(FIM_OPRND(p,i));
		if( FIM_OPRNDT(np) == stringCon )
		{
			DBG("stringConstant: "<<fim_shell_arg_escape(FIM_SACC(np))<<"\n")
			args.push_back(FIM_SACC(np));
		}
		else
		if(FIM_OPRNDT(np) == typeOpr && FIM_OPRNDO(np)=='a')
		{
		  	args_t vargs=var(np,dv,sd);
			for(size_t j=0;j<vargs.size();++j)
				args.push_back(vargs[j]);
		}
		else
		{
			args.push_back(cvar(np,dv,sd).getString());
		}
	}
	//DBG("?:\n")
	return args;
}

using namespace fim;
Var ex(NodeType p)
{
	static int sd=0; // stack depth; shall become member of a class 'Interpreter'
  	args_t args;
	fim_int typeHint;
	const bool dv = (FIM_GV(FIM_VID_DBG_COMMANDS).find('i')>=0);

	if (!p)
		goto ret;

	if (abs(fim::cc.show_must_go_on()) != 0)
	switch(FIM_OPRNDT(p))
	{
		case intCon:
			DBG("intConstant\n")
			return FIM_IACC(p);
	        case floatCon:
			DBG("floatConstant"<<FIM_FACC(p)<<FIM_SYM_ENDL)
			return FIM_FACC(p);
		case vId:
		{
			const fim_char_t*const varId=FIM_SACC(p);
			// may handle here special value "random" ...
			if(FIM_GVT(varId)==FIM_SYM_TYPE_INT)
			{
				DBG("getVar (int): "<<varId<<" = "<<fim::cc.getIntVariable(varId)<<FIM_SYM_ENDL)
				return FIM_GV(varId);
			}
			if(FIM_GVT(varId)==FIM_SYM_TYPE_FLOAT)
			{
				DBG("getVar (float): "<<varId<<" = "<<fim::cc.getFloatVariable(varId)<<FIM_SYM_ENDL)
				return FIM_GV(varId);
			}
			else
			{
				DBG("getVar (string): "<<varId<<" = "<<fim_shell_arg_escape(fim::cc.getStringVariable(varId))<<FIM_SYM_ENDL)
				return FIM_GV(varId);
			}
		}
		case stringCon:
			DBG("stringCon\n")
			// a single string token was encountered
			return Var(FIM_SACC(p));
		case typeOpr:	/*	some operator	*/
			//DBG("typeOpr\n")
		switch(FIM_OPRNDO(p))
		{
			case WHILE:
				DBG("Begin While\n")
				++sd;
		    		cc.unsetVariable(FIM_VID_IN_SLIDESHOW);
				fim::cc.catchLoopBreakingCommand(0);
				while(ex(FIM_OPRND(p,0)).getInt() && FIM_NO_BREAK)
				{
#if FIM_WANT_CMD_QUEUE
					cc.execute_queued();
#endif /* FIM_WANT_CMD_QUEUE */
					ex(FIM_OPRND(p,1));
				}
				--sd;
				DBG("End While\n")
				goto ret;
			case IF:
				DBG("Begin If:"<<(ex(FIM_OPRND(p,0)).getInt())<<FIM_SYM_ENDL)
				++sd;
				if (ex(FIM_OPRND(p,0)).getInt())
					ex(FIM_OPRND(p,1));
				else if (FIM_NOPS(p) > 2)
					ex(FIM_OPRND(p,2));
				--sd;
				DBG("End If\n")
				goto ret;
			case FIM_SYM_SEMICOLON:
				DBG("Semicolon (;)\n") // cmd;cmd
				ex(FIM_OPRND(p,0));
				if (abs(fim::cc.show_must_go_on()) == 2)
				{
					/* Workaround to interrupt an entire semantic command tree after interactive command break.
					 * Shall better use return codes of yyparse() for this. */
					fim::cout << "Command loop interrupted interactively." << FIM_SYM_ENDL;
					goto ret;
				}
				return ex(FIM_OPRND(p,1));
			case FESF:
			if( FIM_NOPS(p) >= 3 )
			{
				const fim_int n1 = ex(FIM_OPRND(p,0)).getInt();
				const fim_int n2 = ex(FIM_OPRND(p,1)).getInt();
				const flist_t fl = cc.browser_.get_file_list();
				fim::string result;

				for( fim_uint fi = FIM_MAX(0,n1-1); fi<n2; ++fi )
				if( fi < fl.size() )
				{
  					args_t fargs;

					if( FIM_NOPS(p) > 3 )
		          		{
						NodeType np=p;	
						np=(FIM_OPRND(np,3)); // right subtree first node

						while( np &&    FIM_NOPS(np) >=1 )
						if( FIM_OPRNDO(np)=='a' )
						{
							DBG("Args...\n")
					  		args_t na=var(np,dv,sd);
							DBG("Args: "<<na.size()<<FIM_SYM_ENDL)
				          		for(fim_size_t i=0;i<na.size();++i)
							{
								fim::string ss = na[i].c_str();
								ss.substitute( "[{][}]", fl[fi] );
								na[i] = ss;
								fargs.emplace_back(std::move(na[i]));
							}
							break;
						}
#if FIM_INTERPRETER_PARANOIA
						else if( FIM_OPRNDO(np)==FIM_SYM_STRING_CONCAT )
						{
							DBG(FIM_INTERPRETER_ERROR)
						}
#endif
					}

					if(p && FIM_OPRND(p,2))
					{
						const fim_char_t * cmd = FIM_SACC(FIM_OPRND(p,2));

						if(cmd)
						{
							DBG("Begin Exec: " << cmd << " " << fargs << "\n")
							++sd;
							result = FIM_EC(cmd,fargs);
							// NOTE: would make sense to interrupt loop after first failing command
							--sd;
							DBG("End Exec: " << cmd << " " << fargs << "\n")
						}
					}
		}
				goto ret;
			}
			else
				goto err;
			case 'r':
			if( FIM_NOPS(p) == 2 )
			{
				fim_int times=ex(FIM_OPRND(p,1)).getInt();
				DBG("Begin Repeat " << times << "x\n")
				++sd;
				if(times<0)
					goto err;
				fim::cc.catchLoopBreakingCommand(0);
				for (fim_int i=0;i<times && FIM_NO_BREAK ;++i)
					ex(FIM_OPRND(p,0));
				--sd;
				DBG("End Repeat " << times << "x\n")
				goto ret;
			}
			else
				goto err;
			case 'x': 
				//DBG("X\n")
			  	/*
			     * when encountering an 'x' node, the first (left) subtree should 
			     * contain the string with the identifier of the command to 
			     * execute.
			     */
				{
#if FIM_INTERPRETER_PARANOIA
			  		if( FIM_NOPS(p) < 1 )
			  		{
						DBG(FIM_INTERPRETER_ERROR)
						goto err;
					}
#endif
			  		if(FIM_NOPS(p)==2)	//int yacc.ypp we specified only 2 ops per x node
		          		{
						NodeType np=p;	
						np=(FIM_OPRND(np,1)); //the right subtree first node
						// DBG("ARGS:"<<FIM_NOPS(np)<<FIM_SYM_ENDL)
						while( np &&    FIM_NOPS(np) >=1 )
						if( FIM_OPRNDO(np)=='a' )
						{
							DBG("Args...\n")
					  		args_t na=var(np,dv,sd);
							DBG("Args: "<<na.size()<<FIM_SYM_ENDL)
				          		for(fim_size_t i=0;i<na.size();++i)
							{
								args.emplace_back(std::move(na[i]));
							}
							break;
						}

#if FIM_INTERPRETER_PARANOIA
						else if( FIM_OPRNDO(np)==FIM_SYM_STRING_CONCAT )
						{
							DBG(FIM_INTERPRETER_ERROR)
						}
#endif
					}
					{
						fim::string result;
						if(p && FIM_OPRND(p,0))
						{
							const fim_char_t * cmd = FIM_SACC(FIM_OPRND(p,0));
							if(cmd)
							{
								DBG("Begin Exec: " << cmd << " " << args << "\n")
								++sd;
								result = FIM_EC(cmd,args);
								--sd;
								DBG("End Exec: " << cmd << " " << args << "\n")
							}
						}
						return fim_atoi(result.c_str());
			  		}
		}
		case 'a':
			// we shouldn't be here, because 'a' (argument) nodes are evaluated elsewhere
			assert(0);
			goto err;
		case '=':
		{
			fim_char_t *s=FIM_NULL;
			s=FIM_SACC(FIM_OPRND(p,0));
			//DBG("SV:"<<s<<FIM_SYM_ENDL)
			typeHint=FIM_OPRNDH(FIM_OPRND(p,0));
			if(typeHint=='a')
			{
				DBG("Assign (=)\n")
				Var v=cvar(FIM_OPRND(p,1),dv,sd);
				FIM_SV(s,v);
			        DBG("Set:"<<s<<"="<<        v.getString()<<" ("<<(fim_char_t)v.getType()<<")\n")
			        DBG("Get:"<<s<<"="<<FIM_GV(s).getString()<<" ("<<(fim_char_t)FIM_GV(s).getType()<<")\n")
			       	return v;
			}
			else
			{
			}
		}
#if FIM_WANT_AVOID_FP_EXCEPTIONS
			case '%': {Var v1=ex(FIM_OPRND(p,0)),v2=ex(FIM_OPRND(p,1)); if(v2.getInt())return v1%v2; else return v2;}
			case '/': {Var v1=ex(FIM_OPRND(p,0)),v2=ex(FIM_OPRND(p,1)); if(v2.getInt())return v1/v2; else return v2;}
#else /* FIM_WANT_AVOID_FP_EXCEPTIONS */
			case '%': return ex(FIM_OPRND(p,0)) % ex(FIM_OPRND(p,1));
			case '/': return ex(FIM_OPRND(p,0)) / ex(FIM_OPRND(p,1));
#endif /* FIM_WANT_AVOID_FP_EXCEPTIONS */
			case '+': return ex(FIM_OPRND(p,0)) + ex(FIM_OPRND(p,1));
			case '!': return (fim_int)(((ex(FIM_OPRND(p,0))).getInt())==0?1:0);
			case UMINUS: return Var((fim_int)0) - ex(FIM_OPRND(p,0));
			case '-': 
				/*DBG("SUB (-)\n");*/
				if ( 2==FIM_NOPS(p) ) {Var d= ex(FIM_OPRND(p,0)) - ex(FIM_OPRND(p,1));return d;}
				else return Var((fim_int)0) - ex(FIM_OPRND(p,0));
			case '*': return ex(FIM_OPRND(p,0)) * ex(FIM_OPRND(p,1));
			case '<': return ex(FIM_OPRND(p,0)) < ex(FIM_OPRND(p,1));
			case '>': return ex(FIM_OPRND(p,0)) > ex(FIM_OPRND(p,1));
			case GE: return ex(FIM_OPRND(p,0)) >= ex(FIM_OPRND(p,1));
			case LE: return ex(FIM_OPRND(p,0)) <= ex(FIM_OPRND(p,1));
			case NE: return ex(FIM_OPRND(p,0)) != ex(FIM_OPRND(p,1));
			case EQ: {/*DBG("EQ (==)\n");*/return ex(FIM_OPRND(p,0)) == ex(FIM_OPRND(p,1));}
			case REGEXP_MATCH: return ex(FIM_OPRND(p,0)).re_match(ex(FIM_OPRND(p,1)));
			case AND:return ex(FIM_OPRND(p,0)) && ex(FIM_OPRND(p,1));
			case OR :return ex(FIM_OPRND(p,0)) || ex(FIM_OPRND(p,1));
			case BOR :return ex(FIM_OPRND(p,0)) | ex(FIM_OPRND(p,1));
			case BAND:return ex(FIM_OPRND(p,0)) & ex(FIM_OPRND(p,1));
			default: goto err;
		}
	}
ret:
	return Var((fim_int)0);
err:
	return Var((fim_int)-1);
}

/*
 * string constant handling
 */
nodeType *scon(fim_char_t*s)
{
	if(s==NULL)yyerror("TOKEN NULL!\n");
	nodeType *p;
	/* allocate node */
	if ((p =(nodeType*) malloc(sizeof(nodeType))) == NULL)
		yyerror(FIM_EMSG_OUT_OF_MEM);
	/* copy information */
	p->type = stringCon; 
	p->scon.s=s;
	return p;
}

nodeType *vscon(fim_char_t*s,int typeHint)
{
#ifdef FIM_RANDOM
	if( strlen(s) > 5 ) // make sure s points to 6+ bytes
#ifndef FIM_BIG_ENDIAN
#if ((SIZEOF_INT)>=8)
	if( *reinterpret_cast<int*>(s+0) == 0x00006d6f646e6172 ) // ..modnar // note: wrong (can't assume two zeros, and eight byte illegal)
#else /* SIZEOF_INT */
	/* this is LSB order, so it is not portable code.  */
	if( *reinterpret_cast<int*>(s+0) == 0x646e6172 // dnar
	&& (*reinterpret_cast<int*>(s+3))== 0x006d6f64    ) // .mod // access legal bytes
	//&& (*reinterpret_cast<int*>(s+4)<<8)== 0x006d6f00    ) // .mo. eight byte illegal
#endif /* SIZEOF_INT */
#else /* FIM_BIG_ENDIAN */
#if ((SIZEOF_INT)>=8)
	if( *reinterpret_cast<int*>(s+0) == 0x72616e646f6d0000 ) // random.. // note: wrong (can't assume two zeros, and eight byte illegal)
#else /* SIZEOF_INT */
	if( *reinterpret_cast<int*>(s+0) == 0x72616e64 // rand
	&& (*reinterpret_cast<int*>(s+3))== 0x646f6d00    ) // dom. // access legal bytes
	//&& (*reinterpret_cast<int*>(s+4)<<8)== 0x006f6d00    ) // .om. // eight byte illegal
#endif /* SIZEOF_INT */
#endif /* FIM_BIG_ENDIAN */
	{
		fim_free(s);
		return con(fim_rand());
	}
#endif /* FIM_RANDOM */

	nodeType *p=scon(s);
	if(p)
		p->type = vId,
		p->typeHint = typeHint; 
	return p;
}

nodeType *fcon(float fValue)
{
	nodeType *p;
	/* allocate node */
	if ((p =(nodeType*) malloc(sizeof(nodeType))) == NULL)
		yyerror(FIM_EMSG_OUT_OF_MEM);
	/* copy information */
	p->type = floatCon;
	p->fid.f = fValue;
	return p;
}

nodeType *con(fim_int value)
{
	nodeType *p;
	/* allocate node */
	if ((p =(nodeType*) malloc(sizeof(nodeType))) == NULL)
		yyerror(FIM_EMSG_OUT_OF_MEM);
	/* copy information */
	p->type = intCon;
	p->con.value = value;
	return p;
}

nodeType *opr(int oper, int nops, ...)
{
	va_list ap;
	nodeType *p;
	int i;
	/* allocate node */
	if ((p =(nodeType*) malloc(sizeof(nodeType) * nops)) == NULL)
		yyerror(FIM_EMSG_OUT_OF_MEM);
	/* copy information */
	p->type = typeOpr;
	p->opr.oper = oper;
	p->opr.nops = nops;
	va_start(ap, nops);
	for (i = 0; i < nops; i++)
	p->opr.op[i] = va_arg(ap, nodeType*);
	va_end(ap);
	return p;
}

void freeNode(nodeType *p)
{
	if (!p) return;
	if (p->type == stringCon)
		{fim_free(p->scon.s);p->scon.s=NULL;}
	if (p->type == vId)
		{fim_free(p->scon.s);p->scon.s=NULL;}
	if (p->type == typeOpr)
	{
		for (int i = 0; i < p->opr.nops; i++)
			freeNode(p->opr.op[i]);
	}
	free (p);
}

void yyerror(const fim_char_t *s)
{
	//fprintf(stdout, "%s \n", s);
	/* omitting std:: causes error on darwin gcc */
	//std::cout << s << "\n";
#if FIM_INDEPENDENT_NAMESPACE
	std::cout << s << FIM_SYM_ENDL;
#else /* FIM_INDEPENDENT_NAMESPACE */
	fim::cout << s << FIM_SYM_ENDL;
#endif /* FIM_INDEPENDENT_NAMESPACE */
}

