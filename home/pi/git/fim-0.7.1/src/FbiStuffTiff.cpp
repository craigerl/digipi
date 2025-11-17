/* $LastChangedDate: 2024-03-22 16:34:38 +0100 (Fri, 22 Mar 2024) $ */
/*
 FbiStuffTiff.cpp : fbi functions for TIFF files, modified for fim

 (c) 2007-2024 Michele Martone
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
#include <inttypes.h>
#include <tiffio.h>

#include "FbiStuff.h"
#include "FbiStuffLoader.h"
#ifdef USE_X11
 #include "viewer.h"
#endif /* USE_X11 */

#define FIM_TIFF_ERRVAL -1
namespace fim
{

struct tiff_state {
    TIFF*          tif;
    //char           emsg[FIM_LIBERR_BUFSIZE];
    tdir_t         ndirs;     /* Number of directories                     */
                              /* (could be interpreted as number of pages) */
    uint32_t         width,height;
    uint16_t         config,nsamples,depth,fillorder,photometric;
    uint32_t*        row;
    uint32_t*        image;
    uint16_t         resunit;
    float          xres,yres;
};

static void*
tiff_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	  struct ida_image_info *i, int thumbnail)
{
    struct tiff_state *h=FIM_NULL;

    fclose(fp);
    h = (struct tiff_state *) fim_calloc(1,sizeof(*h));
    if(!h)goto oops;

    TIFFSetWarningHandler(FIM_NULL);
    h->tif = TIFFOpen(filename,"r");
    if (FIM_NULL == h->tif)
	goto oops;
    /* Determine number of directories */
    h->ndirs = 1;
    while (TIFFReadDirectory(h->tif))
        h->ndirs++;
    i->npages = h->ndirs;
    /* Select requested directory (page) */
    if (!TIFFSetDirectory(h->tif, (tdir_t)page))
        goto oops;
    if(FIM_TIFF_ERRVAL==TIFFGetField(h->tif, TIFFTAG_IMAGEWIDTH,      &h->width))
			goto oops;
    if(FIM_TIFF_ERRVAL==TIFFGetField(h->tif, TIFFTAG_IMAGELENGTH,     &h->height))
			goto oops;
    if(FIM_TIFF_ERRVAL==TIFFGetField(h->tif, TIFFTAG_PLANARCONFIG,    &h->config))
			goto oops;
    if(FIM_TIFF_ERRVAL==TIFFGetField(h->tif, TIFFTAG_SAMPLESPERPIXEL, &h->nsamples))
			goto oops;
    if(FIM_TIFF_ERRVAL==TIFFGetField(h->tif, TIFFTAG_BITSPERSAMPLE,   &h->depth))
			goto oops;
    if(FIM_TIFF_ERRVAL==TIFFGetField(h->tif, TIFFTAG_FILLORDER,       &h->fillorder))
			goto oops;
    if(FIM_TIFF_ERRVAL==TIFFGetField(h->tif, TIFFTAG_PHOTOMETRIC,     &h->photometric))
			goto oops;
    h->row = (uint32_t*)fim_malloc(TIFFScanlineSize(h->tif));
    if(!h->row)goto oops;
    if (FbiStuff::fim_filereading_debug())
#ifndef FIM_PRId32
#define FIM_PRId32 "x"
#endif /* FIM_PRId32 */
	FIM_FBI_PRINTF("tiff: %" FIM_PRId32 "x%" FIM_PRId32 ", planar=%d, "
		"nsamples=%d, depth=%d fo=%d pm=%d scanline=%" FIM_PRId32 "\n",
//	FIM_FBI_PRINTF("tiff: %" "%d" "x%" "%d" ", planar=%d, "
//		"nsamples=%d, depth=%d fo=%d pm=%d scanline=%" "%d" "\n",
		h->width,h->height,h->config,h->nsamples,h->depth,
		h->fillorder,h->photometric,
		TIFFScanlineSize(h->tif));

    if (PHOTOMETRIC_PALETTE   == h->photometric  ||
	PHOTOMETRIC_YCBCR     == h->photometric  ||
	PHOTOMETRIC_SEPARATED == h->photometric  ||
	TIFFIsTiled(h->tif)                      ||
	(1 != h->depth  &&  8 != h->depth)) {
	/* for the more difficuilt cases we let libtiff
	 * do all the hard work.  Drawback is that we lose
	 * progressive loading and decode everything here */
	if (FbiStuff::fim_filereading_debug())
	    FIM_FBI_PRINTF("tiff: reading whole image [TIFFReadRGBAImage]\n");
	h->image=(uint32_t*)fim_malloc(4*h->width*h->height);
        if(!h->image)goto oops;
    } else {
	if (FbiStuff::fim_filereading_debug())
	    FIM_FBI_PRINTF("tiff: reading scanline by scanline\n");
    }

    i->width  = h->width;
    i->height = h->height;

    if (TIFFGetField(h->tif, TIFFTAG_RESOLUTIONUNIT,  &h->resunit) &&
	TIFFGetField(h->tif, TIFFTAG_XRESOLUTION,     &h->xres)    &&
	TIFFGetField(h->tif, TIFFTAG_YRESOLUTION,     &h->yres)) {
	switch (h->resunit) {
	case RESUNIT_NONE:
	    break;
	case RESUNIT_INCH:
	    i->dpi = (unsigned int)h->xres;
	    break;
	case RESUNIT_CENTIMETER:
	    i->dpi = (unsigned int)res_cm_to_inch(h->xres);
	    break;
	}
    }

    return h;

 oops:
    if (h && h->tif)
	TIFFClose(h->tif);
    if(h && h->row)fim_free(h->row);
    if(h && h->image)fim_free(h->image);
    if(h)fim_free(h);
    return FIM_NULL;
}

