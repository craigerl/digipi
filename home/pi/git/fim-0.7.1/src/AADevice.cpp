/* $LastChangedDate: 2024-03-30 12:48:15 +0100 (Sat, 30 Mar 2024) $ */
/*
 AADevice.cpp : aalib device Fim driver file

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

#ifdef FIM_WITH_AALIB

#include "AADevice.h"
#include <aalib.h>

#define FIM_AA_MINWIDTH 2
#define FIM_AA_MINHEIGHT 2
/*
  FIXME : aalib has two resolutions : an input one, and a screen one.
  	  this is not well handled by our code, as we expect a 1:1 mapping.
 */
#define min(x,y) ((x)<(y)?(x):(y))
typedef char fim_aa_char;	/* a type for aalib chars */

static bool aainvalid;

	template <fim_color_t PIXELCOL>
	fim_err_t AADevice::clear_rect_(
		void* dst,	// destination gray array and source rgb array
		fim_coo_t oroff,fim_coo_t ocoff,	// row  and column  offset of the first output pixel
		fim_coo_t orows,fim_coo_t ocols,	// rows and columns drawable in the output buffer
		fim_coo_t ocskip		// output columns to skip for each line
	)
	{
		/* output screen variables */
		const fim_byte_t PIXELVAL = (fim_byte_t) PIXELCOL;
		fim_coo_t 
			oi,// output image row index
			oj;// output image columns index
		fim_coo_t lor,loc;
    		
		if( oroff <0 ) return -8;
		if( ocoff <0 ) return -9;
		if( orows <=0 ) return -10;
		if( ocols <=0 ) return -11;
		if( ocskip<0 ) return -12;

		if( oroff>orows ) return -8-10*100;
		if( ocoff>ocols ) return -9-11*100;

		if( ocskip<ocols ) return -12-11*100;

		/*
		 * orows and ocols is the total number of rows and columns in the output window.
		 * no more than orows-oroff rows and ocols-ocoff columns will be rendered, however
		 * */

		lor = orows-1;
		loc = ocols-1;

/*		cout << iroff << " " << icoff << " " << irows << " " << icols << " " << icskip << "\n";
		cout << oroff << " " << ocoff << " " << orows << " " << ocols << " " << ocskip << "\n";
		cout << idr << " " << idc << " " << "\n";
		cout << loc << " " << lor << " " << "\n";*/

		/* TODO : unroll me and use FIM_LIKELY :) */
		for(oi=oroff;oi<lor;++oi)
		for(oj=ocoff;oj<loc;++oj)
		{
			((fim_byte_t*)(dst))[oi*ocskip+oj]=PIXELVAL;
		}
		return  FIM_ERR_NO_ERROR;
	}

	static fim_err_t matrix_copy_rgb_to_gray(
		void* dst, void* src,	// destination gray array and source rgb array
		fim_coo_t iroff,fim_coo_t icoff,	// row  and column  offset of the first input pixel
		fim_coo_t irows,fim_coo_t icols,	// rows and columns in the input image
		fim_coo_t icskip,		// input columns to skip for each line
		fim_coo_t oroff,fim_coo_t ocoff,	// row  and column  offset of the first output pixel
		fim_coo_t orows,fim_coo_t ocols,	// rows and columns drawable in the output buffer
		fim_coo_t ocskip,		// output columns to skip for each line
		fim_flags_t flags		// some flags :flag values in fim.h : FIXME
	)
	{
		/*
		 * This routine will be optimized, some day.
		 * Note that it is not a member function.
		 * TODO : optimize and generalize, in all possible ways:
		 * 	RGBTOGRAY
		 * 	RGBTOGRAYGRAYGRAY
		 * 	...
		 *
		 * This routine copies the pixelmap starting at (iroff,icoff) in the input image
		 * to the output buffer, starting at (oroff,ocoff).
		 *
		 * The last source pixel copied will be the one :
		 * 	(min(irows-1,orows-1-oroff+iroff),min(icols-1,ocols-1-ocoff+icoff))
		 * in the input buffer and will be written to:
		 * 	(min(orows-1,irows-1-iroff+oroff),min(ocols-1,icols-1-icoff+ocoff))
		 * in the output buffer.
		 *
		 * It assumes that both pixelmaps are stored in row major order, with row strides
		 * of icskip (columns in the  input matrix between rows) for the  input array src,
		 * of ocskip (columns in the output matrix between rows) for the output array dst.
		 *
		 * The routine needs to know the  input image rows (irows), columns (icols).
		 * The routine needs to know the output image rows (orows), columns (ocols).
		 *
		 * It assumes the input pixelmap having three bytes for pixel, and the puts in the 
		 * output buffer the corresponding single byte gray values.
		 * */

		fim_coo_t
			ii,// output image row index
			ij;// output image columns index

		/* output screen variables */
		fim_coo_t 
			oi,// output image row index
			oj;// output image columns index

		fim_color_t gray;
		fim_byte_t*srcp;
		fim_coo_t idr,idc,lor,loc;
    		
		const fim_flags_t mirror=flags&FIM_FLAG_MIRROR, flip=flags&FIM_FLAG_FLIP;//STILL UNUSED : FIXME

		if ( !src ) return FIM_ERR_GENERIC;
	
		if( iroff <0 ) return -3;
		if( icoff <0 ) return -4;
		if( irows <=0 ) return -5;
		if( icols <=0 ) return -6;
		if( icskip<0 ) return -7;
		if( oroff <0 ) return -8;
		if( ocoff <0 ) return -9;
		if( orows <=0 ) return -10;
		if( ocols <=0 ) return -11;
		if( ocskip<0 ) return -12;
		if( flags <0 ) return -13;

		if( iroff>irows ) return -3-4*100 ;
		if( icoff>icols ) return -4-6*100;
		if( oroff>orows ) return -8-10*100;
		if( ocoff>ocols ) return -9-11*100;

		if( icskip<icols ) return -7-6*100;
		if( ocskip<ocols ) return -12-11*100;

		/*
		 * orows and ocols is the total number of rows and columns in the output window.
		 * no more than orows-oroff rows and ocols-ocoff columns will be rendered, however
		 * */

//		dr=(min(irows-1,orows-1-oroff+iroff))-(min(orows-1,irows-1-iroff+oroff));
//		dc=(min(icols-1,ocols-1-ocoff+icoff))-(min(ocols-1,icols-1-icoff+ocoff));

		idr = iroff-oroff;
		idc = icoff-ocoff;

		lor = (min(orows-1,irows-1-iroff+oroff));
		loc = (min(ocols-1,icols-1-icoff+ocoff));

/*		cout << iroff << " " << icoff << " " << irows << " " << icols << " " << icskip << "\n";
		cout << oroff << " " << ocoff << " " << orows << " " << ocols << " " << ocskip << "\n";
		cout << idr << " " << idc << " " << "\n";
		cout << loc << " " << lor << " " << "\n";*/

		/* TODO : unroll me and optimize me :) */
		if(!mirror && !flip)
		for(oi=oroff;FIM_LIKELY(oi<lor);++oi)
		for(oj=ocoff;FIM_LIKELY(oj<loc);++oj)
		{
			ii    = oi + idr;
			ij    = oj + idc;
			srcp  = ((fim_byte_t*)src)+(3*(ii*icskip+ij));
			gray  = (fim_color_t)srcp[0];
			gray += (fim_color_t)srcp[1];
			gray += (fim_color_t)srcp[2];
			((fim_byte_t*)(dst))[oi*ocskip+oj]=(fim_byte_t)(gray/3);
		}
		else
		for(oi=oroff;FIM_LIKELY(oi<lor);++oi)
		for(oj=ocoff;FIM_LIKELY(oj<loc);++oj)
		{
			/*
			 * FIXME : these expressions () are correct, but some garbage is printed in flip mode!
			 * therefore instead of lor-oi+1 we use lor-oi !!
			 * DANGER
			 * */
			/*if(flip)  ii    = (lor-oi) + idr;
			else      ii    = oi + idr;

			if(mirror)ij    = (loc-oj) + idc;
			else      ij    = oj + idc;
			if(ij<0)return FIM_ERR_GENERIC;*/

			ii    = oi + idr;
			ij    = oj + idc;
			
			if(mirror)ij=((icols-1)-ij);
			if( flip )ii=((irows-1)-ii);
			srcp  = ((fim_byte_t*)src)+(3*(ii*icskip+ij));
			if(mirror)ij=((icols-1)-ij);
			if( flip )ii=((irows-1)-ii);

			gray  = (fim_color_t)srcp[0];
			gray += (fim_color_t)srcp[1];
			gray += (fim_color_t)srcp[2];
			((fim_byte_t*)(dst))[oi*ocskip+oj]=(fim_byte_t)(gray/3);
		}

		return  FIM_ERR_NO_ERROR;
	}

