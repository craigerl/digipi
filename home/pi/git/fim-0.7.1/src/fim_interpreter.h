/* $LastChangedDate: 2016-02-16 12:08:20 +0100 (Tue, 16 Feb 2016) $ */
/*
 interpreter.h : Interpreter-related declarations

 (c) 2016-2016 Michele Martone

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

/* This file will grow, as definitions will be moved there from fim.h */

#ifndef FIM_INTERPRETER_H
#define FIM_INTERPRETER_H
#include "fim.h"

#if FIM_INDEPENDENT_NAMESPACE
#define FIM_SCON(V) scon(V)
#define FIM_VSCON(V,T) vscon(V,T)
#define FIM_FCON(V) fcon(V)
#define FIM_ICON(V) con(V)
#define FIM_OPR opr
#define FIM_FREENODE freeNode
#else /* FIM_INDEPENDENT_NAMESPACE */
#define FIM_SCON(V) scon(V)
#define FIM_VSCON(V,T) vscon(V,T)
#define FIM_FCON(V) fcon(V)
#define FIM_ICON(V) con(V)
#define FIM_OPR opr
#define FIM_FREENODE freeNode
#endif /* FIM_INDEPENDENT_NAMESPACE */

/* TODO: need to rename the functions in here */

void yyerror(const fim_char_t *s);
nodeType *opr(int oper, int nops, ...);
//nodeType *fid(float f);
nodeType *con(fim_int value);
nodeType *fcon(float fValue);
nodeType *scon(fim_char_t* s);
nodeType *vscon(fim_char_t*s,int typeHint);
void freeNode(nodeType *p);
Var ex(nodeType *p);

#endif /* FIM_INTERPRETER_H */
