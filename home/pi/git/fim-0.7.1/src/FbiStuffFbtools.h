/* $LastChangedDate: 2013-07-04 23:07:07 +0200 (Thu, 04 Jul 2013) $ */
/*
 FbiStuffFbtools.h : fbi functions from fbtools.c, modified for fim

 (c) 2008-2013 Michele Martone
 (c) 1998-2006 Gerd Knorr <kraxel@bytesex.org>

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
/*
 * This file comes from fbi, and will undergo severe reorganization.
 * */

#include "fim.h"
#include "FramebufferDevice.h"

#ifndef FIM_FBISTUFFFBTOOLS_H
#define FIM_FBISTUFFFBTOOLS_H
namespace fim
{


/* info about videomode - yes I know, quick & dirty... */
/* init + cleanup */
int fb_probe(void);
void fb_catch_exit_signals(void);
void svga_dither_palette(int r, int g, int b);

}
#endif /* FIM_FBISTUFFFBTOOLS_H */
