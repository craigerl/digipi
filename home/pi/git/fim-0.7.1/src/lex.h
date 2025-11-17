/* $LastChangedDate: 2016-02-10 23:49:58 +0100 (Wed, 10 Feb 2016) $ */
/*
 lex.h : Lexer (lex) header file

 (c) 2007-2016 Michele Martone

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
#ifndef FIM_LEX_H
#define FIM_LEX_H
#include "fim_types.h"

typedef enum { intCon, floatCon, typeOpr, stringCon,vId } nodeEnum;
/* constants */
typedef struct {
fim_int value; /* value of constant */
} conNodeType;
/* identifiers */
typedef struct {
fim_char_t *s; /* subscript to sym array */
} stringNodeType;
typedef struct {
float f; /* subscript to sym array */
} fidNodeType;
/* operators */
typedef struct {
int i; /* subscript to sym array */
} idNodeType;
/* operators */
typedef struct {
int oper; /* operator */
int nops; /* number of operands */
struct nodeTypeTag *op[1]; /* operands (expandable) */
} oprNodeType;
typedef struct nodeTypeTag {
nodeEnum type; /* type of node */
int typeHint; /* WARNING : THIS COULD BE HARMFUL, DUE TO C HACKS.. */
/* union must be last entry in nodeType */
/* because operNodeType may dynamically increase */
union {
conNodeType con; /* constants */
stringNodeType scon; /* string constant */
fidNodeType fid; /* identifiers */
idNodeType id; /* identifiers */
oprNodeType opr; /* operators */
};
} nodeType;
extern int sym[26];

#endif /* FIM_LEX_H */

