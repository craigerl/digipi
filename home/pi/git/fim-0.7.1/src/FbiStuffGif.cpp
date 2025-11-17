/* $LastChangedDate: 2022-11-21 01:19:31 +0100 (Mon, 21 Nov 2022) $ */
/*
 FbiStuffGif.cpp : fbi functions for GIF files, modified for fim

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
#include <gif_lib.h>

//#include "loader.h"
#include "FbiStuff.h"
#include "FbiStuffLoader.h"

#if defined(GIFLIB_MAJOR) && (GIFLIB_MAJOR >= 5)
#define FIM_GIFLIB_STATE_HAS_ERRCODE 1
#else
#define FIM_GIFLIB_STATE_HAS_ERRCODE 0
#endif

#if defined(GIFLIB_MAJOR) && ((GIFLIB_MAJOR> 4) || ((GIFLIB_MAJOR==4) && defined(GIFLIB_MINOR) && (GIFLIB_MINOR>=2)))
#define FIM_GIFLIB_RETIRED_PrintGifError 1
#else
#define FIM_GIFLIB_RETIRED_PrintGifError 0
#endif

#if defined(GIFLIB_MAJOR) && ((GIFLIB_MAJOR> 5) || ((GIFLIB_MAJOR==5) && defined(GIFLIB_MINOR) && (GIFLIB_MINOR>=1)))
#define FIM_DGifCloseFile_ARG ,FIM_NULL
#else
#define FIM_DGifCloseFile_ARG
#endif

namespace fim
{
struct gif_state {
    FILE         *infile;
    GifFileType  *gif;
    GifPixelType *row;
    GifPixelType *il;
    int w,h;
    int ErrorCode; /* used by newer versions (e.g.: >= 5) of library */
};

#if FIM_GIFLIB_RETIRED_PrintGifError  
void
FimPrintGifError(struct gif_state * gs) {
#if defined(GIFLIB_MAJOR) && (GIFLIB_MAJOR==4) && defined(GIFLIB_MINOR) && (GIFLIB_MINOR>=2)
    int ErrorCode = GifError();// introduced in 4.2, removed in 5.0
#else
    int ErrorCode = gs->gif->Error;
#endif
    /* On the basis of giflib-5.0.5/util/qprintf.c suggestion, after retirement of PrintGifError  */
    const char *Err = FIM_NULL;

    if(ErrorCode == 0)   
	    return;

#if ( defined(GIFLIB_MAJOR) && (GIFLIB_MAJOR >= 5) )
    Err = GifErrorString(ErrorCode);// Introduced in 4.2; argument from 5.0.0.
#else
    Err = GifErrorString();// introduced in 4.2
#endif
                                                                                                                              
    if (Err != FIM_NULL)
        fprintf(stderr, "GIF-LIB error: %s.\n", Err);
    else
        fprintf(stderr, "GIF-LIB undefined error %d.\n", ErrorCode);
}
#else
	/* Version 4.2 retired the PrintGifError function. */
#define FimPrintGifError(GS) PrintGifError
#endif /* FIM_GIFLIB_RETIRED_PrintGifError */

static GifRecordType
gif_fileread(struct gif_state *h)
{
    GifRecordType RecordType;
    GifByteType *Extension = FIM_NULL;
    int ExtCode = 0, rc = 0;
#if FIM_WANT_FBI_PRINTF
    const fim_char_t *type = FIM_NULL;
#endif /* FIM_WANT_FBI_PRINTF */

    for (;;) {
	if (GIF_ERROR == DGifGetRecordType(h->gif,&RecordType)) {
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("gif: DGifGetRecordType failed\n");
	    FimPrintGifError(h);
	    return (GifRecordType)-1;
	}
	switch (RecordType) {
	case IMAGE_DESC_RECORD_TYPE:
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("gif: IMAGE_DESC_RECORD_TYPE found\n");
	    return RecordType;
	case EXTENSION_RECORD_TYPE:
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("gif: EXTENSION_RECORD_TYPE found\n");
	    for (rc = DGifGetExtension(h->gif,&ExtCode,&Extension);
		 FIM_NULL != Extension;
		 rc = DGifGetExtensionNext(h->gif,&Extension)) {
		if (rc == GIF_ERROR) {
		    if (FbiStuff::fim_filereading_debug())
			FIM_FBI_PRINTF("gif: DGifGetExtension failed\n");
		    FimPrintGifError(h);
		    return (GifRecordType)-1;
		}
#if FIM_WANT_FBI_PRINTF
		if (FbiStuff::fim_filereading_debug()) {
		    switch (ExtCode) {
		    case COMMENT_EXT_FUNC_CODE:     type="comment";   break;
		    case GRAPHICS_EXT_FUNC_CODE:    type="graphics";  break;
		    case PLAINTEXT_EXT_FUNC_CODE:   type="plaintext"; break;
		    case APPLICATION_EXT_FUNC_CODE: type="appl";      break;
		    default:                        type="???";       break;
		    }
		    FIM_FBI_PRINTF("gif: extcode=0x%x [%s]\n",ExtCode,type);
		}
#endif /* FIM_WANT_FBI_PRINTF */
	    }
	    break;
	case TERMINATE_RECORD_TYPE:
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("gif: TERMINATE_RECORD_TYPE found\n");
	    return RecordType;
	default:
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("gif: unknown record type [%d]\n",RecordType);
	    return (GifRecordType)-1;
	}
    }
}

