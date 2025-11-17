/* $LastChangedDate: 2023-03-25 18:31:40 +0100 (Sat, 25 Mar 2023) $ */
/*
 FramebufferDevice.cpp : Linux Framebuffer functions from fbi, adapted for fim

 (c) 2007-2023 Michele Martone
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

#include "FbiStuffFbtools.h"
#include "FramebufferDevice.h"

#ifdef FIM_WITH_NO_FRAMEBUFFER
static void foo(){} /* let's make our compiler happy */
#else /* FIM_WITH_NO_FRAMEBUFFER */

#if HAVE_LINUX_KD_H 
#	include <linux/kd.h>	// KDGETMODE, KDSETMODE, KD_GRAPHICS, ...
#endif /* HAVE_LINUX_KD_H  */
#if HAVE_LINUX_VT_H 
#	include <linux/vt.h>	// VT_GETSTATE, .. 
#endif /* HAVE_LINUX_VT_H  */
#ifdef HAVE_SYS_USER_H
#	include <sys/user.h>	//  for PAGE_MASK (sometimes it is needed to include it here explicitly)
#else /* HAVE_SYS_USER_H */
#	error missing <sys/user.h> !
#endif /* HAVE_SYS_USER_H */
#ifdef HAVE_SYS_MMAN_H
#	include <sys/mman.h>	// PROT_READ, PROT_WRITE, MAP_SHARED
#else /* HAVE_SYS_MMAN_H */
#	error missing <sys/mman.h> !
#endif /* HAVE_SYS_MMAN_H */
#include <signal.h>
#ifdef HAVE_SYS_IOCTL_H
#	include <sys/ioctl.h>
#else /* HAVE_SYS_IOCTL_H */
#	error missing <sys/ioctl.h> !
#endif /* HAVE_SYS_IOCTL_H */

//#include <errno.h>
//#include <sys/ioctl.h>
//#include <sys/mman.h>
//#include <sys/wait.h>
//#include <sys/stat.h>

/*#include <asm/page.h>*/ /* seems like this gives problems */
#if 0
#include <signal.h>	  /* added in fim. missing when compiling with -ansi */
#include <asm/signal.h>	  /* added in fim. missing when compiling with -ansi */
#endif


namespace fim
{
    extern fim_int fim_fmf_; /* FIXME */

#define	FIM_DEBUGGING_FOR_ARM_WITH_VITALY 0
#define FIM_FBI_FB_MODES_LINE_BUFSIZE	80
#define FIM_FBI_FB_MODES_LABEL_BUFSIZE	32
#define FIM_FBI_FB_MODES_VALUE_BUFSIZE	16
/*
   this code will be enabled by default if we can make sure it
   won't break often with kernel updates */
#if	FIM_DEBUGGING_FOR_ARM_WITH_VITALY

static void print_vinfo(struct fb_var_screeninfo *vinfo)
{
	FIM_FPRINTF(stderr,  "Printing vinfo:\n");
	FIM_FPRINTF(stderr,  "\txres: %d\n", vinfo->xres);
	FIM_FPRINTF(stderr,  "\tyres: %d\n", vinfo->yres);
	FIM_FPRINTF(stderr,  "\txres_virtual: %d\n", vinfo->xres_virtual);
	FIM_FPRINTF(stderr,  "\tyres_virtual: %d\n", vinfo->yres_virtual);
	FIM_FPRINTF(stderr,  "\txoffset: %d\n", vinfo->xoffset);
	FIM_FPRINTF(stderr,  "\tyoffset: %d\n", vinfo->yoffset);
	FIM_FPRINTF(stderr,  "\tbits_per_pixel: %d\n", vinfo->bits_per_pixel);
	FIM_FPRINTF(stderr,  "\tgrayscale: %d\n", vinfo->grayscale);
	FIM_FPRINTF(stderr,  "\tnonstd: %d\n", vinfo->nonstd);
	FIM_FPRINTF(stderr,  "\tactivate: %d\n", vinfo->activate);
	FIM_FPRINTF(stderr,  "\theight: %d\n", vinfo->height);
	FIM_FPRINTF(stderr,  "\twidth: %d\n", vinfo->width);
	FIM_FPRINTF(stderr,  "\taccel_flags: %d\n", vinfo->accel_flags);
	FIM_FPRINTF(stderr,  "\tpixclock: %d\n", vinfo->pixclock);
	FIM_FPRINTF(stderr,  "\tleft_margin: %d\n", vinfo->left_margin);
	FIM_FPRINTF(stderr,  "\tright_margin: %d\n", vinfo->right_margin);
	FIM_FPRINTF(stderr,  "\tupper_margin: %d\n", vinfo->upper_margin);
	FIM_FPRINTF(stderr,  "\tlower_margin: %d\n", vinfo->lower_margin);
	FIM_FPRINTF(stderr,  "\thsync_len: %d\n", vinfo->hsync_len);
	FIM_FPRINTF(stderr,  "\tvsync_len: %d\n", vinfo->vsync_len);
	FIM_FPRINTF(stderr,  "\tsync: %d\n", vinfo->sync);
	FIM_FPRINTF(stderr,  "\tvmode: %d\n", vinfo->vmode);
	FIM_FPRINTF(stderr,  "\tred: %d/%d\n", vinfo->red.length, vinfo->red.offset);
	FIM_FPRINTF(stderr,  "\tgreen: %d/%d\n", vinfo->green.length, vinfo->green.offset);
	FIM_FPRINTF(stderr,  "\tblue: %d/%d\n", vinfo->blue.length, vinfo->blue.offset);
	FIM_FPRINTF(stderr,  "\talpha: %d/%d\n", vinfo->transp.length, vinfo->transp.offset);
}

static void print_finfo(struct fb_fix_screeninfo *finfo)
{
	FIM_FPRINTF(stderr,  "Printing finfo:\n");
	FIM_FPRINTF(stderr,  "\tsmem_start = %p\n", (void *)finfo->smem_start);
	FIM_FPRINTF(stderr,  "\tsmem_len = %d\n", finfo->smem_len);
	FIM_FPRINTF(stderr,  "\ttype = %d\n", finfo->type);
	FIM_FPRINTF(stderr,  "\ttype_aux = %d\n", finfo->type_aux);
	FIM_FPRINTF(stderr,  "\tvisual = %d\n", finfo->visual);
	FIM_FPRINTF(stderr,  "\txpanstep = %d\n", finfo->xpanstep);
	FIM_FPRINTF(stderr,  "\typanstep = %d\n", finfo->ypanstep);
	FIM_FPRINTF(stderr,  "\tywrapstep = %d\n", finfo->ywrapstep);
	FIM_FPRINTF(stderr,  "\tline_length = %d\n", finfo->line_length);
	FIM_FPRINTF(stderr,  "\tmmio_start = %p\n", (void *)finfo->mmio_start);
	FIM_FPRINTF(stderr,  "\tmmio_len = %d\n", finfo->mmio_len);
	FIM_FPRINTF(stderr,  "\taccel = %d\n", finfo->accel);
}
#endif /* FIM_DEBUGGING_FOR_ARM_WITH_VITALY */

#define DITHER_LEVEL 8

typedef unsigned long vector[DITHER_LEVEL];
typedef vector  matrix[DITHER_LEVEL];

//#if DITHER_LEVEL == 8 ||  DITHER_LEVEL == 4
//static int matrix   DM ;
//#endif
#if DITHER_LEVEL == 8
#define DITHER_MASK 7
static matrix   DM =
{
    {0, 32, 8, 40, 2, 34, 10, 42},
    {48, 16, 56, 24, 50, 18, 58, 26},
    {12, 44, 4, 36, 14, 46, 6, 38},
    {60, 28, 52, 20, 62, 30, 54, 22},
    {3, 35, 11, 43, 1, 33, 9, 41},
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47, 7, 39, 13, 45, 5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
};

#endif /* DITHER_LEVEL  */

#if DITHER_LEVEL == 4
#define DITHER_MASK 3
static matrix   DM =
{
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5}
};

#endif /* DITHER_LEVEL  */




static FramebufferDevice *ffdp;

