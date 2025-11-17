/* $LastChangedDate: 2024-05-09 00:45:30 +0200 (Thu, 09 May 2024) $ */
/*
 SDLDevice.cpp : sdllib device Fim driver file

 (c) 2008-2024 Michele Martone
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

#ifdef FIM_WITH_LIBSDL

#include "SDLDevice.h"
#if (FIM_WITH_LIBSDL_VERSION == 1)
#define FIM_SDL_FLAGS /*SDL_FULLSCREEN|*/SDL_HWSURFACE
#else /* FIM_WITH_LIBSDL_VERSION == 2  <- experimental */
#define FIM_SDL_FLAGS 0
#define SDL_FULLSCREEN SDL_WINDOW_FULLSCREEN_DESKTOP // :-)
#define SDL_RESIZABLE SDL_WINDOW_RESIZABLE // :-)
#endif /* FIM_WITH_LIBSDL_VERSION */

#if FIM_WASM_DEFAULTS
#define sdld_cerr std::cout
#define sdld_cout std::cout
#else /* FIM_WASM_DEFAULTS */
#define sdld_cerr std::cerr
#define sdld_cout std::cout
#endif /* FIM_WASM_DEFAULTS */

#define FIM_WANT_HARDCODED_ICON 1
#define FIM_SDL_ICONPATH ""
#define FIM_FRAC(VAL,N,D) (((VAL)*(N))/(D))
#define FIM_SDL_KEYSYM_TO_RL(X) ((X) | (1<<31)) // see readline.cpp
#define FIM_SDL_KEYSYM_TO_RL_CTRL(X) 1+((X)-'a');

namespace fim
{
	extern fim_int fim_fmf_; /* FIXME */
	extern CommandConsole cc;
}

#define FIM_SDL_MINWIDTH 2
#define FIM_SDL_MINHEIGHT 2

#define FIM_SDL_ALLOW_QUIT 1
#define FIM_SDL_WANT_KEYREPEAT 1
#define FIM_SDL_WANT_RESIZE 1
#define FIM_SDL_WANT_PERCENTAGE 1
#define FIM_SDL_DEBUG 1
#undef FIM_SDL_DEBUG
#define FIM_WANT_SDL_CLICK_MOUSE_SUPPORT FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
#define FIM_WANT_SDL_CLICK_MENU FIM_WANT_SDL_CLICK_MOUSE_SUPPORT && 1

#if FIM_WANT_MOUSE_CROP
namespace fim
{
	static SDL_Rect lastsrcrect;
	static SDL_Rect lastdstrect;
}
#endif /* FIM_WANT_MOUSE_CROP */

#ifdef FIM_SDL_DEBUG
#define FIM_SDL_DBG_COUT sdld_cout << "SDL:" << __FILE__ ":" << __LINE__ << ":" << __func__ << "() "
#define FIM_SDL_INPUT_DEBUG(C,MSG)  \
{ \
/* i miss sooo much printf() :'( */ \
FIM_SDL_DBG_COUT << (size_t)getmilliseconds() << " : "<<MSG<<" : "; \
sdld_cout.setf ( std::ios::hex, std::ios::basefield ); \
sdld_cout.setf ( std::ios::showbase ); \
sdld_cout << *(fim_key_t*)(C) <<"\n"; \
sdld_cout.unsetf ( std::ios::showbase ); \
sdld_cout.unsetf ( std::ios::hex ); \
}
#else /* FIM_SDL_DEBUG */
#define FIM_SDL_INPUT_DEBUG(C,MSG) {}
#endif /* FIM_SDL_DEBUG */
#define FIM_WANT_POSITION_DISPLAYED FIM_WANT_MOUSE_PAN && 0

typedef int fim_sdl_int;

#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
static struct fim_grid_drawer_t grid_drawer;
static fim_coo_t g_last_click_x{-1};
static fim_coo_t g_last_click_y{-1};

static void flip_draw_help_map_tmp(bool flip)
{
#if FIM_WANT_SDL_CLICK_MENU
	if(!flip)
		if(grid_drawer.fim_draw_help_map_tmp_==0)
			return; // nothing to do
	grid_drawer.fim_draw_help_map_tmp_=1-grid_drawer.fim_draw_help_map_tmp_;
	if(grid_drawer.fim_draw_help_map_tmp_)
	if(Viewport* cv = cc.current_viewport())
		cv->redisplay(); // will indirectly trigger draw_help_map()
#endif /* FIM_WANT_SDL_CLICK_MENU */
}
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */

fim_err_t SDLDevice::parse_optstring(const fim_char_t *os)
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
				case 'm': want_mouse_display=tv; break;
				case 'r': want_resize=tv; break;
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
				case 'h': grid_drawer.toggle_draw_help_map(); if(!tv) grid_drawer.fim_draw_help_map_=0; break;
#endif
				default: sdld_cerr <<"unrecognized specifier character \""<<*os<<"\"\n";goto err;
			}
			++os;
		}
		if(*os)
		{
			if(const int si = sscanf(os,"%d:%d",&current_w,&current_h))
			{
				if ( si == 1)
					current_h = current_w;
#if FIM_SDL_WANT_PERCENTAGE
				if ( strrchr(os,'%') && !strrchr(os,'%')[1] )
#endif
					current_w = FIM_MIN(current_w, 100),
					current_h = FIM_MIN(current_h, 100);
			}
			else
			{
				current_w = current_h = 0;
				sdld_cerr << "user specification of resolution (\""<<os<<"\") wrong: it shall be in \"width:height\" format! \n";
			}
			current_w_=FIM_MAX(current_w,0);
			current_h_=FIM_MAX(current_h,0);
			// sdld_cout << current_w << " : "<< current_h<<"\n";
#if FIM_SDL_WANT_PERCENTAGE
			if ( ! ( strrchr(os,'%') && !strrchr(os,'%')[1] ) )
#endif
			if(!allowed_resolution(current_w_,current_h_))
			{
				// sdld_cerr << "setting to auto detection (0:0)\n";
				current_w = current_h = 0;
			}
		}
	}
	want_windowed_=want_windowed;
	want_mouse_display_=want_mouse_display;
#if FIM_SDL_WANT_RESIZE 
	want_resize_=want_resize;
#else /* FIM_SDL_WANT_RESIZE */
	want_resize_=false;
#endif /* FIM_SDL_WANT_RESIZE */
	current_w_=current_w;
	current_h_=current_h;
	return FIM_ERR_NO_ERROR;
err:
	return FIM_ERR_GENERIC;
}

#if FIM_WANT_POSITION_DISPLAYED
static int gx,gy;
#endif /* FIM_WANT_POSITION_DISPLAYED */

#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	SDLDevice::SDLDevice(MiniConsole& mc_, fim::string opts):DisplayDevice(mc_),
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	SDLDevice::SDLDevice(
			fim::string opts
			):DisplayDevice(),
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	screen_(FIM_NULL),
#if (FIM_WITH_LIBSDL_VERSION == 1)
	vi_(FIM_NULL),
#else /* FIM_WITH_LIBSDL_VERSION */
	texture_(FIM_NULL),
	wi_(FIM_NULL),
	re_(FIM_NULL),
#endif /* FIM_WITH_LIBSDL_VERSION */
	current_w_(0), current_h_(0),
	Bpp_(FIM_CNS_BPP_INVALID),
	bpp_(FIM_CNS_BPP_INVALID),
	opts_(opts),
	want_windowed_(FIM_SDL_FLAGS & SDL_FULLSCREEN ? false : true),
	want_mouse_display_(true),
	want_resize_(true),
