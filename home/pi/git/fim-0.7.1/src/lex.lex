/* $Id: lex.lex 2108 2024-03-09 16:19:26Z dezperado $ */
/*
 lex.lex : Lexer source file template

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

%option noyywrap
%{
#include <math.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>	/* 20110709 now calling flex --nounistd; this way we have to include it by ourselves; this is good because we overcome the C++ isatty redeclaration from within lex.yy.cc, which often does not match that of unistd.h regarding the thrown exceptions   */ 
#include "lex.h"
#include "yacc.tab.hpp"
#include "common.h"
#include "fim.h"	/* fim_calloc, ... */
void yyerror(const fim_char_t *);
#if 0
/* we use %option noyywrap now ! */
#ifdef YY_PROTO
//this branch is taken by flex 2.5.4
//int yywrap YY_PROTO((void)){return 1;}
#else
//this branch is taken by flex 2.5.33
int yywrap (){return 1;}
#endif
#endif

#if FIM_WANT_PIPE_IN_LEXER
int fim_pipedesc[2];
/*#define YY_INPUT(buf,result,max_size) \
{ \
	int r=read(fim_pipedesc[0],buf,1); \
	printf("letti in input : %d\n",r); \
	result = (buf[0]==EOF||r<1)?YY_NULL:1; \
	return; \
}*/

#define YY_INPUT(buf,result,max_size) \
{ \
	int r=read(fim_pipedesc[0],buf,1); \
	result = (buf[0]==EOF||r<1)?EOB_ACT_END_OF_FILE:EOB_ACT_CONTINUE_SCAN; \
	result = (buf[0]==EOF||r<1)?0:1; \
	if(result<=0) {close(fim_pipedesc[0]);close(fim_pipedesc[1]);} \
}
#else /* FIM_WANT_PIPE_IN_LEXER */
fim_cmd_queue_t fim_cmdbuf;

#define YY_INPUT(buf,result,max_size) \
{ \
	const int r = fim_cmdbuf.size() ? 1 : 0; \
	if (r) { buf[0]=fim_cmdbuf.front(); fim_cmdbuf.pop(); } \
	else { buf[0]=EOF; } \
	result = (buf[0]==EOF||r<1)?EOB_ACT_END_OF_FILE:EOB_ACT_CONTINUE_SCAN; \
	result = (buf[0]==EOF||r<1)?0:1; \
	if(result<=0) { } \
}
#endif /* FIM_WANT_PIPE_IN_LEXER */

//allocate and strcpy
#define astrcpy(dst,src) \
{ \
	if((src)==NULL)yyerror("null pointer given!\n"); \
	if(((dst)=(fim_char_t*)fim_calloc(1+strlen(src),1))==NULL) \
		yyerror("out of memory\n"); \
	strcpy((dst),(src)); \
}

//quoted allocate and strcpy
#define qastrcpy(dst,src) \
{ \
	if((src)==NULL)yyerror("null pointer given!\n"); \
	(src+1)[strlen(src+1)-1]='\0'; \
	astrcpy(dst,src+1) \
}

//to lower
#define tl(src) \
{ \
	if((src)==NULL)yyerror("null pointer given!\n"); \
	{fim_char_t*s=src;while(*s){*s=tolower(*s);++s;}} \
}


%}


