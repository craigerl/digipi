/* $LastChangedDate: 2024-02-23 00:49:56 +0100 (Fri, 23 Feb 2024) $ */
/*
 Cache.cpp : Cache manager source file

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
#include "fim.h"
/*	#include <malloc.h>	*/
#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#else /* HAVE_SYS_TIME_H */
	#include <time.h>
#endif /* HAVE_SYS_TIME_H */

#define FIM_CACHE_MIPMAP 1
#define FIM_CACHE_INSPECT 0
#define FIM_PRINT_REPORT(X)   printf("CACHE:%c:%20s:%s",X,__func__,getReport(FIM_CR_CD).c_str());
#if FIM_CACHE_INSPECT
#define FIM_PR(X)   FIM_PRINT_REPORT(X)
//#define FIM_PS(X,S) FIM_PR(X); std::cout << " " << __func__ << "(" << S << ")\n";
#else /* FIM_CACHE_INSPECT */
#define FIM_PR(X) 
//#define FIM_Ps(X) 
#endif /* FIM_CACHE_INSPECT */

#define FIM_VERBOSE_CACHE 1

#ifdef FIM_VERBOSE_CACHE
#define FIM_ALLOW_CACHE_DEBUG getGlobalVariable(FIM_VID_DBG_COMMANDS).find('C') >= 0
#else  /* FIM_VERBOSE_CACHE */
#define FIM_ALLOW_CACHE_DEBUG 0
#endif /* FIM_VERBOSE_CACHE */

#if 0
#define FIM_LOUD_CACHE_STUFF FIM_PR(-10); FIM_LINE_COUT
#else
#define FIM_LOUD_CACHE_STUFF 
#endif
#define FIM_VCBS(VI) ( sizeof(VI) + VI.size() * ( sizeof(vcachels_t::mapped_type) + sizeof(vcachels_t::key_type) ) )
/* TODO: maybe fim_basename_of is excessive ?  */
	extern CommandConsole cc;
namespace fim
{
	std::ostream& operator<<(std::ostream& os, const cache_key_t & key)
	{
		os << ( key.second.second == FIM_E_FILE ? "file " : "stdin " );
		os << fim_basename_of(key.first) << " at page ";
		os << key.second.first << "";
		return os;
	}

	static fim_time_t fim_time(void) FIM_NOEXCEPT /* stand-alone function */
	{
#ifdef HAVE_SYS_TIME_H
		struct timeval tv;
		FIM_CONSTEXPR fim_time_t prec = 1000; /* fraction of second precision */
		gettimeofday(&tv, FIM_NULL);
		return tv.tv_sec * prec + tv.tv_usec / ( 1000000 / prec );
#else /* HAVE_SYS_TIME_H */
		return time(FIM_NULL);
#endif /* HAVE_SYS_TIME_H */
	}

	fim_time_t Cache::get_reltime(void)const
	{
		return fim_time()-time0_;
	}

	Cache::Cache(void)
		:Namespace(&cc)
		,time0_(fim_time())
	{
		FIM_LOUD_CACHE_STUFF;
	}

	int Cache::cached_elements(void)const
	{
		/*	the count of cached images	*/
		FIM_LOUD_CACHE_STUFF;
		return imageCache_.size();
	}

	ImagePtr Cache::get_lru( bool unused )const
	{
		/* get the least recently used element.
		 * if unused is true, only an unused image will be returned, _if any_*/
		lru_t::const_iterator lrui;
		fim_time_t m_time(get_reltime()), l_time=0;
		ImagePtr  l_img(FIM_NULL);
		cachels_t::const_iterator ci;
		FIM_LOUD_CACHE_STUFF;

		if ( cached_elements() < 1 )
			goto ret;

		for( ci=imageCache_.begin();ci!=imageCache_.end();++ci)
		if( ci->second /* <- so we can call this function in some intermediate states .. */
			 && (l_time=last_used(ci->first)) < m_time  &&  (  (! unused) || (used_image(ci->first)<=0)  ) )
		{
			l_img  = ci->second;
			m_time = l_time;
		}
ret:
		return l_img;
	}