static void _fb_switch_signal(int signal)
{
	// WARNING : THIS IS A DIRTY HACK 
	// necessary to enable framebuffer console switching
	ffdp->fb_switch_signal(signal);
}

fim_err_t FramebufferDevice::fs_puts(struct fs_font *f_, fim_coo_t x, fim_coo_t y, const fim_char_t *str)
{
    fim_byte_t *pos,*start;
#if 0
    int j,w;
#endif
    int i,c;
#if FIM_FONT_MAGNIFY_FACTOR <= 0
    const fim_int fim_fmf = fim::fim_fmf_; 
#endif	/* FIM_FONT_MAGNIFY_FACTOR */

    pos  = fb_mem_+fb_mem_offset_;
    pos += fb_fix_.line_length * y;
    for (i = 0; str[i] != '\0'; i++) {
	c = (fim_byte_t)str[i];
	if (FIM_NULL == f_->eindex[c])
	    continue;
	/* clear with bg color */
#if 0
	start = pos + x*fs_bpp_ + f_->fontHeader.max_bounds.descent * fb_fix_.line_length;
	w = (f_->eindex[c]->width*fim_fmf+1)*fs_bpp_;
#ifdef FIM_IS_SLOWER_THAN_FBI
	for (j = 0; j < f_->sheight(); j++) {
/////	    memset_combine(start,0x20,w);
	    fim_bzero(start,w);
	    start += fb_fix_.line_length;
	}
#else /* FIM_IS_SLOWER_THAN_FBI */
	//sometimes we can gather multiple calls..
	if(fb_fix_.line_length==(unsigned int)w)
	{
		//contiguous case
		fim_bzero(start,w*f_->sheight());
	    	start += fb_fix_.line_length*f_->sheight();
	}
	else
	for (j = 0; j < f_->sheight(); j++) {
	    fim_bzero(start,w);
	    start += fb_fix_.line_length;
	}
#endif /* FIM_IS_SLOWER_THAN_FBI */
	/* draw character */
#endif
	start = pos + x*fs_bpp_ + fb_fix_.line_length * (f_->sheight()-fim_fmf*f_->eindex[c]->ascent);
	fs_render_fb(start,fb_fix_.line_length,f_->eindex[c],f_->gindex[c]);
	x += f_->eindex[c]->width*fim_fmf;
	if (0U + x > fb_var_.xres - f_->swidth())
	    return FIM_ERR_GENERIC;
    }
    return FIM_ERR_NO_ERROR;
}

void FramebufferDevice::fs_render_fb(fim_byte_t *ptr, int pitch, FSXCharInfo *charInfo, fim_byte_t *data)
{

/* 
 * These preprocessor macros should serve *only* for font handling purposes.
 * */
#define BIT_ORDER       BitmapFormatBitOrderMSB
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif /* BYTE_ORDER */
#define BYTE_ORDER      BitmapFormatByteOrderMSB
#define SCANLINE_UNIT   BitmapFormatScanlineUnit8
#define SCANLINE_PAD    BitmapFormatScanlinePad8
#define EXTENTS         BitmapFormatImageRectMin

#define SCANLINE_PAD_BYTES 1
#define GLWIDTHBYTESPADDED(bits, nBytes)                                    \
        ((nBytes) == 1 ? (((bits)  +  7) >> 3)          /* pad to 1 byte  */\
        :(nBytes) == 2 ? ((((bits) + 15) >> 3) & ~1)    /* pad to 2 bytes */\
        :(nBytes) == 4 ? ((((bits) + 31) >> 3) & ~3)    /* pad to 4 bytes */\
        :(nBytes) == 8 ? ((((bits) + 63) >> 3) & ~7)    /* pad to 8 bytes */\
        : 0)

    int row,bit,bpr,x;
#if FIM_FONT_MAGNIFY_FACTOR <= 0
    const fim_int fim_fmf = fim::fim_fmf_; 
#endif	/* FIM_FONT_MAGNIFY_FACTOR */

    bpr = GLWIDTHBYTESPADDED((charInfo->right - charInfo->left),
			     SCANLINE_PAD_BYTES);
    for (row = 0; row < (charInfo->ascent + charInfo->descent); row++) {
	for (x = 0, bit = 0; bit < (charInfo->right - charInfo->left); bit++) {
	    if (data[bit>>3] & fs_masktab[bit&7])
		// WARNING !
#if FIM_FONT_MAGNIFY_FACTOR == 1
		fs_setpixel(ptr+x,fs_white_);
#else	/* FIM_FONT_MAGNIFY_FACTOR */
		for(fim_coo_t mi = 0; mi < fim_fmf; ++mi)
		for(fim_coo_t mj = 0; mj < fim_fmf; ++mj)
			fs_setpixel(ptr+((fim_fmf*x+mj*FB_BPP)+(mi)*pitch),fs_white_);
#endif	/* FIM_FONT_MAGNIFY_FACTOR */
	    x += fs_bpp_;
	}
	data += bpr;
	ptr += pitch*fim_fmf;
    }
#undef BIT_ORDER
#undef BYTE_ORDER
#undef SCANLINE_UNIT
#undef SCANLINE_PAD
#undef EXTENTS
#undef SCANLINE_PAD_BYTES
#undef GLWIDTHBYTESPADDED
}


	fim_err_t FramebufferDevice::framebuffer_init(const bool try_boz_patch)
	{
		int rc=0;

		//initialization of the framebuffer text
		FontServer::fb_text_init1(fontname_,&f_);	// FIXME : move this outta here
		/*
		 * will initialize with the user set (or default ones)
		 *  - framebuffer device
		 *  - framebuffer mode
		 *  - virtual terminal
		 * */
		fd_ = fb_init(fbdev_, fbmode_, vt_);
		if(fd_==-1 && !try_boz_patch)
    			return FIM_ERR_GENERIC;
		if(fd_==-1)
			fd_ = fb_init(fbdev_, fbmode_, vt_, try_boz_patch);//maybe we are under screen..
		if(fd_==-1)
			exit(1);
			//return -1;//this is a TEMPORARY and DEAF,DUMB, AND BLIND bug noted by iam
		//setting signals to handle in the right ways signals
		fb_catch_exit_signals();
		fb_switch_init();
		/*
		 * C-z is inhibited now (for framebuffer's screen safety!)
		 */
		signal(SIGTSTP,SIG_IGN);
		//signal(SIGSEGV,cleanup();
		//set text color to white ?
		
		//initialization of the framebuffer device handlers
		if((rc=fb_text_init2()))return rc;
	
			switch (fb_var_.bits_per_pixel) {
		case 8:
			svga_dither_palette(8, 8, 4);
			dither_ = FIM_FBI_TRUE;
			init_dither(8, 8, 4, 2);
			break;
		case 15:
	    	case 16:
	        	if (fb_fix_.visual == FB_VISUAL_DIRECTCOLOR)
	        	    linear_palette(5);
			if (fb_var_.green.length == 5) {
			    lut_init(15);
			} else {
			    lut_init(16);
			}
			break;
		case 24:
	        	if (fb_fix_.visual == FB_VISUAL_DIRECTCOLOR)
	      	      linear_palette(8);
			break;
		case 32:
	        	if (fb_fix_.visual == FB_VISUAL_DIRECTCOLOR)
	          	  linear_palette(8);
			lut_init(24);
			break;
		default:
			FIM_FPRINTF(stderr,  "Oops: %i bit/pixel ???\n",
				fb_var_.bits_per_pixel);
			std::exit(1);
	    	}
	    	if (fb_fix_.visual == FB_VISUAL_DIRECTCOLOR ||
			fb_var_.bits_per_pixel == 8)
		{
			if (-1 == ioctl(fd_,FBIOPUTCMAP,&cmap_)) {
		    		fim_perror("ioctl FBIOPUTCMAP");
			    std::exit(1);
			}
		}
		return FIM_ERR_NO_ERROR;
	}

void FramebufferDevice::dev_init(void)
{
    fim_stat_t dummy;

    if (FIM_NULL != devices_)
	return;
#if HAVE_SYS_STAT_H
    if (0 == stat("/dev/.devfsd",&dummy))
	devices_ = &devs_devfs_;
    else
#endif /* HAVE_SYS_STAT_H */
	devices_ = &devs_default_;

}


