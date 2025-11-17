/* $LastChangedDate: 2024-04-07 18:14:45 +0200 (Sun, 07 Apr 2024) $ */
/*
 CACADevice.cpp : libcaca device Fim driver file

 (c) 2008-2024 Michele Martone

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

#ifdef FIM_WITH_LIBCACA

#include "CACADevice.h"
#include <algorithm> // std::find

#define min(x,y) ((x)<(y)?(x):(y))

#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
static struct fim_grid_drawer_t grid_drawer;
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */

	fim_err_t CACADevice::display(
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
		 * TODO : first cleanup, then generalize code from here and elsewhere to obtain reusable centering and copy code (with aa, fbdev).
		 * */
		void* rgb = ida_image_img?((const struct ida_image*)ida_image_img)->data:FIM_NULL;// source rgb array
		const int mirror=flags&FIM_FLAG_MIRROR, flip=flags&FIM_FLAG_FLIP;
		if ( !rgb ) return FIM_ERR_GENERIC;
	
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
	
		orows  = min( orows, height());
		ocols  = min( ocols,  width()); 
		ocskip = width(); 	//notice ocskip is mofidied further below...

		if( orows  > height() ) return -9 -99*100;
		if( ocols  >  width() ) return -10-99*100;
		if( ocskip <  width() ) return -11-99*100;
		if( icskip<icols ) return -6-5*100;

		ocskip = txt_width();// output columns to skip for each line

#if ( FIM_WANTS_CACA_VERSION == 0 )
		caca_set_window_title("caca-fim");
		caca_clear();
		caca_refresh();

		caca_draw_bitmap(0, 0, caca_get_width() - 1, caca_get_height() - 1, caca_bitmap_, bitmap_);
		caca_draw_bitmap(0, 0, caca_get_width() - 1, caca_get_height() - 1, caca_bitmap_, rgb);
		caca_putstr(0,0,"Unfinished display interface!");
		caca_printf(0,txt_height()-1,"%s","sample caca msg");
		caca_refresh();

		ocskip = width();// output columns to skip for each line
		ocskip = width();// output columns to skip for each line
#endif
#if 0
		std::cout << "irows: " << irows << std::endl;
		std::cout << "icols: " << icols << std::endl;
		std::cout << "iroff: " << iroff << std::endl;
		std::cout << "icoff: " << icoff << std::endl;
		std::cout << "orows: " << orows << std::endl;
		std::cout << "ocols: " << ocols << std::endl;
		std::cout << "oroff: " << oroff << std::endl;
		std::cout << "ocoff: " << ocoff << std::endl;
		std::cout << "txt_width: " << txt_width() << std::endl;
		std::cout << "txt_height: " << txt_height() << std::endl;
#endif

#if ( FIM_WANTS_CACA_VERSION == 1 )
		caca_clear_canvas(cv_);
  		caca_set_color_ansi(cv_, CACA_WHITE, CACA_BLACK);
#if 1
		if(icols<width()) { ocoff =(width() -icols+1)/2; ocols = icols-icoff; }
		else { ocoff = 0; ocols= min(width(),icols-icoff); }
		if(irows<height()) { oroff =(height()-irows+1)/2; orows = irows-iroff; }
		else { oroff = 0; orows= min( height(),irows-iroff); }
		struct caca_dither *dither = caca_create_dither(32, width(), height(), 4 * width(), 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
		const char * das = get_dither_mode();
		if (!das)
			das = "ordered4";
		else
			if (isdigit(*das))
			{
				char const * const * dal = caca_get_dither_algorithm_list(NULL); // a static array, NULL-string terminated
				const ssize_t dai = 2*atoi(das); // {algo,desc}
				const auto fni = std::find(dal, dal+dai+2, nullptr) - dal; // first null index
				if (dai < fni)
					das = dal[dai];
			}
		if ( -1 == caca_set_dither_algorithm(dither, das) )
		{
			const auto dda {4};
			cout << "value " << das << " seems not to be a valid dither algorithm specification: reverting to default (" << dda << ")\n";
			caca_set_dither_algorithm(dither, caca_get_dither_algorithm_list(NULL)[dda]);
		}
		char * dst = (char*)fim_calloc(width()*height(),4);
		if (!dst)
			goto err;
		if(!mirror && !flip)
		for (int r=oroff; r < oroff+orows; ++r )
		for (int c=ocoff; c < ocoff+ocols; ++c )
		for (int p=0; p < 3; ++p )
		{
			const int ir = ((r-oroff)+iroff);
			const int ic = ((c-ocoff)+icoff);
			const char sp = ((char*)rgb)[(ir*icskip + ic)*3 + p];
			dst[(r*width() + c)*4 + p] = sp;
		}
		else
		for (int r=oroff; r < oroff+orows; ++r )
		for (int c=ocoff; c < ocoff+ocols; ++c )
		for (int p=0; p < 3; ++p )
		{
			const int ir = flip  ?irows-((r-oroff)+iroff)-1: ((r-oroff)+iroff);
			const int ic = mirror?icols-((c-ocoff)+icoff)-1:((c-ocoff)+icoff);
			const char sp = ((char*)rgb)[(ir*icskip + ic)*3 + p];
			dst[(r*width() + c)*4 + p] = sp;
		}
		caca_dither_bitmap(cv_, 0, 0, txt_width(), txt_height(), dither, dst);
err:
		fim_free(dst);
#else
    		struct caca_dither *dither = caca_create_dither(24, icols, irows, 3 * icols , 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);
		caca_set_dither_algorithm(dither, caca_get_dither_algorithm_list(NULL)[4]);
		caca_dither_bitmap(cv_, ocoff, oroff, txt_width(), txt_height(), dither, rgb );
#endif
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
		draw_help_map(grid_drawer);
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */
		caca_refresh_display(dp_);
		caca_free_dither(dither);
#endif
		return FIM_ERR_NO_ERROR;
	}

	const char * CACADevice::get_dither_mode(void)const
	{
		const char * dms = dopts_.c_str() ? strstr(dopts_.c_str(), "d:") : NULL;
		return dms?dms+2:dms; // notice it points to dopts_
	}

	fim_err_t CACADevice::initialize(sym_keys_t &sym_keys)
	{
		int rc=0;
		if(dopts_.size() && dopts_[0]=='w')
			;
		else
			setenv(FIM_ENV_DISPLAY,"",1);
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
		if ( const auto hc = std::count(dopts_.begin(),dopts_.end(),'H') == 0 ? std::count(dopts_.begin(),dopts_.end(),'h') : -1 )
				grid_drawer.toggle_draw_help_map(hc<0?0:hc);
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */

#if ( FIM_WANTS_CACA_VERSION == 0 )
		rc = caca_init();
		if(rc)return rc;

		XSIZ_ = caca_get_width() * 2;
		YSIZ_ = caca_get_height() * 2 - 4;

		caca_bitmap_ = caca_create_bitmap(8, XSIZ_, YSIZ_ - 2, XSIZ_, 0, 0, 0, 0);
		if( !caca_bitmap_ ) return FIM_ERR_GENERIC;
		caca_set_bitmap_palette(caca_bitmap_, r_, g_, b_, a_);
		bitmap_ = (char*)malloc(4 * caca_get_width() * caca_get_height() * sizeof(char));
		if(!bitmap_) return FIM_ERR_GENERIC;
		fim_bzero(bitmap_, 4 * caca_get_width() * caca_get_height());

		caca_clear();
		caca_set_color(CACA_COLOR_BLACK,CACA_COLOR_WHITE);
		caca_set_color(CACA_COLOR_RED,CACA_COLOR_BLACK);
		caca_putstr(0,0,"What a caca!");
		caca_refresh();
#endif
#if ( FIM_WANTS_CACA_VERSION == 1 )
		cv_ = caca_create_canvas(32, 16);
		if(cv_ == NULL)
		{
			rc=1;
			goto err;
		}
		dp_ = caca_create_display(cv_);
		if(dp_ == NULL)
		{
			rc=1;
			goto err;
		}
		caca_refresh_display(dp_);
err:
#endif

		dktorlkm_[CACA_KEY_PAGEUP]=FIM_KKE_PAGE_UP;
		dktorlkm_[CACA_KEY_PAGEDOWN]=FIM_KKE_PAGE_DOWN;
		dktorlkm_[CACA_KEY_LEFT]=FIM_KKE_LEFT;
		dktorlkm_[CACA_KEY_RIGHT]=FIM_KKE_RIGHT;
		dktorlkm_[CACA_KEY_UP]=FIM_KKE_UP;
		dktorlkm_[CACA_KEY_DOWN]=FIM_KKE_DOWN;
		//dktorlkm_[CACA_KEY_SPACE]=FIM_KKE_SPACE;
		dktorlkm_[CACA_KEY_END]=FIM_KKE_END;
		dktorlkm_[CACA_KEY_INSERT]=FIM_KKE_INSERT;
		dktorlkm_[CACA_KEY_HOME]=FIM_KKE_HOME;
		dktorlkm_[CACA_KEY_BACKSPACE]=FIM_KKE_BACKSPACE;
		dktorlkm_[CACA_KEY_TAB]=FIM_KKE_TAB;
		dktorlkm_[CACA_KEY_RETURN ]=FIM_KKE_ENTER;
		//dktorlkm_[SDLK_PAUSE]=FIM_KKE_PAUSE;
		dktorlkm_[CACA_KEY_INSERT]=FIM_KKE_INSERT;
		dktorlkm_[CACA_KEY_DELETE]=FIM_KKE_DEL;
		//dktorlkm_[CACA_MENU]=FIM_KKE_MENU;
		dktorlkm_[CACA_KEY_RETURN]=FIM_KKE_ENTER;
		dktorlkm_[CACA_KEY_F1 ]=FIM_KKE_F1;
		dktorlkm_[CACA_KEY_F2 ]=FIM_KKE_F2;
		dktorlkm_[CACA_KEY_F3 ]=FIM_KKE_F3;
		dktorlkm_[CACA_KEY_F4 ]=FIM_KKE_F4;
		dktorlkm_[CACA_KEY_F5 ]=FIM_KKE_F5;
		dktorlkm_[CACA_KEY_F6 ]=FIM_KKE_F6;
		dktorlkm_[CACA_KEY_F7 ]=FIM_KKE_F7;
		dktorlkm_[CACA_KEY_F8 ]=FIM_KKE_F8;
		dktorlkm_[CACA_KEY_F9 ]=FIM_KKE_F9;
		dktorlkm_[CACA_KEY_F10]=FIM_KKE_F10;
		dktorlkm_[CACA_KEY_F11]=FIM_KKE_F11;
		dktorlkm_[CACA_KEY_F12]=FIM_KKE_F12;
		fim_perror(FIM_NULL);

		return rc?FIM_ERR_GENERIC:FIM_ERR_NO_ERROR;
	}

	void CACADevice::finalize(void)
	{
#if ( FIM_WANTS_CACA_VERSION == 0 )
		caca_end();
#endif
#if ( FIM_WANTS_CACA_VERSION == 1 )
		caca_free_display(dp_);
		caca_free_canvas(cv_);
#endif
	}

	fim_coo_t CACADevice::get_chars_per_line(void)const{return txt_width();}
#if ( FIM_WANTS_CACA_VERSION == 0 )
	int CACADevice::txt_width(void)const { return width() ;}
	int CACADevice::txt_height(void)const{ return width() ;}
	int CACADevice::width(void)const { return caca_get_height();}
	int CACADevice::height(void)const{ return caca_get_width() ;}
#endif
#if ( FIM_WANTS_CACA_VERSION == 1 )
	int CACADevice::txt_width(void)const { return caca_get_canvas_width(cv_) ;}
	int CACADevice::txt_height(void)const{ return caca_get_canvas_height(cv_) ;}
	int CACADevice::width(void)const { return caca_get_display_width(dp_);}
	int CACADevice::height(void)const{ return caca_get_display_height(dp_) ;}
#endif
	fim_err_t CACADevice::status_line(const fim_char_t *msg)
	{
#if ( FIM_WANTS_CACA_VERSION == 0 )
		caca_printf(0,txt_height()-1,"%s",msg);
		caca_printf(0,0,"foooooooo");
		caca_putstr(0,0,"foooooooo");
#endif
#if ( FIM_WANTS_CACA_VERSION == 1 )
  		caca_set_color_ansi(cv_, CACA_WHITE, CACA_BLACK);
		caca_put_str(cv_, 0, txt_height()-1, msg);
		caca_refresh_display(dp_);
#endif
		return FIM_ERR_NO_ERROR;
	}
	fim_err_t CACADevice::fs_puts(struct fs_font *f, fim_coo_t x, fim_coo_t y, const fim_char_t *str)
	{
#if ( FIM_WANTS_CACA_VERSION == 0 )
		return FIM_ERR_NO_ERROR;
#endif
#if ( FIM_WANTS_CACA_VERSION == 1 )
		caca_set_color_ansi(cv_, CACA_WHITE, CACA_BLACK);
		const int tw = txt_width();
		const int nl = FIM_INT_FRAC(strlen(str),tw);
		for (int l = 0; l < nl; ++l )
			caca_put_str(cv_, x, y + l, str + l * tw); // let libcaca cut off at tw
		caca_refresh_display(dp_);
		return FIM_ERR_NO_ERROR;
#endif
	}
	fim_err_t CACADevice::clear_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2)
	{
		/* FIXME : only if initialized !*/
		return FIM_ERR_GENERIC;
	}
	fim_bool_t CACADevice::handle_console_switch(void){return true;}
	fim_err_t CACADevice::console_control(fim_cc_t code){return FIM_ERR_GENERIC;}
