/* $LastChangedDate: 2024-04-02 19:24:32 +0200 (Tue, 02 Apr 2024) $ */
/*
 DisplayDevice.cpp : virtual device Fim driver file

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
#include "DisplayDevice.h"

#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	DisplayDevice::DisplayDevice(MiniConsole& mc):
	finalized_(false)
	,mc_(mc)
	,f_(FIM_NULL)
	,fontname_(FIM_NULL)
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	DisplayDevice::DisplayDevice(void):
	finalized_(false)
	,f_(FIM_NULL)
	,fontname_(FIM_NULL)
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	,redraw_(FIM_REDRAW_UNNECESSARY)
	{
		const fim_char_t *line;

	    	if (FIM_NULL != (line = fim_getenv(FIM_ENV_FBFONT)))
			fontname_ = line;
	}

	fim_sys_int DisplayDevice::get_input(fim_key_t * c, bool want_poll)
	{
		fim_sys_int r=0;
#if HAVE_SYS_SELECT_H
		*c=0;
#ifdef  FIM_SWITCH_FIXUP
			/* This way the console switches the right way. */
			/* (The following code dates back to the original fbi.c)
			 */

			/*
			 * patch: the following read blocks the program even when switching console
			 */
			//r=read(fim_stdin_,&c,1); if(c==0x1b){read(0,&c,3);c=(0x1b)+(c<<8);}
			/*
			 * so the next coded shoul circumvent this behaviour!
			 */
			{
				fd_set set;
				fim_sys_int fdmax;
				struct timeval  limit;
				fim_sys_int timeout=1,rc,paused=0;
	
			        FD_ZERO(&set);
			        FD_SET(cc.fim_stdin_, &set);
			        fdmax = 1;
#ifdef FBI_HAVE_LIBLIRC
				/*
				 * expansion code :)
				 */
			        if (-1 != lirc)
				{
			            FD_SET(lirc,&set);
			            fdmax = lirc+1;
			        }
#endif /* FBI_HAVE_LIBLIRC */
			        limit.tv_sec = timeout;
			        limit.tv_usec = 0;
			        rc = select(fdmax, &set, FIM_NULL, FIM_NULL,
			                    (0 != timeout && !paused) ? &limit : FIM_NULL);
				if(handle_console_switch())
				{
					r=-1;	/* originally a 'continue' in a loop */
					goto ret;
				}
				
				if (FD_ISSET(cc.fim_stdin_,&set))
					rc = read(cc.fim_stdin_, c, 4);
				r=rc;
				*c=int2msbf(*c);
			}
#else  /* FIM_SWITCH_FIXUP */
			/* This way the console switches the wrong way. */
			r=read(fim_stdin_,&c,4);	//up to four chars should suffice
#endif  /* FIM_SWITCH_FIXUP */
ret:
#endif /* HAVE_SYS_SELECT_H */
		return r;
	}

	fim_key_t DisplayDevice::catchInteractiveCommand(fim_ts_t seconds)const
	{
		fim_key_t c=-1;/* -1 means 'no character pressed */
#if HAVE_TERMIOS_H
		/*	
		 * This call 'steals' circa 1/10 of second..
		 */
		fd_set          set;
		ssize_t rc=0,r;
		struct termios tattr, sattr;
		//we set the terminal in raw mode.
                if (! isatty(cc.fim_stdin_))
		{
                        sleep(seconds);
			goto ret;
		}

		FD_SET(0, &set);
		//fcntl(0,F_GETFL,&saved_fl);
		tcgetattr (0, &sattr);
		//fcntl(0,F_SETFL,O_BLOCK);
		memcpy(&tattr,&sattr,sizeof(struct termios));
		tattr.c_lflag &= ~(ICANON|ECHO);
		tattr.c_cc[VMIN]  = 0;
		tattr.c_cc[VTIME] = 1 * (seconds==0?1:fmodf(seconds*10,256));
		tcsetattr (0, TCSAFLUSH, &tattr);
		//r=read(fim_stdin_,&c,4);
		// FIXME : read(.,.,3) is NOT portable. DANGER
		unsigned char uc;
		r = read(cc.fim_stdin_,&uc,1);
		c = uc;
		if(r>0&&c==0x1b){rc=read(0,&c,3);if(rc)c=(0x1b)+(c<<8);/* we should do something with rc now */}
		//we restore the previous console attributes
		tcsetattr (0, TCSAFLUSH, &sattr);
		if( r<=0 )
			c=-1;	
ret:		
#endif /* HAVE_TERMIOS_H */
		return c; /* read key */
	}