void FramebufferDevice::console_switch(fim_bool_t is_busy)
{
	//FIXME
	switch (fb_switch_state_) {
	case FB_REL_REQ:
		fb_switch_release();
	case FB_INACTIVE:
		visible_ = 0;
	break;
	case FB_ACQ_REQ:
		fb_switch_acquire();
	case FB_ACTIVE:
		//when stepping in console..
		visible_ = 1;
		ioctl(fd_,FBIOPAN_DISPLAY,&fb_var_);
		redraw_ = FIM_REDRAW_NECESSARY;
	/*
	 * we use to redraw on console switch here; not anymore; 
	 */
	//mc_.cc_.redisplay(); // fbi used to redraw now
	//if (is_busy) status("busy, please wait ...", FIM_NULL);		
	break;
	default:
	break;
    	}
	switch_last_ = fb_switch_state_;
	return;
}

//void FramebufferDevice::svga_display_image_new(struct ida_image *img, int xoff, int yoff,unsigned int bx,unsigned int bw,unsigned int by,unsigned int bh,int mirror,int flip)
void FramebufferDevice::svga_display_image_new(
	const struct ida_image *img,
	int yoff,
	int xoff,
		int irows,int icols,// rows and columns in the input image
		int icskip,	// input columns to skip for each line
	unsigned int by,
	unsigned int bx,
	unsigned int bh,
	unsigned int bw,
		int ocskip,// output columns to skip for each line
	int flags) FIM_NOEXCEPT
{
/*	bx is the box's x origin
 *	by is the box's y origin
 *	bw is the box's width
 *	bh is the box's heigth
 * */

	/*
	 * WARNING : SHOULD ASSeRT BX+BW < FB_VAR.XReS ..!!
	 * */
    unsigned int     dwidth  = FIM_MIN(img->i.width,  bw);
    unsigned int     dheight = FIM_MIN(img->i.height, bh);
    unsigned int     data, video, /*bank,*/ offset, /*bytes,*/ y;
    int yo=(bh-dheight)/2;
    int xo=(bw-dwidth )/2;
    int cxo=bw-dwidth-xo;
    //int cyo=bh-yo;
    int mirror=flags&FIM_FLAG_MIRROR, flip=flags&FIM_FLAG_FLIP;

    if (!visible_)/*COMMENT THIS IF svga_display_image IS NOT IN A CYCLE*/
	return;
    /*fb_clear_screen();//EXPERIMENTAL
    if(xoff&&yoff)clear_rect(0,xoff,0,yoff);*/

    //bytes = FB_BPP;

    /* offset for image data (image > screen, select visible area) */
    offset = (yoff * img->i.width + xoff) * 3;
    
    /* offset for video memory (image < screen, center image) */
    video = 0;//, bank = 0;
    video += FB_BPP * (bx);
    if (img->i.width < bw)
    {	    
	    video += FB_BPP * (xo);
    }
    
    video += fb_fix_.line_length * (by);
    if (img->i.height < bh )
    {	   
	    video += fb_fix_.line_length * (yo);
    }

    if (dheight < bh ) 
    {	    
    	/* clear by lines */
#ifdef FIM_FASTER_CLEARLINES
	if(bw==fb_var_.xres && bx==0)
	{
		/*
		 * +------------------------------+
		 * | whole screen line clear join |
		 * +------------------------------+
		 */
		// wide screen clear
		{ clear_line(FB_BPP, by, bw*(bh), FB_MEM(bx,by)); }
		
		//top and bottom lines clear : maybe better
		//{ clear_line(FB_BPP, by, bw*(yo), FB_MEM(bx,by)); }
		//{ clear_line(FB_BPP, by+yo, bw*(dheight), FB_MEM(bx,by+yo)); }
		//{ clear_line(FB_BPP, by+dheight+yo, bw*(bh-yo-dheight), FB_MEM(bx,by+yo+dheight)); }
	}
	else
#endif /* FIM_FASTER_CLEARLINES */
	{
	    	for ( y = by; y < by+yo;++y) { clear_line(FB_BPP, y, bw, FB_MEM(bx,y)); }
		for ( y = by+dheight+yo; y < by+bh;++y) { clear_line(FB_BPP, y, bw, FB_MEM(bx,y)); }
	}
    }

    if (dwidth < bw )
    {	    
#ifdef FIM_FASTER_CLEARLINES
    	    if(bw==fb_var_.xres && bx==0)
	    {
	    	if (dheight >= bh ) 
			clear_line(FB_BPP, by, bw*(bh), FB_MEM(bx,by));
	    }
	    else
#endif /* FIM_FASTER_CLEARLINES */
    	    for ( y = by; y < by+bh;++y)
	    {
		    clear_line(FB_BPP, y, xo, FB_MEM(bx,y));
		    clear_line(FB_BPP, y, cxo,FB_MEM(bx+xo+dwidth,y));
	    }
    }
    /*for ( y = 0; y < fb_var_.yres;y+=100)fb_line(0, fb_var_.xres, y, y);*/

    /* go ! */
    /*flip patch*/
#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))
#endif /* min */
    //int fb_fix_line_length=FB_MEM_LINE_OFFSET;
    int fb_fix_line_length=fb_fix_.line_length;
    if(flip) {	fb_fix_line_length*=-1; video += (min(img->i.height,dheight)-1)*(fb_fix_.line_length);}
    /*flip patch*/
    /* FIXME : COMPLETE ME ... */

#ifndef FIM_IS_SLOWER_THAN_FBI
    fim_byte_t *(FramebufferDevice::*convert_line_f)(int , int , int , fim_byte_t *, fim_byte_t *, int );
    if(fb_var_.bits_per_pixel==8)
 	   convert_line_f=&fim::FramebufferDevice::convert_line_8;
    else
 	   convert_line_f=&fim::FramebufferDevice::convert_line;
#endif /* FIM_IS_SLOWER_THAN_FBI */

    fim_pxc_t pc = fbi_img_pixel_bytes(img);
    for (data = 0, y = by;
	 data < pc 
	     && data / img->i.width / 3 < dheight;
	 data += img->i.width * 3, video += fb_fix_line_length)
    {
#ifndef FIM_IS_SLOWER_THAN_FBI
	(this->*convert_line_f)
#else
	convert_line
#endif /* FIM_IS_SLOWER_THAN_FBI */
	(fb_var_.bits_per_pixel, y++, dwidth,
		     fb_mem_+video, img->data + data + offset,mirror);/*<- mirror patch*/
    }
}

int FramebufferDevice::fb_init(const fim_char_t *device, fim_char_t *mode, int vt, int try_boz_patch)
{
    /*
     * This member function will probe for a valid framebuffer device.
     *
     * The try_boz_patch will make framebuffer fim go straight ahead ignoring lots of errors.
     * Like the ones when running fim under screen.
     * Like the ones when running fim under X.
     * */
    fim_char_t fbdev[FIM_FBDEV_FILE_MAX_CHARS];
    struct vt_stat vts;

    dev_init();
    tty_ = 0;
    if (vt != 0)
	fb_setvt(vt);

#ifdef FIM_BOZ_PATCH
    if(!try_boz_patch)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,VT_GETSTATE, &vts)) {
	FIM_FPRINTF(stderr, "ioctl VT_GETSTATE: %s (not a linux console?)\n",
		strerror(errno));
	return -1;
    }
    
    /* no device supplied ? we probe for one */
    if (FIM_NULL == device) {
	device = fim_getenv(FIM_ENV_FRAMEBUFFER);
	/* no environment - supplied device ? */
	if (FIM_NULL == device) {
	    struct fb_con2fbmap c2m;
	    if (-1 == (fb_ = open(devices_->fb0,O_RDWR /* O_WRONLY */,0))) {
		FIM_FPRINTF(stderr, "open %s: %s\n",devices_->fb0,strerror(errno));
	        fim_perror(FIM_NULL);
		exit(1);
	    }
	    c2m.console = vts.v_active;
#ifdef FIM_BOZ_PATCH
    if(!try_boz_patch){
#endif /* FIM_BOZ_PATCH */
	    if (-1 == ioctl(fb_, FBIOGET_CON2FBMAP, &c2m)) {
		fim_perror("ioctl FBIOGET_CON2FBMAP");
		exit(1);
	    }
	    close(fb_);
/*	    FIM_FPRINTF(stderr, "map: vt%02d => fb%d\n",
		    c2m.console,c2m.framebuffer);*/
	    sprintf(fbdev,devices_->fbnr,c2m.framebuffer);
	    device = fbdev;
#ifdef FIM_BOZ_PATCH
    	    }
    else
	    device = "/dev/fb0";
#endif /* FIM_BOZ_PATCH */
	}
    }

    /* get current settings (which we have to restore) */
    if (-1 == (fb_ = open(device,O_RDWR /* O_WRONLY */))) {
	FIM_FPRINTF(stderr, "open %s: %s\n",device,strerror(errno));
	fim_perror(FIM_NULL);
	exit(1);
    }
    if (-1 == ioctl(fb_,FBIOGET_VSCREENINFO,&fb_ovar_)) {
	fim_perror("ioctl FBIOGET_VSCREENINFO");
	exit(1);
    }
