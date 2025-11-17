/* $LastChangedDate: 2023-03-26 15:47:40 +0200 (Sun, 26 Mar 2023) $ */
/*
 Imlib2.cpp : Imlib2 device Fim driver file

 (c) 2011-2023 Michele Martone

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
 * NOTES : The Imlib2 support here is INCOMPLETE
 * This code is horrible, inefficient, and error handling is MISSING.
 * Flip/mirror display is missing.
 * Fullscreen display is missing.
 * Mouse handling is missing.
 * We use so little of imlib2 here that it would be better to use X only.
 */
#include "fim.h"

#ifdef FIM_WITH_LIBIMLIB2

#include "Imlib2Device.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#define FIM_IL2_PRINTF printf

#define FIM_IMLIB2_X_INPUT_MASK NoEventMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask 
#define FIM_IMLIB2_X_WINDOW_MASK NoEventMask | StructureNotifyMask 
#define FIM_IMLIB2_X_MASK  FIM_IMLIB2_X_INPUT_MASK | ExposureMask | FIM_IMLIB2_X_WINDOW_MASK  

namespace fim
{
	extern CommandConsole cc;
}

// FIXME: these shall become Imlib2Device members!
static Display *disp=FIM_NULL;
static Visual  *vis=FIM_NULL;
static int      depth;
static bool      initialized=false;
static Colormap cm;
static Window   win;

#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	Imlib2Device::Imlib2Device(MiniConsole& mc_, fim::string opts):DisplayDevice(mc_),
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	Imlib2Device::Imlib2Device(
			fim::string opts
			):DisplayDevice(),
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
   	current_w_(FIM_DEFAULT_WINDOW_HEIGHT), current_h_(FIM_DEFAULT_WINDOW_WIDTH)
	,want_windowed_(false)
	{
		FontServer::fb_text_init1(fontname_,&f_);	// FIXME : move this outta here
		const fim_char_t*const os=opts.c_str();
		parse_optstring(os);
	}

fim_bpp_t Imlib2Device::get_bpp(void)const{return depth; }