	int Cache::free_some_lru(void)
	{
		/*
		 * trigger deletion (and memory free) of cached elements
		 * (a sort of garbage collector)
		 */
		FIM_LOUD_CACHE_STUFF;
		FIM_PR(' ');
		if ( cached_elements() < 1 )
			return 0;
		return erase( get_lru(true)  );
	}

	int Cache::erase_clone(fim::ImagePtr oi)
	{
		/*	erases the image clone from the cache	*/
		FIM_LOUD_CACHE_STUFF;
		FIM_PR(' ');
		if(FIM_ALLOW_CACHE_DEBUG)
			std::cout << FIM_CNS_DBG_CMDS_PFX << "erasing clone " << fim_basename_of(oi->getName()) << "\n";
		//cloneUsageCounter_.erase(oi);
#if FIM_IMG_NAKED_PTRS
		delete oi;
#else /* FIM_IMG_NAKED_PTRS */
		// TBD
#endif /* FIM_IMG_NAKED_PTRS */
		clone_pool_.erase(oi);
		return 0;
	}

	bool Cache::need_free(void)const
	{
		/*	whether we should free some cache ..	*/
		/*
		struct mallinfo mi = mallinfo();
		cout << "allocated : " <<  mi.uordblks << "\n";
		if( mi.uordblks > getIntGlobalVariable(FIM_VID_MAX_CACHED_MEMORY) )
			return true;
		*/
		const fim_int mci = getGlobalIntVariable(FIM_VID_MAX_CACHED_IMAGES);
		const fim_int mcm = getGlobalIntVariable(FIM_VID_MAX_CACHED_MEMORY); /* getIntGlobalVariable */
		const size_t smcm = mcm > 0 ? mcm : 0;

	       	if( smcm > 0 && img_byte_size()/FIM_CNS_CSU > smcm )
			goto rt;

		if(mci==-1)
			goto rf;

		/* return ( cached_elements() > ( ( mci>0)?mci:-1 ) ); */
		if(mci > 0 && cached_elements() > mci)
			goto rt;
rf:
		FIM_PR('n');
		return false;
rt:
		FIM_PR('y');
		return true;
	}

	int Cache::used_image(cache_key_t key)const
	{
		FIM_LOUD_CACHE_STUFF;
		return ( usageCounter_.find(key)!=usageCounter_.end() ) ?  (*(usageCounter_.find(key))).second : 0;
	}

	bool Cache::is_in_clone_cache(fim::ImagePtr oi)const
	{
		FIM_LOUD_CACHE_STUFF;
		if(!oi)
			return false;
		return ( clone_pool_.find(oi)!=clone_pool_.end() )	
			&&
			((*clone_pool_.find(oi)) == oi );
	}

	bool Cache::is_in_cache(cache_key_t key)const
	{
		FIM_LOUD_CACHE_STUFF;
		const bool iic = ( imageCache_.find(key) != imageCache_.end() )
			&&
			((*(imageCache_.find(key))).second!=FIM_NULL) ;
		return iic;
	}

