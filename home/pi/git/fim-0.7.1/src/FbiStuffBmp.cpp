/* $LastChangedDate: 2022-10-30 16:13:52 +0100 (Sun, 30 Oct 2022) $ */
/*
 FbiStuffBmp.cpp : fbi functions for BMP files, modified for fim

 (c) 2008-2022 Michele Martone
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

#if FIM_WITH_BMP

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#ifdef HAVE_ENDIAN_H
 #include <endian.h>
#endif /* HAVE_ENDIAN_H */
#include <algorithm> // fill_n

//#include "loader.h"

namespace fim
{


/* ---------------------------------------------------------------------- */

typedef unsigned int   uint32;
typedef unsigned short uint16;

/* bitmap files are little endian */
#if BYTE_ORDER == LITTLE_ENDIAN
 #define le16_to_cpu(x) (x)
 #define le32_to_cpu(x) (x)
#elif BYTE_ORDER == BIG_ENDIAN
 #define le16_to_cpu(x) (((x>>8) & 0x00ff) |\
                         ((x<<8) & 0xff00))
 #define le32_to_cpu(x) (((x>>24) & 0x000000ff) |\
                         ((x>>8)  & 0x0000ff00) |\
                         ((x<<8)  & 0x00ff0000) |\
                         ((x<<24) & 0xff000000))
#else /* BYTE_ORDER == LITTLE_ENDIAN */
 #error "Oops: unknown byte order"
#endif /* BYTE_ORDER == LITTLE_ENDIAN */

/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct bmp_hdr {
    uint32 foobar;

    uint32 size;              /* == BitMapInfoHeader */
    uint32 width;
    uint32 height;
    uint16 planes;
    uint16 bit_cnt;
    fim_char_t   compression[4];
    uint32 image_size;
    uint32 xpels_meter;
    uint32 ypels_meter;
    uint32 num_colors;        /* used colors */
    uint32 imp_colors;        /* important colors */
    /* may be more for some codecs */
};

struct bmp_cmap {
    fim_byte_t blue;
    fim_byte_t green;
    fim_byte_t red;
    fim_byte_t unused;
};

struct bmp_state {
    struct bmp_hdr  hdr;
    struct bmp_cmap cmap[256];
    FILE *fp;
};