DIGIT    [0-9]
NUMBER [0-9]+
_ID       [a-z][a-z0-9]*
UC  [A-Z]
LC  [a-z]
Q   \'
DQ  \"
C   UC|LC
CD  C|DIGIT
__ID  [a-zA-Z]CD*
ID  [a-z_A-Z][a-z_A-Z0-9]*
ARG [a-z_A-Z0-9]+
HH  [a-f]|[A-F]|[0-9]
__SYMBOL   [%:;,.\-\\$|!/(){}_]
SYMBOL [-()<>=+*\/;{}.,`:$\\^%#]
EXCLMARK !
STRINGC	 {SYMBOL}|{DIGIT}|{UC}|{LC}|[ _]
STRINGC_Q  {STRINGC}|\"
STRINGC_DQ {STRINGC}|\'


%%
^"|" return SYSTEM;
"!"  return NOT;
">=" return GE;
"<=" return LE;
"==" return EQ;
"=~" return REGEXP_MATCH;
"!=" return NE;
"&&" return AND;
"&" return BAND;
"||" return OR;
"|" return BOR;
"while" return WHILE;
"if" return IF;
"else" return ELSE;
"do" return DO;

"i:*"	{
		astrcpy(yylval.sValue,yytext);
		return IDENTIFIER;
	}

([gi]:)?{ID}	{
			/*was: ([gwibv]:)?{ID} */
		astrcpy(yylval.sValue,yytext);
		//tl(yylval.sValue);
		// tolower breaks aliases, but it would be useful on  keywords, above..
		return IDENTIFIER;
	}

"0"[0-9]+ {
		yylval.iValue = strtol(yytext,NULL,8);
		return INTEGER;
	}

"0x"[0-9]+ {
		yylval.iValue = strtol(yytext,NULL,16);
		return INTEGER;
	}

[0-9]+	{
		yylval.iValue = fim_atoi(yytext);
		return INTEGER;
	}

"$" 	{
		yylval.iValue = FIM_CNS_LAST;
		return INTEGER;
	}

"^"	{
		yylval.iValue = FIM_CNS_FIRST;
		return INTEGER;
	}

"'"{DIGIT}+"."{DIGIT}*"'" {
		yylval.fValue = fim_atof(yytext+1);
		return QUOTED_FLOAT;
	}

"\""{DIGIT}+"."{DIGIT}*"\"" {
		yylval.fValue = fim_atof(yytext+1);
		return QUOTED_FLOAT;
	}

\'((\\\')|[^\'])*\' {
	/* single quoted strings escaping */
		trec(yytext+1,"n\\\'","\n\\\'");
		qastrcpy(yylval.sValue,yytext);;
		return STRING;
	}

\"((\\\")|[^\"])*\" {
	/* double quoted strings unescaping */
		trec(yytext+1,"n\\\"","\n\\\"");
		//trhex(yytext+1); // hex escaping already perfomed in trec.
		qastrcpy(yylval.sValue,yytext);;
		return STRING;
	}

"./"{STRINGC}* {/* FIXME : "/"{STRINGC} - like tokens clashed with lone / operator */
		/* FIM_SMART_COMPLETION patch */
		/* a path */
		astrcpy(yylval.sValue,yytext);;
		return FILE_PATH;
	}

"../"{STRINGC}* {
		/* FIM_SMART_COMPLETION patch */
		/* a path */
		astrcpy(yylval.sValue,yytext);;
		return FILE_PATH;
	}

{ID}([.]{ID})+ {
		/* FIM_SMART_COMPLETION patch */
		/* a path */
		astrcpy(yylval.sValue,yytext);;
		return FILE_PATH;
	}


{SYMBOL} {
		return *yytext;
	}

^"/"([^;])+  {  
		astrcpy(yylval.sValue,yytext+1);;
		return SLASH_AND_REGEXP;
	}

{DIGIT}+"."{DIGIT}* {
		yylval.fValue = fim_atof(yytext+0);
		return UNQUOTED_FLOAT;
	}

[ \t]+ { /* we ignore whitespace */ ; }


\n { /* return NEWLINE; */ /* this works ! in this case, it means that we ignore \n */ }


. printf("Unknown character :'%s'\n",yytext);yyerror("Unknown character");
	/*THERE SHOULD GO LEX ERRORS..*/

\#.*$  { /*  ... a comment encountered... */ ; }

%%

#if HAVE_FLEXLEXER_H
yyFlexLexer lexer; // better here than FlexLexer*lexer elsewhere
#endif /* HAVE_FLEXLEXER_H */
