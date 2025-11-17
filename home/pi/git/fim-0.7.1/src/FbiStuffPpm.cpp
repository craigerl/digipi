/* $LastChangedDate: 2022-10-30 16:13:52 +0100 (Sun, 30 Oct 2022) $ */
/*
 FbiStuffPpm.cpp : fbi functions for PPM files, modified for fim

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



#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>

//#include "loader.h"
#include "fim.h"
#include "FbiStuffLoader.h"
#ifdef USE_X11
 #include "viewer.h"
#endif /* USE_X11 */

/* ---------------------------------------------------------------------- */
/* load                                                                   */

namespace fim
{

struct ppm_state {
    FILE          *infile;
    int           width,height;
    int Maxval;
    fim_byte_t *row;
};

static void*
pnm_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	 struct ida_image_info *i, int thumbnail)
{
    struct ppm_state *h;
    fim_char_t line[FIM_FBI_PPM_LINEBUFSIZE],*fr;

    h = (struct ppm_state*) fim_calloc(1,sizeof(*h));
    if(!h)return FIM_NULL;

    h->infile = fp;
    fr=fgets(line,sizeof(line),fp); /* Px */
    if(!fr)goto oops;
    fr=fgets(line,sizeof(line),fp); /* width height */
    if(!fr)goto oops;
    while ('#' == line[0])
    {
	fr=fgets(line,sizeof(line),fp); /* skip comments */
        if(!fr)goto oops;
    }
    sscanf(line,"%d %d",&h->width,&h->height);
    fr=fgets(line,sizeof(line),fp); /* ??? */
    sscanf(line,"%d",&h->Maxval);
    if(!fr)goto oops;
    if (0 == h->width || 0 == h->height)
	goto oops;
    i->width  = h->width;
    i->height = h->height;
    i->npages = 1;
    if( h->Maxval < 256 )
    	h->row = (fim_byte_t*)fim_malloc(h->width*3*1); // 1 byte  read per component
    else
    	h->row = (fim_byte_t*)fim_malloc(h->width*3*2); // 2 bytes read per component (but first taken)
    if(!h->row)goto oops;

    return h;

 oops:
    fim_fclose(fp);
    if(h->row)fim_free(h->row);
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
ppm_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct ppm_state *h = (struct ppm_state *) data;
    int fr;
    if( h->Maxval < 256 )
    	fr=fim_fread(dst,h->width,3,h->infile);
    else
    {
        fim_byte_t *src = h->row;
    	fr=fim_fread(src,h->width,6,h->infile);
    	for (int x = 0; x < h->width; x++)
	{
		dst[3*x+0]=src[2*(3*x+0)];
		dst[3*x+1]=src[2*(3*x+1)];
		dst[3*x+2]=src[2*(3*x+2)];
	}
    }

    if(fr){/* FIXME : there should be error handling */}
}

static void
pgm_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct ppm_state *h = (struct ppm_state *) data;
    fim_byte_t *src;
    int x,fr,inc=1;

    if( h->Maxval >= 256 )
	inc=2;
    fr=fim_fread(h->row,h->width,inc,h->infile);
    if(!fr){/* FIXME : there should be error handling */ return ; }
    src = h->row;
    for (x = 0; x < h->width; x++) {
	dst[0] = src[0];
	dst[1] = src[0];
	dst[2] = src[0];
	dst += 3;
	src += inc;
    }
}

static void
pnm_done(void *data)
{
    struct ppm_state *h = (struct ppm_state *) data;

    fim_fclose(h->infile);
    fim_free(h->row);
    fim_free(h);
}

struct ida_loader ppm_loader = {
    /*magic:*/ "P6",
    /*moff:*/  0,
    /*mlen:*/  2,
    /*name:*/  "ppm",
    /*init:*/  pnm_init,
    /*read:*/  ppm_read,
    /*done:*/  pnm_done,
};

struct ida_loader pgm_loader = {
    /*magic:*/ "P5",
    /*moff:*/  0,
    /*mlen:*/  2,
    /*name:*/  "pgm",
    /*init:*/  pnm_init,
    /*read:*/  pgm_read,
    /*done:*/  pnm_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&ppm_loader);
    fim_load_register(&pgm_loader);
}

#ifdef USE_X11
/* ---------------------------------------------------------------------- */
/* save                                                                   */

static int
ppm_write(FILE *fp, struct ida_image *img)
{
    fprintf(fp,"P6\n"
	    "# written by ida " VERSION "\n"
	    "# http://bytesex.org/ida/\n"
	    "%d %d\n255\n",
            img->i.width,img->i.height);
    fwrite(img->data, img->i.height, 3*img->i.width, fp);
    return 0;
}

static struct ida_writer ppm_writer = {
    /*label:*/  "PPM",
    /*ext:*/    { "ppm", FIM_NULL},
    /*write:*/  ppm_write,
};

static void fim__init init_wr(void)
{
    fim_write_register(&ppm_writer);
}
#endif /* USE_X11 */

}

