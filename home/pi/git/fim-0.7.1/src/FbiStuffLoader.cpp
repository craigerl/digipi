/* $LastChangedDate: 2022-10-15 18:45:07 +0200 (Sat, 15 Oct 2022) $ */
/*
 FbiStuffLoader.cpp : fbi functions for loading files, modified for fim

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

//#include "loader.h"
#include "fim.h"
#include "FbiStuffLoader.h"

/* ----------------------------------------------------------------------- */

namespace fim
{

void load_bits_lsb(fim_byte_t *dst, fim_byte_t *src, int width,
		   int on, int off)
{
    int i,mask,bit;
    
    for (i = 0; i < width; i++) {
	mask = 1 << (i & 0x07);
	bit  = src[i>>3] & mask;
	dst[0] = bit ? on : off;
	dst[1] = bit ? on : off;
	dst[2] = bit ? on : off;
	dst += 3;
    }
}

void load_bits_msb(fim_byte_t *dst, fim_byte_t *src, int width,
		   int on, int off)
{
    int i,mask,bit;
    
    for (i = 0; i < width; i++) {
	mask = 1 << (7 - (i & 0x07));
	bit  = src[i>>3] & mask;
	dst[0] = bit ? on : off;
	dst[1] = bit ? on : off;
	dst[2] = bit ? on : off;
	dst += 3;
    }
}

void load_gray(fim_byte_t *dst, fim_byte_t *src, int width)
{
    int i;

    for (i = 0; i < width; i++) {
	dst[0] = src[0];
	dst[1] = src[0];
	dst[2] = src[0];
	dst += 3;
	src += 1;
    }
}

void load_graya(fim_byte_t *dst, fim_byte_t *src, int width)
{
    int i;

    for (i = 0; i < width; i++) {
	dst[0] = src[0];
	dst[1] = src[0];
	dst[2] = src[0];
	dst += 3;
	src += 2;
    }
}

void load_rgba(fim_byte_t *dst, fim_byte_t *src, int width)
{
    int i;

    for (i = 0; i < width; i++) {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	dst += 3;
	src += 4;
    }
}

/* ----------------------------------------------------------------------- */

int load_add_extra(struct ida_image_info *info, enum ida_extype type,
		   fim_byte_t *data, unsigned int size)
{
    struct ida_extra *extra;

    extra = (struct ida_extra*)fim_malloc(sizeof(*extra));
    if (FIM_NULL == extra)
	return -1;
    if(type==EXTRA_COMMENT) ++size;// dez's
    fim_bzero(extra,sizeof(*extra));
    extra->data = (fim_byte_t*)fim_malloc(size);
    if (FIM_NULL == extra->data) {
	fim_free(extra);
	return -1;
    }
    extra->type = type;
    extra->size = size;
    memcpy(extra->data,data,size);
    if(type==EXTRA_COMMENT) extra->data[size-1]='\0';// dez's
    extra->next = info->extra;
    info->extra = extra;
    return 0;
}

struct ida_extra* load_find_extra(struct ida_image_info *info,
				  enum ida_extype type)
{
    struct ida_extra *extra;

    for (extra = info->extra; FIM_NULL != extra; extra = extra->next)
	if (type == extra->type)
	    return extra;
    return FIM_NULL;
}

int load_free_extras(struct ida_image_info *info)
{
    struct ida_extra *next;

    while (FIM_NULL != info->extra) {
	next = info->extra->next;
	fim_free(info->extra->data);
	fim_free(info->extra);
	info->extra = next;
    }
    return 0;
}

/* ----------------------------------------------------------------------- */

FIM_LIST_HEAD(loaders);
FIM_LIST_HEAD(writers);

void fim_load_register(struct ida_loader *loader)
{
    list_add_tail(&loader->list, &loaders);
}

#ifdef USE_X11
void fim_write_register(struct ida_writer *writer)
{
    list_add_tail(&writer->list, &writers);
}
#endif /* USE_X11 */

	static void fim_loaders_to_buffer(char * buf)
    	{
		/* FIXME: new, should be generalized */
    		struct list_head *item=FIM_NULL;
    		struct ida_loader *loader = FIM_NULL;

		*buf = FIM_SYM_CHAR_NUL;

    		list_for_each(item,&loaders)
		{
        		loader = list_entry(item, struct ida_loader, list);
			if(loader->name && loader->mlen>=1)
				sprintf(buf+strlen(buf)," %s",loader->name); 
    		}
    		list_for_each(item,&loaders)
		{
        		loader = list_entry(item, struct ida_loader, list);
			if(loader->name && loader->mlen<=0)
				sprintf(buf+strlen(buf)," %s",loader->name); 
    		}
	}

	void fim_loaders_to_stderr(FILE * stream)
    	{
		fim_char_t buf[FIM_MAXLINE_BUFSIZE];	// NOTE: a larger buffer would be ok (e.g.: user configurable)...

    		sprintf(buf,"%s","\nSupported file loaders: ");
		fim_loaders_to_buffer(buf+strlen(buf));

    		FIM_FPRINTF(stream,"%s\n",buf);
	}

	string fim_loaders_to_string(void)
    	{
		fim_char_t buf[FIM_MAXLINE_BUFSIZE];	// NOTE: a larger buffer would be ok (e.g.: user configurable)...
		fim_loaders_to_buffer(buf);
		return string(buf);
	}
}
