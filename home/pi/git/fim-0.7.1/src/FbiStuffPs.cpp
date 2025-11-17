/* $LastChangedDate: 2022-11-17 18:00:39 +0100 (Thu, 17 Nov 2022) $ */
/*
 FbiStuffPs.cpp : fim functions for decoding PS files

 (c) 2008-2022 Michele Martone
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

#ifdef HAVE_LIBSPECTRE

extern "C"
{
// we require C linkage for these symbols
#include <libspectre/spectre.h>
}

/*								*/

namespace fim
{
extern CommandConsole cc;

/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct ps_state_t {
	int row_stride;    /* physical row width in output buffer */
	fim_byte_t * first_row_dst;
	int w,h;
	SpectreDocument * sd;
	SpectrePage * sp;
	SpectreRenderContext * src;
	SpectreStatus ss;
};


/* ---------------------------------------------------------------------- */
#define FIM_SPECTRE_DEFAULT_DPI 72
static void*
ps_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	  struct ida_image_info *i, int thumbnail)
{
	const fim_int prdv=cc.getIntVariable(FIM_VID_PREFERRED_RENDERING_DPI);
	const fim_int prd=prdv<1?FIM_RENDERING_DPI:prdv;
	const double scale = 1.0* (((double)prd)/((double)FIM_SPECTRE_DEFAULT_DPI)) ;
	const double rcscale = scale;
	struct ps_state_t * ds=FIM_NULL;

	if(std::string(filename)==FIM_STDIN_IMAGE_NAME){std::cerr<<"sorry, stdin multipage file reading is not supported\n";return FIM_NULL;}	/* a drivers's problem */ 
	if(fp) fclose(fp);

	ds = (struct ps_state_t*)fim_calloc(1,sizeof(struct ps_state_t));

	if(!ds)
		return FIM_NULL;

    	ds->first_row_dst = FIM_NULL;
	ds->sd = FIM_NULL;
	ds->sp = FIM_NULL;
	ds->src = FIM_NULL;
	ds->ss = SPECTRE_STATUS_SUCCESS;

	ds->sd = spectre_document_new();
	if(!ds->sd)
		goto err;

	spectre_document_load(ds->sd,filename);

	ds->ss = spectre_document_status(ds->sd);
	if(ds->ss != SPECTRE_STATUS_SUCCESS)
		goto err;

	ds->src = spectre_render_context_new();
	if(!ds->src)
		goto err;

	i->dpi    = FIM_SPECTRE_DEFAULT_DPI; /* FIXME */

	spectre_render_context_set_scale(ds->src,scale,scale);
	spectre_render_context_set_rotation(ds->src,0);
	spectre_render_context_set_resolution(ds->src,i->dpi,i->dpi);

	i->npages = spectre_document_get_n_pages(ds->sd);
	if(page>=i->npages || page<0)goto err;

	ds->sp = spectre_document_get_page(ds->sd,page);/* pages, from 0 */
	if(!ds->sp)
		goto err;
	ds->ss = spectre_page_status(ds->sp);
	if(ds->ss != SPECTRE_STATUS_SUCCESS)
		goto err;

	spectre_page_get_size(ds->sp, (int*)(&i->width), (int*)(&i->height));
//	spectre_render_context_get_page_size(ds->src, (int*)(&i->width), (int*)(&i->height));
//	spectre_document_get_page_size(ds->sd, (int*)(&i->width), (int*)(&i->height));

	i->width  *= scale;
	i->height *= scale;


	spectre_render_context_set_page_size(ds->src, (int)(i->width), (int)(i->height));
	spectre_render_context_set_scale(ds->src,rcscale,rcscale);

	if(i->width<1 || i->height<1)
		goto err;

	ds->w=i->width;
	ds->h=i->height;

	return ds;

err:

	if(ds->sd )spectre_document_free(ds->sd);
	if(ds->sp )spectre_page_free(ds->sp);
	if(ds->src)spectre_render_context_free(ds->src);
	if(ds)fim_free(ds);
	return FIM_NULL;
}

static void
ps_read(fim_byte_t *dst, unsigned int line, void *data)
{
    	struct ps_state_t *ds = (struct ps_state_t*)data;
	if(!ds)return;

    	if(ds->first_row_dst == FIM_NULL)
    		ds->first_row_dst = dst;
	else return;

	fim_byte_t       *page_data=FIM_NULL;

	//render in RGB32 format
	//spectre_page_render(ds->sp,ds->src,&page_data,&ds->row_stride);
	spectre_page_render_slice(ds->sp,ds->src,0,0,ds->w,ds->h,&page_data,&ds->row_stride);

	ds->ss = spectre_page_status(ds->sp);
	if(ds->ss != SPECTRE_STATUS_SUCCESS)
		return;

	int i,j;
	for(i=0;i<ds->h;++i)
		for(j=0;j<ds->w;++j)
		{
#if 0
			dst[ds->w*i*3+3*j+0]=page_data[ds->row_stride*i+4*j+0];
			dst[ds->w*i*3+3*j+1]=page_data[ds->row_stride*i+4*j+1];
			dst[ds->w*i*3+3*j+2]=page_data[ds->row_stride*i+4*j+2];
#else
			dst[ds->w*i*3+3*j+2]=page_data[ds->row_stride*i+4*j+0];
			dst[ds->w*i*3+3*j+1]=page_data[ds->row_stride*i+4*j+1];
			dst[ds->w*i*3+3*j+0]=page_data[ds->row_stride*i+4*j+2];
#endif
		}
	free(page_data);
}

static void
ps_done(void *data)
{
    	struct ps_state_t *ds = (struct ps_state_t*)data;
	if(!ds) return;

	if(ds->sd )spectre_document_free(ds->sd);
	if(ds->sp )spectre_page_free(ds->sp);
	if(ds->src)spectre_render_context_free(ds->src);

	fim_free(ds);
}

/*
0000000: 2521 5053 2d41 646f 6265 2d33 2e30 0a25  %!PS-Adobe-3.0.%
*/
static struct ida_loader ps_loader = {
    /*magic:*/ "%!PS-",// FI/*XME :*/ are sure this is enough ?
    /*moff:*/  0,
    /*mlen:*/  5,
    /*name:*/  "libspectre",
    /*init:*/  ps_init,
    /*read:*/  ps_read,
    /*done:*/  ps_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&ps_loader);
}

}
#endif // ifdef HAVE_LIBSPECTRE