	int Cache::prefetch(cache_key_t key)
	{
		int retval = 0;
		FIM_PR('*');
		//FIM_PS('*',key);

		FIM_LOUD_CACHE_STUFF;
		if(is_in_cache(key))
		{
			FIM_PR('c');
			goto ret;
		}
#if 0
	  	if(need_free())
			free_some_lru();
		if(need_free())
		{
			FIM_PR('f');
			goto ret; /* skip prefetch if cache is full */
		}
#endif
		if(key.first == FIM_STDIN_IMAGE_NAME)
		{
			FIM_PR('s');
			goto ret;// just a fix in the case the browser is still lame
		}
		if(FIM_ALLOW_CACHE_DEBUG)
			std::cout << FIM_CNS_DBG_CMDS_PFX << "cache prefetch req     " << key << "\n";

    		if( regexp_match(key.first.c_str(),FIM_CNS_ARCHIVE_RE,1) )
		{
			/* FIXME: This is a hack. One shall determine unprefetchability othwerwise. */
			FIM_PR('j');
			goto ret;
		}

		if(!loadNewImage(key,true))
		{
			retval = -1;
			if(FIM_ALLOW_CACHE_DEBUG)
				std::cout << FIM_CNS_DBG_CMDS_PFX << "loading failed\n";
			goto ret;
		}
		else
		{
			FIM_PR('l');
		}
		setGlobalVariable(FIM_VID_CACHED_IMAGES,cached_elements());
		setGlobalVariable(FIM_VID_CACHE_STATUS,getReport());
ret:
		FIM_PR('.');
		return retval;
	}

	int Cache::prefetch(fid_t fid)
	{
		return prefetch(cache_key_t{fid,{FIM_CNS_FIRST_PAGE,FIM_E_FILE}});
	}

	ImagePtr Cache::loadNewImage(cache_key_t key, fim_bool_t delnc)
	{
		const fim_page_t page {key.second.first};
		ImagePtr ni = FIM_NULL;
		FIM_PR('*');

		FIM_LOUD_CACHE_STUFF;
		/*	load attempt as alternative approach	*/
		try
		{
		if( ( ni = ImagePtr( new Image(key.first.c_str(), FIM_NULL, page) ) ) )
		{
			if(FIM_ALLOW_CACHE_DEBUG)
				std::cout << FIM_CNS_DBG_CMDS_PFX << "cache loaded           " << key << "\n";
			if( ni->cacheable() )
				cacheNewImage( ni );
			else
				if (delnc) // delete non cacheable
				{
#if FIM_IMG_NAKED_PTRS
					delete ni;
#else /* FIM_IMG_NAKED_PTRS */
					// TBD
#endif /* FIM_IMG_NAKED_PTRS */
					ni = FIM_NULL;
				}
		}
		}
		catch(FimException e)
		{
			FIM_PR('E');
			ni = FIM_NULL; /* not a big problem */
//			if( e != FIM_E_NO_IMAGE )throw FIM_E_TRAGIC;  /* hope this never occurs :P */
		}
		FIM_PR('.');
		return ni;
	}
	
	ImagePtr Cache::getCachedImage(cache_key_t key)
	{
		/* returns an image if already in cache. */
		ImagePtr ni = FIM_NULL;
		FIM_LOUD_CACHE_STUFF;
		FIM_PR(' ');
	
		if( ( ni = this->imageCache_[key]) )
			this->lru_touch(key);
		return ni;
	}

	bool Cache::cacheNewImage( fim::ImagePtr ni )
	{
		const cache_key_t key = ni->getKey();
		FIM_LOUD_CACHE_STUFF;
		FIM_PR(' ');
		//if(FIM_ALLOW_CACHE_DEBUG)
		//	std::cout << FIM_CNS_DBG_CMDS_PFX << "going to cache: "<< *ni /*<< " [" << ni << "]" */<< "\n";
		if(FIM_ALLOW_CACHE_DEBUG)
			std::cout << FIM_CNS_DBG_CMDS_PFX << "going to cache: "<< key << "\n";
		this->imageCache_[key]=ni;
		lru_touch( key );
		usageCounter_[key]=0; // we don't assume any usage yet
		setGlobalVariable(FIM_VID_CACHED_IMAGES,cached_elements());
		return true;
	}
	