void DisplayDevice::clear_screen_locking(void)
{
	this->lock();
	fim_int ls=cc.getIntVariable(FIM_VID_CONSOLE_ROWS);
	fim_coo_t fh=f_?f_->sheight():1;
	ls=FIM_MIN(ls,height()/fh);
	clear_rect(0, width()-1, 0,fh*ls);
	this->unlock();
}

#ifndef FIM_KEEP_BROKEN_CONSOLE
void DisplayDevice::fb_status_screen_new(const fim_char_t *msg, fim_bool_t draw, fim_flags_t flags)
{
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	fim_err_t r=FIM_ERR_NO_ERROR;
	
	if(flags==0x03)
	{
		/* clear screen sequence */
		mc_.clear();
		goto ret;
	}

	if( flags==0x01 ) { mc_.scroll_down(); goto ret; }
	if( flags==0x02 ) { mc_.scroll_up(); goto ret; }

	r=mc_.add(msg);
	if(r==FIM_ERR_BUFFER_FULL)
	{
		r=mc_.grow();
		if(r==FIM_ERR_GENERIC)
			goto ret;
		r=mc_.add(msg);
		if(r==FIM_ERR_GENERIC)
			goto ret;
	}

	if(!draw )
		goto ret;
	clear_screen_locking();
	mc_.dump();
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
ret:
	return;
}
#endif /* FIM_KEEP_BROKEN_CONSOLE */

fim_err_t DisplayDevice::console_control(fim_cc_t arg)
{
	if(arg==0x01)
		fb_status_screen_new(FIM_NULL,false,arg);
	if(arg==0x02)
		fb_status_screen_new(FIM_NULL,false,arg);
	if(arg==0x03)
		fb_status_screen_new(FIM_NULL,false,arg);
	return FIM_ERR_NO_ERROR;
}

fim_err_t DisplayDevice::format_console(void)
{
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	if(f_)
	{	
		mc_.setRows ((height()/f_->sheight())/2);
		mc_.reformat( width() /f_->swidth()    );
	}
	else
	{
		mc_.setRows ( height()/2 );
		mc_.reformat( get_chars_per_line()    );
	}
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	return FIM_ERR_NO_ERROR;
}

DisplayDevice::~DisplayDevice(void)
{
	/* added in fim : fbi did not have this */
	fim_free_fs_font(f_);
}

#if FIM_WANT_BENCHMARKS
fim_int DisplayDevice::get_n_qbenchmarks(void)const
{
	return 1;
}

string DisplayDevice::get_bresults_string(fim_int qbi, fim_int qbtimes, fim_fms_t qbttime)const
{
	std::ostringstream oss;
	string msg;

	switch(qbi)
	{
		case 0:
			oss << "fim display check" << " : " << (float)(((fim_fms_t)(qbtimes*2))/((qbttime)*1.e-3)) << " clears/s\n";
			msg=oss.str();
	}
	return msg;
}

void DisplayDevice::quickbench_init(fim_int qbi)
{
	switch(qbi)
	{
		case 0:
		std::cout << "fim display check" << " : " << "please be patient\n";
		break;
	}
}

void DisplayDevice::quickbench_finalize(fim_int qbi)
{
}

