/* $LastChangedDate: 2022-10-30 16:13:52 +0100 (Sun, 30 Oct 2022) $ */
/*
 FbiStuffPng.cpp : fbi functions for PNG files, modified for fim

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

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#include <png.h>

//#include "loader.h"
#include "FbiStuff.h"
#include "FbiStuffLoader.h"
#ifdef USE_X11
 #include "viewer.h"
#endif /* USE_X11 */
namespace fim
{

#if FIM_WANT_FBI_PRINTF 
static const fim_char_t *ct[] = {
    "gray",  "X1", "rgb",  "palette",
    "graya", "X5", "rgba", "X7",
};
#endif /* FIM_WANT_FBI_PRINTF */

struct fim_png_state {
    FILE         *infile;
    png_structp  png;
    png_infop    info;
    png_bytep    image;
    png_uint_32  w,h;
    int          color_type;
    struct ida_image_info *i; 
    char * cmt;
};

static FILE*fim_png_fp;
void PNGAPI fim_png_rw_ptr(png_structp s, png_bytep p, png_size_t l)
{
	fim_fread(p, l, 1, fim_png_fp);
}

void fim_png_rd_cmts(void *data, png_infop info)
#ifdef PNG_WRITE_tEXt_SUPPORTED
    {
    	struct fim_png_state *h = (struct fim_png_state *) data;
	png_textp text_ptr = FIM_NULL;
	const int num_comments = png_get_text(h->png, info, &text_ptr, FIM_NULL);
	int ti = 0;
	fim::string fs;

	for (ti=0;ti<num_comments;++ti)
	{
		if( text_ptr[ti].compression == PNG_TEXT_COMPRESSION_NONE || text_ptr[ti].compression == PNG_ITXT_COMPRESSION_NONE )
		{
	    		fs+=text_ptr[ti].key;
	    		fs+=":";
	    		fs+=text_ptr[ti].text;
	    		if( ti < num_comments && num_comments > 1 )
			       	fs+=" ";
		}
	}

	if(num_comments>0)
	if(fs.c_str() && strlen(fs.c_str()))
	{
		const char * s = fs.c_str();

		if(!h->cmt)
			h->cmt = (char*) fim_calloc(strlen(s)+1,1);
		else
		{
			h->cmt = (char*) fim_realloc(h->cmt,strlen(h->cmt)+strlen(s)+1);
			if(h->cmt)
				strcat(h->cmt," ");
		}

		if(h->cmt)
		{
			strcat(h->cmt,s);
		}
	}
    }
#else
   {
   }
#endif

static void*
png_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	 struct ida_image_info *i, int thumbnail)
{
    struct fim_png_state *h;
    int bit_depth, interlace_type;
    int pass, number_passes;
    unsigned int y;
    png_uint_32 resx, resy;
    /*
    png_color_16 *file_bg, my_bg = {
	.red   = 192,
	.green = 192,
	.blue  = 192,
	.gray  = 192,
    };*/
    png_color_16 *file_bg, my_bg ;
	my_bg .red   = 192;
	my_bg .green = 192;
	my_bg .blue  = 192;
	my_bg .gray  = 192;
    int unit;
    
    h = (struct fim_png_state *) fim_calloc(1,sizeof(*h));
    if(!h) goto oops;

    h->infile = fp;
    h->i = i;

    h->png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
				    FIM_NULL, FIM_NULL, FIM_NULL);
    if (FIM_NULL == h->png)
	goto oops;
    h->info = png_create_info_struct(h->png);
    if (FIM_NULL == h->info)
	goto oops;

    fim_png_fp=fp;
#if defined(PNG_LIBPNG_VER) && (PNG_LIBPNG_VER>=10249)
    /* in the above, check on version >=10249 is not sufficient, not necessary. for e.g; 10606 it's necessary */
    png_set_read_fn(h->png,FIM_NULL,fim_png_rw_ptr); /* TODO: shall make use of return second argument */
#else
    h->png->read_data_fn=&fim_png_rw_ptr;