	int Cache::erase(fim::ImagePtr oi)
	{
		/*	erases the image from the cache	*/
		int retval=-1;
		cache_key_t key;
		FIM_PR(' ');

		FIM_LOUD_CACHE_STUFF;
		if(!oi)
			goto ret;

		if( is_in_cache(key = oi->getKey()) )
		{
			usageCounter_[key]=0;
			/* NOTE : the user should call usageCounter_.erase(key) after this ! */
			if(FIM_ALLOW_CACHE_DEBUG)
				std::cout << FIM_CNS_DBG_CMDS_PFX << "cache erases           " << (oi->getKey()) << "\n"; /*<< " with timestamp " << lru_[oi->getKey()] */
			lru_.erase(key);
			imageCache_.erase(key);
			usageCounter_.erase(key);
#if FIM_IMG_NAKED_PTRS
			delete oi;
#else /* FIM_IMG_NAKED_PTRS */
			// TBD
#endif /* FIM_IMG_NAKED_PTRS */
			setGlobalVariable(FIM_VID_CACHED_IMAGES,cached_elements());
			retval = 0;
		}
ret:
		return retval;
	}

	fim_time_t Cache::last_used(cache_key_t key)const
	{
		fim_time_t retval=0;

		FIM_LOUD_CACHE_STUFF;
		if(imageCache_.find(key)==imageCache_.end())
			goto ret;
		if(lru_.find(key)==lru_.end())
			goto ret;
		retval = lru_.find(key)->second;
ret:
		return retval;
	}

	int Cache::lru_touch(cache_key_t key)
	{
		/*
		 * if the specified file is cached, in this way it is marked as used, too.
		 * the usage count is not affected, 
		 * */
		FIM_LOUD_CACHE_STUFF;
		FIM_PR(' ');
//		std::cout << lru_[key] << " -> ";
		lru_[key]= get_reltime();
//		std::cout << lru_[key] << "\n";
		return 0;
	}

	bool Cache::freeCachedImage(ImagePtr image, const ViewportState *vsp, bool force)
	{
		/*
		 * If the supplied image is cached as a master image of a clone, it is freed and deregistered.
		 * If not, no action is performed.
		 * */
		FIM_LOUD_CACHE_STUFF;
		FIM_PR('*');

		if( !image )
			goto err;

		if(vsp)
			viewportInfo_[image->getKey()] = *vsp;
		if( is_in_clone_cache(image) )
		{
			FIM_PR('c');
			if(FIM_ALLOW_CACHE_DEBUG)
				std::cout << FIM_CNS_DBG_CMDS_PFX << "decrement clones count " << image->getKey() << "\n";
			usageCounter_[image->getKey()]--;
			erase_clone(image);	// we _always_ immediately delete clones
			setGlobalVariable(FIM_VID_CACHE_STATUS,getReport());
			goto ret;
		}
		else
		if(!is_in_cache(image->getKey()) )
		{
			FIM_PR('n');
			if(FIM_ALLOW_CACHE_DEBUG)
				std::cout << FIM_CNS_DBG_CMDS_PFX << "can't unuse uncached!! " << image->getKey() << "\n";
		}
		else
		if( is_in_cache(image->getKey()) )
		{
			FIM_PR('-');
			if(FIM_ALLOW_CACHE_DEBUG)
				std::cout << FIM_CNS_DBG_CMDS_PFX
					<< "unuse  " << image->getKey()
					<< "  force:" << force
					<< "  usageCounter_():" << (usageCounter_[image->getKey()])
					<< "  cached_elements():" << cached_elements() << "\n";
			lru_touch( image->getKey() ); // we have been using it until now
			usageCounter_[image->getKey()]--;
#if FIM_WANT_MIPMAPS
			if( getGlobalStringVariable(FIM_VID_CACHE_CONTROL).find('M') == 0 )
				image->mm_free();
#endif /* FIM_WANT_MIPMAPS */
			if(
				( (usageCounter_[image->getKey()])==0 && 
				image->getKey().second.second!=FIM_E_STDIN  )
				 || force)
			{
				const fim_int minciv = getGlobalIntVariable(FIM_VID_MIN_CACHED_IMAGES);
				const fim_int minci = ( minciv < 1 ) ? 4 : minciv;
#if 0
				if( need_free() && image->getKey().second!=FIM_E_STDIN )
				{
					cache_key_t key = image->getKey();
					this->erase( image );
					usageCounter_.erase(key);
				}
#else
				if( ( need_free() && cached_elements() > minci ) || force )
				{
					ImagePtr lrui;
					if(FIM_ALLOW_CACHE_DEBUG)
						std::cout << FIM_CNS_DBG_CMDS_PFX << "cache suggested free  " << image->getKey() << (force?"  forced=1":"  forced=0") << "\n";
					if(force)
						lrui = image;
					else
						lrui = get_lru(true);
//					if(FIM_ALLOW_CACHE_DEBUG && force)
//						std::cout << FIM_CNS_DBG_CMDS_PFX << "cache forced to free  " << lrui->getName() << "\n";
					FIM_PR('o');
					if( lrui ) 
					{
						const cache_key_t key = lrui->getKey();
						if(FIM_ALLOW_CACHE_DEBUG)
							std::cout << FIM_CNS_DBG_CMDS_PFX << "cache to free          " << key << (force?"  forced=1":"  forced=0") << "\n";
						if( ( FIM_VCBS(viewportInfo_) > FIM_CNS_VICSZ ) || force )
							viewportInfo_.erase(key);
						if(( key.second.second != FIM_E_STDIN ))
						{	
							this->erase( lrui );
						}
					}
					else
						if(FIM_ALLOW_CACHE_DEBUG)
							std::cout << FIM_CNS_DBG_CMDS_PFX << "nothing to free: (" << cached_elements() << " cached elements)\n";
					// missing usageCounter_.erase()..
				}
#endif
			}
			setGlobalVariable(FIM_VID_CACHE_STATUS,getReport());
			goto ret;
		}
err:
		FIM_PR('.');
		return false;
ret:
		FIM_PR('.');
		return true;
	}

