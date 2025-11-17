/* $LastChangedDate: 2024-04-02 00:13:06 +0200 (Tue, 02 Apr 2024) $ */
/*
 FbiStuff.cpp : Misc fbi functions, modified for fim

 (c) 2008-2024 Michele Martone
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
 * Much of this file derives originally from fbi and has not been properly refactored.
 * */

#include "fim.h"
#include "fim_plugin.h"
#include "common.h"

#include <cstdarg>	/* va_list, va_arg, va_start, va_end */
#include <cstdio>	/* fdopen, tmpfile */
#include <unistd.h>	/* execlp (popen is dangerous) */
#include <cstdlib>	/* mkstemp */
#include <cstdio>	/* tempnam */
#include <cmath>
#if HAVE_SYS_WAIT_H
#include <sys/wait.h>  /* waitpid */
#endif /* HAVE_SYS_WAIT_H */
#include <cstring>	/* memmem */
#include <cstdarg>	/* va_start, va_end, ... */
//#include <experimental/filesystem> // std::experimental::filesystem::temp_directory_path
#if FIM_WITH_ARCHIVE
#include <archive.h>
#include <archive_entry.h>
//extern "C" { const char * archive_entry_pathname(struct archive_entry *); }
#endif /* FIM_WITH_ARCHIVE */

#define FIM_HAVE_FULL_PROBING_LOADER 0
#ifdef HAVE_FMEMOPEN
//#define FIM_SHALL_BUFFER_STDIN (HAVE_FMEMOPEN && FIM_READ_STDIN_IMAGE)	/* FIXME: new */
#define FIM_SHALL_BUFFER_STDIN 0 /* FIXME: before activating this, we shall clean up some code first (e.g.: cc.fpush(), which is backed up by a temporary file) */
#else /* HAVE_FMEMOPEN */
#define FIM_SHALL_BUFFER_STDIN 0
#endif /* HAVE_FMEMOPEN */

#define FIM_WANTS_SLOW_RESIZE 1
#define FIM_WVMM 0 /* want verbose mip maps (for FIM_WANT_MIPMAPS) */
#define FIM_WFMM 1 /* want faster mip maps */

#define FIM_FBISTUFF_INSPECT 0
#if FIM_FBISTUFF_INSPECT
#define FIM_PR(X) printf("FBISTUFF:%c:%20s\n",X,__func__);
#else /* FIM_FBISTUFF_INSPECT */
#define FIM_PR(X) 
#endif /* FIM_FBISTUFF_INSPECT */

#define FIM_DD_DEBUG() FbiStuff::want_fbi_style_debug()
#if FIM_WANT_CONSOLE_SWITCH_WHILE_LOADING
#define FIM_FB_SWITCH_IF_NEEDED() cc.switch_if_needed() 
#else  /* FIM_WANT_CONSOLE_SWITCH_WHILE_LOADING */
#define FIM_FB_SWITCH_IF_NEEDED() /* */
#endif /* FIM_WANT_CONSOLE_SWITCH_WHILE_LOADING */
#define FIM_MSG_WAIT_PIPING(CP) "please wait while piping through" " '" CP "'..."
#define FIM_MSG_FAILED_PIPE(CP) "piping through" " '" CP "' failed."
#define FIM_TEXTURED_ROTATION 0

namespace fim
{

extern CommandConsole cc;


/* ----------------------------------------------------------------------- */
#if FIM_WANT_MIPMAPS
static fim_err_t mipmap_compute(const fim_coo_t w, const fim_coo_t h, const int hw, const int hh, const fim_byte_t *FIM_RSTRCT src, fim_byte_t * FIM_RSTRCT dst, enum fim_mmo_t mmo)
{
	fim_err_t errval = FIM_ERR_GENERIC;
	/* we compute a quarter of an image, half-sided */

	if(hw<1||hh<1)
	{
		goto err;
	}
#if FIM_WFMM
	/* 'internal' version: faster, fewer writes */
    	if(mmo == FIM_MMO_FASTER)
	for(fim_int hr=0;hr<hh;++hr)
	for(fim_int hc=0;hc<hw;++hc)
	for(fim_int k=0;k<3;++k)
		dst[3*(hr*hw+hc)+k] = src[3*(((2*hr+0)*w+2*hc+0))+k];
	else
	for(fim_int hr=0;hr<hh;++hr)
	for(fim_int hc=0;hc<hw;++hc)
	for(fim_int k=0;k<3;++k)
	{
		fim_int dstv = 
			src[3*(((2*hr+0)*w+2*hc+0))+k]+
			src[3*(((2*hr+0)*w+2*hc+1))+k]+
			src[3*(((2*hr+1)*w+2*hc+0))+k]+
			src[3*(((2*hr+1)*w+2*hc+1))+k]+
			0;
		dst[3*(hr*hw+hc)+k] = fim_byte_t(dstv/4);
	}
#else
	/* 'external' version: slower, more writes */
	for(fim_int r=0,hr=0;r<2*(h/2);++r,hr=r/2)
	for(fim_int c=0,hc=0;c<2*(w/2);++c,hc=c/2)
	for(fim_int k=0;k<3;++k)
			dst[3*(hr*hw+hc)+k]+=src[3*(r*w+c)+k]/4;
#endif
	errval = FIM_ERR_NO_ERROR; 
err:
	return errval;
}

fim_err_t FbiStuff::fim_mipmaps_compute(const struct ida_image *src, fim_mipmap_t * mmp 
	FIM_DEPRECATED("In mipmap implementation shall use std::copy instead of pointers and memcpy."))
{
	/* 
	   Computation of mipmaps.
	   Mipmaps are images dimensioned (on both sides) as fractions of the original: 1/2, 1/4, ...
	   When the user requests a scaled-down image, we can downscale the nearest mipmap, which is faster than using the original.
	   This function was not present in Fbi.
	*/
	fim_mipmap_t mm; /* mipmap structure */
	int w, h, d;
	int mmidx = 0; /* mipmap index */
	fim_fms_t t0;

	if(!src)
	{
		goto err;
	}
	w = src->i.width, h = src->i.height; /* the original */
	mm.nmm=0;
	mm.mmo=mmp->mmo;
	mm.mmoffs[mm.nmm]=0;
	if(FIM_WVMM) std::cout << fbi_img_pixel_bytes(src) << " bytes are needed for the original image\n";
	for(d=2;w>=d && h>=d && mm.nmm<=FIM_MAX_MIPMAPS ;d*=2)
	{
		mm.mmw[mm.nmm]=w/d;
		mm.mmh[mm.nmm]=h/d;
		mm.mmsize[mm.nmm]=(w/d)*(h/d)*3;
		mm.mmb+=mm.mmsize[mm.nmm];
		mm.mmoffs[mm.nmm+1]=mm.mmb;
		++mm.nmm;
	}
	if(FIM_WVMM) std::cout << mm.nmm << " mipmaps can be computed\n";
	if(FIM_WVMM) std::cout << mm.mmb << " bytes are needed for the mipmaps\n";
	if(mm.nmm<1)
	{
		goto err;
	}
	mm.mdp=(fim_byte_t*)fim_calloc(1,mm.mmb);
	if(!mm.mdp)
	{
		goto err;
	}

	if(mm.nmm)
		t0 = getmilliseconds(),
		mipmap_compute(w,h,w/2,h/2,src->data,mm.mdp+mm.mmoffs[0],mm.mmo);
	for(mmidx=1,d=2;mmidx<mm.nmm;++mmidx,d*=2)
	{
		if(FIM_WVMM) std::cout << w/d << " " << h/d <<  " at " << mm.mmoffs[mmidx-1] << " ... " << mm.mmoffs[mmidx] << " : "<< mm.mmsize[mmidx] << "\n";
		mipmap_compute(w/d,h/d,w/(2*d),h/(2*d),mm.mdp+mm.mmoffs[mmidx-1],mm.mdp+mm.mmoffs[mmidx],mm.mmo);
	}
	if(FIM_WVMM) std::cout << __FUNCTION__ << " took " << (getmilliseconds() - t0) << " ms\n";
    	memcpy((void*)mmp,(void*)&mm,sizeof(mm));
	mm.mdp = FIM_NULL; // this is to avoid mm's destructor to free(mm.mdp)
	return FIM_ERR_NO_ERROR; 
err:
	return FIM_ERR_GENERIC;
}
#endif /* FIM_WANT_MIPMAPS */
/* ----------------------------------------------------------------------- */

// filter.c

/* ----------------------------------------------------------------------- */

#if FIM_WANT_OBSOLETE
static void
op_grayscale(const struct ida_image *src, struct ida_rect *rect,
	     fim_byte_t *dst, int line, void *data)
{
    fim_byte_t *scanline;
    int i,g;

    scanline = src->data + line * src->i.width * 3;
    memcpy(dst,scanline,src->i.width * 3);
    if (line < rect->y1 || line >= rect->y2)
	return;
    dst      += 3*rect->x1;
    scanline += 3*rect->x1;
    for (i = rect->x1; i < rect->x2; i++) {
	g = (scanline[0]*30 + scanline[1]*59+scanline[2]*11)/100;
	dst[0] = g;
	dst[1] = g;
	dst[2] = g;
	scanline += 3;
	dst += 3;
    }
}

/* ----------------------------------------------------------------------- */

struct op_3x3_handle {
    struct op_3x3_parm filter;
    int *linebuf;
};

static void*
op_3x3_init(const struct ida_image *src, struct ida_rect *rect,
	    struct ida_image_info *i, void *parm)
{
    struct op_3x3_parm *args = (struct op_3x3_parm*)parm;
    struct op_3x3_handle *h;

    h = (struct op_3x3_handle*)fim_malloc(sizeof(*h));
    if(!h)goto oops;
    memcpy(&h->filter,args,sizeof(*args));
    h->linebuf = (int*)fim_malloc(sizeof(int)*3*(src->i.width));
    if(!h->linebuf)goto oops;

    *i = src->i;
    return h;
oops:
    if(h && h->linebuf)fim_free(h->linebuf);
    if(h)fim_free(h);
    return FIM_NULL;
}

static int inline
op_3x3_calc_pixel(struct op_3x3_parm *p, fim_byte_t *s1,
		  fim_byte_t *s2, fim_byte_t *s3)
{
    int val = 0;

    val += p->f1[0] * s1[0];
    val += p->f1[1] * s1[3];
    val += p->f1[2] * s1[6];
    val += p->f2[0] * s2[0];
    val += p->f2[1] * s2[3];
    val += p->f2[2] * s2[6];
    val += p->f3[0] * s3[0];
    val += p->f3[1] * s3[3];
    val += p->f3[2] * s3[6];
    if (p->mul && p->div)
	val = val * p->mul / p->div;
    val += p->add;
    return val;
}

static void
op_3x3_calc_line(const struct ida_image *src, struct ida_rect *rect,
		 int *dst, unsigned int line, struct op_3x3_parm *p)
{
    fim_byte_t b1[9],b2[9],b3[9];
    fim_byte_t *s1,*s2,*s3;
    unsigned int i,left,right;

    s1 = src->data + (line-1) * src->i.width * 3;
    s2 = src->data +  line    * src->i.width * 3;
    s3 = src->data + (line+1) * src->i.width * 3;
    if (0 == line)
	s1 = src->data + line * src->i.width * 3;
    if (src->i.height-1 == line)
	s3 = src->data + line * src->i.width * 3;

    left  = rect->x1;
    right = rect->x2;
    if (0 == left) {
	/* left border special case: dup first col */
	memcpy(b1,s1,3);
	memcpy(b2,s2,3);
	memcpy(b3,s3,3);
	memcpy(b1+3,s1,6);
	memcpy(b2+3,s2,6);
	memcpy(b3+3,s3,6);
	dst[0] = op_3x3_calc_pixel(p,b1,b2,b3);
	dst[1] = op_3x3_calc_pixel(p,b1+1,b2+1,b3+1);
	dst[2] = op_3x3_calc_pixel(p,b1+2,b2+2,b3+2);
	left++;
    }
    if (src->i.width == right) {
	/* right border */
	memcpy(b1,s1+src->i.width*3-6,6);
	memcpy(b2,s2+src->i.width*3-6,6);
	memcpy(b3,s3+src->i.width*3-6,6);
	memcpy(b1+3,s1+src->i.width*3-3,3);
	memcpy(b2+3,s2+src->i.width*3-3,3);
	memcpy(b3+3,s3+src->i.width*3-3,3);
	dst[src->i.width*3-3] = op_3x3_calc_pixel(p,b1,b2,b3);
	dst[src->i.width*3-2] = op_3x3_calc_pixel(p,b1+1,b2+1,b3+1);
	dst[src->i.width*3-1] = op_3x3_calc_pixel(p,b1+2,b2+2,b3+2);
	right--;
    }
    
    dst += 3*left;
    s1  += 3*(left-1);
    s2  += 3*(left-1);
    s3  += 3*(left-1);
    for (i = left; i < right; i++) {
	dst[0] = op_3x3_calc_pixel(p,s1++,s2++,s3++);
	dst[1] = op_3x3_calc_pixel(p,s1++,s2++,s3++);
	dst[2] = op_3x3_calc_pixel(p,s1++,s2++,s3++);
	dst += 3;
    }
}

static void
op_3x3_clip_line(fim_byte_t *dst, int *src, int left, int right)
{
    int i,val;

    src += left*3;
    dst += left*3;
    for (i = left*3; i < right*3; i++) {
	val = *(src++);
	if (val < 0)
	    val = 0;
	if (val > 255)
	    val = 255;
	*(dst++) = val;
    }
}

static void
op_3x3_work(const struct ida_image *src, struct ida_rect *rect,
	    fim_byte_t *dst, int line, void *data)
{
    struct op_3x3_handle *h = (struct op_3x3_handle *)data;
    fim_byte_t *scanline;

    scanline = (fim_byte_t*) src->data + line * src->i.width * 3;
    memcpy(dst,scanline,src->i.width * 3);
    if (line < rect->y1 || line >= rect->y2)
	return;

    op_3x3_calc_line(src,rect,h->linebuf,line,&h->filter);
    op_3x3_clip_line(dst,h->linebuf,rect->x1,rect->x2);
}

static void
op_3x3_free(void *data)
{
    struct op_3x3_handle *h = (struct op_3x3_handle *)data;

    fim_free(h->linebuf);
    fim_free(h);
}
	    
/* ----------------------------------------------------------------------- */

struct op_sharpe_handle {
    int  factor;
    int  *linebuf;
};

static void*
op_sharpe_init(const struct ida_image *src, struct ida_rect *rect,
	       struct ida_image_info *i, void *parm)
{
    struct op_sharpe_parm *args = (struct op_sharpe_parm *)parm;
    struct op_sharpe_handle *h;