fim_err_t Imlib2Device::parse_optstring(const fim_char_t *os)
{
	bool want_windowed=want_windowed_;
	bool want_mouse_display=want_mouse_display_;
	bool want_resize=want_resize_;
	fim_coo_t current_w=current_w_;
	fim_coo_t current_h=current_h_;

		if(os)
		{
			while(*os&&!isdigit(*os))
			{
				bool tv=true;
				if(isupper(*os))
					tv=false;
				switch(tolower(*os)){
					case 'w': want_windowed=tv; break;
					//case 'm': want_mouse_display=tv; break;
					//case 'r': want_resize=tv; break;
					default: std::cerr<<"unrecognized specifier character \""<<*os<<"\"\n";goto err;
				}
				++os;
			}
		if(*os)
		{
		if(2==sscanf(os,"%d:%d",&current_w,&current_h))

		{
		//	std::cout << w << " : "<< h<<"\n";
			current_w=FIM_MAX(current_w,0);
			current_h=FIM_MAX(current_h,0);
			if(!allowed_resolution(current_w,current_h))
				goto err;
		}
		else
		{
			current_w=current_h=0;
			std::cerr << "user specification of resolution (\""<<os<<"\") wrong: it shall be in \"width:height\" format! \n";
			// TODO: a better invaling string message needed here
		}
		}
		}
		// commit
		want_windowed_=want_windowed;
		want_mouse_display_=want_mouse_display;
		want_resize_=want_resize;
		current_w_=current_w;
		current_h_=current_h;
		return FIM_ERR_NO_ERROR;
err:
		return FIM_ERR_GENERIC;
}

	fim_err_t Imlib2Device::display(
		//struct ida_image *img, // source image structure
		const void *ida_image_img, // source image structure
		//void* rgb,// source rgb array
		fim_coo_t iroff,fim_coo_t icoff, // row and column offset of the first input pixel
		fim_coo_t irows,fim_coo_t icols,// rows and columns in the input image
		fim_coo_t icskip,	// input columns to skip for each line
		fim_coo_t oroff,fim_coo_t ocoff,// row and column offset of the first output pixel
		fim_coo_t orows,fim_coo_t ocols,// rows and columns to draw in output buffer
		fim_coo_t ocskip,// output columns to skip for each line
		fim_flags_t flags// some flags
	)
	{
		fim_coo_t idr,idc,lor,loc;
		fim_coo_t ii,ij;
		fim_coo_t oi,oj;
		// fim_flags_t mirror=flags&FIM_FLAG_MIRROR, flip=flags&FIM_FLAG_FLIP;//STILL UNUSED : FIXME
		fim_byte_t * srcp=FIM_NULL;
   		//Imlib_Image dimage;
		struct ida_image*img=FIM_NULL;
		fim_byte_t* rgb = FIM_NULL;
		DATA32 *ild=FIM_NULL;

		if( iroff <0 ) return -2;
		if( icoff <0 ) return -3;
		if( irows <=0 ) return -4;
		if( icols <=0 ) return -5;
		if( icskip<0 ) return -6;
		if( oroff <0 ) return -7;
		if( ocoff <0 ) return -8;
		if( orows <=0 ) return -9;
		if( ocols <=0 ) return -10;
		if( ocskip<0 ) return -11;
		if( flags <0 ) return -12;

		if( iroff>irows ) return -2-3*100 ;
		if( icoff>icols ) return -3-5*100;
//		if( oroff>orows ) return -7-9*100;//EXP
//		if( ocoff>ocols ) return -8-10*100;//EXP
		if( oroff>height() ) return -7-9*100;//EXP
		if( ocoff>width()) return -8-10*100;//EXP

		if( icskip<icols ) return -6-5*100;
		if( ocskip<ocols ) return -11-10*100;
	
		orows  = FIM_MIN( orows, height());
		ocols  = FIM_MIN( ocols,  width()); 
		ocskip = width(); 	//FIXME maybe this is not enough and should be commented or rewritten!

		if( orows  > height() ) return -9 -99*100;
		if( ocols  >  width() ) return -10-99*100;
		if( ocskip <  width() ) return -11-99*100;
		if( icskip<icols ) return -6-5*100;

		ocskip = width();// output columns to skip for each line

		if(icols<ocols) { ocoff+=(ocols-icols-1)/2; ocols-=(ocols-icols-1)/2; }
		if(irows<orows) { oroff+=(orows-irows-1)/2; orows-=(orows-irows-1)/2; }

		idr = iroff-oroff;
		idc = icoff-ocoff;

		lor = (FIM_MIN(orows-1,irows-1-iroff+oroff));
		loc = (FIM_MIN(ocols-1,icols-1-icoff+ocoff));

		img=(struct ida_image*)ida_image_img;
		rgb = img?img->data:FIM_NULL;// source rgb array

		if(initialized==false) goto ret;

		clear_rect(  0, width()-1, 0, height()-1); 
		if(!img) goto ret;
		if(!rgb) goto ret;
		ild=imlib_image_get_data();
		for(oi=oroff;FIM_LIKELY(oi<lor);++oi)
		for(oj=ocoff;FIM_LIKELY(oj<loc);++oj)
		{
			ii    = oi + idr;
			ij    = oj + idc;
			srcp  = ((fim_byte_t*)rgb)+(3*(ii*icskip+ij));
			fim_byte_t*dstp =((fim_byte_t*)ild  )+(4*(oi*ocskip+oj));
			*dstp=*srcp;
			++dstp; ++srcp; *dstp=*srcp;
			++dstp; ++srcp; *dstp=*srcp;
			++dstp; ++srcp;
		}
             imlib_render_image_on_drawable(0,0);
ret:
		return FIM_ERR_NO_ERROR;
	}

	void Imlib2Device::apply_fullscreen(void)
	{
		Atom prop_fs=XInternAtom(disp,"_NET_WM_STATE_FULLSCREEN",True);
		Atom prop_state=XInternAtom(disp,"_NET_WM_STATE",True);
		XChangeProperty(disp,win,prop_state,XA_ATOM,32,PropModeReplace,(unsigned char*)&prop_fs,want_windowed_?0:1);
	}

	void Imlib2Device::toggle_fullscreen(void)
	{
		want_windowed_=!want_windowed_;
		apply_fullscreen();
	}

	fim_err_t Imlib2Device::il2_initialize(void)
	{
		if(!disp)
   		disp=XOpenDisplay(FIM_NULL);
		if(!disp)goto err;
		if(!vis)
   		vis=DefaultVisual(disp,DefaultScreen(disp));
		if(!vis)goto err;
		if(win)
		XDestroyWindow(disp,win);
   		depth=DefaultDepth(disp,DefaultScreen(disp));
   		cm=DefaultColormap(disp,DefaultScreen(disp));
#if 0
   		win = XCreateSimpleWindow(disp, DefaultRootWindow(disp), 0, 0, current_w_, current_h_, 0, 0, 0);
#else
		{
		XSetWindowAttributes attr;
        	attr.backing_store = NotUseful;
        	attr.override_redirect = False;
        	attr.colormap = cm;
        	attr.border_pixel = 0;
        	attr.background_pixel = 0;
        	attr.save_under = False;
        	attr.event_mask =
            StructureNotifyMask | ButtonPressMask | ButtonReleaseMask |
            PointerMotionMask | EnterWindowMask | LeaveWindowMask |
            KeyPressMask | KeyReleaseMask | ButtonMotionMask | ExposureMask
            | FocusChangeMask | PropertyChangeMask | VisibilityChangeMask;

		win = XCreateWindow(disp, DefaultRootWindow(disp), 0, 0, current_w_, current_h_, 0,
                          depth, InputOutput, vis,
                          CWOverrideRedirect | CWSaveUnder | CWBackingStore
                          | CWColormap | CWBackPixel | CWBorderPixel | CWEventMask, &attr);
		}
#endif
   		XSelectInput(disp,win,FIM_IMLIB2_X_MASK);
		apply_fullscreen();
   		XMapWindow(disp,win);
		/* FIXME: omitting cache sizes --- we do not load neither images, nor fonts */
		/* FIXME: not handling dithering */
		imlib_context_set_display(disp);
		imlib_context_set_visual(vis);
		imlib_context_set_colormap(cm);
		imlib_context_set_drawable(win);
		resize(current_w_,current_h_);
		return FIM_ERR_NO_ERROR;
err:
		return FIM_ERR_GENERIC;
	}

