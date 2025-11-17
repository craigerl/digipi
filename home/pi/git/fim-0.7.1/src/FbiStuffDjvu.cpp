/* $LastChangedDate: 2024-04-16 14:31:00 +0200 (Tue, 16 Apr 2024) $ */
/*
 FbiStuffDjvu.cpp : fim functions for decoding DJVU files

 (c) 2008-2024 Michele Martone
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

/*
 * this code should be fairly correct, although unfinished
 * */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "FbiStuff.h"
#include "FbiStuffLoader.h"

#ifdef HAVE_LIBDJVU

extern "C"
{
#include <libdjvu/ddjvuapi.h>
}

/*								*/

namespace fim
{
typedef char fim_libdjvu_char;
extern CommandConsole cc;
/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct djvu_state_t {
    DDJVUAPI ddjvu_context_t  * dc;
    DDJVUAPI ddjvu_document_t * dd;
    ddjvu_page_t *dp;
    ddjvu_page_rotation_t rotation;
    ddjvu_rect_t rrect;
    ddjvu_rect_t prect;
    ddjvu_format_t * pf;
    int row_stride;    /* physical row width in output buffer */

    fim_byte_t * first_row_dst;
};

/* ---------------------------------------------------------------------- */
/* djvu loader                                                            */

   /* straight out from the DJVU API doc : */
   void handle_ddjvu_messages(ddjvu_context_t *ctx, int wait)
   {
     const ddjvu_message_t *msg;
     if (wait)
       ddjvu_message_wait(ctx);
     while ((msg = ddjvu_message_peek(ctx)))
     {
       switch(msg->m_any.tag)
       {
       case DDJVU_ERROR:      /*....*/ ; break;
       case DDJVU_INFO:       /*....*/ ; break;
       case DDJVU_NEWSTREAM:  /*....*/ ; break;
    //   ....
       default: break;
       }
       ddjvu_message_pop(ctx);
     }
   }

static void*
djvu_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	  struct ida_image_info *i, int thumbnail)
{
	struct djvu_state_t * ds=FIM_NULL;
        // static unsigned int masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
	const fim_int prdv=cc.getIntVariable(FIM_VID_PREFERRED_RENDERING_DPI);
	const fim_int prd=prdv<1?FIM_RENDERING_DPI:prdv;

	if(filename==std::string(FIM_STDIN_IMAGE_NAME))
	{
		std::cerr<<"sorry, stdin multipage file reading is not supported\n";
		goto ret;
	}	/* a drivers' problem */ 

	if(fp) fclose(fp);

	ds = (struct djvu_state_t*)fim_calloc(1,sizeof(struct djvu_state_t));
	if(!ds) return FIM_NULL;
    	ds->first_row_dst = FIM_NULL;

        ds->dc = ddjvu_context_create("fim");
	if(!ds->dc)goto err;
	ds->dd = ddjvu_document_create_by_filename(ds->dc, filename, 0);
	if(!ds->dd)goto err;

	handle_ddjvu_messages(ds->dc,0x1/*0x0*/);
	i->npages = ddjvu_document_get_pagenum(ds->dd);
	if(page>=i->npages || page<0)goto err;
        ds->dp = ddjvu_page_create_by_pageno (ds->dd, page);/* pages, from 0 */
        if(!ds->dp) goto err;
        while (!ddjvu_page_decoding_done (ds->dp)){/* we just kill time (FIXME : inefficient) */}

        ds->prect.w = ddjvu_page_get_width  (ds->dp) ;
	ds->prect.h = ddjvu_page_get_height (ds->dp) ;
#if 1
	ddjvu_pageinfo_t pi;
	ddjvu_document_get_pageinfo(ds->dd,page,&pi);
        ds->prect.w = ((fim_scale_t) (ds->prect.w))* FIM_INT_SCALE_FRAC(prd,static_cast<fim_scale_t>(pi.dpi));
        ds->prect.h = ((fim_scale_t) (ds->prect.h))* FIM_INT_SCALE_FRAC(prd,static_cast<fim_scale_t>(pi.dpi));
	pi.dpi=prd;
#endif

        if(ds->prect.w<1)goto err;
        if(ds->prect.h<1)goto err;

        ds->rotation = DDJVU_ROTATE_0;
        ddjvu_page_set_rotation (ds->dp, ds->rotation);

        ds->prect.x = 0;
        ds->prect.y = 0;

        ds->rrect = ds->prect;

        ds->row_stride=ds->prect.w  *  3;
	
	i->width  = ds->prect.w;
	i->height = ds->prect.h;
	i->dpi    = pi.dpi;

//        ds->pf = ddjvu_format_create (DDJVU_FORMAT_RGBMASK32, 4, masks);
	ds->pf = ddjvu_format_create (DDJVU_FORMAT_RGB24, 0, 0);
	ddjvu_format_set_row_order (ds->pf, 1);
        if(!ds->pf) goto err;

	return ds;
err:
	if(ds->dp)ddjvu_page_release(ds->dp);
	if(ds->dd)ddjvu_document_release(ds->dd);
	if(ds->dc)ddjvu_context_release(ds->dc);
	if(ds->pf)ddjvu_format_release(ds->pf);
ret:
	return FIM_NULL;
}

static void
djvu_read(fim_byte_t *dst, unsigned int line, void *data)
{
    	struct djvu_state_t *ds = (struct djvu_state_t*)data;

	if(!ds)return;

    	if(ds->first_row_dst == FIM_NULL)
    		ds->first_row_dst = dst;
	else return;

        const int rs=ddjvu_page_render (ds->dp, DDJVU_RENDER_COLOR,
                           & (ds->prect),
                           & (ds->rrect),
                           ds->pf,
                           ds->row_stride,
                           (fim_libdjvu_char*)dst);
	if(rs)
	{
	}
	else
	{
		std::cerr << " error rendering current DJVU document page: perhaps it's damaged?\n";
		/* error handling can be improved ... */
	}
        return ;
}

static void
djvu_done(void *data)
{
    	struct djvu_state_t *ds = (struct djvu_state_t*)data;

	if(ds->dp)ddjvu_page_release(ds->dp);
	if(ds->dd)ddjvu_document_release(ds->dd);
	if(ds->dc)ddjvu_context_release(ds->dc);
	if(ds->pf)ddjvu_format_release(ds->pf);

	fim_free(ds);
}

/*
0000000: 4154 2654 464f 524d 0070 ca79 444a 564d  AT&TFORM.p.yDJVM
0000010: 4449 524d 0000 1465 8102 b900 0014 7e00  DIRM...e......~.
*/
static struct ida_loader djvu_loader = {

/*    magic: "DJV",
    moff:  12,
    mlen:  3,*/
 
    /*magic:*/ "AT&TFORM",// FIXME :*/ are sure this is enough ?
    /*moff:*/  0,
    /*mlen:*/  8,
    /*name:*/  "libdjvu",
    /*init:*/  djvu_init,
    /*read:*/  djvu_read,
    /*done:*/  djvu_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&djvu_loader);
}

}
#endif // ifdef HAVE_LIBDJVU