#if (FIM_WITH_LIBSDL_VERSION == 1)
	modes_(FIM_NULL)
#else /* FIM_WITH_LIBSDL_VERSION */
	numDisplayModes_(0)
#endif /* FIM_WITH_LIBSDL_VERSION */
	{
		FontServer::fb_text_init1(fontname_,&f_);	// FIXME : move this outta here
#if FIM_WANT_SDL_OPTIONS_STRING 
		parse_optstring(opts_.c_str());
#endif /* FIM_WANT_SDL_OPTIONS_STRING */
#if (FIM_WITH_LIBSDL_VERSION == 1)
		fim_bzero(&bvi_,sizeof(bvi_));
#endif /* FIM_WITH_LIBSDL_VERSION */
		//current_w_=current_h_=0;
	}

	fim_err_t SDLDevice::display(
		//const struct ida_image *img, // source image structure
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
		
		/*
		 * TODO : generalize this routine and put in common.cpp
		 * */
		/*
		 * FIXME : centering mechanisms missing here; an intermediate function
		 * shareable with FramebufferDevice would be nice, if implemented in AADevice.
		 * */
		//was : void
		fim_byte_t* rgb = ida_image_img?((const struct ida_image*)ida_image_img)->data:FIM_NULL;// source rgb array

		if ( !rgb ) return -1;
	
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

		fim_coo_t ytimesw;

		if(SDL_MUSTLOCK(screen_))
		{
			if(SDL_LockSurface(screen_) < 0) return FIM_ERR_GENERIC;
		}

		fim_coo_t idr,idc,lor,loc;

		idr = iroff-oroff;
		idc = icoff-ocoff;

		lor = oroff+(FIM_MIN(orows,irows-iroff));
		loc = ocoff+(FIM_MIN(ocols,icols-icoff));

		fim_coo_t ii,ij;
		fim_coo_t oi,oj;
		fim_flags_t mirror=flags&FIM_FLAG_MIRROR, flip=flags&FIM_FLAG_FLIP;//STILL UNUSED : FIXME
		fim_byte_t * srcp;
		const int buginsdlorsomewhere=(icols > 16*1024 || irows > 16*1024); /* FIXME: occurring with SDL-1.2.15; to ascertain! (actually problems occur with an 21874x940 pixelmap ) */

		// FIXME : temporary
//		clear_rect(  ocoff, ocoff+ocols, oroff, oroff+orows); 
//		clear_rect(  0, ocols, 0, orows); 
		clear_rect(  0, width()-1, 0, height()-1); 


		if(!mirror && !flip && !buginsdlorsomewhere)
		{
#if 0
		for(oi=oroff;FIM_LIKELY(oi<lor);++oi)
		for(oj=ocoff;FIM_LIKELY(oj<loc);++oj)
		{
			ytimesw = (oi)*screen_->pitch/Bpp_;

			ii    = oi + idr;
			ij    = oj + idc;
			srcp  = ((fim_byte_t*)rgb)+(3*(ii*icskip+ij));

			setpixel(screen_, oj, ytimesw, (fim_coo_t)srcp[0], (fim_coo_t)srcp[1], (fim_coo_t)srcp[2]);
		}
#else
			FIM_CONSTEXPR Uint32 rmask=0x00000000,gmask=0x00000000,bmask=0x00000000,amask=0x00000000;
			if(SDL_Surface *src=SDL_CreateRGBSurfaceFrom(rgb,icols,irows,24,icols*3,rmask,gmask,bmask,amask))
			{
				SDL_Rect srcrect;
				SDL_Rect dstrect;
				srcrect.x=icoff;
				srcrect.y=iroff;
				srcrect.w=icols;
				srcrect.h=irows;
				dstrect.x=ocoff;
				dstrect.y=oroff;
				dstrect.w=ocols;
				dstrect.h=orows;
#if FIM_WANT_MOUSE_CROP
				lastsrcrect = srcrect;
				lastdstrect = dstrect;
#endif /* FIM_WANT_MOUSE_CROP */
				SDL_UpperBlit(src,&srcrect,screen_,&dstrect);
				SDL_FreeSurface(src);
				/* FIXME: shall check error codes */
			}
			else
			{
				/* FIXME: need some error processing, here */
				return FIM_ERR_GENERIC;
			}
#endif
		}
		else
		for(oi=oroff;FIM_LIKELY(oi<lor);++oi)
		for(oj=ocoff;FIM_LIKELY(oj<loc);++oj)
		{

			ytimesw = (oi)*screen_->pitch/Bpp_;
			/* UNFINISHED : FIX ME */
			ii    = oi + idr;
			ij    = oj + idc;
			
			if(mirror)ij=((icols-1)-ij);
			if( flip )ii=((irows-1)-ii);
			srcp  = ((fim_byte_t*)rgb)+(3*(ii*icskip+ij));

			setpixel(screen_, oj, ytimesw, (fim_coo_t)srcp[0], (fim_coo_t)srcp[1], (fim_coo_t)srcp[2]);
		}

#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
		draw_help_map(grid_drawer);
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */
		if(SDL_MUSTLOCK(screen_))
			SDL_UnlockSurface(screen_);

#if (FIM_WITH_LIBSDL_VERSION == 1)
		SDL_Flip(screen_);
#else /* FIM_WITH_LIBSDL_VERSION */
		sdl2_redraw();
#endif /* FIM_WITH_LIBSDL_VERSION */
		return FIM_ERR_NO_ERROR;
	}

	bool SDLDevice::sdl_window_update(void)
	{
#if (FIM_WITH_LIBSDL_VERSION == 1)
		vi_ = SDL_GetVideoInfo();
		if(!vi_)
			return false;
		current_w_=vi_->current_w;
		current_h_=vi_->current_h;
		bpp_      =vi_->vfmt->BitsPerPixel;
		Bpp_      =vi_->vfmt->BytesPerPixel;
		// FIXME: shall update want_windowed_ with effective flags contents
#else /* FIM_WITH_LIBSDL_VERSION */
		if(!wi_)
			return false;
		SDL_GetWindowSize(wi_, &current_w_, &current_h_);
		if(!screen_ || !screen_->format)
			return false;
		bpp_      =screen_->format->BitsPerPixel;
		Bpp_      =screen_->format->BytesPerPixel;
#endif /* FIM_WITH_LIBSDL_VERSION */
		return true;
	}

	fim_err_t SDLDevice::initialize(sym_keys_t &sym_keys)
	{
		fim_coo_t want_width=current_w_, want_height=current_h_/*, want_bpp=0*/;

		setenv("SDL_VIDEO_CENTERED","1",0); 
		
		if (SDL_Init(SDL_INIT_VIDEO) < 0 )
		{
			sdld_cerr << "problems initializing SDL (SDL_Init)\n";
			goto sdlerr;
		}

#if (FIM_WITH_LIBSDL_VERSION == 1)
		if( const SDL_VideoInfo * bvip = SDL_GetVideoInfo() )
		{
			bvi_=*bvip;
			get_modes_list();
		}
#else /* FIM_WITH_LIBSDL_VERSION */
		get_modes_list();
		want_width = want_width ? want_width : FIM_DEFAULT_WINDOW_WIDTH;
		want_height = want_height ? want_height : FIM_DEFAULT_WINDOW_HEIGHT;
		wi_ = SDL_CreateWindow(FIM_CNS_FIM_APPTITLE, SDL_WINDOWPOS_CENTERED /*SDL_WINDOWPOS_UNDEFINED*/, SDL_WINDOWPOS_CENTERED, want_width, want_height, want_resize_?SDL_WINDOW_RESIZABLE:0);
		re_ = SDL_CreateRenderer(wi_, -1, 0);
		SDL_RenderClear(re_);
		SDL_RenderPresent(re_);
#endif /* FIM_WITH_LIBSDL_VERSIO  */
		fim_perror(FIM_NULL);
		
#if (FIM_WITH_LIBSDL_VERSION == 1)
		if(FIM_SDL_WANT_KEYREPEAT)
		{
			fim_sdl_int delay = SDL_DEFAULT_REPEAT_DELAY, interval = SDL_DEFAULT_REPEAT_INTERVAL;
			if( SDL_EnableKeyRepeat(delay,interval) >= 0 )
				SDL_GetKeyRepeat(&delay,&interval);
			fim_perror(FIM_NULL);
		}
#endif /* FIM_WITH_LIBSDL_VERSION */

		if ( want_windowed_ )
		{
#if FIM_SDL_WANT_PERCENTAGE
			const bool pcnt = opts_.find('%') != opts_.npos ? true : false;
#else
			const bool pcnt = false;
#endif
		       	if ( ( want_width == 0 && want_height == 0 ) || pcnt )
				get_resolution( ( pcnt ? '%' : 'a'), want_width, want_height);
		}

		if(resize(want_width,want_height))
		{
			sdld_cerr << "problems initializing SDL (SDL_SetVideoMode)\n";
			finalize();
			goto err;
		}
		fim_perror(FIM_NULL);

#if (FIM_WITH_LIBSDL_VERSION == 1)
		/* Enable Unicode translation ( for a more flexible input handling ) */
	        SDL_EnableUNICODE( 1 );
#endif /* FIM_WITH_LIBSDL_VERSION */
		reset_wm_caption();
		fim_perror(FIM_NULL);

		dktorlkm_[SDLK_PAGEUP]=FIM_KKE_PAGE_UP;
		dktorlkm_[SDLK_PAGEDOWN]=FIM_KKE_PAGE_DOWN;
		dktorlkm_[SDLK_LEFT]=FIM_KKE_LEFT;
		dktorlkm_[SDLK_RIGHT]=FIM_KKE_RIGHT;
		dktorlkm_[SDLK_UP]=FIM_KKE_UP;
		dktorlkm_[SDLK_DOWN]=FIM_KKE_DOWN;
		dktorlkm_[SDLK_SPACE]=FIM_KKE_SPACE;
		dktorlkm_[SDLK_END]=FIM_KKE_END;
		dktorlkm_[SDLK_HOME]=FIM_KKE_HOME;
		dktorlkm_[SDLK_BACKSPACE]=FIM_KKE_BACKSPACE;
		dktorlkm_[SDLK_TAB]=FIM_KKE_TAB;
		dktorlkm_[SDLK_RETURN]=FIM_KKE_ENTER;
		//dktorlkm_[SDLK_PAUSE]=FIM_KKE_PAUSE;
		dktorlkm_[SDLK_INSERT]=FIM_KKE_INSERT;
		dktorlkm_[SDLK_DELETE]=FIM_KKE_DEL;
		//dktorlkm_[SDLK_MENU]=FIM_KKE_MENU;
		dktorlkm_[SDLK_F1 ]=FIM_KKE_F1;
		dktorlkm_[SDLK_F2 ]=FIM_KKE_F2;
		dktorlkm_[SDLK_F3 ]=FIM_KKE_F3;
		dktorlkm_[SDLK_F4 ]=FIM_KKE_F4;
		dktorlkm_[SDLK_F5 ]=FIM_KKE_F5;
		dktorlkm_[SDLK_F6 ]=FIM_KKE_F6;
		dktorlkm_[SDLK_F7 ]=FIM_KKE_F7;
		dktorlkm_[SDLK_F8 ]=FIM_KKE_F8;
		dktorlkm_[SDLK_F9 ]=FIM_KKE_F9;
		dktorlkm_[SDLK_F10]=FIM_KKE_F10;
		dktorlkm_[SDLK_F11]=FIM_KKE_F11;
		dktorlkm_[SDLK_F12]=FIM_KKE_F12;
		fim_perror(FIM_NULL);

		post_wmresize();
		return FIM_ERR_NO_ERROR;
sdlerr:
		if( const fim_char_t * const errstr = SDL_GetError() )
			sdld_cerr << "SDL error string: \"" << errstr  << "\"\n";
err:
		return FIM_ERR_GENERIC;
	}

	void SDLDevice::finalize(void)
	{
		if ( ! finalized_ )
		{
#if (FIM_WITH_LIBSDL_VERSION == 2)
			SDL_FreeSurface(screen_);
			SDL_DestroyTexture(texture_);
#endif /* FIM_WITH_LIBSDL_VERSION */
			SDL_Quit();
		}
		finalized_=true;
	}

	fim_coo_t SDLDevice::get_chars_per_column(void)const
	{
		return height() / f_->sheight();
	}

	fim_coo_t SDLDevice::get_chars_per_line(void)const
	{
		return width() / f_->swidth();
	}

	fim_coo_t SDLDevice::width(void)const
	{
		return current_w_;
	}

	fim_coo_t SDLDevice::height(void)const
	{
		return current_h_;
	}

	inline void SDLDevice::setpixel(SDL_Surface *screen, fim_coo_t x, fim_coo_t y, Uint8 r, Uint8 g, Uint8 b) FIM_NOEXCEPT
	{
		/*
		 * this function is taken straight from the sdl documentation: there are smarter ways to do this.
		 * */

		switch (bpp_)
		{
		case  8:
		{
			Uint8 *pixmem8;
			Uint8 colour;
			colour = SDL_MapRGB( screen->format, b, g, r );
			pixmem8 = (Uint8*)((fim_byte_t*)( screen->pixels)  + (y + x)*Bpp_);
			*pixmem8 = colour;
		}
		break;
		case 15:
		case 16:
		{
			Uint16 *pixmem16;
			Uint16 colour;
			colour = SDL_MapRGB( screen->format, b, g, r );
			pixmem16 = (Uint16*)((fim_byte_t*)( screen->pixels)  + (y + x)*Bpp_);
			*pixmem16 = colour;
		}
		break;
		case 24:
		{
			Uint32 *pixmem32;
			Uint32 colour;
			colour = SDL_MapRGB( screen->format, b, g, r );
			pixmem32 = (Uint32*)((fim_byte_t*)( screen->pixels)  + (y + x)*Bpp_);
			*pixmem32 = colour;
		}
		break;
		case 32:
		{
			Uint32 *pixmem32;
			Uint32 colour;
			colour = SDL_MapRGBA( screen->format, b, g, r, 255 );
			pixmem32 = (Uint32*)((fim_byte_t*)( screen->pixels)  + (y + x)*Bpp_);
			*pixmem32 = colour;
		}
		break;
		default:
		{
			/* 1,2,4 modes unsupported for NOW */
		}
		}

	}