    h = (struct op_sharpe_handle *)fim_calloc(1,sizeof(*h));
    if(!h)goto oops;
    h->factor  = args->factor;
    h->linebuf = (int *)fim_malloc(sizeof(int)*3*(src->i.width));
    if(!h->linebuf)goto oops;

    *i = src->i;
    return h;
oops:
    if(h && h->linebuf)fim_free(h->linebuf);
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
op_sharpe_work(const struct ida_image *src, struct ida_rect *rect,
	       fim_byte_t *dst, int line, void *data)
{
    static struct op_3x3_parm laplace = {
/*  	f1: {  1,  1,  1 },
	f2: {  1, -8,  1 },
	f3: {  1,  1,  1 },*/
  	 {  1,  1,  1 },
	 {  1, -8,  1 },
	 {  1,  1,  1 },
	 0,0,0
    };
    struct op_sharpe_handle *h = (struct op_sharpe_handle *)data;
    fim_byte_t *scanline;
    int i;

    scanline = src->data + line * src->i.width * 3;
    memcpy(dst,scanline,src->i.width * 3);
    if (line < rect->y1 || line >= rect->y2)
	return;

    op_3x3_calc_line(src,rect,h->linebuf,line,&laplace);
    for (i = rect->x1*3; i < rect->x2*3; i++)
	h->linebuf[i] = scanline[i] - h->linebuf[i] * h->factor / 256;
    op_3x3_clip_line(dst,h->linebuf,rect->x1,rect->x2);
}

static void
op_sharpe_free(void *data)
{
    struct op_sharpe_handle *h = (struct op_sharpe_handle *)data;

    fim_free(h->linebuf);
    fim_free(h);
}
#endif /* FIM_WANT_OBSOLETE */

/* ----------------------------------------------------------------------- */

struct op_resize_state {
    float xscale,yscale,inleft;
    float *rowbuf;
    unsigned int width,height,srcrow;
};

static void*
op_resize_init(const struct ida_image *src, struct ida_rect *rect,
	       struct ida_image_info *i, void *parm)
{
    struct op_resize_parm *args = (struct op_resize_parm *)parm;
    struct op_resize_state *h;

    h = (struct op_resize_state *)fim_calloc(1,sizeof(*h));
    if(!h)
	    goto oops;
    h->width  = args->width;
    h->height = args->height;
    h->xscale = (float)args->width/src->i.width;
    h->yscale = (float)args->height/src->i.height;
    h->rowbuf = (float*)fim_malloc(src->i.width * 3 * sizeof(float));
    if(!h->rowbuf)
	    goto oops;
    h->srcrow = 0;
    h->inleft = 1;

    *i = src->i;
    i->width  = args->width;
    i->height = args->height;
    i->dpi    = args->dpi;
    return h;
    oops:
    if(h)
	    fim_free(h);
    return FIM_NULL;
}

#if FIM_WANT_OBSOLETE
#define FIM_HAS_MISC_FBI_OPS 1
#ifdef FIM_HAS_MISC_FBI_OPS
static
void op_resize_work_row_expand(struct ida_image *src, struct ida_rect *rect, fim_byte_t *dst, int line, void *data)
{
	struct op_resize_state *h = (struct op_resize_state *)data;
//#ifndef FIM_WANTS_SLOW_RESIZE	/*uncommenting this triggers failure */
	const int sr=abs((int)h->srcrow); //who knows
//#endif /* FIM_WANTS_SLOW_RESIZE */
	fim_byte_t* srcline=src->data+src->i.width*3*(sr);
	const int Mdx=h->width;
	FIM_REGISTER int sx=0,dx=0;

	/*
	 * this gives a ~ 50% gain
	 * */
		float d0f=0.0,d1f=0.0;
		int   d0i=0,d1i=0;

		if(src->i.width) for (sx=0;sx<(int)src->i.width-1;++sx )
		{
			d1f+=h->xscale;
			d1i=(unsigned int)d1f;

			sx*=3;
			for (dx=d0i;dx<(int)d1i;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  sx  ];
				dst[3*dx+1] = srcline[  sx+1];
				dst[3*dx+2] = srcline[  sx+2] ;
			}
			sx/=3;
	
			d0f=d1f;
			d0i=(unsigned int)d0f;
		}
		{
		// contour fix
			sx*=3;
			for (dx=d0i;dx<Mdx;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  sx  ];
				dst[3*dx+1] = srcline[  sx+1];
				dst[3*dx+2] = srcline[  sx+2] ;
			}
			sx/=3;
		}
		//for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }dx=0;
		if(line==(int)h->height-1)for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }
}
#endif /* FIM_WANT_OBSOLETE */


#ifndef FIM_WANTS_SLOW_RESIZE
static inline void op_resize_work_row_expand_i_unrolled(const struct ida_image *src, struct ida_rect *rect, fim_byte_t *dst, int line, void *data, int sr)
{
	struct op_resize_state *h = (struct op_resize_state *)data;
	const fim_byte_t* srcline=src->data+src->i.width*3*(sr);
	const int Mdx=h->width;
	FIM_REGISTER int sx=0,dx=0;
	/*
	 * interleaved loop unrolling ..
	 * this gives a ~ 50% gain
	 * */
		float d0f=0.0,d1f=0.0;
		int   d0i=0,d1i=0;

		if(src->i.width)
		for (   ;sx<(int)src->i.width-1-4;sx+=4)
		{
			d1f+=h->xscale;
			d1i=(unsigned int)d1f;

			sx*=3;
			for (dx=d0i;dx<d1i;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  sx  ];
				dst[3*dx+1] = srcline[  sx+1];
				dst[3*dx+2] = srcline[  sx+2] ;
			}
	
			d0f=d1f;
			d0i=(unsigned int)d0f;

			d1f+=h->xscale;
			d1i=(unsigned int)d1f;

			sx+=3;
			for (dx=d0i;dx<d1i;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  sx  ];
				dst[3*dx+1] = srcline[  sx+1];
				dst[3*dx+2] = srcline[  sx+2] ;
			}
	
			d0f=d1f;
			d0i=(unsigned int)d0f;

			d1f+=h->xscale;
			d1i=(unsigned int)d1f;

			sx+=3;
			for (dx=d0i;dx<d1i;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  sx  ];
				dst[3*dx+1] = srcline[  sx+1];
				dst[3*dx+2] = srcline[  sx+2] ;
			}
	
			d0f=d1f;
			d0i=(unsigned int)d0f;

			d1f+=h->xscale;
			d1i=(unsigned int)d1f;

			sx+=3;
			for (dx=d0i;dx<d1i;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  sx  ];
				dst[3*dx+1] = srcline[  sx+1];
				dst[3*dx+2] = srcline[  sx+2] ;
			}
			sx-=9;
			sx/=3;

			d0f=d1f;
			d0i=(unsigned int)d0f;
		}

		for (  ;sx<(int)src->i.width ;++sx)
		{
			d1f+=h->xscale;
			d1i=(unsigned int)d1f;
			// contour fix
			for (dx=d0i;dx<Mdx;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  3*sx  ];
				dst[3*dx+1] = srcline[  3*sx+1];
				dst[3*dx+2] = srcline[  3*sx+2] ;
			}
			d0f=d1f;
			d0i=(unsigned int)d0f;
		}
		//for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }dx=0;
		if(line==(int)h->height-1)for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }
}
#endif /* FIM_WANTS_SLOW_RESIZE */

const inline void op_resize_work_unrolled4_row_expand(const struct ida_image *src, struct ida_rect *rect, fim_byte_t *FIM_RSTRCT dst, int line, void *FIM_RSTRCT data, int sr)
{
	struct op_resize_state *FIM_RSTRCT h = (struct op_resize_state *)data;
	const fim_byte_t*FIM_RSTRCT  srcline=src->data+src->i.width*3*(sr);
	const int Mdx=h->width;
	FIM_REGISTER int sx=0,dx=0;

	/*
	 * this gives a ~ 70% gain
	 * */
		float d0f=0.0,d1f=0.0;
		int   d0i=0,d1i=0;

		if(src->i.width) for (   ;sx<(int)src->i.width;++sx )
		{
			d1f+=h->xscale;
			d1i=(unsigned int)d1f;

			FIM_REGISTER fim_byte_t r,g,b;
			r=srcline[  3*sx+ 0];
			g=srcline[  3*sx+ 1];
			b=srcline[  3*sx+ 2];

			for (dx=d0i;dx<d1i-4;dx+=4 )//d1i < Mdx
			{
				dst[3*dx+ 0] = r;
				dst[3*dx+ 1] = g;
				dst[3*dx+ 2] = b;

				dst[3*dx+ 3] = r;
				dst[3*dx+ 4] = g;
				dst[3*dx+ 5] = b;

				dst[3*dx+ 6] = r;
				dst[3*dx+ 7] = g;
				dst[3*dx+ 8] = b;

				dst[3*dx+ 9] = r;
				dst[3*dx+10] = g;
				dst[3*dx+11] = b;
			}
			for (   ;dx<d1i;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = r;
				dst[3*dx+1] = g;
				dst[3*dx+2] = b;
			}
	
			d0f=d1f;
			d0i=(unsigned int)d0f;
		}
		// contour fix
		sx=src->i.width-1;
		for (dx=d0i;dx<Mdx;++dx )//d1i < Mdx
		{
			dst[3*dx+0] = srcline[  3*sx  ];
			dst[3*dx+1] = srcline[  3*sx+1];
			dst[3*dx+2] = srcline[  3*sx+2] ;
		}
		//for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }dx=0;
		if(line==(int)h->height-1)for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }
}

#ifndef FIM_WANTS_SLOW_RESIZE
static inline void op_resize_work_unrolled2_row_expand(const struct ida_image *src, struct ida_rect *rect, fim_byte_t *dst, int line, void *data, int sr)
{
	struct op_resize_state *h = (struct op_resize_state *)data;
	fim_byte_t* srcline=src->data+src->i.width*3*(sr);
	const int Mdx=h->width;
	FIM_REGISTER int sx=0,dx=0;

	/*
	 * this gives a ~ 60% gain
	 * */
		float d0f=0.0,d1f=0.0;
		int   d0i=0,d1i=0;

		if(src->i.width) for (   ;sx<(int)src->i.width-1;++sx )
		{
			d1f+=h->xscale;
			d1i=(unsigned int)d1f;

			FIM_REGISTER fim_byte_t r,g,b;
			r=srcline[  3*sx+ 0];
			g=srcline[  3*sx+ 1];
			b=srcline[  3*sx+ 2];

			for (dx=d0i;FIM_LIKELY(dx<d1i-2);dx+=2 )//d1i < Mdx
			{
				dst[3*dx+ 0] = r;
				dst[3*dx+ 1] = g;
				dst[3*dx+ 2] = b;

				dst[3*dx+ 3] = r;
				dst[3*dx+ 4] = g;
				dst[3*dx+ 5] = b;
			}
			for (   ;dx<d1i;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = r;
				dst[3*dx+1] = g;
				dst[3*dx+2] = b;
			}
	
			d0f=d1f;
			d0i=(unsigned int)d0f;
		}
		{
		// contour fix
			sx*=3;
			for (dx=d0i;dx<Mdx;++dx )//d1i < Mdx
			{
				dst[3*dx+0] = srcline[  sx  ];
				dst[3*dx+1] = srcline[  sx+1];
				dst[3*dx+2] = srcline[  sx+2] ;
			}
			sx/=3;
		}
		//for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }dx=0;
		if(line==(int)h->height-1)for (dx=0;dx<Mdx;++dx ) { dst[3*dx+0]=0x00; dst[3*dx+1]=0x00; dst[3*dx+2]=0x00; }
}
#endif /* FIM_WANTS_SLOW_RESIZE */
#endif /* FIM_HAS_MISC_FBI_OPS */