#if	FIM_DEBUGGING_FOR_ARM_WITH_VITALY
	print_vinfo(&fb_ovar_);
#endif /* FIM_DEBUGGING_FOR_ARM_WITH_VITALY */
    if (-1 == ioctl(fb_,FBIOGET_FSCREENINFO,&fb_fix_)) {
	fim_perror("ioctl FBIOGET_FSCREENINFO");
	exit(1);
    }
#if	FIM_DEBUGGING_FOR_ARM_WITH_VITALY
	print_finfo(&fb_fix_);
#endif /* FIM_DEBUGGING_FOR_ARM_WITH_VITALY */
    if (fb_ovar_.bits_per_pixel == 8 ||
	fb_fix_.visual == FB_VISUAL_DIRECTCOLOR) {
	if (-1 == ioctl(fb_,FBIOGETCMAP,&ocmap_)) {
	    fim_perror("ioctl FBIOGETCMAP");
	    exit(1);
	}
    }
#ifdef FIM_BOZ_PATCH
    if(!try_boz_patch)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,KDGETMODE, &kd_mode_)) {
	fim_perror("ioctl KDGETMODE");
	exit(1);
    }
#ifdef FIM_BOZ_PATCH
    if(!try_boz_patch)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,VT_GETMODE, &vt_omode_)) {
	fim_perror("ioctl VT_GETMODE");
	exit(1);
    }
    tcgetattr(tty_, &term_);
    
    /* switch mode */
    if(-1 == fb_setmode(mode)){
#if 0
	/* 
	 * FIXME:
	 * mm's strict mode checking (right now, this function triggers an exit() but things should change) */
#ifdef FIM_BOZ_PATCH
    	if(!try_boz_patch)
#endif /* FIM_BOZ_PATCH */
	{
		fim_perror("failed setting mode");
		exit(1);
	}
#endif
    }

    
    /* checks & initialisation */
    if (-1 == ioctl(fb_,FBIOGET_FSCREENINFO,&fb_fix_)) {
	fim_perror("ioctl FBIOGET_FSCREENINFO");
	exit(1);
    }
    if (fb_fix_.type != FB_TYPE_PACKED_PIXELS) {
	FIM_FPRINTF(stderr, "can handle only packed pixel frame buffers\n");
	goto err;
    }
#if 0
    switch (fb_var_.bits_per_pixel) {
    case 8:
	white = 255; black = 0; bpp = 1;
	break;
    case 15:
    case 16:
	if (fb_var_.green.length == 6)
	    white = 0xffff;
	else
	    white = 0x7fff;
	black = 0; bpp = 2;
	break;
    case 24:
	white = 0xffffff; black = 0; bpp = fb_var_.bits_per_pixel/8;
	break;
    case 32:
	white = 0xffffff; black = 0; bpp = fb_var_.bits_per_pixel/8;
	fb_setpixels = fb_setpixels4;
	break;
    default:
	FIM_FPRINTF(stderr,  "Oops: %i bit/pixel ???\n",
		fb_var_.bits_per_pixel);
	goto err;
    }
#endif
#ifdef PAGE_MASK
    fb_mem_offset_ = (fb_fix_.smem_start) & (~PAGE_MASK);
#else /* PAGE_MASK */
    /* some systems don't have this symbol outside their kernel headers - will do any harm ? */
    /* FIXME : what are the wider implications of this ? */
    fb_mem_offset_ = 0;
#endif /* PAGE_MASK */
    fb_mem_ = (fim_byte_t*) mmap(FIM_NULL,fb_fix_.smem_len+fb_mem_offset_,
		  PROT_READ|PROT_WRITE,MAP_SHARED,fb_,0);
#ifdef MAP_FAILED
    if (fb_mem_ == MAP_FAILED) 
#else
    if (-1L == static_cast<long>fb_mem_)
#endif
    {
	fim_perror("mmap failed");
	goto err;
    }
    /* move viewport to upper left corner */
    if (fb_var_.xoffset != 0 || fb_var_.yoffset != 0) {
	fb_var_.xoffset = 0;
	fb_var_.yoffset = 0;
	if (-1 == ioctl(fb_,FBIOPAN_DISPLAY,&fb_var_)) {
	    fim_perror("ioctl FBIOPAN_DISPLAY");
	    goto err;
	}
    }
#ifdef FIM_BOZ_PATCH
    if(!try_boz_patch)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,KDSETMODE, KD_GRAPHICS)) {
	fim_perror("ioctl KDSETMODE");
	goto err;
    }
    fb_activate_current(tty_);

    /* cls */
    fb_memset(fb_mem_+fb_mem_offset_,0,fb_fix_.smem_len);

#ifdef FIM_BOZ_PATCH
    with_boz_patch_=try_boz_patch;
#endif /* FIM_BOZ_PATCH */
    return fb_;

 err:
    cleanup();
    exit(1);
    return -1;
}

void FramebufferDevice::fb_memset (void *addr, int c, size_t len)
{
#if 1 /* defined(__powerpc__) */
    unsigned int i;
    
    i = (c & 0xff) << 8;
    i |= i << 16;
#ifdef FIM_IS_SLOWER_THAN_FBI
    len >>= 2;
    unsigned int *p;
    for (p = (unsigned int*) addr; len--; p++)
	*p = i;
#else /* FIM_IS_SLOWER_THAN_FBI */
    fim_memset(addr, c, len );
#endif /* FIM_IS_SLOWER_THAN_FBI */
#else
    fim_memset(addr, c, len);
#endif
}

void FramebufferDevice::fb_setvt(int vtno)
{
    struct vt_stat vts;
    fim_char_t vtname[12];
    
    if (vtno < 0) {
	if (-1 == ioctl(tty_,VT_OPENQRY, &vtno) || vtno == -1) {
	    fim_perror("ioctl VT_OPENQRY");
	    exit(1);
	}
    }

    vtno &= 0xff;
    sprintf(vtname, devices_->ttynr, vtno);
    if ( chown(vtname, getuid(), getgid())){
	FIM_FPRINTF(stderr, "chown %s: %s\n",vtname,strerror(errno));
	exit(1);
    }
    if (-1 == access(vtname, R_OK | W_OK)) {
	FIM_FPRINTF(stderr, "access %s: %s\n",vtname,strerror(errno));
	exit(1);
    }
    switch (fork()) {
    case 0:
	break;
    case -1:
	fim_perror("fork");
	exit(1);
    default:
	exit(0);
    }
    close(tty_);
    close(0);
    close(1);
    close(2);
    setsid();

    	if( -1 == open(vtname,O_RDWR) )
	{
		fim_perror(FIM_NULL);
	}
	{
		/* on some systems, we get 'int dup(int)', declared with attribute warn_unused_result */
    		int ndd;
    		ndd = dup(0);
		ndd+= dup(0);
	}

#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,VT_GETSTATE, &vts)) {
	fim_perror("ioctl VT_GETSTATE");
	exit(1);
    }
    orig_vt_no_ = vts.v_active;
    if (-1 == ioctl(tty_,VT_ACTIVATE, vtno)) {
	fim_perror("ioctl VT_ACTIVATE");
	exit(1);
    }
    if (-1 == ioctl(tty_,VT_WAITACTIVE, vtno)) {
	fim_perror("ioctl VT_WAITACTIVE");
	exit(1);
    }
}

