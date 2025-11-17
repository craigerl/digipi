/* $LastChangedDate: 2013-07-04 23:07:07 +0200 (Thu, 04 Jul 2013) $ */
/*
 fim_plugin.h : Fim plugin definitions

 (c) 2011-2013 Michele Martone

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

#ifndef FIM_PLUGIN_FIM_H
#define FIM_PLUGIN_FIM_H
#include "fim.h"

#if FIM_WANT_EXPERIMENTAL_PLUGINS
fim_err_t fim_post_read_plugins_exec(struct ida_image *img, const fim_char_t * filename);
#endif /* FIM_WANT_EXPERIMENTAL_PLUGINS */
#endif /* FIM_PLUGIN_FIM_H */
