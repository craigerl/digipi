/* $LastChangedDate: 2022-11-22 13:08:47 +0100 (Tue, 22 Nov 2022) $ */
/*
 FbiStuffMagick.cpp : fim functions for decoding image files using libGraphicsMagick

 (c) 2011-2022 Michele Martone

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

/* FIXME: This code is very inefficient, so please regard it as temporary */
/* FIXME: Error handling is incomplete */
/* TODO: CatchException fprintf's stuff: it is not adequate error reporting; we would prefer an error string instead  */
/* Tested with MagickLibVersion defined as 0x090600 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "../config.h"
#include "fim_types.h"
#include "fim_wrappers.h"
#include "FbiStuffList.h"
#include "FbiStuffLoader.h"

#ifdef HAVE_LIBGRAPHICSMAGICK
#include <magick/api.h>

/* versions as far as e.g. 1.13.12 have this bug, in coders/txt.c:328, due to a FIM_NULL p returning in case of empty text file */
#ifdef MagickLibVersion
#define HAVE_LIBGRAPHICSMAGICK_TXT_FILEXTENSION_BUG (MagickLibVersion<=0x151200) /* 0x090600 was previously supposed to fix the problem but it did not, seemingly (as of 1.3.20) ... still:
 fim: magick/semaphore.c:531: LockSemaphoreInfo: Assertion `semaphore_info != (SemaphoreInfo *) ((void *)0)' failed.
 Aborted
*/
#else /* MagickLibVersion */
#define HAVE_LIBGRAPHICSMAGICK_TXT_FILEXTENSION_BUG 1
#endif /* MagickLibVersion */

struct magick_state_t {
	ImagePtr image; /* Warning: this is NOT to be confused with fim's Image class */
	ImagePtr cimage; /* Warning: this is NOT to be confused with fim's Image class */
	ImageInfo* image_info;
	MagickPassFail mpf; 
	ExceptionInfo exception;
	struct fim::ida_image_info *i; 
};

static struct magick_state_t ms; // TODO: get rid of this

namespace fim
{

static void magick_cleanup()
{
	if(ms.image)DestroyImageList(ms.image);
	if(ms.i->npages>1)
		if(ms.cimage)DestroyImageList(ms.cimage);
	if(ms.image_info)DestroyImageInfo(ms.image_info);
	//DestroyExceptionInfo(ms.exception);
	DestroyMagick();
}

static void*
magick_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	  struct ida_image_info *i, int thumbnail)
{
	fim_bzero(&ms,sizeof(ms));
	ms.mpf=MagickFail;
    	ms.i = i;

	if(!fp && !filename)
		goto nocleanuperr;
	if(!filename)
		goto nocleanuperr;
	if(!fp)
		goto nocleanuperr;

#if HAVE_LIBGRAPHICSMAGICK_TXT_FILEXTENSION_BUG
	if(filename)
	{
		fim_size_t fnl=strlen(filename);
		if(fnl>=3 && 0==strcasecmp(filename+fnl-3,"txt"))
			goto nocleanuperr;
		if(fnl>=4 && 0==strcasecmp(filename+fnl-4,"text"))
			goto nocleanuperr;
	}
#endif /* HAVE_LIBGRAPHICSMAGICK_TXT_FILEXTENSION_BUG */
	InitializeMagick(filename);
	GetExceptionInfo(&ms.exception);
	if (ms.exception.severity != UndefinedException)
	{
        	//CatchException(&ms.exception);
		goto err;
	}
	ms.image_info=CloneImageInfo((ImageInfo *) FIM_NULL);
	if(ms.image_info==FIM_NULL)
		goto err;

	/* FIXME need correctness check on dimensions values ! */
	if(strlen(filename)>MaxTextExtent-1)
		goto err;
	(void) strncpy(ms.image_info->filename,filename,MaxTextExtent-1);
	ms.image_info->filename[MaxTextExtent-1]='\0';
	ms.image=ReadImage(ms.image_info,&ms.exception);
	if (ms.exception.severity != UndefinedException)
	{
        	//CatchException(&ms.exception);
		goto err;
	}
	if(!ms.image)
		goto err;
	i->npages = GetImageListLength(ms.image); /* ! */
	i->width  = (ms.image)->columns;
	i->height = (ms.image)->rows;

	if(page>=i->npages || page<0)goto err;

	if(i->width<1 || i->height<1 || i->npages<1)
		goto err;

	ms.cimage=GetImageFromList(ms.image,page);
	if (ms.exception.severity != UndefinedException)
	{
 		//CatchException(&ms.exception);
	}
	if(!ms.cimage)
	{
		goto err;
	}
	return &ms;
err:
	magick_cleanup();
nocleanuperr:
	return FIM_NULL; 
}

static void
magick_read(fim_byte_t *dst, unsigned int line, void *data)
{
	/* note: inefficient */
	for(unsigned int c=0;c<(ms.cimage)->columns;++c)
	{
		PixelPacket pp=AcquireOnePixel( ms.cimage, c, line, &ms.exception );
		dst[c*3+0]=pp.red;
		dst[c*3+1]=pp.green;
		dst[c*3+2]=pp.blue;
	}
}

static void
magick_done(void *data)
{
	magick_cleanup();
}

/*
 not a filetype-specific decoder
*/
struct ida_loader magick_loader = {
    /*magic:*/ "",
    /*moff:*/  0,
    /*mlen:*/  0,
    /*name:*/  "libGraphicsMagick",
    /*init:*/  magick_init,
    /*read:*/  magick_read,
    /*done:*/  magick_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&magick_loader);
}

}
#endif /* HAVE_LIBGRAPHICSMAGICK */
