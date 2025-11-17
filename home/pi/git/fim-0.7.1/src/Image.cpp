/* $LastChangedDate: 2024-03-23 12:22:42 +0100 (Sat, 23 Mar 2024) $ */
/*
 Image.cpp : Image manipulation and display

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

//#include "Image.h"
#include "fim.h"

#if FIM_WANT_EXIFTOOL
#include "ExifTool.h"
#endif /* FIM_WANT_EXIFTOOL */

#define FIM_IMAGE_INSPECT 0
#if FIM_IMAGE_INSPECT
#define FIM_PR(X) printf("IMAGE:%c:%20s: f:%d/%d p:%d/%d %s\n",X,__func__,(int)getGlobalIntVariable(FIM_VID_FILEINDEX),(int)getGlobalIntVariable(FIM_VID_FILELISTLEN),(int)getGlobalIntVariable(FIM_VID_PAGE),(int)getIntVariable(FIM_VID_PAGES),(cacheable()?"cacheable":"uncacheable"));
#else /* FIM_IMAGE_INSPECT */
#define FIM_PR(X) 
#endif /* FIM_IMAGE_INSPECT */

#define FIM_WANT_ASCALE_FRIENDLY_ROTATION 1

#if FIM_CACHE_DEBUG
#if FIM_WANT_PIC_CMTS
	std::ostream& operator<<(std::ostream& os, const ImgDscs & id)
	{
		return id.print(os);
	}
#endif /* FIM_WANT_PIC_CMTS */
#endif /* FIM_CACHE_DEBUG */

namespace fim
{
	static void fim_desaturate_rgb(fim_byte_t * data, fim_pxc_t howmany)
	{
		FIM_REGISTER int avg;
		for( fim_byte_t * p = data; p < data + howmany ;p+=3)
		{ avg=p[0]+p[1]+p[2]; p[0]=p[1]=p[2]=(fim_byte_t) (avg/3); }
	}

	static void fim_simulate_cvd(fim_byte_t * data, fim_pxc_t howmany, enum fim_cvd_t cvd, bool daltonize)
	{
		/* Based on formulas from from http://www.daltonize.org */
		fim_pif_t l,m,s; // long medium short [cones]
		fim_pif_t dl,dm,ds; // long medium short [cones]
		fim_pif_t er,eg,eb; // red green blue
		fim_byte_t r,g,b; // red green blue
		fim_byte_t dr,dg,db; // red green blue

#define FIM_PD3_T(RC,R,GC,G,BC,B,T)	\
	((RC*(T)R)+(GC*(T)G)+(BC*(T)B))

#define FIM_RGB_TRIM(V) FIM_MIN(FIM_MAX(0.0,V),255.0)

#define FIM_PD3(RC,R,GC,G,BC,B)		\
	(FIM_PD3_T(RC,R,GC,G,BC,B,fim_pif_t))

#define FIM_CPS(V1,V2) (((fim_pif_t)(V1))-((fim_pif_t)(V2)))

		for( fim_byte_t * p = data; p < data + howmany ;p+=3)
		{
			r=p[0];
			g=p[1];
			b=p[2];

			dl=l=FIM_PD3(17.8824  ,r, 43.5161,g, 4.11935,b),
			dm=m=FIM_PD3(3.45565  ,r, 27.1554,g, 3.86714,b),
			ds=s=FIM_PD3(0.0299566,r,0.184309,g, 1.46709,b);

			if(cvd == FIM_CVD_PROTANOPIA )
				dl=FIM_PD3(0.0      ,l,2.02344 ,m,-2.52581,s),
				dm=FIM_PD3(0.0      ,l,1.0     ,m, 0.0    ,s),
				ds=FIM_PD3(0.0      ,l,0.0     ,m, 1.0    ,s);
			if(cvd == FIM_CVD_DEUTERANOPIA)
				dl=FIM_PD3(1.0      ,l,0.0,m,0.0     ,s),
				dm=FIM_PD3(0.494207 ,l,0.0,m,1.24827 ,s),
				ds=FIM_PD3(0.0      ,l,0.0,m,1.0     ,s);
			if(cvd == FIM_CVD_TRITANOPIA)
				dl=FIM_PD3(1.0      ,l,0.0     ,m,0.0     ,s),
				dm=FIM_PD3(0.0      ,l,1.0     ,m,0.0     ,s),
				ds=FIM_PD3(-0.395913,l,0.801109,m,0.0     ,s);

			dr=FIM_RGB_TRIM(FIM_PD3( 0.080944   ,dl,-0.130504  ,dm, 0.116721,ds)),
			dg=FIM_RGB_TRIM(FIM_PD3(-0.0102485  ,dl, 0.0540194 ,dm,-0.113614,ds)),
			db=FIM_RGB_TRIM(FIM_PD3(-0.000365294,dl,-0.00412163,dm, 0.693513,ds));

			if(daltonize)
			{
				er=FIM_CPS(r,dr);
				eg=FIM_CPS(g,dg);
				eb=FIM_CPS(b,db);
				dr+=FIM_RGB_TRIM(FIM_PD3(0.0      ,er,0.0     ,eg,0.0     ,eb));
				dg+=FIM_RGB_TRIM(FIM_PD3(0.7      ,er,1.0     ,eg,0.0     ,eb));
				db+=FIM_RGB_TRIM(FIM_PD3(0.7      ,er,0.0     ,eg,1.0     ,eb));
			}

			p[0]=dr;
			p[1]=dg;
			p[2]=db;
		}
#undef FIM_PD3
#undef FIM_PD3_T
#undef FIM_CPS
#undef FIM_RGB_TRIM
	}

	static bool fim_generate_24bit_identity(fim_byte_t * data, fim_pxc_t howmany)
	{
		int val=0x0;

		for( fim_byte_t * p = data; p < data + howmany ;p+=3)
		{
			p[2] = (fim_byte_t )( ( val & 0x000000FF ) >>  0 );//b
			p[1] = (fim_byte_t )( ( val & 0x0000FF00 ) >>  8 );
			p[0] = (fim_byte_t )( ( val & 0x00FF0000 ) >> 16 );//r
			++val;
		}
		return true;
	}