#endif
    png_init_io(h->png, h->infile);
    png_read_info(h->png, h->info);
    png_get_IHDR(h->png, h->info, &h->w, &h->h,
		 &bit_depth,&h->color_type,&interlace_type, FIM_NULL,FIM_NULL);
    png_get_pHYs(h->png, h->info, &resx, &resy, &unit);
    i->width  = h->w;
    i->height = h->h;
    if (PNG_RESOLUTION_METER == unit)
	i->dpi = res_m_to_inch(resx);
    if (FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("png: color_type=%s #1\n",ct[h->color_type]);
    i->npages = 1;
    
    png_set_packing(h->png);
    if (bit_depth == 16)
	png_set_strip_16(h->png);
    if (h->color_type == PNG_COLOR_TYPE_PALETTE)
	png_set_palette_to_rgb(h->png);
    if (h->color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
// according to http://www.libpng.org/pub/png/libpng-manual.txt, retrieved 20101129
#ifdef PNG_LIBPNG_VER
 #if (PNG_LIBPNG_VER>=10209)
	png_set_expand_gray_1_2_4_to_8(h->png);
 #else
	png_set_gray_1_2_4_to_8(h->png);
 #endif /* PNG_LIBPNG_VER */
#else
 #error  need a proper function name for png_set_gray_1_2_4_to_8, here.
#endif /* PNG_LIBPNG_VER */
    if (png_get_bKGD(h->png, h->info, &file_bg)) {
	png_set_background(h->png,file_bg,PNG_BACKGROUND_GAMMA_FILE,1,1.0);
    } else {
	png_set_background(h->png,&my_bg,PNG_BACKGROUND_GAMMA_SCREEN,0,1.0);
    }

    number_passes = png_set_interlace_handling(h->png);
    png_read_update_info(h->png, h->info);

    h->color_type = png_get_color_type(h->png, h->info);
    if (FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("png: color_type=%s #2\n",ct[h->color_type]);
    
    h->image = (png_byte*)fim_malloc(i->width * i->height * 4);
    if(!h->image) goto oops;

    for (pass = 0; pass < number_passes-1; pass++) {
	if (FbiStuff::fim_filereading_debug())
	    FIM_FBI_PRINTF("png: pass #%d\n",pass);
	for (y = 0; y < i->height; y++) {
	    png_bytep row = h->image + y * i->width * 4;
	    png_read_rows(h->png, &row, FIM_NULL, 1);
	}
    }

    fim_png_rd_cmts(h,h->info);

    return h;

 oops:
    if (h && h->image)
	fim_free(h->image);
    if (h->png)
	png_destroy_read_struct(&h->png, FIM_NULL, FIM_NULL);
    fim_fclose(h->infile);
    if(h->cmt)
    	fim_free(h->cmt);
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
png_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct fim_png_state *h = (struct fim_png_state *) data;

    png_bytep row = h->image + line * h->w * 4;
    switch (h->color_type) {
    case PNG_COLOR_TYPE_GRAY:
	png_read_rows(h->png, &row, FIM_NULL, 1);
	load_gray(dst,row,h->w);
	break;
    case PNG_COLOR_TYPE_RGB:
	png_read_rows(h->png, &row, FIM_NULL, 1);
	memcpy(dst,row,3*h->w);
	break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
	png_read_rows(h->png, &row, FIM_NULL, 1);
	load_rgba(dst,row,h->w);
	break;
    case PNG_COLOR_TYPE_GRAY_ALPHA:
	png_read_rows(h->png, &row, FIM_NULL, 1);
	load_graya(dst,row,h->w);
	break;
    default:
	/* shouldn't happen */
	FIM_FBI_PRINTF("Oops: %s:%d\n",__FILE__,__LINE__);
	exit(1);
    }
}

static void
png_done(void *data)
{
    struct fim_png_state *h =(struct fim_png_state *) data;
    png_infop end_info = png_create_info_struct(h->png);

    png_read_end(h->png, end_info );
    fim_png_rd_cmts(h,end_info);

    if(h->cmt)
    {
	load_add_extra(h->i,EXTRA_COMMENT,(fim_byte_t*)h->cmt,strlen(h->cmt));
    	fim_free(h->cmt);
	load_free_extras(h->i);
    }

    fim_free(h->image);
    png_destroy_read_struct(&h->png, &h->info, &end_info);
    fim_fclose(h->infile);
    fim_free(h);
}

//used in FbiStuff.cpp
#ifdef FIM_WITH_LIBPNG 
struct ida_loader png_loader
#else
static struct ida_loader png_loader
#endif /* FIM_WITH_LIBPNG */
= {
    /*magic:*/ "\x89PNG",
    /*moff:*/  0,
    /*mlen:*/  4,
    /*name:*/  "libpng",
    /*init:*/  png_init,
    /*read:*/  png_read,
    /*done:*/  png_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&png_loader);
}

#ifdef USE_X11
/* ---------------------------------------------------------------------- */
/* save                                                                   */

static int
png_write(FILE *fp, struct ida_image *img)
{
    png_structp png_ptr = FIM_NULL;
    png_infop info_ptr  = FIM_NULL;
    png_bytep row;
    unsigned int y;
    
   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump based method,
    * you can supply FIM_NULL for the last three parameters.  We also check that
    * the library version is compatible with the one used at compile time,
    * in case we are using dynamically linked libraries.  REQUIRED.
    */
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,FIM_NULL,FIM_NULL,FIM_NULL);
    if (png_ptr == FIM_NULL)
	goto oops;

   /* Allocate/initialize the image information data.  REQUIRED */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == FIM_NULL)
       goto oops;
   if (setjmp(png_jmpbuf(png_ptr)))
       goto oops;

   png_init_io(png_ptr, fp);
   png_set_IHDR(png_ptr, info_ptr, img->i.width, img->i.height, 8,
		PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
   if (img->i.dpi) {
       png_set_pHYs(png_ptr, info_ptr,
		    res_inch_to_m(img->i.dpi),
		    res_inch_to_m(img->i.dpi),
		    PNG_RESOLUTION_METER);
   }
   png_write_info(png_ptr, info_ptr);
   png_set_packing(png_ptr);

   for (y = 0; y < img->i.height; y++) {
       row = img->data + y * 3 * img->i.width;
       png_write_rows(png_ptr, &row, 1);
   }
   png_write_end(png_ptr, info_ptr);
   png_destroy_write_struct(&png_ptr, &info_ptr);
   return 0;

 oops:
   FIM_FBI_PRINTF("can't save image: libpng error\n");
   if (png_ptr)
       png_destroy_write_struct(&png_ptr,  (png_infopp)FIM_NULL);
   return -1;
}

static struct ida_writer png_writer = {
    /*label:*/  "PNG",
   /* ext:*/    { "png", FIM_NULL},
    /*write:*/  png_write,
};

static void fim__init init_wr(void)
{
    fim_write_register(&png_writer);
}


#endif /* USE_X11 */
}
