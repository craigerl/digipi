/* $LastChangedDate: 2024-03-22 23:36:00 +0100 (Fri, 22 Mar 2024) $ */
/*
 DebugConsole.cpp : Fim virtual console display.

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

#ifndef FIM_WANT_NO_OUTPUT_CONSOLE
namespace fim
{
#if FIM_WANT_OBSOLETE
		int MiniConsole::line_length(int li)
		{
			if(li<cline_)
			{
				return li<cline_?(line_[li+1]-line_[li]):(ccol_);
			}
			else
			if(li<0)
				return 0;
			// in the case li==cline_, ccol_==bp_-buffer_ will do the job:
			return ccol_;
		}
#endif /* FIM_WANT_OBSOLETE */

		fim_err_t MiniConsole::do_dump(int f, int l)const
		{
			/*
			 * if f <= l and f>= 0 
			 * lines from min(f,cline_) to min(l,cline_) will be dumped
			 *
			 * if f<0 and l>=0 and f+l<0 and -f<=cline_,
			 * lines from cline_+f to cline_-l will be dumped
			 * */
			int fh; // font height
			const int cancols = displaydevice_->get_chars_per_line(); // canvas columns
			const int maxcols = FIM_MIN(cancols,lwidth_)+1;
			fim_char_t *buf=FIM_NULL;
			fim_err_t errval = FIM_ERR_GENERIC;
	
			if(f<0 && l>=0 && f+l<0 && -f<=cline_)
		       	{
			       	f=cline_+f;
			       	l=cline_-l;
		       	}
			else
			if(f<=l && f>=0)
			{
				f=FIM_MIN(cline_,f);
				l=FIM_MIN(cline_,l);
			}
			else
				goto err;
			
			if(cancols<1)
			{
#if FIM_WANT_OVERLY_VERBOSE_DUMB_CONSOLE
				if(cancols==0)
				{
					// a fixup for the dumb console
					const int n=1+line_[l]-line_[f];
					fim_char_t*ibuf=FIM_NULL;
					if(n>0 && (ibuf=(fim_char_t*) fim_malloc(n))!=FIM_NULL)
					{
						strncpy(ibuf,line_[f],n);
						ibuf[n-1]='\0';
						displaydevice_->fs_puts(displaydevice_->f_,0,0,ibuf);
						fim_free(ibuf);
					}
				}
				else
#endif /* FIM_WANT_OVERLY_VERBOSE_DUMB_CONSOLE */
					goto err;
			}

			if(*bp_)
				goto err;
			//if *bp_ then we could print garbage so we exit before damage is done

			fh=displaydevice_->f_ ? displaydevice_->f_->sheight():1; // FIXME : this is not clean
			l-=f;
		       	l%=(rows_+1);
		       	l+=f;

			/* FIXME : the following line_ is redundant in fb, but not in SDL 
			 * moreover, it seems useless in AA (could be a bug)
			 * */
			if(fh*(l-f+1)>=displaydevice_->height())
				goto done;
			buf = (fim_char_t*) fim_malloc(cancols+1);
			if(!buf)
				goto err;
			displaydevice_->clear_rect(0, displaydevice_->width()-1, 0 ,fh*(l-f+1) );

			// fs_puts alone won't draw to screen, but to the back plane, so unlock/flip will be necessary
			displaydevice_->lock();

	    		for(int i=f  ;i<=l   ;++i)
			{
				int t = ( (i<cline_) ? (line_[i+1]-line_[i]) : (ccol_) )%maxcols;
				if( t<0 )
					goto err; // hmmm
				strncpy(buf,line_[i],t);
				while( t>0 && buf[t-1]=='\n' )
					--t; // rewind newlines
				while(t<cancols)
					buf[t++]=' '; // after text, fill with blanks
				/* lwidth_ is user set, but we truncate to cancols if exceeding */
				buf[ cancols ]='\0';
				displaydevice_->fs_puts(displaydevice_->f_, 0, fh*(i-f), buf);
			}

			// extra empty lines (originally for aa)
	    		for(int i=0  ;i<scroll_ ;++i)
			{
				const int t = maxcols;
				std::fill_n(buf,t,' ');
				buf[t-1]='\0';
				displaydevice_->fs_puts(displaydevice_->f_, 0, fh*((l-f+1)+i), buf);
			}
			displaydevice_->unlock();
			displaydevice_->flush();
done:
			errval = FIM_ERR_NO_ERROR;
err:
			if(buf)
				fim_free(buf);
			return errval;
		}

		fim_err_t MiniConsole::add(const fim_char_t* cso_)
		{
			fim_char_t *s=FIM_NULL,*b=FIM_NULL;
			int nc;
			int nl;
			const int ol=cline_;
			fim_char_t *cs=FIM_NULL;/* using co would mean provoking the compiler */
			fim_char_t*cso=(fim_char_t*)cso_;// FIXME

#if FIM_WANT_MILDLY_VERBOSE_DUMB_CONSOLE
			if(displaydevice_ /*&& 0==displaydevice_->get_chars_per_column()*/ ) /* note: need to destroy displaydevice_ after GUI-actioned window destroy */
			{
				// we display input as soon as it received.
				if(this->getGlobalIntVariable(FIM_VID_DISPLAY_CONSOLE))
					displaydevice_->fs_puts(displaydevice_->f_,0,0,(const fim_char_t*)cso);
			}
#endif /* FIM_WANT_MILDLY_VERBOSE_DUMB_CONSOLE */
			cs=dupstr(cso);

			if(!cs)
				goto rerr;
			nc=strlen(cs);
			if(!nc)
				goto rerr;
			nl=lines_count(cs,lwidth_);
			if(lwidth_<1)
				goto rerr;
			nl=lines_count(cs,lwidth_);
			// we count exactly the number of new entries needed in the arrays we have
			if((s=const_cast<fim_char_t*>(strchr(cs,'\n')))!=FIM_NULL && s!=cs)
				nl+=(ccol_+(s-cs-1))/lwidth_;// single line_ with \n or multiline
			else nl+=(strlen(cs)+ccol_)/lwidth_;	// single line_, with no terminators

			/*
			 * we check for room (please note that nl >= the effective new lines introduced , while
			 * nc amounts to the exact extra room needed )
			 * */
			if( ( nc+1+(bp_-buffer_) ) > bsize_ || nl+1+cline_ > lsize_ )
			{
				fim_free(cs);
				return FIM_ERR_BUFFER_FULL;//no room : realloc needed ; 1 is for secur1ty
			}
			scroll_=scroll_-nl<0?0:scroll_-nl;

			// we copy the whole new string in our buffer_
			strcpy(bp_,cs);
			fim_free(cs); cs=FIM_NULL;
			sanitize_string_from_nongraph_except_newline(bp_,0);
			s=bp_-ccol_;// we will work in our buffer_ space now on
			b=s;
			while(*s && (s=(fim_char_t*)next_row(s,lwidth_))!=FIM_NULL && *s)
			{
				line_[++cline_]=s;// we keep track of each new line_
				ccol_=0;
				bp_=s;
			}// !s || !*s
			if(!*s && s-b==lwidth_){line_[++cline_]=(bp_=s);}// we keep track of the last line_ too
			

			if(ol==cline_)
			{
				ccol_=strlen(line_[cline_]);	// we update the current (right after last) column
				bp_+=strlen(bp_);	// the buffer_ points to the current column
			}
			else
			{
				ccol_=strlen(bp_);	// we update the current (right after last) column
				bp_+=ccol_;	// the buffer_ points to the current column
			}
			return FIM_ERR_NO_ERROR;
rerr:
			fim_free(cs);
			return FIM_ERR_GENERIC;
		}

		fim_err_t MiniConsole::setRows(int nr)
		{
			/*
			 * We update the displayed rows_, if this is physically possible
			 * If nr is negative, no correctness checks will occur.
			 * ( useful if calling this routine with FIM_NULL displaydevice.. )
			 * */
			int maxrows;
			if(nr<0)
			{
				rows_=-nr;
				return FIM_ERR_NO_ERROR;
			}
			maxrows = displaydevice_->get_chars_per_column();
			if(nr>0 && nr<=maxrows)
			{
				rows_=nr;
				return FIM_ERR_NO_ERROR;
			}
			return FIM_ERR_GENERIC;
		}

		MiniConsole::MiniConsole(CommandConsole& gcc, DisplayDevice *dd,int lw, int r)
		:
		Namespace(&gcc),
		buffer_(FIM_NULL),
		line_(FIM_NULL),
		bp_(FIM_NULL),
		bsize_(0),
		lsize_(0),
		ccol_(0),
		cline_(0),
		lwidth_(0),
		rows_(0),
		scroll_(0),
		displaydevice_(dd)
		{
			const int BS=FIM_CONSOLE_BLOCKSIZE;	//block size of 1k

			bsize_ = BS * FIM_CONSOLE_DEF_WIDTH;
			lsize_ = BS *   8;

			lwidth_=lw<=0?FIM_CONSOLE_DEF_WIDTH:lw;
			rows_=r<=0?FIM_CONSOLE_DEF_ROWS:r;

			cline_ =0;
			ccol_  =0;
			buffer_=(fim_char_t*) fim_calloc(bsize_,sizeof(fim_char_t ));
			line_  =(fim_char_t**)fim_calloc(lsize_,sizeof(fim_char_t*));
			bp_    =buffer_;
			if(!buffer_ || !line_)
			{
				bsize_=0;
				lsize_=0;
				if(buffer_)
					fim_free(buffer_);
				if(line_  )
					fim_free(line_);
			}
			else
			{
				line_[cline_]=buffer_;
			}
			add(FIM_MSG_CONSOLE_FIRST_LINE_BANNER);
		}

