/* $LastChangedDate: 2024-03-22 16:34:38 +0100 (Fri, 22 Mar 2024) $ */
/*
 FbiStuffAVIF.cpp : fim functions to decode AVIF files using libavif.

 (c) 2023-2024 Michele Martone
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
#include "avif/avif.h"

#if FIM_WITH_LIBAVIF

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
/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct AVIF_state {
	FILE *fp;
	int w; /* image width: 1... */
	int h; /* image height: 1... */
	int np; /* number of pages: 1... */
    	size_t flen; /* file length */
	int bytes_per_line; /* rgb has bytes_per_line bytes per line */
    	avifRGBImage aviffimg;
    	avifDecoder * decoder{};
};

static void
AVIF_done(void *data)
{
	// ok as cleanup function after AVIF_init

	struct AVIF_state *h = (struct AVIF_state *) data;

	avifRGBImageFreePixels(&h->aviffimg); // frees memory allocated by avifRGBImageAllocatePixels()
	avifDecoderDestroy(h->decoder);

	if(h->fp)
		fclose(h->fp);
	fim_free(h);
}

static void*
AVIF_init(FILE *fp, const fim_char_t *filename, unsigned int page, struct ida_image_info *i, int thumbnail)
{
	/* it is safe to ignore filename, page, thumbnail */
	struct AVIF_state *h = FIM_NULL;
	long fo = 0;
	avifResult result;
	avifDecoder * decoder {};

	h = (struct AVIF_state *)fim_calloc(1,sizeof(*h));
	if(!h)
		return FIM_NULL;

	decoder = h->decoder = avifDecoderCreate();
	avifRGBImage & rgb = h->aviffimg;

	result = avifDecoderSetIOFile(decoder, filename);
	if (result != AVIF_RESULT_OK) {
		fprintf(stderr, "Cannot open file for read: %s\n", filename);
		goto oops;
	}

	result = avifDecoderParse(decoder);
	if (result != AVIF_RESULT_OK) {
		fprintf(stderr, "Failed to decode AVIF image: %s\n", avifResultToString(result));
		goto oops;
	}

	i->npages = decoder->imageCount;
	h->w = i->width = h->decoder->image->width;
       	h->h = i->height = h->decoder->image->height;

	if (FbiStuff::fim_filereading_debug())
		printf("avif: parsed AVIF: %ux%u (%ubpc)\n", decoder->image->width, decoder->image->height, decoder->image->depth);
	
	for (unsigned int pagei=0; pagei < page; ++pagei)
		if (avifDecoderNextImage(decoder) != AVIF_RESULT_OK)
		{
			fprintf(stderr, "Problem opening page: %d\n", page);
			goto oops;
		}

	if (avifDecoderNextImage(decoder) == AVIF_RESULT_OK)
       	{
		avifRGBImageSetDefaults(&rgb, decoder->image);
		rgb.depth = 8;
		rgb.format = AVIF_RGB_FORMAT_RGB; // so that avifRGBImagePixelSize is 3 and not 4
		avifRGBImageAllocatePixels(&rgb);
		h->bytes_per_line = i->width * avifRGBImagePixelSize(&rgb);

		result = avifImageYUVToRGB(decoder->image, &rgb);
		if (result != AVIF_RESULT_OK) {
			fprintf(stderr, "Conversion from YUV failed: %s (%s)\n", filename, avifResultToString(result));
			goto oops;
		}

		if (FbiStuff::fim_filereading_debug())
			printf("avif: Image: %d/%d:\n", decoder->imageIndex, decoder->imageCount);

		if (rgb.depth > 8)
		{
			fprintf(stderr, "Image depth %d > 8\n", rgb.depth);
			goto oops;
		}
	}
       	else
       	{
		fprintf(stderr, "avif: seemingly no image\n");
		goto oops;
	}

    	h->fp = fp;

	if(fseek(h->fp,0,SEEK_END)!=0)
		goto oops;

    	fo = ftell(h->fp);
	h->flen = fo; /* FIXME: evil conversion */
    	if( fo == -1 )
		goto oops;
	
	return h;
oops:
	if(h)
	{
		AVIF_done(h);
	}
	return FIM_NULL;
}

static void
AVIF_read(fim_byte_t *dst, unsigned int line, void *data)
{
	struct AVIF_state *h = (struct AVIF_state *) data;
    	const auto rgb = h->aviffimg.pixels;
	int c;

	for (c=0;c<h->w;++c)
	{
		dst[c*3+0] = rgb[h->bytes_per_line*line+c*3+0];
		dst[c*3+1] = rgb[h->bytes_per_line*line+c*3+1];
		dst[c*3+2] = rgb[h->bytes_per_line*line+c*3+2];
	}
}

struct ida_loader AVIF_loader = {
/*
 * 0000000: 0000 0020 6674 7970 6176 6966 0000 0000  ... ftypavif....
 * 0000000: 0000 002c 6674 7970 6176 6973 0000 0000  ...,ftypavis....
 */
    /*magic:*/ "ftypavi",
    /*moff:*/  4,
    /*mlen:*/  7,
    /*name:*/  "libavif",
    /*init:*/  AVIF_init,
    /*read:*/  AVIF_read,
    /*done:*/  AVIF_done,
};

static void fim__init init_rd(void)
{
	fim_load_register(&AVIF_loader);
}

}
#endif /* FIM_WITH_LIBAVIF */