static void
op_resize_work(const struct ida_image *FIM_RSTRCT src, struct ida_rect *rect,
	       fim_byte_t *FIM_RSTRCT dst, int line, void *FIM_RSTRCT data)
{
    struct op_resize_state *FIM_RSTRCT h = (struct op_resize_state *)data;
    float outleft,left,weight,d0,d1,d2;
    const fim_byte_t *FIM_RSTRCT csrcline;
    float *FIM_RSTRCT fsrcline;
    FIM_REGISTER unsigned int i,sx,dx;
    float *FIM_RSTRCT rowbuf = h->rowbuf; 

#ifndef FIM_WANTS_SLOW_RESIZE
    const int sr=abs((int)h->srcrow); //who knows
#endif /* FIM_WANTS_SLOW_RESIZE */

    /* scale y */
    fim_bzero(rowbuf, src->i.width * 3 * sizeof(float));
    outleft = 1/h->yscale;
    while (outleft > 0  &&  h->srcrow < src->i.height) {
	if (outleft < h->inleft) {
	    weight     = outleft * h->yscale;
	    h->inleft -= outleft;
	    outleft    = 0;
	} else {
	    weight     = h->inleft * h->yscale;
	    outleft   -= h->inleft;
	    h->inleft  = 0;
	}
#if 0
        if(FIM_DD_DEBUG())
	    FIM_FBI_PRINTF("y:  %6.2f%%: %d/%d => %d/%d\n",
		    weight*100,h->srcrow,src->height,line,h->height);
#endif
	csrcline = src->data + h->srcrow * src->i.width * 3;
	for (i = 0; i < src->i.width * 3; i++)
	    rowbuf[i] += (float)csrcline[i] * weight;
	if (0 == h->inleft) {
	    h->inleft = 1;
	    h->srcrow++;
	}
    }

#ifndef FIM_WANTS_SLOW_RESIZE
	/*
	 * a little tweaked resize : the following loop takes the most of cpu resources in a typical fim run!
	 */
	/* scale x */
	left = 1.0f;
	fsrcline = h->rowbuf;
    	const float c_outleft = 1.0f/h->xscale;
	//	cout << "c_outleft : "  << c_outleft << "\n";
	//	cout << "h->width : "  << (int)h->width << "\n";
	const unsigned int Mdx=h->width,msx=src->i.width;
	if(h->xscale>1.0)//here we handle the case of magnification
	{
#ifdef FIM_WANTS_SLOW_RESIZE
		fim_byte_t*FIM_RSTRCT srcline=src->data+src->i.width*3*(sr);
#endif /* FIM_WANTS_SLOW_RESIZE */

#ifndef FIM_WANTS_SLOW_RESIZE
		if(h->xscale>2.0)
		{
			if(h->xscale>4.0)
				op_resize_work_unrolled4_row_expand( src, rect, dst, line, data, sr);
			else
				op_resize_work_unrolled2_row_expand( src, rect, dst, line, data, sr);
		}
		else
			op_resize_work_row_expand_i_unrolled( src, rect, dst, line, data, sr);
#if FIM_WANT_OBSOLETE
//			op_resize_work_row_expand( src, rect, dst, line, data);
#endif /* FIM_WANT_OBSOLETE */

#else /* FIM_WANTS_SLOW_RESIZE */
		float fsx=0.0;
		for (sx=0,dx=0; dx<Mdx; ++dx)
		{
	#if 1
			fsx+=c_outleft;		// += is usually much lighter than a single *
			sx=((unsigned int)fsx)%src->i.width;// % is essential
			dst[0] = srcline[3*sx];
			dst[1] = srcline[3*sx+1]; 
			dst[2] = srcline[3*sx+2] ;
			dst += 3;
	#else
			fsx+=c_outleft;
			sx=((unsigned int)fsx)%src->i.width;// % is (maybe) essential
			dst[0] = (fim_byte_t) fsrcline[3*sx];
			dst[1] = (fim_byte_t) fsrcline[3*sx+1]; 
			dst[2] = (fim_byte_t) fsrcline[3*sx+2] ;
			dst += 3;
	#endif
		}
#endif /* FIM_WANTS_SLOW_RESIZE */
	}
#define ZEROF 0.0f
	else    // image minification
	for (sx = 0, dx = 0; dx < Mdx; dx++) {
	d0 = d1 = d2 = ZEROF;
	outleft = c_outleft;
	while (outleft > ZEROF &&  sx < msx) {
	    if (outleft < left) {
		weight   = outleft * h->xscale;
		left    -= outleft;
		outleft  = ZEROF;
	    } else {
		weight   = left * h->xscale;
		outleft -= left;
		left     = ZEROF;
	    }
	    d0 += fsrcline[3*sx] * weight;
	    d1 += fsrcline[3*sx+1] * weight;
	    d2 += fsrcline[3*sx+2] * weight;
	
	    if (ZEROF == left) {
		left = 1.0f;
		sx++;
	    }
	}
	dst[0] = (fim_byte_t)d0;
	dst[1] = (fim_byte_t)d1;
	dst[2] = (fim_byte_t)d2;
	dst += 3;
    }
	return ;
#else
    /* the original, slow cycle */
    /* scale x */
    left = 1;
    fsrcline = h->rowbuf;
    for (sx = 0, dx = 0; dx < h->width; dx++) {
	d0 = d1 = d2 = 0;
	outleft = 1/h->xscale;
	while (outleft > 0  &&  dx < h->width  &&  sx < src->i.width) {
	    if (outleft < left) {
		weight   = outleft * h->xscale;
		left    -= outleft;
		outleft  = 0;
	    } else {
		weight   = left * h->xscale;
		outleft -= left;
		left     = 0;
	    }
#if 0
	    if(FIM_DD_DEBUG())
		FIM_FBI_PRINTF(" x: %6.2f%%: %d/%d => %d/%d\n",
			weight*100,sx,src->width,dx,h->width);
#endif
	    d0 += fsrcline[3*sx+0] * weight;
	    d1 += fsrcline[3*sx+1] * weight;
	    d2 += fsrcline[3*sx+2] * weight;
	    if (0 == left) {
		left = 1;
		sx++;
	    }
	}
	dst[0] = (fim_byte_t)d0;
	dst[1] = (fim_byte_t)d1;
	dst[2] = (fim_byte_t)d2;
	dst += 3;
    }
#endif /* FIM_WANTS_SLOW_RESIZE */
}

static void
op_resize_done(void *data)
{
    struct op_resize_state *h = (struct op_resize_state *)data;

    fim_free(h->rowbuf);
    fim_free(h);
}
    
/* ----------------------------------------------------------------------- */

struct op_rotate_state {
    float angle,sina,cosa;
    struct ida_rect calc;
    int cx,cy;
};

static void*
op_rotate_init(const struct ida_image *src, struct ida_rect *rect,
	       struct ida_image_info *i, void *parm)
{
    struct op_rotate_parm *args = (struct op_rotate_parm *)parm;
    struct op_rotate_state *h;
    float  diag;

    h = (struct op_rotate_state *)fim_malloc(sizeof(*h));
    if(!h)return FIM_NULL;
    /* fim's : FIXME : FIM_NULL check missing */
    h->angle = args->angle * 2 * M_PI / 360;
    h->sina  = sin(h->angle);
    h->cosa  = cos(h->angle);
    /* fim's : cX means source center's X */
    h->cx    = (rect->x2 - rect->x1) / 2 + rect->x1;
    h->cy    = (rect->y2 - rect->y1) / 2 + rect->y1;

    /* the area we have to process (worst case: 45°) */
    diag     = sqrt((rect->x2 - rect->x1)*(rect->x2 - rect->x1) +
		    (rect->y2 - rect->y1)*(rect->y2 - rect->y1))/2;
    /* fim's : diag is a half diagonal */
    /* fim's : calc is the source input rectangle bounded
     * by source image valid coordinates ... */
    h->calc.x1 = (int)(h->cx - diag);
    h->calc.x2 = (int)(h->cx + diag);
    h->calc.y1 = (int)(h->cy - diag);
    h->calc.y2 = (int)(h->cy + diag);
    if (h->calc.x1 < 0)
	h->calc.x1 = 0;
    if (h->calc.x2 > (int)src->i.width)
	h->calc.x2 = (int)src->i.width;
    if (h->calc.y1 < 0)
	h->calc.y1 = 0;
    if (h->calc.y2 > (int)src->i.height)
	h->calc.y2 = (int)src->i.height;

     *i = src->i;
     /* TODO : it would be nice to expand and contract
      * the whole canvas just to fit the rotated image in !
      * */
    return h;
}

static inline
const fim_byte_t* const op_rotate_getpixel(const struct ida_image *src, struct ida_rect *rect,
				  int sx, int sy, int dx, int dy)
{
    static const fim_byte_t black[] = { 0, 0, 0};
#if FIM_TEXTURED_ROTATION
    const int xdiff  =   rect->x2 - rect->x1;
    const int ydiff  =   rect->y2 - rect->y1;
#endif
    if (sx < rect->x1 || sx >= rect->x2 ||
	sy < rect->y1 || sy >= rect->y2) {
#if FIM_TEXTURED_ROTATION
	/* experimental : textured rotation (i.e.: with wrapping) */
	while(sx <  rect->x1)sx+=xdiff;
	while(sx >= rect->x2)sx-=xdiff;
	while(sy <  rect->y1)sy+=ydiff;
	while(sy >= rect->y2)sy-=ydiff;
	return src->data + sy * src->i.width * 3 + sx * 3;
#else
	/* original */
	if (dx < rect->x1 || dx >= rect->x2 ||
	    dy < rect->y1 || dy >= rect->y2)
	    return src->data + dy * src->i.width * 3 + dx * 3;
	return black;
#endif
    }
    return src->data + sy * src->i.width * 3 + sx * 3;
}

static void
op_rotate_work(const struct ida_image *src, struct ida_rect *rect,
	       fim_byte_t *dst, int y, void *data)
{
    struct op_rotate_state *h = (struct op_rotate_state *) data;
    const fim_byte_t *pix;
    float fx,fy,w;
    int x,sx,sy;

    pix = src->data + y * src->i.width * 3;
    /*
     * useless (fim)
     * memcpy(dst,pix,src->i.width * 3);
     */
    if (y < h->calc.y1 || y >= h->calc.y2)
	return;
/*
    fx = h->cosa * (0 - h->cx) - h->sina * (y - h->cy) + h->cx;
    sx = (int)fx;
    sx *= 0;
    dst += 3*(h->calc.x1+sx);*/
    fim_bzero(dst, (h->calc.x2-h->calc.x1) * 3);
   for (x = h->calc.x1; x < h->calc.x2; x++, dst+=3) {
	fx = h->cosa * (x - h->cx) - h->sina * (y - h->cy) + h->cx;
	fy = h->sina * (x - h->cx) + h->cosa * (y - h->cy) + h->cy;
	sx = (int)fx;
	sy = (int)fy;
	if (fx < 0)
	    sx--;
	if (fy < 0)
	    sy--;
	fx -= sx;
	fy -= sy;

	pix = op_rotate_getpixel(src,rect,sx,sy,x,y);
	w = (1-fx) * (1-fy);
	dst[0] += (fim_byte_t)(pix[0] * w);
	dst[1] += (fim_byte_t)(pix[1] * w);
	dst[2] += (fim_byte_t)(pix[2] * w);
	pix = op_rotate_getpixel(src,rect,sx+1,sy,x,y);
	w = fx * (1-fy);
	dst[0] += (fim_byte_t)(pix[0] * w);
	dst[1] += (fim_byte_t)(pix[1] * w);
	dst[2] += (fim_byte_t)(pix[2] * w);
	pix = op_rotate_getpixel(src,rect,sx,sy+1,x,y);
	w = (1-fx) * fy;
	dst[0] += (fim_byte_t)(pix[0] * w);
	dst[1] += (fim_byte_t)(pix[1] * w);
	dst[2] += (fim_byte_t)(pix[2] * w);
	pix = op_rotate_getpixel(src,rect,sx+1,sy+1,x,y);
	w = fx * fy;
	dst[0] += (fim_byte_t)(pix[0] * w);
	dst[1] += (fim_byte_t)(pix[1] * w);
	dst[2] += (fim_byte_t)(pix[2] * w);
    }
}

static void
op_rotate_done(void *data)
{
    struct op_rotate_state *h = (struct op_rotate_state *)data;

    fim_free(h);
}

/* ----------------------------------------------------------------------- */
void  op_none_done(void *data) {}
#if FIM_WANT_OBSOLETE
static fim_byte_t op_none_data;
void* op_none_init(const struct ida_image *src,  struct ida_rect *sel,
		   struct ida_image_info *i, void *parm)
{
    *i = src->i;
    return &op_none_data;
}
/* ----------------------------------------------------------------------- */