#if FIM_WANT_OBSOLETE
		fim_err_t MiniConsole::do_dump(int amount)const
		{
			/*
			 * We dump:
			 * 	the last amount of lines if	amount >  0
			 * 	all the lines if		amount == 0
			 * 	the first ones if		amount <  0
			 * */
			if(amount > 0)
			{
				// dumps the last amount of lines
				amount=amount>cline_?cline_:amount;
				do_dump(cline_-amount+1,cline_);
			}
			else
			if(amount ==0)
			{
				// dumps all of the lines
				do_dump(0,cline_);
			}
			else
			if(amount < 0)
			{
				// dumps the first amount of lines
				if(-amount>=cline_)
					amount+=cline_;
				do_dump(0,-amount);
			}
			return FIM_ERR_NO_ERROR;
		}
#endif /* FIM_WANT_OBSOLETE */

		fim_err_t MiniConsole::grow_lines(int glines)
		{
			/*
			 * grow of a specified amount of lines the lines array
			 *
			 * see the doc for grow() to get more
			 * */
			/* TEST ME AND FINISH ME */
			if(glines< 0)
				return FIM_ERR_GENERIC;
			if(glines==0)
				return FIM_ERR_NO_ERROR;
			fim_char_t **p;
			p=line_;
			line_=(fim_char_t**)fim_realloc(line_,bsize_+glines*sizeof(fim_char_t*));
			if(!line_){line_=p;return FIM_ERR_GENERIC;/* no change */}
			lsize_+=glines;
			return FIM_ERR_NO_ERROR;
		}

		fim_err_t MiniConsole::grow_buffer(int gbuffer)
		{
			/*
			 * grow of a specified amount of lines the buffer_ array
			 *
			 * see the doc for grow() to get more
			 * */
			/* TEST ME AND FINISH ME */
			if(gbuffer< 0)
				return  FIM_ERR_GENERIC;
			if(gbuffer==0)
				return  FIM_ERR_NO_ERROR;
			fim_char_t *p;
			int i;
			ptrdiff_t d;
			p=buffer_;
			buffer_=(fim_char_t*)fim_realloc(buffer_,(bsize_+gbuffer)*sizeof(fim_char_t));
			if(!buffer_){buffer_=p;return FIM_ERR_GENERIC;/* no change */}
			if((d=(p-buffer_))!=0)// in the case a shift is needed
			{
				for(i=0;i<=cline_;++i)
					line_[i]-=d;
				bp_-=d;
			}
			bsize_+=gbuffer;
			return FIM_ERR_NO_ERROR;
		}

		fim_err_t MiniConsole::grow(void)
		{
			/*
			 * We grow a specified amount both the line_ count and the line_ buffer_.
			 * */
			return grow(FIM_CONSOLE_BLOCKSIZE,8*FIM_CONSOLE_BLOCKSIZE);
		}

		fim_err_t MiniConsole::grow(int glines, int gbuffer)
		{
			/*
			 * grow of a specified amount of lines or bytes the 
			 * current line_ and text buffers; i.e.: make room
			 * to support glines more lines and gbuffer more characters.
			 *
			 * grow values can be negative. in this case, the current 
			 * buffers will be shrunk of the specified amounts.
			 *
			 * consistency will be preserved by deleting a certain amount
			 * of lines: the older ones.
			 *
			 * a zero amount for a value imply the line_ or buffer_ arrays
			 * to remain untouched.
			 * */
			/* FINISH ME AND TEST ME */
			int gb,gl;
			gb=grow_buffer(gbuffer);
			gl=grow_lines (glines);
			return ( gb==FIM_ERR_NO_ERROR && FIM_ERR_NO_ERROR ==gl )?FIM_ERR_NO_ERROR:FIM_ERR_GENERIC;// 0 if both 0
		}

		fim_err_t MiniConsole::reformat(int newlwidth)
		{
			/*
			 * This member function reformats the whole buffer_ array; that is, it recomputes
			 * line_ information for it, thus updating the whole line_ array contents.
			 * It may fail, in the case a new line_ width (smaller) is desired, because
			 * more line_ information would be needed.
			 *
			 * If the new lines are longer than before, then it could not fail.
			 * Upon a successful execution, the width is updated.
			 * 
			 * */
			int nls;
			if(newlwidth==lwidth_)
				return FIM_ERR_NO_ERROR;//are we sure?
			if(newlwidth< lwidth_)
			{
				// realloc needed
				if ( ( nls=lines_count(buffer_, newlwidth) + 1 ) < lsize_ )
				if ( grow_lines( nls )!=FIM_ERR_NO_ERROR )
					return FIM_ERR_GENERIC;
			}
			if(newlwidth> lwidth_ || ( lines_count(buffer_, newlwidth) + 1 < lsize_ ) )
			{
				// no realloc, no risk
				fim::string buf=buffer_;
				if( buf.size() == (size_t)(bp_-buffer_) )
				{
					ccol_=0;cline_=0;lwidth_=newlwidth;*line_=buffer_;bp_=buffer_;
					// the easy way
					add(buf.c_str());// by adding a very big chunk of text, we make sure it gets formatted.
					return FIM_ERR_NO_ERROR;
				}
				// if some error happened in buf string initialization, we return -1
				return FIM_ERR_GENERIC;
			}
			return FIM_ERR_GENERIC;
		}

		fim_err_t MiniConsole::dump(void)
		{
			/*
			 * We dump on screen the textual console contents.
			 * We consider user set variables.
			 * */
			const fim_int co=getGlobalIntVariable(FIM_VID_CONSOLE_LINE_OFFSET);
			const fim_int lw=getGlobalIntVariable(FIM_VID_CONSOLE_LINE_WIDTH );
			const fim_int ls=getGlobalIntVariable(FIM_VID_CONSOLE_ROWS       );
			setGlobalVariable(FIM_VID_CONSOLE_BUFFER_TOTAL,bsize_);
			setGlobalVariable(FIM_VID_CONSOLE_BUFFER_FREE, (fim_int)(bsize_-(bp_-buffer_)));
			setGlobalVariable(FIM_VID_CONSOLE_BUFFER_USED, (fim_int)(bp_-buffer_));
			setGlobalVariable(FIM_VID_CONSOLE_BUFFER_LINES,cline_);
			// we possibly update internal variables now
			setRows(ls);
			if( lw > 0 && lw!=lwidth_ )
				reformat(lw);
			if(co>=0)
			{
				scroll_=scroll_%(rows_+1);
				if(scroll_>0)
					return do_dump((cline_-rows_+1-co)>=0?(cline_-(rows_-scroll_)+1-co):0,cline_-co);
				else
					return do_dump((cline_-rows_+1-co)>=0?(cline_-rows_+1-co):0,cline_-co);
			}
			else
				return do_dump(-co-1,cline_);
			return FIM_ERR_GENERIC;
		}

