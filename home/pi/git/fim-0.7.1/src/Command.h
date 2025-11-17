/* $LastChangedDate: 2023-10-12 23:28:06 +0200 (Thu, 12 Oct 2023) $ */
/*
 Command.h : Fim Command class header file

 (c) 2007-2023 Michele Martone

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
#ifndef FIM_COMMAND_H
#define FIM_COMMAND_H
#include "fim.h"
#include <functional>
namespace fim
{
	using fim_cmd_id = fim::string; // command id
	using fim_cls = fim::string; // command line statement

class Command FIM_FINAL
{
	fim_cmd_id cmd_;
	fim::string help_;
	public:
	template<class O> explicit Command(fim_cmd_id cmd, fim::string help, O*op, fim::string(O::*fp)(const args_t&));

	private:
	typedef std::function<fim::string (const args_t&)> cmf_t;
	cmf_t cmf_;

	public:
	const fim_cmd_id & cmd(void)const;
	const fim::string & getHelp(void)const;
	fim::string execute(const args_t&args);
	bool operator < (const Command&c)const{return cmd_< c.cmd_;}
	bool operator <=(const Command&c)const{return cmd_<=c.cmd_;}
	~Command(void) { }
};
	template<class O>
	Command::Command(fim_cmd_id cmd, fim::string help, O*op, fim::string(O::*fp)(const args_t&))
	       	:cmd_(cmd),help_(help),cmf_(std::bind(fp,op,std::placeholders::_1)) {}

} /* namespace fim */
#endif /* FIM_COMMAND_H */
