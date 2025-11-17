/* $LastChangedDate: 2022-10-30 16:13:52 +0100 (Sun, 30 Oct 2022) $ */
/*
 FbiStuffBit1.cpp : reading any file as a raw 1 bit per pixel pixelmap

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

struct bit1_state {
    FILE *fp;
    uint32 w;
    uint32 h;
    long flen; /* for ftell() */
};

static void*
bit1_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	 struct ida_image_info *i, int thumbnail)
{
    struct bit1_state *h=FIM_NULL;
    const fim_int prwv=cc.getIntVariable(FIM_VID_PREFERRED_RENDERING_WIDTH);
    const fim_int prw=prwv<1?FIM_BITRENDERING_DEF_WIDTH:prwv;

    h = (struct bit1_state *)fim_calloc(1,sizeof(*h));
    if(!h)
	    goto oops;
    h->fp = fp;
    if(fseek(fp,0,SEEK_END)!=0)
	    goto oops;
    if((h->flen=ftell(fp))==-1)
	    goto oops;

    if(8*h->flen < static_cast<long>(prw))
    {
	i->width  = h->w = h->flen*8;
	i->height = h->h = 1;
    }
    else
    {
	i->width  = h->w = prw;	// must be congruent to 8
	i->height = h->h = FIM_INT_FRAC(8*h->flen,h->w); // should pad
    }

    i->npages = 1;
    return h;
 oops:
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
bit1_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct bit1_state *h = (struct bit1_state *) data;
    unsigned int ll,y,x = 0;
    
	y  = line ;
	if(y==h->h-1)
	{
		ll = h->flen - h->w * (h->h-1) / 8;
	}
	else
		ll = h->w / 8;

	fseek(h->fp,0 + y * ll,SEEK_SET);

	for (x = 0; x < h->w; x+=8)
	{
		fim_byte_t c = fgetc(h->fp);
		*(dst++) = (c & 1 << 0)?255:0;
		*(dst++) = (c & 1 << 0)?255:0;
		*(dst++) = (c & 1 << 0)?255:0;
		*(dst++) = (c & 1 << 1)?255:0;
		*(dst++) = (c & 1 << 1)?255:0;
		*(dst++) = (c & 1 << 1)?255:0;
		*(dst++) = (c & 1 << 2)?255:0;
		*(dst++) = (c & 1 << 2)?255:0;
		*(dst++) = (c & 1 << 2)?255:0;
		*(dst++) = (c & 1 << 3)?255:0;
		*(dst++) = (c & 1 << 3)?255:0;
		*(dst++) = (c & 1 << 3)?255:0;
		*(dst++) = (c & 1 << 4)?255:0;
		*(dst++) = (c & 1 << 4)?255:0;
		*(dst++) = (c & 1 << 4)?255:0;
		*(dst++) = (c & 1 << 5)?255:0;
		*(dst++) = (c & 1 << 5)?255:0;
		*(dst++) = (c & 1 << 5)?255:0;
		*(dst++) = (c & 1 << 6)?255:0;
		*(dst++) = (c & 1 << 6)?255:0;
		*(dst++) = (c & 1 << 6)?255:0;
		*(dst++) = (c & 1 << 7)?255:0;
		*(dst++) = (c & 1 << 7)?255:0;
		*(dst++) = (c & 1 << 7)?255:0;
	}
//	if(y==h->h-1) fim_bzero(dst,h->w*8-8*x);
}

static void
bit1_done(void *data)
{
    struct bit1_state *h = (struct bit1_state *) data;

    fclose(h->fp);
    fim_free(h);
}

struct ida_loader bit1_loader = {
/*
 * 0000000: 7f45 4c46 0101 0100 0000 0000 0000 0000  .ELF............
 */
    /*magic:*/ "ELF",
    /*moff:*/  1,
    /*mlen:*/  3,
    /*name:*/  "Bit1",
    /*init:*/  bit1_init,
    /*read:*/  bit1_read,
    /*done:*/  bit1_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&bit1_loader);
}


}
#endif /* FIM_WANT_RAW_BITS_RENDERING */
