/* $LastChangedDate: 2022-10-30 16:13:52 +0100 (Sun, 30 Oct 2022) $ */
/*
 FbiStuffXyz.cpp : An example file for reading new file types with hypothetical library libjp2.

 (c) 2014-2022 Michele Martone
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

#if FIM_WITH_LIBJASPER
/* Experimental, preliminar, unfinished support for JPEG-2K (JPEG-2000) files loading.
 * Error handling and performance shall be dealt with...
 *
 * A few references:
 *  http://www.ece.uvic.ca/~frodo/jasper
 *  http://www.jpeg.org/jpeg2000/index.html
 *  https://tools.ietf.org/html/rfc3745
 */

#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#ifdef HAVE_ENDIAN_H
 #include <endian.h>
#endif /* HAVE_ENDIAN_H */
#include <jasper/jasper.h>

namespace fim
{

extern CommandConsole cc;

typedef unsigned char fim_byte_t ;

#define JP2_ERR -1
#define JP2_OK 0

#define	jp2_vctocc(i, co, cs) \
  (( (i) - (co)) / (cs))

static int jp2_image_render(jas_image_t *image, int vw, int vh, fim_byte_t *vdata)
{
	/* Derived from jas_image_render in the  jasper-1.900.1/src/appl/jiv.c example. */
	int i;
	int j;
	int k;
	int x;
	int y;
	int v[3];
	fim_byte_t *vdatap;
	int cmptlut[3];
	int width;
	int height;
	int hs;
	int vs;
	int tlx;
	int tly;
	FIM_CONSTEXPR int sc = 256;

	if ((cmptlut[0] = jas_image_getcmptbytype(image,
	  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_R))) < 0 ||
	  (cmptlut[1] = jas_image_getcmptbytype(image,
	  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_G))) < 0 ||
	  (cmptlut[2] = jas_image_getcmptbytype(image,
	  JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_RGB_B))) < 0)
		goto error;

	width = jas_image_cmptwidth(image, cmptlut[0]);
	height = jas_image_cmptheight(image, cmptlut[0]);
	tlx = jas_image_cmpttlx(image, cmptlut[0]);
	tly = jas_image_cmpttly(image, cmptlut[0]);
	vs = jas_image_cmptvstep(image, cmptlut[0]);
	hs = jas_image_cmpthstep(image, cmptlut[0]);
	for (i = 1; i < 3; ++i) {
		if (jas_image_cmptwidth(image, cmptlut[i]) != width ||
		  jas_image_cmptheight(image, cmptlut[i]) != height)
			goto error;
	}

	for (i = 0; i < vh; ++i) {
		vdatap = &vdata[( i) * (3 * vw)];
		for (j = 0; j < vw; ++j) {
			x = jp2_vctocc(j, tlx, hs);
			y = jp2_vctocc(i, tly, vs);
			for (k = 0; k < 3; ++k) {
				v[k] = jas_image_readcmptsample(image, cmptlut[k], x, y);
				v[k] <<= 16 - jas_image_cmptprec(image, cmptlut[k]);
				if (v[k] < 0) {
					v[k] = 0;
				} else if (v[k] > 65535) {
					v[k] = 65535;
				}
			}
			v[0]/=sc;
			v[1]/=sc;
			v[2]/=sc;
			*vdatap++ = v[0];
			*vdatap++ = v[1];
			*vdatap++ = v[2];
		}
	}
	return JP2_OK;
error:
	return JP2_ERR;
}

struct jp2_state {
	FILE *fp;
	int w; /* image width: 1... */
	int h; /* image height: 1... */
    	size_t flen; /* file length */
	fim_byte_t*rgb; /* pixels, from upper left to lower right, line by line */
	int bytes_per_line; /* rgb has bytes_per_line bytes per line */
	jas_stream_t *in;
	jas_image_t *image;
	jas_cmprof_t *outprof;
};