/*
 * This is embryo code and should be used for experimental purposes only!
 */
	fim_coo_t CACADevice::get_chars_per_column(void)const{return txt_height();}
	fim_coo_t CACADevice::font_height(void)const
	{
		return FIM_LIBCACA_FONT_HEIGHT;
	}

	fim_coo_t CACADevice::status_line_height(void)const
	{
		return FIM_LIBCACA_FONT_HEIGHT;
	}

	fim_sys_int CACADevice::get_input(fim_key_t * c, bool want_poll)
	{
#if ( FIM_WANTS_CACA_VERSION == 0 )
		fim_sys_int rc = 0;
		const int ce = caca_get_event(CACA_EVENT_ANY);
		if (ce == CACA_EVENT_RESIZE )
		{
			rc = 1;
			std::cout << "resize !" << *c <<  "\n";
		}
		if (ce & CACA_EVENT_QUIT )
		{
			rc = 1;
			std::cout << "quit !" << "\n";
		}
		if (ce & CACA_EVENT_KEY_PRESS)
		{
			rc = 1;
			*c = (ce & 0xffff); // CACA_EVENT_ANY in caca.h, not in caca0.h
			//*c = (ce & CACA_EVENT_ANY);
			std::cout << "pressed: " << *c <<  " !\n";
		}
#endif
#if ( FIM_WANTS_CACA_VERSION == 1 )
		fim_sys_int rc = 0;
		caca_event ev;
		int ce = caca_get_event(dp_, CACA_EVENT_ANY /*CACA_EVENT_KEY_PRESS*/, &ev, 1000); // microseconds timeout or -1 for blocking
		const enum caca_event_type et = caca_get_event_type(&ev);
		ce = ev.data.key.ch;
		if ( et == CACA_EVENT_MOUSE_MOTION )
		{
#if FIM_WANT_MOUSE_PAN
			if(Viewport* cv = cc.current_viewport())
				cv->mouse_event_pan(caca_get_event_mouse_x(&ev), caca_get_event_mouse_y(&ev), get_chars_per_line(), get_chars_per_column()),
				rc = 1;
#endif /* FIM_WANT_MOUSE_PAN */
		}
		else
		if ( et == CACA_EVENT_MOUSE_PRESS )
		{
			rc = 0;
#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
			const int xm = caca_get_mouse_x(dp_);
			const int ym = caca_get_mouse_y(dp_);
			int mbp{};

			if (caca_get_event_mouse_button(&ev)==1)
				mbp |= FIM_MOUSE_LBUTTON_MASK;
			if (caca_get_event_mouse_button(&ev)==2)
				mbp |= FIM_MOUSE_MBUTTON_MASK;
			if (caca_get_event_mouse_button(&ev)==3)
				mbp |= FIM_MOUSE_RBUTTON_MASK;
			if( mbp && !cc.inConsole())
				grid_drawer.get_input_inner_mouse_click_general(c, xm, ym, get_chars_per_line(), get_chars_per_column(), mbp),
				rc = 1;
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */
		}
		else
		if ( et == CACA_EVENT_RESIZE )
		{
			rc = 0;
			cc.display_resize(0,0);
		}
		else
		if ( et == CACA_EVENT_QUIT )
		{
			rc = 1;
			*c=cc.find_keycode_for_bound_cmd(FIM_FLT_QUIT);
		}
		else
		if ( et == CACA_EVENT_KEY_PRESS)
		{
			fim_key_t k = 0;
			rc = 1;
			*c = (ce & CACA_EVENT_ANY);

        		k = caca_get_event_key_ch(&ev);

			if (dktorlkm_.find(k)!=dktorlkm_.end())
				k = dktorlkm_[k];

			if (k)
				*c = k;
			else
				*c = (ce & CACA_EVENT_ANY);
			// std::cout << "pressed: " << (char)*c <<  " !\n";
		}
#endif
		return rc;
	}

	fim_err_t CACADevice::resize(fim_coo_t w, fim_coo_t h)
	{
#if ( FIM_WANTS_CACA_VERSION == 1 )
#endif
		return FIM_ERR_NO_ERROR;
	}
	fim_err_t CACADevice::set_wm_caption(const fim_char_t *msg)
	{
#if ( FIM_WANTS_CACA_VERSION == 1 )
		caca_set_display_title(dp_,msg);
#endif
		return FIM_ERR_NO_ERROR;
	}
	fim_err_t CACADevice::reinit(const fim_char_t *dopts)
	{
		dopts_ = dopts;
		return FIM_ERR_NO_ERROR;
	}
#endif /* FIM_WITH_LIBCACA */
