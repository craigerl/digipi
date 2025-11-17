/* $LastChangedDate: 2018-01-09 22:49:26 +0100 (Tue, 09 Jan 2018) $ */
/*
 FbiStuffMatrixMarket.cpp : fim functions for decoding Matrix Market files

 (c) 2009-2023 Michele Martone
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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "FbiStuff.h"
#include "FbiStuffLoader.h"

#ifdef HAVE_MATRIX_MARKET_DECODER

/* This is an experimental library of mine, yet unreleased */
#include <rsb.h>

namespace fim
{

/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct mm_state_t {
	fim_char_t * filename;
	fim_byte_t * first_row_dst;
	int width  ;
	int height ;
};


/* ---------------------------------------------------------------------- */

static void*
mm_init(FILE *fp, const fim_char_t *filename, unsigned int page,
	  struct ida_image_info *i, int thumbnail)
{
	rsb_coo_idx_t rows,cols;
	rsb_nnz_idx_t nnz;
	struct mm_state_t *h;
	h = (struct mm_state_t *)fim_calloc(1,sizeof(*h));
	const int rows_max=FIM_RENDERING_MAX_ROWS,cols_max=FIM_RENDERING_MAX_COLS;

	if(!h)goto err;
    	h->first_row_dst=FIM_NULL;

	h->filename=FIM_NULL;
	i->dpi    = FIM_RENDERING_DPI; /* FIXME */
	i->npages = 1;

	if(rsb_lib_init(RSB_NULL_INIT_OPTIONS))
		goto err;

#if RSB_LIBRSB_VER < 10300
	if(rsb_file_mtx_get_dimensions(filename, &cols, &rows, &nnz, FIM_NULL))
		goto err;
#else /* RSB_LIBRSB_VER */
	if(rsb_file_mtx_get_dims(filename, &cols, &rows, &nnz, FIM_NULL))
		goto err;
#endif /* RSB_LIBRSB_VER */

#if 1
	if(cols<1 || rows<1)
		goto err;

#if FIM_EXPERIMENTAL_IMG_NMSPC
	if(i->nsp)
	{
		i->nsp->setVariable(string("columns"),cols);
		i->nsp->setVariable(string("rows"),rows);
		i->nsp->setVariable(string("nnz"),nnz);
	}
#endif /* FIM_EXPERIMENTAL_IMG_NMSPC */
	if(cols>cols_max)
		cols=cols_max;
	if(rows>rows_max)
		rows=rows_max;
#endif

	i->width  = cols;
	i->height = rows;
	h->width  = cols;
	h->height = rows;

	h->filename=dupstr(filename);

	if(!h->filename)
		goto err;

	return h;
err:
	if( h ) fim_free(h);
	return FIM_NULL;
}

static void
mm_read(fim_byte_t *dst, unsigned int line, void *data)
{
	struct mm_state_t *h = (struct mm_state_t*)data;

	if(!h)return;
    	if(h->first_row_dst == FIM_NULL)
    		h->first_row_dst = dst;
	else
		return;

#if 0
	if(rsb_get_pixmap_RGB_from_matrix(h->filename, dst, h->width, h->height))
#else
	if(rsb_file_mtx_render(dst,h->filename,h->width,h->width,h->height,RSB_MARF_RGB))
#endif
		goto err;
err:
	return;
}

static void
mm_done(void *data)
{
	struct mm_state_t *h = (struct mm_state_t*)data;
	if(!data)
		goto err;
	if(h->filename)
		fim_free(h->filename);

	if(rsb_lib_exit(RSB_NULL_EXIT_OPTIONS))
		goto err;
err:
	return;
}

/*
00000000  25 25 4D 61 74 72 69 78 4D 61 72 6B 65 74 20 6D 61 74 72 69 78 20 63 6F 6F 72 64 69 %%MatrixMarket matrix coordi
*/
static struct ida_loader mm_loader = {
    magic: "%%MatrixMarket matrix",
    moff:  0,
    mlen:  20,
    name:  "MatrixMarket",
    init:  mm_init,
    read:  mm_read,
    done:  mm_done,
};

static void fim__init init_rd(void)
{
    fim_load_register(&mm_loader);
}

}

#endif // ifdef HAVE_MATRIX_MARKET_DECODER