static void
tiff_read(fim_byte_t *dst, unsigned int line, void *data)
{
    struct tiff_state *h = (struct tiff_state *) data;
    int s,on,off;

    if (h->image) {
	/* loaded whole image using TIFFReadRGBAImage() */
	uint32_t *row = h->image + h->width * (h->height - line -1);
	load_rgba(dst,(fim_byte_t*)row,h->width);
	return;
    }
    
    if (h->config == PLANARCONFIG_CONTIG) {
	TIFFReadScanline(h->tif, h->row, line, 0);
    } else if (h->config == PLANARCONFIG_SEPARATE) {
	for (s = 0; s < h->nsamples; s++)
	    TIFFReadScanline(h->tif, h->row, line, s);
    }

    switch (h->nsamples) {
    case 1:
	if (1 == h->depth) {
	    /* black/white */
	    on = 0, off = 0;
	    if (PHOTOMETRIC_MINISWHITE == h->photometric)
		on = 0, off = 255;
	    if (PHOTOMETRIC_MINISBLACK == h->photometric)
		on = 255, off = 0;
#if 0
	    /* Huh?  Does TIFFReadScanline handle this already ??? */
	    if (FILLORDER_MSB2LSB == h->fillorder)
		load_bits_msb(dst,(fim_byte_t*)(h->row),h->width,on,off);
	    else
		load_bits_lsb(dst,(fim_byte_t*)(h->row),h->width,on,off);
#else
	    load_bits_msb(dst,(fim_byte_t*)(h->row),h->width,on,off);
#endif
	} else {
	    /* grayscaled */
	    load_gray(dst,(fim_byte_t*)(h->row),h->width);
	}
	break;
    case 3:
	/* rgb */
	memcpy(dst,h->row,3*h->width);
	break;
    case 4:
	/* rgb+alpha */
	load_rgba(dst,(fim_byte_t*)(h->row),h->width);
	break;
    }
}

static void
tiff_done(void *data)
{
    struct tiff_state *h = (struct tiff_state *) data;

    TIFFClose(h->tif);
    if (h->row)
	fim_free(h->row);
    if (h->image)
	fim_free(h->image);
    fim_free(h);
}

static struct ida_loader tiff1_loader = {
    /*magic:*/ "MM\x00\x2a",
    /*moff:*/  0,
    /*mlen:*/  4,
    /*name:*/  "libtiff-MM",
    /*init:*/  tiff_init,
    /*read:*/  tiff_read,
    /*done:*/  tiff_done,
};
static struct ida_loader tiff2_loader = {
    /*magic:*/ "II\x2a\x00",
    /*moff:*/  0,
    /*mlen:*/  4,
    /*name:*/  "libtiff-II",
    /*init:*/  tiff_init,
    /*read:*/  tiff_read,
    /*done:*/  tiff_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&tiff1_loader);
    fim_load_register(&tiff2_loader);
}

#ifdef USE_X11
/* ---------------------------------------------------------------------- */
/* save                                                                   */

static int
tiff_write(FILE *fp, struct ida_image *img)
{
    TIFF          *TiffHndl;
    tdata_t       buf;
    unsigned int  y;

    TiffHndl = TIFFFdOpen(fileno(fp),"42.tiff","w");
    if (TiffHndl == FIM_NULL)
	return -1;
    TIFFSetField(TiffHndl, TIFFTAG_IMAGEWIDTH, img->i.width);
    TIFFSetField(TiffHndl, TIFFTAG_IMAGELENGTH, img->i.height);
    TIFFSetField(TiffHndl, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(TiffHndl, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(TiffHndl, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(TiffHndl, TIFFTAG_SAMPLESPERPIXEL, 3);
    TIFFSetField(TiffHndl, TIFFTAG_ROWSPERSTRIP, 2);
    TIFFSetField(TiffHndl, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
#if 0 /* fixme: make this configureable */
    TIFFSetField(TiffHndl, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(TiffHndl, TIFFTAG_PREDICTOR, 2);
#endif
    if (img->i.dpi) {
	float dpi = img->i.dpi;
	TIFFSetField(TiffHndl, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	TIFFSetField(TiffHndl, TIFFTAG_XRESOLUTION,    dpi);
	TIFFSetField(TiffHndl, TIFFTAG_YRESOLUTION,    dpi);
    }

    for (y = 0; y < img->i.height; y++) {
	buf = img->data + 3*img->i.width*y;
	TIFFWriteScanline(TiffHndl, buf, y, 0);
    }
    TIFFClose(TiffHndl);
    return 0;
}

static struct ida_writer tiff_writer = {
    /*  label:*/  "TIFF",
    /*  ext:*/    { "tif", "tiff", FIM_NULL},
    /*  write:*/  tiff_write,
};

static void fim__init init_wr(void)
{
    fim_write_register(&tiff_writer);
}

#endif /* USE_X11 */
}