#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
static fim_sys_int get_input_inner_mouse_click(fim_key_t * c, SDL_Event*eventp, fim_sys_int *keypressp, bool want_poll)
{
	//case SDL_MOUSEMOTION:
	if( true ) // need  cc.getStringVariable(FIM_VID_WANT_MOUSE_CTRL))  or similar...
	{
		fim_coo_t xm,ym;
		const Uint8 ms = SDL_GetMouseState(&xm,&ym);
#if FIM_WANT_MOUSE_CROP
#if (FIM_WITH_LIBSDL_VERSION == 1)
		const SDLMod mod = SDL_GetModState();
#else /* FIM_WITH_LIBSDL_VERSION */
		const SDL_Keymod mod = SDL_GetModState();
#endif /* FIM_WITH_LIBSDL_VERSION */
		const bool shift_on = ( (mod & KMOD_LSHIFT) || (mod & KMOD_LSHIFT) );

		if( cc.find_keycode_for_bound_cmd(FIM_FLT_CROP) )
		if(shift_on)
		{
			using namespace fim;
			if ( g_last_click_x >= 0 && g_last_click_y >= 0 )
			{
				xm = xm - lastdstrect.x + lastsrcrect.x;
				ym = ym - lastdstrect.y + lastsrcrect.y;
				const fim_coo_t x1 = FIM_MIN(xm, g_last_click_x );
				const fim_coo_t x2 = FIM_MAX(xm, g_last_click_x );
				const fim_coo_t y1 = FIM_MIN(ym, g_last_click_y );
				const fim_coo_t y2 = FIM_MAX(ym, g_last_click_y );

				if (x2>x1)
				if (y2>y1)
				{
					std::ostringstream oss;
					oss << x1 << " " << y1 << " " << x2 << " " << y2;
					cc.setVariable(FIM_VID_CROP_ONCE, oss.str());
					*c=1+('k'-'a');
					g_last_click_x = g_last_click_y = -1;
					return 1;
				}
				g_last_click_x = g_last_click_y = -1;
			}
			g_last_click_x = xm - lastdstrect.x + lastsrcrect.x;
			g_last_click_y = ym - lastdstrect.y + lastsrcrect.y;
		}
#endif /* FIM_WANT_MOUSE_CROP */

#if FIM_WANT_SDL_CLICK_MOUSE_SUPPORT
		const Viewport* cv = cc.current_viewport();
		return grid_drawer.get_input_inner_mouse_click_general(c, xm, ym, cv->viewport_width(), cv->viewport_height(), ((ms&SDL_BUTTON_LMASK)?FIM_MOUSE_LBUTTON_MASK:0) + ((ms&SDL_BUTTON_MMASK)?FIM_MOUSE_MBUTTON_MASK:0) + ((ms&SDL_BUTTON_RMASK)?FIM_MOUSE_RBUTTON_MASK:0) + ((ms&SDL_BUTTON_X1MASK)?FIM_MOUSE_X1_AXIS_MASK:0) + ((ms&SDL_BUTTON_X2MASK)?FIM_MOUSE_X2_AXIS_MASK:0)
#if (FIM_WITH_LIBSDL_VERSION == 1)
			       	+ ((ms&SDL_BUTTON(SDL_BUTTON_WHEELUP))?FIM_MOUSE_WHEELUP_MASK:0) + ((ms&SDL_BUTTON(SDL_BUTTON_WHEELDOWN))?FIM_MOUSE_WHEELDN_MASK:0)
#endif /* FIM_WITH_LIBSDL_VERSION */
				// yet to handle in FIM_WITH_LIBSDL_VERSION == 2
				);
#endif /* FIM_WANT_SDL_CLICK_MOUSE_SUPPORT */
	}
	return 0;
}
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */

#if (FIM_WITH_LIBSDL_VERSION == 2)
fim_sys_int SDLDevice::get_input_inner(fim_key_t * c, SDL_Event*eventp, fim_sys_int *keypressp, bool want_poll) const
{
	fim_sys_int ret=0;
	SDL_Event event=*eventp;
	static int alt_on=false;
	static bool focus_lost_ = false;

#if FIM_WASM_DEFAULTS
	want_poll = 1;
#endif /* FIM_WASM_DEFAULTS */
	*c = FIM_SYM_NULL_KEY;

	if(want_poll)
		ret=SDL_PollEvent(&event);
	else
		ret=SDL_WaitEvent(&event);
	if(ret)
	{
		if(event.type==SDL_KEYUP)
		{
			if ( ( event.key.keysym.sym == SDLK_LALT ) || ( event.key.keysym.sym == SDLK_RALT ))
			{
				FIM_SDL_INPUT_DEBUG(c," SDL_KEYUP: ALT");
				alt_on = false;
			}
			if(!SDL_PollEvent(&event))
			{
				goto done;
			}
			else
			{
				FIM_SDL_INPUT_DEBUG(c," GOT NEXT EVENT");
			}
		}

		switch (event.type)
		{
			case SDL_KEYUP:
			if ( ( event.key.keysym.sym == SDLK_LALT ) || ( event.key.keysym.sym == SDLK_RALT ))
			{
				FIM_SDL_INPUT_DEBUG(c," SDL_KEYUP: ALT");
				alt_on = false;
			}
			break;
			case SDL_DROPFILE:
				cc.push(event.drop.file,FIM_FLAG_PUSH_REC/*|FIM_FLAG_PUSH_BACKGROUND|FIM_FLAG_PUSH_FILE_NO_CHECK*/); // TODO: perhaps add flag to jump there; and make FIM_FLAG_PUSH_REC effective from this context.
				cc.browser_.fcmd_goto(args_t{std::string("?")+event.drop.file}); // mere $ could not cope with jump to existing
				sdl2_redraw();
			break;
			case SDL_DISPLAYEVENT:
				FIM_SDL_INPUT_DEBUG(c," EVENT HERE OF TYPE SDL_DISPLAYEVENT " << event.type );
			break;
			case SDL_SYSWMEVENT:
				FIM_SDL_INPUT_DEBUG(c," EVENT HERE OF TYPE SDL_SYSWMEVENT " << event.type );
			break;
			case SDL_WINDOWEVENT:
			{
				if (event.window.event==SDL_WINDOWEVENT_FOCUS_LOST)
					focus_lost_ = true;
				if (event.window.event==SDL_WINDOWEVENT_TAKE_FOCUS)
					focus_lost_ = false;
				FIM_SDL_INPUT_DEBUG(c," EVENT HERE OF TYPE SDL_WINDOWEVENT " << (int)event.window.event << " : " 
						<< (event.window.event==SDL_WINDOWEVENT_ENTER?" SDL_WINDOWEVENT_ENTER ":"") 
						<< (event.window.event==SDL_WINDOWEVENT_LEAVE?" SDL_WINDOWEVENT_LEAVE ":"") 
						<< (event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED?" SDL_WINDOWEVENT_FOCUS_GAINED ":"") 
						<< (event.window.event==SDL_WINDOWEVENT_FOCUS_LOST?" SDL_WINDOWEVENT_FOCUS_LOST ":"") 
						<< (event.window.event==SDL_WINDOWEVENT_CLOSE?" SDL_WINDOWEVENT_CLOSE ":"") 
						<< (event.window.event==SDL_WINDOWEVENT_TAKE_FOCUS?" SDL_WINDOWEVENT_TAKE_FOCUS ":"") 
					);
#if FIM_SDL_WANT_RESIZE 
				int nw, nh;
				SDL_GetWindowSize(wi_, &nw, &nh);
				if(nw!=current_w_)
				if(nh!=current_h_)
					cc.display_resize(nw,nh);
#endif /* FIM_SDL_WANT_RESIZE */
				sdl2_redraw();
			}
			break;
			case SDL_QUIT:
#if FIM_SDL_ALLOW_QUIT
				*c=cc.find_keycode_for_bound_cmd(FIM_FLT_QUIT);
				if (!*c)
					FIM_DD_HURRY_QUIT();
				return 1;
#endif /* FIM_SDL_ALLOW_QUIT */
				*keypressp = 1; // fixme
			break;
			case SDL_TEXTINPUT:
			if (alt_on)
				break;
			// printable chars of any case and some symbols
			if (event.text.text[0])
			{
				*c = event.text.text[0];
				FIM_SDL_INPUT_DEBUG(c," SDL_TEXTINPUT: " << " text:" << event.text.text << " =c:" << *c);
				return 1;
			}
			break;
			case SDL_KEYDOWN:
			if ( ( event.key.keysym.sym == SDLK_LALT ) || ( event.key.keysym.sym == SDLK_RALT ))
			{
				FIM_SDL_INPUT_DEBUG(c," SDL_KEYDOWN: ALT");
				alt_on = true;
				return 0;
			}

			if (alt_on)
			if (isalpha(event.key.keysym.sym))
			{
				// ALT-key
				*c = FIM_SDL_KEYSYM_TO_RL(event.key.keysym.sym);
				FIM_SDL_INPUT_DEBUG(c," SDL_KEYDOWN: mod: " << event.key.keysym.mod << " sym:" << event.key.keysym.sym << "=" << (char)(event.key.keysym.sym) << " scancode -> c:" << *c << " (+ALT)");
				return 1;
			}

			if(event.key.keysym.mod  & KMOD_RCTRL || event.key.keysym.mod  & KMOD_LCTRL )
			{
				// CTRL-key
				if (event.key.keysym.sym < 0x100) // isalpha & co don't require this
				if (isalpha(event.key.keysym.sym)) // if (event.key.keysym.sym >= 'a') && (event.key.keysym.sym <= 'z')
				if (isalnum(event.key.keysym.sym))
				if (isprint(event.key.keysym.sym))
				{
					*c = FIM_SDL_KEYSYM_TO_RL_CTRL(event.key.keysym.sym);
					FIM_SDL_INPUT_DEBUG(c," SDL_KEYDOWN: mod: " << event.key.keysym.mod << " sym:" << event.key.keysym.sym << "=" << (char)(event.key.keysym.sym) << " scancode:" << event.key.keysym.scancode << "=" << (char)(event.key.keysym.scancode) << " translated -> c:" << *c << " (+CTRL)");
					return 1;
				}
				return 0;
			}

			if ( focus_lost_ ) /* otherwise SDL can intercept e.g. a Tab from the Alt-Tab when reactivating the window */
				return 0;

			if (event.key.keysym.sym > 0x80 || !isprint(event.key.keysym.sym))
			if ( ! (event.key.keysym.mod & KMOD_RSHIFT || event.key.keysym.mod & KMOD_LSHIFT ) ) // shift not accepted
			{
				*c = event.key.keysym.sym;

				const auto km = dktorlkm_.find(*c);
				if (km !=dktorlkm_.end())
					*c = km->second;

				FIM_SDL_INPUT_DEBUG(c," SDL_KEYDOWN: mod: " << event.key.keysym.mod << " sym:" << event.key.keysym.sym << " =c:" << *c << " poll:" << want_poll << " " << fim_key_escape(cc.find_key_sym(event.key.keysym.sym, false)));
				return 1;
			}
			if (event.key.keysym.sym > 0x80 ||  isprint(event.key.keysym.sym))
			{
				// FIM_SDL_INPUT_DEBUG(c," SDL_KEYDOWN: sym:" << event.key.keysym.sym << " (ignored)");
				return 0;
			}
			*c = event.key.keysym.sym;
			FIM_SDL_INPUT_DEBUG(c," SDL_KEYDOWN: sym:" << event.key.keysym.sym << " =c:" << *c);
			return *c ? 1 : 0;
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
			case SDL_MOUSEBUTTONDOWN:
				return get_input_inner_mouse_click(c, eventp, keypressp, want_poll);
			break;
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */
			case SDL_MOUSEWHEEL:
#if FIM_WANT_MOUSE_WHEEL_SCROLL
				if(Viewport* cv = cc.current_viewport())
				{
					cv->mouse_scroll(event.wheel.y);
#if FIM_WANT_MOUSE_PAN
					//TODO: substitute following by centering in mouse_scroll
					cv->mouse_event_pan(event.wheel.x, event.wheel.y, cv->viewport_width(), cv->viewport_height());
#endif /* FIM_WANT_MOUSE_PAN */
				}
#endif /* FIM_WANT_MOUSE_WHEEL_SCROLL */
			break;
#if FIM_WANT_MOUSE_PAN
			case SDL_MOUSEMOTION:
				if(Viewport* cv = cc.current_viewport())
					cv->mouse_event_pan(event.motion.x, event.motion.y, cv->viewport_width(), cv->viewport_height());
			break;
#endif /* FIM_WANT_MOUSE_PAN */
		}
	}
done:
	if (*c)
		FIM_SDL_INPUT_DEBUG(c,"no key");
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
	flip_draw_help_map_tmp(false);
#endif
	return 0;
}
#endif /* FIM_WITH_LIBSDL_VERSION */