//#define width() aa_imgwidth(ascii_context_)
//#define height() aa_imgheight(ascii_context_)

//#define width() aa_scrwidth(ascii_context_)
//#define height() aa_scrheight(ascii_context_)

	fim_err_t AADevice::display(
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
		void* rgb = ida_image_img?((const struct ida_image*)ida_image_img)->data:FIM_NULL;// source rgb array
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
		ocskip = width(); 	//FIXME maybe this is not enough and should be commented or rewritten!

		if( orows  > height() ) return -9 -99*100;
		if( ocols  >  width() ) return -10-99*100;
		if( ocskip <  width() ) return -11-99*100;
		if( icskip<icols ) return -6-5*100;

//		I must still decide on the destiny of the following
//		orows  = min( irows-iroff, height());
//		ocols  = min( icols-icoff,  width());// rows and columns to draw in output buffer
//		ocskip = width();// output columns to skip for each line
//
		ocskip = aa_scrwidth(ascii_context_);// output columns to skip for each line
		ocskip = aa_imgwidth(ascii_context_);// output columns to skip for each line

		/*
		 * FIXME : since aa_flush() poses requirements to the way single viewports are drawn, 
		 * AADevice will behave single-windowed until I get a better understanding of aalib.
		 * Therefore these sanitizing line.
		 */
		//oroff  = ocoff = 0;
		//if(oroff)oroff=min(oroff,40);

		/* we zero the pixel field */
		//img or scr ?!
		//fim_bzero(aa_image(ascii_context_),aa_imgheight(ascii_context_)*ocskip);
		//fim_bzero(aa_image(ascii_context_),width()*height());
		AADevice::clear_rect_<FIM_CNS_BLACK>( aa_image(ascii_context_), oroff,ocoff, oroff+orows,ocoff+ocols, ocskip); 

	//	cout << iroff << " " << icoff << " " << irows << " " << icols << " " << icskip << "\n";
/*		cout << oroff << " " << ocoff << " " << orows << " " << ocols << " " << ocskip << "\n";
		cout << " aa_scrwidth(ascii_context_):" << aa_scrwidth(ascii_context_) << "  ";
		cout << "aa_scrheight(ascii_context_):" <<aa_scrheight(ascii_context_) << "\n";
		cout << " aa_imgwidth(ascii_context_):" << aa_imgwidth(ascii_context_) << "  ";
		cout << "aa_imgheight(ascii_context_):" <<aa_imgheight(ascii_context_) << "\n";
		cout << "ocskip " << ocskip << "\n";
		cout << "ocols "  << ocols  << "\n";*/
		
		if(icols<ocols) { ocoff+=(ocols-icols-1)/2; ocols-=(ocols-icols-1)/2; }
		if(irows<orows) { oroff+=(orows-irows-1)/2; orows-=(orows-irows-1)/2; }

		fim_err_t r;
#if (!FIM_AALIB_DRIVER_DEBUG)
		if((r=matrix_copy_rgb_to_gray(
				aa_image(ascii_context_),rgb,
				iroff,icoff, // row and column offset of the first input pixel
				irows,icols,// rows and columns in the input image
				icskip,	// input columns to skip for each line
				oroff,ocoff,// row and column offset of the first output pixel
				oroff+orows,ocoff+ocols,// rows and columns to draw in output buffer
				ocskip,// output columns to skip for each line
				flags
			)))
			return r;
#endif	/* !FIM_AALIB_DRIVER_DEBUG */
			//return -50;

/*		aa_putpixel(ascii_context_,ocols-1,orows-1,0xAA);
		aa_putpixel(ascii_context_,0,orows-1,0xAA);
		aa_putpixel(ascii_context_,ocols-1,0,0xAA);*/
/*		aa_putpixel(ascii_context_, aa_scrwidth(ascii_context_)+0, aa_scrheight(ascii_context_)+0, 0xAA);
		aa_putpixel(ascii_context_, aa_scrwidth(ascii_context_)+1, aa_scrheight(ascii_context_)+1, 0xAA);
		aa_putpixel(ascii_context_, aa_scrwidth(ascii_context_)+2, aa_scrheight(ascii_context_)+2, 0xAA);
		aa_putpixel(ascii_context_, aa_scrwidth(ascii_context_)+3, aa_scrheight(ascii_context_)+3, 0xAA);
		aa_putpixel(ascii_context_, aa_scrwidth(ascii_context_)+4, aa_scrheight(ascii_context_)+4, 0xAA);
		aa_putpixel(ascii_context_, aa_scrwidth(ascii_context_)+5, aa_scrheight(ascii_context_)+5, 0xAA);
		aa_putpixel(ascii_context_, aa_scrwidth(ascii_context_)+6, aa_scrheight(ascii_context_)+6, 0xAA);

		aa_putpixel(ascii_context_, aa_imgwidth(ascii_context_)-1, aa_imgheight(ascii_context_)-1, 0xAA);
		aa_putpixel(ascii_context_, aa_imgwidth(ascii_context_)-2, aa_imgheight(ascii_context_)-2, 0xAA);*/

	//	((fim_aa_char*)aa_image(ascii_context_))[]=;
		
//		std::cout << "width() : " << width << "\n"; //		std::cout << "height() : " << height << "\n";
		aa_render (ascii_context_, &aa_defrenderparams,0, 0, width() , height() );
		aa_flush(ascii_context_);
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t AADevice::reinit(const fim_char_t *rs)
	{
		if(strstr(rs,"w")!=FIM_NULL)
			allow_windowed=1;
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t AADevice::initialize(sym_keys_t &sym_keys)
	{
		aa_parseoptions (FIM_NULL, FIM_NULL, FIM_NULL, FIM_NULL);
		aainvalid=false;
		ascii_context_ = FIM_NULL;

		memcpy (&ascii_hwparms_, &aa_defparams, sizeof (struct aa_hardware_params));

		//ascii_rndparms = aa_getrenderparams();
		//aa_parseoptions (&ascii_hwparms_, ascii_rndparms, &argc, argv);

//		NOTE: if uncommenting this, remember to #ifdef HAVE_GETENV
//		fim_aa_char *e;fim_sys_int v;
//		if((e=getenv("COLUMNS"))!=FIM_NULL && (v=fim_atoi(e))>0) ascii_hwparms_.width  = v-1;
//		if((e=getenv("LINES"  ))!=FIM_NULL && (v=fim_atoi(e))>0) ascii_hwparms_.height = v-1;
//		if((e=getenv("COLUMNS"))!=FIM_NULL && (v=fim_atoi(e))>0) ascii_hwparms_.recwidth  = v;
//		if((e=getenv("LINES"  ))!=FIM_NULL && (v=fim_atoi(e))>0) ascii_hwparms_.recheight = v;

//		ascii_hwparms_.width  = 80;
//		ascii_hwparms_.height = 56;
//
//		ascii_hwparms_.width  = 128-1;
//		ascii_hwparms_.height = 48 -1;

		/*ascii_hwparms_.width()  = 4;
		ascii_hwparms_.height() = 4;*/
		
		name_[0] = FIM_SYM_CHAR_NUL;
		name_[1] = FIM_SYM_CHAR_NUL;
		if(allow_windowed==0)
			setenv(FIM_ENV_DISPLAY,"",1); /* running fim -o aalib in a window may render fim unusable */
		ascii_save_.name = (fim_aa_char*)name_;
		ascii_save_.format = &aa_text_format;
		ascii_save_.file = FIM_NULL;
//		ascii_context_ = aa_init (&save_d, &ascii_hwparms_, &ascii_save_);
		ascii_context_ = aa_autoinit (&ascii_hwparms_);
		if(!ascii_context_)
		{
			std::cout << "problem initializing aalib!\n";
			return FIM_ERR_GENERIC;
		}
		if(!aa_autoinitkbd(ascii_context_, 0))
		{
			std::cout << "problem initializing aalib keyboard!\n";
			return FIM_ERR_GENERIC;
		}
		/*if(!aa_autoinitmouse(ascii_context_, 0))
		{
			std::cout << "problem initializing aalib mouse!\n";
			return FIM_ERR_GENERIC;
		}*/
		aa_hidecursor (ascii_context_);

		/* won't help in the ill X11-windowed aalib shipped with many distros case */
		dktorlkm_[AA_LEFT]=FIM_KKE_LEFT;
		dktorlkm_[AA_RIGHT]=FIM_KKE_RIGHT;
		dktorlkm_[AA_UP]=FIM_KKE_UP;
		dktorlkm_[AA_DOWN]=FIM_KKE_DOWN;
		dktorlkm_[AA_ESC]=FIM_KKE_ESC;
		const int aa_f0 = 65870 - 1;
		dktorlkm_[aa_f0+ 1]=FIM_KKE_F1;
		dktorlkm_[aa_f0+ 2]=FIM_KKE_F2;
		dktorlkm_[aa_f0+ 3]=FIM_KKE_F3;
		dktorlkm_[aa_f0+ 4]=FIM_KKE_F4;
		dktorlkm_[aa_f0+ 5]=FIM_KKE_F5;
		dktorlkm_[aa_f0+ 6]=FIM_KKE_F6;
		dktorlkm_[aa_f0+ 7]=FIM_KKE_F7;
		dktorlkm_[aa_f0+ 8]=FIM_KKE_F8;
		dktorlkm_[aa_f0+ 9]=FIM_KKE_F9;
		dktorlkm_[aa_f0+10]=FIM_KKE_F10;
		dktorlkm_[aa_f0+11]=FIM_KKE_F11;
		dktorlkm_[aa_f0+12]=FIM_KKE_F12;

	/*	The mulx and muly are not reliable */
#if 0
		if(ascii_context_->mulx>0 && ascii_context_->muly>0)
			cc.setVariable("aascale",FIM_INT_SCALE_FRAC(ascii_context_->muly,static_cast<fim_scale_t>(ascii_context_->mulx)));
		cc.setVariable("aamuly",(ascii_context_->muly));
		cc.setVariable("aamulx",(ascii_context_->mulx));
#endif
		return FIM_ERR_NO_ERROR;
	}

	void AADevice::finalize(void)
	{
		finalized_=true;
		aa_close(ascii_context_);
	}
	fim_coo_t AADevice::get_chars_per_line(void)const{return aa_scrwidth(ascii_context_);}
	fim_coo_t AADevice::get_chars_per_column(void)const{return aa_scrheight(ascii_context_);}
	fim_coo_t AADevice::txt_width(void)const { return aa_scrwidth(ascii_context_ ) ;}
	fim_coo_t AADevice::txt_height(void)const{ return aa_scrheight(ascii_context_) ;}
	fim_coo_t AADevice::width(void)const { return aa_imgwidth(ascii_context_ ) ;}
	fim_coo_t AADevice::height(void)const{ return aa_imgheight(ascii_context_) ;}
#if 0
	fim_err_t AADevice::format_console(void)
	{
#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
		//mc_.setRows ( -height()/2);
		mc_.setRows ( get_chars_per_column()/2 );
		mc_.reformat(  txt_width()   );
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
		return FIM_ERR_NO_ERROR;
	}
#endif
	fim_err_t AADevice::fs_puts(struct fs_font *f, fim_coo_t x, fim_coo_t y, const fim_char_t *str)
	{
#if (!FIM_AALIB_DRIVER_DEBUG)
		aa_puts(ascii_context_,x,y,
			//AA_REVERSE,
			AA_NORMAL,
			//AA_SPECIAL,
			(const fim_aa_char*)str);
#endif /* FIM_AALIB_DRIVER_DEBUG */
		return FIM_ERR_NO_ERROR;
	}

	void AADevice::flush(void)
	{
		aa_flush(ascii_context_);
	}

	fim_err_t AADevice::clear_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1, fim_coo_t y2)
	{
		/* FIXME : only if initialized !
		 * TODO : define the exact conditions to use this member function
		 * */
		
		return clear_rect_<FIM_CNS_BLACK>(aa_image(ascii_context_),y1, x1, y2-y1+1, x2-x1+1,aa_imgwidth(ascii_context_));
	}

	fim_err_t AADevice::fill_rect(fim_coo_t x1, fim_coo_t x2, fim_coo_t y1,fim_coo_t y2, fim_color_t color)
	{
		if( color == FIM_CNS_WHITE )
			return clear_rect_<FIM_CNS_WHITE>(aa_image(ascii_context_),y1, x1, y2-y1+1, x2-x1+1,aa_imgwidth(ascii_context_));
		else
			return clear_rect_<FIM_CNS_BLACK>(aa_image(ascii_context_),y1, x1, y2-y1+1, x2-x1+1,aa_imgwidth(ascii_context_));
	}

	fim_err_t AADevice::status_line(const fim_char_t *msg)
	{
		const fim_coo_t th=txt_height();
		if(th<1)
			goto err;
#if (!FIM_AALIB_DRIVER_DEBUG)
		aa_printf(ascii_context_,0,th-1,AA_NORMAL,"%s",msg);
#endif /* FIM_AALIB_DRIVER_DEBUG */
		aa_flush(ascii_context_);
err:
		return FIM_ERR_NO_ERROR;
	}

	AADevice::~AADevice(void)
	{
		/* seems like in some cases some aa stuff doesn't get freed. is this possible ? */
		if(!finalized_)
			finalize();
	}

	static aa_context *fim_aa_ascii_context; /* This was a global variable just for the use in fim_aa_get_input. Now one it's of no use.  */
	static fim_sys_int fim_aa_get_input(fim_key_t * c, bool want_poll, bool * srp)
	{
		fim_key_t e = 0;

		if(srp)
		       	*srp = true;
		if(!c)
			return 0;
		*c = FIM_SYM_NULL_KEY;
		*c = aa_getevent(fim_aa_ascii_context,0);/* 1 for blocking wait */
		if(*c==AA_UNKNOWN)
			*c=0;
		if(*c)
			std::cout << "";/* FIXME : removing this breaks things. console-related problem, I guess */
		if(!*c)
			return 0;

		switch (*c)
		{
			case (AA_UP   ):     e=FIM_KKE_UP; break;
			case (AA_DOWN ):     e=FIM_KKE_DOWN;break;
			case (AA_LEFT ):     e=FIM_KKE_LEFT;break;
			case (AA_RIGHT):     e=FIM_KKE_RIGHT;break;
			case (AA_BACKSPACE): e=FIM_KKE_BACKSPACE;break;

			/* Note: these five bindings work only under X11 .. */
			case (65765): e=FIM_KKE_PAGE_UP;break;
			case (65766): e=FIM_KKE_PAGE_DOWN;break;
			case (65779): e=FIM_KKE_INSERT;break;
			case (65760): e=FIM_KKE_HOME;break;
			case (65767): e=FIM_KKE_END;break;

			case (AA_ESC): e=FIM_KKE_ESC; break;
		}
		if (e)
		{
			*c = e;
			return 1;
		}
		if(srp)
			*srp = false;
		return 0;
	}

	fim_sys_int AADevice::get_input(fim_key_t * c, bool want_poll)
	{
		fim_sys_int rc = 0;
		bool sr = false;

		fim_aa_ascii_context = ascii_context_;
		rc = fim_aa_get_input(c, want_poll, &sr);

		if(sr)
			return rc;

		rc = 1;
		switch (*c)
		{
			case (65907):
				status_line((const fim_char_t*)"control key not yet supported in aa. sorry!");
				/* left ctrl (arbitrary) */
			break;
			case (65908):
				status_line((const fim_char_t*)"control key not yet supported in aa. sorry!");
				/* right ctrl (arbitrary) */
			break;
			case (65909):
				status_line((const fim_char_t*)"lock key not yet supported in aa. sorry!");
				/* right ctrl (arbitrary) */
			break;
			case (65906):
				status_line((const fim_char_t*)"shift key not yet supported in aa. sorry!");
				/* shift (arbitrary) */
			break;
			case (AA_MOUSE):
				status_line((const fim_char_t *)"mouse events not yet supported in aa. sorry!");
				/* */
			break;
			case (AA_RESIZE):
				//status_line((const fim_char_t *)"window resizing not yet supported. sorry!");
				cc.display_resize(0,0);
				/*aa_resize(ascii_context_);*//*we are not yet ready : the FimWindow and Viewport stuff .. */
				rc = 0;
				/* */
			break;
		}

		if (dktorlkm_.find(*c)!=dktorlkm_.end())
			*c = dktorlkm_[*c];

		//std::cout << "event : " << *c << "\n";
		//if(*c<0x80) return 1;
		return rc;
	}

	fim_err_t AADevice::resize(fim_coo_t w, fim_coo_t h)
	{
		/* resize is handled by aalib */
		FIM_CONSTEXPR bool want_resize_=true;
#if AA_LIB_VERSIONCODE>=104000
		/* aalib version 104000 calls exit(-1) on zero-width/height contexts. this is simply stupid.
		   the code we have here is not completely clean, but it catches the situation and proposes
 		   a fallback solution, by reinitializing the library video mode to reasonable defaults. */
		{
			fim_coo_t width=0, height=0;
			ascii_context_->driver->getsize(ascii_context_, &width, &height);
			if (width < FIM_AA_MINWIDTH || height < FIM_AA_MINHEIGHT )
			{
				/* this is a fix to avoid a segfault in aalib following to-zero window resize */
				aa_close(ascii_context_);
				memcpy (&ascii_hwparms_, &aa_defparams, sizeof (struct aa_hardware_params));
				ascii_context_ = aa_autoinit (&ascii_hwparms_);
				if(!aa_autoinitkbd(ascii_context_, 0))
					;// error handling / reporting missing
				return FIM_ERR_NO_ERROR;
			}
		}
#else /* AA_LIB_VERSIONCODE */
		/* FIXME: shall track back the first evil aalib version */
#endif /* AA_LIB_VERSIONCODE */
		if(!want_resize_)
			return FIM_ERR_GENERIC;

		if(1==aa_resize(ascii_context_))
		{
			cc.setVariable(FIM_VID_SCREEN_WIDTH, width() );
			cc.setVariable(FIM_VID_SCREEN_HEIGHT,height());
			return FIM_ERR_NO_ERROR;
		}
		return FIM_ERR_GENERIC;
	}

	fim_coo_t AADevice::status_line_height(void)const
	{
		return FIM_AALIB_FONT_HEIGHT;
	}

	fim_coo_t AADevice::font_height(void)const
	{
		return FIM_AALIB_FONT_HEIGHT;
	}

#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
	AADevice::AADevice(MiniConsole& mc_, fim::string opts ):DisplayDevice(mc_),
#else /* FIM_WANT_NO_OUTPUT_CONSOLE */
	AADevice( fim::string opts ):DisplayDevice(),
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
	allow_windowed(0)
	{
		this->reinit(opts.c_str());
	}

#endif /* FIM_WITH_AALIB */