	static void fim_negate_rgb(fim_byte_t * data, fim_pxc_t howmany)
	{
		//This is just an example; in C++17 (not yet in compiler when writing this) we shall be able to have:
		//#include <execution>
		//std::for_each(std::execution::parallel_policy,data,data+howmany,[](fim_byte_t & b){b = ~ b;});
		std::for_each(data,data+howmany,[](fim_byte_t & b){b = ~ b;});
	}

#if FIM_WANT_IMG_SHRED 
	static void fim_shred(fim_byte_t * data, fim_pxc_t howmany)
	{
		fim_desaturate_rgb(data, howmany);
		fim_negate_rgb(    data, howmany);
	}
#endif /* FIM_WANT_IMG_SHRED  */

	fim_coo_t Image::original_width(void)const
	{
		fim_coo_t ow;
		assert(fimg_);
		if(orientation_%2)
		       	ow = fimg_->i.height;
		else
			ow = fimg_->i.width;
		return ow;
	}

	fim_coo_t Image::original_height(void)const
	{
		fim_coo_t ow;
		assert(fimg_);
		if(orientation_%2)
		       	ow = fimg_->i.width;
		else
			ow = fimg_->i.height;
		return ow;
	}

	int Image::width(void)const
	{
		assert(img_);
		return img_->i.width;
	}

	int Image::height(void)const
	{
		assert(img_);
		return img_->i.height;
	}

	void Image::desc_update(void)
	{
#if FIM_WANT_PIC_CMTS
		fim_fn_t key(fname_.c_str());
		if(cc.id_.find(key) != cc.id_.end() )
		{
			setVariable(FIM_VID_COMMENT,(cc.id_[key]));
		}
		else
		{
			key = fim_fn_t (fim_basename_of(fname_));
			if(cc.id_.find(key) != cc.id_.end() )
				setVariable(FIM_VID_COMMENT,(cc.id_[key]));
		}
#if FIM_WANT_PIC_LVDN
		assign_ns(cc.id_.vd_[key]);
#endif /* FIM_WANT_PIC_LVDN */
#endif /* FIM_WANT_PIC_CMTS */
	}

bool Image::fetchExifToolInfo(const fim_char_t *fname)
{
#if FIM_WANT_EXIFTOOL
	fim_int ue = getGlobalIntVariable(FIM_VID_EXIFTOOL);
	/* one might execute this code in a background thread */
	std::ostringstream oss;

	if ( ExifTool *et = new ExifTool )
	{
		if ( TagInfo *info = et->ImageInfo(fname,FIM_NULL,2) )
      	 	{
        		for (TagInfo *i=info; i; i=i->next)
		       	{
				oss << i->name << " = " << i->value << ";" << "\n";
				//std::cout << "reading " << i->name << "...\n";
        		}
        		delete info;
   	 	}
	       	else if (et->LastComplete() <= 0)
	       	{
			std::cerr << "Error executing exiftool!" << std::endl;
   	 	}
    		char *err = et->GetError();
	    	if (err)
		       	std::cerr << err;
		delete et;      // delete our ExifTool object
		//std::cout << "setting: " << oss.str() << "\n",
		if(ue == 1)
			setVariable(FIM_VID_COMMENT,getVariable(FIM_VID_COMMENT)+oss.str());
		if(ue == 2)
			setVariable(FIM_VID_EXIFTOOL_COMMENT,oss.str());
		return true;
	}
#endif /* FIM_WANT_EXIFTOOL */
	return false;
}

	Image::Image(const fim_char_t *fname, FILE*fd, fim_page_t page):
#ifdef FIM_NAMESPACES
		Namespace(&cc,FIM_SYM_NAMESPACE_IMAGE_CHAR),
#endif /* FIM_NAMESPACES */
		scale_(0.0),
		ascale_(0.0),
		newscale_(0.0),
		angle_(0.0),
		newangle_(0.0),
		page_(0),
                img_     (FIM_NULL),
                fimg_    (FIM_NULL),
		orientation_(FIM_NO_ROT),
		fis_(fim::string(fname)==fim::string(FIM_STDIN_IMAGE_NAME)?FIM_E_STDIN:FIM_E_FILE),
                fname_     (FIM_CNS_DEFAULT_IFNAME),
		fs_(0)

	{
		reset();
		if( !load(fname,fd,page) || !check_valid() || (!fimg_) ) 
		{
			FIM_PR('e');
			// FIXME: sometimes load() intentionally skips a file. an appropriate message shall be printed out
			cout << "warning: invalid loading "<<fname<<" ! \n";
			if( getGlobalIntVariable(FIM_VID_DISPLAY_STATUS_BAR)||getGlobalIntVariable(FIM_VID_DISPLAY_BUSY))
				cc.set_status_bar( fim::string("error while loading \"")+ fim::string(fname)+ fim::string("\"") , "*");
			throw FimException();
		}
		else
		{
			FIM_PR(' ');
#if FIM_WANT_PIC_CMTS
			/* Picture commentary. user-set overrides the file's own. */
			struct ida_extra* ie=load_find_extra(&(img_->i),EXTRA_COMMENT);

			if(ie)
				setVariable(FIM_VID_COMMENT,(fim_char_t*)(ie->data));

			if(fname)
				desc_update();
#endif /* FIM_WANT_PIC_CMTS */

#if FIM_WANT_EXIFTOOL
			if(fname && getGlobalIntVariable(FIM_VID_EXIFTOOL) != 0)
				fetchExifToolInfo(fname);
#endif /* FIM_WANT_EXIFTOOL */
		}
	}

