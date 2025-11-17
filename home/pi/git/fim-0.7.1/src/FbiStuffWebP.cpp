/* $LastChangedDate: 2023-10-06 13:48:28 +0200 (Fri, 06 Oct 2023) $ */
/*
 FbiStuffWebP.cpp : fim functions to decode WebP files using libwebp

 (c) 2023-2023 Michele Martone
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

#if FIM_WITH_LIBWEBP

#include <webp/decode.h> 
#include <webp/demux.h> 

#include <fstream> // ifstream
#include <stdio.h>
#include <vector>
#include <cstdlib>
#include <string.h>
#include <errno.h>

namespace fim
{
extern CommandConsole cc;

/* ---------------------------------------------------------------------- */
static fim_err_t webp_load_image_fp(FILE *fp, unsigned int page, unsigned char * rgb, int bytes_per_line)
{
	// bogus function
	return FIM_ERR_NO_ERROR;
}
/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct webp_state {
	FILE *fp{};
	int w; /* image width: 1... */
	int h; /* image height: 1... */
	int np; /* number of pages: 1... */
    	size_t flen; /* file length */
	fim_byte_t*rgb{}; /* pixels, from upper left to lower right, line by line */
	int bytes_per_line; /* rgb has bytes_per_line bytes per line */
	WebPDemuxer* demux{};
	WebPIterator iter{};
};

static void*
webp_init(FILE *fp, const fim_char_t *filename, unsigned int page, struct ida_image_info *i, int thumbnail)
{
	/* it is safe to ignore filename, page, thumbnail */
	struct webp_state *h = FIM_NULL;
    	fim_err_t errval = FIM_ERR_GENERIC;
	long fo = 0;
	std::vector<uint8_t> wd;

	h = (struct webp_state *)fim_calloc(1,sizeof(*h));

	if(!h)
		goto oops;

    	h->fp = fp;

	if(fseek(h->fp,0,SEEK_END)!=0)
		goto oops;

    	fo = ftell(h->fp);

	if(fseek(fp,0,SEEK_SET) != 0)
		goto oops;

    	if( fo == -1 )
		goto oops;

	h->flen = fo; /* FIXME: evil conversion */
	wd.resize(fo);

	if(fread(wd.data(), fo, 1, fp) != 1)
		goto oops;

	{
		const WebPData webp_data {wd.data(), static_cast<size_t>(fo)};
		h->demux = WebPDemux(&webp_data);
		if(!h->demux)
			goto oops;

		if (WebPDemuxGetFrame(h->demux, page+1, &h->iter))
		{
			int width, height;
			h->rgb = WebPDecodeRGB(wd.data(), fo, &width, &height);

			i->npages = h->np = 1;
			h->w = i->width = width;
			h->h = i->height = height;
			h->bytes_per_line = i->width * 3;

			if(!h->rgb)
			{
				std::cout << "Failed fim_malloc!\n";
				goto oops;
			}
		}
	}

	errval = webp_load_image_fp(h->fp, page, h->rgb, h->bytes_per_line);
	if(errval != FIM_ERR_NO_ERROR)
	{
		std::cout << "Failed webp_load_image_fp!\n";
		goto oops;
	}

	return h;
oops:
	if(h)
	{
		if(h->demux)
 			WebPFree(h->rgb),
			WebPDemuxReleaseIterator(&h->iter),
 			WebPDemuxDelete(h->demux);
		fim_free(h);
	}
	return FIM_NULL;
}

static void
webp_read(fim_byte_t *dst, unsigned int line, void *data)
{
	struct webp_state *h = (struct webp_state *) data;
	int c;

	for (c=0;c<h->w;++c)
	{
		dst[c*3+0] = h->rgb[h->bytes_per_line*line+c*3+0];
		dst[c*3+1] = h->rgb[h->bytes_per_line*line+c*3+1];
		dst[c*3+2] = h->rgb[h->bytes_per_line*line+c*3+2];
	}
}

static void
webp_done(void *data)
{
	struct webp_state *h = (struct webp_state *) data;

	if(h->demux)
 		WebPFree(h->rgb),
		WebPDemuxReleaseIterator(&h->iter),
 		WebPDemuxDelete(h->demux);
	fclose(h->fp);
	fim_free(h);
}

struct ida_loader webp_loader = {
/*
 * 0000000: 5249 4646 02ec 0000			RIFF
 */
    /*magic:*/ "RIFF",
    /*moff:*/  0,
    /*mlen:*/  4,
    /*name:*/  "libwebp",
    /*init:*/  webp_init,
    /*read:*/  webp_read,
    /*done:*/  webp_done,
};

static void fim__init init_rd(void)
{
	fim_load_register(&webp_loader);
}

}
#endif /* FIM_WITH_LIBWEBP */