static fim_err_t initialize_keys(sym_keys_t &sym_keys)
{
		sym_keys[FIM_KBD_BACKSPACE]=XK_BackSpace;
		sym_keys[FIM_KBD_DEL]=XK_Delete;
		sym_keys[FIM_KBD_ESC]=XK_Escape;
		sym_keys[FIM_KBD_PAGEUP]=XK_KP_Page_Up;
		sym_keys[FIM_KBD_PAGEDOWN]=XK_KP_Page_Down;
		sym_keys[FIM_KBD_LEFT]=XK_Left;
		sym_keys[FIM_KBD_RIGHT]=XK_Right;
		sym_keys[FIM_KBD_UP]=XK_Up;
		sym_keys[FIM_KBD_DOWN]=XK_Down;
		sym_keys[FIM_KBD_SPACE]=XK_space;
		sym_keys[FIM_KBD_END]=XK_KP_End;
		sym_keys[FIM_KBD_HOME]=XK_KP_Home;
		//XK_KP_Begin
		//sym_keys["F1" ]=SDLK_F1;
		sym_keys["0"]=XK_KP_0;
		sym_keys["1"]=XK_KP_1;
		sym_keys["2"]=XK_KP_2;
		sym_keys["3"]=XK_KP_3;
		sym_keys["4"]=XK_KP_4;
		sym_keys["5"]=XK_KP_5;
		sym_keys["6"]=XK_KP_6;
		sym_keys["7"]=XK_KP_7;
		sym_keys["8"]=XK_KP_8;
		sym_keys["9"]=XK_KP_9;
		sym_keys[FIM_KBD_PLUS]=XK_KP_Add;
		sym_keys[FIM_KBD_MINUS]=XK_KP_Subtract;
		sym_keys[FIM_KBD_SLASH]=XK_KP_Divide;
		sym_keys[FIM_KBD_ASTERISK]=XK_KP_Multiply;
		sym_keys[FIM_KBD_GT]=XK_greater;
		sym_keys[FIM_KBD_LT]=XK_less;
		sym_keys[FIM_KBD_UNDERSCORE]=XK_underscore;
		sym_keys["F1" ]=XK_F1;
		sym_keys["F2" ]=XK_F2;
		sym_keys["F3" ]=XK_F3;
		sym_keys["F4" ]=XK_F4;
		sym_keys["F5" ]=XK_F5;
		sym_keys["F6" ]=XK_F6;
		sym_keys["F7" ]=XK_F7;
		sym_keys["F8" ]=XK_F8;
		sym_keys["F9" ]=XK_F9;
		sym_keys["F10"]=XK_F10;
		sym_keys["F11"]=XK_F11;
		sym_keys["F12"]=XK_F12;
		sym_keys[FIM_KBD_COLON]=XK_colon;
		sym_keys[FIM_KBD_SEMICOLON]=XK_semicolon;
		//sym_keys[FIM_KBD_a]=XK_a;
		//XK_bar
		//XK_minus
		//XK_plus
		fim_perror(FIM_NULL);
		cc.key_syms_update();
		return FIM_ERR_NO_ERROR;
}

	fim_err_t Imlib2Device::initialize(sym_keys_t &sym_keys)
	{
		/* FIXME */
		initialize_keys(sym_keys);
		return il2_initialize();
	}

	void Imlib2Device::finalize(void)
	{
		finalized_=true;
	}

	fim_coo_t Imlib2Device::get_chars_per_column(void)const
	{
		return height() / f_->height;
	}

	fim_coo_t Imlib2Device::get_chars_per_line(void)const
	{
		return width() / f_->width;
	}

	fim_coo_t Imlib2Device::width(void)const
	{
		return current_w_;
	}

	fim_coo_t Imlib2Device::height(void)const
	{
		return current_h_;
	}

	fim_sys_int Imlib2Device::get_input(fim_key_t * c, bool want_poll)
	{
		return get_input_i2l(c);
	}