	void Image::reset(void)
	{
                fimg_    = FIM_NULL;
                img_     = FIM_NULL;
		reset_view_props();
		if( getGlobalIntVariable(FIM_VID_AUTOTOP ) )
			setVariable(FIM_VID_AUTOTOP,getGlobalIntVariable(FIM_VID_AUTOTOP));
	}

	void Image::reset_view_props(void)
	{
                scale_   = FIM_CNS_SCALE_DEFAULT;
                newscale_= FIM_CNS_SCALE_DEFAULT;
                ascale_  = FIM_CNS_SCALE_DEFAULT;
                angle_   = FIM_CNS_ANGLE_DEFAULT;
                orientation_=FIM_NO_ROT;

		setVariable(FIM_VID_SCALE  ,newscale_*100);
		setVariable(FIM_VID_ASCALE ,ascale_);
		setVariable(FIM_VID_ANGLE  ,angle_);
		setVariable(FIM_VID_NEGATED , 0);
		setVariable(FIM_VID_DESATURATED, 0);
		setVariable(FIM_VID_ORIENTATION, FIM_NO_ROT);
	}

	void Image::set_exif_extra(fim_int shouldrotate, fim_int shouldmirror, fim_int shouldflip)
	{
		if(shouldrotate)
			setVariable(FIM_VID_EXIF_ORIENTATION,shouldrotate);
		if(shouldmirror)
			setVariable(FIM_VID_EXIF_MIRRORED,1);
		if(shouldflip)
			setVariable(FIM_VID_EXIF_FLIPPED,1);
	}
	
static void ers(const char*value, Image * image)
{
		// EXIF orientation value can be of the form "X - Y", with X and Y in
		// {top,bottom,left,right}
		//
		// from http://sylvana.net/jpegcrop/exif_orientation.html 
		// we got the following combinations:
		// Value	0th Row	0th Column
		// 1	top	left side
		// 2	top	right side
		// 3	bottom	right side
		// 4	bottom	left side
		// 5	left side	top
		// 6	right side	top
		// 7	right side	bottom
		// 8	left side	bottom
		//
		// neatly depicted in an F letter example:
		//
		//   1        2       3      4         5            6           7          8
		//
		//   888888  888888      88  88      8888888888  88                  88  8888888888
		//   88          88      88  88      88  88      88  88          88  88      88  88
		//   8888      8888    8888  8888    88          8888888888  8888888888          88
		//   88          88      88  88
		//   88          88  888888  888888
		//
		// note that (in this order):
		// 2,3,5,7 want a mirror transformation
		// 4,3 want a flip transformation
		// 7,8 want a cw rotation
		// 5,6 want a ccw rotation
		//
		bool shouldmirror,shouldflip;
		fim_int shouldrotate = 0;
	       	fim_char_t r,c;
		const fim_char_t *p = FIM_NULL;
		fim_char_t f;

		if(!value || FIM_NULL == strchr(value,'-'))
			goto uhmpf;

		p = strchr(value,'-')+1;
		r=tolower(value[0]);
		c=tolower(p[0]);
		switch(r)
		{
			case 't':
			switch(c){
				case 'l':f=1; break;
				case 'r':f=2; break;
				default: f=0;
			} break;
			case 'b':
			switch(c){
				case 'r':f=3; break;
				case 'l':f=4; break;
				default: f=0;
			} break;
			case 'l':
			switch(c){
				case 't':f=5; break;
				case 'b':f=8; break;
				default: f=0;
			} break;
			case 'r':
			switch(c){
				case 't':f=6; break;
				case 'b':f=7; break;
				default: f=0;
			} break;
			default: f=0;
		}
		if(f==0)
			goto uhmpf;
		shouldmirror=(f==2 || f==3 || f==5 || f==7);
		shouldflip=(f==4 || f==3);
		if (f==5 || f==6) shouldrotate = Image::FIM_ROT_R; // cw
		if (f==7 || f==8) shouldrotate = Image::FIM_ROT_L; // ccw
		//std::cout << "EXIF_TAG_ORIENTATION FOUND !\n",
		//std::cout << "VALUE: " <<(int)f << r<< c<<
		//shouldmirror << shouldrotate << shouldflip,
		//std::cout << "\n";
		if(shouldmirror && shouldflip && !shouldrotate)
			shouldmirror = false,
			shouldflip = false,
			shouldrotate = Image::FIM_ROT_U;
		image->set_exif_extra(shouldrotate, shouldmirror, shouldflip);
uhmpf:
		return;
	}

