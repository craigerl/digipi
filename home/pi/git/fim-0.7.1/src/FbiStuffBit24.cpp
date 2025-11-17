/* $LastChangedDate: 2022-10-30 16:13:52 +0100 (Sun, 30 Oct 2022) $ */
/*
 FbiStuffBit24.cpp : reading any file as a raw 24 bits per pixel pixelmap

 (c) 2007-2022 Michele Martone
 based on code (c) 1998-2006 Gerd Knorr <kraxel@bytesex.org>

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

#include "fim.h"

#if FIM_WANT_RAW_BITS_RENDERING

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#ifdef HAVE_ENDIAN_H
 #include <endian.h>
#endif /* HAVE_ENDIAN_H */

namespace fim
{

extern CommandConsole cc;

/* ---------------------------------------------------------------------- */

typedef unsigned int   uint32;
typedef unsigned short uint16;

/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct bit24_state {
    FILE *fp;
    uint32 w;
    uint32 h;
    long flen; /* for ftell() */
};

static void*
bit24_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	 struct ida_image_info *i, int thumbnail)
{
    struct bit24_state *h=FIM_NULL;
    const fim_int prwv=cc.getIntVariable(FIM_VID_PREFERRED_RENDERING_WIDTH);
    const fim_int prw=prwv<1?FIM_BITRENDERING_DEF_WIDTH:prwv;
 
    h = (struct bit24_state *)fim_calloc(1,sizeof(*h));
    if(!h)
	    goto oops;
    h->fp = fp;
    if(fseek(fp,0,SEEK_END)!=0)
	    goto oops;
    if((h->flen=ftell(fp))==-1)
	    goto oops;

    if(h->flen < static_cast<long>(prw*3))
    {
    	i->width  = h->w = h->flen/3;
    	i->height = h->h = 1;
    }
    else
    {
    	i->width  = h->w = prw;
    	i->height = h->h = FIM_INT_FRAC(h->flen,h->w*3); // should pad
    }

    i->npages = 1;
    return h;
 oops:
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
bit24_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct bit24_state *h = (struct bit24_state *) data;
    unsigned int ll,y,x = 0;
    
	y  = line ;
	if(y==h->h-1)
		ll = h->flen - h->w*3 * (h->h-1);
	else
	{
		ll = h->w * 3;
	}

	fseek(h->fp,0 + y * ll,SEEK_SET);

	for (x = 0; x < h->w; x++)
	{
		*(dst++) = fgetc(h->fp);
		*(dst++) = fgetc(h->fp);
		*(dst++) = fgetc(h->fp);
	}
//	if(y==h->h-1) fim_bzero(dst,h->w*3-3*x);
}

static void
bit24_done(void *data)
{
    struct bit24_state *h = (struct bit24_state *) data;

    fclose(h->fp);
    fim_free(h);
}

struct ida_loader bit24_loader = {
/*
 * 0000000: 7f45 4c46 0101 0100 0000 0000 0000 0000  .ELF............
 */
    /*magic:*/ "ELF",
    /*moff:*/  1,
    /*mlen:*/  3,
    /*name:*/  "Bit24",
    /*init:*/  bit24_init,
    /*read:*/  bit24_read,
    /*done:*/  bit24_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&bit24_loader);
}


}
#endif /* FIM_WANT_RAW_BITS_RENDERING */