fim_sys_int Imlib2Device::get_input_i2l(fim_key_t * c)
{
	int rc=-1;
	fim_key_t pk=0;

        updates_ = imlib_updates_init();
	// if(True==XCheckMaskEvent(disp,FIM_IMLIB2_X_INPUT_MASK /*KeyPressMask,&ev_))
	// if(True==XCheckMaskEvent(disp,FIM_IMLIB2_X_INPUT_MASK,&ev_))
	if(True==XCheckMaskEvent(disp,KeyPressMask,&ev_))
	{
		switch (ev_.type)
		{
			case ConfigureNotify:
			case ConfigureRequest:
			case ResizeRequest:
			case ResizeRedirectMask:
			case StructureNotifyMask:
			break;
			case KeyPress:
			{
				char buf[FIM_ATOX_BUFSIZE];
				int nc=0;
				KeySym  ks;
				buf[nc]=FIM_SYM_CHAR_NUL;
      				nc=XLookupString(&ev_.xkey,buf,sizeof(buf),&ks,FIM_NULL);
				buf[nc]=FIM_SYM_CHAR_NUL;
				//FIM_IL2_PRINTF("PKEY :%d:%s:%d\n",ev_.xkey.keycode,buf,ks);
				if( *buf)
					rc=1;
				else
					break;
				if(nc==1)
					pk=*buf;
				else
					pk=ks;
				//if(ev_.xkey.keycode==23)pk='\t';/* FIXME */
				//FIM_IL2_PRINTF("PRESSED :%d %c\n",pk,pk);
      				ks=XLookupKeysym(&ev_.xkey,0);
				if(ks!=NoSymbol)
					;//FIM_IL2_PRINTF("SYM :%d %c\n",ks,ks);
				else
					rc=0;
			}
			break;
			case KeyRelease:
			//FIM_IL2_PRINTF("KEY RELEASE:%d %c\n",0,0);
			break;
			case Expose:
			updates_=imlib_update_append_rect(updates_,ev_.xexpose.x,ev_.xexpose.y,ev_.xexpose.width,ev_.xexpose.height);
			break;
			case ButtonPress:
			//FIM_IL2_PRINTF("MOUSE PRESSED :%d %d\n",ev_.xbutton.x,ev_.xbutton.y);
			break;
			case MotionNotify:
			// FIM_IL2_PRINTF("MOTION NOTIFY:%d %c\n",0,0);
			break;
			default:
			break;
               }
	}
        updates_=imlib_updates_merge_for_rendering(updates_,current_w_,current_h_);
        for (current_update_=updates_; 
             current_update_; 
             current_update_ = imlib_updates_get_next(current_update_))
		; // FIXME; shall restructure the drawing functions sequence to use imlib_updates_merge_for_rendering

        if (updates_)
           imlib_updates_free(updates_),
           updates_=FIM_NULL;

	if(!c)
		rc=0;
	else
	{
		if(rc==1)
		*c=pk;
	}

 	XWindowAttributes attributes;
 	XGetWindowAttributes(disp,win,&attributes);
	if(current_w_!=attributes.width || current_h_!=attributes.height)
	{
		// FIXME: move outta here this check & resize (shall intercept the event first, though)
 		current_w_=attributes.width;
 		current_h_=attributes.height;
		cc.display_resize(current_w_,current_h_);
	}
	return rc;
}

	fim_err_t Imlib2Device::fill_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2, fim_color_t color)
	{
		// FIXME: this code may be not portable
		fim_color_t rmask=0xFF000000,gmask=0x00FF0000,bmask=0x0000FF00,amask=0x000000FF;
		if(initialized==false) goto ret;
		imlib_context_set_color((color&rmask)>>24,(color&gmask)>>16,(color&bmask)>>8,(color&amask));
		imlib_image_fill_rectangle(x1,y1,x2-x1+1,y2-y1+1);
		imlib_render_image_on_drawable(0,0);
ret:
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t Imlib2Device::clear_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2)
	{
		// FIXME: this code may be not portable
		if(initialized==false) goto ret;
		imlib_context_set_color(0x0,0x0,0x0,0xFF);
		imlib_image_fill_rectangle(x1,y1,x2-x1+1,y2-y1+1);
		imlib_render_image_on_drawable(0,0);
ret:
		return FIM_ERR_NO_ERROR;
	}

