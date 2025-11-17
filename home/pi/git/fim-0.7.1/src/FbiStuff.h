/* $LastChangedDate: 2022-12-11 15:22:59 +0100 (Sun, 11 Dec 2022) $ */
/*
 FbiStuff.h : Misc fbi functionality routines, modified for fim

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

#ifndef FIM_FBI_STUFF_H
#define FIM_FBI_STUFF_H

#include "fim.h"

#include "FramebufferDevice.h"
#include "FbiStuffLoader.h"
#include "FontServer.h"

#ifdef USE_X11
# include <X11/Intrinsic.h>
#endif /* USE_X11 */
#include <cstdio>//FILE
#include <cstdlib>//free()
#include <cerrno>//free()
#include <cstring>//memset,strerror,..

#if FIM_HAS_TIMEOUT
#define FIM_EXECLP_TIMEOUT "15"
#define FIM_TIMED_EXECLP(CMD,...) FbiStuff::fim_execlp("timeout","timeout",FIM_EXECLP_TIMEOUT,CMD,__VA_ARGS__)
#else /* FIM_HAS_TIMEOUT */
#define FIM_TIMED_EXECLP(CMD,...) FbiStuff::fim_execlp(CMD,CMD,__VA_ARGS__)
#endif /* FIM_HAS_TIMEOUT */

// WARNING : THIS IS PURE REDUNDANCE !
/*
struct list_head {
	struct list_head *next, *prev;
};*/

extern struct list_head loaders;


namespace fim
{

class FbiStuff FIM_FINAL {
public:
static void free_image(struct ida_image *img);
static FILE* fim_execlp(const fim_char_t *arg, ...);/* new */
static struct ida_image* read_image(const fim_char_t *filename, FILE* fd, fim_int page=0, Namespace *nsp=FIM_NULL);
#if FIM_WANT_MIPMAPS
static fim_err_t fim_mipmaps_compute(const struct ida_image *src, fim_mipmap_t * mmp);
#endif /* FIM_WANT_MIPMAPS */
#if FIM_WANT_CROP 
static struct ida_image* crop_image(struct ida_image *src, ida_rect rect);
#endif /* FIM_WANT_CROP */
static struct ida_image* rotate_image90(struct ida_image *src, unsigned int rotation);
static struct ida_image* rotate_image(struct ida_image *src, float angle);
static struct ida_image* scale_image(const struct ida_image *src, float scale, float ascale
#if FIM_WANT_MIPMAPS
		, const fim_mipmap_t * mmp=FIM_NULL
#endif /* FIM_WANT_MIPMAPS */
	);
static int fim_filereading_debug(void);
static bool want_fbi_style_debug(void);
};

// filter.h

struct op_3x3_parm {
    int f1[3];
    int f2[3];
    int f3[3];
    int mul,div,add;
};

struct op_sharpe_parm {
    int factor;
};

struct op_resize_parm {
    int width;
    int height;
    int dpi;
};

struct op_rotate_parm {
    int angle;
};


//int             new_image;
// end filter.h

/* 
 * fim's functions
 * */
struct ida_image * fbi_image_clone(const struct ida_image *img);
#if FIM_WANT_OBSOLETE
struct ida_image * fbi_image_black(fim_coo_t w, fim_coo_t h);
#endif /* FIM_WANT_OBSOLETE */
fim_pxc_t fbi_img_pixel_count(const struct ida_image *img);
fim_pxc_t fbi_img_pixel_bytes(const struct ida_image *img);

}

#endif /* FIM_FBI_STUFF_H */