int FramebufferDevice::fb_setmode(fim_char_t const * const name)
{
    FILE *fp;
    fim_char_t line[FIM_FBI_FB_MODES_LINE_BUFSIZE],
	 label[FIM_FBI_FB_MODES_LABEL_BUFSIZE],
	 value[FIM_FBI_FB_MODES_VALUE_BUFSIZE];
    int  geometry=0, timings=0;
 
    /* load current values */
    if (-1 == ioctl(fb_,FBIOGET_VSCREENINFO,&fb_var_)) {
	fim_perror("ioctl FBIOGET_VSCREENINFO");
	exit(1);
    }
    
    /* example name="640x480-72"; */
    if (FIM_NULL == name)
    {
	goto nerr;
    }
    if (FIM_NULL == (fp = fopen("/etc/fb.modes","r")))
    {
	goto nerr;
    }
    while (FIM_NULL != fgets(line,FIM_FBI_FB_MODES_LINE_BUFSIZE-1,fp)) {
	if (1 == sscanf(line, "mode \"%31[^\"]\"",label) &&
	    0 == strcmp(label,name)) {
	    /* fill in new values */
	    fb_var_.sync  = 0;
	    fb_var_.vmode = 0;
	    while (FIM_NULL != fgets(line,FIM_FBI_FB_MODES_LINE_BUFSIZE-1,fp) &&
		   FIM_NULL == strstr(line,"endmode")) {
//		if (5 == sscanf(line," geometry %d %d %d %d %d",
		if (5 == sscanf(line," geometry %u %u %u %u %u",
				&fb_var_.xres,&fb_var_.yres,
				&fb_var_.xres_virtual,&fb_var_.yres_virtual,
				&fb_var_.bits_per_pixel))
		    geometry = 1;
//		if (7 == sscanf(line," timings %d %d %d %d %d %d %d",
		if (7 == sscanf(line," timings %u %u %u %u %u %u %u",
				&fb_var_.pixclock,
				&fb_var_.left_margin,  &fb_var_.right_margin,
				&fb_var_.upper_margin, &fb_var_.lower_margin,
				&fb_var_.hsync_len,    &fb_var_.vsync_len))
		    timings = 1;
		if (1 == sscanf(line, " hsync %15s",value) &&
		    0 == strcasecmp(value,"high"))
		    fb_var_.sync |= FB_SYNC_HOR_HIGH_ACT;
		if (1 == sscanf(line, " vsync %15s",value) &&
		    0 == strcasecmp(value,"high"))
		    fb_var_.sync |= FB_SYNC_VERT_HIGH_ACT;
		if (1 == sscanf(line, " csync %15s",value) &&
		    0 == strcasecmp(value,"high"))
		    fb_var_.sync |= FB_SYNC_COMP_HIGH_ACT;
		if (1 == sscanf(line, " extsync %15s",value) &&
		    0 == strcasecmp(value,"true"))
		    fb_var_.sync |= FB_SYNC_EXT;
		if (1 == sscanf(line, " laced %15s",value) &&
		    0 == strcasecmp(value,"true"))
		    fb_var_.vmode |= FB_VMODE_INTERLACED;
		if (1 == sscanf(line, " double %15s",value) &&
		    0 == strcasecmp(value,"true"))
		    fb_var_.vmode |= FB_VMODE_DOUBLE;
	    }
	    /* ok ? */
	    if (!geometry || !timings)
	    	goto err;
	    /* set */
	    fb_var_.xoffset = 0;
	    fb_var_.yoffset = 0;

	    if (-1 == ioctl(fb_,FBIOPUT_VSCREENINFO,&fb_var_))
		fim_perror("ioctl FBIOPUT_VSCREENINFO");

	    /*
	     * FIXME
	     * mm : this should be placed here and uncommented : */
	    /*
	    if (-1 == ioctl(fb_,FBIOGET_FSCREENINFO,&fb_fix_)) {
		fim_perror("ioctl FBIOGET_VSCREENINFO");
		exit(1);
	    }*/
	    /* look what we have now ... */
	    if (-1 == ioctl(fb_,FBIOGET_VSCREENINFO,&fb_var_)) {
		fim_perror("ioctl FBIOGET_VSCREENINFO");
		exit(1);
	    }
	    goto ret;
	}
    }
err:
    return -1;
nerr: // not really an error
ret:
    return 0;
}

int FramebufferDevice::fb_activate_current(int tty)
{
/* Hmm. radeonfb needs this. matroxfb doesn't. (<- fbi comment) */
    struct vt_stat vts;
    
#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty,VT_GETSTATE, &vts)) {
	fim_perror("ioctl VT_GETSTATE");
	goto err;
    }
#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty,VT_ACTIVATE, vts.v_active)) {
	fim_perror("ioctl VT_ACTIVATE");
	goto err;
    }
#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty,VT_WAITACTIVE, vts.v_active)) {
	fim_perror("ioctl VT_WAITACTIVE");
	goto err;
    }
    return 0;
err:
	return -1;
}

fim_err_t FramebufferDevice::status_line(const fim_char_t *msg)
{
    int y;
    
    if (!visible_)
	goto ret;

    if(fb_var_.yres< 1U + f_->sheight() + ys_)
	/* we need enough pixels, and have no assumptions on weird visualization devices_ */
	goto rerr;

    y = fb_var_.yres -1 - f_->sheight() - ys_;
//    fb_memset(fb_mem_ + fb_fix_.line_length * y, 0, fb_fix_.line_length * (f_->height+ys_));
    clear_rect(0, fb_var_.xres-1, y+1,y+f_->sheight()+ys_);

    fb_line(0, fb_var_.xres, y, y);
    fs_puts(f_, 0, y+ys_, msg);
ret:
    return FIM_ERR_NO_ERROR;
rerr:
    return FIM_ERR_GENERIC;
}

void FramebufferDevice::fb_text_box(int x, int y, fim_char_t *lines[], unsigned int count)
{
    unsigned int i,len,max, x1, x2, y1, y2;

    if (!visible_)
	goto err;

    max = 0;
    for (i = 0; i < count; i++) {
	len = strlen(lines[i]);
	if (max < len)
	    max = len;
    }
    x1 = x;
    x2 = x + max * f_->width;
    y1 = y;
    y2 = y + count * f_->height;

    x += xs_; x2 += 2*xs_;
    y += ys_; y2 += 2*ys_;
    
    clear_rect(x1, x2, y1, y2);
    fb_rect(x1, x2, y1, y2);
    for (i = 0; i < count; i++) {
	fs_puts(f_,x,y,(const fim_char_t*)lines[i]);
	y += f_->height;
    }
err:
	return;
}

void FramebufferDevice::fb_line(int x1, int x2, int y1,int y2)
{
/*static void fb_line(int x1, int x2, int y1,int y2)*/
    int x,y,h;
    float inc;

    if (x2 < x1)
	h = x2, x2 = x1, x1 = h;
    if (y2 < y1)
	h = y2, y2 = y1, y1 = h;
    if (x2 - x1 < y2 - y1) {
	inc = (float)(x2-x1)/(float)(y2-y1);
	for (y = y1; y <= y2; y++) {
	    x = x1 + (int)(inc * (float)(y - y1));
	    fb_setpixel(x,y,fs_white_);
	}
    } else {
	inc = (float)(y2-y1)/(float)(x2-x1);
	for (x = x1; x <= x2; x++) {
	    y = y1 + (int)(inc * (float)(x - x1));
	    fb_setpixel(x,y,fs_white_);
	}
    }
}


void FramebufferDevice::fb_rect(int x1, int x2, int y1,int y2)
{
    fb_line(x1, x2, y1, y1);
    fb_line(x1, x2, y2, y2);
    fb_line(x1, x1, y1, y2);
    fb_line(x2, x2, y1, y2);
}