#if (FIM_WITH_LIBSDL_VERSION == 1)
	fim_sys_int get_input_inner(fim_key_t * c, SDL_Event*eventp, fim_sys_int *keypressp, bool want_poll)
	{
		bool ctrl_on=0;
		bool alt_on=0;
		//bool shift_on=0;
		fim_sys_int ret=0;
		SDL_Event event=*eventp;

		*c = FIM_SYM_NULL_KEY;

		if(want_poll)
			ret=SDL_PollEvent(&event);
		else
			ret=SDL_WaitEvent(&event);
		if(ret)
		{
			if(event.type==SDL_KEYUP)
				if(!SDL_PollEvent(&event))
					goto done;

			switch (event.type)
			{
#if FIM_SDL_WANT_RESIZE 
				case SDL_VIDEORESIZE:
					cc.display_resize(event.resize.w,event.resize.h);
				break;
#endif /* FIM_SDL_WANT_RESIZE */
				case SDL_QUIT:
#if FIM_SDL_ALLOW_QUIT
				*c=cc.find_keycode_for_bound_cmd(FIM_FLT_QUIT);
				return 1;
				//cc.quit();
#endif /* FIM_SDL_ALLOW_QUIT */
				*keypressp = 1; // fixme
				
				break;
				case SDL_KEYDOWN:
				if(event.key.keysym.mod == KMOD_RCTRL || event.key.keysym.mod == KMOD_LCTRL )
					ctrl_on=true;
				if(event.key.keysym.mod == KMOD_RALT  || event.key.keysym.mod == KMOD_LALT  ) 
					alt_on=true;
				if(event.key.keysym.mod == KMOD_RSHIFT  || event.key.keysym.mod == KMOD_LSHIFT  )
					;//shift_on=true;

				if( event.key.keysym.unicode == 0x0 )
				if(alt_on)
				{
					*c=(fim_byte_t)event.key.keysym.unicode;
					*c|=(1<<31);	/* FIXME : a dirty trick */
					return 1;
				}

				if( 	event.key.keysym.unicode < 0x80)
				{
					/* 
					 * SDL documentation 1.2.12 about unicode:
					 * It is useful to note that unicode values < 0x80 translate directly
					 * a characters ASCII value.
					 * */
			//		if(event.key.keysym.mod == KMOD_RCTRL || event.key.keysym.mod == KMOD_LCTRL ) sdld_cout << "ctrl ! \n";
					*c=(fim_byte_t)event.key.keysym.unicode;

					if(ctrl_on)
					{
						// if(*c-1+'a'=='c')std::exit(-1);//works 
#if 0
						if(*c-1+'a'<='z')
						{
							//sdld_cout << "with control : " << *c+'a'-1 << "\n";
						}
						else
						{
							/* other chars */
							*c-='a';
							*c+= 1 ;
						}
#else
#endif
						FIM_SDL_INPUT_DEBUG(c,"ctrl is on");
					}
					if(*c)	/* !iscntrl(c) */
					{
						/* the usual chars */
						FIM_SDL_INPUT_DEBUG(c,"keysym");
						return 1;
					}
					else	/*  iscntrl(c) */
					{
						FIM_SDL_INPUT_DEBUG(c,"iscntrl");
						return 0;
					}
					/*
					 * p.s.: note that we get 0 in some cases (e.g.: KMOD_RSHIFT, ...).
					 * */
				}
				else
				{
					FIM_SDL_INPUT_DEBUG(c,"wchar");
					cout << "sorry, no support for wide chars in fim\n";
					/*  no support for wide chars in fim */
					return 0;
				}
				FIM_SDL_INPUT_DEBUG(c,"unknown");
				return 0;
				break;
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
				case SDL_MOUSEBUTTONDOWN:
					return get_input_inner_mouse_click(c, eventp, keypressp, want_poll);
				break;
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */
				case SDL_KEYUP:
				return 0;
				break;
				/* Note: No SDL_MOUSEWHEEL in SDL-1.2: SDL_BUTTON_WHEELUP, SDL_BUTTON_WHEELDOWN */
				case SDL_MOUSEMOTION:
#if FIM_WANT_MOUSE_PAN
					if(Viewport* cv = cc.current_viewport())
						cv->mouse_event_pan(event.motion.x, event.motion.y, cv->viewport_width(), cv->viewport_height());
#endif /* FIM_WANT_MOUSE_PAN */
				break;
				default:
				FIM_SDL_INPUT_DEBUG(c,"default-unknown");
			}
			return 0;
		}
done:
		if (*c)
			FIM_SDL_INPUT_DEBUG(c,"no key");
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
		flip_draw_help_map_tmp(false);
#endif
		return 0;
	}