	ImagePtr Cache::useCachedImage(cache_key_t key, ViewportState *vsp)
	{
		/*
		 * Shall rename to get().
		 *
		 * The caller invokes this member function to obtain an Image object pointer.
		 * If the object is cached and it already used, a clone is built and returned.
		 *
		 * If we have an unused master copy, we return that.
		 *
		 * Then declare this image as used and increase a relative counter.
		 *
		 * A freeImage action will do the converse operation (and delete).
		 * If the image is not already cached, it is loaded, if possible.
		 *
		 * So, if there is no such image, FIM_NULL is returned
		 * */
		ImagePtr image = FIM_NULL;

		FIM_LOUD_CACHE_STUFF;
		FIM_PR('*');

		if(FIM_ALLOW_CACHE_DEBUG)
			std::cout << FIM_CNS_DBG_CMDS_PFX << "cache check for " << ( key.second.second == FIM_E_FILE ? " file  ": " stdin ")<<key.first<<" at page " << key.second.first << "\n";
		if(!is_in_cache(key)) 
		{
			if(FIM_ALLOW_CACHE_DEBUG)
				std::cout << FIM_CNS_DBG_CMDS_PFX << "cache does not have    " << key << "\n";
			image = loadNewImage(key,false);
			if(!image)
				goto ret; // bad luck!
			if(!image->cacheable())
				goto ret; // we keep it but don't cache it
			usageCounter_[key]=1;
		}
		else // is_in_cache(key)
		{
			image = getCachedImage(key);// in this way we update the LRU cache :)
			if(!image)
			{
				if(FIM_ALLOW_CACHE_DEBUG)
					std::cout << FIM_CNS_DBG_CMDS_PFX << "critical internal cache error!\n";
				goto done;
			}
			if( used_image( key ) )
			{
				// if the image was already used, cloning occurs
				try
				{
					//ImagePtr oi=image;
					image = ImagePtr ( new Image(*image) ); // cloning
					if(FIM_ALLOW_CACHE_DEBUG)
						std::cout << FIM_CNS_DBG_CMDS_PFX << "  cloned image: \"" << key << "\" "<< image << "\n";
						//std::cout << "  cloned image: \"" <<fim_basename_of(image->getName())<< "\" "<< image << " from \""<<fim_basename_of(oi->getName()) <<"\" " << oi << "\n";
				}
				catch(FimException e)
				{
					FIM_PR('E');
					/* we will survive :P */
					image = FIM_NULL; /* we make sure no taint remains */
//					if( e != FIM_E_NO_IMAGE )throw FIM_E_TRAGIC;  /* hope this never occurs :P */
				}
				if(!image)
					goto ret; //means that cloning failed.
				clone_pool_.insert(image); // we have a clone
				//cloneUsageCounter_[image]=1;
			}
#if FIM_WANT_MIPMAPS
			if( getGlobalStringVariable(FIM_VID_CACHE_CONTROL).find('M') == 0 )
				image->mm_make();
#endif /* FIM_WANT_MIPMAPS */
			lru_touch( key );
			// if loading and eventual cloning succeeded, we count the image as used of course
			usageCounter_[key]++;
		}
done:
		setGlobalVariable(FIM_VID_CACHE_STATUS,getReport());
ret:
		if(vsp && image)
		{
			*vsp = viewportInfo_[image->getKey()];
			/* *vsp = viewportInfo_[key]; */
		}
		FIM_PR('.');
		return image;
	}

