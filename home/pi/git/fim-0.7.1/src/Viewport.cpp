/* $LastChangedDate: 2024-04-08 14:22:00 +0200 (Mon, 08 Apr 2024) $ */
/*
 Viewport.cpp : Viewport class implementation

 (c) 2007-2024 Michele Martone

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
#include "Viewport.h"
#include <cmath>	// ceilf
#define FIM_WANT_VIEWPORT_TRANSFORM 1
#define FIM_HSCALE_AFTER_CMT 1
namespace fim
{
#if FIM_WANT_BDI
	static Image fim_dummy_img;
#endif	/* FIM_WANT_BDI */

	Viewport::Viewport(
			CommandConsole& commandConsole,
			DisplayDevice *displaydevice
#ifdef FIM_WINDOWS
			,const Rect & corners
#endif /* FIM_WINDOWS */
			)
			:
#ifdef FIM_NAMESPACES
			Namespace(&commandConsole,FIM_SYM_NAMESPACE_VIEWPORT_CHAR),
#endif /* FIM_NAMESPACES */
			psteps_(false),
			displaydevice_(displaydevice)
#ifdef FIM_WINDOWS
			,corners_(corners)
#endif /* FIM_WINDOWS */
			,image_(FIM_NULL)
			,commandConsole_(commandConsole)
	{
		if(!displaydevice_)
			throw FIM_E_TRAGIC;
		reset();
	}

	Viewport::Viewport(const Viewport& rhs)
		:
#ifdef FIM_NAMESPACES
		Namespace(rhs),
#endif /* FIM_NAMESPACES */
		psteps_(rhs.psteps_)
		,displaydevice_(rhs.displaydevice_)
#ifdef FIM_WINDOWS
		,corners_(rhs.corners_)