static void*
bmp_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	 struct ida_image_info *i, int thumbnail)
{
    struct bmp_state *h;
    int fr;

    h = (struct bmp_state *)fim_calloc(1,sizeof(*h));
    if(!h)goto oops;

    h->fp = fp;

    fim_fseek(fp,10,SEEK_SET);
    fr=fim_fread(&h->hdr,sizeof(struct bmp_hdr),1,fp);
    if(!fr)goto oops;

#if BYTE_ORDER == BIG_ENDIAN
    h->hdr.foobar      = le32_to_cpu(h->hdr.foobar);
    h->hdr.size        = le32_to_cpu(h->hdr.size);
    h->hdr.width       = le32_to_cpu(h->hdr.width);
    h->hdr.height      = le32_to_cpu(h->hdr.height);
    
    h->hdr.planes      = le16_to_cpu(h->hdr.planes);
    h->hdr.bit_cnt     = le16_to_cpu(h->hdr.bit_cnt);
    
    h->hdr.image_size  = le32_to_cpu(h->hdr.image_size);
    h->hdr.xpels_meter = le32_to_cpu(h->hdr.xpels_meter);
    h->hdr.ypels_meter = le32_to_cpu(h->hdr.ypels_meter);
    h->hdr.num_colors  = le32_to_cpu(h->hdr.num_colors);
    h->hdr.imp_colors  = le32_to_cpu(h->hdr.imp_colors);
#endif /* BYTE_ORDER == BIG_ENDIAN */

    if (FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("bmp: hdr=%d size=%dx%d planes=%d"
		" bits=%d size=%d res=%dx%d colors=%d/%d | %d\n",
		h->hdr.size,h->hdr.width,h->hdr.height,
		h->hdr.planes,h->hdr.bit_cnt,h->hdr.image_size,
		h->hdr.xpels_meter,h->hdr.ypels_meter,
		h->hdr.num_colors,h->hdr.imp_colors,h->hdr.foobar);
    if (h->hdr.bit_cnt != 1  &&
	h->hdr.bit_cnt != 4  &&
	h->hdr.bit_cnt != 8  &&
	h->hdr.bit_cnt != 24) {
	if(FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("bmp: can't handle depth [%d]\n",h->hdr.bit_cnt);
	goto oops;
    }
    if (h->hdr.compression[0] || h->hdr.compression[1] ||
	h->hdr.compression[2] || h->hdr.compression[3]) {
	if(FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("bmp: can't handle compressed bitmaps [%c%c%c%c]\n",
		h->hdr.compression[0],
		h->hdr.compression[1],
		h->hdr.compression[2],
		h->hdr.compression[3]);
	goto oops;
    }

    if (0 == h->hdr.num_colors && h->hdr.bit_cnt <= 8)
	h->hdr.num_colors = (1 << h->hdr.bit_cnt);
    if (h->hdr.num_colors > 256)
	h->hdr.num_colors = 256;
    if (h->hdr.num_colors) {
	fim_fseek(fp,14+h->hdr.size,SEEK_SET);
	fr=fim_fread(&h->cmap,sizeof(struct bmp_cmap),h->hdr.num_colors,fp);
        if(!fr)goto oops;
    }
    
    i->width  = h->hdr.width;
    i->height = h->hdr.height;
    if (h->hdr.xpels_meter)
	i->dpi = res_m_to_inch(h->hdr.xpels_meter);
    i->npages = 1;
    return h;

 oops:
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
bmp_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct bmp_state *h = (struct bmp_state *) data;
    unsigned int ll,y,x,pixel,byte = 0;
    
    ll = (((h->hdr.width * h->hdr.bit_cnt + 31) & ~0x1f) >> 3);
    y  = h->hdr.height - line - 1;
    fim_fseek(h->fp,h->hdr.foobar + y * ll,SEEK_SET);

    switch (h->hdr.bit_cnt) {
    case 1:
	for (x = 0; x < h->hdr.width; x++) {
	    if (0 == (x & 0x07))
		byte = fim_fgetc(h->fp);
	    pixel = byte & (0x80 >> (x & 0x07)) ? 1 : 0;
	    *(dst++) = h->cmap[pixel].red;
	    *(dst++) = h->cmap[pixel].green;
	    *(dst++) = h->cmap[pixel].blue;
	}
	break;
    case 4:
	for (x = 0; x < h->hdr.width; x++) {
	    if (x & 1) {
		pixel = byte & 0xf;
	    } else {
		byte = fim_fgetc(h->fp);
		pixel = byte >> 4;
	    }
	    *(dst++) = h->cmap[pixel].red;
	    *(dst++) = h->cmap[pixel].green;
	    *(dst++) = h->cmap[pixel].blue;
	}
	break;
    case 8:
	for (x = 0; x < h->hdr.width; x++) {
	    pixel = fim_fgetc(h->fp);
	    *(dst++) = h->cmap[pixel].red;
	    *(dst++) = h->cmap[pixel].green;
	    *(dst++) = h->cmap[pixel].blue;
	}
	break;
    case 24:
	for (x = 0; x < h->hdr.width; x++) {
	    dst[2] = fim_fgetc(h->fp);
	    dst[1] = fim_fgetc(h->fp);
	    dst[0] = fim_fgetc(h->fp);
	    dst += 3;
	}
	break;
    default:
	std::fill_n(dst,h->hdr.width*3,128);
	break;
    }
}

static void
bmp_done(void *data)
{
    struct bmp_state *h = (struct bmp_state *) data;

    fim_fclose(h->fp);
    fim_free(h);
}

static struct ida_loader bmp_loader = {
    /*magic:*/ "BM",
    /*moff:*/  0,
    /*mlen:*/  2,
    /*name:*/  "bmp",
    /*init:*/  bmp_init,
    /*read:*/  bmp_read,
    /*done:*/  bmp_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&bmp_loader);
}


}
#endif /* FIM_WITH_BMP */

