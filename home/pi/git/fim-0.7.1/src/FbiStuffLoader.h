/* $LastChangedDate: 2022-11-20 02:50:16 +0100 (Sun, 20 Nov 2022) $ */
/*
 FbiStuffLoader.h : fbi functions for loading files, modified for fim

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


#ifndef FIM_STUFF_LOADER_H
#define FIM_STUFF_LOADER_H

#include "fim_string.h"
#include "fim_types.h"
#include "FbiStuffList.h"

//#include "list.h"
#ifdef USE_X11
# include <X11/Intrinsic.h>
#endif /* USE_X11 */

namespace fim
{

enum ida_extype {
    EXTRA_COMMENT = 1,
    EXTRA_EXIF    = 2
};

struct ida_extra {
    enum ida_extype   type;
    fim_byte_t     *data;
    unsigned int      size;
    struct ida_extra  *next;
};

/* image data and metadata */
struct ida_image_info {
    unsigned int      width;
    unsigned int      height;
    unsigned int      dpi;
    unsigned int      npages;
    struct ida_extra  *extra;
#ifdef FIM_EXPERIMENTAL_ROTATION
    unsigned int      fim_extra_flags;/* FIXME : unclean: regard this as a hack (flag set on a rotated image) */
#endif /* FIM_EXPERIMENTAL_ROTATION */

    int               thumbnail;
    unsigned int      real_width;
    unsigned int      real_height;
#if FIM_EXPERIMENTAL_IMG_NMSPC
    fim::Namespace *nsp;
#endif /* FIM_EXPERIMENTAL_IMG_NMSPC */
};

struct ida_image {
    struct ida_image_info  i;
    fim_byte_t          *data;
};
struct ida_rect {
    int x1,y1,x2,y2;
};

/* load image files */
struct ida_loader {
    //const fim_byte_t  *magic;
    const fim_char_t  *magic;
    int   moff;
    int   mlen;
    const fim_char_t  *name;
    void* (*init)(FILE *fp, const fim_char_t *filename, unsigned int page,
		  struct ida_image_info *i, int thumbnail);
    void  (*read)(fim_byte_t *dst, unsigned int line, void *data);
    void  (*done)(void *data);
    struct list_head list;
};

/* filter + operations */
struct ida_op {
    const fim_char_t  *name;
    void* (*init)(const struct ida_image *src, struct ida_rect *rect,
		  struct ida_image_info *i, void *parm);
    void  (*work)(const struct ida_image *src, struct ida_rect *rect,
		  fim_byte_t *dst, int line,
		  void *data);
    void  (*done)(void *data);
};

#if FIM_WANT_OBSOLETE
void* op_none_init(struct ida_image *src, struct ida_rect *rect,
		   struct ida_image_info *i, void *parm);
#endif /* FIM_WANT_OBSOLETE */
void  op_none_done(void *data);
#if FIM_WANT_OBSOLETE
void  op_free_done(void *data);
#endif /* FIM_WANT_OBSOLETE */

#ifdef USE_X11
/* save image files */
struct ida_writer {
    const fim_char_t  *label;
    const fim_char_t  *ext[8];
    int   (*write)(FILE *fp, struct ida_image *img);
    int   (*conf)(Widget widget, struct ida_image *img);
    struct list_head list;
};
#endif /* USE_X11 */

/* ----------------------------------------------------------------------- */
/* resolution                                                              */

#define res_cm_to_inch(x) ((x * 2540 + 5) / 1000)
#define res_m_to_inch(x)  ((x * 2540 + 5) / 100000)
#define res_inch_to_m(x)  ((x * 100000 + 5) / 2540)

/* ----------------------------------------------------------------------- */

/* helpers */
void load_bits_lsb(fim_byte_t *dst, fim_byte_t *src, int width,
		   int on, int off);
void load_bits_msb(fim_byte_t *dst, fim_byte_t *src, int width,
		   int on, int off);
void load_gray(fim_byte_t *dst, fim_byte_t *src, int width);
void load_graya(fim_byte_t *dst, fim_byte_t *src, int width);
void load_rgba(fim_byte_t *dst, fim_byte_t *src, int width);

int load_add_extra(struct ida_image_info *info, enum ida_extype type,
		   fim_byte_t *data, unsigned int size);
struct ida_extra* load_find_extra(struct ida_image_info *info,
				  enum ida_extype type);
int load_free_extras(struct ida_image_info *info);

/* ----------------------------------------------------------------------- */

/* other */
//extern int debug;
extern struct ida_loader ppm_loader;
extern struct ida_loader sane_loader;
extern struct ida_writer ps_writer;
extern struct ida_writer jpeg_writer;

/* lists */
#define fim__init __attribute__ ((constructor))
#define fim__fini __attribute__ ((destructor))

extern struct list_head loaders;
extern struct list_head writers;

void fim_load_register(struct ida_loader *loader);
void fim_write_register(struct ida_writer *writer);
void fim_loaders_to_stderr(FILE * stream);

string fim_loaders_to_string(void);
}
#endif /* FIM_STUFF_LOADER_H */