	bool Image::load(const fim_char_t *fname, FILE* fd, int want_page)
	{
		/*
		 *	an image is loaded and initializes this image.
		 *	returns false if the image does not load
		 */
		bool retval = false;
#if FIM_WANT_IMAGE_LOAD_TIME
    		fim_fms_t dt=getmilliseconds();
#endif /* FIM_WANT_IMAGE_LOAD_TIME */

		FIM_PR('*');
		if(fname==FIM_NULL && fname_==FIM_CNS_EMPTY_STRING)
		{
			FIM_PR('e');
			goto ret;//no loading = no state change
		}
		this->free();
		fname_=fname;
		if( getGlobalIntVariable(FIM_VID_DISPLAY_STATUS_BAR)||getGlobalIntVariable(FIM_VID_DISPLAY_BUSY))
		{
			if( getGlobalIntVariable(FIM_VID_WANT_PREFETCH) == 1)
				cc.set_status_bar("please wait while prefetching...", "*");
			else
				cc.set_status_bar("please wait while reloading...", "*");
		}

		fimg_ = FbiStuff::read_image(fname,fd,want_page,this);
#if 0
		if(fimg_)
		{
			// fim_free(fimg_->data);
			/* Such dimensions break SDL */
    			fimg_->i.width = 100*1000*1000;
	       		fimg_->i.height = 1;
    			fimg_->data = fim_pm_alloc(fimg_->i.width, fimg_->i.height);
			for(int i=0;i<fimg_->i.width*fimg_->i.height;++i)
    				fimg_->data[i*3+0]=i%256,
    				fimg_->data[i*3+1]=i%256,
    				fimg_->data[i*3+2]=i%256;
		}
#endif

#if FIM_WANT_MIPMAPS
    		if(fimg_)
			mm_make();
#endif /* FIM_WANT_MIPMAPS */

    		if(strcmp(FIM_STDIN_IMAGE_NAME,fname)==0)
		{
			fis_ = FIM_E_STDIN; // yes, it seems redundant but it is necessary
		}
		else 
		{
#if FIM_WANT_KEEP_FILESIZE
#if HAVE_SYS_STAT_H
			struct stat stat_s;
			if(-1!=stat(fname,&stat_s))
			{
				fs_=stat_s.st_size;
			}
#endif /* HAVE_SYS_STAT_H */
#endif /* FIM_WANT_KEEP_FILESIZE */
		}

		img_=fimg_;	/* no scaling: one copy only */
		should_redraw();

		if(! img_)
		{
			FIM_PR('!');
			cout<<"warning: image loading error!\n"   ;
			goto ret;
		}
		else
		{
		       	page_=want_page;
		}

#ifdef FIM_NAMESPACES
#if FIM_WANT_IMAGE_LOAD_TIME
		setVariable(FIM_VID_IMAGE_LOAD_TIME,(fim_float_t)((getmilliseconds()-dt)/1000.0));
#endif /* FIM_WANT_IMAGE_LOAD_TIME */
		setVariable(FIM_VID_PAGE, page_);
		//setVariable(FIM_VID_LASTPAGEINDEX, getIntVariable(FIM_VID_PAGE));
		setVariable(FIM_VID_PAGES  ,fimg_->i.npages);
		setVariable(FIM_VID_HEIGHT ,fimg_->i.height);
		setVariable(FIM_VID_WIDTH ,fimg_->i.width );
		setVariable(FIM_VID_SHEIGHT, img_->i.height);
		setVariable(FIM_VID_SWIDTH, img_->i.width );
		setVariable(FIM_VID_FIM_BPP, getGlobalIntVariable(FIM_VID_FIM_BPP) );
		setVariable(FIM_VID_FILENAME,fname_);

		setVariable(FIM_VID_SCALE  ,newscale_*100);
		setVariable(FIM_VID_ASCALE,ascale_);
		setVariable(FIM_VID_ANGLE , angle_);
		setVariable(FIM_VID_NEGATED , 0);
		setVariable(FIM_VID_DESATURATED, 0);
		setVariable(FIM_VID_ORIENTATION, FIM_NO_ROT);
#endif /* FIM_NAMESPACES */

		setGlobalVariable(FIM_VID_HEIGHT ,fimg_->i.height);
		setGlobalVariable(FIM_VID_WIDTH  ,fimg_->i.width );
		setGlobalVariable(FIM_VID_SHEIGHT, img_->i.height);
		setGlobalVariable(FIM_VID_SWIDTH , img_->i.width );
		//setGlobalVariable(FIM_VID_SCALE  ,newscale_*100);
		//setGlobalVariable(FIM_VID_ASCALE ,ascale_);
	
		if( getGlobalIntVariable(FIM_VID_DISPLAY_STATUS_BAR)||getGlobalIntVariable(FIM_VID_DISPLAY_BUSY))
			cc.browser_.display_status(cc.browser_.current().c_str()); /* FIXME: an ugly way to force the proper status display */
		if(isSetVar("EXIF_Orientation"))
			ers(getStringVariable("EXIF_Orientation").c_str(),this);

		FIM_PR('.');
		retval = true;
ret:
		return retval;
	}

	Image::~Image(void)
	{
		FIM_PR('*');
#ifdef FIM_CACHE_DEBUG
		std::cout << "freeing Image " << this << "\n";
#endif /* FIM_CACHE_DEBUG */
		this->free();
		FIM_PR('.');
	}

        bool Image::is_tiny(void)const
	{
		if(!img_)
			return true;
	       	return ( img_->i.width<=1 || img_->i.height<=1 )?true:false;
	}

	fim_err_t Image::scale_multiply(fim_scale_t sm)
	{
		if(scale_*sm>0.0)
			newscale_=scale_*sm;
		return do_scale_rotate();
	}

	fim_err_t Image::set_scale(fim_scale_t ns)
	{
		newscale_=ns;
		return do_scale_rotate();
	}


        bool Image::check_valid(void)const
        {
		if(!img_)
			img_ = fimg_;
                if(!img_)
                        return false;
		else
			return true;
        }

        void Image::free(void)
        {
		FIM_PR('*');
#if FIM_WANT_IMG_SHRED 
        	if(FIM_WANT_IMG_SHRED ) /* activate this only for debug purposes */
        		this->shred();
#endif /* FIM_WANT_IMG_SHRED  */
                if(fimg_!=img_ && img_ )
		       	FbiStuff::free_image(img_ );
                if(fimg_     )
		       	FbiStuff::free_image(fimg_);
#if FIM_WANT_MIPMAPS
		mm_free();
#endif /* FIM_WANT_MIPMAPS */
                reset();
		FIM_PR('.');
        }

#if FIM_WANT_IMG_SHRED 
        void Image::shred(void)
        {
		FIM_PR('*');
                if(fimg_!=img_ && img_ )
		{
			fim_shred(img_->data, fbi_img_pixel_bytes(img_));
		}
                if(fimg_     )
		{
		       	FbiStuff::free_image(fimg_);
		}
		FIM_PR('.');
        }
#endif /* FIM_WANT_IMG_SHRED  */

#if FIM_WANT_CROP
	fim_err_t Image::do_crop(const ida_rect prect)
	{
		struct ida_image * img = fimg_;
		const int x1 = FIM_INT_PCNT(prect.x1, img->i.width);
		const int y1 = FIM_INT_PCNT(prect.y1, img->i.height);
		const int x2 = FIM_INT_PCNT(prect.x2, img->i.width-1);
		const int y2 = FIM_INT_PCNT(prect.y2, img->i.height-1);

		if ( x2 > x1 && y2 > y1 )
		if ( x1 >= 0 && x2 < (int) img->i.width )
		if ( y1 >= 0 && y2 < (int) img->i.height )
		{
			struct ida_image *rb = NULL;
			const ida_rect rect { x1, y1, x2, y2 };
			rb  = FbiStuff::crop_image(img,rect);
			if(rb)
			{
				img = rb;
				setVariable(FIM_VID_HEIGHT ,img->i.height);
				setVariable(FIM_VID_WIDTH  ,img->i.width );
				setVariable(FIM_VID_SHEIGHT,img->i.height);
				setVariable(FIM_VID_SWIDTH ,img->i.width );
				FbiStuff::free_image(fimg_);
				if( img_ != fimg_ )
				       	FbiStuff::free_image(img_);
				fimg_ = img_ = img;
				mm_make();
		       		should_redraw();
			}
		}
		return FIM_ERR_NO_ERROR;
	}
#endif /* FIM_WANT_CROP */

