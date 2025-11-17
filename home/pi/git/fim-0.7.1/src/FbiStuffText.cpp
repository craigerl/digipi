/* $LastChangedDate: 2013-07-04 21:56:00 +0200 (Thu, 04 Jul 2013) $ */
/*
 FbiStuffBitText.cpp : fbi functions for rendering image bytes as text

 (c) 2013-2022 Michele Martone

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
 * this is basically toy code, so enjoy!
 * */


#include "fim.h"

#if FIM_WANT_TEXT_RENDERING

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

struct text_state {
    FILE *fp;
    uint32 w;
    uint32 h;
    uint32 flen;
    struct fs_font *f_;
    fim_int fw,fh;
    long maxc;
    fim_int cw,ch;
};

static void*
text_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	 struct ida_image_info *i, int thumbnail)
{
    struct text_state *h=FIM_NULL;
    long ftellr;
    const fim_int prwv=cc.getIntVariable(FIM_VID_PREFERRED_RENDERING_WIDTH);
    const fim_int prw=prwv<1?FIM_BITRENDERING_DEF_WIDTH:prwv;
    
    h = (struct text_state *)fim_calloc(1,sizeof(*h));
    if(!h)
	    goto oops;

    FontServer::fb_text_init1(FIM_NULL,&(h->f_));	// FIXME : move this outta here
    if(!h->f_)
	    goto oops;
    h->fp = fp;
    h->fw = h->f_->width;
    h->fh = h->f_->height;

    h->cw = prw / h->fw;

    if(fseek(fp,0,SEEK_END)!=0)
	    goto oops;
    ftellr=ftell(fp);
    if((ftellr)==-1)
	    goto oops;
    if(ftellr==0)
    	ftellr=1; /* (artificial) promotion to 1x1 */
    h->maxc = (128*1024*1024)/(3*h->fw*h->fh);

    ftellr = FIM_MIN(h->maxc,ftellr);
    /* FIXME: shall make max allocation limit configurable :) */
    h->cw = FIM_MIN(h->cw, ftellr);

    h->ch = ( ( ftellr + h->cw - 1 ) / h->cw );

    i->npages = 1;
    i->width  = h->w = /* prw */ h->fw * h->cw;
    i->height = h->h = h->ch * h->fh;
    return h;
 oops:

    if(h && h->f_)fim_free_fs_font(h->f_);
    if(h)fim_free(h);
    return FIM_NULL;
}

static void fs_render_fb(fim_byte_t *ptr, int pitch, FSXCharInfo *charInfo, int fs_bpp_, fim_byte_t *data)
{
/* FIXME: shall produce generic version of this code and move somewhere else */

/* 
 * These preprocessor macros should serve *only* for font handling purposes.
 * */
#define BIT_ORDER       BitmapFormatBitOrderMSB
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif /* BYTE_ORDER */
#define BYTE_ORDER      BitmapFormatByteOrderMSB
#define SCANLINE_UNIT   BitmapFormatScanlineUnit8
#define SCANLINE_PAD    BitmapFormatScanlinePad8
#define EXTENTS         BitmapFormatImageRectMin

#define SCANLINE_PAD_BYTES 1
#define GLWIDTHBYTESPADDED(bits, nBytes)                                    \
        ((nBytes) == 1 ? (((bits)  +  7) >> 3)          /* pad to 1 byte  */\
        :(nBytes) == 2 ? ((((bits) + 15) >> 3) & ~1)    /* pad to 2 bytes */\
        :(nBytes) == 4 ? ((((bits) + 31) >> 3) & ~3)    /* pad to 4 bytes */\
        :(nBytes) == 8 ? ((((bits) + 63) >> 3) & ~7)    /* pad to 8 bytes */\
        : 0)

    int row,bit,x;
    const int bpr = GLWIDTHBYTESPADDED((charInfo->right - charInfo->left),
			     SCANLINE_PAD_BYTES);
    for (row = 0; row < (charInfo->ascent + charInfo->descent); row++) {
	for (x = 0, bit = 0; bit < (charInfo->right - charInfo->left); bit++) {
	    if (data[bit>>3] & fs_masktab[bit&7])
		// WARNING !
		// fs_setpixel(ptr+x,fs_white_);
		ptr[x+0]=0xFF,
		ptr[x+1]=0xFF,
		ptr[x+2]=0xFF;
	    x += fs_bpp_;
	}
	data += bpr;
	ptr += pitch;
    }

#undef BIT_ORDER
#undef BYTE_ORDER
#undef SCANLINE_UNIT
#undef SCANLINE_PAD
#undef EXTENTS
#undef SCANLINE_PAD_BYTES
#undef GLWIDTHBYTESPADDED
}



static void
text_read(fim_byte_t *dst, unsigned int line, void *data)
{
	struct text_state *h = (struct text_state *) data;
	int fr,cc=0;

    	if(line==0)
	{
		fim_bzero(dst,3*h->h*h->w);
		fseek(h->fp, 0,SEEK_SET );
		while( ( fr = fgetc(h->fp) ) != EOF && cc < h->maxc  )
		{
			fim_byte_t *dstp=dst+3*( (cc/h->cw)*h->fh*h->w + (cc%h->cw)*h->fw);

			if(!isprint(fr))
				fr=FIM_SYM_UNKNOWN_CHAR;
			fs_render_fb(dstp, h->w*3 , h->f_->eindex[fr], 3, h->f_->gindex[fr]);
			cc++;
		}
	}
}

static void
text_done(void *data)
{
    struct text_state *h = (struct text_state *) data;

    fclose(h->fp);
    if(h && h->f_)fim_free_fs_font(h->f_);
    fim_free(h);
}

struct ida_loader text_loader = {
/*
 not a filetype-specific decoder
 */
    /*magic:*/ "",
    /*moff:*/  0,
    /*mlen:*/  0,
    /*name:*/  "Text",
    /*init:*/  text_init,
    /*read:*/  text_read,
    /*done:*/  text_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&text_loader);
}


}
#endif /* FIM_WANT_TEXT_RENDERING */