struct ida_op desc_grayscale = {
    /*name:*/  "grayscale",
    /*init:*/  op_none_init,
    /*work:*/  op_grayscale,
    /*done:*/  op_none_done,
};
struct ida_op desc_3x3 = {
    /*name:*/  "3x3",
    /*init:*/  op_3x3_init,
    /*work:*/  op_3x3_work,
    /*done:*/  op_3x3_free,
};
struct ida_op desc_sharpe = {
    /*name:*/  "sharpe",
    /*init:*/  op_sharpe_init,
    /*work:*/  op_sharpe_work,
    /*done:*/  op_sharpe_free,
};
#endif /* FIM_WANT_OBSOLETE */
struct ida_op desc_resize = {
    /*name:*/  "resize",
    /*init:*/  op_resize_init,
    /*work:*/  op_resize_work,
    /*done:*/  op_resize_done,
};
struct ida_op desc_rotate = {
    /*name:*/  "rotate",
    /*init:*/  op_rotate_init,
    /*work:*/  op_rotate_work,
    /*done:*/  op_rotate_done,
};

// end filter.c
//



// op.c

#include <stdio.h>
#include <cstdlib>
#include <string.h>

/* ----------------------------------------------------------------------- */
/* functions                                                               */

static fim_byte_t op_none_data_;

#if FIM_WANT_OBSOLETE
static void
op_flip_vert_(const struct ida_image *src, struct ida_rect *rect,
	     fim_byte_t *dst, int line, void *data)
{
    fim_byte_t *scanline;

    scanline = (fim_byte_t*)src->data + (src->i.height - line - 1) * src->i.width * 3;
    memcpy(dst,scanline,src->i.width*3);
}

static void
op_flip_horz_(const struct ida_image *src, struct ida_rect *rect,
	     fim_byte_t *dst, int line, void *data)
{
    fim_byte_t *scanline;
    unsigned int i;

    scanline = (fim_byte_t*)src->data + (line+1) * src->i.width * 3;
    for (i = 0; i < src->i.width; i++) {
	scanline -= 3;
	dst[0] = scanline[0];
	dst[1] = scanline[1];
	dst[2] = scanline[2];
	dst += 3;
    }
}
#endif /* FIM_WANT_OBSOLETE */

static void*
op_rotate_init_(const struct ida_image *src, struct ida_rect *rect,
	       struct ida_image_info *i, void *parm)
{
    *i = src->i;
    i->height = src->i.width;
    i->width  = src->i.height;
    i->dpi    = src->i.dpi;
    return &op_none_data_;
}

static void
op_rotate_cw_(const struct ida_image *src, struct ida_rect *rect,
	     fim_byte_t *dst, int line, void *data)
{
    fim_byte_t *pix;
    unsigned int i;

    pix = (fim_byte_t*) src->data + fbi_img_pixel_bytes(src) + line * 3;
    for (i = 0; i < src->i.height; i++) {
	pix -= src->i.width * 3;
	dst[0] = pix[0];
	dst[1] = pix[1];
	dst[2] = pix[2];
	dst += 3;
    }
}

static void
op_rotate_ccw_(const struct ida_image *src, struct ida_rect *rect,
	      fim_byte_t *dst, int line, void *data)
{
    fim_byte_t *pix;
    unsigned int i;

    pix = (fim_byte_t*) src->data + (src->i.width-line-1) * 3;
    for (i = 0; i < src->i.height; i++) {
	dst[0] = pix[0];
	dst[1] = pix[1];
	dst[2] = pix[2];
	pix += src->i.width * 3;
	dst += 3;
    }
}

#if FIM_WANT_OBSOLETE
static void
op_invert_(const struct ida_image *src, struct ida_rect *rect,
	  fim_byte_t *dst, int line, void *data)
{
    fim_byte_t *scanline;
    int i;

    scanline = src->data + line * src->i.width * 3;
    memcpy(dst,scanline,src->i.width * 3);
    if (line < rect->y1 || line >= rect->y2)
	return;
    dst      += 3*rect->x1;
    scanline += 3*rect->x1;
    for (i = rect->x1; i < rect->x2; i++) {
	dst[0] = 255-scanline[0];
	dst[1] = 255-scanline[1];
	dst[2] = 255-scanline[2];
	scanline += 3;
	dst += 3;
    }
}
#endif /* FIM_WANT_OBSOLETE */

#if FIM_WANT_CROP
static void*
op_crop_init_(const struct ida_image *src, struct ida_rect *rect,
	     struct ida_image_info *i, void *parm)
{
    if (rect->x2 - rect->x1 == (int)src->i.width &&
	rect->y2 - rect->y1 == (int)src->i.height)
	return FIM_NULL;
    *i = src->i;
    i->width  = rect->x2 - rect->x1;
    i->height = rect->y2 - rect->y1;
    return &op_none_data_;
}

static void
op_crop_work_(const struct ida_image *src, struct ida_rect *rect,
	     fim_byte_t *dst, int line, void *data)
{
    fim_byte_t *scanline;
    int i;

    scanline = src->data + (line+rect->y1) * src->i.width * 3 + rect->x1 * 3;
    for (i = rect->x1; i < rect->x2; i++) {
	dst[0] = scanline[0];
	dst[1] = scanline[1];
	dst[2] = scanline[2];
	scanline += 3;
	dst += 3;
    }
}
#endif /* FIM_WANT_CROP */

#if FIM_WANT_OBSOLETE
static void*
op_autocrop_init_(const struct ida_image *src, struct ida_rect *unused,
		 struct ida_image_info *i, void *parm)
{
#ifdef FIM_USE_DESIGNATED_INITIALIZERS
    static struct op_3x3_parm filter = {
	/*f1:*/ { -1, -1, -1 },
	/*f2:*/ { -1,  8, -1 },
	/*f3:*/ { -1, -1, -1 },
    };
#else
    /* I have no quick fix for this ! (m.m.) 
     * However, designated initializers are a a C99 construct
     * and are usually tolerated by g++.
     * */
    static struct op_3x3_parm filter = {
	/*f1:*/ { -1, -1, -1 },
	/*f2:*/ { -1,  8, -1 },
	/*f3:*/ { -1, -1, -1 },
    };
#endif /* FIM_USE_DESIGNATED_INITIALIZERS */
    struct ida_rect rect;
    struct ida_image img;
    int x,y,limit;
    fim_byte_t *line;
    void *data;
    
    /* detect edges */
    rect.x1 = 0;
    rect.x2 = src->i.width;
    rect.y1 = 0;
    rect.y2 = src->i.height;
    data = desc_3x3.init(src, &rect, &img.i, &filter);

    img.data = fim_pm_alloc(img.i.width, img.i.height);
    if(!img.data)return FIM_NULL;

    for (y = 0; y < (int)img.i.height; y++)
	desc_3x3.work(src, &rect, img.data+3*img.i.width*y, y, data);
    desc_3x3.done(data);
    limit = 64;

    /* y border */
    for (y = 0; y < (int)img.i.height; y++) {
	line = img.data + img.i.width*y*3;
	for (x = 0; x < (int)img.i.width; x++)
	    if (line[3*x+0] > limit ||
		line[3*x+1] > limit ||
		line[3*x+2] > limit)
		break;
	if (x != (int)img.i.width)
	    break;
    }
    rect.y1 = y;
    for (y = (int)img.i.height-1; y > rect.y1; y--) {
	line = img.data + img.i.width*y*3;
	for (x = 0; x < (int)img.i.width; x++)
	    if (line[3*x+0] > limit ||
		line[3*x+1] > limit ||
		line[3*x+2] > limit)
		break;
	if (x != (int)img.i.width)
	    break;
    }
    rect.y2 = y+1;

    /* x border */
    for (x = 0; x < (int)img.i.width; x++) {
	for (y = 0; y < (int)img.i.height; y++) {
	    line = img.data + (img.i.width*y+x) * 3;
	    if (line[0] > limit ||
		line[1] > limit ||
		line[2] > limit)
		break;
	}
	if (y != (int)img.i.height)
	    break;
    }
    rect.x1 = x;
    for (x = (int)img.i.width-1; x > rect.x1; x--) {
	for (y = 0; y < (int)img.i.height; y++) {
	    line = img.data + (img.i.width*y+x) * 3;
	    if (line[0] > limit ||
		line[1] > limit ||
		line[2] > limit)
		break;
	}
	if (y != (int)img.i.height)
	    break;
    }
    rect.x2 = x+1;

    fim_free(img.data);
#if 0 /* Disabled 20151213 */
    if(FIM_DD_DEBUG())
	FIM_FBI_PRINTF("y: %d-%d/%d  --  x: %d-%d/%d\n",
		rect.y1, rect.y2, img.i.height,
		rect.x1, rect.x2, img.i.width);
#endif

    if (0 == rect.x2 - rect.x1  ||  0 == rect.y2 - rect.y1)
	return FIM_NULL;
    
    *unused = rect;
    *i = src->i;
    i->width  = rect.x2 - rect.x1;
    i->height = rect.y2 - rect.y1;
    return &op_none_data_;
}

/* ----------------------------------------------------------------------- */
void  op_free_done(void *data) { fim_free(data); }
#endif /* FIM_WANT_OBSOLETE */

/* ----------------------------------------------------------------------- */

#if FIM_WANT_OBSOLETE
struct ida_op desc_flip_vert = {
    /*name:*/  "flip-vert",
    /*init:*/  op_none_init,
    /*work:*/  op_flip_vert_,
    /*done:*/  op_none_done,
};
struct ida_op desc_flip_horz = {
    /*name:*/  "flip-horz",
    /*init:*/  op_none_init,
    /*work:*/  op_flip_horz_,
    /*done:*/  op_none_done,
};
#endif /* FIM_WANT_OBSOLETE */
struct ida_op desc_rotate_cw = {
    /*name:*/  "rotate-cw",
    /*init:*/  op_rotate_init_,
    /*work:*/  op_rotate_cw_,
    /*done:*/  op_none_done,
};
struct ida_op desc_rotate_ccw = {
    /*name:*/  "rotate-ccw",
    /*init:*/  op_rotate_init_,
    /*work:*/  op_rotate_ccw_,
    /*done:*/  op_none_done,
};
#if FIM_WANT_OBSOLETE
struct ida_op desc_invert = {
    /*name:*/  "invert",
    /*init:*/  op_none_init,
    /*work:*/  op_invert_,
    /*done:*/  op_none_done,
};
#endif /* FIM_WANT_OBSOLETE */
#if FIM_WANT_CROP
struct ida_op desc_crop = {
    /*name:*/  "crop",
    /*init:*/  op_crop_init_,
    /*work:*/  op_crop_work_,
    /*done:*/  op_none_done,
};
#endif /* FIM_WANT_CROP */
#if FIM_WANT_OBSOLETE
struct ida_op desc_autocrop = {
    /*name:*/  "autocrop",
    /*init:*/  op_autocrop_init_,
    /*work:*/  op_crop_work_,
    /*done:*/  op_none_done,
};
#endif /* FIM_WANT_OBSOLETE */

// end op.c



#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <errno.h>

#ifdef USE_X11
 #include "viewer.h"
#endif /* USE_X11 */

/* ---------------------------------------------------------------------- */
/* load                                                                   */


#ifdef HAVE_LIBGRAPHICSMAGICK
	extern struct ida_loader magick_loader ;
#endif /* HAVE_LIBGRAPHICSMAGICK */

#ifdef FIM_WITH_LIBPNG 
	extern struct ida_loader png_loader ;
#endif /* FIM_WITH_LIBPNG */

extern struct ida_loader ppm_loader ;
extern struct ida_loader pgm_loader ;
#if FIM_WANT_TEXT_RENDERING
extern struct ida_loader text_loader ;
#endif /* FIM_WANT_TEXT_RENDERING */
#if FIM_WANT_RAW_BITS_RENDERING
extern struct ida_loader bit24_loader ;
extern struct ida_loader bit1_loader ;
#endif /* FIM_WANT_RAW_BITS_RENDERING */
#if FIM_WITH_UFRAW
extern struct ida_loader nef_loader ;
#endif /* FIM_WITH_UFRAW */

// 20080108 WARNING
// 20080801 removed the loader functions from this file, as init_rd was not fim__init : did I break something ?
//static void fim__init init_rd(void)
/*static void init_rd(void)
{
    fim_load_register(&ppm_loader);
    fim_load_register(&pgm_loader);
}*/

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
    /*  label:*/  "PPM",
    /*  ext:*/    { "ppm", FIM_NULL},
    /*  write:*/  ppm_write,
    /* FIXME : still missing some struct members */
};

// 20080108 WARNING
//static void fim__init init_wr(void)
static void init_wr(void)
{
    fim_write_register(&ppm_writer);
}
#endif /* USE_X11 */




/*static void free_image(struct ida_image *img)*/
void FbiStuff::free_image(struct ida_image *img)
{
    if (img) {
	if (img->data)
	    fim_free(img->data);
	fim_free(img);
    }
}