static void*
jp2_init(FILE *fp, const fim_char_t *filename, unsigned int page, struct ida_image_info *i, int thumbnail)
{
	/* it is safe to ignore filename, page, thumbnail */
	struct jp2_state *h = FIM_NULL;
    	fim_err_t errval = FIM_ERR_GENERIC;
	long fo = 0;

	h = (struct jp2_state *)fim_calloc(1,sizeof(*h));

	if(!h)
		goto oops;

    	h->fp = fp;

	if(fseek(h->fp,0,SEEK_END)!=0)
		goto oops;

    	fo = ftell(h->fp);
	h->flen = fo; /* FIXME: evil conversion */
    	if( fo == -1 )
		goto oops;

	if (jas_init())
	{
		goto oops;
	}

/*
see also:
jas_stream_t *jas_stream_fopen(const char *filename, const char *mode);
jas_stream_t *jas_stream_memopen(char *buf, int bufsize);
jas_stream_t *jas_stream_fdopen(int fd, const char *mode);
jas_stream_t *jas_stream_freopen(const char *path, const char *mode, FILE *fp);
*/

	if(filename)
	{
		if (!(h->in = jas_stream_fopen(filename, "rb")))
		{
			fprintf(stderr, "error: cannot open file %s\n", filename);
			goto oops;
		}
	}
	else
		goto oops;

	if (!(h->image = jas_image_decode(h->in, -1, 0)))
	{
		fprintf(stderr, "error: cannot load image data\n");
		goto oops;
	}
#if 0
	if (!(h->outprof = jas_cmprof_createfromclrspc(JAS_CLRSPC_SRGB)))
	{
		goto oops;
	}
	jas_image_t *altimage;
	if (!(altimage = jas_image_chclrspc(h->image, outprof, JAS_CMXFORM_INTENT_PER)))
		goto oops;*/
	fprintf(stderr, "num of components %d\n", jas_image_numcmpts(h->image));
	fprintf(stderr, "dimensions %d %d\n", jas_image_width(h->image), jas_image_height(h->image));
#endif

	i->width = jas_image_width(h->image);
	i->height = jas_image_height(h->image);
	i->npages = 1;

	h->rgb = (fim_byte_t*) fim_calloc( i->width * i->height * 3 , sizeof(fim_byte_t));

	if(!h->rgb)
	{
		std::cout << "Failed fim_calloc!\n";
    		goto oops;
	}

	if( JP2_OK != jp2_image_render(h->image, i->width, i->height, h->rgb) )
	{
		goto oops;
	}

	h->w = i->width;
       	h->h = i->height;
	h->bytes_per_line = i->width * 3;

	if(!h->rgb)
	{
		std::cout << "Failed fim_malloc!\n";
    		goto oops;
	}

	jas_stream_close(h->in);

	errval = FIM_ERR_NO_ERROR;
	if(errval != FIM_ERR_NO_ERROR)
	{
		std::cout << "Failed jp2_load_image_fp!\n";
		goto oops;
	}

	jas_cleanup();

	return h;
oops:
	if(h)
	{
		if(h->in)
			jas_stream_close(h->in);
		if(h->rgb)
			fim_free(h->rgb);
		fim_free(h);
	}
	jas_cleanup();
	return FIM_NULL;
}

static void
jp2_read(fim_byte_t *dst, unsigned int line, void *data)
{
	/* this gets called every line. can be left empty if useless. */
	struct jp2_state *h = (struct jp2_state *) data;
	int c;

	for (c=0;c<h->w;++c)
	{
		dst[c*3+0] = h->rgb[h->bytes_per_line*line+c*3+0];
		dst[c*3+1] = h->rgb[h->bytes_per_line*line+c*3+1];
		dst[c*3+2] = h->rgb[h->bytes_per_line*line+c*3+2];
	}
}

static void
jp2_done(void *data)
{
	struct jp2_state *h = (struct jp2_state *) data;

	fclose(h->fp);
	fim_free(h->rgb);
	fim_free(h);
}

struct ida_loader jp2_loader = {
/*
 * 12 chars, of which 3 heading zeros
 * 0000000: 0000 000c 6a50 2020 0d0a 870a   ....jP  ....
 * (https://www.iana.org/assignments/media-types/image/jp2)
 */
    /*magic:*/ "\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a",
    /*moff:*/  3,
    /*mlen:*/  9,
    /*name:*/  "jp2",
    /*init:*/  jp2_init,
    /*read:*/  jp2_read,
    /*done:*/  jp2_done,
};

static void fim__init init_rd(void)
{
	fim_load_register(&jp2_loader);
}

}
#endif /* FIM_WITH_LIBJASPER */