#endif /* FIM_WITH_LIBSDL_VERSION */

	fim_sys_int SDLDevice::get_input(fim_key_t * c, bool want_poll)
	{
		int keypress_ = 0;
		fim_sys_int iv = get_input_inner(c,&event_,&keypress_,want_poll);
		return iv;
	}

	fim_err_t SDLDevice::fill_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2, fim_color_t color) FIM_NOEXCEPT
	{
		fim_coo_t y;
		/*
		 * This could be optimized
		 * */
		for(y=y1;y<=y2;++y)
		{
			fim_memset(((fim_byte_t*)(screen_->pixels)) + y*screen_->pitch + x1*Bpp_,color, (x2-x1+1)* Bpp_);
		}
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t SDLDevice::clear_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2) FIM_NOEXCEPT
	{
		fim_coo_t y;
		/*
		 * This could be optimized
		 * */
		for(y=y1;y<=y2;++y)
		{
			fim_bzero(((fim_byte_t*)(screen_->pixels)) + y*screen_->pitch + x1*Bpp_, (x2-x1+1)* Bpp_);
		}
		return FIM_ERR_NO_ERROR;
	}

void SDLDevice::fs_render_fb(fim_coo_t x_, fim_coo_t y, FSXCharInfo *charInfo, fim_byte_t *data) FIM_NOEXCEPT
{

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
	FIM_CONSTEXPR Uint8 rc = 0xff, gc = 0xff, bc = 0xff;
	// const Uint8 rc = 0x00, gc = 0x00, bc = 0xff;
	const fim_sys_int bpr = GLWIDTHBYTESPADDED((charInfo->right - charInfo->left), SCANLINE_PAD_BYTES);
	const Uint16 pitch = screen_->pitch;
	const Uint16 incr = pitch / Bpp_;

	for (row = 0; row < (charInfo->ascent + charInfo->descent); row++)
	{
		for (x = 0, bit = 0; bit < (charInfo->right - charInfo->left); bit++, x++) 
		{
			if (data[bit>>3] & fs_masktab[bit&7])
			{	// WARNING !
#if FIM_FONT_MAGNIFY_FACTOR == 0
				const fim_int fim_fmf = fim::fim_fmf_; 
#endif	/* FIM_FONT_MAGNIFY_FACTOR */
#if FIM_FONT_MAGNIFY_FACTOR <  0
				fim_int fim_fmf = fim::fim_fmf_; 
#endif	/* FIM_FONT_MAGNIFY_FACTOR */
#if FIM_FONT_MAGNIFY_FACTOR == 1
				setpixel(screen_,x_+x,(y+row)*incr,rc,gc,bc);
#else	/* FIM_FONT_MAGNIFY_FACTOR */
				for(fim_coo_t mi = 0; mi < fim_fmf; ++mi)
				for(fim_coo_t mj = 0; mj < fim_fmf; ++mj)
					setpixel(screen_,x_+fim_fmf*x+mj,(y+fim_fmf*row+mi)*incr,rc,gc,bc);
#endif	/* FIM_FONT_MAGNIFY_FACTOR */
			}
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
}

fim_err_t SDLDevice::fs_puts(struct fs_font *f_, fim_coo_t x, fim_coo_t y, const fim_char_t *str) FIM_NOEXCEPT
{
    fim_sys_int i,c/*,j,w*/;

    for (i = 0; str[i] != '\0'; i++) {
	c = (fim_byte_t)str[i];
	if (FIM_NULL == f_->eindex[c])
	    continue;
	/* clear with bg color */
//	w = (f_->eindex[c]->width+1)*Bpp_;
#if 0
#ifdef FIM_IS_SLOWER_THAN_FBI
	for (j = 0; j < f_->sheight(); j++) {
/////	    memset_combine(start,0x20,w);
	    fim_bzero(start,w);
	    start += fb_fix.line_length;
	}
#else
	//sometimes we can gather multiple calls..
	if(fb_fix.line_length==w)
	{
		//contiguous case
		fim_bzero(start,w*f_->sheight());
	    	start += fb_fix.line_length*f_->sheight();
	}
	else
	for (j = 0; j < f_->sheight(); j++) {
	    fim_bzero(start,w);
	    start += fb_fix.line_length;
	}
#endif
#endif
	/* draw character */
	//fs_render_fb(fb_fix.line_length,f_->eindex[c],f_->gindex[c]);
	fs_render_fb(x,y,f_->eindex[c],f_->gindex[c]);
	x += f_->eindex[c]->swidth();
	/* FIXME : SLOW ! */
	if (((fim_coo_t)x) > width() - f_->swidth())
		goto err;
    }
    // FIXME
	return FIM_ERR_NO_ERROR;
err:
	return FIM_ERR_GENERIC;
}

	fim_err_t SDLDevice::status_line(const fim_char_t *msg)
	{
		fim_err_t errval = FIM_ERR_NO_ERROR;
		fim_coo_t y,ys=3;// FIXME

		if(SDL_MUSTLOCK(screen_) && SDL_LockSurface(screen_) < 0)
		{
			 errval = FIM_ERR_GENERIC;
			 goto done;
		}

		if(get_chars_per_column()<1)
			goto done;
		y = height() - f_->sheight() - ys;
		if(y<0 )
			goto done;
		clear_rect(0, width()-1, y+1,y+f_->sheight()+ys-1);
		fs_puts(f_, 0, y+ys, msg);
		fill_rect(0,width()-1, y, y, FIM_CNS_WHITE);

#if (FIM_WITH_LIBSDL_VERSION == 1)
		if(SDL_MUSTLOCK(screen_)) 
			SDL_UnlockSurface(screen_);
		SDL_Flip(screen_);
#else /* FIM_WITH_LIBSDL_VERSION */
		sdl2_redraw();
#endif /* FIM_WITH_LIBSDL_VERSION */
done:
		return errval;
	}
#if (FIM_WITH_LIBSDL_VERSION == 2)
	void SDLDevice::sdl2_redraw(void)const
	{
		SDL_UpdateTexture(texture_, NULL, screen_->pixels, screen_->pitch);
		SDL_RenderClear(re_);
		SDL_RenderCopy(re_, texture_, NULL, NULL);
		SDL_RenderPresent(re_);
	}
#endif /* FIM_WITH_LIBSDL_VERSION */

	fim_key_t SDLDevice::catchInteractiveCommand(fim_ts_t seconds)const
	{
		/*
		 * Whether there is some input in the input queue.
		 * Note that in this way the event_ will be lost.
		 * */
		fim_key_t c=0;
		SDL_Event levent{};
		fim_sys_int lkeypress=0;
		fim_tms_t sms=10,ms=seconds*1000;// sms: sleep ms

		FIM_SDL_INPUT_DEBUG(&c,"");
#if FIM_WASM_DEFAULTS
		// waiting for love
#else /* FIM_WASM_DEFAULTS */
#if 0
		for(;seconds>0;--seconds)
			sleep(1);
		if(!get_input_inner(&c,&levent,&lkeypress))
		{
			//sdld_cout << "input:" << c << "\n";
			return c;
		}
		else
			return -1;
#else
		do
		{
			if(ms>0)
				usleep((useconds_t)(sms*1000)),ms-=sms;
			// we read input twice: it seems we have a number of "spurious" inputs. 
			if(1==get_input_inner(&c,&levent,&lkeypress,true)) goto done;
		}
		while(ms>0);
		return -1;
done:
		usleep((useconds_t)(sms*1000)),ms-=sms;
#endif
#endif /* FIM_WASM_DEFAULTS */
		return c;
	}

	void SDLDevice::flush(void)
	{
	}

	void SDLDevice::lock(void)
	{
#if (FIM_WITH_LIBSDL_VERSION == 1)
		if(SDL_MUSTLOCK(screen_))
		{
			if(SDL_LockSurface(screen_) < 0) return;
		}
#else /* FIM_WITH_LIBSDL_VERSION */
		// nothing
#endif /* FIM_WITH_LIBSDL_VERSION */
	}

	void SDLDevice::unlock(void)
	{
#if (FIM_WITH_LIBSDL_VERSION == 1)
		if(SDL_MUSTLOCK(screen_)) SDL_UnlockSurface(screen_);
		SDL_Flip(screen_);
#else /* FIM_WITH_LIBSDL_VERSION */
		sdl2_redraw();
#endif /* FIM_WITH_LIBSDL_VERSIO  */
	}


	void SDLDevice::get_modes_list(void)
	{
#if (FIM_WITH_LIBSDL_VERSION == 1)
		if (!modes_)
			if ( bvi_.vfmt )
				modes_ = SDL_ListModes(bvi_.vfmt, SDL_HWSURFACE|SDL_FULLSCREEN);
#else /* FIM_WITH_LIBSDL_VERSION */
		numDisplayModes_ = SDL_GetNumDisplayModes(0);
#endif /* FIM_WITH_LIBSDL_VERSION */
	}
		
	bool SDLDevice::allowed_resolution(fim_coo_t w, fim_coo_t h) const
	{
		if(w==0 || h==0)
			goto ok;
		if(w<FIM_SDL_MINWIDTH || h<FIM_SDL_MINHEIGHT)
			if ( ! want_windowed_ )
				return false;
		if(w<f_->swidth() || h<f_->sheight())
			return false;
ok:
		return true;
	}

	fim_err_t SDLDevice::resize(fim_coo_t w, fim_coo_t h)
	{
		SDL_Surface *nscreen_=FIM_NULL;
		fim_sdl_int want_flags=screen_?screen_->flags:FIM_SDL_FLAGS;
#if FIM_WANT_HARDCODED_ICON
		unsigned char icondata[] =
#include "default_icon_byte_array.h"
		SDL_Surface *icon = FIM_NULL;
#endif /* FIM_WANT_HARDCODED_ICON */

		if(want_resize_)
			want_flags|=SDL_RESIZABLE;
		else
			want_flags&=~SDL_RESIZABLE;

		if(want_windowed_)
			want_flags&=~SDL_FULLSCREEN;
		else
			want_flags|=SDL_FULLSCREEN;

		if(!allowed_resolution(w,h))
			return FIM_ERR_GENERIC;

		SDL_ShowCursor(want_mouse_display_?1:0);

		if(w==0 && h==0)
		{
			int nr = 1, dr = 2;

			if( want_flags & SDL_FULLSCREEN )
				nr = 1, dr = 1;

#if (FIM_WITH_LIBSDL_VERSION == 1)
			w = bvi_.current_w;
		       	h = bvi_.current_h;
#else /* FIM_WITH_LIBSDL_VERSION */
			SDL_GetWindowSize(wi_, &w, &h);
#endif /* FIM_WITH_LIBSDL_VERSION */
			w = FIM_FRAC(w,nr,dr);
		       	h = FIM_FRAC(h,nr,dr);
		}
#if (FIM_WITH_LIBSDL_VERSION == 1)
		w = FIM_MIN(w,bvi_.current_w);
		h = FIM_MIN(h,bvi_.current_h);
		//sdld_cout << "using " << bvi_.current_w << " "<<  bvi_.current_h << " !\n";
#else /* FIM_WITH_LIBSDL_VERSION */
		{
			SDL_DisplayMode displayMode;
			SDL_GetDesktopDisplayMode(0,&displayMode);
			const int w_=displayMode.w;
			const int h_=displayMode.h;
			w = FIM_MIN(w,w_);
			h = FIM_MIN(h,h_);
		}
#endif /* FIM_WITH_LIBSDL_VERSION */

#if FIM_WANT_HARDCODED_ICON
		icon = SDL_LoadBMP_RW(SDL_RWFromMem(icondata, sizeof(icondata)), 1);
#if (FIM_WITH_LIBSDL_VERSION == 1)
		SDL_WM_SetIcon(icon, FIM_NULL);
#else /* FIM_WITH_LIBSDL_VERSION */
		SDL_SetWindowIcon(wi_, icon);
#endif /* FIM_WITH_LIBSDL_VERSION */
		SDL_FreeSurface(icon);
#endif /* FIM_WANT_HARDCODED_ICON */

		//sdld_cout << "resizing to " << w << " "<< h << "\n";
#if (FIM_WITH_LIBSDL_VERSION == 1)
		if (FIM_NULL==(nscreen_ = SDL_SetVideoMode(w, h, bpp_, want_flags)))
		{
			///sdld_cout << "resizing to " << w << " "<< h << " FAILED!\n";
			return FIM_ERR_GENERIC;
		}
#else /* FIM_WITH_LIBSDL_VERSION */
		if ( screen_)
		{
			SDL_FreeSurface(screen_);
			SDL_DestroyTexture(texture_);
			screen_=FIM_NULL;
		}
		if(wi_)
		{
//			if(!want_windowed_) w = h = 0;
			SDL_SetWindowFullscreen(wi_,want_flags);
			if((want_flags & SDL_FULLSCREEN) == SDL_FULLSCREEN)
			{
				sdl_window_update();
				w=current_w_;
				h=current_h_;
			}
			SDL_SetWindowSize(wi_,w,h);
			sdl_window_update();
		}
		if (!screen_)
		{
			if (FIM_NULL==(nscreen_ = SDL_CreateRGBSurface(0, w/*?w:10*/, h/*?h:10*/, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000)))
			{
				return FIM_ERR_GENERIC;
			}
			else
			{
				current_w_ = w, current_h_ = h;
				texture_ = SDL_CreateTexture(re_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
			}
		}
#endif /* FIM_WITH_LIBSDL_VERSION */
		screen_=nscreen_;

		if(want_flags&SDL_FULLSCREEN)
			reset_wm_caption();
		if(!sdl_window_update())
		{
			sdld_cerr << "problems initializing SDL (SDL_GetVideoInfo)\n";
			return FIM_ERR_GENERIC;
		}

		post_wmresize();
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t SDLDevice::reinit(const fim_char_t *rs)
	{
#if FIM_SDL_WANT_RESIZE 
		if ( parse_optstring(rs) == FIM_ERR_NO_ERROR )
		{
			if ( cc.display_resize(current_w_,current_h_) == FIM_ERR_NO_ERROR )
			{
				opts_ = rs;
				return FIM_ERR_NO_ERROR;
			}
		}
#else
		cout << "reinit not allowed\n";
#endif
		return FIM_ERR_GENERIC;
	}

	fim_err_t SDLDevice::set_wm_caption(const fim_char_t *msg)
	{
		fim_err_t rc=FIM_ERR_NO_ERROR;
#if FIM_WANT_CAPTION_CONTROL
		if( msg && want_windowed_ )
#if (FIM_WITH_LIBSDL_VERSION == 1)
			SDL_WM_SetCaption(msg,FIM_SDL_ICONPATH);
#else /* FIM_WITH_LIBSDL_VERSION */
			SDL_SetWindowTitle(wi_,msg);
#endif /* FIM_WITH_LIBSDL_VERSION */
		else
		       	rc=FIM_ERR_UNSUPPORTED;
#else
		rc=FIM_ERR_UNSUPPORTED;
#endif
		return rc;
	}
	
	fim_err_t SDLDevice::reset_wm_caption(void)const
	{
#if (FIM_WITH_LIBSDL_VERSION == 1)
		SDL_WM_SetCaption(FIM_CNS_FIM_APPTITLE,FIM_SDL_ICONPATH);
#else /* FIM_WITH_LIBSDL_VERSION */
		SDL_SetWindowTitle(wi_,FIM_CNS_FIM_APPTITLE);
#endif /* FIM_WITH_LIBSDL_VERSION */
		return FIM_ERR_NO_ERROR;
	}

	fim_coo_t SDLDevice::status_line_height(void)const
	{
		return f_ ? border_height_ + f_->sheight() : 0;
	}

	fim_err_t SDLDevice::post_wmresize(void)
	{
		cc.setVariable(FIM_VID_SCREEN_WIDTH, current_w_);
		cc.setVariable(FIM_VID_SCREEN_HEIGHT,current_h_);
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
		// textual console reformatting
		//mc_.setRows ( get_chars_per_column()/(2*f_->sheight()) );
		mc_.setGlobalVariable(FIM_VID_CONSOLE_ROWS,(height()/(2*f_->sheight())));
		mc_.reformat(    width() /    f_->swidth()   );
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t SDLDevice::get_resolution(const char spec, fim_coo_t & w, fim_coo_t & h) const
	{
		int i;
#if (FIM_WITH_LIBSDL_VERSION == 1)
		if ( !modes_ )
			return FIM_ERR_GENERIC;

		if( modes_ == (SDL_Rect **) -1 )
			return FIM_ERR_GENERIC;
		for(i=0;modes_[i];++i)
#ifdef FIM_SDL_DEBUG
			printf("  %d x %d\n", modes_[i]->w, modes_[i]->h);
#else
			;
#endif
		const int mc = i;
#else /* FIM_WITH_LIBSDL_VERSION */
		if( numDisplayModes_ < 1 )
			return FIM_ERR_GENERIC;
		const int mc = numDisplayModes_;
		i = mc - 1;
#endif /* FIM_WITH_LIBSDL_VERSION */

		if (mc)
		switch (spec)
		{
			case 'L':
				i = 0;
			break;
			case 'l':
				i = std::min ( 1, mc - 1 );
			break;
			case 's':
				i = std::max ( 0, mc - 2 );
			break;
			case 'S':
				i = mc - 1;
			break;
#if (FIM_WITH_LIBSDL_VERSION == 1)
			default:
			case 'a':
				i = -1;
				w = bvi_.current_w;
				h = bvi_.current_h;
				h-= bvi_.current_h / 3;
				w-= bvi_.current_w / 3;
			break;
#if FIM_SDL_WANT_PERCENTAGE
			case '%':
				h = (bvi_.current_h * h) / 100;
				w = (bvi_.current_w * w) / 100;
				i = -1;
			break;
#endif /* FIM_SDL_WANT_PERCENTAGE */
			case 'A':
				w = bvi_.current_w;
				h = bvi_.current_h;
				h-= bvi_.current_h / 10;
				i = -1;
			break;
#else /* FIM_WITH_LIBSDL_VERSION */
			default:
			case 'a':
				i = -1;
				w = current_w_;
				h = current_h_;
				h-= current_h_ / 3;
				w-= current_w_ / 3;
			break;
#if FIM_SDL_WANT_PERCENTAGE
			case '%':
			{
				SDL_DisplayMode displayMode;
				SDL_GetDesktopDisplayMode(0,&displayMode);
				h = (displayMode.h * h) / 100;
				w = (displayMode.w * w) / 100;
				i = -1;
			}
			break;
#endif /* FIM_SDL_WANT_PERCENTAGE */
			case 'A':
				w = current_w_;
				h = current_h_;
				h-= current_h_ / 10;
				i = -1;
			break;
#endif /* FIM_WITH_LIBSDL_VERSION */
		}
#if (FIM_WITH_LIBSDL_VERSION == 1)
		if ( i >= 0 )
			w = modes_[i]->w,
			h = modes_[i]->h;
#else /* FIM_WITH_LIBSDL_VERSION */
		if ( i >= 0 )
		{
			SDL_DisplayMode displayMode;
			const int displayIndex_=i;
			SDL_GetDisplayMode(displayIndex_,numDisplayModes_,&displayMode);
			w = displayMode.w;
			h = displayMode.h;
		}
#endif /* FIM_WITH_LIBSDL_VERSION */
		return FIM_ERR_NO_ERROR;
	}
#endif /* FIM_WITH_LIBSDL */