void Imlib2Device::fs_render_fb(fim_coo_t x_, fim_coo_t y, FSXCharInfo *charInfo, fim_byte_t *data)
{
	if(initialized==false) goto err;
/* 
 * These preprocessor macros should serve *only* for font handling purposes.
 * */
#define BIT_ORDER       BitmapFormatBitOrderMSB
#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
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

	fim_coo_t row,bit,x;
	fim_sys_int bpr;
	DATA32 *ild;
	ild=imlib_image_get_data();
	if(!ild)goto err;
	bpr = GLWIDTHBYTESPADDED((charInfo->right - charInfo->left), SCANLINE_PAD_BYTES);
	for (row = 0; row < (charInfo->ascent + charInfo->descent); row++)
	{
		for (x = 0, bit = 0; bit < (charInfo->right - charInfo->left); bit++) 
		{
			if (data[bit>>3] & fs_masktab[bit&7])
			{
				// WARNING !
				ild[((y+row)*(current_w_)+(x_+x))]=0xFFFFFFFF;
			}
			++x;
		}
		data += bpr;
	}

#undef BIT_ORDER
#undef BYTE_ORDER
#undef SCANLINE_UNIT
#undef SCANLINE_PAD
#undef EXTENTS
#undef SCANLINE_PAD_BYTES
#undef GLWIDTHBYTESPADDED
err:
	return;
}