void DisplayDevice::quickbench(fim_int qbi)
{
	/*
		a quick draw benchmark and sanity check.
		currently performs only the clear function.
	*/
	const fim_coo_t x1 = 0, x2 = width()-1, y1 = 0, y2 = height()-1;
	switch(qbi)
	{
		case 0:
			for (int i=0;i<2;++i)
			{
				this->lock();
				// clear_rect(x1, x2, y1, y2);
				if(i%2)
					fill_rect(x1, x2, y1, y2, FIM_CNS_WHITE);
				else
					fill_rect(x1, x2, y1, y2, FIM_CNS_BLACK);
				this->unlock();
			}
		break;
	}
}
#endif /* FIM_WANT_BENCHMARKS */

	fim_err_t DisplayDevice::resize(fim_coo_t w, fim_coo_t h){return FIM_ERR_NO_ERROR;}
	fim_err_t DisplayDevice::reinit(const fim_char_t *rs){return FIM_ERR_NO_ERROR;}
	fim_err_t DisplayDevice::set_wm_caption(const fim_char_t *msg){return FIM_ERR_UNSUPPORTED;}
	fim_err_t DisplayDevice::menu_ctl(char action, const fim_char_t *menuspec){return FIM_ERR_UNSUPPORTED;}

	fim_coo_t DisplayDevice::suggested_font_magnification(const fim_coo_t wf, const fim_coo_t hf)const
	{
		// suggests a font magnification until font is wider than 1/wf and taller than 1/hf wrt screen.
		// if a value is non positive, it will be ignored.
		fim_coo_t fmf = 1;

		if(wf>0 && hf<0)
			while ( wf * fmf * this->f_->width  < this->width() )
				++fmf;
		if(wf<0 && hf>0)
			while ( hf * fmf * this->f_->height < this->height() )
				++fmf;
		if(wf>0 && hf>0)
			while ( wf * fmf * this->f_->width  < this->width() &&
				hf * fmf * this->f_->height < this->height() )
				++fmf;
		return fmf;
	}

	fim_err_t DisplayDevice::fs_putc(struct fs_font *f, fim_coo_t x, fim_coo_t y, const fim_char_t c)
	{
		fim_char_t s[2] = {c,FIM_SYM_CHAR_NUL};
		return fs_puts(f, x, y, s);
	}

	void DisplayDevice::fs_multiline_puts(const char *str, fim_int doclear, int vw, int wh)
	{
		const int fh=this->f_ ? this->f_->sheight():1; // FIXME : this is not clean
		const int fw=this->f_ ? this->f_->swidth():1; // FIXME : this is not clean
		const int sl = strlen(str), rw = vw / fw;
		const int cpl = this->get_chars_per_line();

		if(doclear && cpl)
		{
			const int lc = FIM_INT_FRAC(sl,cpl); /* lines count */
			this->clear_rect(0, vw-1, 0, FIM_MIN(fh*lc,wh-1));
		}

		for( int li = 0 ; sl > rw * li ; ++li )
			if((li+1)*fh<wh) /* FIXME: maybe this check shall better reside in fs_puts() ? */
				this->fs_puts(this->f_, 0, fh*li, str+rw*li);
	}

	fim_err_t DisplayDevice::fill_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2, fim_color_t icolor, fim_color_t ocolor) FIM_NOEXCEPT
	{
		/*
		 * This could be optimized.
		 * e.g. four lines around inner box would be better.
		 * */
		fim_err_t rv = FIM_ERR_NO_ERROR;
		rv = fill_rect(x1, x2, y1, y2, icolor);
		if(x2-x1<3 || y2-y1<3)
			rv = fill_rect(x1+1, x2-1, y1+1, y2-1, ocolor);
		return rv;
	}

	void DisplayDevice::console_switch(fim_bool_t is_busy){}

/* ---- FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT stuff BEGIN -- */