	fim_err_t Image::do_rotate( void )
	{
		if( img_ && ( orientation_==FIM_ROT_L || orientation_ == FIM_ROT_R ))
		{
			// we make a backup.. who knows!
			// FIXME: should use a faster and memory-smarter member function: in-place
			struct ida_image *rb=img_;
			rb  = FbiStuff::rotate_image90(rb,orientation_==FIM_ROT_L?FIM_I_ROT_L:FIM_I_ROT_R);
			if(rb)
			{
				FbiStuff::free_image(img_);
				img_=rb;
			}
		}
		if( img_ && orientation_ == FIM_ROT_U)
		{	
			// we make a backup.. who knows!
			struct ida_image *rbb=FIM_NULL,*rb=FIM_NULL;
			// FIXME: should use a faster and memory-smarter member function: in-place
			rb  = FbiStuff::rotate_image90(img_,FIM_I_ROT_L);
			if(rb)
				rbb  = FbiStuff::rotate_image90(rb,FIM_I_ROT_L);
			if(rbb)
			{
				FbiStuff::free_image(img_);
				FbiStuff::free_image(rb);
				img_=rbb;
			}
			else
			{
				if(rbb)
					FbiStuff::free_image(rbb);
				if(rb )
					FbiStuff::free_image(rb);
			}
		}

		/* we rotate only in case there is the need to do so */
		if( img_ && ( angle_ != newangle_ || newangle_) )
		{	
			// we make a backup.. who knows!
			struct ida_image *rbb=FIM_NULL,*rb=FIM_NULL;
			rb  = FbiStuff::rotate_image(img_,newangle_);
			if(rb)
				rbb  = FbiStuff::rotate_image(rb,0);
			if(rbb)
			{
				FbiStuff::free_image(img_);
				FbiStuff::free_image(rb);
				img_=rbb;
			}
			else
			{
				if(rbb)
					FbiStuff::free_image(rbb);
				if(rb )
					FbiStuff::free_image(rb);
			}
		}
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t Image::do_scale_rotate( fim_scale_t ns )
	{
		/*
		 * effective image rescaling
		 * */
		fim_pgor_t neworientation;
		fim_angle_t	gascale;
		fim_scale_t	newascale;
		fim_angle_t	gangle;

		if(ns>0.0)
			newscale_=ns;//patch

		if( !check_valid() )
			goto err;
		if(is_tiny() && newscale_<scale_)
		{
			newscale_=scale_;
			goto ret;
		}

		neworientation=getOrientation();
		gascale=getGlobalFloatVariable(FIM_VID_ASCALE);
		newascale=getFloatVariable(FIM_VID_ASCALE);
		newascale=(newascale>0.0 && newascale!=1.0)?newascale:((gascale>0.0 && gascale!=1.0)?gascale:1.0);
		
		//float newascale=getFloatVariable(FIM_VID_ASCALE); if(newascale<=0.0) newascale=1.0;
		/*
		 * The global angle_ variable value will override the local if not 0 and the local unset
		 * */
		gangle  =getGlobalFloatVariable(FIM_VID_ANGLE),
			newangle_=getFloatVariable(FIM_VID_ANGLE);
		newangle_=angle_?newangle_:((gangle!=0.0)?gangle:newangle_);

		if(	newscale_ == scale_
			&& newascale == ascale_
			&& neworientation == orientation_
			//&& newangle_ == angle_
			&& ( !newangle_  && !angle_ )
		)
		{
			goto ret;/*no need to rescale*/
		}
		orientation_ = FIM_MOD(neworientation,FIM_ROT_ROUND);

		setGlobalVariable(FIM_VID_SCALE,newscale_*100);
		if(fimg_)
		{
			/*
			 * In case of memory allocation failure, we would
			 * like to recover the current image. 
			 *
			 * Here it would be nice to add some sort of memory manager 
			 * keeping score of copies and ... too complicated ...
			 */
			struct ida_image *backup_img=img_;

			if(getGlobalIntVariable(FIM_VID_DISPLAY_STATUS_BAR)||getGlobalIntVariable(FIM_VID_DISPLAY_BUSY))
				cc.set_status_bar("please wait while rescaling...", "*");


#if FIM_WANT_ASCALE_FRIENDLY_ROTATION
			if( img_ && ( orientation_==FIM_ROT_L || orientation_ == FIM_ROT_R ))
				if( newascale != 1.0 )
					newascale = 1.0 / newascale;
#endif /* FIM_WANT_ASCALE_FRIENDLY_ROTATION */
#define FIM_PROGRESSIVE_RESCALING 0
#if FIM_PROGRESSIVE_RESCALING
			/*
			 * progressive rescaling is computationally convenient in when newscale_<scale_
			 * at the cost of a progressively worsening image quality (especially when newscale_~scale_)
			 * and a sequence ----+ will suddenly 'clear' out the image quality, so it is not a desirable
			 * option ...
			 * */
			if( 
				//( newscale_>scale_ && scale_ > 1.0) ||
				( newscale_<scale_ && scale_ < 1.0) )
				img_ = scale_image( img_,newscale_/scale_,newascale);
			else
				img_ = scale_image(fimg_,newscale_,newascale);
#else
			img_ = FbiStuff::scale_image(fimg_,newscale_,newascale
#if FIM_WANT_MIPMAPS
					,(getGlobalIntVariable(FIM_VID_WANT_MIPMAPS)>0)?(&mm_):FIM_NULL
#endif /* FIM_WANT_MIPMAPS */
					);
#endif /* FIM_PROGRESSIVE_RESCALING */
#if FIM_WANT_ASCALE_FRIENDLY_ROTATION
			if( img_ && ( orientation_==FIM_ROT_L || orientation_ == FIM_ROT_R ))
				if( newascale != 1.0 )
					newascale = 1.0 / newascale;
#endif /* FIM_WANT_ASCALE_FRIENDLY_ROTATION */
			do_rotate();

			if(!img_)
			{
				img_=backup_img;
				if(getGlobalIntVariable(FIM_VID_DISPLAY_BUSY))
					cc.set_status_bar( "rescaling failed (insufficient memory?!)", getInfo().c_str());
				sleep(1);	//just to give a glimpse..
			}
			else 
			{
				/* reallocation succeeded */
				if( backup_img && backup_img!=fimg_ )
				       	FbiStuff::free_image(backup_img);
				scale_=newscale_;
				ascale_=newascale;
				angle_ =newangle_;
	        		should_redraw();
			}

			/*
			 * it is important to set these values after rotation, too!
			 * */
			setVariable(FIM_VID_HEIGHT ,fimg_->i.height);
			setVariable(FIM_VID_WIDTH  ,fimg_->i.width );
			setVariable(FIM_VID_SHEIGHT, img_->i.height);
			setVariable(FIM_VID_SWIDTH , img_->i.width );
			setVariable(FIM_VID_ASCALE , ascale_ );
			//setGlobalVariable(FIM_VID_ANGLE  ,  angle_ );
		}
		else
		       	should_redraw(); /* FIXME: here shall not really redraw */
		orientation_=neworientation;
ret:
		return FIM_ERR_NO_ERROR;
err:
		return FIM_ERR_GENERIC;
	}

	fim_err_t Image::reduce(fim_scale_t factor)
	{
		newscale_ = scale_ / factor;
		return do_scale_rotate();
	}

	fim_err_t Image::magnify(fim_scale_t factor, fim_bool_t aes)
	{
		newscale_ = scale_ * factor;
#if FIM_WANT_APPROXIMATE_EXPONENTIAL_SCALING
		if(newscale_<2.0 && aes && ascale_ == 1.0)
		{
			fim_scale_t newscale = 1.0;
			while ( newscale / 2 >= newscale_ )
				newscale /= 2;
			newscale_ = newscale;
		}
#endif /* FIM_WANT_APPROXIMATE_EXPONENTIAL_SCALING */
		return do_scale_rotate();
	}

	Image::Image(const Image& rhs):
#ifdef FIM_NAMESPACES
		Namespace(rhs.rnsp_,FIM_SYM_NAMESPACE_IMAGE_CHAR),
#endif /* FIM_NAMESPACES */
		scale_(rhs.scale_),
		ascale_(rhs.ascale_),
		newscale_(rhs.newscale_),
		angle_(rhs.angle_),
		newangle_(rhs.newangle_),
		page_(rhs.page_),
                img_     (FIM_NULL),
                fimg_    (FIM_NULL),
		orientation_(rhs.orientation_),
		fis_(rhs.fis_),
                fname_     (rhs.fname_),
		fs_(0)
	{
		reset();
		img_  = fbi_image_clone(rhs.img_ );
		fimg_ = fbi_image_clone(rhs.fimg_);
	}

fim_int Image::shall_mirror(void)const
{
	const fim_int automirror= getGlobalIntVariable(FIM_VID_AUTOMIRROR);
	const fim_int me_mirrored = getIntVariable(FIM_VID_MIRRORED);
	//fim_int mirrored = getGlobalIntVariable("v:" FIM_VID_MIRRORED);
	return
	(((automirror== 1)/*|(mirrored== 1)*/|(is_mirrored()))&& !((automirror==-1)/*|(mirrored==-1)*/|(me_mirrored==-1)));
}

fim_int Image::check_flip(void)const
{
	const fim_int autoflip = getGlobalIntVariable(FIM_VID_AUTOFLIP);
	const fim_int am_flipped = getIntVariable(FIM_VID_FLIPPED);
	//fim_int flipped = getGlobalIntVariable("v:" FIM_VID_FLIPPED);
	return
	(((autoflip == 1)/*|(flipped == 1)*/| is_flipped()) && !((autoflip ==-1)/*|(flipped ==-1)*/|(am_flipped==-1)));
}

fim_int Image::check_negated(void)const
{
	return getIntVariable(FIM_VID_NEGATED);
}

fim_int Image::check_desaturated(void)const
{
	return getIntVariable(FIM_VID_DESATURATED);
}

fim_int Image::check_autocenter(void)const
{
	return getIntVariable(FIM_VID_WANT_AUTOCENTER);
}

fim_int Image::check_autotop(void)const
{
	return getIntVariable(FIM_VID_AUTOTOP);
}

fim::string Image::getInfo(void)const
{
	if(!fimg_)
		return FIM_CNS_EMPTY_RESULT;

	static fim_char_t linebuffer[FIM_STATUSLINE_BUF_SIZE];
#if FIM_WANT_CUSTOM_INFO_STATUS_BAR
	fim::string ifs(getGlobalStringVariable(FIM_VID_INFO_FMT_STR));

	if( !ifs.empty() )
	{
		fim::string clb = cc.getInfoCustom(ifs.c_str());
		snprintf(linebuffer, sizeof(linebuffer),"%s",clb.c_str());
		goto labeldone;
	}
	else
#endif /* FIM_WANT_CUSTOM_INFO_STATUS_BAR */
{
	/* FIXME: for cleanup, shall eliminate this branch and introduce a default string. */
	fim_char_t pagesinfobuffer[FIM_PRINTFNUM_BUFSIZE*2+3];
	fim_char_t imagemode[3],*imp;
	const fim_int n=getGlobalIntVariable(FIM_VID_FILEINDEX);
	imp=imagemode;

	if(check_flip())
		*(imp++)=FIM_SYM_FLIPCHAR;
	if(shall_mirror())
		*(imp++)=FIM_SYM_MIRRCHAR;
	*imp='\0';

	if(fimg_->i.npages>1)
		snprintf(pagesinfobuffer,sizeof(pagesinfobuffer)," [%d/%d]",(int)page_+1,(int)fimg_->i.npages);
	else
		*pagesinfobuffer='\0';

#if FIM_WANT_DISPLAY_MEMSIZE
	const size_t ms = fbi_img_pixel_bytes(fimg_); /* memory size */
#endif /* FIM_WANT_DISPLAY_MEMSIZE */

	snprintf(linebuffer, sizeof(linebuffer)-1,
	     "[ %s%.0f%% %dx%d%s%s %d/%d ]"
#if FIM_WANT_DISPLAY_FILESIZE
	     " %dkB"
#endif /* FIM_WANT_DISPLAY_FILESIZE */
#if FIM_WANT_DISPLAY_MEMSIZE
	     " %dMB"
#endif /* FIM_WANT_DISPLAY_MEMSIZE */
	     ,
	     /*fcurrent->tag*/ 0 ? "* " : "",
	     (scale_*100),
	     (int)this->width(), (int)this->height(),
	     imagemode,
	     pagesinfobuffer,
	     (int)(n?n:1), /* ... */
	     (int)(getGlobalIntVariable(FIM_VID_FILELISTLEN))
#if FIM_WANT_DISPLAY_FILESIZE
	     ,fs_/FIM_CNS_K
#endif /* FIM_WANT_DISPLAY_FILESIZE */
#if FIM_WANT_DISPLAY_MEMSIZE
	     ,ms/FIM_CNS_M
#endif /* FIM_WANT_DISPLAY_MEMSIZE */
	     );
}
labeldone:
	return fim::string(linebuffer);
}

	fim_err_t Image::update_meta(bool fresh)
	{
		fim_err_t errval = FIM_ERR_NO_ERROR;

		if(fresh)
			should_redraw(FIM_REDRAW_NECESSARY); // FIXME: this is duplication
		setVariable(FIM_VID_FRESH,fresh ? 1 : 0);
		if(fimg_)
			setVariable(FIM_VID_PAGES,fimg_->i.npages);

                fim_pgor_t neworientation=getOrientation();
		if( neworientation!=orientation_)
			if( ( errval = do_scale_rotate() ) == FIM_ERR_NO_ERROR )
				orientation_=neworientation;
		return errval;
	}

	fim_pgor_t Image::getOrientation(void)const
	{
		/*
		 * not very intuitive
		 * */
		fim_int eo = FIM_NO_ROT, weo = cc.getIntVariable(FIM_VID_WANT_EXIF_ORIENTATION);
		eo += getIntVariable(FIM_VID_EXIF_ORIENTATION) * ( weo ? 1 : 0 );
		return (FIM_MOD(
		( eo +
	       	 getIntVariable(FIM_VID_ORIENTATION)
		//+getGlobalIntVariable("v:" FIM_VID_ORIENTATION)
		+getGlobalIntVariable(FIM_VID_ORIENTATION)
		) ,4));
	}

	fim_err_t Image::rotate( fim_angle_t angle )
	{
		fim_angle_t newangle=this->angle_+angle;
		if( !check_valid() )
		       	return FIM_ERR_GENERIC;
		setVariable(FIM_VID_ANGLE,newangle);
		return do_scale_rotate();
	}

	bool Image::goto_page(fim_page_t j)
	{
		const fim_fn_t s(fname_);
		bool retval = false;

		FIM_PR('*');
		if( !fimg_ )
			goto ret;
		if( j<0 )
			j=fimg_->i.npages-1;
		if( j>page_ ? have_nextpage(j-page_) : have_prevpage(page_-j) )
		{
			//std::cout<<"about to goto page "<<j<<"\n";
			retval = load(s.c_str(),FIM_NULL,j);
			//return true;
		}
		else
			goto ret;
ret:
		FIM_PR('.');
		return retval;
	} 

	cache_key_t Image::getKey(void)const
	{
		return cache_key_t{fname_.c_str(),{page_,fis_}};
	}

	bool Image::is_multipage(void)const
	{
		if( fimg_ && ( fimg_->i.npages>1 ) )
			return true;
		return false;
	}

	bool Image::have_page(int page)const
	{
		return ( page >=0 && (0U + page) < (0U + fimg_->i.npages) ); // 0U is to impose signedness and type on either word length

	}

	bool Image::have_nextpage(int j)const
	{
		return (is_multipage() && have_page(page_+j));
	} 

	bool Image::have_prevpage(int j)const
	{
		return (is_multipage() && have_page(page_-j));
	}
 
	bool Image::is_mirrored(void)const
	{
		return FIM_XOR( this->getIntVariable(FIM_VID_EXIF_MIRRORED)==1, this->getIntVariable(FIM_VID_MIRRORED)==1 );
	}

	bool Image::is_flipped(void)const
	{

		return FIM_XOR( this->getIntVariable(FIM_VID_EXIF_FLIPPED) ==1, this->getIntVariable(FIM_VID_FLIPPED)==1 );
	}

	bool Image::colorblind(enum fim_cvd_t cvd, bool daltonize)
	{
		if( fimg_ &&  fimg_->data)
			fim_simulate_cvd(fimg_->data, fbi_img_pixel_bytes(fimg_), cvd, daltonize);

		if(  img_ &&   img_->data && ! (fimg_ && img_->data==fimg_->data) )
			fim_simulate_cvd(img_->data, fbi_img_pixel_bytes(img_), cvd, daltonize);

#if FIM_WANT_MIPMAPS
		if(  mm_.mdp)
			fim_simulate_cvd(mm_.mdp, mm_.mmb, cvd, daltonize);
#endif /* FIM_WANT_MIPMAPS */

       		should_redraw();

		return true;
	}

	fim_err_t Image::desaturate(void)
	{
		if( fimg_ &&  fimg_->data)
			fim_desaturate_rgb(fimg_->data, fbi_img_pixel_bytes(fimg_));

		if(  img_ &&   img_->data)
			fim_desaturate_rgb(img_->data, fbi_img_pixel_bytes(img_));

#if FIM_WANT_MIPMAPS
		if(  mm_.mdp)
			fim_desaturate_rgb(mm_.mdp, mm_.mmb);
#endif /* FIM_WANT_MIPMAPS */

		setVariable(FIM_VID_DESATURATED ,1-getIntVariable(FIM_VID_DESATURATED ));
       		should_redraw();
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t Image::identity(void)
	{
		if( fimg_ &&  fimg_->data)
			fim_generate_24bit_identity(fimg_->data, fbi_img_pixel_bytes(fimg_));

		if(  img_ &&   img_->data)
			fim_generate_24bit_identity(img_->data, fbi_img_pixel_bytes(img_));

#if FIM_WANT_MIPMAPS
		if(  mm_.mdp)
			fim_generate_24bit_identity(mm_.mdp, mm_.mmb);
#endif /* FIM_WANT_MIPMAPS */
       		should_redraw();
		return FIM_ERR_NO_ERROR;
	}

	fim_err_t Image::negate(void)
	{

		if( fimg_ &&  fimg_->data)
			fim_negate_rgb(fimg_->data, fbi_img_pixel_bytes(fimg_));

		if(  img_ &&   img_->data)
			fim_negate_rgb(img_->data, fbi_img_pixel_bytes(img_));

#if FIM_WANT_MIPMAPS
		if(  mm_.mdp)
			fim_negate_rgb(mm_.mdp, mm_.mmb);
#endif /* FIM_WANT_MIPMAPS */

		setVariable(FIM_VID_NEGATED ,1-getIntVariable(FIM_VID_NEGATED ));
       		should_redraw();
		return FIM_ERR_NO_ERROR;
	}

	int Image::n_pages()const{return (fimg_?fimg_->i.npages:0);}

	size_t Image::byte_size(void)const
	{
		size_t ms = 0;

		if(fimg_)
			ms += fbi_img_pixel_bytes(fimg_);
		if(fimg_!=img_ && img_)
			ms += fbi_img_pixel_bytes(img_);
#if FIM_WANT_MIPMAPS
		ms += mm_.byte_size();
#endif /* FIM_WANT_MIPMAPS */
		return ms;
	}

#if FIM_WANT_BDI
	Image::Image(enum fim_tii_t tii)
	{
		reset();

#if FIM_WANT_OBSOLETE
		if( FIM_TII_16M == tii )
		{
			const fim_coo_t fourk=256*16;
			img_  = fbi_image_black(fourk,fourk);
			fimg_ = img_;

			if(img_)
				identity();
			else
				cout << "warning: problem generating an image\n";
		}
		else
#endif /* FIM_WANT_OBSOLETE */
		{
			/* nevertheless this instance shall support all operations on it */
			assert(!check_valid());
		}
	}

	const fim_char_t* Image::getName(void)const{return fname_.c_str();}
#endif	/* FIM_WANT_BDI */

#if FIM_WANT_MIPMAPS
	void Image::mm_free(void) { mm_.dealloc(); }
	void Image::mm_make(void)
	{
	       	enum fim_mmo_t mmo = FIM_MMO_NORMAL;
		fim_int wmm = getGlobalIntVariable(FIM_VID_WANT_MIPMAPS);

		if( wmm<=0 || has_mm() )
			return;
		if(wmm>1)
			mmo = FIM_MMO_FASTER;
		mm_.dealloc();
		mm_.mmo=mmo;
	       	FbiStuff::fim_mipmaps_compute(fimg_,&mm_);
	}
	bool Image::has_mm(void)const { return mm_.ok(); }
	size_t Image::mm_byte_size(void)const
	{
		// might implement has_mm in terms of this
		return this->mm_.byte_size();
	}
#endif /* FIM_WANT_MIPMAPS */
	bool Image::cacheable(void)const {
		//return this->n_pages() == 1 ;
		return true;
	}
	void Image::set_auto_props(fim_int autocenter, fim_int autotop)
	{
		setVariable(FIM_VID_WANT_AUTOCENTER,autocenter);
		setVariable(        FIM_VID_AUTOTOP,autotop);
	}
void Image::get_irs(char *imp)const
{
	// imp shall be at least 4 chars long
	if(this->check_flip())
		*(imp++)=FIM_SYM_FLIPCHAR;
	if(this->shall_mirror())
		*(imp++)=FIM_SYM_MIRRCHAR;
	switch(this->orientation_)
	{
		case Image::FIM_ROT_L:
			*(imp++)=Image::FIM_ROT_L_C;
		break;
		case Image::FIM_ROT_U:
			 *(imp++)=Image::FIM_ROT_U_C;
		break;
		case Image::FIM_ROT_R:
			 *(imp++)=Image::FIM_ROT_R_C;
		break;
		case Image::FIM_NO_ROT:
		default:
			/**/
		break;
	}
	*imp=FIM_SYM_CHAR_NUL;
}
size_t Image::get_pixelmap_byte_size(void)const
{
	return fim::fbi_img_pixel_bytes(fimg_);
}
} /* namespace fim */