#endif /* FIM_WINDOWS */
		,image_(FIM_NULL)
		,commandConsole_(rhs.commandConsole_)
	{
		steps_ = rhs.steps_;
		hsteps_ = rhs.hsteps_;
		vsteps_ = rhs.vsteps_;
		top_ = rhs.top_;
		left_ = rhs.left_;
		panned_ = rhs.panned_;
	#ifdef FIM_CACHE_DEBUG
		if(rhs.image_)
			std::cout << "Viewport:Viewport():maybe will cache \"" <<rhs.image_->getName() << "\" from "<<rhs.image_<<FIM_CNS_NEWLINE ;
		else
			std::cout << "no image_ to cache..\n";
	#endif /* FIM_CACHE_DEBUG */
		if(rhs.image_ && rhs.image_->check_valid())
		{
			ViewportState viewportState;
			setImage( commandConsole_.browser_.cache_.useCachedImage(rhs.image_->getKey(),&viewportState) );
			this->ViewportState::operator=(viewportState);
		}
	}

	fim_bool_t Viewport::pan_up(fim_pan_t s)
	{
		fim_bool_t rv = true;
		panned_ |= 0x1;
		if(s<0)
			rv = pan_down(-s);
		else
		{
			if(this->onTop())
				return false;
			s=(s==0)?steps_:s;
			top_ -= s;
		}
		return rv;
	}

	fim_bool_t Viewport::pan_down(fim_pan_t s)
	{
		fim_bool_t rv = true;
		panned_ |= 0x1;
		if(s<0)
			rv = pan_up(-s);
		else
		{
			if(this->onBottom())
				return false;
			s=(s==0)?steps_:s;
			top_ += s;
		}
		return rv;
	}

	fim_bool_t Viewport::pan_right(fim_pan_t s)
	{
		fim_bool_t rv = true;
		panned_ |= 0x2;
		if(s<0)
			rv = pan_left(-s);
		else
		{
			if(onRight())
				return false;
			s=(s==0)?steps_:s;
			left_+=s;
		}
		return rv;
	}

	fim_bool_t Viewport::pan_left(fim_pan_t s)
	{
		fim_bool_t rv = true;
		panned_ |= 0x2;
		if(s<0)
			rv = pan_right(-s);
		else
		{
			if(onLeft())
				return false;
			s=(s==0)?steps_:s;
			left_-=s;
		}
		return rv;
	}

	bool Viewport::onBottom(fim_coo_t approx_fraction)const
	{
		if( check_invalid() )
			return false;
		const fim_coo_t ph = pixelable_height();
		const fim_coo_t tol = approx_fraction > 1 ? ph / approx_fraction : 0;
		return (top_ + ph + tol >= image_->height());
	}

	bool Viewport::onRight(fim_coo_t approx_fraction)const
	{
		if( check_invalid() )
			return false;
		const fim_coo_t tol = approx_fraction > 1 ? viewport_width() / approx_fraction : 0;
		return (left_ + viewport_width() + tol >= image_->width());
	}

	bool Viewport::onLeft(fim_coo_t approx_fraction)const
	{
		if( check_invalid() )
			return false;
		const fim_coo_t tol = approx_fraction > 1 ? viewport_width() / approx_fraction : 0;
		return (left_ <= tol );
	}

	bool Viewport::onTop(fim_coo_t approx_fraction)const
	{
		if( check_invalid() )
			return false;
		const fim_coo_t ph = pixelable_height();
		const fim_coo_t tol = approx_fraction > 1 ? ph / approx_fraction : 0;
		return (top_ <= tol );
	}

	fim_coo_t Viewport::viewport_width(void)const
	{
#ifdef FIM_WINDOWS
		return corners_.width();
#else /* FIM_WINDOWS */
		return displaydevice_->width();
#endif /* FIM_WINDOWS */
	}

	fim_coo_t Viewport::extra_height(void)const
	{
		const fim_coo_t eh = extra_bottom_height() + extra_top_height();
		return eh;
	}

	fim_coo_t Viewport::extra_bottom_height(void)const
	{
		const fim_coo_t lh = displaydevice_->status_line_height();
		const fim_coo_t ds = (getGlobalIntVariable(FIM_VID_DISPLAY_STATUS)==1?1:0);
		return ds * lh;
	}

	fim_coo_t Viewport::extra_top_height(void)const
	{
		fim_coo_t slh = 0;
#if FIM_HSCALE_AFTER_CMT
		const fim_coo_t ds = (getGlobalIntVariable(FIM_VID_DISPLAY_STATUS)==1?1:0);

		if(ds && image_ && displaydevice_->get_bpp())
		{
			const fim_int wcoi = getGlobalIntVariable(FIM_VID_COMMENT_OI);
#if FIM_WANT_PIC_CMTS
			if(wcoi>=3)
			{
				const fim_coo_t fh = displaydevice_->font_height();
				const std::string i = get_caption_text();
				const fim_coo_t cpl = displaydevice_->get_chars_per_line();
				const fim_coo_t el = ((i.size()+cpl-1)/cpl);
				slh=fh*el;
			}
#endif /* FIM_WANT_PIC_CMTS */
			slh = FIM_MIN(viewport_height()/2,slh);
		}
#endif
		return slh;
	}

	fim_coo_t Viewport::viewport_height(void)const
	{
#ifdef FIM_WINDOWS
		const fim_coo_t vph = corners_.height();
#else /* FIM_WINDOWS */
		const fim_coo_t vph = displaydevice_->height();
#endif /* FIM_WINDOWS */
		return vph;
	}

	fim_coo_t Viewport::pixelable_height(void)const
	{
		fim_coo_t vph = viewport_height();
		vph -= extra_height();
		return vph;
	}

	bool Viewport::redisplay(void)
	{
		// we 'force' a redraw
		should_redraw();
		return display();
	}

	fim_coo_t Viewport::xorigin(void)const
	{
		// horizontal origin coordinate (upper)
#ifdef FIM_WINDOWS
		return corners_.xorigin();
#else /* FIM_WINDOWS */
		return 0;
#endif/* FIM_WINDOWS */
	}

	fim_coo_t Viewport::yorigin(void)const
	{
		// vertical origin coordinate (upper)
#ifdef FIM_WINDOWS
		return corners_.yorigin();
#else /* FIM_WINDOWS */
		return 0;
#endif/* FIM_WINDOWS */
	}

	bool Viewport::shall_negate(void)const
	{
		return ((getGlobalIntVariable(FIM_VID_AUTONEGATE)== 1)&&(image_->check_negated()==0));
	}

	bool Viewport::shall_desaturate(void)const
	{
		return ((getGlobalIntVariable(FIM_VID_AUTODESATURATE)== 1)&&(image_->check_desaturated()==0));
	}

	bool Viewport::shall_mirror(void)const
	{
		return image_->shall_mirror();
	}

	bool Viewport::shall_autotop(void)const
	{
		return /*getGlobalIntVariable(FIM_VID_AUTOTOP)   |*/ image_->check_autotop() /* | getIntVariable(FIM_VID_AUTOTOP)*/;
	}

	bool Viewport::shall_flip(void)const
	{
		return image_->check_flip();
	}

	bool Viewport::display(void)
	{
		/*
		 *	the display function draws the image in the framebuffer
		 *	memory.
		 *	no scaling occurs, only some alignment.
		 *
		 *	returns true when some drawing occurred.
		 */
		if(! need_redraw())
			return false;
		if( check_invalid() )
		{
			if( displaydevice_ )
				displaydevice_->clear_rect( xorigin(), xorigin()+viewport_width()-1, yorigin(), yorigin()+viewport_height()-1);
			return false;
		}

		fim_int autotop = shall_autotop();
		const fim_pan_t vh = this->viewport_height();
		const fim_pan_t ph = this->pixelable_height();

		this->update_meta(false);

		if(shall_negate())
			image_->negate();
		if(shall_desaturate())
			image_->desaturate();

		if ( ( autotop || image_->check_autocenter()==1 ) && need_redraw() )
		{
			if(autotop && image_->height()>=ph)
		  	{
			    top_=autotop>0?0:image_->height()-ph;
			    panned_ |= 0x1; // FIXME: shall do the same for l/r and introduce constants.
			}
			/* start with centered image, if larger than screen */
			if (image_->width()  > this->viewport_width() )
				left_ = (image_->width() - this->viewport_width()) / 2;
			if (image_->height() > ph &&  autotop==0)
				top_ = (image_->height() - ph) / 2;
			image_->set_auto_props(0, 0);
		}
		{
			/*
			 * Old fbi code snippets.
			 */
	    		if (image_->height() <= ph)
	    		{
				top_ = 0;
	    		}
			else 
			{
				if (top_ < 0)
					top_ = 0;
				if (top_ + ph > image_->height())
		    			top_ = image_->height() - ph;
	    		}
			if (image_->width() <= this->viewport_width())
			{
				left_ = 0;
	    		}
			else
			{
				if (left_ < 0)
				    left_ = 0;
				if (left_ + this->viewport_width() > image_->width())
			    		left_ = image_->width() - this->viewport_width();
		    	}
		}
		
		if( need_redraw())
		{
			const bool mirror = shall_mirror();
			const bool flip = shall_flip();
			const fim_flags_t flags = (mirror?FIM_FLAG_MIRROR:0)|(flip?FIM_FLAG_FLIP:0);

			should_redraw(FIM_REDRAW_UNNECESSARY);
			image_->should_redraw(FIM_REDRAW_UNNECESSARY);

#if FIM_WANT_VIEWPORT_TRANSFORM
			this->transform(mirror, flip);
#endif /* FIM_WANT_VIEWPORT_TRANSFORM */
#ifdef FIM_WINDOWS
			if(displaydevice_ )
#endif /* FIM_WINDOWS */
			displaydevice_->display(
					image_->get_ida_image(),
					top_,
					left_,
					image_->height(),
					image_->width(),
					image_->width(),
					extra_top_height()+yorigin(),
					xorigin(),
					ph,
				       	viewport_width(),
				       	viewport_width(),
					flags
					);
#if FIM_WANT_VIEWPORT_TRANSFORM
			this->transform(mirror, flip);
#endif /* FIM_WANT_VIEWPORT_TRANSFORM */
#if FIM_WANT_PIC_CMTS
			/* FIXME: temporary; move to fs_puts_multiline() */
			if(image_)
			if(fim_int wcoi = getGlobalIntVariable(FIM_VID_COMMENT_OI))
			{
#if 0
				int fh=displaydevice_->f_ ? displaydevice_->f_->sheight():1; // FIXME : this is not clean
				int fw=displaydevice_->f_ ? displaydevice_->f_->swidth():1; // FIXME : this is not clean
				int sl = strlen(image_->getStringVariable(FIM_VID_COMMENT)), rw = viewport_width() / fw, wh = viewport_height();
				for( int li = 0 ; sl > rw * li ; ++li )
					if((li+1)*fh<wh) /* FIXME: maybe this check shall better reside in fs_puts() ? */
					displaydevice_->fs_puts(displaydevice_->f_, 0, fh*li, cmnts+rw*li);
#else
				displaydevice_->fs_multiline_puts( get_caption_text().c_str(), wcoi-1, viewport_width(), vh);
#endif
			}
#endif /* FIM_WANT_PIC_CMTS */

#define FIM_WANT_BROWSER_PROGRESS_BAR 0 /* new */
#if FIM_WANT_BROWSER_PROGRESS_BAR 
			const fim_float_t bp = commandConsole_.browser_.file_progress() * 100.0;
			const fim_pan_t bw = 1; // bar width
			const fim_pan_t vw = viewport_width();
			const fim_coo_t xc = vw; // x coordinate
			const fim_color_t bc = FIM_CNS_WHITE;
			if(xc>bw) displaydevice_->fill_rect(xc-bw, xc, 0, FIM_FLT_PCNT_OF_100(bp,vh-1), bc);
			// if(xc>bw) displaydevice_->fill_rect(xc-bw, xc, FIM_FLT_PCNT_OF_100(bp,vh-1),vh, bc);
			// displaydevice_->fill_rect(0, 1, 0, FIM_FLT_PCNT_OF_100(bp,displaydevice_->height()-1), bc);
			// displaydevice_->fill_rect(0, FIM_FLT_PCNT_OF_100(bp,displaydevice_->width()-1), 0, 1, bc);
#endif /* FIM_WANT_BROWSER_PROGRESS_BAR */
			return true;
		}
		return false;
	}

	void Viewport::auto_scale(void)
	{
		fim_scale_t xs,ys;

		if( check_invalid() )
			return;
		else
			xs = viewport_xscale(),
			ys = viewport_yscale();

		image_->do_scale_rotate(FIM_MIN(xs,ys));
	}

	void Viewport::auto_scale_if_bigger(void)
	{
		if( check_invalid() )
			return;
		else
		{
			if((this->viewport_width()<(image_->original_width()*image_->ascale()))
			||(this->pixelable_height() < image_->original_height()))
				auto_scale();
		}
	}

        Image* Viewport::getImage(void)const
	{
#if FIM_WANT_BDI
		if(!image_)
			return &fim_dummy_img;
		else
#endif	/* FIM_WANT_BDI */
#if FIM_IMG_NAKED_PTRS
			return image_;
#else /* FIM_IMG_NAKED_PTRS */
			return image_.get();
#endif /* FIM_IMG_NAKED_PTRS */
	}

        const Image* Viewport::c_getImage(void)const
	{
		return getImage();
	}

        void Viewport::setImage(fim::ImagePtr ni)
	{
#ifdef FIM_CACHE_DEBUG
		std::cout << "setting image \""<< ni->getKey() << " in viewport: "<< ni << "\n\n";
#endif /* FIM_CACHE_DEBUG */

		if(ni)
			free_image(false);
		reset();
		image_ = ni;
	}

        void Viewport::steps_reset(void)
	{
#ifdef FIM_WINDOWS
		steps_ = getGlobalIntVariable(FIM_VID_STEPS);
		if(steps_<FIM_CNS_STEPS_MIN)
			steps_ = FIM_CNS_STEPS_DEFAULT_N;
		else
			psteps_=(getGlobalStringVariable(FIM_VID_STEPS).re_match("%$"));

		hsteps_ = getGlobalIntVariable(FIM_VID_HSTEPS);
		if(hsteps_<FIM_CNS_STEPS_MIN)
			hsteps_ = steps_;
		else psteps_=(getGlobalStringVariable(FIM_VID_HSTEPS).re_match("%$"));

		vsteps_ = getGlobalIntVariable(FIM_VID_VSTEPS);
		if(vsteps_<FIM_CNS_STEPS_MIN)
			vsteps_ = steps_;
		else psteps_=(getGlobalStringVariable(FIM_VID_VSTEPS).re_match("%$"));
#else  /* FIM_WINDOWS */
		hsteps_ = vsteps_ = steps_ = FIM_CNS_STEPS_DEFAULT_N;
		psteps_ = FIM_CNS_STEPS_DEFAULT_P;
#endif /* FIM_WINDOWS */
	}

        void Viewport::reset(void)
        {
		if(image_)
			image_->reset_view_props(),
			image_->set_auto_props(1, 0);

		should_redraw();
                top_  = 0;
                left_ = 0;
        	steps_reset();
        }

	void Viewport::auto_height_scale(void)
	{
		fim_scale_t newscale;
		if( check_invalid() )
			return;
		newscale = viewport_yscale();
		image_->do_scale_rotate(newscale);
	}

	void Viewport::auto_width_scale(void)
	{
		if( check_invalid() )
			return;

		image_->do_scale_rotate(viewport_xscale());
	}

	void Viewport::free_image(bool force)
	{
		if(image_)
			commandConsole_.browser_.cache_.freeCachedImage(image_,this,force);
		image_ = FIM_NULL;
	}

        bool Viewport::check_valid(void)const
	{
		return ! check_invalid();
	}

        bool Viewport::check_invalid(void)const
	{
		if(!image_)
			return true;
		else
			return !image_->check_valid();
	}

	void Viewport::scale_position_magnify(fim_scale_t factor)
	{
		/*
		 * scale image positioning variables by adjusting by a multiplying factor
		 * */
		if(factor<=0.0)
			return;
		left_ = (fim_off_t)ceilf(((fim_scale_t)left_)*factor);
		top_  = (fim_off_t)ceilf(((fim_scale_t)top_ )*factor);
		/*
		 * should the following be controlled by some optional variable ?
		 * */
		//if(!panned_  /* && we_want_centering */ )
			this->recenter();
	}

	void Viewport::scale_position_reduce(fim_scale_t factor)
	{
		/*
		 * scale image positioning variables by adjusting by a multiplying factor
		 * */
		if(factor<=0.0)
			return;
		left_ = (fim_off_t)ceilf(FIM_INT_SCALE_FRAC(left_,factor));
		top_  = (fim_off_t)ceilf(FIM_INT_SCALE_FRAC(top_ ,factor));
		//if(!panned_  /* && we_want_centering */ )
			this->recenter();
	}

	void Viewport::recenter_horizontally(void)
	{
		if(image_)
			left_ = (image_->width() - this->viewport_width()) / 2;
	}

	void Viewport::recenter_vertically(void)
	{
		if(image_)
			top_ = (image_->height() - this->pixelable_height()) / 2;
	}

	void Viewport::recenter(void)
	{
		if(!(panned_ & 0x02))
			recenter_horizontally();
		if(!(panned_ & 0x01))
			recenter_vertically();
	}

	Viewport::~Viewport(void)
	{
		free_image(false);
	}

	template <class PT>
	bool Viewport::pan_to_t(const PT px, const PT py)
	{
		/* merge this with a pan function */
		assert( 0 <= px && px <= 100 && 0 <= py && py <= 100 );
		PT _px = px, _py = py;

#if FIM_WANT_VIEWPORT_TRANSFORM
		if(image_ && image_->is_flipped())
			_py = 100 - _py;
		if(image_ && image_->is_mirrored())
			_px = 100 - _px;
#endif /* FIM_WANT_VIEWPORT_TRANSFORM */

		if(image_)
		{
			fim_pan_t ih = image_->height();
			fim_pan_t iw = image_->width();
			fim_pan_t vh = this->pixelable_height();
			fim_pan_t vw = this->viewport_width();
			fim_off_t top = top_, left = left_;

			if(ih>vh)
				top = FIM_INT_PCNT_SAFE(_py,ih-vh);
			if(iw>vw)
				left = FIM_INT_PCNT_SAFE(_px,iw-vw);

			if( top != top_ || left != left_ )
			{
				top_ = top;
			       	left_ = left;
				should_redraw();
			}
		}
		return true;
	}

	bool Viewport::pan_to(const fim_pan_t px, const fim_pan_t py)
	{
		return pan_to_t<fim_pan_t>(px, py);
	}

	bool Viewport::pan_to(const double px, const double py)
	{
		return pan_to_t<double>(px, py);
	}

	bool Viewport::pan(const fim_char_t*a1, const fim_char_t*a2)
	{
		if(a1 || a2)
			return do_pan({ (a1 ? a1 : a2 ) });
		else
			return do_pan({});
	}

	bool Viewport::do_pan(const args_t& args)
	{
		fim_pan_t hs=0,vs=0;
		fim_bool_t ps=false;
		fim_char_t f=FIM_SYM_CHAR_NUL,s=FIM_SYM_CHAR_NUL;
		const fim_char_t*const fs=args[0];
		const fim_char_t*ss=FIM_NULL;
		fim_bool_t prv = true;
		fim_bool_t wpt = false;

		if(args.size()<1 || (!fs))
			goto ret;
		f=tolower(*fs);
		if(!fs[0])
			goto ret;
		s=tolower(fs[1]);
        	steps_reset();

		if(args.size()>=2)
		{
			if ( isdigit(*args[0]) && isdigit(*args[1]) )
			{
				ps=((ss=args[0]) && *ss && ((ss[strlen(ss)-1])=='%') && (ss=args[1]) && *ss && ((ss[strlen(ss)-1])=='%'));
				wpt = ps;
				vs=(int)(args[0]);
				hs=(int)(args[1]);
			}
			else
			{
				ps=((ss=args[1]) && *ss && ((ss[strlen(ss)-1])=='%'));
				vs=hs=(int)(args[1]);
			}
		}
		else
		{
			ps = psteps_;
			vs = vsteps_;
			hs = hsteps_;
		}
		if(ps)
		{
			vs = FIM_INT_PCNT(pixelable_height(),vs);
			hs = FIM_INT_PCNT(viewport_width(), hs);
		}

#if FIM_WANT_VIEWPORT_TRANSFORM
		if(image_ && image_->is_flipped())
			vs=-vs;
		if(image_ && image_->is_mirrored())
			hs=-hs;
#endif /* FIM_WANT_VIEWPORT_TRANSFORM */

		if (wpt)
		{
			pan_to(hs,vs);
		}
		else
		switch(f)
		{
			case('u'):
				prv=pan_up(vs);
			break;
			case('d'):
				prv=pan_down(vs);
			break;
			case('r'):
				prv=pan_right(hs);
			break;
			case('l'):
				prv=pan_left(hs);
			break;
			case('n'):
			case('s'):
			if(f=='n')
		       		prv=pan_up(vs);
			if(f=='s')
			       	prv=pan_down(vs);
			switch(s)
			{
				case('e'):
					prv=pan_left(hs);
				break;
				case('w'):
					prv=pan_right(hs);
				break;
				default:
					goto ret;
			}
			break;
			default:
			goto ret;
		}
		should_redraw();
ret:
		return prv;
	}

	size_t Viewport::byte_size(void)const
	{
		size_t bs = 0;
		bs += sizeof(*this);
		return bs;
	}

	int Viewport::snprintf_centering_info(char *str, size_t size)const
	{
		int vum,vlm;
		int hum,hlm;
		float va,ha;
		char vc='c',hc='c';
		int res=0;
		const char*cs=FIM_NULL;
		const char*es="";
		const float bt=99.0;
		const float ct=1.0;

		vum = top_;
	        vlm = image_->height() - pixelable_height() - vum;
		hum = left_;
	       	hlm = image_->width() - viewport_width() - hum;
		vum = FIM_MAX(vum,0);
		vlm = FIM_MAX(vlm,0);
		hum = FIM_MAX(hum,0);
		hlm = FIM_MAX(hlm,0);

		if(vum<1 && vlm<1)
		{
			va=0.0;
			// res = snprintf(str+res, size-res, "-");
		}
		else
		{
			va=((float)(vum))/((float)(vum+vlm));
			va-=0.5;
			va*=200.0;
			if(va<0.0)
			{
				va*=-1.0,vc='^';
				if(va>=bt)
					cs="top";
			}
			else
			{
				vc='v';
				if(va>=bt)
					cs="bot";
			}
			if(va<ct)
				cs="";
			if(cs)
				res += snprintf(str+res, size-res, "%s%s",es,cs);
			else
				res += snprintf(str+res, size-res, "%s%2.0f%%%c",es,va,vc);
		}
	
		if(cs)
			es=" ",cs=FIM_NULL;

		if(hum<1 && hlm<1)
			ha=0.0;
		else
		{
			ha=((float)(hum))/((float)(hum+hlm));
		       	ha-=0.5;
			ha*=200.0;
			if(ha<0.0)
			{
				ha*=-1.0,hc='<';
				if(ha>=bt)
					cs="left";
			}
			else
			{
				hc='>';
				if(ha>=bt)
					cs="right";
			}
			if(ha<ct)
				cs="";
			if(cs)
				res += snprintf(str+res, size-res, "%s%s",es,cs);
			else
				res += snprintf(str+res, size-res, "%s%2.0f%%%c",es,ha,hc);
		}
		return res;
	}

	void Viewport::align(const char c)
	{
		switch( c )
		{
		case 'b':
		{
			if(this->onBottom())
				goto ret;
			if( check_valid() )
				top_ = image_->height() - this->pixelable_height();
		}
		break;
		case 't':
		{
			if(this->onTop())
				goto ret;
			top_=0;
		}
		break;
		case 'l':
			left_=0;
		break;
		case 'r':
	       		left_ = image_->width() - viewport_width();
		break;
		case 'c':
			//this->recenter();
			this->recenter_vertically(); this->recenter_horizontally();
		break;
		default:
			goto ret;
		}
		should_redraw();
ret:
		return;
	}

	Viewport& Viewport::operator= (const Viewport&rhs){return *this;/* a disabled assignment */}

	void Viewport::transform(bool mirror, bool flip)
	{
		if(mirror)
		{ if( image_ && image_->width() > viewport_width() ) left_ = image_->width() - viewport_width() - left_; }
		if(flip)
		{ if( image_ && image_->height() > pixelable_height() ) top_ = image_->height() - pixelable_height() - top_; }
	}
	
	fim_bool_t Viewport::need_redraw(void)const
	{
		return ( ( redraw_ != FIM_REDRAW_UNNECESSARY ) || ( image_ && image_->need_redraw() ) || ( displaydevice_ && displaydevice_->need_redraw() ) );
       	}
	void Viewport::should_redraw(enum fim_redraw_t sr)
       	{
		redraw_ = sr;
       	} 

	fim_scale_t Viewport::viewport_xscale(void)const
	{
		assert(image_);
		return FIM_INT_SCALE_FRAC(this->viewport_width(),image_->original_width()*image_->ascale());
	}

	fim_scale_t Viewport::viewport_yscale(void)const
	{
		assert(image_);
		const fim_coo_t ph = this->pixelable_height();
		return FIM_INT_SCALE_FRAC(ph,static_cast<fim_scale_t>(image_->original_height()));
	}

	fim_cxr Viewport::img_color(const args_t& args)
	{
		if( !c_getImage() )
			goto nop;

		if(args.size()>0)
		{
			bool daltonize=false;
			enum fim_cvd_t cvd=FIM_CVD_NO;

			if( args[0] == "desaturate" )
				if( getImage()->desaturate() )
					goto nop;
			if( args[0] == "negate" )
				if( getImage()->negate() )
					goto nop;
			if( args[0] == "identity" )
				if( getImage()->identity() )
					goto nop;

			if( args.size()>1 && ( args[1] == "daltonize" || args[1] == "D" ) )
				daltonize=true;
			if( args[0] == "c" || args[0] == "colorblind"  ||
			    args[0] == "d" || args[0] == "deuteranopia" )
				cvd = FIM_CVD_DEUTERANOPIA;
			if( args[0] == "p" || args[0] == "protanopia" )
				cvd = FIM_CVD_PROTANOPIA;
			if( args[0] == "t" || args[0] == "tritanopia" )
				cvd = FIM_CVD_TRITANOPIA  ;

			if(cvd!=FIM_CVD_NO)
				if( getImage()->colorblind(cvd,daltonize) )
					goto nop;
		}
nop:
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_err_t Viewport::img_rotate(fim_angle_t angle)
	{
		if(getImage())
			return getImage()->rotate(angle);
		return FIM_ERR_NO_ERROR;
	}

	bool Viewport::img_goto_page(fim_page_t j)
	{
		if(getImage())
		{
			return getImage()->goto_page(j);
		}
		return false;
	}

#if FIM_WANT_CROP
	fim_cxr Viewport::img_crop(const args_t& args, const string &current)
	{
		ida_rect prect {25,25,75,75};
#if FIM_WANT_MOUSE_CROP
		const std::string rsv = cc.getStringVariable(FIM_VID_CROP_ONCE);

		if( args.size() == 0 && ! rsv.empty() )
		{
			std::istringstream is(rsv);
			int x1, y1, x2, y2;
			is >> x1;
			is >> y1;
			is >> x2;
			is >> y2;

			const int ye = image_->height();
			const int xe = image_->width();

			x1 = x1 / ( xe / 100.0 );
			x2 = x2 / ( xe / 100.0 );
			y1 = y1 / ( ye / 100.0 );
			y2 = y2 / ( ye / 100.0 );

			cc.setVariable(FIM_VID_CROP_ONCE, 0);
			prect = {x1,y1,x2,y2};
		}
#endif /* FIM_WANT_MOUSE_CROP */

		if(args.size() == 1)
		{
			const int ap = (int)(args[0]);
			const int x1 = (100-ap)/2;
			const int y1 = (100-ap)/2;
			const int x2 = 100-x1;
			const int y2 = 100-y1;
			prect = {x1,y1,x2,y2};
		}

		if(args.size() == 2)
		{
			const int xs = (int)(args[0]);
			const int ys = (int)(args[1]);
			const int x1 = (100-xs)/2;
			const int y1 = (100-ys)/2;
			const int x2 = 100-x1;
			const int y2 = 100-y1;
			prect = {x1,y1,x2,y2};
		}

		if(args.size() == 4)
		{
			const int x1 = (int)(args[0]);
			const int y1 = (int)(args[1]);
			const int x2 = (int)(args[2]);
			const int y2 = (int)(args[3]);
			prect = {x1,y1,x2,y2};
		}

		if ( image_ )
		{
			FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PRESCALE,current)
			image_->do_crop(prect);
			FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTSCALE)
		}
		return FIM_CNS_EMPTY_RESULT;
	}