#if FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT
	fim_err_t DisplayDevice::draw_help_map(fim_grid_drawer_t & gd)
	{
		if(!(gd.fim_draw_help_map_ || gd.fim_draw_help_map_tmp_))
			return FIM_ERR_NO_ERROR;

		if(gd.fim_draw_help_map_==0)
		{
			// nothing to draw
		}

		const bool ipod = f_ != FIM_NULL; // is pixel-oriented device
		const fim_coo_t xw = ipod ? width() : get_chars_per_line();
		const fim_coo_t yh = ipod ? height() : get_chars_per_column();

		if(gd.fim_draw_help_map_==2)
		{
			const fim_coo_t xt = (xw+2)/3;
			const fim_coo_t yt = (yh+2)/3;
			const fim_coo_t xtl = (xt+15)/16;
			const fim_coo_t ytl = (yt+15)/16;
			const fim_coo_t eth = 1; // extra thickness
			const fim_color_t ic = FIM_CNS_WHITE;
			const fim_color_t oc = FIM_CNS_BLACK;
			// test
			//fill_rect(xw/2, xw/2, 0, yh-1, ic, oc); // test middle vertical 
			//fill_rect(0, xw-1, yh/2, yh/2, ic, oc); // test middle horizontal
			// horizontal
			fill_rect(0, xtl, 1*yt-eth, 1*yt+eth, ic, oc); // left top
			fill_rect(0, xtl, 2*yt-eth, 2*yt+eth, ic, oc); // left bottom
			fill_rect(xw-xtl, xw-1, 1*yt-eth, 1*yt+eth, ic, oc); // right top
			fill_rect(xw-xtl, xw-1, 2*yt-eth, 2*yt+eth, ic, oc); // right bottom
			// vertical
			fill_rect(1*xt-eth, 1*xt+eth, 0*ytl, 1*ytl, ic, oc); // left top
			fill_rect(2*xt-eth, 2*xt+eth, 0*ytl, 1*ytl, ic, oc); // right top
			fill_rect(1*xt-eth, 1*xt+eth, yh-ytl, yh-1, ic, oc); // left bottom
			fill_rect(2*xt-eth, 2*xt+eth, yh-ytl, yh-1, ic, oc); // right bottom

#if FIM_WANT_POSITION_DISPLAYED
			const fim_coo_t yp = gy;
			const fim_coo_t xp = gx;

			if(yp>=eth && yp<yh-eth)
				fill_rect(0, xw, yp-eth, yp+eth, ic, oc); // h

			if(xp>=eth && xp<xw-eth)
				fill_rect(  xp-eth,   xp+eth, 0, yh-1, ic, oc); // v
#endif /* FIM_WANT_POSITION_DISPLAYED */
		}

		if(const fim::string wmc = cc.getStringVariable(FIM_VID_WANT_MOUSE_CTRL))
		if( wmc.size()>=9 && strncpy(gd.key_char_grid_,wmc.c_str(),9) )
		if(gd.fim_draw_help_map_==1 || gd.fim_draw_help_map_tmp_)
		{
			const fim_coo_t xt = (xw+5)/6;
			const fim_coo_t yt = (yh+5)/6;
			const fim_coo_t eth = 1;
			const fim_coo_t fd = ipod ? (FIM_MAX(f_->swidth(),f_->sheight())) : 1;

			if(xw > 6*fd && yh > 6*fd)
			{
				const fim_color_t ic = FIM_CNS_WHITE;
				const fim_color_t oc = FIM_CNS_BLACK;

				fill_rect(0*2*xt, xw, 1*2*yt-eth, 1*2*yt+eth, ic, oc);
				fill_rect(0*2*xt, xw, 2*2*yt-eth, 2*2*yt+eth, ic, oc);
				fill_rect(1*2*xt-eth, 1*2*xt+eth, 0, yh-1, ic, oc);
				fill_rect(2*2*xt-eth, 2*2*xt+eth, 0, yh-1, ic, oc);
				// left column
				fs_putc(f_, 1*xt, 1*yt, gd.key_char_grid_[0]);
				fs_putc(f_, 1*xt, 3*yt, gd.key_char_grid_[1]);
				fs_putc(f_, 1*xt, 5*yt, gd.key_char_grid_[2]);
				// middle column
				fs_putc(f_, 3*xt, 1*yt, gd.key_char_grid_[3]);
				fs_putc(f_, 3*xt, 3*yt, gd.key_char_grid_[4]);
				fs_putc(f_, 3*xt, 5*yt, gd.key_char_grid_[5]);
				// right column
				fs_putc(f_, 5*xt, 1*yt, gd.key_char_grid_[6]);
				fs_putc(f_, 5*xt, 3*yt, gd.key_char_grid_[7]);
				fs_putc(f_, 5*xt, 5*yt, gd.key_char_grid_[8]);
			}
		}
		return FIM_ERR_NO_ERROR;
	}