void FramebufferDevice::fb_setpixel(int x, int y, unsigned int color)
{
    fim_byte_t *ptr;

    ptr  = fb_mem_;
    ptr += y * fb_fix_.line_length;
    ptr += x * fs_bpp_;
    fs_setpixel(ptr, color);
}

void FramebufferDevice::fb_clear_rect(int x1, int x2, int y1,int y2)
{
    fim_byte_t *ptr;
    int y,h;

    if (!visible_)
	goto ret;

    if (x2 < x1)
	h = x2, x2 = x1, x1 = h;
    if (y2 < y1)
	h = y2, y2 = y1, y1 = h;
    ptr  = fb_mem_;
    ptr += y1 * fb_fix_.line_length;
    ptr += x1 * fs_bpp_;

    for (y = y1; y <= y2; y++) {
	fb_memset(ptr, 0, (x2 - x1 + 1) * fs_bpp_);
	ptr += fb_fix_.line_length;
    }
ret:
    return;
}

void FramebufferDevice::clear_screen(void)
{
    if (visible_)
	fb_memset(fb_mem_,0,fb_fix_.smem_len);
}

void FramebufferDevice::cleanup(void)
{
    /* restore console */
    if (-1 == ioctl(fb_,FBIOPUT_VSCREENINFO,&fb_ovar_))
	fim_perror("ioctl FBIOPUT_VSCREENINFO");
    if (-1 == ioctl(fb_,FBIOGET_FSCREENINFO,&fb_fix_))
	fim_perror("ioctl FBIOGET_FSCREENINFO");
#if 0
    printf("id:%s\t%ld\t%ld\t%ld\t\n",fb_fix_.id,fb_fix_.accel,fb_fix_.xpanstep,fb_fix_.xpanstep);
#endif
    if (fb_ovar_.bits_per_pixel == 8 ||
	fb_fix_.visual == FB_VISUAL_DIRECTCOLOR) {
	if (-1 == ioctl(fb_,FBIOPUTCMAP,&ocmap_))
	    fim_perror("ioctl FBIOPUTCMAP");
    }
    close(fb_);

#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,KDSETMODE, kd_mode_))
	fim_perror("ioctl KDSETMODE");
#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,VT_SETMODE, &vt_omode_))
	fim_perror("ioctl VT_SETMODE");
    if (orig_vt_no_ && -1 == ioctl(tty_, VT_ACTIVATE, orig_vt_no_))
	fim_perror("ioctl VT_ACTIVATE");
    if (orig_vt_no_ && -1 == ioctl(tty_, VT_WAITACTIVE, orig_vt_no_))
	fim_perror("ioctl VT_WAITACTIVE");

    // there's no need to restore the tty : this is performed outside ( 20081221 )
    //tcsetattr(tty_, TCSANOW, &term_);
    //close(tty_);
}

fim_byte_t * FramebufferDevice::convert_line_8(int bpp, int line, int owidth, fim_byte_t *dst, fim_byte_t *buffer, int mirror) FIM_NOEXCEPT/*fim mirror patch*/
{
    fim_byte_t  *ptr  = (fim_byte_t *)dst;
	dither_line(buffer, ptr, line, owidth, mirror);
	ptr += owidth;
	return ptr;
}

fim_byte_t * FramebufferDevice::convert_line(int bpp, int line, int owidth, fim_byte_t *dst, fim_byte_t *buffer, int mirror) FIM_NOEXCEPT/*fim mirror patch*/
{
    fim_byte_t  *ptr  = (fim_byte_t *)dst;
    unsigned short *ptr2 = (unsigned short*)dst;
    unsigned int  *ptr4 = (unsigned int *)dst;
    int x;
    int xm;/*fim mirror patch*/

    switch (fb_var_.bits_per_pixel) {
    case 8:
	dither_line(buffer, ptr, line, owidth, mirror);
	ptr += owidth;
	return ptr;
    case 15:
    case 16:
#ifdef FIM_IS_SLOWER_THAN_FBI
	/*swapped RGB patch*/
	for (x = 0; x < owidth; x++) {
            xm=mirror?owidth-1-x:x;
	    ptr2[xm] = lut_red_[buffer[x*3]] |
		lut_green_[buffer[x*3+1]] |
		lut_blue_[buffer[x*3+2]];
	}
#else /* FIM_IS_SLOWER_THAN_FBI */
	if(FIM_LIKELY(!mirror))
	for (x = 0; x < owidth; x++) {
	    ptr2[x] = lut_red_[buffer[x*3+2]] |
		lut_green_[buffer[x*3+1]] |
		lut_blue_[buffer[x*3]];
	}
	else
	for (x = 0,xm=owidth; x < owidth; x++) {
            xm--;
	    ptr2[xm] = lut_red_[buffer[x*3+2]] |
		lut_green_[buffer[x*3+1]] |
		lut_blue_[buffer[x*3]];
	}
#endif /* FIM_IS_SLOWER_THAN_FBI */
	ptr2 += owidth;
	return (fim_byte_t*)ptr2;
    case 24:
#ifdef FIM_IS_SLOWER_THAN_FBI
	for (x = 0; x < owidth; x++) {
            xm=mirror?owidth-1-x:x;
	    ptr[3*xm+2] = buffer[3*x+0];
	    ptr[3*xm+1] = buffer[3*x+1];
	    ptr[3*xm+0] = buffer[3*x+2];
	}
#else /* FIM_IS_SLOWER_THAN_FBI */
	/*swapped RGB patch*/
	if(FIM_LIKELY(!mirror))
	{
		/*
		 * this code could be faster if using processor specific routines..
		 * ... or maybe even not ?
		 */
		//owidth*=3;
#if 0
		for (x = 0; x < owidth; x+=3)
		{
	            ptr[x+2] = buffer[x+0];
		    ptr[x+1] = buffer[x+1];
		    ptr[x+0] = buffer[x+2];
		}
#else
		/*
		 * this is far worse than the preceding !
		 */
		memcpy(ptr,buffer,owidth*3);
		//FIM_REGISTER fim_char_t t;
		//FIM_REGISTER i=x;
		/*since RGB and GBR swap already done, this is not necessary*/
		/*for (i = 0; i < owidth; i+=3)
		{
	            t=ptr[i];
	            ptr[i]=ptr[i+2];
	            ptr[i+2]=t;
		}*/
#endif
		//owidth/=3;
	}
	else
	for (x = 0; x < owidth; x++) {
	    x*=3;
            xm=3*owidth-x-3;
	    ptr[xm+2] = buffer[x+2];
	    ptr[xm+1] = buffer[x+1];
	    ptr[xm+0] = buffer[x+0];
	    x/=3;
	}
#endif
	ptr += owidth * 3;
	return ptr;
    case 32:
#ifndef FIM_IS_SLOWER_THAN_FBI
	/*swapped RGB patch*/
	for (x = 0; x < owidth; x++) {
            xm=mirror?owidth-1-x:x;
	    ptr4[xm] = lut_red_[buffer[x*3+2]] |
		lut_green_[buffer[x*3+1]] |
		lut_blue_[buffer[x*3]];
	}
#else /* FIM_IS_SLOWER_THAN_FBI */
	if(FIM_LIKELY(!mirror))
	for (x = 0; x < owidth; x++) {
	    ptr4[x] = lut_red_[buffer[x*3]] |
		lut_green_[buffer[x*3+1]] |
		lut_blue_[buffer[x*3+2]];
	}
	else
	for (x = 0; x < owidth; x++) {
	    ptr4[owidth-1-x] = lut_red_[buffer[x*3]] |
		lut_green_[buffer[x*3+1]] |
		lut_blue_[buffer[x*3+2]];
	}
#endif /* FIM_IS_SLOWER_THAN_FBI */
	ptr4 += owidth;
	return (fim_byte_t*)ptr4;
    default:
	/* keep compiler happy */
	return FIM_NULL;
    }
}