#endif /* FIM_WANT_CROP */

	fim_cxr Viewport::img_scale(const args_t& args, const string &current)
	{
		/*
		 * scales the image to a certain scale factor
		 *  - no user error checking -- poor error reporting for the user
		 *  - wxh / w:h syntax needed
		 *  - shall better belong to viewport
		 */
		fim_scale_t newscale = FIM_CNS_SCALEFACTOR_ZERO;
		fim_char_t fc = FIM_SYM_CHAR_NUL;
		const fim_char_t*ss = FIM_NULL;
		int sl = 0;
		bool pcsc = false;
		bool aes = false; // approximate exponential scaling

		if( args.size() < 1 || !(ss=args[0].c_str() ))
			goto nop;
		fc = tolower(*ss);
		sl = strlen(ss);

#if FIM_WANT_APPROXIMATE_EXPONENTIAL_SCALING
		{
			if( fc == '<' )
			{
				newscale = 0.5;
				fc='+';
				aes = true;
				goto comeon;
			}
			if( fc == '>' )
			{
				newscale = 2;
				fc='+';
				aes = true;
				goto comeon;
			}
		}
#endif /* FIM_WANT_APPROXIMATE_EXPONENTIAL_SCALING */

		if( isalpha(fc) )
		{
			if( !( fc == 'w' || fc == 'h' || fc == 'a' || fc == 'b' ) )
				goto nop;
		}
		else
		{
			if( sl == 1 && ( fc =='+' || fc == '-' ) )
			{
				if( fc == '+' )
					newscale = getGlobalFloatVariable(FIM_VID_MAGNIFY_FACTOR);
				if( fc == '-' )
					newscale = getGlobalFloatVariable(FIM_VID_REDUCE_FACTOR);
				goto comeon;
			}
			if( sl >= 2 && ( fc == '+' || fc == '-' ) )
			{
				fim_char_t sc =ss[1];

				if( fc == '+' )
				{
					fim_scale_t vmf = getGlobalFloatVariable(FIM_VID_MAGNIFY_FACTOR);
					fim_scale_t vrf = getGlobalFloatVariable(FIM_VID_REDUCE_FACTOR);
					fim_scale_t sfd = getGlobalFloatVariable(FIM_VID_SCALE_FACTOR_DELTA);
					fim_scale_t sfm = getGlobalFloatVariable(FIM_VID_SCALE_FACTOR_MULTIPLIER);
					if( sfd <= FIM_CNS_SCALEFACTOR_ZERO )
						sfd = FIM_CNS_SCALEFACTOR_DELTA;
					if( sfm <= FIM_CNS_SCALEFACTOR_ONE )
					       	sfm = FIM_CNS_SCALEFACTOR_MULTIPLIER;

					switch(sc)
					{
						case('+'):
						{
							vrf += sfd;
							vmf += sfd;
						}
						break;
						case('-'):
						{
							vrf -= sfd;
							vmf -= sfd;
						}
						break;
						case('*'):
						{
							vrf *= sfm;
							vmf *= sfm;
						}
						break;
						case('/'):
						{
							vrf /= sfm;
							vmf /= sfm;
						}
						break;
						default:
						goto noplus;
					}

					setGlobalVariable(FIM_VID_REDUCE_FACTOR, vrf);
					setGlobalVariable(FIM_VID_MAGNIFY_FACTOR,vmf);
					goto nop;
				}
noplus:
				if( fc == '+' || fc == '-')
				{
					newscale = fim_atof(ss+1);
					pcsc = (strstr(ss,"%") != FIM_NULL );
					if(pcsc)
						newscale *= .01;
					if( !newscale )
						goto nop;
#if 1
					if( fc == '+' )
						newscale = 1.0 + newscale;
					if( fc == '-' )
						newscale = 1.0 - newscale;
					fc = FIM_SYM_CHAR_NUL;
#endif
					goto comeon;
				}
				goto nop; //unreachable (FIXME: this and a few others..)
			}
			if( sl )
			{
				if(fc=='*')
				{
					++ss;
					if(!*ss)
						goto nop; /* a '*' alone. may assign a special meaning to this... */
				}
				newscale = fim_atof(ss);
				if(fc=='*')
				{
					fc = '+';
					goto comeon;
				}
				pcsc = (strstr(ss,"%") != FIM_NULL );
				if(pcsc)
					newscale *= .01;
				if( newscale == FIM_CNS_SCALEFACTOR_ZERO )
				       	goto nop;
				pcsc = false;
				goto comeon;
			}
			goto nop;
		}
comeon:
#if FIM_WANT_BDI
		if(1)
#else	/* FIM_WANT_BDI */
		if(c_getImage())
#endif	/* FIM_WANT_BDI */
		{
			fim_err_t errval = FIM_ERR_NO_ERROR;
			FIM_AUTOCMD_EXEC_PRE(FIM_ACM_PRESCALE,current)
			this->update_meta(false);
			if( c_getImage() )
			switch( fc )
			{
				case('w'):
					this->auto_width_scale();
				break;
				case('h'):
				this->auto_height_scale();
				break;
				case('a'):
					this->auto_scale();
				break;
				case('b'):
					this->auto_scale_if_bigger();
				break;
				case('-'):
				{
					if( newscale )
					{
						if(c_getImage())
							errval = getImage()->reduce(newscale);
						this->scale_position_reduce(newscale);
					}
					else	
					{
						if(c_getImage())
							errval = getImage()->reduce();
						this->scale_position_reduce();
					}
				}
				break;
				case('+'):
				{
					if( newscale )
					{
						if(c_getImage())
							errval = getImage()->magnify(newscale,aes);
						this->scale_position_magnify(newscale);
					}
					else	
					{
						if(c_getImage())
							errval = getImage()->magnify();
						this->scale_position_magnify();
					}
				}
				break;
				default:
				if( pcsc )
					errval = getImage()->scale_multiply(newscale);
				else
					errval = getImage()->set_scale(newscale);
			}
			FIM_AUTOCMD_EXEC_POST(FIM_ACM_POSTSCALE)
			if( errval != FIM_ERR_NO_ERROR )
			{
				// Here might handle reporting of an error..
			}
		}
nop:
		return FIM_CNS_EMPTY_RESULT;
	}

	fim_err_t Viewport::update_meta(bool fresh)
	{
		fim_err_t errval = FIM_ERR_NO_ERROR;
		if(c_getImage())
			getImage()->update_meta(false);
		return errval;
	}

	std::string Viewport::get_caption_text(void)const
	{
		std::string ct;

		if(isSetGlobalVar(FIM_VID_COMMENT_OI_FMT))
		{
			const fim_uint cpl = displaydevice_->get_chars_per_line();
			const fim_uint maxchrs = cpl*(displaydevice_->get_chars_per_column()/2);
			const std::string i = commandConsole_.getInfoCustom(getGlobalStringVariable(FIM_VID_COMMENT_OI_FMT));
			if(i.length()>maxchrs)
			{
				ct=i.substr(0,maxchrs);
				if(maxchrs>3) ct[maxchrs-1]='.', ct[maxchrs-2]='.', ct[maxchrs-3]='.';
			}
			else
				ct=i;
		}
		return ct;
	}

	void Viewport::mouse_event_pan(const fim_int mx, const fim_int my, const fim_coo_t xv, const fim_coo_t yv)
	{
#if FIM_WANT_MOUSE_PAN
		//sdld_cout << current_w_    << " " << event.motion.y    << "\n";
		//sdld_cout << event.motion.x    << " " << event.motion.y    << "\n";
		//sdld_cout << event.motion.xrel << " " << event.motion.yrel << "\n";
		static bool discardedfirst = false;
		if(discardedfirst)
		if(const Image* ci = this->c_getImage())
		if(ci->check_valid())
		if( mx > 0 && my > 0 )
		{
			// 1/(2*bf) of each screen side will be insensitive to mouse movement.
			const fim_off_t bf = 5;
			const fim_off_t vx = xv;
			const fim_off_t vy = yv;
			const fim_off_t bx = vx / bf;
			const fim_off_t by = vy / bf;
			const fim_coo_t riw = ci->width();
			const fim_coo_t rih = ci->height();

#define FIM_WANT_POSITION_DISPLAYED FIM_WANT_MOUSE_PAN && 0
#if FIM_WANT_POSITION_DISPLAYED
			gx = (riw<vx) ? -1 : mx;
			gy = (rih<vy) ? -1 : my;
#endif /* FIM_WANT_POSITION_DISPLAYED */

			if(FIM_WANT_VARIABLE_RESOLUTION_MOUSE_SCROLL)
			{
				// variable resolution mouse scroll (maximal smoothess)
				// max of xres/yres discrete scrolls.
				const fim_off_t xres = FIM_MIN(vx-bx,riw), yres = FIM_MIN(vy-by,rih);
				double px = FIM_DELIMIT_TO_X(FIM_INT_DET_PX(mx-bx/2,vx-bx,xres),xres);
				double py = FIM_DELIMIT_TO_X(FIM_INT_DET_PX(my-by/2,vy-by,yres),yres);
				px*=100.0/xres;
				py*=100.0/yres;
				this->pan_to(FIM_DELIMIT_TO_100(px),FIM_DELIMIT_TO_100(py));
			}
			else
			{
				// percentage scroll.
				// downside: if image very wide/tall/big, will
				// steps might be very large.
				const fim_off_t px = FIM_DELIMIT_TO_100(FIM_INT_DET_PCNT(mx-bx/2,vx-bx));
				const fim_off_t py = FIM_DELIMIT_TO_100(FIM_INT_DET_PCNT(my-by/2,vy-by));
				//sdld_cout << "pct:"<< px << " " << py << "\n";
				this->pan_to(px,py);
			}
			this->display(); // draw only if necessary
			//this->redisplay(); // draw always
		}
		discardedfirst=true;
#endif /* FIM_WANT_MOUSE_PAN */
	}

	void Viewport::mouse_scroll(const fim_float_t x)
	{
		// as of now, only sign of x matters
		const auto f = +x;
#if FIM_WANT_MOUSE_WHEEL_SCROLL
		if(auto img = c_getImage())
		{
			const fim_float_t cs = img->get_scale();
			const fim_float_t ms = 16.0*sqrt(((double)(viewport_width()*viewport_height()))/((double)(img->original_width()*img->original_height()))); // TODO: continue from here to develop a concept of "large enough image"
			const fim_coo_t tf = 2; // tolerance factor
			const bool xt = img->width()  > tf * viewport_width() && // x limit trespassed
					img->width()  > tf * img->original_width();
			const bool yt = img->height() > tf * viewport_height() && // y limit trespassed
					img->height() > tf * img->original_height();
			const fim_float_t sf = (f<0) ? f : FIM_MIN(ms/cs,f);

			if ( ( cs<ms && !(xt && yt) ) || sf<0)
				img_scale({(sf<0)?"-":"+"}, commandConsole_.browser_.current());
		}
#endif /* FIM_WANT_MOUSE_WHEEL_SCROLL */
	}
} /* namespace fim */