void fim_grid_drawer_t::toggle_draw_help_map(const int state)
{
       	const auto fim_draw_help_map = fim_draw_help_map_;
	if (state>=0)
       		fim_draw_help_map_=state%3;
	else
       		fim_draw_help_map_=(fim_draw_help_map_+1)%3;
       	if ( fim_draw_help_map != fim_draw_help_map_ )
		if(Viewport* cv = cc.current_viewport())
			cv->redisplay(); // will indirectly trigger draw_help_map()
}

fim_sys_int fim_grid_drawer_t::get_input_inner_mouse_click_general(fim_key_t * c, const fim_coo_t xm, const fim_coo_t ym, const fim_coo_t xv, const fim_coo_t yv, const int mbp)
{
	const fim_coo_t xt = (xv+2)/3;
	const fim_coo_t yt = (yv+2)/3;
	const bool pmb1 = (FIM_MOUSE_LBUTTON_MASK & mbp);
	const bool pmb2 = (FIM_MOUSE_MBUTTON_MASK & mbp);
	const bool pmb3 = (FIM_MOUSE_RBUTTON_MASK & mbp);
	const bool pmx1 = (FIM_MOUSE_X1_AXIS_MASK & mbp);
	const bool pmx2 = (FIM_MOUSE_X2_AXIS_MASK & mbp);
	const bool pmwu = (FIM_MOUSE_WHEELUP_MASK & mbp);
	const bool pmwd = (FIM_MOUSE_WHEELDN_MASK & mbp);

	if(!cc.inConsole() && pmb1)
	{
		if( xm < xt )
		{
			if( ym < yt )
			{
				*c=key_char_grid_[0]; return 1;
			}
			else
			if( ym < 2*yt )
			{
				*c=key_char_grid_[1]; return 1;
			}
			else
			{
				*c=key_char_grid_[2]; return 1;
			}
		}
		else
		if( xm < 2*xt )
		{
			if( ym < yt )
			{
				*c=key_char_grid_[3]; return 1;
			}
			else
			if( ym < 2*yt )
			{
				*c=key_char_grid_[4]; return 1;
			}
			else
			{
				*c=key_char_grid_[5]; return 1;
			}
		}
		else
		{
			if( ym < yt )
			{
				*c=key_char_grid_[6]; return 1;
			}
			else
			if( ym < 2*yt )
			{
				*c=key_char_grid_[7]; return 1;
			}
			else
			{
				*c=key_char_grid_[8]; return 1;
			}
		}
	}
		//cout << "mouse clicked at "<<x<<" "<<y<<" : "<< ((x>cv->viewport_width()/2)?'r':'l') <<"; state: "<<ms<<"\n";
#if 0
		if(ms&SDL_BUTTON_RMASK) cout << "rmask\n";
		if(ms&SDL_BUTTON_LMASK) cout << "lmask\n";
		if(ms&SDL_BUTTON_MMASK) cout << "mmask\n";
		if(ms&SDL_BUTTON_X1MASK) cout << "x1mask\n";
		if(ms&SDL_BUTTON_X2MASK) cout << "x2mask\n";
#endif /* 1 */
		if(!cc.inConsole())
		{
			if(pmb1) { *c=FIM_DSK_NEXT; return 1; }
			if(pmb3) { toggle_draw_help_map(); return 0; }
			if(pmb2) { toggle_draw_help_map(); return 0; }
			//if(ms&SDL_BUTTON_RMASK) { *c='b'; return 1; }
			//if(ms&SDL_BUTTON_MMASK) { *c='q'; return 1; }
			if(pmx1) { *c=FIM_DSK_MAGNIFY;return 1; }
			if(pmx2) { *c=FIM_DSK_REDUCE; return 1; }
			if(pmwu) { *c=FIM_DSK_MAGNIFY; return 1; }
			if(pmwd) { *c=FIM_DSK_REDUCE; return 1; }
		}
	return 0;
}
#endif /* FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT */
/* ---- FIM_WANT_PROOF_OF_CONCEPT_MOUSE_SUPPORT stuff END -- */