	ImagePtr Cache::setAndCacheStdinCachedImage(ImagePtr image)
	{
		/* Cache an image coming from stdin (that is, not reloadable).
		 * */
		const fim_page_t page = FIM_CNS_FIRST_PAGE;
		const cache_key_t key{FIM_STDIN_IMAGE_NAME,{page,FIM_E_STDIN}};
		FIM_LOUD_CACHE_STUFF;
		FIM_PR('*');

		if(!image)
			goto ret;
		
		try
		{
			image = ImagePtr ( new Image(*image) ); // cloning
			if(image)
				cacheNewImage( image );
		}
		catch(FimException e)
		{
			FIM_PR('E');
			/* we will survive :P */
			image = FIM_NULL; /* we make sure no taint remains */
//			if( e != FIM_E_NO_IMAGE )throw FIM_E_TRAGIC;  /* hope this never occurs :P */
		}
		if(!image)
			goto ret; //means that cloning failed.
		setGlobalVariable(FIM_VID_CACHE_STATUS,getReport());
ret:
		return image;	//so, it could be a clone..
	}

	fim::string Cache::getReport(int type)const
	{
		/* TODO: rename to info(). */
		std::ostringstream cache_report;

		if(type == FIM_CR_CN || type == FIM_CR_CD)
		{
			fim_char_t buf[FIM_PRINTFNUM_BUFSIZE];
			const fim_int mci = getGlobalIntVariable(FIM_VID_MAX_CACHED_IMAGES);
			fim_int mcm = getGlobalIntVariable(FIM_VID_MAX_CACHED_MEMORY);
			mcm = mcm >= 0 ? mcm*FIM_CNS_CSU:0;
			cache_report << " " << "count:" << cached_elements() << "/" << mci << " occupation:";
			fim_snprintf_XB(buf, sizeof(buf), img_byte_size());
			cache_report << buf << "/";
			fim_snprintf_XB(buf, sizeof(buf), mcm);
			cache_report << buf << " ";
			for(ccachels_t::const_iterator ci=usageCounter_.begin();ci!=usageCounter_.end();++ci)
			if(
			  ( type == FIM_CR_CD && ( imageCache_.find(ci->first) != imageCache_.end() ) ) ||
			  ( type == FIM_CR_CN && ( imageCache_.find(ci->first) != imageCache_.end()  && ci->second) )
			  )
			{
				cache_report << "f=" << fim_basename_of((*ci).first.first) << ":";
				cache_report << "p=" << (*ci).first.second.first << ":";
				cache_report << "t=" << (*ci).second << ":";
				fim_snprintf_XB(buf, sizeof(buf), imageCache_.find(ci->first)->second->byte_size());
				cache_report << buf << "@" << last_used(ci->first) << " ";
			}
			cache_report << "\n";
			goto ret;
		}
		cache_report << "cache contents : \n";
		FIM_LOUD_CACHE_STUFF;
#if 0
		cachels_t::const_iterator ci;
		for( ci=imageCache_.begin();ci!=imageCache_.end();++ci)
		{	
			cache_report << ((*ci).first) << " " << fim::string(usageCounter_[((*ci).first)]) << "\n";
		}
#else
		for(ccachels_t::const_iterator ci=usageCounter_.begin();ci!=usageCounter_.end();++ci)
		{	
			cache_report<<((*ci).first.first) <<":" <<((*ci).first.second.first) <<":" <<((*ci).first.second.second) <<" ,usage:" <<((*ci).second) <<"\n";
		}
		cache_report << "clone pool contents : \n";
		for(std::set< fim::ImagePtr >::const_iterator  cpi=clone_pool_.begin();cpi!=clone_pool_.end();++cpi)
		{	
			cache_report << fim_basename_of((*cpi)->getName()) <<" " 
#if FIM_IMG_NAKED_PTRS
				<< string((int*)(*cpi))
#else /* FIM_IMG_NAKED_PTRS */
				<< ((int*)((*cpi).get()))
#endif /* FIM_IMG_NAKED_PTRS */
				<<",";
		}
		cache_report << "\n";
#endif
ret:
		return cache_report.str();
	}

