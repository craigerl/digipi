/* $LastChangedDate: 2024-05-09 00:45:30 +0200 (Thu, 09 May 2024) $ */
/*
 FbiStuffQoi.cpp : open QOI files

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

#if FIM_WITH_QOI

#include <stdio.h>
#define QOI_IMPLEMENTATION 1 /* so that functions are inlined here */
#include <qoi.h>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#ifdef HAVE_ENDIAN_H
 #include <endian.h>
#endif /* HAVE_ENDIAN_H */

namespace fim
{
/* ---------------------------------------------------------------------- */
struct qoi_state {
	FILE *fp{};
	int width{}; /* image width: 1... */
	int height{}; /* image height: 1... */
	int np{1}; /* number of pages: 1... */
    	size_t flen{}; /* file length */
	fim_byte_t*rgb{}; /* libqoi-allocated pixels (malloc/free), from upper left to lower right, line by line */
	void*fbd{}; /* file buffered data */
	int bytes_per_line{}; /* rgb has bytes_per_line bytes per line */
};

static void*
fim_qoi_init(FILE *fp, const fim_char_t *filename, unsigned int page, struct ida_image_info *i, int thumbnail)
{
	/* it is safe to ignore filename, page, thumbnail */
	struct qoi_state *h = FIM_NULL;
    	//fim_err_t errval = FIM_ERR_GENERIC;
	qoi_desc desc;
	long fo = 0;

	h = (struct qoi_state *)fim_calloc(1,sizeof(*h));

	if(!h)
		goto oops;

	if(fseek(fp,0,SEEK_END)!=0)
		goto oops;
	if(ftell(fp)==-1)
		goto oops;

    	h->fp = fp;

     	fo = ftell(h->fp);
	h->flen = fo; /* FIXME: evil conversion */

	if(fseek(fp,0,SEEK_SET)!=0)
		goto oops;

    	if( fo == -1 )
		goto oops;
	if (fp)
		h->fbd = fim_calloc(1, fo);

	if (h->fbd && fo > 0)
	{
		// note that filename may be FIM_STDIN_IMAGE_NAME
		desc.channels = 3;
    		desc.colorspace = QOI_SRGB;
		fread(h->fbd, 1, fo, fp);
		h->rgb = (fim_byte_t*) qoi_decode(h->fbd, fo, &desc, 3);
	}
	else
	{
		// not foreseen by calling arguments...
		// h->rgb = (fim_byte_t*) qoi_read(filename, &desc, 3);
	}
	if ( ! h->rgb )
		goto oops;
	h->height = i->height = desc.height;
	h->width = i->width = desc.width;
	h->bytes_per_line = desc.width*3;
	return h;
oops:
	if(h)
	{
		if(h->rgb)
			free(h->rgb); // libqoi
		fim_free(h);
	}
	return FIM_NULL;
}

static void
fim_qoi_read(fim_byte_t *dst, unsigned int line, void *data)
{
	struct qoi_state *h = (struct qoi_state *) data;

	if ( line == 0 && data )
	{
		memcpy(dst,h->rgb,h->height * h->bytes_per_line );
	}
	else
	{
		std::cout << "Failed fim_qoi_read!\n";
	}
}

static void
fim_qoi_done(void *data)
{
	struct qoi_state *h = (struct qoi_state *) data;

	if (h->fp)
		fclose(h->fp);
	if(h->rgb)
		free(h->rgb); // libqoi
	if(h->fbd)
		fim_free(h->fbd);
	fim_free(h);
}

struct ida_loader qoi_loader = {
/*
 * 0000000: 716f 6966 .... .... .... .... ..                   qoif.. ..
 */
    /*magic:*/ "qoif",
    /*moff:*/  0,
    /*mlen:*/  4,
    /*name:*/  "libqoi",
    /*init:*/  fim_qoi_init,
    /*read:*/  fim_qoi_read,
    /*done:*/  fim_qoi_done,
};

static void fim__init init_rd(void)
{
	fim_load_register(&qoi_loader);
}
}
#endif /* FIM_WITH_QOI */
