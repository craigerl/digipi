/* $LastChangedDate: 2022-10-30 16:13:52 +0100 (Sun, 30 Oct 2022) $ */
/*
 FbiStuffXyz.cpp : An example file for reading new file types with hypothetical library libxyz.

 (c) 2014-2018 Michele Martone
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

/*
 If you want to include a new file format reader in fim, use this file as a playground.
 This is an example file decoder for our test ".xyz" file format.
 An *.xyz file may look like the quoted text that follows:
 "xyz 255 255 1 This is a sample file to be opened by the sample decoder in FbiStuffXyz.cpp."

 Set FIM_WITH_LIBXYZ to 1 to enable this sample decoder and be able to open *.xyz files.
 Then do:
 echo "xyz 255 255 1 This is a sample file to be opened by the sample decoder in FbiStuffXyz.cpp." > file.xyz
 src/fim file.xyz
 */
#if FIM_WITH_LIBXYZ

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
/* hypothetic libxyz routines */
static fim_err_t xyz_load_image_info_fp(FILE *fp, int * numpagesp, unsigned int *wp, unsigned int *hp)
{
	/* gives width and height and number of pages (assuming each page same size) */
	fseek(fp,3,SEEK_SET);
	fscanf(fp,"%d %d %d",wp,hp,numpagesp);
	std::cout << "Identified a " << *wp << " x " << *hp << " file with " << *numpagesp << " pages.\n";
       	return FIM_ERR_NO_ERROR;
}

static fim_err_t xyz_load_image_fp(FILE *fp, unsigned int page, unsigned char * rgb, int bytes_per_line)
{
	int l,c,w,h,numpages; /* page starts at 0 */
	fseek(fp,3,SEEK_SET);
	fscanf(fp,"%d %d %d",&w,&h,&numpages);
	std::cout << "Will read page "<< page << " of " << w << " x " << h << " file with " << numpages << " pages.\n";

	if(bytes_per_line < w*3)
		goto err;

	for (l=0;l<h;++l)
	{
		for (c=0;c<w && c<l;++c)
		{
			rgb[l*bytes_per_line+c*3+0] = 255;
			rgb[l*bytes_per_line+c*3+1] = c;
			rgb[l*bytes_per_line+c*3+2] = 255;
		}
		for (c=l+1;c<w;++c)
		{
			rgb[l*bytes_per_line+c*3+0] = c;
			rgb[l*bytes_per_line+c*3+1] = 255;
			rgb[l*bytes_per_line+c*3+2] = 255;
		}
	}
	std::cout << "Read the file\n";

       	return FIM_ERR_NO_ERROR;
err:
	std::cout << "A reading error occured!\n";
	return FIM_ERR_GENERIC;
}
/* ---------------------------------------------------------------------- */
/* load                                                                   */

struct xyz_state {
	FILE *fp;
	int w; /* image width: 1... */
	int h; /* image height: 1... */
	int np; /* number of pages: 1... */
    	size_t flen; /* file length */
	fim_byte_t*rgb; /* pixels, from upper left to lower right, line by line */
	int bytes_per_line; /* rgb has bytes_per_line bytes per line */
};

static void*
xyz_init(FILE *fp, const fim_char_t *filename, unsigned int page, struct ida_image_info *i, int thumbnail)
{
	/* it is safe to ignore filename, page, thumbnail */
	struct xyz_state *h = FIM_NULL;
    	fim_err_t errval = FIM_ERR_GENERIC;
	long fo = 0;

	h = (struct xyz_state *)fim_calloc(1,sizeof(*h));

	if(!h)
		goto oops;

    	h->fp = fp;

	if(fseek(h->fp,0,SEEK_END)!=0)
		goto oops;

    	fo = ftell(h->fp);
	h->flen = fo; /* FIXME: evil conversion */
    	if( fo == -1 )
		goto oops;

	errval = xyz_load_image_info_fp(h->fp, &h->np, &i->width, &i->height);
	if(errval != FIM_ERR_NO_ERROR)
	{
		std::cout << "Failed xyz_load_image_info_fp !\n";
 	   	goto oops;
	}

	i->npages = 1;
	h->w = i->width;
       	h->h = i->height;
	h->bytes_per_line = i->width * 3;
	h->rgb = (fim_byte_t*)fim_malloc(i->height * h->bytes_per_line );
	if(!h->rgb)
	{
		std::cout << "Failed fim_malloc!\n";
    		goto oops;
	}

	errval = xyz_load_image_fp(h->fp, page, h->rgb, h->bytes_per_line);
	if(errval != FIM_ERR_NO_ERROR)
	{
		std::cout << "Failed xyz_load_image_fp!\n";
		goto oops;
	}

	return h;
oops:
	if(h)
	{
		if(h->rgb)
			fim_free(h->rgb);
		fim_free(h);
	}
	return FIM_NULL;
}

static void
xyz_read(fim_byte_t *dst, unsigned int line, void *data)
{
	/* this gets called every line. can be left empty if useless. */
	struct xyz_state *h = (struct xyz_state *) data;
	int c;

	/* 
	 * In this example, we copy back from a buffer.
	 * A particular file format may require a call to e.g. xyz_read_line().
	 * */
	if(line == 0)
		std::cout << "Reading first line of the file..\n";
	for (c=0;c<h->w;++c)
	{
		dst[c*3+0] = h->rgb[h->bytes_per_line*line+c*3+0];
		dst[c*3+1] = h->rgb[h->bytes_per_line*line+c*3+1];
		dst[c*3+2] = h->rgb[h->bytes_per_line*line+c*3+2];
	}
	if(line==h->h/2)
	       	std::cout << "Read half of the file\n";
	if(line == h->h-1)
	       	std::cout << "Read the entire file\n";
}

static void
xyz_done(void *data)
{
	struct xyz_state *h = (struct xyz_state *) data;

	std::cout << "Done! Closing the file.\n";
	fclose(h->fp);
	fim_free(h->rgb);
	fim_free(h);
}

struct ida_loader xyz_loader = {
/*
 * 0000000: 7879 7a2e 2e2e 202e 0a                   xyz... ..
 */
    /*magic:*/ "xyz",
    /*moff:*/  0,
    /*mlen:*/  3,
    /*name:*/  "xyz",
    /*init:*/  xyz_init,
    /*read:*/  xyz_read,
    /*done:*/  xyz_done,
};

static void fim__init init_rd(void)
{
	fim_load_register(&xyz_loader);
}

}
#endif /* FIM_WITH_LIBXYZ */
