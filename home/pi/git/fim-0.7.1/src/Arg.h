/* $LastChangedDate: 2024-01-07 12:46:19 +0100 (Sun, 07 Jan 2024) $ */
/*
 Arg.h : argument lists

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
#ifndef FIM_ARG_H
#define FIM_ARG_H
#include <vector>
#include "fim_string.h"
namespace fim
{
typedef std::vector<fim::string> args_t;
int fim_args_opt_count(const args_t& args, const char oc);
bool fim_args_opt_have(const args_t& args, fim::string optname);
std::ostream& operator<<(std::ostream& os, const args_t& v);
args_t fim_shell_args_escape(const args_t & args, bool quoted=true);
fim::string fim_args_as_cmd(const args_t & args);
}
#endif /* FIM_ARG_H */