#if FIM_WANT_OBSOLETE
		fim_err_t MiniConsole::do_dump(void)const
		{
			/*
			 * We dump on screen the textual console contents.
			 * */
			return do_dump((cline_-rows_+1)>=0?(cline_-rows_+1):0,cline_);
		}
#endif /* FIM_WANT_OBSOLETE */

		fim_err_t MiniConsole::clear(void)
		{
			scroll_=rows_;
			return FIM_ERR_NO_ERROR;
		}

		fim_err_t MiniConsole::scroll_down(void)
		{
			scroll_=scroll_<1?0:scroll_-1;
			return FIM_ERR_NO_ERROR;
		}

		fim_err_t MiniConsole::scroll_up(void)
		{
			scroll_=scroll_<rows_?scroll_+1:scroll_;
			return FIM_ERR_NO_ERROR;
		}

		size_t MiniConsole::byte_size(void)const
		{
			size_t bs = 0;
			bs += bsize_ + lsize_;
			return bs;
		}
#if FIM_WANT_OBSOLETE
		fim_err_t MiniConsole::add(const fim_byte_t* cso){return add((const fim_char_t*)cso);}
#endif /* FIM_WANT_OBSOLETE */

	MiniConsole::~MiniConsole(void)
	{
		fim_free(line_);
		fim_free(buffer_);
#if FIM_WITH_DEBUG 
		{
			FIM_MEMDBG_EXT_DECLS
			if (g_allocs_n || g_allocs_bytes)
			{
				std::cout << " g_allocs_n: " << g_allocs_n << std::endl;
				std::cout << " g_allocs_bytes: " << g_allocs_bytes << std::endl;
				std::cout << " maxrss: " << fim_maxrss() << std::endl;
			}
			assert(g_allocs_n + g_allocs_bytes == 0);
		}
#endif
	}
}
#endif /* FIM_WANT_NO_OUTPUT_CONSOLE */