fim_err_t Imlib2Device::fs_puts(struct fs_font *f_, fim_coo_t x, fim_coo_t y, const fim_char_t *str)
{
    fim_err_t rc=FIM_ERR_GENERIC;
    fim_sys_int i,c/*,j,w*/;
	if(initialized==false)
	       	goto ret;

    for (i = 0; str[i] != '\0'; i++) {
	c = (fim_byte_t)str[i];
	if (FIM_NULL == f_->eindex[c])
	    continue;
	fs_render_fb(x,y,f_->eindex[c],f_->gindex[c]);
	x += f_->eindex[c]->width;
	/* FIXME : SLOW ! */
	if (((fim_coo_t)x) > width() - f_->width)
		goto derr; /* FIXME: seems like this is often triggered. */
    }
    	rc=FIM_ERR_NO_ERROR;
derr:
	imlib_render_image_on_drawable(0,0);
ret:
	return rc;
}

	fim_err_t Imlib2Device::status_line(const fim_char_t *msg)
	{
		fim_coo_t y,ys=3;// FIXME

		if(get_chars_per_column()<1)
			goto done;
		y = height() - f_->height - ys;
		if(y<0 )
			goto done;
		clear_rect(0, width()-1, y+1,y+f_->height+ys-1);
		fs_puts(f_, 0, y+ys, msg);
		fill_rect(0,width()-1, y, y, FIM_CNS_WHITE);
done:
		return FIM_ERR_NO_ERROR;
	}

	fim_key_t Imlib2Device::catchInteractiveCommand(fim_ts_t seconds)const
	{
		// FIXME: missing handling code, here
		return -1;
	}

	void Imlib2Device::flush(void)
	{
	}

	void Imlib2Device::lock(void)
	{
	}

	void Imlib2Device::unlock(void)
	{
	}

	bool Imlib2Device::allowed_resolution(fim_coo_t w, fim_coo_t h)
	{
		return true;
	}

	fim_err_t Imlib2Device::resize(fim_coo_t w, fim_coo_t h)
	{
		if(initialized==true)
			XResizeWindow(disp,win,w,h),
			imlib_free_image();
		initialized=true;
             	buffer_=imlib_create_image(current_w_,current_h_);
		imlib_context_set_image(buffer_);
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t Imlib2Device::reinit(const fim_char_t *rs)
	{
		if(parse_optstring(rs)!=FIM_ERR_NO_ERROR)
			goto err;
		return cc.display_resize(current_w_,current_h_);
	err:
		//std::cerr<<"problems!\n";
		return FIM_ERR_GENERIC;
	}

	fim_err_t Imlib2Device::set_wm_caption(const fim_char_t *msg)
	{
		// FIXME: unfinished
#if 1
		fim_err_t rc=FIM_ERR_UNSUPPORTED;
#else
		fim_err_t rc=FIM_ERR_NO_ERROR;
		if(!msg)
			goto err;
       		XStoreName(disp,win,msg);
err:
#endif
		return rc;
	}
	
	fim_err_t Imlib2Device::reset_wm_caption(void)
	{
		// FIXME: unfinished
#if 1
       		XStoreName(disp,win,"");
		return FIM_ERR_NO_ERROR;
#else
		return FIM_ERR_UNSUPPORTED;
#endif
	}
#endif

	fim_coo_t Imlib2Device::status_line_height(void)const
	{
		return f_ ? border_height_ + f_->height : 0;
	}
