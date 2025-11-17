/* $LastChangedDate: 2015-01-17 15:47:05 +0100 (Sat, 17 Jan 2015) $ */
/*
 readline.h : Code dealing with the GNU readline library.

 (c) 2008-2015 Michele Martone

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
#ifndef FIM_READLINE_H
#define FIM_READLINE_H

#include <readline/readline.h>	/*	the GNU readline library	*/
#include <readline/history.h> 	/*	the GNU readline library	*/
#include <readline/keymaps.h> 	/*	the GNU readline library	*/
#include "fim_types.h"

namespace fim
{
	fim_char_t* fim_readline(const fim_char_t *prompt);
}

namespace rl
{
	void initialize_readline (fim_bool_t with_no_display_device, fim_bool_t wcs);
	int fim_search_rl_startup_hook(void);
}

#endif /* FIM_READLINE_H */