FILE* FbiStuff::fim_execlp(const fim_char_t *cmd, ...)
{
#if HAVE_PIPE
	/* new */
	va_list ap;
        int rc;
	FILE *fp=FIM_NULL;
	int p[2];
	#define FIM_SUBPROCESS_MAXARGV 128
	fim_char_t * argv[FIM_SUBPROCESS_MAXARGV],*s;	/* FIXME */
	int argc=0;

	if(0!=pipe(p))
		goto err;

	switch(pid_t pid = fork())
	{
		case -1:
			fim_perror("fork");
			close(p[0]);
			close(p[1]);
			goto err; // FIXME
		case 0:/* child */
			dup2(p[1],1/*stdout*/);
			close(p[0]);
			close(p[1]);
	        	va_start(ap,cmd);
			while(FIM_NULL!=(s=va_arg(ap,fim_char_t*)) && argc<FIM_SUBPROCESS_MAXARGV-1)
			{
				argv[argc]=s;
				argc++;
			}
			argv[argc]=FIM_NULL;

	        	va_end(ap);
	        	rc=execvp(cmd,argv);
			exit(rc);
		default:/* parent */
#if FIM_HAS_TIMEOUT
			waitpid(pid,NULL,0); // TODO: pass int wstatus and check
#else /* FIM_HAS_TIMEOUT */
			waitpid(pid,NULL,WNOHANG);
			sleep(1); // to avoid parent process to find e.g. no result file
#endif /* FIM_HAS_TIMEOUT */
			close(p[1]);
			fp = fdopen(p[0],"r");
			if(FIM_NULL==fp)
				goto err;
			return fp;
	}
err:
#endif /* HAVE_PIPE */
	return FIM_NULL;
}

#ifdef FIM_WANT_SEEK_MAGIC
static long find_byte_stream(FILE *fp, const fim_char_t *byte_stream, size_t base_offset)
{
	size_t rb,sl,goff=0,boff=0;
	const size_t bl = FIM_FILE_BUF_SIZE;
	fim_char_t buf[bl];

	if(!byte_stream)
		goto err;

	sl=strlen(byte_stream);

	if(sl==0)
	{
		goto err;
	}

	if(sl>bl)
	{
		goto err;
	}

	if(base_offset)
	{
    		if(fim_fseek(fp,base_offset,SEEK_SET)!=0)
			goto err;
		goff=base_offset;
	}
	else
		goff+=ftell(fp);

	while((rb=fim_fread(buf+boff,1,bl-boff,fp))>0)
	{
		if(memmem(buf,rb+boff,byte_stream,sl))
			return goff+(((fim_char_t*)memmem(buf,rb+boff,byte_stream,sl))-buf);
		if(rb+boff<bl)
			goto err;
		boff=sl-1;
		memmove(buf,buf+bl-boff,boff);
		goff+=bl-boff;
	}
err:
	return 0;
}
#endif /* FIM_WANT_SEEK_MAGIC */

static void rgb2bgr(fim_byte_t *data, const fim_coo_t w, const fim_coo_t h) 
{

	FIM_REGISTER fim_byte_t t;
	FIM_REGISTER fim_byte_t *p=data,
		 	*pm=p+w*3*h;
	while(p<pm)
	{
            t=*p;
            *p=p[2];
            p[2]=t;
	    p+=3;
	}
}

static fim::string fim_tempnam(const fim::string pfx="", const fim::string sfx="")
{
	std::string tfn;
	std::string tsfx = "__FIM_TEMPORARY_FILE";
	tsfx += sfx;

#ifdef FIM_TMP_FILENAME 
	if( FIM_TMP_FILENAME && strlen(FIM_TMP_FILENAME) > 1 )
		tfn = FIM_TMP_FILENAME;
	else
#endif /* FIM_TMP_FILENAME */
	{
		char * const tn = tempnam(NULL,tsfx.c_str()); // deprecated
		tfn = tn;
		free(tn);
		//tfn = std::string(std::experimental::filesystem::temp_directory_path());
		//tfn = tmpnam(NULL); // deprecated
		// std::cout << "using temporary file:" << tfn << std::endl;
	}

	return tfn;
}

/*static struct ida_image**/
struct ida_image* FbiStuff::read_image(const fim_char_t *filename, FILE* fd, fim_int page, Namespace *nsp)
{
    /*
     * This function is complicated and should be reworked, in some way.
     * FIXME : many memory allocations are not checked for failure: DANGER
     * */
#ifdef FIM_TRY_INKSCAPE
#ifdef FIM_WITH_LIBPNG 
    const char * xmlh = R"***(<?xml version="1.0" encoding="UTF-8")***"; // a raw string literal does not need escape sequences with backslash
    //fim_char_t command[FIM_PIPE_CMD_BUFSIZE]; /* FIXME: overflow risk ! */
#endif /* FIM_WITH_LIBPNG  */
#endif /* FIM_TRY_INKSCAPE */
    struct ida_loader *loader = FIM_NULL;
    struct ida_image *img=FIM_NULL;
    struct list_head *item=FIM_NULL;
    fim_char_t blk[FIM_FILE_PROBE_BLKSIZE];
    FILE *fp=FIM_NULL;
    unsigned int y;
    void *data=FIM_NULL;
    size_t fr=0;
#if FIM_HAVE_FULL_PROBING_LOADER
    const bool rozlsl=false;/* retry on zero length signature loader */
#endif /* FIM_HAVE_FULL_PROBING_LOADER */
#if FIM_ALLOW_LOADER_VERBOSITY
    const fim_int vl=(cc.getIntVariable(FIM_VID_VERBOSITY));
#else /* FIM_ALLOW_LOADER_VERBOSITY */
    const fim_int vl=0;
#endif /* FIM_ALLOW_LOADER_VERBOSITY */
#if FIM_SHALL_BUFFER_STDIN
    fim_byte_t * sbuf=FIM_NULL;
    size_t sbbs=FIM_NULL;
#endif /* FIM_SHALL_BUFFER_STDIN */
    int want_retry=0;
    fim_int read_offset_l = 0, read_offset_u = 0;
    const fim::string pc = cc.getStringVariable(FIM_VID_PREAD);
    const bool can_pipe = cc.getIntVariable(FIM_VID_NO_EXTERNAL_LOADERS)==0 && regexp_match(filename, FIM_CNS_PIPEABLE_PATH_RE);
#if FIM_WITH_ARCHIVE
    int npages = 0;
    fim::string re = cc.getGlobalStringVariable(FIM_VID_ARCHIVE_FILES);

    if( re == FIM_CNS_EMPTY_STRING )
	    re = FIM_CNS_ARCHIVE_RE;
#endif /* FIM_WITH_ARCHIVE */
    fim::string tpfn = fim_tempnam("fim_temporary_file",".png");
    FIM_PR('*');
    
    //if(vl)FIM_VERB_PRINTF("approaching loading \"%s\", FILE*:%p\n",filename,fd);
    if(vl)
	    FIM_VERB_PRINTF("verbosely (level %d) loading page %d of %spipable filename \"%s\"\n",(int)vl,(int)page,(can_pipe?"":"un"),filename);
    //WARNING
    //new_image = 1;

#if FIM_SHALL_BUFFER_STDIN
    if(fd!=FIM_NULL)
    if(strcmp(filename,FIM_STDIN_IMAGE_NAME)==0) 
    {
	    if(vl)
		    FIM_VERB_PRINTF("will attempt to use fmemopen\n");

	    sbuf=slurp_binary_FD(fd,&sbbs);
	    if(sbuf==FIM_NULL || !sbbs)
	    {
		if(sbuf)fim_free(sbuf);
    		if(vl)FIM_VERB_PRINTF("problems slurping the file\n");
	    }
	    else
	    {
		fd=fmemopen(sbuf,sbbs,"rb");
    		if(vl)FIM_VERB_PRINTF("using fmemopen\n");
	    }
    }
#endif /* FIM_SHALL_BUFFER_STDIN */
    // Warning: this fd passing 
    // is a trick for reading stdin...
    // ... and it is simpler that rewriting loader stuff.
    // but much dirtier :/
    if(fd==FIM_NULL){
    /* open file */
    if (FIM_NULL == (fp = fim_fopen(filename, "r"))) {
	if(FIM_DD_DEBUG())
		FIM_FBI_PRINTF("open %s: %s\n",filename,strerror(errno));
	return FIM_NULL;
    }
    } else fp=fd;

#if FIM_WITH_ARCHIVE
    if( regexp_match(filename,re.c_str(),1) )
    {
	struct archive *a = FIM_NULL;
	struct archive_entry *entry = FIM_NULL;
	int r,pi;
	const size_t bs = 10240;

    	if(vl) FIM_VERB_PRINTF("filename matches archives regexp: \"%s\"\n",re.c_str());

	re = cc.getGlobalStringVariable(FIM_VID_PUSHDIR_RE);

	if( re == FIM_CNS_EMPTY_STRING )
		re = FIM_CNS_PUSHDIR_RE;
    	if(vl) FIM_VERB_PRINTF("will scan archive for files matching regexp: \"%s\"\n",re.c_str());

	if( fim_getenv("PAGE") )
		page = fim_atoi( fim_getenv("PAGE") );

	a = archive_read_new();
	if (a == FIM_NULL)
		goto noa;
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);
	r = archive_read_open_filename(a, filename, bs); // filename=FIM_NULL for stdin
	if (r != ARCHIVE_OK)
	{
		printf("Problems opening archive %s\n",filename);
		goto noa;
	}

	for (pi=0;;)
	{
                const char * pn = FIM_NULL;
		r = archive_read_next_header(a, &entry);
      		if (r == ARCHIVE_EOF)
		{
			npages = pi  ;
			if(vl)
				printf("ARCHIVE_EOF reached after %d files.\n",(int)npages);
			break;
		}
		if (r != ARCHIVE_OK)
		{
			printf("Problems reading header of %s\n",filename);
			break;
		}
		pn = archive_entry_pathname(entry);

    		if( pn && regexp_match(pn,re.c_str(),1) && strlen(pn)>0 && pn[strlen(pn)-1] != FIM_CNS_DIRSEP_CHAR ) /* skip directories */
		{
			//std::cout << re << " " << pi << " " << pn << " " << page  << ".\n"; // FIXME
			if(pi == page)
			{
				static int fap[2];
    				if(vl)
					printf("Opening page %d of %s, subfile %s\n",(int)page,filename,pn);
				//archive_read_data_into_fd(a,1);
				if(0)
				{
					const void *buff = FIM_NULL;
					int64_t offset = 0;
					size_t tsize = 0, size = 0;

					if( 0 != pipe2(fap,O_NONBLOCK) )
					//if( 0 != pipe(fap) )
						goto noa;
					printf("Pipe to %s\n",pn);

					tsize = 0, size = 0;
					for (;;) {
						r = archive_read_data_block(a, &buff, &size, &offset);
						if (r == ARCHIVE_EOF)
							break;
						if (r != ARCHIVE_OK)
							break;
						write(fap[1],buff,size);
						tsize += size;
						// ...
					}
					printf("piped %zd bytes\n",(size_t)tsize);
					close(fap[1]);
					fp = fdopen(fap[0],"r");
					fd = FIM_NULL;
					fp = fim_fread_tmpfile(fp); // FIXME: a pipe saturates quickly (at 64 k on recent Linux...)
					close(fap[0]);
				}
				else
				{
					FILE *tfd=FIM_NULL;
					if( ( tfd=tmpfile() )!=FIM_NULL )
					{	
						const int tfp = fileno(tfd);
						r = archive_read_data_into_fd(a,tfp);
						rewind(tfd);
						fd = FIM_NULL;
						fp = tfd;
					}
					else
					{
						std::cout << "Problem opening embedded file!\n"; // FIXME
						archive_read_data_skip(a);
					}
				}
				filename = FIM_STDIN_IMAGE_NAME;
			}
			else
			{
				//archive_read_data_skip(a);
				if(vl)printf("SKIPPING MATCHING [%d/%d] %s in %s\n",(int)pi,(int)page,pn,filename);
			}
			++pi;
		}
		else
		{
			if(vl)printf("SKIPPING NON MATCHING [%d/%d] %s in %s\n",(int)pi,(int)page,pn,filename);
			//archive_read_data_skip(a);
		}
	}
noa:
	if (a != FIM_NULL)
		archive_read_close(a),
		archive_read_free(a);
    }
#endif /* FIM_WITH_ARCHIVE */
    read_offset_l =                 cc.getIntVariable(FIM_VID_OPEN_OFFSET_L);
    read_offset_u = read_offset_l + cc.getIntVariable(FIM_VID_OPEN_OFFSET_U);
    read_offset_u = FIM_MAX(read_offset_l,read_offset_u);
    if(vl)
    {
	    if(read_offset_l||read_offset_u)
		    FIM_VERB_COUT << "..found at " << read_offset_l << ":" << read_offset_u << "\n";
	    else
		    FIM_VERB_PRINTF("no file seek range specified\n");
    }
with_offset:
    if(read_offset_l>0)
	    fim_fseek(fp,read_offset_l,SEEK_SET);