fim_byte_t * FramebufferDevice::clear_line(int bpp, int line, int owidth, fim_byte_t *dst) FIM_NOEXCEPT 
{
    fim_byte_t  *ptr  = (fim_byte_t*)dst;
    unsigned short *ptr2 = (unsigned short*)dst;
    unsigned int  *ptr4 = (unsigned int*)dst;
    unsigned clear_byte=0x00;
#ifdef FIM_IS_SLOWER_THAN_FBI
    int x;
#endif /* FIM_IS_SLOWER_THAN_FBI */

    switch (fb_var_.bits_per_pixel) {
    case 8:
	fim_bzero(ptr, owidth);
	ptr += owidth;
	return ptr;
    case 15:
    case 16:
#ifdef FIM_IS_SLOWER_THAN_FBI
	for (x = 0; x < owidth; x++) {
	    ptr2[x] = 0x0;
	}
#else /* FIM_IS_SLOWER_THAN_FBI */
	std::fill_n(ptr,2*owidth,clear_byte);
#endif /* FIM_IS_SLOWER_THAN_FBI */
	ptr2 += owidth;
	return (fim_byte_t*)ptr2;
    case 24:
#ifdef FIM_IS_SLOWER_THAN_FBI
	for (x = 0; x < owidth; x++) {
	    ptr[3*x+2] = 0x0;
	    ptr[3*x+1] = 0x0;
	    ptr[3*x+0] = 0x0;
	}
#else /* FIM_IS_SLOWER_THAN_FBI */
	std::fill_n(ptr,3*owidth,clear_byte);
#endif /* FIM_IS_SLOWER_THAN_FBI */
	ptr += owidth * 3;
	return ptr;
    case 32:
#ifdef FIM_IS_SLOWER_THAN_FBI
	for (x = 0; x < owidth; x++) {
	    ptr4[x] = 0x0;
	}
#else /* FIM_IS_SLOWER_THAN_FBI */
	std::fill_n(ptr,4*owidth,clear_byte);
#endif /* FIM_IS_SLOWER_THAN_FBI */
	ptr4 += owidth;
	return (fim_byte_t*)ptr4;
    default:
	/* keep compiler happy */
	return FIM_NULL;
    }
}

void FramebufferDevice::init_dither(int shades_r, int shades_g, int shades_b, int shades_gray)
{
    int             i, j;
    fim_byte_t   low_shade, high_shade;
    unsigned short  index;
    float           red_colors_per_shade;
    float           green_colors_per_shade;
    float           blue_colors_per_shade;
    float           gray_colors_per_shade;

    red_mult_ = shades_g * shades_b;
    green_mult_ = shades_b;

    red_colors_per_shade = 256.0 / (shades_r - 1);
    green_colors_per_shade = 256.0 / (shades_g - 1);
    blue_colors_per_shade = 256.0 / (shades_b - 1);
    gray_colors_per_shade = 256.0 / (shades_gray - 1);

    /* this avoids a shift when checking these values */
    for (i = 0; i < DITHER_LEVEL; i++)
	for (j = 0; j < DITHER_LEVEL; j++)
	    DM[i][j] *= 0x10000;

    /*  setup arrays containing three bytes of information for red, green, & blue  */
    /*  the arrays contain :
     *    1st byte:    low end shade value
     *    2nd byte:    high end shade value
     *    3rd & 4th bytes:    ordered dither matrix index
     */

    for (i = 0; i < 256; i++) {

	/*  setup the red information  */
	{
	    low_shade = (fim_byte_t) (i / red_colors_per_shade);
	    high_shade = low_shade + 1;

	    index = (unsigned short)
		(((i - low_shade * red_colors_per_shade) / red_colors_per_shade) *
		 (DITHER_LEVEL * DITHER_LEVEL + 1));

	    low_shade *= red_mult_;
	    high_shade *= red_mult_;

	    red_dither_[i] = (index << 16) + (high_shade << 8) + (low_shade);
	}

	/*  setup the green information  */
	{
	    low_shade = (fim_byte_t) (i / green_colors_per_shade);
	    high_shade = low_shade + 1;

	    index = (unsigned short)
		(((i - low_shade * green_colors_per_shade) / green_colors_per_shade) *
		 (DITHER_LEVEL * DITHER_LEVEL + 1));

	    low_shade *= green_mult_;
	    high_shade *= green_mult_;

	    green_dither_[i] = (index << 16) + (high_shade << 8) + (low_shade);
	}

	/*  setup the blue information  */
	{
	    low_shade = (fim_byte_t) (i / blue_colors_per_shade);
	    high_shade = low_shade + 1;

	    index = (unsigned short)
		(((i - low_shade * blue_colors_per_shade) / blue_colors_per_shade) *
		 (DITHER_LEVEL * DITHER_LEVEL + 1));

	    blue_dither_[i] = (index << 16) + (high_shade << 8) + (low_shade);
	}

	/*  setup the gray information  */
	{
	    low_shade = (fim_byte_t) (i / gray_colors_per_shade);
	    high_shade = low_shade + 1;

	    index = (unsigned short)
		(((i - low_shade * gray_colors_per_shade) / gray_colors_per_shade) *
		 (DITHER_LEVEL * DITHER_LEVEL + 1));

	    gray_dither_[i] = (index << 16) + (high_shade << 8) + (low_shade);
	}
    }
}

void inline FramebufferDevice::dither_line(fim_byte_t *src, fim_byte_t *dst, int y, int width,int mirror) FIM_NOEXCEPT
{
    FIM_REGISTER long   a, b
#ifndef FIM_IS_SLOWER_THAN_FBI
    __attribute((aligned(16)))
#endif /* FIM_IS_SLOWER_THAN_FBI */
    ;

    long           *ymod, xmod;

    ymod = (long int*) DM[y & DITHER_MASK];
    /*	fim mirror patch */
    FIM_REGISTER const int inc=mirror?-1:1;
    dst=mirror?dst+width-1:dst;
    /*	fim mirror patch */
    if(FIM_UNLIKELY(width<1))
	    goto nodither; //who knows

#ifndef FIM_IS_SLOWER_THAN_FBI
    switch(width%8){
    	case 0:	goto dither0; break ;
    	case 1:	goto dither1; break ;
    	case 2:	goto dither2; break ;
    	case 3:	goto dither3; break ;
    	case 4:	goto dither4; break ;
    	case 5:	goto dither5; break ;
    	case 6:	goto dither6; break ;
    	case 7:	goto dither7; break ;
    }
#endif /* FIM_IS_SLOWER_THAN_FBI */

    while (FIM_LIKELY(width)) {

#if 0

 #define DITHER_CORE \
	xmod = --width & DITHER_MASK; \
\
	b = blue_dither_[*(src++)];  \
	b >>= (ymod[xmod] < b)?8:0; \
	a = green_dither_[*(src++)]; \
	a >>= (ymod[xmod] < a)?8:0; \
	b += a; \
	a = red_dither_[*(src++)]; \
	a >>= (ymod[xmod] < a)?8:0; \
	*(dst) = b+a & 0xff; \
    /*	fim mirror patch */
	dst+=inc;

#else
 #define DITHER_CORE \
	{ \
	width--; \
	xmod = width & DITHER_MASK; \
 	const long ymod_xmod=ymod[xmod]; \
\
	b = blue_dither_[*(src++)]; \
	if (ymod_xmod < b) \
	    b >>= 8; \
\
	a = green_dither_[*(src++)]; \
	if (ymod_xmod < a) \
	    a >>= 8; \
	b += a; \
\
	a = red_dither_[*(src++)]; \
	if (ymod_xmod < a) \
	    a >>= 8; \
	b += a; \
	*(dst) = b & 0xff; \
    /*	fim mirror patch 	*/ \
	dst+=inc; \
	} \
	/*	*(dst++) = b & 0xff;*/ 
#endif

#ifndef FIM_IS_SLOWER_THAN_FBI
dither0:
	DITHER_CORE
dither7:
	DITHER_CORE
dither6:
	DITHER_CORE
dither5:
	DITHER_CORE
dither4:
	DITHER_CORE
dither3:
	DITHER_CORE
dither2:
	DITHER_CORE
#endif /* FIM_IS_SLOWER_THAN_FBI */
dither1:
	DITHER_CORE
    }
nodither:
	return;
}
void FramebufferDevice::dither_line_gray(fim_byte_t *src, fim_byte_t *dst, int y, int width)
{
    long           xmod;
    FIM_REGISTER long   a;
    long           *ymod = (long int*) DM[y & DITHER_MASK];

    while (width--) {
	xmod = width & DITHER_MASK;

	a = gray_dither_[*(src++)];
	if (ymod[xmod] < a)
	    a >>= 8;

	*(dst++) = a & 0xff;
    }
}
void FramebufferDevice::fb_switch_release(void)
{
    ioctl(tty_, VT_RELDISP, 1);
    fb_switch_state_ = FB_INACTIVE;
    if (debug_)
	FIM_FPRINTF(stderr, "vt: release\n");
}
void FramebufferDevice::fb_switch_acquire(void)
{
    ioctl(tty_, VT_RELDISP, VT_ACKACQ);
    fb_switch_state_ = FB_ACTIVE;
    if (debug_)
	FIM_FPRINTF(stderr, "vt: acquire\n");
}
int FramebufferDevice::fb_switch_init(void)
{
    struct sigaction act,old;

    fim_bzero(&act,sizeof(act));
    
    ffdp=this;// WARNING : A DIRTY HACK
    act.sa_handler  = _fb_switch_signal;
    sigemptyset(&act.sa_mask);
    sigaction(SIGUSR1,&act,&old);
    sigaction(SIGUSR2,&act,&old);
#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,VT_GETMODE, &vt_mode_)) {
	fim_perror("ioctl VT_GETMODE");
	exit(1);
    }
    vt_mode_.mode   = VT_PROCESS;
    vt_mode_.waitv  = 0;
    vt_mode_.relsig = SIGUSR1;
    vt_mode_.acqsig = SIGUSR2;
    