	Cache::~Cache(void)
	{
		cachels_t::const_iterator ci;
		FIM_LOUD_CACHE_STUFF;
		for( ci=imageCache_.begin();ci!=imageCache_.end();++ci)
			if(ci->second)
			{
				if(FIM_ALLOW_CACHE_DEBUG)
					std::cout << FIM_CNS_DBG_CMDS_PFX << "cache erases also      " << (ci->first) << "\n";
#if FIM_IMG_NAKED_PTRS
				delete ci->second;
#else /* FIM_IMG_NAKED_PTRS */
				// TBD
#endif /* FIM_IMG_NAKED_PTRS */
			}
		imageCache_.clear(); /* destroy Image objects */
	}

	size_t Cache::img_byte_size(void)const
	{
		size_t bs = 0;
		cachels_t::const_iterator ci;

		FIM_LOUD_CACHE_STUFF;
		for( ci=imageCache_.begin();ci!=imageCache_.end();++ci)
			if(ci->second)
				bs += ci->second->byte_size();
		return bs;
	}

	size_t Cache::byte_size(void)const
	{
		size_t bs = 0;
		cachels_t::const_iterator ci;
		FIM_LOUD_CACHE_STUFF;
		bs += img_byte_size();
		bs += sizeof(*this);
		bs += FIM_VCBS(viewportInfo_);
		/* 
		bs += sizeof(usageCounter_);
		TODO: incomplete ...
		*/
		return bs;
	}

	void Cache::touch(cache_key_t key)
	{
		FIM_PR('*');
		getCachedImage(key);
		FIM_PR('.');
       	}

	void Cache::desc_update(void)
	{
#if FIM_WANT_PIC_CMTS
		cachels_t::const_iterator ci;

		FIM_LOUD_CACHE_STUFF;
		for( ci=imageCache_.begin();ci!=imageCache_.end();++ci)
			if(ci->second)
				ci->second->desc_update();
#endif /* FIM_WANT_PIC_CMTS */
	}
} /* namespace */