#ifdef FIM_WANT_SEEK_MAGIC
	std::string sm = cc.getStringVariable(FIM_VID_SEEK_MAGIC);
	if( int sl = sm.length() )
	{
	/*
		the user should be able to specify a magic string like:
		sm="\xFF\xD8\xFF\xE0";
	*/
        	if(vl>0)FIM_VERB_PRINTF("probing file signature (long %d) \"%s\"..\n",sl,sm.c_str());
		const long regexp_offset = find_byte_stream(fp, sm.c_str(), read_offset_l);
		if(regexp_offset>0)
		{
			read_offset_l=regexp_offset;
			read_offset_u=read_offset_l+sl;
        		if(vl>0)
				FIM_VERB_COUT << "..found at " << read_offset_l << ":" << read_offset_u << "\n";
			fim_fseek(fp,read_offset_l,SEEK_SET);
			cc.setVariable(FIM_VID_SEEK_MAGIC,"");
		}
	}
#endif /* FIM_WANT_SEEK_MAGIC */
    fim_bzero(blk,sizeof(blk));
    if((fr=fim_fread(blk,1,sizeof(blk),fp))<0)
    {
      /* should we care about the error code ? */
      return FIM_NULL;	/* new */
    }
    if(fr < sizeof(blk)) // FIXME: shall compare to min(sizeof(blk),filesize)
    {
      if(fr == 0)
      {
        std::cerr << "Reading an empty file ?\n";
        // FIXME: need to handle this case.
	//goto ret;
      }
      else
      {
        // std::cout << "Read only " << fr << " bytes for block probing!\n";
        // FIXME: tolerating and going further, for now (it might be a tiny file).
	// goto ret;
      }
    }
    fim_rewind(fp);
    if(read_offset_l>0)
	    fim_fseek(fp,read_offset_l,SEEK_SET);

#ifdef FIM_TRY_CONVERT
    if(strcmp(filename,FIM_STDIN_IMAGE_NAME)!=0) 
    if( pc != FIM_CNS_EMPTY_STRING )
    if( loader==FIM_NULL && can_pipe )
    {
		fim::string fpc = pc;
		fpc.substitute("[{][}]",filename);
		fpc += " | convert - ppm:" + tpfn;
		if(vl)FIM_VERB_PRINTF("About to use: %s\n", fpc.c_str());
		cc.set_status_bar(FIM_MSG_WAIT_PIPING(+fpc+), "*");

		std::system(fpc);

		if (FIM_NULL == (fp = fim_fopen(tpfn,"r")))
			cc.set_status_bar(FIM_MSG_FAILED_PIPE(+pc+), "*");
		else
		{
			unlink(tpfn);
			loader = &ppm_loader;
			if(nsp) nsp->setVariable(FIM_VID_FILE_BUFFERED_FROM,tpfn);
			if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_CONVERT);
        		goto found_a_loader;
		}
    }
#endif /* FIM_TRY_CONVERT */

#ifdef FIM_TRY_DCRAW
    if(strcmp(filename,FIM_STDIN_IMAGE_NAME)!=0) 
    if(strstr(filename,"--")!=filename) // dcraw has no -- option
    if(const auto sl = strlen(filename))
    if(sl>4)
    if(strcasestr(filename+sl-4,".NEF")==filename+sl-4)
    if( loader==FIM_NULL && can_pipe )
    {
		const fim::string fpc = fim::string(FIM_EPR_DCRAW " -c ") + filename + " > " + tpfn;
		if(vl)FIM_VERB_PRINTF("About to use: %s\n", fpc.c_str());
		cc.set_status_bar(FIM_MSG_WAIT_PIPING(+fpc+), "*");

		std::system(fpc);

		if (FIM_NULL == (fp = fim_fopen(tpfn,"r")))
			cc.set_status_bar(FIM_MSG_FAILED_PIPE(+pc+), "*");
		else
		{
			unlink(tpfn);
			loader = &ppm_loader;
			if(nsp) nsp->setVariable(FIM_VID_FILE_BUFFERED_FROM,tpfn);
			if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_DCRAW);
        		goto found_a_loader;
		}
    }
#endif /* FIM_TRY_DCRAW */

#if FIM_WITH_UFRAW
    if (FIM_NULL == loader && filename && is_file_nonempty(filename) ) /* FIXME: this is a hack */
    if(regexp_match(filename,".*NEF$") || regexp_match(filename,".*nef$")) // FIXME: use case flag
    {
        if(vl>0)FIM_VERB_PRINTF("NEF name hook.\n");
	loader = &nef_loader;
        goto found_a_loader;
    }
#endif /* FIM_WITH_UFRAW */

#if FIM_ALLOW_LOADER_STRING_SPECIFICATION
    {
    const fim::string ls=cc.getStringVariable(FIM_VID_FILE_LOADER);
    want_retry=(cc.getIntVariable(FIM_VID_RETRY_LOADER_PROBE));
    if(ls!=FIM_CNS_EMPTY_STRING)
    if(FIM_NULL==loader)/* we could have forced one */
    {
    if(vl)FIM_VERB_PRINTF("using user specified loader string: %s\n",ls.c_str());
    list_for_each(item,&loaders) {
        loader = list_entry(item, struct ida_loader, list);
    	if(vl)FIM_VERB_PRINTF("loader %s\n",loader->name);
	if (!strcmp(loader->name,ls.c_str()))
		goto found_a_loader;
    }
#if 0
#ifdef FIM_TRY_INKSCAPE
#ifdef FIM_WITH_LIBPNG 
    if(ls=="svg")
    {
	    if(vl)FIM_VERB_PRINTF("using svg loader.\n");
	    goto use_svg;
    }
#endif /* FIM_WITH_LIBPNG  */
#endif /* FIM_TRY_INKSCAPE */
#endif
    	if(vl)FIM_VERB_PRINTF("user specified loader string: %s is invalid!\n",ls.c_str());
    }
		loader = FIM_NULL;
    }
#endif /* FIM_ALLOW_LOADER_STRING_SPECIFICATION */

#if FIM_WANT_TEXT_RENDERING
    {
    	const fim_int bd=cc.getIntVariable(FIM_VID_TEXT_DISPLAY);
    	if(bd==1)
	{
		loader = &text_loader;
		goto found_a_loader;
	}
    }
#endif
#if FIM_WANT_RAW_BITS_RENDERING
    {
    const fim_int bd=cc.getIntVariable(FIM_VID_BINARY_DISPLAY);
    if(bd!=0)
    {
    	if(bd==1)
		loader = &bit1_loader;
	else
	{
    		if(bd==24)
			loader = &bit24_loader;
		else
			;// FIXME: need some error reporting
	}
    }
    }
#endif /* FIM_WANT_RAW_BITS_RENDERING */
probe_loader:
    /* pick loader */
#if FIM_WANT_MAGIC_FIMDESC
    if (FIM_NULL == loader && 0 == strncmp(blk, FIM_CNS_MAGIC_DESC, strlen(FIM_CNS_MAGIC_DESC)) )
    {
        if(vl>0)FIM_VERB_PRINTF("Seems like a FIM file list ..\n");
	cc.set_status_bar("ok, a file list...", "*");
	cc.id_.fetch(filename, fim::g_sc);
	cc.push_from_id();
	cc.setVariable(FIM_VID_COMMENT_OI,FIM_OSW_LOAD_IMG_DSC_FILE_VID_COMMENT_OI_VAL);
	goto shall_skip_header;
    }
#endif /* FIM_WANT_MAGIC_FIMDESC */
#ifdef FIM_SKIP_KNOWN_FILETYPES
    if (FIM_NULL == loader && (*blk==0x42) && (*(fim_byte_t*)(blk+1)==0x5a))
    {
        if(vl>1)FIM_VERB_PRINTF("skipping BZ2 ..\n");
	cc.set_status_bar("skipping 'bz2'...", "*");
	goto shall_skip_header;
    }
/* gz is another ! */
/*    if (FIM_NULL == loader && (*blk==0x30) && (*(fim_byte_t*)(blk+1)==0x30))
    {
	cc.set_status_bar("skipping 'gz'...", "*");
	return FIM_NULL;
    }*/
#ifndef HAVE_LIBPOPPLER
    if (FIM_NULL == loader && (*blk==0x25) && (*(fim_byte_t*)(blk+1)==0x50 )
     && FIM_NULL == loader && (*(fim_byte_t*)(blk+2)==0x44) && (*(fim_byte_t*)(blk+3)==0x46))
    {
	cc.set_status_bar("skipping 'pdf' (use fimgs for this)...", "*");
        if(vl>1)FIM_VERB_PRINTF("skipping PDF ..\n");
	goto shall_skip_header;
    }
#endif /* HAVE_LIBPOPPLER */
#ifndef HAVE_LIBSPECTRE
    if (FIM_NULL == loader && (*blk==0x25) && (*(fim_byte_t*)(blk+1)==0x21 )
     && FIM_NULL == loader && (*(fim_byte_t*)(blk+2)==0x50) && (*(fim_byte_t*)(blk+3)==0x53))
    {
        if(vl>1)FIM_VERB_PRINTF("skipping PostScript ..\n");
	cc.set_status_bar("skipping 'ps' (use fimgs for this)...", "*");
	goto shall_skip_header;
    }
#endif /* HAVE_LIBSPECTRE */
#endif /* FIM_SKIP_KNOWN_FILETYPES */ 
    /* TODO: should sort loaders by mlen, descendingly */
    if(FIM_NULL==loader)/* we could have forced one */
    list_for_each(item,&loaders)
    {
        loader = list_entry(item, struct ida_loader, list);
    	if(loader->mlen < 1)
	    continue;
	if (FIM_NULL == loader->magic)
	    break;
        if(vl>1)FIM_VERB_PRINTF("probing %s ..\n",loader->name);
	if (0 == memcmp(blk+loader->moff,loader->magic,loader->mlen))
	    break;
	loader = FIM_NULL;
    }
    if(loader!=FIM_NULL)
    {
    		if(vl)FIM_VERB_PRINTF("found loader %s by magic number\n",loader->name);
		goto found_a_loader;
    }

    if( loader==FIM_NULL && (!can_pipe || read_offset_l > 0) )
    {
    		if(vl)FIM_VERB_PRINTF("skipping external loading programs...\n");
		goto after_external_converters;
    }

#ifdef FIM_WITH_LIBPNG 
#ifdef FIM_TRY_DIA
    if(vl>1)FIM_VERB_PRINTF("probing " FIM_EPR_DIA " ..\n");
    if (FIM_NULL == loader && (*blk==0x1f) && (*(fim_byte_t*)(blk+1)==0x8b))// i am not sure if this is the FULL signature!
    {
	/* a gimp xcf file was found, and we try to use xcftopnm (fim) */
	cc.set_status_bar(FIM_MSG_WAIT_PIPING(FIM_EPR_DIA), "*");
	if(FIM_NULL!=(fp=FIM_TIMED_EXECLP(FIM_EPR_DIA,filename,"-e",tpfn.c_str(),FIM_NULL))&& 0==fim_fclose (fp))
	{
		if (FIM_NULL == (fp = fim_fopen(tpfn.c_str(),"r")))
		{
			/* this could happen in case dia was removed from the system */
			cc.set_status_bar(FIM_MSG_FAILED_PIPE(FIM_EPR_DIA), "*");
			goto shall_skip_header;
		}
		else
		{
			unlink(tpfn.c_str());
			loader = &png_loader;
			if(nsp) nsp->setVariable(FIM_VID_FILE_BUFFERED_FROM,tpfn);
			if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_DIA);
		}
   	}
   }
#endif /* FIM_TRY_DIA */
#endif /* FIM_WITH_LIBPNG  */
#ifdef FIM_TRY_XFIG
    if(vl>1)FIM_VERB_PRINTF("probing " FIM_EPR_FIG2DEV " ..\n");
    if (FIM_NULL == loader && (0 == memcmp(blk,"#FIG",4)))
    {
	cc.set_status_bar(FIM_MSG_WAIT_PIPING(FIM_EPR_FIG2DEV), "*");
	/* a xfig file was found, and we try to use fig2dev (fim) */
	if(FIM_NULL==(fp=FIM_TIMED_EXECLP(FIM_EPR_FIG2DEV,"-L","ppm",filename,FIM_NULL)))
	{
		cc.set_status_bar(FIM_MSG_FAILED_PIPE(FIM_EPR_FIG2DEV), "*");
		goto shall_skip_header;
	}
	loader = &ppm_loader;
	if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_FIG2DEV);
    }
#endif /* FIM_TRY_XFIG */
#ifdef FIM_TRY_XCFTOPNM
    if(vl>1)FIM_VERB_PRINTF("probing " FIM_EPR_XCFTOPNM " ..\n");
    if (FIM_NULL == loader && (0 == memcmp(blk,"gimp xcf file",13)))
    {
	cc.set_status_bar(FIM_MSG_WAIT_PIPING(FIM_EPR_XCFTOPNM), "*");
	/* a gimp xcf file was found, and we try to use xcftopnm (fim) */
	if(FIM_NULL==(fp=FIM_TIMED_EXECLP(FIM_EPR_XCFTOPNM,filename,FIM_NULL)))
    	{
		cc.set_status_bar(FIM_MSG_FAILED_PIPE(FIM_EPR_XCFTOPNM), "*");
		goto shall_skip_header;
	}
	loader = &ppm_loader;
	if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_XCFTOPNM);
    }