#ifdef FIM_BOZ_PATCH
    if(!with_boz_patch_)
#endif /* FIM_BOZ_PATCH */
    if (-1 == ioctl(tty_,VT_SETMODE, &vt_mode_)) {
	fim_perror("ioctl VT_SETMODE");
	exit(1);
    }
    return 0;
}

void FramebufferDevice::fb_switch_signal(int signal)
{
    if (signal == SIGUSR1) {
	/* release */
	fb_switch_state_ = FB_REL_REQ;
	if (debug_)
	    FIM_FPRINTF(stderr, "vt: SIGUSR1\n");
    }
    if (signal == SIGUSR2) {
	/* acquisition */
	fb_switch_state_ = FB_ACQ_REQ;
	if (debug_)
	    FIM_FPRINTF(stderr, "vt: SIGUSR2\n");
    }
}


int FramebufferDevice::fb_text_init2(void)
{
    return fs_init_fb(255);
}
	int  FramebufferDevice::fb_font_width(void)const { return f_->swidth(); }
	int  FramebufferDevice::fb_font_height(void)const { return f_->sheight(); }

int FramebufferDevice::fs_init_fb(int white8)
{
    switch (fb_var_.bits_per_pixel) {
    case 8:
	fs_white_ = white8; fs_black_ = 0; fs_bpp_ = 1;
	fs_setpixel = setpixel1;
	break;
    case 15:
    case 16:
	if (fb_var_.green.length == 6)
	    fs_white_ = 0xffff;
	else
	    fs_white_ = 0x7fff;
	fs_black_ = 0; fs_bpp_ = 2;
	fs_setpixel = setpixel2;
	break;
    case 24:
	fs_white_ = 0xffffff; fs_black_ = 0; fs_bpp_ = fb_var_.bits_per_pixel/8;
	fs_setpixel = setpixel3;
	break;
    case 32:
	fs_white_ = 0xffffff; fs_black_ = 0; fs_bpp_ = fb_var_.bits_per_pixel/8;
	fs_setpixel = setpixel4;
	break;
    default:
	FIM_FPRINTF(stderr,  "Oops: %i bit/pixel ???\n",
		fb_var_.bits_per_pixel);
	return -1;
    }
    return 0;
}

#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	FramebufferDevice::FramebufferDevice(MiniConsole& mc):	
	DisplayDevice(mc)
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	FramebufferDevice::FramebufferDevice(void):	
	DisplayDevice()
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	,vt_(0)
	,dither_(FIM_FBI_FALSE)
	,fbgamma_(FIM_CNS_GAMMA_DEFAULT)
	,fs_bpp_(), fs_black_(), fs_white_()
	,visible_(1)
	//,x11_font_("10x20")
	,ys_( 3)
	,xs_(10)
	,debug_(FIM_WANT_FBI_FBDEV_DIAGNOSTICS)
	,fs_setpixel(FIM_NULL)
	,fbdev_(FIM_NULL)
	,fbmode_(FIM_NULL)
#ifdef FIM_BOZ_PATCH
	,with_boz_patch_(0)
#endif /* FIM_BOZ_PATCH */
	,fb_mem_offset_(0)
	,fb_switch_state_(FB_ACTIVE)
	,orig_vt_no_(0)
	,devices_(FIM_NULL)
#ifndef FIM_KEEP_BROKEN_CONSOLE
	//mc_(48,12),
//	int R=(fb_var_.yres/fb_font_height())/2,/* half screen */
//	C=(fb_var_.xres/fb_font_width());
#endif /* FIM_KEEP_BROKEN_CONSOLE */
	{
		const fim_char_t *line;

		cmap_.start  =  0;
		cmap_.len    =  256;
		cmap_.red  =  red_;
		cmap_.green  =  green_;
		cmap_.blue  =  blue_;
		//! transp!
		devs_default_.fb0=   FIM_DEFAULT_FB_FILE;
		devs_default_.fbnr=  "/dev/fb%d";
		devs_default_.ttynr= "/dev/tty%d";
		devs_devfs_.fb0=   "/dev/fb/0";
		devs_devfs_.fbnr=  "/dev/fb/%d";
		devs_devfs_.ttynr= "/dev/vc/%d";
		ocmap_.start = 0;
		ocmap_.len   = 256;
		ocmap_.red=ored_;
		ocmap_.green=ogreen_;
		ocmap_.blue=oblue_;

	    	if (FIM_NULL != (line = fim_getenv(FIM_ENV_FBGAMMA)))
	        	fbgamma_ = fim_atof(line);
	}

}

fim_err_t FramebufferDevice::display(
	const void *ida_image_img,
	fim_coo_t yoff,
	fim_coo_t xoff,
	fim_coo_t irows,fim_coo_t icols,// rows and columns in the input image
	fim_coo_t icskip,	// input columns to skip for each line
	fim_coo_t by,
	fim_coo_t bx,
	fim_coo_t bh,
	fim_coo_t bw,
	fim_coo_t ocskip,// output columns to skip for each line
	fim_flags_t flags)
{
	if(by<0)
		goto err;
	if(bx<0)
		goto err;
	if(bw<0)
		goto err;
	if(bh<0)
		goto err;

	/* 
	 * Uncommenting this clear_rect() might fix a few cases where a clear
	 * screen seems missing, but also introduce flickering --- this has to
	 * be fixed differently than enabling this as done in r882.
	 */
	// clear_rect(  0, width()-1, 0, height()-1);
	svga_display_image_new(
		(const struct ida_image*)ida_image_img,
		yoff,
		xoff,
		irows,icols,// rows and columns in the input image
		icskip,	// input columns to skip for each line
		by,
		bx,
		bh,
		bw,
		ocskip,// output columns to skip for each line
		flags);
	redraw_ = FIM_REDRAW_UNNECESSARY;
	return FIM_ERR_NO_ERROR;
err:
	return FIM_ERR_GENERIC;
}

void FramebufferDevice::finalize (void)
{
	finalized_=true;
	clear_screen();
	cleanup();
}

FramebufferDevice::~FramebufferDevice(void)
{
}

fim_coo_t FramebufferDevice::status_line_height(void)const
{
	return f_ ? border_height_ + f_->sheight() : 0;
}
#endif  //ifdef FIM_WITH_NO_FRAMEBUFFER, else