#if 0
static void
gif_skipimage(struct gif_state *h)
{
    fim_byte_t *line;
    int i;

    if (FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("gif: skipping image record ...\n");
    DGifGetImageDesc(h->gif);
    line = fim_malloc(h->gif->SWidth);
    for (i = 0; i < h->gif->SHeight; i++)
	DGifGetLine(h->gif, line, h->gif->SWidth);
    fim_free(line);
}
#endif

static void*
gif_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	 struct ida_image_info *info, int thumbnail)
{
    struct gif_state *h = FIM_NULL;
    GifRecordType RecordType;
    int i, image = 0;
    
    h = (gif_state*)fim_calloc(1,sizeof(*h));
    if(!h)goto oops;

    h->infile = fp;
#if FIM_GIFLIB_STATE_HAS_ERRCODE
    h->gif = DGifOpenFileHandle(fileno(fp),&h->ErrorCode);	/* TODO: shall make use of h->ErrorCode */
#else
    h->gif = DGifOpenFileHandle(fileno(fp));
#endif
    if(!h->gif)goto oops; /* opening gifs from stdin seems to cause DGifOpenFileHandle=FIM_NULL */
    h->row = (GifPixelType*)fim_malloc(h->gif->SWidth * sizeof(GifPixelType));
    if(!h->row)goto oops;

    while (0 == image) {
	RecordType = gif_fileread(h);
	switch (RecordType) {
	case IMAGE_DESC_RECORD_TYPE:
	    if (GIF_ERROR == DGifGetImageDesc(h->gif)) {
		if (FbiStuff::fim_filereading_debug())
		    FIM_FBI_PRINTF("gif: DGifGetImageDesc failed\n");
		FimPrintGifError(h);
	    }
	    if (FIM_NULL == h->gif->SColorMap &&
		FIM_NULL == h->gif->Image.ColorMap) {
		if (FbiStuff::fim_filereading_debug())
		    FIM_FBI_PRINTF("gif: oops: no colormap found\n");
		goto oops;
	    }
#if 0
	    info->width  = h->w = h->gif->SWidth;
	    info->height = h->h = h->gif->SHeight;
#else
	    info->width  = h->w = h->gif->Image.Width;
	    info->height = h->h = h->gif->Image.Height;
#endif
            info->npages = 1;
	    image = 1;
	    if (FbiStuff::fim_filereading_debug())
		FIM_FBI_PRINTF("gif: reading image record ...\n");
	    if (h->gif->Image.Interlace) {
		if (FbiStuff::fim_filereading_debug())
		    FIM_FBI_PRINTF("gif: interlaced\n");
		{
			h->il = (GifPixelType*)fim_malloc(h->w * h->h * sizeof(GifPixelType));
    			if(!h->il)goto oops;
		}
		for (i = 0; i < h->h; i += 8)
		    DGifGetLine(h->gif, h->il + h->w*i,h->w);
		for (i = 4; i < h->gif->SHeight; i += 8)
		    DGifGetLine(h->gif, h->il + h->w*i,h->w);
		for (i = 2; i < h->gif->SHeight; i += 4)
		    DGifGetLine(h->gif, h->il + h->w*i,h->w);
	    }
	    break;
	case TERMINATE_RECORD_TYPE:
	default:
	    goto oops;
	}
    }
    if (0 == info->width || 0 == info->height)
	goto oops;

    if (FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("gif: s=%dx%d i=%dx%d\n",
		h->gif->SWidth,h->gif->SHeight,
		h->gif->Image.Width,h->gif->Image.Height);
    return h;

 oops:
    if (FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("gif: fatal error, aborting\n");
    if(h->gif)
    {
    	DGifCloseFile(h->gif FIM_DGifCloseFile_ARG);
    }
    fclose(h->infile);
    if(h && h->il )fim_free(h->il );
    if(h && h->row)fim_free(h->row);
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
gif_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct gif_state *h = (struct gif_state *) data;
    GifColorType *cmap;
    int x;
    
    if (h->gif->Image.Interlace) {
	if (line % 2) {
	    DGifGetLine(h->gif, h->row, h->w);
	} else {
	    memcpy(h->row, h->il + h->w * line, h->w);
	}
    } else {
	DGifGetLine(h->gif, h->row, h->w);
    }
    cmap = h->gif->Image.ColorMap ?
	h->gif->Image.ColorMap->Colors : h->gif->SColorMap->Colors;
    for (x = 0; x < h->w; x++) {
        dst[0] = cmap[h->row[x]].Red;
	dst[1] = cmap[h->row[x]].Green;
	dst[2] = cmap[h->row[x]].Blue;
	dst += 3;
    }
}

static void
gif_done(void *data)
{
    struct gif_state *h = (struct gif_state *) data;

    if (FbiStuff::fim_filereading_debug())
	FIM_FBI_PRINTF("gif: done, cleaning up\n");
    DGifCloseFile(h->gif FIM_DGifCloseFile_ARG);
    fclose(h->infile);
    if (h->il)
	fim_free(h->il);
    fim_free(h->row);
    fim_free(h);
}

static struct ida_loader gif_loader = {
    /*magic:*/ "GIF",
    /*moff:*/  0,
    /*mlen:*/  3,
    /*name:*/  "libungif",
    /*init:*/  gif_init,
    /*read:*/  gif_read,
    /*done:*/  gif_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&gif_loader);
}

}