#endif /* FIM_TRY_XCFTOPNM */
#ifdef FIM_TRY_XCF2PNM
    if(vl>1)FIM_VERB_PRINTF("probing " FIM_EPR_XCF2PNM " ..\n");
    if (FIM_NULL == loader && (0 == memcmp(blk,"gimp xcf file",13)))
    {
	cc.set_status_bar(FIM_MSG_WAIT_PIPING(FIM_EPR_XCF2PNM), "*");
	/* a gimp xcf file was found, and we try to use xcf2pnm (fim) */
	if(FIM_NULL!=(fp=FIM_TIMED_EXECLP(FIM_EPR_XCF2PNM,filename,"-o",tpfn.c_str(),FIM_NULL))&&0==fim_fclose(fp))
    	{
		if (FIM_NULL == (fp = fim_fopen(tpfn,"r")))
		{
			cc.set_status_bar(FIM_MSG_FAILED_PIPE(FIM_EPR_XCF2PNM), "*");
			goto shall_skip_header;
		}
		else
		{
			unlink(tpfn);
			loader = &ppm_loader;
			if(nsp) nsp->setVariable(FIM_VID_FILE_BUFFERED_FROM,tpfn);
			if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_XCF2PNM);
		}
	}
	loader = &ppm_loader;
    }
#endif /* FIM_TRY_XCF2PNM */
//#if 0
#ifdef FIM_TRY_INKSCAPE
#ifdef FIM_WITH_LIBPNG 
    if (FIM_NULL == loader && (
		    (0 == memcmp(blk,xmlh,36) ||
		    (0 == memcmp(blk,"<svg",4))
		     )))
    //if(regexp_match(filename,".*svg$"))
    {
//use_svg:
    	/*
	 * FIXME : uses tmpfile() here. DANGER!
	 * */
	/* an svg file was found, and we try to use inkscape with it
	 * note that braindamaged inkscape doesn't export to stdout ...
	 * */
	cc.set_status_bar(FIM_MSG_WAIT_PIPING(FIM_EPR_INKSCAPE), "*");
	//sprintf(command,FIM_EPR_INKSCAPE" \"%s\" --export-png \"%s\"", filename,tpfn);
	//sprintf(command,FIM_EPR_INKSCAPE" \"%s\" --without-gui --export-png \"%s\"", filename,tpfn);
	// The following are the arguments to inkscape in order to convert an SVG into PNG:
        if(vl>1)FIM_VERB_PRINTF("probing " FIM_EPR_INKSCAPE " ..\n");
	if(FIM_NULL!=(fp=FIM_TIMED_EXECLP(FIM_EPR_INKSCAPE,filename,"--without-gui","--export-png",tpfn.c_str(),FIM_NULL))&&0==fim_fclose(fp))
	{
		if (FIM_NULL == (fp = fim_fopen(tpfn,"r")))
		{
			cc.set_status_bar(FIM_MSG_FAILED_PIPE(FIM_EPR_INKSCAPE), "*");
			goto shall_skip_header;
		}
		else
		{
			unlink(tpfn);
			loader = &png_loader;
			if(nsp) nsp->setVariable(FIM_VID_FILE_BUFFERED_FROM,tpfn);
			if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_INKSCAPE);
		}
	}
    }
#endif /* FIM_TRY_INKSCAPE */
#if 0
/*
 * Warning : this is potentially dangerous and so we wait a little before working on this.
 * */
    if((FIM_NULL == loader && (0 == memcmp(blk,"#!/usr/bin/fim",14))) ||
       (FIM_NULL == loader && (0 == memcmp(blk,"#!/usr/sbin/fim",15))) ||
       (FIM_NULL == loader && (0 == memcmp(blk,"#!/usr/local/bin/fim",20))) ||
       (FIM_NULL == loader && (0 == memcmp(blk,"#!/usr/local/sbin/fim",21)))
       )
    {
	cc.set_status_bar("loading Fim script file ...", "*");
	cc.executeFile(filename);
	return FIM_NULL;
    }
#endif
#endif /* FIM_HAVE_FULL_PROBING_LOADER */
//#endif
#ifdef FIM_TRY_CONVERT
#if 0
    // problem: pipe buffer is usually smaller than an entire ppm image :-(
    if (FIM_NULL == loader) {
        if(vl>1)FIM_VERB_PRINTF("probing " FIM_EPR_CONVERT " ..\n");
	cc.set_status_bar(FIM_MSG_WAIT_PIPING(FIM_EPR_CONVERT), "*");
	/* no loader found, try to use ImageMagick's convert */
	if(FIM_NULL==(fp=FIM_TIMED_EXECLP(FIM_EPR_CONVERT,filename,"ppm:-",FIM_NULL)))
	{
		cc.set_status_bar(FIM_MSG_FAILED_PIPE(FIM_EPR_CONVERT), "*");
		goto shall_skip_header;
	}
	loader = &ppm_loader;
    }
#else
    // note: this solution happens a few times throghout the file, and should be factored
    if (FIM_NULL == loader)
    {
	cc.set_status_bar(FIM_MSG_WAIT_PIPING(FIM_EPR_CONVERT), "*");
	if(FIM_NULL!=(fp=FIM_TIMED_EXECLP(FIM_EPR_CONVERT,filename,(std::string("ppm:")+tpfn).c_str(),FIM_NULL))&&0==fim_fclose(fp))
	{
		if (FIM_NULL == (fp = fim_fopen(tpfn,"r")))
		{
			cc.set_status_bar(FIM_MSG_FAILED_PIPE(FIM_EPR_CONVERT), "*");
			goto shall_skip_header;
		}
		else
		{
			unlink(tpfn);
			loader = &ppm_loader;
			if(nsp) nsp->setVariable(FIM_VID_FILE_BUFFERED_FROM,tpfn);
			if(nsp) nsp->setVariable(FIM_VID_EXTERNAL_CONVERTER,FIM_EPR_CONVERT);
		}
	}
    }
#endif
#endif /* FIM_TRY_CONVERT */

after_external_converters:	/* external loaders failed */

#if !FIM_HAVE_FULL_PROBING_LOADER
#ifdef HAVE_LIBGRAPHICSMAGICK
    /* FIXME: with this scheme, this is the only 0-mlen loader allowed */
    if (FIM_NULL == loader
		    && read_offset_l == 0
#if 1
		    && filename && is_file_nonempty(filename) /* FIXME: need an appropriate error/warning printout in this case */
#endif /* */
		    )
	loader = &magick_loader;
    else
	;
#endif /* HAVE_LIBGRAPHICSMAGICK */
#else /* FIM_HAVE_FULL_PROBING_LOADER */
    /* Incomplete: the problem is related to the descriptor: after the first probe, 
     * the file descriptor may not be available anymore, in case of standard input,
     * unless some more advanced solution is found.
     * */
    if(FIM_NULL==loader)
    if(rozlsl)
    list_for_each(item,&loaders)
    {
        loader = list_entry(item, struct ida_loader, list);
    	if(loader->mlen > 0)
	    continue;
	loader = FIM_NULL;
    }
#endif /* FIM_HAVE_FULL_PROBING_LOADER */

    if (FIM_NULL == loader)
	    goto head_not_found;

found_a_loader:	/* we have a loader */

    if(vl)FIM_VERB_PRINTF("using loader %s\n",loader->name);
    /* load image */
    img = (struct ida_image*)fim_calloc(1,sizeof(*img));/* calloc, not malloc: we want zeros */
    if(!img)goto errl;

#ifdef FIM_EXPERIMENTAL_ROTATION
    /* 
     * warning : there is a new field in ida_image_info (fim_extra_flags) 
     * which gets cleared to 0 (default) in this way.
     * */
#endif /* FIM_EXPERIMENTAL_ROTATION */
    // cc.set_status_bar("loading...", "*");
#if FIM_EXPERIMENTAL_IMG_NMSPC
	img->i.nsp = nsp;
#endif /* FIM_EXPERIMENTAL_IMG_NMSPC */
    data = loader->init(fp,filename,page,&img->i,0);
#ifdef FIM_READ_STDIN_IMAGE
    if(strcmp(filename,FIM_STDIN_IMAGE_NAME)==0) { close(0); if(dup(2)){/* FIXME : should we report this ?*/}/* if the image is loaded from stdin, we close its stream */}
#endif /* FIM_READ_STDIN_IMAGE */
    if (FIM_NULL == data) {
	if(vl)FIM_VERB_PRINTF("loader failed\n");
	if(FIM_DD_DEBUG())
		FIM_FBI_PRINTF("loading %s [%s] FAILED\n",filename,loader->name);
	free_image(img);
	img=FIM_NULL;
	if(want_retry)
	{
		want_retry=0;
		loader=FIM_NULL;
    		if(vl)FIM_VERB_PRINTF("retrying with probing..\n");
		goto probe_loader;
	}
	goto shall_skip_header;
    }
    img->data = fim_pm_alloc(img->i.width, img->i.height);
    if(!img->data)goto errl;
#ifndef FIM_IS_SLOWER_THAN_FBI
    for (y = 0; y < img->i.height; y++) {
	loader->read(img->data + img->i.width * 3 * y, y, data);
    }
#else /* FIM_IS_SLOWER_THAN_FBI */
    for (y = 0; y < img->i.height; y++) {
	FIM_FB_SWITCH_IF_NEEDED();
	loader->read(img->data + img->i.width * 3 * y, y, data);
    }
#endif /* FIM_IS_SLOWER_THAN_FBI */

#ifndef FIM_IS_SLOWER_THAN_FBI
    /*
     * this patch aligns the pixel bytes in the order they should
     * be dumped to the video memory, resulting in much faster image
     * drawing in fim than in fbi !
     * */
	rgb2bgr(img->data,img->i.width,y); 
#endif /* FIM_IS_SLOWER_THAN_FBI */
    loader->done(data);
#if FIM_WITH_ARCHIVE
    if(npages)
	    img->i.npages = npages; /* FIXME: temporarily here */
#endif /* FIM_WITH_ARCHIVE */
#if FIM_WANT_REMEMBER_LAST_FILE_LOADER
    if(img && loader)
    {
	if(nsp)
		nsp->setVariable(FIM_VID_FILE_LOADER,loader->name);
	cc.setVariable(FIM_VID_LAST_FILE_LOADER,loader->name);
    }
#endif /* FIM_WANT_REMEMBER_LAST_FILE_LOADER */
#if FIM_WANT_EXPERIMENTAL_PLUGINS
    	if(img)
		fim_post_read_plugins_exec(img,filename);
#endif /* FIM_WANT_EXPERIMENTAL_PLUGINS */

#if FIM_WANT_RESIZE_HUGE_AFTER_LOAD
    	if(cc.getIntVariable(FIM_VID_RESIZE_HUGE_ON_LOAD)==1)
	if( cc.current_viewport() && img->i.width>0 && img->i.width>0 )
	{
		const fim_scale_t mf = FIM_CNS_HUGE_IMG_TO_VIEWPORT_PROPORTION; // max factor (of image to viewport)
		const fim_scale_t ef = FIM_CNS_HUGE_IMG_TO_VIEWPORT_PROPORTION; // estimation factor (of max viewport to current window)
		const fim_scale_t mswe = ef*cc.current_viewport()->viewport_width(); // max screen width estimate
		const fim_scale_t mshe = ef*cc.current_viewport()->viewport_height(); // max screen height estimate
		const fim_scale_t ws = FIM_INT_SCALE_FRAC(img->i.width  , mf*mswe);
		const fim_scale_t hs = FIM_INT_SCALE_FRAC(img->i.height , mf*mshe);

		if( ws > FIM_CNS_SCALEFACTOR_ONE && hs > FIM_CNS_SCALEFACTOR_ONE )
		{
			// std::cout << img->i.width << " " << img->i.height << std::endl;
			// std::cout << ws << " " << hs << std::endl;
			const fim_scale_t sf = floor(FIM_MIN(ws,hs));
			struct ida_image *simg = FbiStuff::scale_image(img,1.0/sf,FIM_CNS_SCALEFACTOR_ONE
#if FIM_WANT_MIPMAPS
					,FIM_NULL
#endif /* FIM_WANT_MIPMAPS */
					);
			if(simg)
			{
	    			if(vl)
					FIM_VERB_PRINTF("image is huge (%d x %d): scaled it down to %d x %d\n", img->i.height, img->i.width, simg->i.height, simg->i.width);
		       		FbiStuff::free_image(img);
				img=simg;
			}
		}
	}
#endif /* FIM_WANT_RESIZE_HUGE_AFTER_LOAD */
    goto ret;

shall_skip_header:
head_not_found: /* no appropriate loader found for this image */
    img=FIM_NULL;
    if( read_offset_u > read_offset_l )
    {
	    read_offset_l++;
	    if(vl)
		    FIM_VERB_COUT << "file seek range adjusted to: " << read_offset_l << ":" << read_offset_u << "\n";
	    goto with_offset;
    }
errl:
    if(img && img->data)fim_free(img->data);
    if(img )fim_free(img);
#if FIM_SHALL_BUFFER_STDIN
    if(sbuf)fim_free(sbuf);
#endif /* FIM_SHALL_BUFFER_STDIN */
ret:
    if( read_offset_l > 0 && nsp )
	    nsp->setVariable(FIM_VID_OPEN_OFFSET_L,read_offset_l);
    FIM_PR('.');
    return img;
}

/*
 * crop the image to a rectangle
 * */
#if FIM_WANT_CROP
struct ida_image*
FbiStuff::crop_image(struct ida_image *src, ida_rect rect)
{
    struct ida_image *dest;
    void *data;
    unsigned int y;
    struct ida_op *desc_p;

    dest =(ida_image*) fim_malloc(sizeof(*dest));
    /* fim: */ if(!dest)goto err;
    fim_bzero(dest,sizeof(*dest));
    
    desc_p=&desc_crop;

    data = desc_p->init(src,&rect,&dest->i,NULL);
    dest->data = fim_pm_alloc(dest->i.width, dest->i.height);
    /* fim: */ if(!(dest->data)){fim_free(dest);dest=FIM_NULL;goto err;}
    for (y = 0; y < dest->i.height; y++) {
	FIM_FB_SWITCH_IF_NEEDED();
	desc_p->work(src,&rect,
			 dest->data + 3 * dest->i.width * y,
			 y, data);
    }
    desc_p->done(data);
err:
    return dest;
}
#endif /* FIM_WANT_CROP */

/*
 * rotate the image 90 degrees (M_PI/2) at a time (fim)
 * */
struct ida_image*
FbiStuff::rotate_image90(struct ida_image *src, unsigned int rotation)
{
    /* 0: CCW, 1: CW */
    struct op_resize_parm p;
    struct ida_rect  rect;
    struct ida_image *dest;
    void *data;
    unsigned int y;
    struct ida_op *desc_p;

    dest =(ida_image*) fim_malloc(sizeof(*dest));
    /* fim: */ if(!dest)goto err;
    fim_bzero(dest,sizeof(*dest));
    fim_bzero(&rect,sizeof(rect));
    fim_bzero(&p,sizeof(p));
    
    p.width  = src->i.width;
    p.height = src->i.height;
    p.dpi    = src->i.dpi;
    if (0 == p.width)
	p.width = 1;
    if (0 == p.height)
	p.height = 1;
    
    rotation%=2;
    if(rotation==0){desc_p=&desc_rotate_ccw;}
    else	   {desc_p=&desc_rotate_cw ;}

    data = desc_p->init(src,&rect,&dest->i,&p);
    dest->data = fim_pm_alloc(dest->i.width, dest->i.height);
    /* fim: */ if(!(dest->data)){fim_free(dest);dest=FIM_NULL;goto err;}
    for (y = 0; y < dest->i.height; y++) {
	FIM_FB_SWITCH_IF_NEEDED();
	desc_p->work(src,&rect,
			 dest->data + 3 * dest->i.width * y,
			 y, data);
    }
    desc_p->done(data);
err:
    return dest;
}

struct ida_image*	
FbiStuff::rotate_image(struct ida_image *src, float angle)
{
    /*
     * this whole code was originally (in fbi) not meant to change canvas.
     * */
    struct op_rotate_parm p;
    /* fim's 20080831 */
    struct ida_rect  rect;
    struct ida_image *dest;
    void *data;

    dest = (ida_image*)fim_malloc(sizeof(*dest));
    /* fim: */ if(!dest)goto err;
    fim_bzero(dest,sizeof(*dest));
    fim_bzero(&rect,sizeof(rect));
    fim_bzero(&p,sizeof(p));

    /* source rectangle */
    rect.x1=0;
    rect.x2=src->i.width;
    rect.y1=0;
    rect.y2=src->i.height;
    
#ifdef FIM_EXPERIMENTAL_ROTATION
    if(! src->i.fim_extra_flags)
    {
    /*
     * this is code for a preliminary 'canvas' enlargement prior to image rotation.
     * unfinished code.
     * */   
    fim_off_t diagonal = (fim_off_t) FIM_HYPOTHENUSE_OF_INT(src->i.width,src->i.height);
    fim_off_t n_extra  = (diagonal - src->i.height  )/2;
    fim_off_t s_extra  = (diagonal - src->i.height - n_extra     );
    fim_off_t w_extra  = (diagonal - src->i.width      )/2;
    fim_off_t e_extra  = (diagonal - src->i.width - w_extra  );
    /* we allocate a new, larger canvas */
    fim_byte_t * larger_data = (fim_byte_t*)fim_calloc(diagonal * diagonal * 3,1);
    if(larger_data)
    {
	    for(fim_off_t y = n_extra; y < diagonal - s_extra; ++y )
	    	memcpy(larger_data + (y * diagonal + w_extra )*3 , src->data + (y-n_extra) * src->i.width * 3 , src->i.width*3);
	    src->i.width = diagonal;
	    src->i.height = diagonal;
	    /* source rectangle fix */
	    rect.x1+=w_extra;
	    rect.x2+=e_extra;
	    rect.y1+=n_extra;
	    rect.y2+=s_extra;
	    fim_free(src->data);
	    src->data=larger_data;
    	    src->i.fim_extra_flags=1;	/* to avoid this operation to repeat on square images or already rotated images */
    }
    /* on allocation failure (e.g.: a very long and thin image) we cannot do more.
     * uh, maybe we could tell the user about the allocation failure..*/
    else
    	cc.set_status_bar( "rescaling failed (insufficient memory?!)", "*");
    }
#endif /* FIM_EXPERIMENTAL_ROTATION */

    p.angle    = (int) angle;
    data = desc_rotate.init(src,&rect,&dest->i,&p);
    dest->data = fim_pm_alloc(dest->i.width, dest->i.height, true);
    /* fim: */ if(!(dest->data)){fim_free(dest);dest=FIM_NULL;goto err;}
    for (fim_uint y = 0; y < dest->i.height; y++) {
	FIM_FB_SWITCH_IF_NEEDED();
	desc_rotate.work(src,&rect,
			 dest->data + 3 * dest->i.width * y,
			 y, data);
    }

    desc_rotate.done(data);
err:
    return dest;
}


#define FIM_OPTIMIZATION_20120129 1

#if FIM_WANT_MIPMAPS
static int find_mipmap_idx(struct ida_image & msrc, const fim_mipmap_t * mmp, fim_int width, fim_int height)
{

	int mmi;

	for(mmi=0;mmi<mmp->nmm && mmp->mmw[mmi]>=width && mmp->mmh[mmi]>=height ;++mmi)
	{
		msrc.i.width  = mmp->mmw[mmi];
		msrc.i.height = mmp->mmh[mmi];
		msrc.data     = mmp->mdp + mmp->mmoffs[mmi];
	}
	return mmi;
}
#endif /* FIM_WANT_MIPMAPS */
/*
int is_power_of_two(float val)
{
	int mmi = 0;

	if(val < 0)
		val = -val;

	if(val > 1.0)
		val = 1.0/val;

	while( val / 2.0 >= 1.0 )
		val /= 2.0, --mmi;
}
*/

struct ida_image*	
FbiStuff::scale_image(const struct ida_image *src, /*const fim_mipmap_t *mmp,*/ float scale, float ascale
#if FIM_WANT_MIPMAPS
		, const fim_mipmap_t * mmp
#endif /* FIM_WANT_MIPMAPS */
		)
{
    struct op_resize_parm p;
    struct ida_rect  rect;
    struct ida_image *dest=FIM_NULL;
    void *data=FIM_NULL;
    unsigned int y;
#if FIM_WANT_MIPMAPS
    int mmi=-1;
    struct ida_image msrc;
#endif /* FIM_WANT_MIPMAPS */
    if(ascale<=0.0||ascale>=100.0) /* fim: */
	    ascale=1.0;

    dest = (ida_image*)fim_malloc(sizeof(*dest));
    /* fim: */ if(!dest)
	    goto err;
    fim_bzero(dest,sizeof(*dest));
    fim_bzero(&rect,sizeof(rect));
    fim_bzero(&p,sizeof(p));
 
    p.width  = (int)ceilf((float)src->i.width  * scale * ascale);
    p.height = (int)ceilf((float)src->i.height * scale);
    p.dpi    = (int)(src->i.dpi);

    if (0 == p.width)
	p.width = 1;
    if (0 == p.height)
	p.height = 1;
   
#if FIM_WANT_MIPMAPS
    if(mmp && ascale == 1.0 && scale < 1.0)
    {
	msrc = *src;
	mmi = find_mipmap_idx(msrc, mmp, p.width, p.height);
	if(mmi>0)
	{
		src=&msrc;
		mmi--;
		if(FIM_WVMM) std::cout << "for scale " << scale << std::endl;
		if(FIM_WVMM) std::cout << "using mipmap " << mmi << " / " << mmp->nmm << std::endl;
		if(FIM_WVMM) std::cout << mmp->mmw[mmi] << " x " << mmp->mmh[mmi] << "" << std::endl;
		if(FIM_WVMM) std::cout << p.width << " x " << p.height << " -> " << std::endl;
	}
	else
	{
		if(FIM_WVMM) std::cout << "for scale " << scale << std::endl;
		if(FIM_WVMM) std::cout << "not using mipmap " << std::endl;
	}
    }
#endif /* FIM_WANT_MIPMAPS */

    data = desc_resize.init(src,&rect,&dest->i,&p);
    if(data==FIM_NULL)
    {
	fim_free(dest);
    	goto err;
    }
    dest->data = fim_pm_alloc(dest->i.width, dest->i.height);
    if(!(dest->data))
    {
    	    fim_char_t s[FIM_STATUSLINE_BUF_SIZE];
	    sprintf(s, "Failed allocating a %d x %d pixelmap.", dest->i.width, dest->i.height);
#if FIM_ALLOW_LOADER_VERBOSITY
    	    const fim_int vl=(cc.getIntVariable(FIM_VID_VERBOSITY));
#else /* FIM_ALLOW_LOADER_VERBOSITY */
    	    const fim_int vl=0;
#endif /* FIM_ALLOW_LOADER_VERBOSITY */
	    if(vl>0)FIM_VERB_PRINTF("%s\n",s);
	    fim::cout << s << "\n"; 
	    cc.set_status_bar(s, "*");
	    fim_free(data);
	    fim_free(dest);
	    goto err;
    }

#if FIM_WANT_MIPMAPS
    if(mmi>=0 && msrc.i.width == dest->i.width && msrc.i.height == dest->i.height )
    {
	if(FIM_WVMM) std::cout << "using mipmap without scaling" << std::endl;
	memcpy(dest->data,src->data,fbi_img_pixel_bytes(dest)); /* a special case */
	goto done;
    }
#endif /* FIM_WANT_MIPMAPS */

#if FIM_OPTIMIZATION_20120129
    if(ascale==scale && ascale==1.0)
	    memcpy(dest->data,src->data,fbi_img_pixel_bytes(dest)); /* a special case */
    else
#endif /* FIM_OPTIMIZATION_20120129 */
    for (y = 0; y < dest->i.height; y++) {
	FIM_FB_SWITCH_IF_NEEDED();
	desc_resize.work(src,&rect,
			 dest->data + 3 * dest->i.width * y,
			 y, data);
    }
done:
    desc_resize.done(data);
err:
    return dest;
}

#if FIM_WANT_OBSOLETE
struct ida_image * fbi_image_black(fim_coo_t w, fim_coo_t h)
{
	// TODO: use fbi_image_black in fbi_image_clone !
	struct ida_image *nimg=FIM_NULL;
	fim_coo_t n;

	if(!(nimg=(ida_image*)fim_calloc(1,sizeof(struct ida_image))))
		goto err;

	nimg->i.width=w;
       	nimg->i.height=h;
	n = fbi_img_pixel_bytes(nimg);
	
	nimg->data = (fim_byte_t*)fim_calloc(1, n );

	if(!(nimg->data))
	{
		fim_free(nimg);
		nimg = FIM_NULL;
		goto err;
	}
err:
	return nimg;
}
#endif /* FIM_WANT_OBSOLETE */

fim_pxc_t fbi_img_pixel_count(const struct ida_image *img)
{
	if(!img)
		return 0;
	return img->i.width*img->i.height;
}

fim_pxc_t fbi_img_pixel_bytes(const struct ida_image *img)
{
	return 3 * fbi_img_pixel_count(img);
}

struct ida_image * fbi_image_clone(const struct ida_image *img)
{
	/* note that to fulfill free_image(), the descriptor and data couldn't be allocated together
	 * */
	struct ida_image *nimg=FIM_NULL;

	if(!img || !img->data)
		goto err;
	int n;
	if(!(nimg=(ida_image*)fim_calloc(1,sizeof(struct ida_image))))
		goto err;

	memcpy(nimg,img,sizeof(struct ida_image));
	/*note .. no checks .. :P */
	n = img->i.width * img->i.height * 3;
	
	nimg->data = (fim_byte_t*)fim_malloc( n );
	if(!(nimg->data))
	{
		fim_free(nimg);
		nimg = FIM_NULL;
		goto err;
	}
	memcpy(nimg->data, img->data,n);
err:
	return nimg;
}

	int FbiStuff::fim_filereading_debug(void)
	{
		return FIM_DD_DEBUG();
	}

	bool FbiStuff::want_fbi_style_debug(void)
	{
		return FIM_WANT_FBI_INNER_DIAGNOSTICS;
	}
} /* namespace fim */

